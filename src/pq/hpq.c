/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "clchack.h"
#include "pq.h"
#include "hpqimpl.h"



#define PRIVATE			static

#ifndef NULL
#define NULL 0
#endif

UNUSED static const char RCSid[] = "$Id: hpq.c,v 1.12 2004/07/08 04:40:21 lindauer Exp $";
UNUSED static char const version[] = VERSION;


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
PRIVATE void restore_heap(register header_s *, int start);
PRIVATE void restore_heap_up(register header_s *, int start);
PRIVATE void restore_heap_down(register header_s *, int start);
int __hpq_verify(header_s *hp, int current);



/*
 * Create the priority queue
 *
 * THREADS: MT_SAFE
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

#ifdef BK_USING_PTHREADS
  hp->flags = flags;
  hp->iter_cnt = 0;
  hp->iter = NULL;

  if (pthread_mutex_init(&(hp->lock), NULL) != 0)
  {
    free( (char *)hp ) ;
    HANDLE_ERROR( (header_s *)NULL, NULL, PQ_ENOMEM,
		  "HPQ __hpq_create: malloc failed\n" ) ;
  }
#endif /* BK_USING_PTHREADS */

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
 *
 * THREADS: MT-SAFE
 */
void __hpq_destroy(pq_h handle)
{
  header_s *hp = HHP( handle ) ;

#ifdef BK_USING_PTHREADS
  if (hp->iter)
    free(hp->iter);
  pthread_mutex_destroy(&hp->lock);
#endif /* BK_USING_PTHREADS */

  free( (char *) hp->objects ) ;
  free( (char *)hp ) ;
}



/*
 * Insert a node onto the priority queue
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
int __hpq_insert(pq_h handle, pq_obj object)
{
  register header_s *hp = HHP( handle ) ;
  register unsigned i, parent ;
  int ret = PQ_ERR;

  if ( object == NULL )
    HANDLE_ERROR( hp, PQ_ERR, PQ_ENULLOBJECT,
		  "HPQ __hpq_insert: NULL object\n" ) ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  /*
   * Make sure there is room to store the object
   */
  if ( hp->cur_size >= hp->max_size && grow( hp ) == PQ_ERR )
    goto done;

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

  ret = PQ_OK;
 done:
#ifdef BK_USING_PTHREADS
  if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return(ret);
}



/*
 * Grow the table.
 * Algorithm:
 *		while the table_size is less than CUTOFF, double the size.
 *		if it grows greater than CUTOFF, increase the size by INCREMENT
 *		(these number are in entries, not bytes)
 *
 * THREADS: UNSAFE
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
  pq_obj object = NULL;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  if ( hp->cur_size == 0 )
    goto done;

  object = hp->objects[ 0 ] ;
  hp->objects[ 0 ] = hp->objects[ --hp->cur_size ] ;
  restore_heap_down( hp, 0 ) ;

 done:
#ifdef BK_USING_PTHREADS
  if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return( object ) ;
}



/*
 * Restore the heap when we don't know whether the damage will
 * require the change to be propagated upward or downward
 *
 * THREADS: UNSAFE
 */
PRIVATE void restore_heap(register header_s *hp, int start)
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
 *
 * THREADS: UNSAFE
 */
