/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

static const char RCSid[] = "$Id: bst.c,v 1.11 2002/07/24 21:20:15 seth Exp $";

#include <stdlib.h>
#include "bstimpl.h"
#include "clchack.h"

#define NODE_ALLOC( hp )               TNP( fsm_alloc( (hp)->alloc ) )
#define NODE_FREE( hp, np )            fsm_free( (hp)->alloc, (char *)(np) )

PRIVATE tnode_s *previous_node(register header_s *hp, register tnode_s *x);

/*
 * Find the minimum node of the subtree with root 'start'
 */
#define FIND_MINIMUM( hp, start, x )	x = start; while ( LEFT( x ) != NIL( hp ) ) x = LEFT( x )

/*
 * Find the maximum node of the subtree with root 'start'
 */
#define FIND_MAXIMUM( hp, start, x )	x = start; while ( RIGHT( x ) != NIL( hp ) ) x = RIGHT( x )



/*
 * Returns a pointer to the node that contains the specified object
 * or NIL( hp ) if the object is not found under the node
 */
PRIVATE tnode_s *find_object_in_tree(header_s *hp, tnode_s *np, dict_obj object)
{
  dheader_s 		*dhp 	= DHP( hp ) ;
  register tnode_s	*null	= NIL( hp ) ;
  register int 		v ;
	
  while ( np != null )
  {
    v = (*dhp->oo_comp)( object, OBJ( np ) ) ;
    if ( v == 0 )
    {
      if ( object == OBJ( np ) )
	break ;
      else
      {
	/*
	 * for equal (non-unique) must check both right and left
	 * recurse on right, iterate on left
	 */
	tnode_s *rp = find_object_in_tree( hp, RIGHT( np ), object ); 

	if ( rp != null )
	  return ( rp );

	v = -1 ;
      }
    }
    np = ( v < 0 ) ? LEFT( np ) : RIGHT( np ) ;
  }
  return( np ) ;
}


/*
 * Returns a pointer to the node that contains the specified object
 * or NIL( hp ) if the object is not found
 */
PRIVATE tnode_s *find_object(header_s *hp, dict_obj object)
{
  register tnode_s	*np	= ROOT( hp ) ;
	
  return( find_object_in_tree( hp, np, object ) ) ;
}



/*
 * Create a tree (either simple or red-black)
 */
dict_h bst_create(dict_function oo_comp, dict_function ko_comp, int flags)
{
  register header_s	*hp ;
  unsigned		tnode_size ;
  bool_int 		balanced_tree	= flags & DICT_BALANCED_TREE ;
  char			*id = "bst_create" ;

  if ( ! __dict_args_ok( id, flags, oo_comp, ko_comp, DICT_ORDERED ) )
    return( NULL_HANDLE ) ;

  hp = THP( malloc( sizeof( header_s ) ) ) ;
  if ( hp == NULL_HEADER )
    return( __dict_create_error( id, flags, DICT_ENOMEM ) ) ;
	
  /*
   * Create an allocator
   */
  tnode_size = sizeof( struct tree_node ) ;
  if ( balanced_tree )
    tnode_size = sizeof( struct balanced_tree_node ) ;

  hp->alloc = fsm_create( tnode_size, 0,
			  ( flags & DICT_NOCOALESCE ) ? FSM_NOCOALESCE : FSM_NOFLAGS ) ;
  if ( hp->alloc == NULL )
  {
    free( (char *)hp ) ;
    return( __dict_create_error( id, flags, DICT_ENOMEM ) ) ;
  }

  LEFT( ANCHOR( hp ) ) = RIGHT( ANCHOR( hp ) ) = NIL( hp ) ;
  OBJ( ANCHOR( hp ) ) = NULL_OBJ ;
  OBJ( NIL( hp ) ) = NULL_OBJ ;

  if ( balanced_tree )
  {
    COLOR( ANCHOR( hp ) ) = BLACK ;
    COLOR( NIL( hp ) ) = BLACK ;
  }
	
  /*
   * Initialize dictionary header, hints
   */
  __dict_init_header( DHP( hp ), oo_comp, ko_comp, flags ) ;
  HINT_CLEAR( hp, last_search ) ;
  HINT_CLEAR( hp, last_successor ) ;
  HINT_CLEAR( hp, last_predecessor ) ;

#if defined SAFE_ITERATE || defined FAST_ACTIONS
  /* Make sure iteration stuff is initialized too. */
  bst_iterate(hp, DICT_FROM_START);
#endif
  return( (dict_h) hp ) ;
}



