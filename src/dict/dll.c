/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

#include <stdlib.h>
#include "clchack.h"
#include "dllimpl.h"

UNUSED static const char RCSid[] = "$Id: dll.c,v 1.16 2004/07/08 04:40:20 lindauer Exp $";



/*
 * Create and initialize the DLL header
 *
 * THREADS: MT_SAFE (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_h dll_create(dict_function oo_comp, dict_function ko_comp, int flags)
{
  header_s			*hp ;
  char				*id = "dll_create" ;
  int				 fsma_flags = 0;

  if (!(flags & (DICT_ORDERED|DICT_UNORDERED)))
    flags |= DICT_ORDERED;			// Ordered by default

  if ( ! __dict_args_ok( id, flags, oo_comp, ko_comp, DICT_ORDERED | DICT_UNORDERED, &fsma_flags ) )
    return( NULL_HANDLE ) ;

  hp = (header_s *) malloc( sizeof( header_s ) ) ;
  if ( hp == NULL )
    return( __dict_create_error( id, flags, DICT_ENOMEM ) ) ;

#ifdef BK_USING_PTHREADS
  hp->flags = flags;

  if (pthread_mutex_init(&(hp->lock), NULL) != 0)
  {
    free( (char *)hp ) ;
    return( __dict_create_error( id, flags, DICT_ENOMEM ) ) ;
  }
#endif /* BK_USING_PTHREADS */

  /*
   * Create an allocator
   */
  hp->alloc = fsm_create( sizeof( node_s ), 0, fsma_flags);
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

#ifdef BK_USING_PTHREADS
  hp->iter_cnt = 0;
  hp->iter = NULL;
#else /* BK_USING_PTHREADS */
#if defined SAFE_ITERATE || defined FAST_ACTIONS
  /* Make sure iteration stuff is initialized too. */
  dll_iterate((dict_h)hp, DICT_FROM_START);
#endif /* SAFE_ITERATE || FAST_ACTIONS */
#endif /* BK_USING_PTHREADS */

  return( (dict_h) hp ) ;
}



/*
 * Destroy a dll and contents
 *
 * THREADS: MT_SAFE (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
void dll_destroy(dict_h handle)
{
  header_s	*hp	= LHP( handle ) ;
#ifdef COALESCE
  dheader_s	*dhp	= DHP( hp ) ;
  node_s *x = NULL;
  node_s *y = NULL;

  if (!(dhp->flags & DICT_NOCOALESCE))
  {
    NEXT(PREV(hp->head)) = NULL;		/* Break dll loop */
    for (x=hp->head; x; x=y)
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

#ifdef BK_USING_PTHREADS
  if (hp->iter)
    free(hp->iter);
  pthread_mutex_destroy(&hp->lock);
#endif /* BK_USING_PTHREADS */

  fsm_destroy( hp->alloc ) ;
  free( (char *)hp ) ;
}



