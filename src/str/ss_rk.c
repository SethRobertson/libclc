/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */


#include "ss_impl.h"
#include "clchack.h"

/*
 * Multiply a number by the radix we are using
 */
#define RADIX_MULT( n )						( (n) << NBIC )

#define UCHAR( c )							((unsigned char)(c))


PRIVATE int rk_setup(register header_s *hp) ;
PRIVATE char *rk_match(register header_s *hp, char *str, int len) ;
PRIVATE void rk_done(header_s *hp) ;

struct ss_ops __strs_rkops = { rk_setup, rk_match, rk_done } ;


PRIVATE int rk_setup(register header_s *hp)
{
	register int		i ;
	struct rk_header	*rkp		= RK_HEADER( hp ) ;
	u_wide_int			patval	= 0 ;
	u_wide_int			digit_1	= 1 ;

	/*
	 * Compute pattern value
	 */
	for ( i = 0 ; i < SS_PATLEN( hp ) ; i++ )
		patval = ( RADIX_MULT( patval ) + UCHAR( SS_PATTERN( hp )[i] ) ) % PRIME ;

	for ( i = 0 ; i < SS_PATLEN( hp )-1 ; i++ )
		digit_1 = RADIX_MULT( digit_1 ) % PRIME ;

	rkp->rk_patval = patval ;
	rkp->rk_digit1 = digit_1 ;
	return( SS_OK ) ;
}


PRIVATE void rk_done(header_s *hp)
{
#ifdef lint
	hp = hp ;
#endif
}


PRIVATE char *rk_match(register header_s *hp, char *str, int len)
{
	register int				i ;
	register unsigned char	uc ;
	register u_wide_int		strval	= 0 ;
	register u_wide_int		patval	= RK_HEADER( hp )->rk_patval ;
	register u_wide_int		digit_1	= RK_HEADER( hp )->rk_digit1 ;
	int							patlen	= SS_PATLEN( hp ) ;
	char							*endpat	= &SS_PATTERN( hp )[ patlen ] ;

	/*
	 * Calculate initial value of 'str'
	 * Note that we are guaranteed that len >= pattern length
	 */
	for ( i = 0 ; i < patlen ; i++ )
	{
		uc = UCHAR( SS_MAP( hp, str[i] ) ) ;
		strval = ( RADIX_MULT( strval ) + uc ) % PRIME ;
	}

	for ( i = 0 ;; i++ )
	{
		register u_wide_int t1 ;

		if ( strval == patval )
		{
			char *pp, *sp ;

			for ( pp = SS_PATTERN( hp ), sp = &str[i] ;; sp++, pp++ )
			{
				if ( pp == endpat )
					return( &str[i] ) ;
				if ( *pp != SS_MAP( hp, *sp ) )
					break ;
			}
		}

		if ( i == len-patlen+1 )
			break ;

		/*
		 * The formula we evaluate is:
		 *
		 *	strval = ( RADIX_MULT( ( strval - UCHAR( str[i] )*digit_1 ) ) +
		 *						UCHAR( str[i+patlen] ) ) % PRIME ;
		 *
		 * We have to make sure that the subtraction does not produce
		 * a negative number since that causes strval to be wrong.
		 */
		uc = UCHAR( SS_MAP( hp, str[i] ) ) ;
		t1 = ( uc * digit_1 ) % PRIME ;
		if ( t1 > strval )
			strval += PRIME ;
		uc = UCHAR( SS_MAP( hp, str[i+patlen] ) ) ;
		strval = ( RADIX_MULT( strval - t1 ) + uc ) % PRIME ;
	}
	return( NULL ) ;
}