#ifdef COALESCE
/*
 * recursive delete
 */
static void bst_redelete(header_s *hp, tnode_s *np)
{
  // if (!hp || !np || np == NIL(hp))
  //return;

  if (LEFT(np) != NIL(hp))
    bst_redelete(hp, LEFT(np));

  if (RIGHT(np) != NIL(hp))
    bst_redelete(hp, RIGHT(np));

  NODE_FREE(hp, np);
}
#endif /* COALESCE */


/*
 * Destroy a tree
 */
void bst_destroy(dict_h handle)
{
  header_s	*hp	= THP( handle ) ;
  dheader_s	*dhp	= DHP( hp ) ;

#ifdef COALESCE
  if (!(dhp->flags & DICT_NOCOALESCE) && !TREE_EMPTY(hp))
    bst_redelete(hp, ROOT(hp));
#endif /* COALESCE */

  fsm_destroy( hp->alloc ) ;
  free( (char *)hp ) ;
}



/*
 * Common code for tree insertions
 */
PRIVATE int tree_insert(header_s *hp, register int uniq, dict_obj object, dict_obj *objectp)
{
  dheader_s		*dhp	= DHP( hp ) ;
  register tnode_s	*x	= ROOT( hp ) ;
  register tnode_s	*px	= ANCHOR( hp ) ;
  tnode_s		*newnode ;
  register int 		v ;

  if ( object == NULL_OBJ )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, DICT_ERR ) ;

  /*
   * On exit from this loop, 'px' will point to a leaf node
   * Furthermore, 'v' will hold the comparison value between the
   * object stored at this node and the new 'object'.
   *
   * v must be initialized to -1 so that when we insert a node into an 
   * empty tree that node will be the *left* child of the anchor node
   */
  v = -1 ;
  while ( x != NIL( hp ) )
  {
    v = (*dhp->oo_comp)( object, OBJ( x ) ) ;

    if ( v == 0 )
    {
      if ( uniq )
      {
	if ( objectp != NULL )
	  *objectp = OBJ( x ) ;
	ERRNO( dhp ) = DICT_EEXISTS ;
	return( DICT_ERR ) ;
      }
      else
	v = -1 ;			/* i.e. LESS THAN */
    }

    px = x ;
    x = ( v < 0 ) ? LEFT( x ) : RIGHT( x ) ;
  }

  /*
   * Allocate a new node and initialize all its fields
   */
  newnode = NODE_ALLOC( hp ) ;
  if ( newnode == NULL_NODE )
  {
    ERRNO( dhp ) = DICT_ENOMEM ;
    return( DICT_ERR ) ;
  }
  LEFT( newnode ) = RIGHT( newnode ) = NIL( hp ) ;
  OBJ( newnode ) = object ;
  PARENT( newnode ) = px ;

  if ( v < 0 )
    LEFT( px ) = newnode ;
  else
    RIGHT( px ) = newnode ;

  if ( dhp->flags & DICT_BALANCED_TREE )
    __dict_rbt_insfix( hp, newnode ) ;
	
  if ( objectp != NULL )
    *objectp = object ;

  return( DICT_OK ) ;
}


/*
 * Insert the specified object in the tree
 */
int bst_insert(dict_h handle, dict_obj object)
{
  header_s *hp = THP( handle ) ;

  return( tree_insert( hp,
		       hp->dh.flags & DICT_UNIQUE_KEYS, object, (dict_obj *)NULL ) ) ;
}


