/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */


/*
 * $Id: bstimpl.h,v 1.5 2003/06/17 05:10:51 seth Exp $
 */

#include "dictimpl.h"
#include "bst.h"

/*
 * We allocate a tree_node or a balanced_tree_node depending on the type
 * of tree. The code requires that both node types have the same memory
 * representation (except for the extra field(s) at the end of the
 * balanced_tree_node).
 */
struct tree_node
{
	struct tree_node	*left ;
	struct tree_node	*right ;
	struct tree_node	*parent ;
	dict_obj		 obj ;
} ;

typedef struct tree_node tnode_s ;

#define TNP( p )                       ((tnode_s *)(p))
#define NULL_NODE                      TNP( NULL )

#define LEFT( p )                      (p)->left
#define RIGHT( p )                     (p)->right
#define PARENT( p )                    (p)->parent
#define OBJ( p )                       (p)->obj

enum node_color { RED, BLACK } ;

struct balanced_tree_node
{
	tnode_s				node ;
	enum node_color	color ;
} ;

typedef struct balanced_tree_node btnode_s ;

#define BTNP( p )			((btnode_s *)(p))
#define COLOR( p )			BTNP(p)->color


/*
 * ABOUT HINTS:
 *    a. We keep hints to avoid searches of the tree.
 *    b. A hint is either correct or non-existent.
 *    c. To avoid bad hints, bst_delete clears all hints
 *    d. An operation that uses/consults a hint always clears it or resets it.
 *
 *   --------------------------------------------------------------------
 *  | OPERATIONS |                        HINTS                          |
 *  |------------|-------------------------------------------------------|
 *  |            |    SEARCH    |    SUCCESSOR       |      PREDECESSOR  |
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
   tnode_s   *last_search ;
   tnode_s   *last_successor ;
   tnode_s   *last_predecessor ;
} ;


#define HINT_GET( hp, hintname )       (hp)->hint.hintname
#define HINT_SET( hp, hintname, v )    (hp)->hint.hintname = v
#define HINT_CLEAR( hp, hintname )     HINT_SET( hp, hintname, NIL( hp ) )
#define HINT_MATCH( hp, hintname, v )  ( OBJ( (hp)->hint.hintname ) == v )


#include "fsma.h"

struct tree_iterator
{
	enum dict_direction	direction ;
	tnode_s			*next ;
} ;


/*
 * The 'nil' field is used instead of NULL, to indicate the absence of
 * a node. This allows us to avoid explicit tests against NULL in case
 * of boundary conditions.
 *
 * The only unusual thing in this implementation is the 'anchor' field
 * which is used as the actual root of the tree. The user-visible root of
 * the tree is always the *left* child of the anchor. The TREE_EMPTY macro
 * below tests this condition.
 */
struct tree_header
{
	dheader_s			dh ;
	struct hints			hint ;
	fsma_h				alloc ;
	btnode_s			anchor ;
	btnode_s			nil ;
#ifdef BK_USING_PTHREADS
	u_int				flags;
        pthread_mutex_t			lock;
	int				tip_cnt;
        struct tree_iterator	      **tip;
#else /* BK_USING_PTHREADS */
        struct tree_iterator	        iter;
#endif /* BK_USING_PTHREADS */
} ;

typedef struct tree_header header_s ;

#define THP( p )                       ((header_s *)(p))
#define DHP( hp )                      (&(hp->dh))
#define NULL_HEADER                    THP( NULL )

#define ANCHOR( hp )                   TNP( (&(hp)->anchor) )
#define ROOT( hp )                     LEFT( ANCHOR( hp ) )
#define NIL( hp )                      TNP( (&(hp)->nil) )

#define TREE_EMPTY( hp )               ( ROOT( hp ) == NIL( hp ) )

#define NODE_ALLOC( hp )               TNP( fsm_alloc( (hp)->alloc ) )
#define NODE_FREE( hp, np )            fsm_free( (hp)->alloc, (char *)(np) )

void __dict_rbt_insfix(header_s *hp, tnode_s *newnode);
void __dict_rbt_delfix(header_s *hp, tnode_s *x);

