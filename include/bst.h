/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef __BST_H
#define __BST_H

/*
 * $Id: bst.h,v 1.5 2003/04/01 04:40:56 seth Exp $
 */

#include "dict.h"

dict_h   bst_create		 (dict_function oo_compare, dict_function ko_compare, int flags)  ;
void	 bst_destroy		 ( dict_h bh )  ;
int	 bst_insert		 ( dict_h bh, dict_obj obj )  ;
int	 bst_insert_uniq	 ( dict_h bh, dict_obj obj, dict_obj *objp )  ;
#define bst_append bst_insert
#define bst_append_uniq bst_insert_uniq
int	 bst_delete		 ( dict_h bh, dict_obj obj )  ;
dict_obj bst_search		 ( dict_h bh, dict_key key )  ;
dict_obj bst_minimum		 ( dict_h bh )  ;
dict_obj bst_maximum		 ( dict_h bh )  ;
dict_obj bst_successor		 ( dict_h bh, dict_obj obj )  ;
dict_obj bst_predecessor	 ( dict_h bh, dict_obj obj )  ;
dict_iter bst_iterate		 ( dict_h bh, enum dict_direction )  ;
void bst_iterate_done		 ( dict_h bh, dict_iter iter )  ;
dict_obj bst_nextobj		 ( dict_h bh, dict_iter iter )  ;
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

