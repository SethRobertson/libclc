/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

static const char RCSid[] = "$Id: hpq.c,v 1.7 2002/07/18 22:52:49 dupuy Exp $";
static char const version[] = VERSION;

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "pq.h"
#include "hpqimpl.h"
#include "clchack.h"



#define PRIVATE			static

#ifndef NULL
#define NULL 0
#endif


/*
 * Do we want to have a totally correct tree or have a funnel
 * where the left child of zero is itself and the right child
 * is one.  (From then on it is a normal tree, there is just a
 * one element trunk on top).
 */
#ifdef CORRECT_TREE
unsigned PARENTTMP;		/* Required for macro so PARENT(root) == root */
#define PARENT( i )		( (PARENTTMP = ( ((i)+1) >> 1 )) ? PARENTTMP - 1 : PARENTTMP )
#define LEFT( i )		(( ((i)+1) << 1 ) - 1)
#define RIGHT( i )		( LEFT( i ) + 1 )
#else /* CORRECT_TREE */
#define PARENT( i )		( i >> 1 )
#define LEFT( i )		( i << 1 )
#define RIGHT( i )		( LEFT( i ) + 1 )
#endif /* CORRECT_TREE */


#define INITIAL_ARRAY_SIZE	100		/* entries */
#define CUTOFF			(INITIAL_ARRAY_SIZE * 128)
#define INCREMENT		CUTOFF
#define EXISTS( hp, i )		( i < hp->cur_size )
#define IS_BETTER( hp, i, j )	( (*hp->is_better)( hp->objects[ i ], hp->objects[ j ] ) )
#define SWAP( hp, i, j )					\
		do{						\
		  pq_obj t = hp->objects[ i ] ;		\
		  hp->objects[ i ] = hp->objects[ j ] ;	\
		  hp->objects[ j ] = t ;			\
		}while(0)



int pq_errno ;


PRIVATE int grow(header_s *);
PRIVATE void restore_heap(register header_s *, unsigned start);
PRIVATE void restore_heap_up(register header_s *, unsigned start);
PRIVATE void restore_heap_down(register header_s *, unsigned start);
int __hpq_verify(header_s *hp, unsigned int current);



/*
 * Create the priority queue
 */
pq_h __hpq_create(int (*func)(pq_obj, pq_obj), int flags)
{
  register header_s *hp ;

  /*
   * Check if the user has provided the necessary comparison functions
   */
  if ( func == NULL )
    HANDLE_ERROR( (header_s *)NULL, NULL, PQ_ENOFUNC,
		  "HPQ __hpq_create: missing object-object comparator\n" ) ;

  hp = HHP( malloc( sizeof( header_s ) ) ) ;
  if ( hp == NULL )
    HANDLE_ERROR( (header_s *)NULL, NULL, PQ_ENOMEM,
		  "HPQ __hpq_create: malloc failed\n" ) ;
	
  /*
   * Allocate object array
   */
  hp->objects = (pq_obj *) malloc( INITIAL_ARRAY_SIZE * sizeof( pq_obj ) ) ;
  if ( hp->objects == NULL )
  {
    free( hp ) ;
    HANDLE_ERROR( (header_s *)NULL, NULL, PQ_ENOMEM, 
		  "HPQ __hpq_create: malloc failed\n" ) ;
  }

  /*
   * Initialize the header
   */
  hp->is_better = func ;
  hp->dicterrno = 0 ;
  hp->flags = flags ;
  hp->max_size = INITIAL_ARRAY_SIZE ;
  hp->cur_size = 0 ;
  return( (pq_h) hp ) ;
}



/*
 * Destroy the priority queue and the object table (but not the objects)
 */
void __hpq_destroy(pq_h handle)
{
  header_s *hp = HHP( handle ) ;

  free( (char *) hp->objects ) ;
  free( (char *)hp ) ;
}



/*
 * Insert a node onto the priority queue
 */
