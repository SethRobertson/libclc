/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */
static const char RCSid[] = "$Id: ht.c,v 1.16 2003/04/09 19:51:56 seth Exp $";

#ifdef DEBUG
#include <stdio.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "clchack.h"
#include "htimpl.h"


int ht_num_table_entries = DEFAULT_TABLE_ENTRIES;
int ht_num_bucket_entries = DEFAULT_BUCKET_ENTRIES;


#ifndef SWITCH_HT_TO_DLL

enum lookup_type { EMPTY, FULL } ;



#define CUR_MIN_PERHACK
#ifdef CUR_MIN_PERHACK

#define HASH( hp, func, arg, save )	( &(hp)->table[ (save) = ((*(func))( arg ) % (hp)->args.ht_table_entries) ] )
#define HASH_OBJECT( hp, obj, save )  HASH( hp, (hp)->args.ht_objvalue, obj, save )
#define HASH_KEY( hp, key, save )     HASH( hp, (hp)->args.ht_keyvalue, key, save )

static int junkptr = 0;

#else // CUR_MIN_PERHACK

#define HASH( hp, func, arg, save )	( &(hp)->table[ (*(func))( arg ) % (hp)->args.ht_table_entries ] )
#define HASH_OBJECT( hp, obj, save )  HASH( hp, (hp)->args.ht_objvalue, obj, save )
#define HASH_KEY( hp, key, save )     HASH( hp, (hp)->args.ht_keyvalue, key, save )

#endif // CUR_MIN_PERHACK


#define N_PRIMES		( sizeof( primes ) / sizeof( unsigned ) )


/*
 * Used for the selection of a good array size
 */
static unsigned primes[] = { 3, 5, 7, 11, 13, 17, 23, 29 } ;


/*
 * Pick a good number for hashing using the hint argument as a hint
 * on the order of magnitude.
 *
 * Algorithm:
 *	1. Find an odd number for testing numbers for primes. The starting
 *		point is 2**k-1 where k is selected such that
 *		2**k-1 <= hint < 2**(k+1)-1
 *	2. Check all odd numbers from the starting point on until you find
 *		one that is not a multiple of the selected primes.
 *
 * THREADS: MT-SAFE
 */ 
PRIVATE unsigned find_good_size(register unsigned int hint)
{
  register unsigned int		k ;
  unsigned			starting_point ;
  register unsigned		size ;


  /*
   * Find starting point
   *
   * XXX:	a hint that is too large ( 1 << ( WORD_SIZE - 1 ) ) will cause
   *			weird behavior
   */
  for ( k = 0 ; ; k++ )
    if ( hint < (unsigned int)( 1 << k ) - 1 )
      break ;
	
  starting_point  = ( 1 << (k-1) ) - 1 ;
	
  /*
   * XXX:	This may be slow, especially on machines without division
   *			hardware (for example, SPARC V[78] implementations).
   */
  for ( size = starting_point ;; size += 2 )
  {
    register unsigned int j ;

    for ( j = 0 ; j < sizeof( primes ) / sizeof( unsigned ) ; j++ )
      if ( size % primes[j] == 0 )
	goto next;
    return( size ) ;
  next: 
    continue; // Stupid insight complaints
  }
}



/*
 * Create a new hash table
 *
 * THREADS: MT-SAFE (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_h ht_create(dict_function oo_comp, dict_function ko_comp, int flags, struct ht_args *argsp)
{
  header_s			*hp ;
  int				allocator_flags ;
  unsigned			table_size, bucket_size ;
  char				*id = "ht_create" ;

  if ( !__dict_args_ok( id, flags, oo_comp, ko_comp, DICT_UNORDERED, &allocator_flags ) )
    return( NULL_HANDLE ) ;

  if ( argsp->ht_objvalue == NULL || argsp->ht_keyvalue == NULL )
    return( __dict_create_error( id, flags, DICT_ENOHVFUNC ) ) ;

  hp = HHP( malloc( sizeof( header_s ) ) ) ;
#ifdef DEBUG
  fprintf(stderr,"Mallocing hash hash header: %d\n", sizeof(header_s));
#endif /* DEBUG */
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
   * Allocate the hash table
   */
  if ( argsp->ht_table_entries == 0 )
    argsp->ht_table_entries = ht_num_table_entries ;
  else
  {
    if (!(flags & DICT_HT_STRICT_HINTS))
      argsp->ht_table_entries = find_good_size( argsp->ht_table_entries ) ;
  }

  table_size = argsp->ht_table_entries * sizeof( tabent_s ) ;
  hp->table = TEP( malloc( table_size ) ) ;

