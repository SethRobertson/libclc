/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */



#include <ctype.h>
#include "str.h"
#include "clchack.h"

#ifndef NULL
#define NULL				0
#endif


#ifndef TRIVIAL_STR_FIND

/*
 * look for an instance of s2 in s1
 * Returns a pointer to the beginning of s2 in s1
 */
char *str_find(register char *str, register char *sstr)
{
   register int ssfc = *sstr++ ;    /* sub-string first char */

	if ( ssfc == 0 )			/* empty string is always a match */
		return( str ) ;

	while ( *str )
	{
		char *current = str ;
		register int strc = *str++ ;
		register char *sp ;                    /* string pointer */
		register char *ssp ;                   /* sub-string pointer */

		if ( strc != ssfc )
			continue ;
	
		/*
		 * We don't need to make the end of str a special case since
		 * the comparison of *sp against *ssp is guaranteed to fail
		 */
		for ( sp = str, ssp = sstr ;; sp++, ssp++ )
		{
			if ( *ssp == 0 )
				return( current ) ;
			if ( *sp != *ssp )
				break ;
		}
	}

	return( 0 ) ;
}


#define LOWER_CASE( c )					( (c) + 'a' - 'A' )

/*
 * str_casefind is similar to str_find except that it ignores the
 * case of the alphabetic characters
 */
char *str_casefind(register char *str, char *sstr)
{
	register int ssfc = *sstr++ ;		/* sub-string first char */

	if ( ssfc == 0 )
		return( str ) ;

	if ( isalpha( ssfc ) && isupper( ssfc ) )
		ssfc = LOWER_CASE( ssfc ) ;

	while ( *str )
	{
		char *current = str ;
		register int strc = *str++ ;
		char *sp ;							/* string pointer */
		char *ssp ;							/* sub-string pointer */

		if ( isalpha( strc ) && isupper( strc ) )
			strc = LOWER_CASE( strc ) ;
		if ( strc != ssfc )
			continue ;
	
		for ( sp = str, ssp = sstr ;; sp++, ssp++ )
		{
			register int sc = *sp ;				/* string char */
			register int ssc = *ssp ;			/* substring char */

			/*
			 * End-of-substring means we got a match
			 */
			if ( ssc == 0 )
				return( current ) ;

			/*
			 * Convert to lower case if alphanumeric
			 */
			if ( isalpha( sc ) && isupper( sc ) )
				sc = LOWER_CASE( sc ) ;
			if ( isalpha( ssc ) && isupper( ssc ) )
				ssc = LOWER_CASE( ssc ) ;
			if ( sc != ssc )
				break ;
		}
	}

	return( 0 ) ;
}


#else		/* defined( TRIVIAL_STR_FIND ) */

/*
 * look for an instance of s2 in s1
 * Returns a pointer to the beginning of s2 in s1
 */
char *str_find( s1, s2 )
	char *s1 ;
	char *s2 ;
{
   int i ;
   int l1 = strlen( s1 ) ;
   int l2 = strlen( s2 ) ;

   for ( i = 0 ; i < l1 - l2 + 1 ; i++ )
      if ( strncmp( &s1[ i ], s2, l2 ) == 0 )
         return( &s1[ i ] ) ;
   return( NULL ) ;
}


char *str_casefind( s1, s2 )
	char *s1 ;
	char *s2 ;
{
   int i ;
   int l1 = strlen( s1 ) ;
   int l2 = strlen( s2 ) ;

   for ( i = 0 ; i < l1 - l2 + 1 ; i++ )
      if ( strncasecmp( &s1[ i ], s2, l2 ) == 0 )
         return( &s1[ i ] ) ;
   return( NULL ) ;
}

#endif 	/* TRIVIAL_STR_FIND */


/*
 * Fill string s with character c
 */
void str_fill(register char *s, register char c)
{
	while ( *s ) *s++ = c ;
}


char *str_lower(char *s)
{
	register char *p ;
	register int offset = 'a' - 'A' ;

	for ( p = s ; *p ; p++ )
		if ( isascii( *p ) && isupper( *p ) )
			*p += offset ;
	return( s ) ;
}


char *str_upper(char *s)
{
	register char *p ;
	register int offset = 'a' - 'A' ;

	for ( p = s ; *p ; p++ )
		if ( isascii( *p ) && islower( *p ) )
			*p -= offset ;
	return( s ) ;
}


