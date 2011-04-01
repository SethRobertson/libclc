/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */


/*
 * $Id: htimpl.h,v 1.11 2003/06/17 05:10:51 seth Exp $
 */

#include "dictimpl.h"
#include "ht.h"

#include "fsma.h"



/*
 * The following definition is a little deceptive.
 * The bucket really looks like this:
 *
 *    -------------
 *   |    next     |
 *   |-------------|
 *   |             |
 *   |    data     |
 *   | (variable)  |
 *   |   (size)    |
 *   |             |
 *   |             |
 *   |             |
 *   |_____________|
 *
 * This definition provides for the chain operations.
 */
struct bucket
{
	struct bucket *next ;
} ;

typedef struct bucket bucket_s ;

#define BP( p )			((bucket_s *)(p))
#define BUCKET_NULL		BP(0)
#define BUCKET_OBJECTS( bp )	((dict_obj *)(&((bucket_s *)(&(bp)->next))[1]))

/*
 * Defaults
 */
#define DEFAULT_TABLE_ENTRIES       8191
#define DEFAULT_BUCKET_ENTRIES      15



struct table_entry
{
	bucket_s		*head_bucket ;
	unsigned		n_free ;
	unsigned		n_free_max ;
} ;

typedef struct table_entry tabent_s ;

#define TEP( p )		((tabent_s *)(p))


#define ENTRY_HAS_CHAIN( tep )		( (tep)->head_bucket != NULL )
#define ENTRY_IS_EMPTY( tep )		( (tep)->n_free == (tep)->n_free_max)

/*
 * <KLUDGE>In order to count how many buckets had hash collisions, we always
 * expand on second clashing insert, regardless of bucket size.</KLUDGE>
 */
#ifdef HASH_STATS

#define ENTRY_IS_FULL( tep, hp )	( (tep)->n_free == 0 || ( (hp)->args.ht_stats && (tep)->n_free_max == (hp)->args.ht_bucket_entries && ! ENTRY_IS_EMPTY( tep ) ) )

#else  /* !HASH_STATS */

#define ENTRY_IS_FULL( tep, hp )	( (tep)->n_free == 0 )

#endif /* !HASH_STATS */

struct ht_iter
{
	unsigned int 		current_table_entry ;
	bucket_s		*current_bucket ;
	int 			next_bucket_offset ;
} ;


/*
 * A hash table is implemented as an array of table entries, each
 * pointing to a linked list of buckets containing the actual data
 */
struct ht_header
{
	dheader_s 		dh ;
	fsma_h 			alloc ;
	struct table_entry	*table ;
	struct ht_args 		args ;
	unsigned int		flags ;
#ifdef BK_USING_PTHREADS
        pthread_mutex_t		lock ;
	int			iter_cnt ;
	struct ht_iter		**iter ;
#else /* BK_USING_PTHREADS */
	struct ht_iter 		iter ;
#endif /* BK_USING_PTHREADS */
#ifdef CUR_MIN_PERF_HACK
        unsigned int		cur_min ;
#endif /* CUR_MIN_PERF_HACK */
	unsigned int		obj_cnt ;
} ;

typedef struct ht_header header_s ;


/*
 * Statistics on hash table performance
 */
struct ht_stats
{
	unsigned int		max_cnt ;
	unsigned int		max_chain ;
	unsigned int		inserts ;
	unsigned int		deletes ;
	unsigned int		clashes ;
	unsigned int		searches ;
	unsigned int		failures ;
	unsigned int		minmaxes ;
	unsigned int		succ_steps ;
	unsigned int		iterations ;
	unsigned int		iter_steps ;
	unsigned int		errors ;
	unsigned int		used ;
	unsigned int		overused ;
} ;

typedef struct ht_stats stats_s ;

#define HSP( p )		((stats_s *)(p))

#define HHP( p )		((header_s *)p)
#define DHP( hp )		(&(hp->dh))

typedef enum { KEY_SEARCH, OBJECT_SEARCH } search_e ;