#ifdef DEBUG
  fprintf(stderr,"Mallocing hash table: %d\n", table_size);
#endif /* DEBUG */
  if ( hp->table == NULL )
  {
    free( hp ) ;
    return( __dict_create_error( id, flags, DICT_ENOMEM ) ) ;
  }

  /*
   * Determine the bucket size and create an allocator
   */
  if ( argsp->ht_bucket_entries == 0 )
    argsp->ht_bucket_entries = ht_num_bucket_entries ;
  bucket_size = sizeof( bucket_s ) +
    argsp->ht_bucket_entries * sizeof( dict_obj ) ;
	
  /*
   * XXX: can't use fast allocator if we set FSM_ZERO_ALLOC
   *      (we no longer do this for buckets of size 1)
   *
   * XXX: should be able to give some indication to FSMA about the
   *		  slots/chunk; currently we use the default.
   */
  allocator_flags |= argsp->ht_bucket_entries == 1 ? 0 : FSM_ZERO_ALLOC ;
  hp->alloc = fsm_create( bucket_size, 0, allocator_flags ) ;
  if ( hp->alloc == NULL )
  {
    free( hp ) ;
    free( hp->table ) ;
    return( __dict_create_error( id, flags, DICT_ENOMEM ) ) ;
  }

  __dict_init_header( DHP( hp ), oo_comp, ko_comp, flags ) ;

  /*
   * Clear the table
   */
  (void) memset( hp->table, 0, (int) table_size ) ;

  hp->args = *argsp ;

#ifdef BK_USING_PTHREADS
  hp->iter_cnt = 0;
  hp->iter = NULL;
#endif /* BK_USING_PTHREADS */

  /*
   * Set cur_min to zero. This is a little bogus since 0 is a valid
   * slot (but not the valid minimum) but the first insert will fix
   * this up.  Anyway, ht_minimum will special case no objects being
   * in the hash table.
   */
  hp->cur_min = 0;
  hp->obj_cnt = 0;
  return( (dict_h) hp ) ;
}



/*
 * Destroy a hash table and contents
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
void ht_destroy(dict_h handle)
{
  header_s		*hp = HHP( handle ) ;

#ifdef COALESCE
  bucket_s *bp,*np;
  unsigned int i;

  for ( i = 0 ; i < hp->args.ht_table_entries ; i++ )
  {
    tabent_s *tep = &hp->table[i] ;
    if (ENTRY_HAS_CHAIN(tep))
    {
      for(bp=tep->head_bucket;bp;bp=np)
      {
	np = bp->next;
	fsm_free(hp->alloc, bp);
      }
    }
  }
#endif /* COALESCE */

#ifdef BK_USING_PTHREADS
  if (hp->iter)
    free(hp->iter);
  pthread_mutex_destroy(&hp->lock);
#endif /* BK_USING_PTHREADS */

  fsm_destroy( hp->alloc ) ;
  free( hp->table ) ;
  free( hp ) ;
}



/*
 * Bucket chain reverse lookup:
 * 	Return a pointer to the last dict_obj of the bucket chain that
 * 	starts with bp (search up to the bucket 'stop' but *not* that bucket)
 *
 * THREADS: UNSAFE
 */