PRIVATE void restore_heap_down(register header_s *hp, int start)
{
  register int current = start ;
  register int better = current ;

  for ( ;; )
  {
    register int left = LEFT( current ) ;
    register int right = RIGHT( current ) ;

    /*
     * Meaning of variables:
     *
     *		current:		the current tree node
     *		left:			its left child
     *		right:			its right child
     *		better:		the best of current,left,right
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
 *
 * THREADS: UNSAFE
 */
PRIVATE void restore_heap_up(register header_s *hp, int start)
{
  register int current = start ;

  /* Check for trivial case */
  if ( !EXISTS( hp, start ) )
    return;

  for ( ;; )
  {
    register int parent = PARENT( current ) ;

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
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
int __hpq_delete(pq_h handle, register pq_obj object)
{
  register header_s *hp = HHP( handle ) ;
  register int i ;

  if ( object == NULL )
    HANDLE_ERROR( hp, PQ_ERR, PQ_ENULLOBJECT,
		  "HPQ __hpq_delete: NULL object\n" ) ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

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
      goto error;
  }

  hp->objects[ i ] = hp->objects[ --hp->cur_size ] ;
  restore_heap( hp, i ) ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return( PQ_OK ) ;

 error:

#ifdef BK_USING_PTHREADS
  if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  HANDLE_ERROR( hp, PQ_ERR, PQ_ENOTFOUND, "HPQ __hpq_delete: object not found\n" ) ;
}



/*
 * Iterate the tree (back to front)
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
pq_iter __hpq_iterate(pq_h handle)
{
  header_s *hp = HHP( handle ) ;
  pq_iter iter = NULL;
#ifdef BK_USING_PTHREADS
  int itercnt = 0;
#endif /* BK_USING_PTHREADS */

  if (!(iter = malloc(sizeof(*iter))))
    HANDLE_ERROR( hp, NULL, PQ_ENOMEM, "HPQ __hpq_iterate: Out of Memory\n" );
  *iter = hp->cur_size;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();

  for (itercnt=0; itercnt<hp->iter_cnt; itercnt++)
  {
    if (!hp->iter[itercnt])
    {
      hp->iter[itercnt] = iter;
      goto done;
    }
  }

  if (iter)
  {
    pq_iter *new;
    if (!(new = realloc(hp->iter, sizeof(int *)*(hp->iter_cnt+1))))
   {
      free(iter);
      iter = NULL;
      goto done;
    }
    hp->iter = new;
    hp->iter[hp->iter_cnt] = iter;
    hp->iter_cnt++;
  }

 done:
  if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return(iter);
}



/*
 * Return the elements, back to front
 *
 * N.B. Not too much protection (any?) against damage done to the
 * priority queue while you are iterating.
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
pq_obj __hpq_nextobj(pq_h handle, pq_iter iter)
{
  register header_s *hp = HHP( handle ) ;
  pq_obj obj = NULL;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  if ( *iter > hp->cur_size )
    *iter = hp->cur_size;

  if ( *iter == 0 )
    goto done;

  obj = hp->objects[*iter];

 done:

#ifdef BK_USING_PTHREADS
  if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return(obj);
}



/*
 * Clean up a no-longer-used iterator
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
void __hpq_iterate_done(pq_h handle, pq_iter iter)
{
#ifdef BK_USING_PTHREADS
  register header_s *hp = HHP( handle ) ;
  int			itercnt;

  if (iter)
  {
    if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
      abort();

    for (itercnt=0; itercnt<hp->iter_cnt; itercnt++)
    {
      if (hp->iter[itercnt] == iter)
      {
	hp->iter[itercnt] = NULL;
	break;
      }
    }

    if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
      abort();

    free(iter);
  }
#endif /* BK_USING_PTHREADS */

  return;
}



/*
 * unlisted recursive function to check the validity of a pq
 * Created for finding a deletion problem
 *
 * Returns the object count of the first node which is incorrect
 * (or -2 if the node in error is zero)
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
int __hpq_verify(header_s *hp, int current)
{
  register int left = LEFT( current ) ;
  register int right = RIGHT( current ) ;
  int ret = 0;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  if (hp->cur_size == 0 && current == 0)
    goto done;

  if (!EXISTS(hp, current))
  {
    ret = -1;
    goto done;
  }

  if (EXISTS(hp, right))
  {
    if (IS_BETTER(hp,right,current))
    {
      ret = current||-2;
      goto done;
    }
    if ((ret = __hpq_verify(hp,right)))
      goto done;
  }
  if (EXISTS(hp, left)
#ifndef CORRECT_TREE
      && left != current
#endif /* CORRECT_TREE */
      )
  {
    if (IS_BETTER(hp,left,current))
    {
      ret = current||-2;
      goto done;
    }
    if (ret = __hpq_verify(hp,left))
      goto done;
  }
 done:

#ifdef BK_USING_PTHREADS
  if ((hp->flags & PQ_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return(ret);
}



/**
 * Provide errno to name conversion
 */
struct name_value
{
  int nv_value ;
  char *nv_name ;
};



/**
 * Actually convert errnos to text strings
 */
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



/**
 * Function to decode error numbers to strings
 *
 * THREADS: MT-SAFE
 */
char *__hpq_error_reason(int dicterrno)
{
  int ctr;
  char *ret;

  for(ctr = 0; error_codes[ctr].nv_name && dicterrno != error_codes[ctr].nv_value; ctr++) ;

  if ((ret = error_codes[ctr].nv_name))
    return(ret);

  return("Unknown CLC error");
}



/**
 * Function to return error string and (optionally) error number
 * for the current error for this CLC
 *
 * THREADS: UNSAFE (since error in handle is not thread-private
 */
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