/*
 * Insert the specified object in the tree only if it is unique
 */
int bst_insert_uniq(dict_h handle, dict_obj object, dict_obj *objectp)
{
  return( tree_insert( THP( handle ), TRUE, object, objectp ) ) ;
}


/*
 * Delete the specified object
 */
int bst_delete(dict_h handle, dict_obj object)
{
  header_s		*hp	= THP( handle ) ;
  dheader_s		*dhp	= DHP( hp ) ;
  register tnode_s	*null	= NIL( hp ) ;
  tnode_s		*delnp	= NULL ;
  register tnode_s	*y, *x ;
  tnode_s		*py ;
#ifdef SAFE_ITERATE
  struct tree_iterator	*tip		= &THP( handle )->iter ;
#ifdef PARANOID_ITERATE
  dict_obj		safe_nextobj	= NULL;
#endif /* PARANOID_ITERATE */
#endif /* SAFE_ITERATE */
#ifdef FAST_ACTIONS
  tnode_s *tmp=NULL;
#endif /* FAST_ACTIONS */

  if ( object == NULL_OBJ )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, DICT_ERR ) ;

#ifdef FAST_ACTIONS
  if ( tip->next && tip->next != ANCHOR(hp) && tip->next != NIL(hp) &&
       (tmp=previous_node(hp, tip->next)) &&
       (OBJ(tmp)==object))
  {
    delnp = tmp;
  }
  else  if ( HINT_MATCH( hp, last_predecessor, object ) )
    delnp = HINT_GET( hp, last_predecessor ) ;
  else if ( HINT_MATCH( hp, last_successor, object ) )
    delnp = HINT_GET( hp, last_successor ) ;
  else 
#endif /* FAST_ACTIONS */
    if ( HINT_MATCH( hp, last_search, object ) )
      delnp = HINT_GET( hp, last_search ) ;
    else
    {
      delnp = find_object( hp, object ) ;
      if ( delnp == null )
      {
	ERRNO( dhp ) = DICT_ENOTFOUND ;
	return( DICT_ERR ) ;
      }
    }

#ifdef SAFE_ITERATE	
  if (tip->next)
  {
    if (OBJ(tip->next) == object)
    {
      bst_nextobj(handle, tip);
    }
#ifdef PARANOID_ITERATE
    if (tip->next)
      safe_nextobj = OBJ(tip->next);
#endif /* PARANOID_ITERATE */
  }
#endif /* SAFE_IITERATE */

  HINT_CLEAR( hp, last_search ) ;
  HINT_CLEAR( hp, last_successor ) ;
  HINT_CLEAR( hp, last_predecessor ) ;


  /*
   * y is the node actually being removed. It may be 'delnp' if
   * 'delnp' has at most 1 child, otherwise it is 'delnp's successor
   */
  if ( LEFT( delnp ) == null || RIGHT( delnp ) == null )
  {
    y = delnp ;
  }
  else
  {
    /*
     * We can use the FIND_MINIMUM macro since we are guaranteed
     * there is a right child
     */
    FIND_MINIMUM( hp, RIGHT( delnp ), y ) ;
    OBJ( delnp ) = OBJ( y ) ;
#ifdef SAFE_ITERATE
    /* Update the iterator if it is pointing at the moved object */
    if ( tip->next == y )
    {
      tip->next = delnp;
    }
#endif /* SAFE_ITERATE */

  }

  /*
   * Set x to one of y's children:
   *		Case 1: y == delnp
   *			pick any child
   *		Case 2: y == successor( delnp )
   *			y can only have a right child (if any), so pick that.
   * Next, we link x to y's parent.
   *
   * The use of the NIL and ANCHOR nodes relieves us from checking
   * boundary conditions:
   *		existence of x	(when setting the PARENT field of x)
   *		y being the root of the tree
   */
  x = ( LEFT( y ) != null ) ? LEFT( y ) : RIGHT( y ) ;
  py = PARENT( y ) ;
  PARENT( x ) = py ;
  if ( y == LEFT( py ) )
    LEFT( py ) = x ;
  else
    RIGHT( py ) = x ;