PRIVATE dict_obj *bc_reverse_lookup(bucket_s *bp, unsigned int entries, bucket_s *stop)
{
  dict_obj		*result ;
  dict_obj		*bucket_list ;
  int			j ;

  if ( bp == stop )
    return( NULL ) ;

  result = bc_reverse_lookup( bp->next, entries, stop ) ;
  if ( result )
    return( result ) ;

  bucket_list = BUCKET_OBJECTS( bp ) ;

  for ( j = entries-1 ; j >= 0 ; j-- )
    if ( bucket_list[ j ] != NULL )
      return( &bucket_list[ j ] ) ;
  return( NULL ) ;
}



/*
 * Bucket chain lookup:
 *	Return a pointer to the first NULL (if type is EMPTY) or non-NULL
 *	(if type is FULL) dict_obj in the bucket chain.
 *
 * THREADS: UNSAFE
 */
PRIVATE dict_obj *bc_lookup(bucket_s *start, unsigned int entries, enum lookup_type type)
{
  register bucket_s	*bp ;
  register int		look_for_empty = ( type == EMPTY ) ;

  for ( bp = start ; bp != NULL ; bp = bp->next )
  {
    unsigned int j ;
    dict_obj *bucket_list = BUCKET_OBJECTS( bp ) ;

    for ( j = 0 ; j < entries ; j++ )
      if ( ( bucket_list[j] == NULL ) == look_for_empty )
      {
	return( &bucket_list[j] ) ;
      }
  }
  return( NULL ) ;
}



/*
 * Search the bucket chain for the specified object
 * Returns the pointer of the bucket where the object was found.
 *
 * THREADS: UNSAFE
 */
PRIVATE bucket_s *bc_search(bucket_s *chain, unsigned int entries, dict_obj object, int *idx)
{
  bucket_s		*bp ;

  for ( bp = chain ; bp ; bp = bp->next )
  {
    dict_obj *bucket_list = BUCKET_OBJECTS( bp ) ;
    unsigned int i ;

    for ( i = 0 ; i < entries ; i++ )
      if ( bucket_list[ i ] == object )
      {
	if ( idx )
	  *idx = i ;
	return( bp ) ;
      }
  }
  return( NULL ) ;
}



/*
 * Add a bucket to the chain of the specified table entry.
 * Returns a pointer to the first slot of the new bucket or NULL on failure.
 *
 * THREADS: UNSAFE
 */
PRIVATE dict_obj *te_expand(tabent_s *tep, header_s *hp)
{
  dheader_s	*dhp = DHP( hp ) ;
  bucket_s     	*bp ;

  bp = (bucket_s *) fsm_alloc( hp->alloc ) ;
  if ( bp == NULL )
    HANDLE_ERROR( dhp, DICT_ENOMEM, NULL ) ;

  if ( !( hp->alloc->flags & FSM_ZERO_ALLOC ) )
    *BUCKET_OBJECTS( bp ) = NULL; /* zero out single entry */

  /*
   * Put the bucket at the head of this entry's chain
   */
  bp->next = tep->head_bucket ;
  tep->head_bucket = bp ;

  /*
   * Update entry info
   */
  tep->n_free += hp->args.ht_bucket_entries ;
  tep->n_free_max += hp->args.ht_bucket_entries ;
  return( BUCKET_OBJECTS( bp ) ) ;
}



/*
 * Search a table entry for an object
 *
 * THREADS: UNSAFE
 */
PRIVATE dict_obj *te_search(tabent_s *tep, header_s *hp, search_e type, dict_h arg)
{
  dheader_s	*dhp = DHP( hp ) ;
  bucket_s     	*bp ;

  for ( bp = tep->head_bucket ; bp != NULL ; bp = bp->next )
  {
    unsigned int i ;
    int result ;
    dict_obj *bucket_list = BUCKET_OBJECTS( bp ) ;

    for ( i = 0 ; i < hp->args.ht_bucket_entries ; i++ )
      if ( bucket_list[ i ] != NULL )
      {
	result = ( type == KEY_SEARCH )
	  ? (*dhp->ko_comp)( (dict_key)arg, bucket_list[ i ] )
	  : (*dhp->oo_comp)( (dict_obj)arg, bucket_list[ i ] ) ;
	if ( result == 0 )
	  return( &bucket_list[ i ] ) ;
      }
  }
  return( NULL ) ;
}