int __hpq_insert(pq_h handle, pq_obj object)
{
  register header_s *hp = HHP( handle ) ;
  register unsigned i, parent ;

  if ( object == NULL )
    HANDLE_ERROR( hp, PQ_ERR, PQ_ENULLOBJECT,
		  "HPQ __hpq_insert: NULL object\n" ) ;

  /*
   * Make sure there is room to store the object
   */
  if ( hp->cur_size >= hp->max_size && grow( hp ) == PQ_ERR )
    return( PQ_ERR ) ;

  /*
   * Put the node on the end and trickle it up the priority queue
   * until it reaches the correct place.
   * 
   * Should we really call trickle_up?  This version is more efficient
   * since you do not copy in the new object until you know where it
   * is going.
   */
  i = hp->cur_size++ ;

#ifndef WANT_TRICKLEUP
  parent = PARENT( i ) ;
  while ( i > 0 && (*hp->is_better)( object, hp->objects[ parent ] ) )
  {
    hp->objects[ i ] = hp->objects[ parent ] ;
    i = parent ;
    parent = PARENT( i ) ;
  }
#endif /* !WANT_TRICKLEUP */

  hp->objects[ i ] = object ;

#ifdef WANT_TRICKLEUP
  restore_heap_up(hp,i);
#endif /* WANT_TRICKLEUP */


  return( PQ_OK ) ;
}



/*
 * Grow the table.
 * Algorithm:
 *		while the table_size is less than CUTOFF, double the size.
 * 		if it grows greater than CUTOFF, increase the size by INCREMENT
 *		(these number are in entries, not bytes)
 */
PRIVATE int grow(header_s *hp)
{
  unsigned new_size ;
  char *new_objects ;

  if ( hp->max_size < CUTOFF )
    new_size = hp->max_size * 2 ;
  else
    new_size = hp->max_size + INCREMENT ;

  new_objects = (char *)realloc( (char *)hp->objects, new_size * sizeof( pq_obj ) ) ;
  if ( new_objects == NULL )
    HANDLE_ERROR( hp, PQ_ERR, PQ_ENOMEM,
		  "HPQ grow: out of memory\n" ) ;
	
  hp->max_size = new_size ;
  hp->objects = (pq_obj *) new_objects ;
  return( PQ_OK ) ;
}



/*
 * Take the head node off the top of the priority queue
 *
 * Move the last object onto the root and trickle down
 */
pq_obj __hpq_extract_head(pq_h handle)
{
  register header_s *hp = HHP( handle ) ;
  pq_obj object ;

  if ( hp->cur_size == 0 )
    return( NULL ) ;
	
  object = hp->objects[ 0 ] ;
  hp->objects[ 0 ] = hp->objects[ --hp->cur_size ] ;
  restore_heap_down( hp, 0 ) ;
  return( object ) ;
}



/*
 * Restore the heap when we don't know whether the damage will
 * require the change to be propagated upward or downward
 */
PRIVATE void restore_heap(register header_s *hp, unsigned int start)
{
  register unsigned parent = PARENT( start ) ;

  /* Check for trivial case */
  if ( !EXISTS( hp, start ) )
    return;

  if ( IS_BETTER( hp, start, parent ) )
    restore_heap_up(hp,start);
  else
    restore_heap_down(hp,start);
}



/*
 * Trickle an object down through the tree until it is back in
 * correct order
 */
PRIVATE void restore_heap_down(register header_s *hp, unsigned int start)
{
  register unsigned current = start ;
  register unsigned better = current ;

  for ( ;; )
  {
    register unsigned left = LEFT( current ) ;
    register unsigned right = RIGHT( current ) ;

    /*
     * Meaning of variables:
     *
     *		current:		the current tree node
     *		left:			its left child
     *		right:			its right child
     *		better: 		the best of current,left,right
     *
     * We start the loop with better == current
     *
     * The code takes advantage of the fact that the existence of
     * the right child implies the existence of the left child.
     * It works by finding the better of the two children (and puts
     * that in better) and comparing that against current.
     */
    if ( EXISTS( hp, right ) )
      better = IS_BETTER( hp, left, right ) ? left : right ;
    else if ( EXISTS( hp, left ) )
      better = left ;

    if ( better == current || IS_BETTER( hp, current, better ) )
      break ;
    else 
    {
      SWAP( hp, current, better ) ;
      current = better ;
    }
  }
}



/*
 * Restore the heap to correctness, propagating the current node up
 * the tree until it reaches the correct place.
 */
PRIVATE void restore_heap_up(register header_s *hp, unsigned int start)
{
  register unsigned current = start ;

  /* Check for trivial case */
  if ( !EXISTS( hp, start ) )
    return;

  for ( ;; )
  {
    register unsigned parent = PARENT( current ) ;

    if ( IS_BETTER( hp, current, parent ) )
    {
      if (current == parent)
      {
	/* Stupid moron programmer -- the same node cannot be better than itself! */
	break;
      }
      SWAP( hp, current, parent ) ;
      current = parent ;
    }
    else
    {
      /* We have risen to our level of incompetence :-) */
      break ;
    }
  }
}



