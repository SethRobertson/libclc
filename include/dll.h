/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef __DLL_H
#define __DLL_H

/*
 * $Id: dll.h,v 1.2 2001/07/05 15:19:12 seth Exp $
 */

#include "dict.h"

dict_h	 dll_create		 (dict_function oo_compare, dict_function ko_compare, int flags, int *errnop) ;
void	 dll_destroy		 ( dict_h lh )  ;
int 	 dll_insert		 ( dict_h lh, dict_obj obj )  ;
int 	 dll_insert_uniq 	 ( dict_h lh, dict_obj, dict_obj * )  ;
int 	 dll_append		 ( dict_h lh, dict_obj obj )  ;
int 	 dll_append_uniq 	 ( dict_h lh, dict_obj, dict_obj * )  ;
int	 dll_delete 		 ( dict_h lh, dict_obj obj )  ;
dict_obj dll_search 		 ( dict_h lh, dict_key key )  ;
dict_obj dll_minimum 		 ( dict_h lh )  ;
dict_obj dll_maximum 		 ( dict_h lh )  ;
dict_obj dll_successor 		 ( dict_h lh, dict_obj obj )  ;
dict_obj dll_predecessor 	 ( dict_h lh, dict_obj obj )  ;
void	 dll_iterate		 ( dict_h lh, enum dict_direction )  ;
dict_obj dll_nextobj		 ( dict_h lh )  ;

#endif 	/* __DLL_H */