/*
 * Internal common routine for performing inserts
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
PRIVATE int ht_do_insert(header_s *hp, int uniq, register dict_obj object, dict_obj *objectp)
{
  dheader_s		*dhp = DHP( hp ) ;
  tabent_s		*tep ;
  dict_obj		*object_slot ;
  unsigned int		min_index = 0;
  int			errret;

  if ( object == NULL )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, DICT_ERR ) ;
	
#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  tep = HASH_OBJECT( hp, object, min_index ) ;

  /*
   * We search the entry chain only if it exists and uniqueness is required.
   */
  if ( ENTRY_HAS_CHAIN( tep ) && uniq )
  {
    object_slot = te_search( tep, hp, OBJECT_SEARCH, (dict_h) object ) ;
    if ( object_slot != NULL )
    {
      if ( objectp != NULL )
	*objectp = *object_slot ;
      errret = DICT_EEXISTS ;
      goto error;
    }
  }

  /*
   * If the entry chain is full, expand it
   */
  if ( ENTRY_IS_FULL( tep ) )
  {
    object_slot = te_expand( tep, hp ) ;
    if ( object_slot == NULL )
    {
      errret = ERRNO( dhp );
      goto error;
    }
  }
  else
    object_slot = bc_lookup( tep->head_bucket, hp->args.ht_bucket_entries, EMPTY ) ;
  tep->n_free-- ;

#ifdef CUR_MIN_PERHACK
  if (min_index < hp->cur_min || hp->obj_cnt == 0)
    hp->cur_min = min_index;
#endif // CUR_MIN_PERHACK

  hp->obj_cnt++;

  *object_slot = object ;
  if ( objectp != NULL )
    *objectp = *object_slot ;

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
 * Insert an object normally
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
int ht_insert(dict_h handle, dict_obj object)
{
  header_s		*hp = HHP( handle ) ;

  return( ht_do_insert( hp,
			hp->dh.flags & DICT_UNIQUE_KEYS, object, (dict_obj *)NULL ) ) ;
}



