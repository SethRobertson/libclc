/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */


#include <stdlib.h>

#include "ss_impl.h"
#include "clchack.h"

PRIVATE int kmp_setup(register header_s *hp) ;
PRIVATE char *kmp_match(header_s *hp, char *str, int len) ;
PRIVATE void kmp_done(header_s *hp) ;

struct ss_ops __strs_kmpops = { kmp_setup, kmp_match, kmp_done } ;


PRIVATE void compute_next(header_s *hp)
{
	char			*pattern = SS_PATTERN( hp ) ;
	int			patlen	= SS_PATLEN( hp ) ;
	next_int		*next		= KMP_HEADER( hp )->next ;
	int			q ;
	next_int 	k ;

	k = next[ 0 ] = -1 ;

	for ( q = 0 ; q < patlen-1 ; )
	{
		/*
		 * The invariant of the following loop is:
		 * if k>=0, then
		 *		pattern[ 0..k-1 ] SUFFIX pattern[ 0..q-1 ]  ( <==> next[ q ] = k )
		 * This condition is true on entry to the loop.
		 */
		while ( k >= 0 && pattern[ k ] != pattern[ q ] )
			k = next[ k ] ;

		/*
		 * Case 1: k == -1
		 *		Setting next[ q+1 ] = 0 is ok since it implies that the next
		 *		position in the pattern to check is position 0 (i.e. start
		 *		from the beginning).
		 *	Case 2: k >= 0.
		 *		Since we exited the loop, pattern[ k ] == pattern[ q ].
		 *		Therefore,
		 *			pattern[ 0..k ] SUFFIX pattern[ 0..q ] ==> next[ q+1 ] = k+1
		 */
		k++, q++ ;
#ifdef PATH_COMPRESSION
		if ( pattern[ k ] == pattern[ q ] )
			next[ q ] = next[ k ] ;
#endif
		next[ q ] = k ;
	}
}


PRIVATE int kmp_setup(register header_s *hp)
{
	register next_int *next ;

	next = (next_int *) malloc( (unsigned)SS_PATLEN( hp )*sizeof( next_int ) ) ;
	if ( next == (next_int *)0 )
		return( SS_ERR ) ;

	KMP_HEADER( hp )->next = next ;

	compute_next( hp ) ;

	return( SS_OK ) ;
}


PRIVATE char *kmp_match(header_s *hp, char *str, int len)
{
	register int			i ;
	register next_int 	q ;
	next_int					*next		= KMP_HEADER( hp )->next ;
	char						*pattern = SS_PATTERN( hp ) ;
	register int			patlen	= SS_PATLEN( hp ) ;

	/*
	 * As a special case, we consider pattern[ -1..0 ] to be the empty string.
	 */
	for ( q = 0, i = 0 ; i < len ; i++ )
	{
		register char current_char = SS_MAP( hp, str[ i ] ) ;

again:
		/*
		 * At this point:
		 *		pattern[ 0..q-1 ] is a suffix of str[ 0..i-1 ]
		 */
		if ( pattern[ q ] == current_char )
		{
			q++ ;
			if ( q == patlen )
				return( &str[ i - patlen + 1 ] ) ;
		}
		else
		{
			/*
			 * Let q' = next[ q ]. If q' >= 0, then
			 *		pattern[ 0..q'-1 ] SUFFIX pattern[ 0..q-1 ]
			 *	which implies that
			 *		pattern[ 0..q'-1 ] SUFFIX str[ 0..i-1 ]
			 * Therefore, it is ok to set q = q'.
			 */
			q = next[ q ] ;
			if ( q >= 0 )
				goto again ;
			q++ ;
		}
	}

	return( CHAR_NULL ) ;
}


PRIVATE void kmp_done(header_s *hp)
{
	(void) free( (char *)KMP_HEADER( hp )->next ) ;
}

