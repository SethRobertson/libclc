/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */
static char RCSid[] = "$Id: ht.c,v 1.6 2001/11/05 19:31:45 seth Exp $" ;

#ifdef DEBUG
#include <stdio.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "htimpl.h"
#include "clchack.h"


#ifdef __INSIGHT__
#define SWITCH_HT_TO_DLL
#endif /* __INSIGHT__ */


int ht_num_table_entries = DEFAULT_TABLE_ENTRIES;
int ht_num_bucket_entries = DEFAULT_BUCKET_ENTRIES;


#ifndef SWITCH_HT_TO_DLL

enum lookup_type { EMPTY, FULL } ;



#define HASH( hp, func, arg )	( &(hp)->table[ (*(func))( arg ) % (hp)->args.ht_table_entries ] )
#define HASH_OBJECT( hp, obj )  HASH( hp, (hp)->args.ht_objvalue, obj )
#define HASH_KEY( hp, key )     HASH( hp, (hp)->args.ht_keyvalue, key )


#define N_PRIMES							( sizeof( primes ) / sizeof( unsigned ) )


/*
 * Used for the selection of a good array size
 */
static unsigned primes[] = { 3, 5, 7, 11, 13, 17, 23, 29 } ;


/*
 * Pick a good number for hashing using the hint argument as a hint
 * on the order of magnitude.
 *
 * Algorithm:
 *		1. Find an odd number for testing numbers for primes. The starting
 *			point is 2**k-1 where k is selected such that
 *					2**k-1 <= hint < 2**(k+1)-1
 *		2. Check all odd numbers from the starting point on until you find
 *			one that is not a multiple of the selected primes.
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
	goto next ;
    return( size ) ;
  next: ;
  }
}


/*
 * Create a new hash table
 */
dict_h ht_create(dict_function oo_comp, dict_function ko_comp, int flags, struct ht_args *argsp)
{
  header_s			*hp ;
  int				allocator_flags ;
  unsigned			table_size, bucket_size ;
  char				*id = "ht_create" ;

  if ( !__dict_args_ok( id, flags, oo_comp, ko_comp, DICT_UNORDERED ) )
    return( NULL_HANDLE ) ;

  if ( argsp->ht_objvalue == NULL || argsp->ht_keyvalue == NULL )
    return( __dict_create_error( id, flags, DICT_ENOHVFUNC ) ) ;

  hp = HHP( malloc( sizeof( header_s ) ) ) ;
#ifdef DEBUG
  fprintf(stderr,"Mallocing hash hash header: %d\n", sizeof(header_s));
#endif /* DEBUG */
  if ( hp == NULL )
    return( __dict_create_error( id, flags, DICT_ENOMEM ) ) ;

  /*
   * Allocate the hash table
   */
  if ( argsp->ht_table_entries == 0 )
    argsp->ht_table_entries = ht_num_table_entries ;
  else
  {
    if (!(flags | DICT_HT_STRICT_HINTS))
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
  allocator_flags = argsp->ht_bucket_entries == 1 ? 0 : FSM_ZERO_ALLOC ;
  if ( flags & DICT_NOCOALESCE )
    allocator_flags |= FSM_NOCOALESCE ;
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
  return( (dict_h) hp ) ;
}



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

  fsm_destroy( hp->alloc ) ;
  free( hp->table ) ;
  free( hp ) ;
}


/*
 * Bucket chain reverse lookup:
 * 	Return a pointer to the last dict_obj of the bucket chain that
 * 	starts with bp (search up to the bucket 'stop' but *not* that bucket)
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
 *		Return a pointer to the first NULL (if type is EMPTY) or non-NULL
 *		(if type is FULL) dict_obj in the bucket chain.
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
	return( &bucket_list[j] ) ;
  }
  return( NULL ) ;
}



/*
 * Search the bucket chain for the specified object
 * Returns the pointer of the bucket where the object was found.
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
 */
PRIVATE dict_obj *te_expand(tabent_s *tep, header_s *hp)
{
  dheader_s	*dhp = DHP( hp ) ;
  bucket_s     	*bp ;

  bp = (bucket_s *) fsm_alloc( hp->alloc ) ;
  if ( bp == NULL )
    HANDLE_ERROR( dhp, "te_expand", DICT_ENOMEM, NULL ) ;

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
  return( BUCKET_OBJECTS( bp ) ) ;
}


