/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

static char RCSid[] = "$Id: dll.c,v 1.6 2002/02/22 07:21:56 dupuy Exp $" ;

#include <stdlib.h>
#include "dllimpl.h"
#include "clchack.h"

dict_h dll_create(dict_function oo_comp, dict_function ko_comp, int flags)
{
  header_s			*hp ;
  char				*id = "dll_create" ;

  if ( ! __dict_args_ok( id, flags, oo_comp, ko_comp, DICT_ORDERED + DICT_UNORDERED ) )
    return( NULL_HANDLE ) ;

  hp = (header_s *) malloc( sizeof( header_s ) ) ;
  if ( hp == NULL )
    return( __dict_create_error( id, flags, DICT_ENOMEM ) ) ;
	
  /*
   * Create an allocator
   */
  hp->alloc = fsm_create( sizeof( node_s ), 0,
			  ( flags & DICT_NOCOALESCE ) ? FSM_NOCOALESCE : FSM_NOFLAGS ) ;
  if ( hp->alloc == NULL )
  {
    free( (char *)hp ) ;
    return( __dict_create_error( id, flags, DICT_ENOMEM ) ) ;
  }
	
  /*
   * Allocate and initialize head node
   */
  hp->head = (node_s *) fsm_alloc( hp->alloc ) ;
  if ( hp->head == NULL )
  {
    fsm_destroy( hp->alloc ) ;
    free( (char *)hp ) ;
    return( __dict_create_error( id, flags, DICT_ENOMEM ) ) ;
  }

  NEXT( hp->head ) = PREV( hp->head ) = hp->head ;
  OBJ( hp->head ) = NULL ;

  /*
   * Initialize dictionary header, hints
   */
  __dict_init_header( DHP( hp ), oo_comp, ko_comp, flags ) ;
  HINT_CLEAR( hp, last_search ) ;
  HINT_CLEAR( hp, last_successor ) ;
  HINT_CLEAR( hp, last_predecessor ) ;

#if defined SAFE_ITERATE || defined FAST_ACTIONS
  /* Make sure iteration stuff is initialized too. */
  dll_iterate(hp, DICT_FROM_START);
#endif

  return( (dict_h) hp ) ;
}



void dll_destroy(dict_h handle)
{
  header_s	*hp	= LHP( handle ) ;
  dheader_s	*dhp	= DHP( hp ) ;

#ifdef COALESCE
  node_s *x, *y;

  if (!(dhp->flags & DICT_NOCOALESCE))
  {
    NEXT(PREV(hp->head)) = NULL;		/* Break dll loop */
    for(x=hp->head;x;x=y)
    {
      y = NEXT(x);
      fsm_free(hp->alloc, x);
    }
  }
  else
#endif /* COALESCE */
  {
    /* Necessary during fsma debugging */
    fsm_free(hp->alloc, hp->head);
  }

  fsm_destroy( hp->alloc ) ;
  free( (char *)hp ) ;
}



PRIVATE int dll_do_insert(register header_s *hp, bool_int must_be_uniq, register dict_obj object, dict_obj *objectp, int flags)
{
  register dheader_s	*dhp = DHP( hp ) ;
  register bool_int 	unordered_list = ( dhp->flags & DICT_UNORDERED ) ;
  register node_s	*np = NULL ;
  node_s		*newnode ;
  node_s		*before, *after ;

  if ( object == NULL )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, DICT_ERR ) ;

  if (!unordered_list || must_be_uniq)
  {
    /*
     * Find node n such that key(OBJ(n)) >= key(object)
     */
    for ( np = ( flags & DLL_POSTPEND ) ? PREV( hp->head) : NEXT( hp->head ) ; np != hp->head ; np = ( flags & DLL_POSTPEND ) ? PREV( np ) : NEXT( np ) )
    {
      register int v = (*dhp->oo_comp)( OBJ( np ), object ) ;

      if (!unordered_list)
      {
	if (flags & DLL_POSTPEND)
	{
	  if (v < 0)
	    break;
	}
	else
	{
	  if (v > 0)
	    break;
	}
      }

      if ( v == 0 )
      {
	if ( must_be_uniq )
	{
	  if ( objectp != NULL )
	    *objectp = OBJ( np ) ;
	  ERRNO( dhp ) = DICT_EEXISTS ;
	  return( DICT_ERR ) ;
	}
	else
	  break ;
      }
    }
  }
  if ( unordered_list)
  {
    if ( flags & DLL_POSTPEND )
      np = PREV(hp->head) ;
    else
      np = NEXT(hp->head) ;
  }

  newnode = (node_s *) fsm_alloc( hp->alloc ) ;
  if ( newnode == NULL )
    HANDLE_ERROR( dhp, DICT_ENOMEM, DICT_ERR ) ;

  if (flags & DLL_POSTPEND)
  {
    /*
     * The new node is inserted AFTER np
     */
    before = np ;
    after = NEXT(np) ;
  }
  else
  {
    /*
     * The new node is inserted BEFORE np
     */
    before = PREV( np ) ;
    after = np ;
  }
  NEXT( newnode ) = after ;
  PREV( newnode ) = before ;
  NEXT( before ) = newnode ;
  PREV( after ) = newnode ;
  OBJ( newnode ) = object ;
  if ( objectp != NULL )
    *objectp = object ;
  return( DICT_OK ) ;
}