/*
 * Insert an object, with duplicate return
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
int ht_insert_uniq(dict_h handle, dict_obj object, dict_obj *objectp)
{
  header_s    *hp = HHP( handle ) ;
  dheader_s	*dhp = DHP( hp ) ;

  if ( dhp->oo_comp == NULL_FUNC )
    HANDLE_ERROR( dhp, DICT_ENOOOCOMP, DICT_ERR ) ;
  return( ht_do_insert( hp, TRUE, object, objectp ) ) ;
}



/*
 * Delete an object
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
int ht_delete(dict_h handle, dict_obj object)
{
  header_s		*hp = HHP( handle ) ;
  dheader_s		*dhp = DHP( hp ) ;
  tabent_s		*tep ;
  int			bucket_index ;
  bucket_s		*bp ;
  int			errret;

  if ( object == NULL )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, DICT_ERR ) ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  tep = HASH_OBJECT( hp, object, junkptr ) ;
  if ( ! ENTRY_HAS_CHAIN( tep ) || ENTRY_IS_EMPTY( tep) )
  {
    errret = DICT_ENOTFOUND;
    goto error;
  }

  if (!(bp = bc_search( tep->head_bucket, hp->args.ht_bucket_entries, object, &bucket_index )))
  {
    errret = DICT_ENOTFOUND;
    goto error;
  }

  BUCKET_OBJECTS( bp )[ bucket_index ] = NULL ;
  tep->n_free++ ;

  hp->obj_cnt--;

#ifdef CUR_MIN_PERHACK
  // Note that cur_min may be too low, but we perform lazy evaluation on the minimum
  if (!hp->obj_cnt)
    hp->cur_min = 0;
#endif // CUR_MIN_PERHACK

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

  HANDLE_ERROR(dhp, errret, DICT_ERR);
}



/*
 * Find an object by key
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_obj ht_search(dict_h handle, dict_key key)
{
  header_s		*hp	= HHP( handle ) ;
  tabent_s		*tep;
  dict_obj		*objp;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  tep = HASH_KEY( hp, key, junkptr ) ;
  objp = te_search( tep, hp, KEY_SEARCH, (dict_h) key ) ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return( ( objp == NULL ) ? NULL_OBJ : *objp ) ;
}



/*
 * Find the lowest object
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_obj ht_minimum(dict_h handle)
{
  header_s		*hp		= HHP( handle ) ;
  unsigned		bucket_entries	= hp->args.ht_bucket_entries ;
  unsigned int		i ;
  dict_obj		obj = NULL_OBJ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  if (hp->obj_cnt)
  {
    for ( i = hp->cur_min ; i < hp->args.ht_table_entries ; i++ )
    {
      tabent_s *tep = &hp->table[i] ;
      dict_obj *found ;

      if ( ! ENTRY_HAS_CHAIN( tep ) || ENTRY_IS_EMPTY(tep) )
	continue ;
      found = bc_lookup( tep->head_bucket, bucket_entries, FULL ) ;
      if ( found )
      {
#ifdef CUR_MIN_PERHACK
	hp->cur_min = i;
#endif // CUR_MIN_PERHACK

	obj = *found;
	goto done;
      }
    }
    // We should never get here, but whatever...
    hp->cur_min = 0;
  }

 done:
#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return( obj ) ;
}



/*
 * Find the highest node
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_obj ht_maximum(dict_h handle)
{
  header_s		*hp		= HHP( handle ) ;
  unsigned		bucket_entries	= hp->args.ht_bucket_entries ;
  dict_obj obj = NULL_OBJ;
  int i ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  for ( i = hp->args.ht_table_entries-1 ; i >= 0 ; i-- )
  {
    tabent_s *tep = &hp->table[i] ;
    dict_obj *found ;

    if ( ! ENTRY_HAS_CHAIN( tep ) || ENTRY_IS_EMPTY( tep) )
      continue ;

    found = bc_reverse_lookup( tep->head_bucket,
			       bucket_entries, BUCKET_NULL ) ;
    if ( found )
    {
      obj = *found;
      break;
    }
  }
#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return( obj ) ;
}



/*
 * Find the node after the one provided
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_obj ht_successor(dict_h handle, dict_obj object)
{
  header_s		*hp		= HHP( handle ) ;
  dheader_s		*dhp		= DHP( hp ) ;
  tabent_s		*table_end	= &hp->table[ hp->args.ht_table_entries ] ;
  unsigned		bucket_entries	= hp->args.ht_bucket_entries ;
  tabent_s		*tep ;
  bucket_s		*bp		= NULL ;
  int			bucket_index ;
  unsigned int		i ;
  int			errret;
  dict_obj		ret = NULL_OBJ;

  if ( object == NULL )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, NULL_OBJ ) ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  tep = HASH_OBJECT( hp, object, junkptr ) ;
  if ( ! ENTRY_HAS_CHAIN( tep ) ||
       ( bp = bc_search( tep->head_bucket,
			 bucket_entries, object, &bucket_index ) ) == NULL )
  {
    errret = DICT_EBADOBJECT;
    goto error;
  }

  ERRNO( dhp ) = DICT_ENOERROR ;

  for ( i = bucket_index+1 ; i < bucket_entries ; i++ )
    if ( BUCKET_OBJECTS( bp )[ i ] != NULL )
    {
      ret = BUCKET_OBJECTS( bp )[ i ];
      goto done;
    }

  for ( bp = bp->next ;; )
  {
    dict_obj *found = bc_lookup( bp, bucket_entries, FULL ) ;

    if ( found )
    {
      ret = *found;
      goto done;
    }
    tep++ ;
    if ( tep >= table_end )
      goto done;
    bp = tep->head_bucket ;
  }

 done:
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
  HANDLE_ERROR( dhp, errret, NULL_OBJ ) ;
}



/*
 * Find the predecessor to the listed node
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_obj ht_predecessor(dict_h handle, dict_obj object)
{
  header_s		*hp		= HHP( handle ) ;
  dheader_s		*dhp		= DHP( hp ) ;
  unsigned		bucket_entries	= hp->args.ht_bucket_entries ;
  tabent_s		*tep ;
  bucket_s		*stop ;
  dict_obj		*found ;
  int			bucket_index ;
  int			i ;
  int			errret;
  dict_obj		ret = NULL_OBJ;

  if ( object == NULL )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, NULL_OBJ ) ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  tep = HASH_OBJECT( hp, object, junkptr ) ;
  stop = bc_search( tep->head_bucket, bucket_entries, object, &bucket_index ) ;
  if ( stop == NULL )
  {
    errret = DICT_EBADOBJECT;
    goto error;
  }
	
  ERRNO( dhp ) = DICT_ENOERROR ;

  for ( i = bucket_index-1 ; i >= 0 ; i-- )
    if ( BUCKET_OBJECTS( stop )[ i ] != NULL )
    {
      ret = BUCKET_OBJECTS( stop )[ i ];
      break;
    }
	
  for ( ;; )
  {
    found = bc_reverse_lookup( tep->head_bucket, bucket_entries, stop ) ;
    if ( found )
    {
      ret = *found;
      goto done;
    }
    stop = NULL ;
    if ( tep <= hp->table )
      goto done;
    tep-- ;
  }

 done:
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
  HANDLE_ERROR( dhp, errret, NULL_OBJ ) ;
}



/*
 * Sets the iterator to the beginning of the next used entry.
 * The current table entry *is* included in the search.
 *
 * THREADS: MT-SAFE
 */