#ifdef PARANOID_ITERATE
  if (tip->next)
  {
    // These errors should not happen
    if ((OBJ(tip->next) != safe_nextobj) || tip->next == y)
    {
      // Find bloody object

      if (!safe_nextobj)
	tip->next = NULL;
      else
      {
	// Search for safeobj
	while ( tip->next != null )
	{
	  if (OBJ( tip->next ) == safe_nextobj)
	    break;
	  tip->next = ( (*dhp->oo_comp)( safe_nextobj, OBJ( tip->next ) ) < 0 ) ? LEFT( tip->next ) : RIGHT( tip->next ) ;
	}
      }
    }
  }
#endif /* PARANOID_ITERATE */

  /*
   * If this is a balanced tree and we unbalanced it, do the necessary repairs
   */
  if ( ( dhp->flags & DICT_BALANCED_TREE ) && COLOR( y ) == BLACK )
    __dict_rbt_delfix( hp, x ) ;

  NODE_FREE( hp, y ) ;

  return( DICT_OK ) ;
}



/*
 * Find an object with the specified key
 */
dict_obj bst_search(dict_h handle, dict_key key)
{
  header_s		*hp	= THP( handle ) ;
  dheader_s		*dhp	= DHP( hp ) ;
  register tnode_s	*np	= ROOT( hp ) ;
  register tnode_s	*null = NIL( hp ) ;
  register int 		v ;

  while ( np != null )
  {
    v = (*dhp->ko_comp)( key, OBJ( np ) ) ;
    if ( v == 0 )
      break ;
    else
      np = ( v < 0 ) ? LEFT( np ) : RIGHT( np ) ;
  }
  HINT_SET( hp, last_search, np ) ; 		/* update search hint */
  return( OBJ( np ) ) ;
}



/*
 * Returns a pointer to the object with the smallest key value or
 * NULL_OBJ if the tree is empty.
 */
dict_obj bst_minimum(dict_h handle)
{
  register header_s	*hp = THP( handle ) ;
  register tnode_s	*np ;

  if ( TREE_EMPTY( hp ) )
    return( NULL_OBJ ) ;
  FIND_MINIMUM( hp, ROOT( hp ), np ) ;
  HINT_SET( hp, last_successor, np ) ;
  return( OBJ( np ) ) ;
}



/*
 * Returns a pointer to the object with the greatest key value or
 * NULL_OBJ if the tree is empty.
 */
dict_obj bst_maximum(dict_h handle)
{
  register header_s	*hp = THP( handle ) ;
  register tnode_s	*np ;

  if ( TREE_EMPTY( hp ) )
    return( NULL_OBJ ) ;
  FIND_MAXIMUM( hp, ROOT( hp ), np ) ;
  HINT_SET( hp, last_predecessor, np ) ;
  return( OBJ( np ) ) ;
}



/*
 * When there is no next node, this function returns ANCHOR( hp )
 */
PRIVATE tnode_s *next_node(register header_s *hp, register tnode_s *x)
{
  register tnode_s	*px ;
  register tnode_s	*next ;

  if ( RIGHT( x ) != NIL( hp ) )
  {
    FIND_MINIMUM( hp, RIGHT( x ), next ) ;
  }
  else
  {
    px = PARENT( x ) ;
    /*
     * This loop will end at the root since the root is the *left*
     * child of the anchor
     */
    while ( x == RIGHT( px ) )
    {
      x = px ;
      px = PARENT( x ) ;
    }
    next = px ;
  }
  return( next ) ;
}



/*
 * Returns a pointer to the object with the next >= key value or
 * NULL_OBJ if the given object is the last one on the tree.
 * It is an error to apply this function to an empty tree.
 */