int dll_insert(dict_h handle, dict_obj object)
{
  header_s		*hp = LHP( handle ) ;

  return( dll_do_insert( hp, hp->dh.flags & DICT_UNIQUE_KEYS,
			 object, (dict_obj *)NULL,
			 DLL_PREPEND ) ) ;
}



int dll_insert_uniq(dict_h handle, dict_obj object, dict_obj *objectp)
{
  header_s		*hp	= LHP( handle ) ;
  dheader_s	*dhp	= DHP( hp ) ;

  if ( dhp->oo_comp == NULL_FUNC )
    HANDLE_ERROR( dhp, DICT_ENOOOCOMP, DICT_ERR ) ;
  return( dll_do_insert( hp, TRUE, object, objectp, DLL_PREPEND ) ) ;
}


int dll_append(dict_h handle, dict_obj object)
{
  header_s		*hp = LHP( handle ) ;

  return( dll_do_insert( hp, hp->dh.flags & DICT_UNIQUE_KEYS,
			 object, (dict_obj *)NULL,
			 DLL_POSTPEND ) ) ;
}



int dll_append_uniq(dict_h handle, dict_obj object, dict_obj *objectp)
{
  header_s		*hp	= LHP( handle ) ;
  dheader_s	*dhp	= DHP( hp ) ;

  if ( dhp->oo_comp == NULL_FUNC )
    HANDLE_ERROR( dhp, DICT_ENOOOCOMP, DICT_ERR ) ;
  return( dll_do_insert( hp, TRUE, object, objectp, DLL_POSTPEND ) ) ;
}


int dll_delete(dict_h handle, register dict_obj object)
{
  register header_s	*hp	= LHP( handle ) ;
  dheader_s		*dhp	= DHP( hp ) ;
  register node_s	*np=NULL ;
  node_s		*after, *before ;
#ifdef SAFE_ITERATE
  struct dll_iterator	*dip		= &LHP( handle )->iter ;
#endif


  if ( object == NULL )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, DICT_ERR ) ;

#ifdef SAFE_ITERATE	
  if (dip->next && (OBJ(dip->next) == object) )
  {
    dll_nextobj(handle, dip);
  }
#endif

#ifdef FAST_ACTIONS
  if ( dip->next && PREV(dip->next) && (OBJ( PREV(dip->next))==object) )
    np = PREV(dip->next);
  if ( OBJ( hp->hint.last_predecessor ) == object )
    np = hp->hint.last_predecessor;
  else if ( OBJ( hp->hint.last_successor ) == object )
    np = hp->hint.last_successor;
  else
#endif /* FAST_ACTIONS */
    if ( OBJ( hp->hint.last_search ) == object )
      np = hp->hint.last_search ;
    else
      for ( np = NEXT( hp->head ) ;; np = NEXT( np ) )
	if ( np == hp->head )
	{
	  ERRNO( dhp ) = DICT_ENOTFOUND ;
	  return( DICT_ERR ) ;
	}
	else if ( object == OBJ( np ) )
	  break ;

  /*
   * First disconnect, then release
   */
  after = NEXT( np ) ;
  before = PREV( np ) ;
  NEXT( before ) = after ;
  PREV( after ) = before ;
  OBJ( np ) = NULL ;
  fsm_free( hp->alloc, (char *)np ) ;

  /*
   * Clear all hints
   */
  HINT_CLEAR( hp, last_search ) ;
  HINT_CLEAR( hp, last_successor ) ;
  HINT_CLEAR( hp, last_predecessor ) ;

  return( DICT_OK ) ;
}


dict_obj dll_search(dict_h handle, register dict_key key)
{
  register header_s	*hp		= LHP( handle ) ;
  register dheader_s	*dhp		= DHP( hp ) ;
  register bool_int	unordered_list	= ( dhp->flags & DICT_UNORDERED ) ;
  register node_s	*np ;

  for ( np = NEXT( hp->head ) ; np != hp->head ; np = NEXT( np ) )
  {
    register int v = (*dhp->ko_comp)( key, OBJ( np ) ) ;

    if ( v == 0 )
    {
      hp->hint.last_search = np ;		/* update search hint */
      return( OBJ( np ) ) ;
    }
    else if ( v < 0 && ! unordered_list )
      break ;
  }
  return( NULL_OBJ ) ;
}