/*
 * Internal common routine which actually performs insert
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
PRIVATE int dll_do_insert(register header_s *hp, bool_int must_be_uniq, register dict_obj object, dict_obj *objectp, int flags)
{
  register dheader_s	*dhp = DHP( hp ) ;
  register bool_int	unordered_list = ( dhp->flags & DICT_UNORDERED ) ;
  register node_s	*np = NULL ;
  node_s		*newnode ;
  node_s		*before, *after ;
  int			errret ;

  if ( object == NULL )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, DICT_ERR ) ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

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
	  errret = DICT_EEXISTS ;
	  goto error;
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
  {
    errret = DICT_ENOMEM;
    goto error;
  }

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

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return( DICT_OK ) ;

 error:
#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  HANDLE_ERROR( dhp, errret, DICT_ERR ) ;
}



/*
 * Normal insert routine
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
int dll_insert(dict_h handle, dict_obj object)
{
  header_s		*hp = LHP( handle ) ;

  return( dll_do_insert( hp, hp->dh.flags & DICT_UNIQUE_KEYS,
			 object, (dict_obj *)NULL,
			 DLL_PREPEND ) ) ;
}



/*
 * Insert with duplicate return
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
int dll_insert_uniq(dict_h handle, dict_obj object, dict_obj *objectp)
{
  header_s		*hp	= LHP( handle ) ;
  dheader_s	*dhp	= DHP( hp ) ;

  if ( dhp->oo_comp == NULL_FUNC )
    HANDLE_ERROR( dhp, DICT_ENOOOCOMP, DICT_ERR ) ;
  return( dll_do_insert( hp, TRUE, object, objectp, DLL_PREPEND ) ) ;
}



/*
 * Insert where current object is postpended to dups
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
int dll_append(dict_h handle, dict_obj object)
{
  header_s		*hp = LHP( handle ) ;

  return( dll_do_insert( hp, hp->dh.flags & DICT_UNIQUE_KEYS,
			 object, (dict_obj *)NULL,
			 DLL_POSTPEND ) ) ;
}



/*
 * For API completeness.  Same as insert_uniq()
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
int dll_append_uniq(dict_h handle, dict_obj object, dict_obj *objectp)
{
  header_s		*hp	= LHP( handle ) ;
  dheader_s	*dhp	= DHP( hp ) ;

  if ( dhp->oo_comp == NULL_FUNC )
    HANDLE_ERROR( dhp, DICT_ENOOOCOMP, DICT_ERR ) ;
  return( dll_do_insert( hp, TRUE, object, objectp, DLL_POSTPEND ) ) ;
}



/*
 * Delete a node, given the object
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
int dll_delete(dict_h handle, register dict_obj object)
{
  register header_s	*hp	= LHP( handle ) ;
  dheader_s		*dhp	= DHP( hp ) ;
  register node_s	*np=NULL ;
  node_s		*after, *before ;
#ifdef BK_USING_PTHREADS
  int			itercnt;
  node_s		*tmp = NULL;
#else /* BK_USING_PTHREADS */
#ifdef SAFE_ITERATE
  struct dll_iterator	*dip		= &LHP( handle )->iter ;
#endif
#endif /* BK_USING_PTHREADS */

  if ( object == NULL )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, DICT_ERR ) ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

#ifdef FAST_ACTIONS
#ifdef BK_USING_PTHREADS
  /* See if the interator contains a hint as to where obj might be */
  for (itercnt=0; itercnt<hp->iter_cnt; itercnt++)
  {
    if (hp->iter[itercnt] && hp->iter[itercnt]->next && hp->iter[itercnt]->next != hp->head)
    {
      if (hp->iter[itercnt]->direction == DICT_FROM_START && (tmp = PREV(hp->iter[itercnt]->next)) && OBJ(tmp) == object)
      {
	np = tmp;
	break;
      }
      if (hp->iter[itercnt]->direction == DICT_FROM_END && (tmp = NEXT(hp->iter[itercnt]->next)) && OBJ(tmp) == object)
      {
	np = tmp;
	break;
      }
    }
  }
  if (np)
    ; /* Intentionally blank */
#else /* BK_USING_PTHREADS */
  if ( dip->next && PREV(dip->next) && (OBJ( PREV(dip->next))==object) )
    np = PREV(dip->next);
#endif /* BK_USING_PTHREADS */
  else if ( OBJ( hp->hint.last_predecessor ) == object )
    np = hp->hint.last_predecessor;
  else if ( OBJ( hp->hint.last_successor ) == object )
    np = hp->hint.last_successor;
  else
#endif /* FAST_ACTIONS */
    if ( OBJ( hp->hint.last_search ) == object )
      np = hp->hint.last_search ;
    else
    {
      for ( np = NEXT( hp->head ) ;; np = NEXT( np ) )
	if ( np == hp->head )
	{
	  goto error;
	}
	else if ( object == OBJ( np ) )
	  break ;
    }

#ifdef SAFE_ITERATE
#ifdef BK_USING_PTHREADS
  // See if the iterator is pointing to the dying node
  for (itercnt=0; itercnt<hp->iter_cnt; itercnt++)
  {
    if (hp->iter[itercnt] && hp->iter[itercnt]->next == np)
    {
      if ( hp->iter[itercnt]->direction == DICT_FROM_START )
	hp->iter[itercnt]->next = NEXT( hp->iter[itercnt]->next ) ;
      else
	hp->iter[itercnt]->next = PREV( hp->iter[itercnt]->next ) ;
    }
  }
