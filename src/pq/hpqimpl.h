/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

/*
 * $Id: hpqimpl.h,v 1.3 2001/07/07 13:41:16 seth Exp $
 */

#include "hpq.h"

#define HHP( p )			((header_s *)p)

#define HANDLE_ERROR( header, retval, errval, msg )	\
		do {					\
		  if (header)				\
		    (header)->dicterrno = errval ;		\
                  else					\
		    pq_errno = errval ;			\
		  return( retval ) ;			\
		} while (0)


