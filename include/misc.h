/*
 * (c) Copyright 1992 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */


#ifndef __MISC_H
#define __MISC_H


/*
 * $Id: misc.h,v 1.2 2003/06/17 05:10:50 seth Exp $
 */

/*
 * Align to a power of 2
 */
#define align2( num, al )					(((num)+(al)-1) & ~((al)-1))

#ifdef __ARGS
#undef __ARGS
#endif

#ifdef PROTOTYPES
#   define __ARGS( s )               s
#else
#   define __ARGS( s )               ()
#endif


char *make_string __ARGS( ( unsigned count, ... ) ) ;
char *make_pathname __ARGS( ( unsigned count, ... ) ) ;

char **argv_copy_and_clear __ARGS( ( char **argv, int start, int count ) ) ;
char *dirname __ARGS( ( char *pathname ) ) ;
#ifndef __linux__
char *basename __ARGS( ( char *pathname ) ) ;
#endif /* !__linux__ */

#endif 	/* __MISC_H */

