/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

static char RCSid[] = "$Id: ss_so.c,v 1.2 2003/06/17 05:10:55 seth Exp $" ;

#include <stdlib.h>

#include "ss_impl.h"
#include "clchack.h"

PRIVATE int so_setup(header_s *hp) ;
PRIVATE char *so_match(register header_s *hp, char *str, int len) ;
PRIVATE void so_done(header_s *hp) ;

struct ss_ops __strs_soops = { so_setup, so_match, so_done } ;

/*
 * There is a single twist in this implementation of the shift-or algorithm:
 * To make the check for complete match faster, we are using the sign-bit
 * of the word. This means that everything is shifted to the left by
 * 			(word_size - pattern_length)
 */

PRIVATE int so_setup(header_s *hp)
{
	register wide_int		*maskbuf ;
	register wide_int		mask ;
	register wide_int		offset_mask ;
	register int			i ;
	int						offset ;
	register int			patlen	= SS_PATLEN( hp ) ;
	register char			*pattern = SS_PATTERN( hp ) ;

	if ( patlen > WIDE_INT_SIZE )
		return( SS_ERR ) ;

	maskbuf = (wide_int *) malloc( ALPHABET_SIZE * sizeof( wide_int ) ) ;
	if ( maskbuf == (wide_int *)NULL )
		return( SS_ERR ) ;

	offset = WIDE_INT_SIZE - patlen ;
	offset_mask = ( (~0) << offset ) ;

	/*
	 * The bits of each word that won't be used must be set to 0
	 */
	for ( i = 0 ; i < ALPHABET_SIZE ; i++ )
		maskbuf[ i ] = offset_mask ;

	for ( i = 0, mask = 1 << offset ; i < patlen ; i++, mask <<= 1 )
		maskbuf[ (unsigned char) pattern[ i ] ] &= ~mask ;

	SO_HEADER( hp )->mask = maskbuf ;
	SO_HEADER( hp )->offset_mask = offset_mask ;
	return( SS_OK ) ;
}


PRIVATE char *so_match(register header_s *hp, char *str, int len)
{
	register char			*p ;
	register char			pfc				= SS_PATTERN( hp )[ 0 ] ;
	register wide_int		*mask				= SO_HEADER( hp )->mask ;
	register char			*endmatch		= &str[ len - SS_PATLEN( hp ) + 1 ] ;
	char						*endstr			= &str[ len ] ;
	wide_int					no_match_state = ~0 & SO_HEADER( hp )->offset_mask ;

	/*
	 * The shift-or algorithm can be described by the following for-loop:
	 *
	 *	for ( p = str ; p < endstr ; p++ )
	 *	{
	 *		state = ( state << 1 ) | mask[ (unsigned char) SS_MAP( hp, *p ) ] ;
	 *		if ( state >= 0 )
	 *			return( &p[ -SS_PATLEN( hp ) + 1 ] ) ;
	 *	}
	 *
	 * For efficiency reasons, the algorithm is used only after the first
	 * character of the pattern is matched against a character of the string.
	 */

	for ( p = str ; p < endmatch ; p++ )
	{
		register wide_int state ;

		if ( SS_MAP( hp, *p ) != pfc )
			continue ;

		for ( state = no_match_state ; p < endstr ; p++ )
		{
			state = ( state << 1 ) | mask[ (unsigned char) SS_MAP( hp, *p ) ] ;
			if ( state >= 0 )
				return( &p[ -SS_PATLEN( hp ) + 1 ] ) ;
			if ( state == no_match_state )
				break ;
		}
	}
	return( CHAR_NULL ) ;
}


PRIVATE void so_done(header_s *hp)
{
	(void) free( (char *) SO_HEADER( hp )->mask ) ;
}

