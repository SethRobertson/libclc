/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

/*
 * $Id: hpqimpl.h,v 1.1 2001/05/26 22:04:50 seth Exp $
 */

#include "hpq.h"

#define HHP( p )			((header_s *)p)

#define HANDLE_ERROR( flags, retval, errp, errval, msg )		\
			do { \
				if ( flags & PQ_RETURN_ERROR )						\
				{																\
					*errp = errval ;										\
					return( retval ) ;									\
				}																\
				else															\
				{																\
					char *s = msg ;										\
																				\
					(void) write( 2, s, strlen( s ) ) ;				\
					abort() ;												\
					_exit( 1 ) ; 											\
					/* NOTREACHED */										\
				} \
			} while (0)


