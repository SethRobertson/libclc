/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef __DLL_H
#define __DLL_H

/*
 * $Id: dll.h,v 1.1 2001/05/26 22:04:51 seth Exp $
 */

#include "dict.h"

dict_h dll_create
	__ARGS( (
					dict_function oo_compare,
					dict_function ko_compare,
					int flags,
					int *errnop
			) ) ;
void 		dll_destroy			__ARGS( ( dict_h lh ) ) ;
int 		dll_insert			__ARGS( ( dict_h lh, dict_obj obj ) ) ;
int 		dll_insert_uniq 	__ARGS( ( dict_h lh, dict_obj, dict_obj * ) ) ;
int 		dll_append			__ARGS( ( dict_h lh, dict_obj obj ) ) ;
int 		dll_append_uniq 	__ARGS( ( dict_h lh, dict_obj, dict_obj * ) ) ;
int 		dll_delete 			__ARGS( ( dict_h lh, dict_obj obj ) ) ;
dict_obj dll_search 			__ARGS( ( dict_h lh, dict_key key ) ) ;
dict_obj dll_minimum 		__ARGS( ( dict_h lh ) ) ;
dict_obj dll_maximum 		__ARGS( ( dict_h lh ) ) ;
dict_obj dll_successor 		__ARGS( ( dict_h lh, dict_obj obj ) ) ;
dict_obj dll_predecessor 	__ARGS( ( dict_h lh, dict_obj obj ) ) ;
void 		dll_iterate			__ARGS( ( dict_h lh, enum dict_direction ) ) ;
dict_obj dll_nextobj			__ARGS( ( dict_h lh ) ) ;

#endif 	/* __DLL_H */

