/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

#ifndef __HT_H
#define __HT_H

/*
 * $Id: ht.h,v 1.9 2003/06/17 05:10:50 seth Exp $
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
#define HT_ARG_COPY_SIZE (sizeof(unsigned) + sizeof(unsigned) + sizeof(ht_func) + sizeof(ht_func))
#ifdef HASH_STATS
  struct ht_stats *ht_stats ;
#endif
} ;



dict_h		ht_create	 (dict_function oo_compare, dict_function ko_compare, int flags, struct ht_args *args )  ;
void 		ht_destroy	 ( dict_h hh )  ;
int 		ht_insert 	 ( dict_h hh, dict_obj obj )  ;
int 		ht_insert_uniq	 ( dict_h hh, dict_obj, dict_obj *objp )  ;
#define		ht_append ht_insert
#define		ht_append_uniq ht_insert_uniq
int 		ht_delete	 ( dict_h hh, dict_obj obj )  ;
dict_obj	ht_search	 ( dict_h hh, dict_key key )  ;
dict_obj	ht_minimum	 ( dict_h hh )  ;
dict_obj	ht_maximum	 ( dict_h hh )  ;
dict_obj	ht_successor	 ( dict_h hh, dict_obj )  ;
dict_obj	ht_predecessor	 ( dict_h hh, dict_obj )  ;
dict_iter	ht_iterate	 ( dict_h hh, enum dict_direction direction )  ;
void		ht_iterate_done	 ( dict_h hh, dict_iter )  ;
dict_obj	ht_nextobj	 ( dict_h hh, dict_iter )  ;
char *	 	ht_error_reason	 ( dict_h hh, int *errnop )  ;

#endif	/* __HT_H */