#else /* BK_USING_PTHREADS */
  if (dip->next && (OBJ(dip->next) == object) )
  {
    dll_nextobj(handle, (dict_iter)dip);
  }
#endif /* BK_USING_PTHREADS */
#endif /* SAFE_ITERATE */

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

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return( DICT_OK ) ;

 error:
#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  HANDLE_ERROR( dhp, DICT_ENOTFOUND, DICT_ERR ) ;
}



/*
 * Try to find the node for a key
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_obj dll_search(dict_h handle, register dict_key key)
{
  register header_s	*hp		= LHP( handle ) ;
  register dheader_s	*dhp		= DHP( hp ) ;
  register bool_int	unordered_list	= ( dhp->flags & DICT_UNORDERED ) ;
  register node_s	*np;
  dict_obj		ret = NULL;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  for ( np = NEXT( hp->head ) ; np != hp->head ; np = NEXT( np ) )
  {
    register int v = (*dhp->ko_comp)( key, OBJ( np ) ) ;

    if ( v == 0 )
    {
      hp->hint.last_search = np ;		/* update search hint */
      ret = OBJ(np);
      break;
    }
    else if ( v < 0 && ! unordered_list )
      break ;
  }

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return(ret);
}



/*
 * Returns a pointer to the object with the smallest key value or
 * NULL if the list is empty.
 *
 * NOTE: here we depend on the fact that OBJ( head ) == NULL
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_obj dll_minimum(dict_h handle)
{
  header_s		*hp = LHP( handle ) ;
  node_s		*np = NEXT( hp->head ) ;
  dict_obj		ret;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  hp->hint.last_successor = np ;			/* update hint */

  ret = OBJ( np );

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */
  return( ret );
}


/*
 * Returns a pointer to the object with the greatest key value or
 * NULL if the list is empty.
 *
 * NOTE: here we depend on the fact that OBJ( head ) == NULL
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_obj dll_maximum(dict_h handle)
{
  header_s		*hp = LHP( handle ) ;
  node_s		*np = PREV( hp->head ) ;
  dict_obj		ret;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  hp->hint.last_predecessor = np ;			/* update hint */
  ret = OBJ( np );

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return(ret);
}


/*
 * Returns a pointer to the object with the next >= key value or
 * NULL if the list is empty or the given object is the last one on the
 * list.
 *
 * NOTE: here we depend on the fact that OBJ( head ) == NULL
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_obj dll_successor(dict_h handle, register dict_obj object)
{
  register header_s	*hp	= LHP( handle ) ;
  dheader_s		*dhp	= DHP( hp ) ;
  register node_s	*np ;
  node_s		*successor ;
  dict_obj		ret;

  if ( object == NULL )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, NULL_OBJ ) ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  if ( OBJ( hp->hint.last_successor ) == object )
    successor = NEXT( hp->hint.last_successor ) ;
  else
  {
    ERRNO( dhp ) = DICT_ENOERROR ;
    for ( np = NEXT( hp->head ) ; np != hp->head ; np = NEXT( np ) )
      if ( OBJ( np ) == object )
	break ;
    if ( np == hp->head )
      goto error;
    successor = NEXT( np ) ;
  }
  hp->hint.last_successor = successor ;
  ret = OBJ( successor );

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return(ret);

 error:
#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  HANDLE_ERROR( dhp, DICT_EBADOBJECT, NULL_OBJ ) ;
}



/*
 * Returns a pointer to the object with the next <= key value or
 * NULL if the list is empty or the given object is the first one on the
 * list.
 *
 * NOTE: here we depend on the fact that OBJ( head ) == NULL
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_obj dll_predecessor(dict_h handle, register dict_obj object)
{
  register header_s	*hp	= LHP( handle ) ;
  dheader_s		*dhp	= DHP( hp ) ;
  node_s		*predecessor ;
  register node_s	*np ;
  dict_obj		ret;

  if ( object == NULL )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, NULL_OBJ ) ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  if ( OBJ( hp->hint.last_predecessor ) == object )
    predecessor = PREV( hp->hint.last_predecessor ) ;
  else
  {
    ERRNO( dhp ) = DICT_ENOERROR ;
    for ( np = PREV( hp->head ) ; np != hp->head ; np = PREV( np ) )
      if ( OBJ( np ) == object )
	break ;
    if ( np == hp->head )
      goto error;
    predecessor = PREV( np ) ;
  }
  hp->hint.last_predecessor = predecessor ;
  ret = OBJ( predecessor );

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return( ret );

 error:
#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  HANDLE_ERROR( dhp, DICT_EBADOBJECT, NULL_OBJ ) ;
}



/*
 * Start iterating from start/end of list
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_iter dll_iterate(dict_h handle, enum dict_direction direction)
{
  register header_s	*hp	= LHP( handle ) ;
#ifdef BK_USING_PTHREADS
  dheader_s		*dhp	= DHP( hp ) ;
  struct dll_iterator	*dip;
  int itercnt;
#else /* BK_USING_PTHREADS */
  struct dll_iterator	*dip	= &hp->iter ;
