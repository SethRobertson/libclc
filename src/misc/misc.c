/*
 * (c) Copyright 1992 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

static const char RCSid[] = "$Id: misc.c,v 1.3 2003/06/17 05:10:52 seth Exp $";
static const char misc_version[] = VERSION;

#include <stdarg.h>
#include <stdlib.h>

/*
 * MISCELLANEOUS FUNCTIONS
 */

#include "misc.h"
#include "string.h"
#include "clchack.h"

#ifndef NULL
#define NULL		0
#endif



/*
 * Create a new argv array,
 * copy the original to the new one,
 * and clear the old one
 */
char **argv_copy_and_clear(char **org_argv, int start, int count)
	                 							/* original argv */
	           									/* from where to start copy/clear */
	           									/* how many entries to copy/clear */
{
	char **new_argv ;
	char *p ;
	int i ;
	int j ;
#ifndef __STDC__
  	char *malloc() ;
#endif

	new_argv = (char **) malloc( count * sizeof( char * ) ) ;
	if ( new_argv == NULL )
		return( NULL ) ;

	for ( i = 0 ; i < count ; i++ )
	{
		new_argv[ i ] = make_string( 1, org_argv[ start+i ] ) ;
		if ( new_argv[ i ] == NULL )
		{
			for ( j = i-1 ; j >= 0 ; j-- )
				free( new_argv[ j ] ) ;
			free( (char *) new_argv ) ;
			return( NULL ) ;
		}
		for ( p = org_argv[ start+i ] ; *p ; p++ )
			*p = ' ' ;
	}
	return( new_argv ) ;
}

#ifndef __linux__
/*
 * We always return a pointer in pathname
 */
char *basename(char *pathname)
{
	char *s = strrchr( pathname, '/' ) ;

	if ( s == NULL )
		return( pathname ) ;
	else
		return( s+1 ) ;
}
#endif /* !__linux__ */


/*
 * We always return a malloced string
 *
 * There are 2 special cases:
 *
 *		1) pathname == "/"
 *				In this case we return "/"
 *		2) pathname does not contain a '/'
 *				In this case we return "."
 */
char *dirname(char *pathname)
{
	int len ;
	char *s = strrchr( pathname, '/' ) ;
	char *p ;
#ifndef __STDC__
  	char *malloc() ;
#endif

	if ( s == NULL )
		return( make_string( 1, "." ) ) ;
	else
	{
		len = s - pathname ;
		if ( len == 0 )
			return( make_string( 1, "/" ) ) ;
	}

	p = (char *) malloc( len+1 ) ;
	if ( p == NULL )
		return( NULL ) ;
	else
	{
		strncpy( p, pathname, len )[ len ] = '\0' ;
		return( p ) ;
	}
}


char *make_string( unsigned count, ...)
{
	va_list ap ;
	register unsigned i ;
	register unsigned len = 0 ;
	register char *s, *p ;
	char *new_string ;

	if ( count == 0 )
		return( NULL ) ;

	va_start( ap, count ) ;
	for ( i = 0 ; i < count ; i++ )
	{
		s = va_arg( ap, char * ) ;
		if ( s == NULL )
			continue ;
		len += strlen( s ) ;
	}
	va_end( ap ) ;

	new_string = (char *) malloc( len + 1 ) ;
	if ( new_string == NULL )
		return( NULL ) ;

	p = new_string ;
	va_start( ap, count ) ;
	for ( i = 0 ; i < count ; i++ )
	{
		s = va_arg( ap, char * ) ;
		if ( s == NULL )
			continue ;
		while (( *p++ = *s++ )) ;
		p-- ;
	}
	va_end( ap ) ;
	return( new_string ) ;
}


char *make_pathname( unsigned count, ... )
{
	va_list ap ;
	register unsigned i ;
	register unsigned len = 0 ;
	register char *s, *p ;
	char *pathname ;

	if ( count == 0 )
		return( NULL ) ;

	va_start( ap, count ) ;
	for ( i = 0 ; i < count ; i++ )
	{
		s = va_arg( ap, char * ) ;
		len += strlen( s ) ;
	}
	va_end( ap ) ;

	pathname = (char *) malloc( len + count ) ;
	if ( pathname == NULL )
		return( NULL ) ;

	p = pathname ;
	va_start( ap , count) ;
	for ( i = 0 ; i < count ; i++ )
	{
		s = va_arg( ap, char * ) ;
		while (( *p++ = *s++ )) ;
		*(p-1) = '/' ;			/* change '\0' to '/' */
	}
	*(p-1) = '\0' ;
	return( pathname ) ;
}

