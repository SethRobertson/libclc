/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

static char RCSid[] = "$Id" ;

#include <stdlib.h>

#include "ss_impl.h"
#include "clchack.h"

PRIVATE int bmh_setup(header_s *hp) ;
PRIVATE char *bmh_match(header_s *hp, register char *str, int len) ;
PRIVATE void bmh_done(header_s *hp) ;

struct ss_ops __strs_bmhops = { bmh_setup, bmh_match, bmh_done } ;


PRIVATE int bmh_setup(header_s *hp)
{
	register int	patlen	= SS_PATLEN( hp ) ;
	register int	limit		= patlen - 1 ;			/* patlen is > 0 */
	register char	*pattern = SS_PATTERN( hp ) ;
	register int	i ;
	shift_int		*shift ;

	shift = (shift_int *) malloc( ALPHABET_SIZE * sizeof( shift_int ) ) ;
	if ( shift == (shift_int *)NULL )
		return( SS_ERR ) ;

	for ( i = 0 ; i < ALPHABET_SIZE ; i++ )
		shift[ i ] = patlen ;

	for ( i = 0 ; i < limit ; i++ )
		shift[ (unsigned char) pattern[ i ] ] = limit - i ;

	BMH_HEADER( hp )->shift = shift ;
	return( SS_OK ) ;
}


PRIVATE char *bmh_match(header_s *hp, register char *str, int len)
{
	register int	i ;
	int				patlen	= SS_PATLEN( hp ) ;
	char				*pattern = SS_PATTERN( hp ) ;
	register char	lpc		= pattern[ patlen-1 ] ;	/* last pattern character */
	shift_int		*shift	= BMH_HEADER( hp )->shift ;

	i = patlen - 1 ;
	while ( i < len )
	{
		char c = SS_MAP( hp, str[ i ] ) ;

		if ( c == lpc )
		{
			int j, k ;

			for ( j = patlen-1, k = i ;; )
			{
				if ( j == 0 )
					return( &str[ k ] ) ;
				j--, k-- ;
				if ( pattern[ j ] != SS_MAP( hp, str[ k ] ) )
					break ;
			}
		}
		i += shift[ (unsigned char) c ] ;
	}
	return( CHAR_NULL ) ;
}


PRIVATE void bmh_done(header_s *hp)
{
	(void) free( (char *)BMH_HEADER( hp )->shift ) ;
}