PRIVATE void iter_next(header_s *hp, struct ht_iter *ip)
{
  register unsigned int	i ;

  for ( i = ip->current_table_entry ; i < hp->args.ht_table_entries ; i++ )
    if ( ENTRY_HAS_CHAIN( &hp->table[i] ) )
    {
      ip->current_bucket = hp->table[i].head_bucket ;
      ip->next_bucket_offset = 0 ;
      break ;
    }
  ip->current_table_entry = i ;
}


/*
 * We don't make any use of 'direction'
 *
 * THREADS: MT-SAFE
 */
dict_iter ht_iterate(dict_h handle, enum dict_direction direction)
{
  header_s		*hp = HHP( handle ) ;
#ifdef BK_USING_PTHREADS
  dheader_s		*dhp		= DHP( hp ) ;
  struct ht_iter	*iter;
  int itercnt;
#else /* BK_USING_PTHREADS */
  struct ht_iter	*iter	= &hp->iter ;
#endif /* BK_USING_PTHREADS */

#ifdef BK_USING_PTHREADS
  if (!(iter = malloc(sizeof(*iter))))
    HANDLE_ERROR( dhp, DICT_ENOMEM, NULL ) ;
#endif /* BK_USING_PTHREADS */

  iter->current_table_entry = 0 ;
  iter_next( hp, iter ) ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
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
    struct ht_iter **new;
    if (!(new = realloc(hp->iter, sizeof(struct tree_iterator *)*(hp->iter_cnt+1))))
    {
      free(iter);
      goto error;
    }
    hp->iter = new;
    hp->iter[hp->iter_cnt] = iter;
    hp->iter_cnt++;
  }

 done:
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  return(iter);

#ifdef BK_USING_PTHREADS
 error:
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();

  HANDLE_ERROR( dhp, DICT_ENOMEM, NULL_OBJ ) ;
#endif /* BK_USING_PTHREADS */
}



/*
 * Done with iterator
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
void ht_iterate_done(dict_h handle, dict_iter iter)
{
#ifdef BK_USING_PTHREADS
  header_s		*hp = HHP( handle ) ;
  int			itercnt;

  if (iter)
  {
    if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
      abort();

    for (itercnt=0; itercnt<hp->iter_cnt; itercnt++)
    {
      if (hp->iter[itercnt] == iter)
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
 * As for next object in iterator
 *
 * THREADS: THREAD-REENTRANT (assuming DICT_NOCOALESCE or DICT_THREADED_*)
 */
