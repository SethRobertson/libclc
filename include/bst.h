/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef __BST_H
#define __BST_H

/*
 * $Id: bst.h,v 1.1 2001/05/26 22:04:51 seth Exp $
 */

#include "dict.h"

dict_h bst_create			__ARGS( (
												dict_function oo_compare,
												dict_function ko_compare,
												int flags,
												int *errnop
										) ) ;
void	bst_destroy			__ARGS( ( dict_h bh ) ) ;
int	bst_insert			__ARGS( ( dict_h bh, dict_obj obj ) ) ;
int	bst_insert_uniq	__ARGS( ( dict_h bh, dict_obj obj, dict_obj *objp ) ) ;
int	bst_delete			__ARGS( ( dict_h bh, dict_obj obj ) ) ;
dict_obj bst_search		__ARGS( ( dict_h bh, dict_key key ) ) ;
dict_obj bst_minimum		__ARGS( ( dict_h bh ) ) ;
dict_obj bst_maximum		__ARGS( ( dict_h bh ) ) ;
dict_obj bst_successor	__ARGS( ( dict_h bh, dict_obj obj ) ) ;
dict_obj bst_predecessor __ARGS( ( dict_h bh, dict_obj obj ) ) ;
void		bst_iterate		__ARGS( ( dict_h bh, enum dict_direction ) ) ;
dict_obj	bst_nextobj		__ARGS( ( dict_h bh ) ) ;


#ifdef BST_DEBUG

typedef enum { BST_PREORDER, BST_INORDER, BST_POSTORDER } bst_order_e ;

struct bst_depth
{
	int depth_min ;
	int depth_max ;
} ;

void		bst_getdepth		__ARGS( ( dict_h bh, struct bst_depth * ) ) ;
void		bst_traverse		__ARGS( ( dict_h bh, bst_order_e, void (*)() ) ) ;

#endif	/* BST_DEBUG */

#endif 	/* __BST_H */

