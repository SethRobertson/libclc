/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */


#ifndef __DICTIMPL_H
#define __DICTIMPL_H

/*
 * $Id: dictimpl.h,v 1.2 2001/07/07 02:58:22 seth Exp $
 */

#include "dict.h"

extern int dict_errno;

struct dict_header
{
	dict_function	oo_comp ;
	dict_function	ko_comp ;
	int		flags ;
	int		errno ;
} ;

typedef struct dict_header dheader_s ;

typedef int bool_int ;

#define ERRNO( dhp )					((dhp)->errno)

#ifndef NULL
#define NULL 0
#endif

#define INT_NULL							((int *)0)

#define NULL_OBJ							((dict_obj)NULL)
#define NULL_HANDLE						((dict_h)NULL)
#define NULL_FUNC							((dict_function)NULL)

#define PRIVATE                     static

#ifndef FALSE
#define FALSE                       0
#define TRUE                        1
#endif

#define ORDER_FLAGS						( DICT_ORDERED + DICT_UNORDERED )

#define BAD_ORDER( flags )				( ( flags & ORDER_FLAGS ) == ORDER_FLAGS )


#define HANDLE_ERROR( dhp, id, errval, retval )		\
	do {						\
	  ERRNO( dhp ) = errval ;			\
	  return( retval ) ;				\
	} while (0)

int __dict_args_ok(char *caller, int flags, dict_function oo_comp, dict_function ko_comp, int allowed_orders);
void __dict_init_header(dheader_s *dhp, dict_function oo_comp, dict_function ko_comp, int flags) ;
dict_h __dict_create_error(char *caller, int flags, int error_code) ;
char *__dict_error_reason(int errno) ;

#endif	/* __DICTIMPL_H */