/*
 * Returns a pointer to the object with the smallest key value or
 * NULL if the list is empty.
 *
 * NOTE: here we depend on the fact that OBJ( head ) == NULL
 */
dict_obj dll_minimum(dict_h handle)
{
  header_s		*hp = LHP( handle ) ;
  node_s		*np = NEXT( hp->head ) ;

  hp->hint.last_successor = np ;			/* update hint */
  return( OBJ( np ) ) ;
}


/*
 * Returns a pointer to the object with the greatest key value or
 * NULL if the list is empty.
 *
 * NOTE: here we depend on the fact that OBJ( head ) == NULL
 */
dict_obj dll_maximum(dict_h handle)
{
  header_s		*hp = LHP( handle ) ;
  node_s		*np = PREV( hp->head ) ;

  hp->hint.last_predecessor = np ;			/* update hint */
  return( OBJ( np ) ) ;
}


/*
 * Returns a pointer to the object with the next >= key value or
 * NULL if the list is empty or the given object is the last one on the
 * list.
 *
 * NOTE: here we depend on the fact that OBJ( head ) == NULL
 */
dict_obj dll_successor(dict_h handle, register dict_obj object)
{
  register header_s	*hp	= LHP( handle ) ;
  dheader_s		*dhp	= DHP( hp ) ;
  register node_s	*np ;
  node_s		*successor ;

  if ( object == NULL )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, NULL_OBJ ) ;

  if ( OBJ( hp->hint.last_successor ) == object )
    successor = NEXT( hp->hint.last_successor ) ;
  else
  {
    ERRNO( dhp ) = DICT_ENOERROR ;
    for ( np = NEXT( hp->head ) ; np != hp->head ; np = NEXT( np ) )
      if ( OBJ( np ) == object )
	break ;
    if ( np == hp->head )
      HANDLE_ERROR( dhp, DICT_EBADOBJECT, NULL_OBJ ) ;
    successor = NEXT( np ) ;
  }
  hp->hint.last_successor = successor ;
  return( OBJ( successor ) ) ;
}



/*
 * Returns a pointer to the object with the next <= key value or
 * NULL if the list is empty or the given object is the first one on the
 * list.
 *
 * NOTE: here we depend on the fact that OBJ( head ) == NULL
 */
dict_obj dll_predecessor(dict_h handle, register dict_obj object)
{
  register header_s	*hp	= LHP( handle ) ;
  dheader_s		*dhp	= DHP( hp ) ;
  node_s		*predecessor ;
  register node_s	*np ;

  if ( object == NULL )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, NULL_OBJ ) ;

  if ( OBJ( hp->hint.last_predecessor ) == object )
    predecessor = PREV( hp->hint.last_predecessor ) ;
  else
  {
    ERRNO( dhp ) = DICT_ENOERROR ;
    for ( np = PREV( hp->head ) ; np != hp->head ; np = PREV( np ) )
      if ( OBJ( np ) == object )
	break ;
    if ( np == hp->head )
      HANDLE_ERROR( dhp, DICT_EBADOBJECT, NULL_OBJ ) ;
    predecessor = PREV( np ) ;
  }
  hp->hint.last_predecessor = predecessor ;
  return( OBJ( predecessor ) ) ;
}


dict_iter dll_iterate(dict_h handle, enum dict_direction direction)
{
  register header_s	*hp	= LHP( handle ) ;
  dheader_s		*dhp	= DHP( hp ) ;
  struct dll_iterator	*dip	= &hp->iter ;

  /* TODO -- create dynamically allocated iterator */

  if ( dhp->flags & DICT_UNORDERED )
    dip->direction = DICT_FROM_START ;
  else
    dip->direction = direction ;

  if ( dip->direction == DICT_FROM_START )
    dip->next = NEXT( hp->head ) ;
  else
    dip->next = PREV( hp->head ) ;

  return(dip);
}


void dll_iterate_done(dict_h handle, dict_iter iter)
{
  /* TODO -- delete dynamically allocated iterator */

  return;
}


dict_obj dll_nextobj(dict_h handle, dict_iter iter)
{
  struct dll_iterator	*dip		= iter ;
  node_s		*current	= dip->next ;

  if ( dip->direction == DICT_FROM_START )
    dip->next = NEXT( current ) ;
  else
    dip->next = PREV( current ) ;
  return( OBJ( current ) ) ;
}




char *dll_error_reason(dict_h handle, int *errnop)
{
  header_s	*hp		= LHP( handle ) ;
  int		dicterrno;

  if (handle)
    dicterrno = ERRNO(DHP(hp));
  else
    dicterrno = dict_errno;

  if (errnop) *errnop = dicterrno;

  return(__dict_error_reason(dicterrno));
}