/*
 * Delete an arbitrary node
 *
 * Move the last node into its place and fix up the tree
 */
int __hpq_delete(pq_h handle, register pq_obj object)
{
  register header_s *hp = HHP( handle ) ;
  register unsigned i ;

  if ( object == NULL )
    HANDLE_ERROR( hp, PQ_ERR, PQ_ENULLOBJECT,
		  "HPQ __hpq_delete: NULL object\n" ) ;

  /*
   * First find it
   */
  for ( i = 0 ;; i++ )
  {
    if ( i < hp->cur_size )
      if ( object == hp->objects[ i ] )
	break ;
      else
	continue ;
    else
      HANDLE_ERROR( hp, PQ_ERR, PQ_ENOTFOUND,
		    "HPQ __hpq_delete: object not found\n" ) ;
  }

  hp->objects[ i ] = hp->objects[ --hp->cur_size ] ;
  restore_heap( hp, i ) ;
  return( PQ_OK ) ;
}



/*
 * Iterate the tree (back to front)
 */
void __hpq_iterate(pq_h handle)
{
  header_s *hp = HHP( handle ) ;

  hp->iter = hp->cur_size;
}



/*
 * Return the elements, back to front
 *
 * N.B. Not too much protection (any?) against damage done to the
 * priority queue in *front* of the current node, and you certainly
 * can have damage of one form or another.
 */
pq_obj __hpq_nextobj(pq_h handle)
{
  register header_s *hp = HHP( handle ) ;

  if ( hp->iter > hp->cur_size )
    hp->iter = hp->cur_size;

  if ( hp->iter == 0 )
    return( NULL ) ;
	
  return( hp->objects[ --hp->iter ] ) ;
}



/*
 * unlisted recursive function to check the validity of a pq
 * Created for finding a deletion problem
 *
 * Returns the object count of the first node which is incorrect
 * (or -2 if the node in error is zero)
 */
int __hpq_verify(header_s *hp, unsigned int current)
{
  register unsigned left = LEFT( current ) ;
  register unsigned right = RIGHT( current ) ;
  int ret;

  if (hp->cur_size == 0 && current == 0)
    return(0);

  if (!EXISTS(hp, current))
    return(-1);

  if (EXISTS(hp, right))
  {
    if (IS_BETTER(hp,right,current))
    {
      return(current||-2);
    }
    if ((ret = __hpq_verify(hp,right)))
      return(ret);
  }
  if (EXISTS(hp, left)
#ifndef CORRECT_TREE
      && left != current
#endif /* CORRECT_TREE */
      )
  {
    if (IS_BETTER(hp,left,current))
    {
      return(current||-2);
    }
    if ((ret = __hpq_verify(hp,left)))
      return(ret);
  }
  return(0);
}



struct name_value
{
  int nv_value ;
  char *nv_name ;
} ;


static struct name_value error_codes[] =
{
  {	PQ_ENOFUNC,		"User Supplied Comparison Function Missing"	},
  {	PQ_ENOMEM,		"Out of Memory"					},
  {	PQ_ENULLOBJECT,		"Cannot Insert a NULL Object"			},
  {	PQ_ENOTFOUND,		"Cannot Find Object"				},
  {	PQ_ENOTSUPPORTED,	"Not Supported"					},
  {	PQ_ENOERROR,		"No error"					},
  {	-1,			NULL						}
} ;



char *__hpq_error_reason(int dicterrno)
{
  int ctr;
  char *ret;

  for(ctr = 0; error_codes[ctr].nv_name && dicterrno != error_codes[ctr].nv_value; ctr++) ;

  if ((ret = error_codes[ctr].nv_name))
    return(ret);

  return("Unknown CLC error");
}



char *pq_error_reason(pq_h handle, int *errnop)
{
  header_s *hp = HHP( handle ) ;
  int dicterrno;

  if (hp)
    dicterrno = hp->dicterrno;
  else
    dicterrno = pq_errno;

  if (errnop) *errnop = dicterrno;
  return(__hpq_error_reason(dicterrno));
}
