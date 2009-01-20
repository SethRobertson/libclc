/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

UNUSED static char RCSid[] = "$Id: ss_sbm.c,v 1.2 2003/06/17 05:10:55 seth Exp $" ;

#include <stdlib.h>

#include "ss_impl.h"
#include "ss_sbm.h"
#include "clchack.h"

PRIVATE int sbm_setup(header_s *hp) ;
PRIVATE char *sbm_match(header_s *hp, char *str, int len) ;
PRIVATE void sbm_done(header_s *hp) ;


struct ss_ops __strs_sbmops = { sbm_setup, sbm_match, sbm_done } ;


PRIVATE int sbm_setup(header_s *hp)
{
	last_int			*last_occurrence ;
	register int	i ;
	int				patlen	= SS_PATLEN( hp ) ;
	char				*pattern = SS_PATTERN( hp ) ;

	last_occurrence = (last_int *) malloc( ALPHABET_SIZE * sizeof( last_int ) ) ;
	if ( last_occurrence == (last_int *)0 )
		return( SS_ERR ) ;

	for ( i = 0 ; i < ALPHABET_SIZE ; i++ )
		last_occurrence[ i ] = -1 ;
	for ( i = 0 ; i < patlen ; i++ )
		last_occurrence[ (unsigned char) pattern[ i ] ] = i ;

	SBM_HEADER( hp )->last_occurrence = last_occurrence ;
	return( SS_OK ) ;
}



PRIVATE char *sbm_match(header_s *hp, char *str, int len)
{
	register int	j ;
	register int	s						= 0 ;
	char				*pattern				= SS_PATTERN( hp ) ;
	int				patlen				= SS_PATLEN( hp ) ;
	last_int			*last_occurrence	= SBM_HEADER( hp )->last_occurrence ;

	while ( s <= len - patlen )
	{
		register char c ;
		last_int lo ;

		/*
		 * Try matching pattern right-to-left
		 */
		for ( j = patlen-1 ;; )
		{
			c = SS_MAP( hp, str[ s+j ] ) ;
			if ( pattern[ j ] == c )
				if ( j )
					j-- ;
				else
					return( &str[ s ] ) ;
			else
				break ;
		}
		lo = last_occurrence[ (unsigned char) c ] ;
		if ( j > lo )
			s += j - lo ;
		else
			s++ ;
	}
	return( CHAR_NULL ) ;
}


PRIVATE void sbm_done(header_s *hp)
{
	(void) free( (char *) SBM_HEADER( hp )->last_occurrence ) ;
}