dict_obj bst_successor(dict_h handle, dict_obj object)
{
  register header_s	*hp	= THP( handle ) ;
  dheader_s		*dhp	= DHP( hp ) ;
  register tnode_s	*x ;
  register tnode_s	*successor ;

  if ( object == NULL_OBJ )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, NULL_OBJ ) ;

  if ( TREE_EMPTY( hp ) )
    HANDLE_ERROR( dhp, DICT_EBADOBJECT, NULL_OBJ ) ;

  if ( HINT_MATCH( hp, last_successor, object ) )
    x = HINT_GET( hp, last_successor ) ;
  else
  {
    x = find_object( hp, object ) ;
    if ( x == NIL( hp ) )
      HANDLE_ERROR( dhp, DICT_EBADOBJECT, NULL_OBJ ) ;
  }

  successor = next_node( hp, x ) ;

  HINT_SET( hp, last_successor, successor ) ;
  ERRNO( DHP( hp ) ) = DICT_ENOERROR ;		/* in case we return NULL_OBJ */
  return( OBJ( successor ) ) ;
}



/*
 * When there is no next node, this function returns ANCHOR( hp )
 */
PRIVATE tnode_s *previous_node(register header_s *hp, register tnode_s *x)
{
  register tnode_s	*px ;
  register tnode_s	*previous ;

  if ( LEFT( x ) != NIL( hp ) )
  {
    FIND_MAXIMUM( hp, LEFT( x ), previous ) ;
  }
  else
  {
    /*
     * XXX:	To avoid testing against the ANCHOR we can temporarily make the
     * 		root of the tree the *right* child of the anchor
     */
    px = PARENT( x ) ;
    while ( px != ANCHOR( hp ) && x == LEFT( px ) )
    {
      x = px ;
      px = PARENT( x ) ;
    }
    previous = px ;
  }
  return( previous ) ;
}
	


/*
 * Returns a pointer to the object with the next <= key value or
 * NULL if the given object is the first one on the tree.
 * It is an error to apply this function to an empty tree.
 */
dict_obj bst_predecessor(dict_h handle, dict_obj object)
{
  register header_s	*hp	= THP( handle ) ;
  dheader_s		*dhp	= DHP( hp ) ;
  tnode_s		*predecessor ;
  register tnode_s	*x ;

  if ( object == NULL_OBJ )
    HANDLE_ERROR( dhp, DICT_ENULLOBJECT, NULL_OBJ ) ;

  if ( TREE_EMPTY( hp ) )
    HANDLE_ERROR( dhp, DICT_EBADOBJECT, NULL_OBJ ) ;

  if ( HINT_MATCH( hp, last_predecessor, object ) )
    x = HINT_GET( hp, last_predecessor ) ;
  else
  {
    x = find_object( hp, object ) ;
    if ( x == NIL( hp ) )
      HANDLE_ERROR( dhp, DICT_EBADOBJECT, NULL_OBJ ) ;
  }

  predecessor = previous_node( hp, x ) ;

  HINT_SET( hp, last_predecessor, predecessor ) ;
  ERRNO( DHP( hp ) ) = DICT_ENOERROR ;
  return( OBJ( predecessor ) ) ;
}



dict_iter bst_iterate(dict_h handle, enum dict_direction direction)
{
  register header_s	*hp	= THP( handle ) ;
  struct tree_iterator	*tip	= &hp->iter ;
  tnode_s		*np ;

  tip->direction = direction ;
  if ( TREE_EMPTY( hp ) )
    tip->next = NULL ;
  else
  {
    if ( direction == DICT_FROM_START )
    {
      FIND_MINIMUM( hp, ROOT( hp ), np ) ;
    }
    else
    {
      FIND_MAXIMUM( hp, ROOT( hp ), np ) ;
    }
    tip->next = np ;
  }

  return(tip);
}



void bst_iterate_done(dict_h handle, dict_iter iter)
{
  /* TODO -- delete dynamically allocated iterator */

  return;
}