/*
 * Search a table entry for an object
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


PRIVATE int ht_do_insert(header_s *hp, int uniq, register dict_obj object, dict_obj *objectp)
{
  dheader_s		*dhp = DHP( hp ) ;
  tabent_s		*tep ;
  dict_obj		*object_slot ;

  if ( object == NULL )
    HANDLE_ERROR( dhp, "ht_do_insert", DICT_ENULLOBJECT, DICT_ERR ) ;
	
  tep = HASH_OBJECT( hp, object ) ;

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
      ERRNO( dhp ) = DICT_EEXISTS ;
      return( DICT_ERR ) ;
    }
  }

  /*
   * If the entry chain is full, expand it
   */
  if ( ENTRY_IS_FULL( tep ) )
  {
    object_slot = te_expand( tep, hp ) ;
    if ( object_slot == NULL )
      return( DICT_ERR ) ;
  }
  else
    object_slot = bc_lookup( tep->head_bucket, hp->args.ht_bucket_entries, EMPTY ) ;
  tep->n_free-- ;

  *object_slot = object ;
  if ( objectp != NULL )
    *objectp = *object_slot ;
  return( DICT_OK ) ;
}



int ht_insert(dict_h handle, dict_obj object)
{
  header_s		*hp = HHP( handle ) ;

  return( ht_do_insert( hp,
			hp->dh.flags & DICT_UNIQUE_KEYS, object, (dict_obj *)NULL ) ) ;
}


int ht_insert_uniq(dict_h handle, dict_obj object, dict_obj *objectp)
{
  header_s    *hp = HHP( handle ) ;
  dheader_s	*dhp = DHP( hp ) ;

  if ( dhp->oo_comp == NULL_FUNC )
    HANDLE_ERROR( dhp, "ht_insert_uniq", DICT_ENOOOCOMP, DICT_ERR ) ;
  return( ht_do_insert( hp, TRUE, object, objectp ) ) ;
}


int ht_delete(dict_h handle, dict_obj object)
{
  header_s		*hp = HHP( handle ) ;
  dheader_s		*dhp = DHP( hp ) ;
  tabent_s		*tep ;
  int			bucket_index ;
  bucket_s		*bp ;

  if ( object == NULL )
    HANDLE_ERROR( dhp, "ht_delete", DICT_ENULLOBJECT, DICT_ERR ) ;

  tep = HASH_OBJECT( hp, object ) ;
  if ( ! ENTRY_HAS_CHAIN( tep ) )
  {
    ERRNO( dhp ) = DICT_ENOTFOUND ;
    return( DICT_ERR ) ;
  }

  bp = bc_search( tep->head_bucket,
		  hp->args.ht_bucket_entries, object, &bucket_index ) ;
  if ( bp != NULL )
  {
    BUCKET_OBJECTS( bp )[ bucket_index ] = NULL ;
    tep->n_free++ ;
    return( DICT_OK ) ;
  }
  else
  {
    ERRNO( dhp ) = DICT_ENOTFOUND ;
    return( DICT_ERR ) ;
  }
}


dict_obj ht_search(dict_h handle, dict_key key)
{
  header_s		*hp	= HHP( handle ) ;
  tabent_s		*tep	= HASH_KEY( hp, key ) ;
  dict_obj		*objp = te_search( tep, hp, KEY_SEARCH, (dict_h) key ) ;

  return( ( objp == NULL ) ? NULL_OBJ : *objp ) ;
}



dict_obj ht_minimum(dict_h handle)
{
  header_s		*hp		= HHP( handle ) ;
  unsigned		bucket_entries	= hp->args.ht_bucket_entries ;
  unsigned int		i ;

  for ( i = 0 ; i < hp->args.ht_table_entries ; i++ )
  {
    tabent_s *tep = &hp->table[i] ;
    dict_obj *found ;

    if ( ! ENTRY_HAS_CHAIN( tep ) )
      continue ;
    found = bc_lookup( tep->head_bucket, bucket_entries, FULL ) ;
    if ( found )
      return( *found ) ;
  }
  return( NULL_OBJ ) ;
}


