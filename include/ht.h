/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef __HT_H
#define __HT_H

/*
 * $Id: ht.h,v 1.2 2001/07/05 15:19:12 seth Exp $
 */

#include "dict.h"

typedef unsigned ht_val ;
typedef ht_val (*ht_func)(void *) ;

struct ht_args
{
	unsigned ht_table_entries ;
	unsigned ht_bucket_entries ;
	ht_func ht_objvalue ;
	ht_func ht_keyvalue ;
} ;



dict_h		ht_create	 (dict_function oo_compare, dict_function ko_compare, int flags, int *errnop, struct ht_args *args )  ;
void 		ht_destroy	 ( dict_h hh )  ;
int 		ht_insert 	 ( dict_h hh, dict_obj obj )  ;
int 		ht_insert_uniq	 ( dict_h hh, dict_obj, dict_obj *objp )  ;
int 		ht_delete	 ( dict_h hh, dict_obj obj )  ;
dict_obj	ht_search	 ( dict_h hh, dict_key key )  ;
dict_obj	ht_minimum	 ( dict_h hh )  ;
dict_obj	ht_maximum	 ( dict_h hh )  ;
dict_obj	ht_successor	 ( dict_h hh, dict_obj )  ;
dict_obj	ht_predecessor	 ( dict_h hh, dict_obj )  ;
void		ht_iterate	 ( dict_h hh, enum dict_direction direction )  ;
dict_obj	ht_nextobj	 ( dict_h hh )  ;

extern int ht_num_table_entries;
extern int ht_num_bucket_entries;


#endif	/* __HT_H */