dict_obj bst_nextobj(dict_h handle, dict_iter iter)
{
  register header_s	*hp		= THP( handle ) ;
  struct tree_iterator	*tip		= iter ;
  tnode_s		*current	= tip->next ;

  if ( current == NULL )
    return( NULL_OBJ ) ;

  if ( tip->direction == DICT_FROM_START )
    tip->next = next_node( hp, current ) ;
  else
    tip->next = previous_node( hp, current ) ;

  if ( tip->next == ANCHOR( hp ) )
    tip->next = NULL ;
  return( OBJ( current ) ) ;
}



char *bst_error_reason(dict_h handle, int *errnop)
{
  header_s	*hp		= THP( handle ) ;
  int		dicterrno;

  if (handle)
    dicterrno = ERRNO(DHP(hp));
  else
    dicterrno = dict_errno;

  if (errnop) *errnop = dicterrno;

  return(__dict_error_reason(dicterrno));
}



#ifdef BST_DEBUG

#include <stdio.h>


PRIVATE void preorder( hp, np, action )
     header_s	*hp ;
     tnode_s		*np ;
     void		(*action)() ;
{
  if ( np == NIL( hp ) )
    return ;

  (*action)( OBJ( np ) ) ;
  preorder( hp, LEFT( np ), action ) ;
  preorder( hp, RIGHT( np ), action ) ;
}


PRIVATE void inorder( hp, np, action )
     header_s		*hp ;
     tnode_s		*np ;
     void			(*action)() ;
{
  if ( np == NIL( hp ) )
    return ;

  inorder( hp, LEFT( np ), action ) ;
  (*action)( OBJ( np ) ) ;
  inorder( hp, RIGHT( np ), action ) ;
}


PRIVATE void postorder( hp, np, action )
     header_s		*hp ;
     tnode_s		*np ;
     void			(*action)() ;
{
  if ( np == NIL( hp ) )
    return ;

  postorder( hp, LEFT( np ), action ) ;
  postorder( hp, RIGHT( np ), action ) ;
  (*action)( OBJ( np ) ) ;
}


void bst_traverse( handle, order, action )
     dict_h		handle ;
     bst_order_e	order ;
     void			(*action)() ;
{
  header_s *hp = THP( handle ) ;

  switch ( order )
  {
  case BST_INORDER:
    inorder( hp, ROOT( hp ), action ) ;
    break ;

  case BST_PREORDER:
    preorder( hp, ROOT( hp ), action ) ;
    break ;
		
  case BST_POSTORDER:
    postorder( hp, ROOT( hp ), action ) ;
    break ;
  }
}


#ifdef MIN
#undef MIN
#endif
#define MIN( a, b )        ( a < b ? a : b )

#ifdef MAX
#undef MAX
#endif
#define MAX( a, b )        ( a > b ? a : b )

void get_depth( hp, np, dp )
     header_s				*hp ;
     tnode_s           *np ;
     struct bst_depth	*dp ;
{
  struct bst_depth	ldep, rdep ;

  if ( np == NIL( hp ) )
  {
    dp->depth_min = dp->depth_max = 0 ;
    return ;
  }
  get_depth( hp, LEFT( np ), &ldep ) ;
  get_depth( hp, RIGHT( np ), &rdep ) ;
  dp->depth_min = MIN( ldep.depth_min, rdep.depth_min ) + 1 ;
  dp->depth_max = MAX( ldep.depth_max, rdep.depth_max ) + 1 ;
  if ( DHP( hp )->flags & DICT_BALANCED_TREE )
  {
    if ( dp->depth_max > 2*dp->depth_min )
      (void) fprintf( stderr, "tree is not balanced\n" ) ;
  }
}

void bst_getdepth( handle, dp )
     dict_h				handle ;
     struct bst_depth	*dp ;
{
  header_s 	*hp = THP( handle ) ;

  get_depth( hp, ROOT( hp ), dp ) ;
}

#endif	/* BST_DEBUG */

