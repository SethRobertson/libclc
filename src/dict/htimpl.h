/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */


/*
 * $Id: htimpl.h,v 1.5 2002/08/27 11:17:39 seth Exp $
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

#define TEP( p )							((tabent_s *)(p))


#define ENTRY_HAS_CHAIN( tep )		( (tep)->head_bucket != NULL )
#define ENTRY_IS_FULL( tep )		( (tep)->n_free == 0 )
#define ENTRY_IS_EMPTY( tep )		( (tep)->n_free == (tep)->n_free_max)


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
	struct ht_iter 		iter ;
        unsigned int		cur_min;
  	unsigned int		obj_cnt;
} ;

typedef struct ht_header header_s ;


#define HHP( p )		((header_s *)p)
#define DHP( hp )		(&(hp->dh))

typedef enum { KEY_SEARCH, OBJECT_SEARCH } search_e ;

