/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */


#ifndef __DICTIMPL_H
#define __DICTIMPL_H

/*
 * $Id: dictimpl.h,v 1.8 2004/06/07 22:01:27 jtt Exp $
 */

#include "dict.h"

extern int dict_errno;

struct dict_header
{
	dict_function	oo_comp ;
	dict_function	ko_comp ;
	int		flags ;
	int		dicterrno ;
} ;

typedef struct dict_header dheader_s ;

typedef int bool_int ;

#define ERRNO( dhp )			((dhp)->dicterrno)

#ifndef NULL
#define NULL 0
#endif

#define INT_NULL			((int *)NULL)
#define NULL_OBJ			((dict_obj)NULL)
#define NULL_HANDLE			((dict_h)NULL)
#define NULL_FUNC			((dict_function)NULL)

#define PRIVATE                     static

#ifndef FALSE
#define FALSE                       0
#define TRUE                        1
#endif

#define ORDER_FLAGS		( DICT_ORDERED + DICT_UNORDERED )

#define BAD_ORDER( flags )	( ( flags & ORDER_FLAGS ) == ORDER_FLAGS )


#define HANDLE_ERROR( dhp, errval, retval )		\
	do {						\
	  ERRNO( dhp ) = errval ;			\
	  return( retval ) ;				\
	} while (0)

int __dict_args_ok(char *caller, int flags, dict_function oo_comp, dict_function ko_comp, int allowed_orders, int *fsmaflags);
void __dict_init_header(dheader_s *dhp, dict_function oo_comp, dict_function ko_comp, int flags) ;
dict_h __dict_create_error(char *caller, int flags, int error_code) ;

#ifdef __INSURE__
#define realloc(p,l) ((p)?realloc((p),(l)):malloc(l))
#endif /* __INSURE__ */

#endif	/* __DICTIMPL_H */

