/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef __BST_H
#define __BST_H

/*
 * $Id: bst.h,v 1.3 2001/07/07 02:58:20 seth Exp $
 */

#include "dict.h"

dict_h   bst_create		 (dict_function oo_compare, dict_function ko_compare, int flags)  ;
void	 bst_destroy		 ( dict_h bh )  ;
int	 bst_insert		 ( dict_h bh, dict_obj obj )  ;
int	 bst_insert_uniq	 ( dict_h bh, dict_obj obj, dict_obj *objp )  ;
int	 bst_delete		 ( dict_h bh, dict_obj obj )  ;
dict_obj bst_search		 ( dict_h bh, dict_key key )  ;
dict_obj bst_minimum		 ( dict_h bh )  ;
dict_obj bst_maximum		 ( dict_h bh )  ;
dict_obj bst_successor		 ( dict_h bh, dict_obj obj )  ;
dict_obj bst_predecessor	 ( dict_h bh, dict_obj obj )  ;
void	 bst_iterate		 ( dict_h bh, enum dict_direction )  ;
dict_obj bst_nextobj		 ( dict_h bh )  ;
char *	 bst_error_reason	 ( dict_h bh, int *errnop )  ;


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