dict_obj ht_nextobj(dict_h handle, dict_iter iter)
{
  header_s		*hp = HHP( handle ) ;
  struct ht_iter	*ip = iter;
  unsigned int		i ;
  dict_obj		obj = NULL_OBJ;

#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_lock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */

  while ( ip->current_table_entry < hp->args.ht_table_entries )
  {
    do
    {
      for ( i = ip->next_bucket_offset ;
	    i < hp->args.ht_bucket_entries ; i++ )
      {
	dict_obj *bucket_list = BUCKET_OBJECTS( ip->current_bucket ) ;

	if ( bucket_list[i] != NULL )
	{
	  ip->next_bucket_offset = i+1 ;
	  obj = bucket_list[i];
	  goto done;
	}
      }
      ip->current_bucket = ip->current_bucket->next ;
      ip->next_bucket_offset = 0;
    }
    while ( ip->current_bucket ) ;

    ip->current_table_entry++ ;
    iter_next( hp, ip ) ;
  }

 done:
#ifdef BK_USING_PTHREADS
  if ((hp->flags & DICT_THREADED_SAFE) && pthread_mutex_unlock(&hp->lock) != 0)
    abort();
#endif /* BK_USING_PTHREADS */
  return( obj ) ;
}



/*
 * Return reason for most recent error
 *
 * THREADS: UNSAFE
 */
char *ht_error_reason(dict_h handle, int *errnop)
{
  header_s	*hp		= HHP( handle ) ;
  int		dicterrno;

  if (handle)
    dicterrno = ERRNO(DHP(hp));
  else
    dicterrno = dict_errno;

  if (errnop) *errnop = dicterrno;

  return(__dict_error_reason(dicterrno));
}



#else  /* SWITCH_HT_TO_DLL */

#include "dll.h"

/*
 * <TODO>This is probably obsolete and unnecessary; if Insure is working ok,
 * this can be eliminated.</TODO>
 *
 * Hash tables confuse Insight and cause it to get fatal internal errors,
 * especially when destroying hash tables (probably a bug in the leak-detection
 * code dealing with that array full of pointers to allocated memory).  This
 * makes fully Insight-compiled programs pretty much unusable.  So we replace
 * all of the hash table functions with their equivalent DLL function.  This is
 * a lot slower of course, but just running with Insight turned on slows
 * everything tremendously anyhow, so who cares?  It's more important not to
 * crash.
 */

dict_h ht_create(dict_function oo_comp, dict_function ko_comp, int flags, int *errnop, struct ht_args *argsp)
{
  argsp = argsp;
  return dll_create( oo_comp, ko_comp, flags);
}

void ht_destroy(dict_h handle)
{
  dll_destroy( handle );
}

int ht_insert(dict_h handle, dict_obj object)
{
  return dll_insert( handle, object );
}

int ht_insert_uniq(dict_h handle, dict_obj object, dict_obj *objectp)
{
  return dll_insert_uniq( handle, object, objectp );
}

int ht_delete(dict_h handle, dict_obj object)
{
  return dll_delete( handle, object );
}

dict_obj ht_search(dict_h handle, dict_key key)
{
  return dll_search( handle, key );
}

dict_obj ht_minimum(dict_h handle)
{
  return dll_minimum( handle );
}

dict_obj ht_maximum(dict_h handle)
{
  return dll_maximum( handle );
}

dict_obj ht_successor(dict_h handle, dict_obj object)
{
  return dll_successor( handle, object );
}

dict_obj ht_predecessor(dict_h handle, dict_obj object)
{
  return dll_predecessor( handle, object );
}

dict_iter ht_iterate(dict_h handle, enum dict_direction direction)
{
  return dll_iterate( handle, direction );
}

void ht_iterate_done(dict_h handle, dict_iter iter)
{
  dll_iterate_done(handle, iter);
}

dict_obj ht_nextobj(dict_h handle, dict_iter iter)
{
  return dll_nextobj( handle, iter );
}

char *ht_error_reason(dict_h handle, int *errnop)
{
  return dll_error_reason( handle, errnop );
}

#endif /* SWITCH_HT_TO_DLL */
