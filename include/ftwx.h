/*
 * (c) Copyright 1992 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */


#ifndef __FTWX_H
#define __FTWX_H

/*
 * $Id: ftwx.h,v 1.2 2003/06/17 05:10:50 seth Exp $
 */

#ifndef __FTWX_NO_FTW
#include <ftw.h>
#else
#define FTW_F   0
#define FTW_D   1
#define FTW_DNR 2
#define FTW_NS  3
#endif


/*
 * Flags
 */
#define FTWX_ALL        -1
#define FTWX_FOLLOW     0x1


#ifdef __ARGS
#undef _ARGS
#endif

#ifdef PROTOTYPES
#  define __ARGS( s )               s
#else
#  define __ARGS( s )               ()
#endif

int ftwx __ARGS( ( char *path, int (*func)(), int depth, int flags ) ) ;

#endif 	/* __FTWX_H */