#endif /* BK_USING_PTHREADS */

#ifdef BK_USING_PTHREADS
  if (!(dip = malloc(sizeof(*dip))))
    HANDLE_ERROR( dhp, DICT_ENOMEM, NULL_OBJ ) ;
#endif /* BK_USING_PTHREADS */

#ifdef UNORDERED_LISTS_HAVE_NO_ORDER
  if ( dhp->flags & DICT_UNORDERED )
    dip->direction = DICT_FROM_START ;
  else
#endif /*UNORDERED_LISTS_HAVE_NO_ORDER*/
    dip->direction = direction ;

  if ( dip->direction == DICT_FROM_START )
    dip->next = NEXT( hp->head ) ;
  else
    dip->next = PREV( hp->head ) ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();

  for (itercnt=0; itercnt<hp->iter_cnt; itercnt++)
  {
    if (!hp->iter[itercnt])
    {
      hp->iter[itercnt] = dip;
      goto done;
    }
  }

  if (dip)
  {
    struct dll_iterator **new;
    if (!(new = realloc(hp->iter, sizeof(struct tree_iterator *)*(hp->iter_cnt+1))))
    {
      free(dip);
      goto error;
    }
    hp->iter = new;
    hp->iter[hp->iter_cnt] = dip;
    hp->iter_cnt++;
  }

 done:
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return((struct dict_iter *)dip);

#ifdef BK_USING_PTHREADS
 error:
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();

  HANDLE_ERROR( dhp, DICT_ENOMEM, NULL_OBJ ) ;
#endif /* BK_USING_PTHREADS */
}



/*
 * Clean up a no-longer-used iterator
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
void dll_iterate_done(dict_h handle, dict_iter iter)
{
#ifdef BK_USING_PTHREADS
  register header_s	*hp	= LHP( handle ) ;
  int			itercnt;

  if (iter)
  {
    if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
      abort();

    for (itercnt=0; itercnt<hp->iter_cnt; itercnt++)
    {
      if (hp->iter[itercnt] == (struct dll_iterator *)iter)
      {
	hp->iter[itercnt] = NULL;
	break;
      }
    }

    if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
      abort();

    free(iter);
  }
#endif /* BK_USING_PTHREADS */

  return;
}



/*
 * Iterate to the next object
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_obj dll_nextobj(dict_h handle, dict_iter iter)
{
#ifdef BK_USING_PTHREADS
  register header_s	*hp	= LHP( handle ) ;
#endif /* BK_USING_PTHREADS */
  struct dll_iterator	*dip		= (struct dll_iterator *)iter ;
  node_s		*current;
  dict_obj		ret;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  current = dip->next;

  if ( dip->direction == DICT_FROM_START )
    dip->next = NEXT( current ) ;
  else
    dip->next = PREV( current ) ;

  ret = OBJ( current );

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return(ret);
}



/*
 * Return the error of the reason?
 *
 * THREADS: UNSAFE
 */
char *dll_error_reason(dict_h handle, int *errnop)
{
  header_s	*hp		= LHP( handle ) ;
  int		dicterrno;

  if (handle)
    dicterrno = ERRNO(DHP(hp));
  else
    dicterrno = dict_errno;

  if (errnop) *errnop = dicterrno;

  return(dict_error_reason(dicterrno));
}
