/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */


/*
 * $Id: dllimpl.h,v 1.3 2003/04/09 19:51:56 seth Exp $
 */

#include "dictimpl.h"
#include "dll.h"

struct list_node
{
	struct list_node	*next ;
	struct list_node	*prev ;
	dict_obj		obj ;
} ;

typedef struct list_node node_s ;


/*
 * Macros for list_node's
 */
#define NEXT( n )				(n)->next
#define PREV( n )				(n)->prev
#define OBJ( n )				(n)->obj

#define DLL_PREPEND	0
#define DLL_POSTPEND	1

/*
 * The oo_compare function is used for insertions and deletions.
 * The ko_compare function is used in searches.
 * These functions are expected to return:
 *	a negative value 	if	key(o1) < key(o2)
 *	0			if 	key(o1) = key(o2)
 *	a positive value 	if	key(o1) > key(o2)
 *
 *
 * The list is sorted according to some key. The package does not
 * care what the key is. The order is defined by the two comparison
 * functions (which, of course, should be consistent).
 *
 * We assume that key values get larger when we follow the _next_ chain
 * and they get smaller when we follow the _prev_ chain.
 *
 *
 * ABOUT HINTS:
 * 	a. We keep hints to avoid linear searches of the linked list.
 * 	b. A hint is either correct or non-existent.
 * 	c. To avoid bad hints, dll_delete sets the DATA field of a 
 *			list_node to NULL.
 * 	d. We do not allow insertions of NULL.
 *	e. An operation that uses/consults a hint always clears it or resets it.
 *
 *   --------------------------------------------------------------------
 *  | OPERATIONS |                        HINTS                          |
 *  |------------|-------------------------------------------------------|
 *  |	     	 |    SEARCH    |    SUCCESSOR	     |    PREDECESSOR    |
 *  |------------|--------------|--------------------|-------------------|
 *  | insert     |       X      |        X           |         X         |
 *  | delete     | USE & CLEAR  |      CLEAR         |       CLEAR       |
 *  | search     |      SET     |        X           |         X         |
 *  | minimum    |       X      |       SET          |         X         |
 *  | maximum    |       X      |        X           |        SET        |
 *  | successor  |       X      |    USE & SET       |         X         |
 *  | predecessor|       X      |        X           |     USE & SET     |
 *   --------------------------------------------------------------------
 *
 */


struct hints
{
	node_s *last_search ;
	node_s *last_successor ;
	node_s *last_predecessor ;
} ;

#include "fsma.h"

struct dll_iterator
{
	node_s			*next ;
	enum dict_direction	direction ;
} ;

struct dll_header
{
	struct dict_header 	dh ;
	struct hints 		hint ;
	fsma_h 			alloc ;		/* FSM allocator */
	node_s			*head ;
#ifdef BK_USING_PTHREADS
	u_int			flags;
        pthread_mutex_t		lock;
	int			iter_cnt;
	struct dll_iterator	**iter ;
#else /* BK_USING_PTHREADS */
	struct dll_iterator	iter ;
#endif /* BK_USING_PTHREADS */
} ;

typedef struct dll_header header_s ;

#define DHP( hp )		(&(hp->dh))
#define LHP( p )		((struct dll_header *) (p))

#define HINT_CLEAR( hp, hint_name )	hp->hint.hint_name = hp->head


