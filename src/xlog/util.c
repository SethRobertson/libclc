/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */


#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "clchack.h"

extern int sys_nerr;
extern char *sys_errlist[];

#define NUL						'\0'
#define NULL					0


/*
 * Search the given buffer for an occurrence of "%m"
 */
char *__xlog_add_errno(char *buf, int len)
{
	register char *s ;

	for ( s = buf ; s < &buf[ len-1 ] ; s++ )
		if ( *s == '%' && *(s+1) == 'm' )
			return( s ) ;
	return( NULL ) ;
}



char *__xlog_explain_errno(char *buf, unsigned int *size)
{
	register int len ;

	if ( errno < sys_nerr )
	{

		(void) strncpy( buf, sys_errlist[ errno ], (int)*size ) ;
		for ( len = 0 ; len < *size ; len++ )
			if ( buf[ len ] == NUL )
				break ;
		*size = len ;
	}
	else
		len = strx_nprint( buf, *size, "errno = %d", errno ) ;
	return( buf ) ;
}


char *__xlog_new_string(char *s)
{
	unsigned size = strlen( s ) + 1 ;
	char *p = malloc( size ) ;
	char *strcpy(char *, const char *) ;

	return( ( p != NULL ) ? strcpy( p, s ) : p ) ;
}