dict_obj ht_maximum(dict_h handle)
{
  header_s		*hp		= HHP( handle ) ;
  unsigned		bucket_entries	= hp->args.ht_bucket_entries ;
  int i ;

  for ( i = hp->args.ht_table_entries-1 ; i >= 0 ; i-- )
  {
    tabent_s *tep = &hp->table[i] ;
    dict_obj *found ;

    if ( ! ENTRY_HAS_CHAIN( tep ) )
      continue ;
    found = bc_reverse_lookup( tep->head_bucket,
			       bucket_entries, BUCKET_NULL ) ;
    if ( found )
      return( *found ) ;
  }
  return( NULL_OBJ ) ;
}


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
  char			*id = "ht_successor" ;

  if ( object == NULL )
    HANDLE_ERROR( dhp, id, DICT_ENULLOBJECT, NULL_OBJ ) ;

  tep = HASH_OBJECT( hp, object ) ;
  if ( ! ENTRY_HAS_CHAIN( tep ) ||
       ( bp = bc_search( tep->head_bucket,
			 bucket_entries, object, &bucket_index ) ) == NULL )
    HANDLE_ERROR( dhp, id, DICT_EBADOBJECT, NULL_OBJ ) ;

  ERRNO( dhp ) = DICT_ENOERROR ;

  for ( i = bucket_index+1 ; i < bucket_entries ; i++ )
    if ( BUCKET_OBJECTS( bp )[ i ] != NULL )
      return( BUCKET_OBJECTS( bp )[ i ] ) ;

  for ( bp = bp->next ;; )
  {
    dict_obj *found = bc_lookup( bp, bucket_entries, FULL ) ;

    if ( found )
      return( *found ) ;
    tep++ ;
    if ( tep >= table_end )
      return( NULL_OBJ ) ;
    bp = tep->head_bucket ;
  }
}


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
  char			*id = "ht_predecessor" ;

  if ( object == NULL )
    HANDLE_ERROR( dhp, id, DICT_ENULLOBJECT, NULL_OBJ ) ;

  tep = HASH_OBJECT( hp, object ) ;
  stop = bc_search( tep->head_bucket, bucket_entries, object, &bucket_index ) ;
  if ( stop == NULL )
    HANDLE_ERROR( dhp, id, DICT_EBADOBJECT, NULL_OBJ ) ;
	
  ERRNO( dhp ) = DICT_ENOERROR ;

  for ( i = bucket_index-1 ; i >= 0 ; i-- )
    if ( BUCKET_OBJECTS( stop )[ i ] != NULL )
      return( BUCKET_OBJECTS( stop )[ i ] ) ;
	
  for ( ;; )
  {
    found = bc_reverse_lookup( tep->head_bucket, bucket_entries, stop ) ;
    if ( found )
      return( *found ) ;
    stop = NULL ;
    if ( tep <= hp->table )
      return( NULL_OBJ ) ;
    tep-- ;
  }
}



/*
 * Sets the iterator to the beginning of the next used entry.
 * The current table entry *is* included in the search.
 */
PRIVATE void iter_next(header_s *hp)
{
  register unsigned int	i ;
  struct ht_iter *ip = &hp->iter ;

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
 */
dict_iter ht_iterate(dict_h handle, enum dict_direction direction)
{
  header_s					*hp = HHP( handle ) ;

#ifdef lint
  direction = direction ;
#endif
  hp->iter.current_table_entry = 0 ;
  iter_next( hp ) ;

  /* TODO - create dynamically allocated iterator */
  return(&hp->iter);
}


void ht_iterate_done(dict_h handle, dict_iter iter)
{
  /* TODO -- delete dynamically allocated iterator */

  return;
}


dict_obj ht_nextobj(dict_h handle, dict_iter iter)
{
  header_s		*hp = HHP( handle ) ;
  struct ht_iter	*ip = iter;
  unsigned int		i ;

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
	  return( bucket_list[i] ) ;
	}
      }
      ip->current_bucket = ip->current_bucket->next ;
      ip->next_bucket_offset = 0;
    }
    while ( ip->current_bucket ) ;

    ip->current_table_entry++ ;
    iter_next( hp ) ;
  }
  return( NULL_OBJ ) ;
}



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
  return dll_create( oo_comp, ko_comp, flags, errnop);
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
