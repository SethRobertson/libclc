/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */


#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "ss_impl.h"
#include "clchack.h"

/*
 * NOTE: The brute force method (with the __strs_bfops) must be always
 * 		available so that we can switch to it if another method fails.
 */
extern struct ss_ops __strs_bfops ;
extern struct ss_ops __strs_rkops ;
extern struct ss_ops __strs_kmpops ;
extern struct ss_ops __strs_sbmops ;
extern struct ss_ops __strs_bmhops ;
extern struct ss_ops __strs_soops ;

/*
 * NOTE: This table is arranged according to increasing method number.
 *			This allows quick indexing into it using the user-provided
 *			method as a hint:
 *				if ( selection_table[ user_method ].method == user_method )
 *					FOUND
 *				else
 *					DO SEQUENTIAL SEARCH
 *			This allows both quick access and a change of method numbers
 *			in the future without requiring recompilation of programs in
 *			order to work with new versions of the library.
 */
static struct ss_select selection_table[] =
	{
		{ STRS_BF,					&__strs_bfops		},
		{ STRS_RK,					&__strs_rkops		},
		{ STRS_KMP,					&__strs_kmpops		},
		{ STRS_SBM,					&__strs_sbmops		},
		{ STRS_BMH,					&__strs_bmhops		},
		{ STRS_SO,					&__strs_soops		},
		{ 0,							0						}
	} ;

static char identity_map[ ALPHABET_SIZE ] ;
static char upper_to_lower_map[ ALPHABET_SIZE ] ;

static int tables_initialized ;

/*
 * This header is returned when an empty pattern is given to strs_setup.
 * The rest of the functions check ss_patlen and do nothing if that is zero.
 * ss_patlen in this header will be initialized to zero.
 */
static header_s empty_pattern_header ;


PRIVATE void initialize_tables(void)
{
	int i ;

	for ( i = 0 ; i < sizeof( upper_to_lower_map ) ; i++ )
	{
		if ( isascii( i ) && isupper( i ) )
			upper_to_lower_map[ i ] = i + 'a' - 'A' ;
		else
			upper_to_lower_map[ i ] = i ;
		identity_map[ i ] = i ;
	}
}


/*
 * Initializes header
 *
 * Note that 'pattern' does not need to be a NUL-terminated string.
 */
PRIVATE int init(register header_s *hp, int flags, char *pattern, int patlen)
{
	int requested_method = SS_GETMETHOD( flags ) ;
	register struct ss_select *selp ;

	if ( ! tables_initialized )
	{
		initialize_tables() ;
		tables_initialized = TRUE ;
	}

	/*
	 * Initialize header fields
	 */
	SS_FLAGS( hp ) = SS_GETFLAGS( flags ) ;
	SS_PATLEN( hp ) = patlen ;
	if ( SS_SWITCH( hp ) && patlen < 4 )
		SS_OPS( hp ) = &__strs_bfops ;		/* brute force */
	else
	{
		/*
		 * Determine ops
		 */
		if ( selection_table[ requested_method ].sel_method == requested_method )
			selp = &selection_table[ requested_method ] ;
		else
			for ( selp = &selection_table[ 0 ] ; selp->sel_ops ; selp++ )
				if ( requested_method == selp->sel_method )
					break ;
		if ( selp->sel_ops )
			SS_OPS( hp ) = selp->sel_ops ;
		else if ( SS_SWITCH( hp ) )
			SS_OPS( hp ) = &__strs_bfops ;		/* brute force */
		else
			return( SS_ERR ) ;
	}

	if ( SS_MALLOC( hp ) )
	{
		SS_PATTERN( hp ) = malloc( (unsigned)SS_PATLEN( hp ) ) ;
		if ( SS_PATTERN( hp ) == CHAR_NULL )
		{
			(void) free( (char *)hp ) ;
			return( SS_ERR ) ;
		}
		(void) memcpy( SS_PATTERN( hp ), pattern, (int)SS_PATLEN( hp ) ) ;
	}
	else
		SS_PATTERN( hp ) = pattern ;

	/*
	 * If the user asked for case-insensitive search, we create our own
	 * copy of the pattern in lower case. If the pattern is malloc'ed
	 * we overwrite, otherwise we malloc some memory and clear the
	 * STRS_NOMALLOC flag.
	 */
	if ( SS_IGNCASE( hp ) )
	{
		char *new_pattern ;
		register int i ;

		SS_SETMAP( hp, upper_to_lower_map ) ;

		if ( SS_MALLOC( hp ) )
			new_pattern = SS_PATTERN( hp ) ;
		else
		{
			new_pattern = malloc( (unsigned)SS_PATLEN( hp ) + 1 ) ;
			if ( new_pattern == CHAR_NULL )
				return( SS_ERR ) ;
			SS_SETMALLOC( hp ) ;			/* clears the STRS_NOMALLOC flag */
		}
		for ( i = 0 ; i < SS_PATLEN( hp ) ; i++ )
			new_pattern[ i ] = SS_MAP( hp, SS_PATTERN( hp )[ i ] ) ;
		SS_PATTERN( hp ) = new_pattern ;
	}
	else
		SS_SETMAP( hp, identity_map ) ;

	for ( ;; )
	{
		if ( SS_SETUP( hp ) == SS_OK )
			return( SS_OK ) ;
		else
		{
			if ( ! SS_SWITCH( hp ) || SS_OPS( hp ) == &__strs_bfops )
			{
				if ( SS_MALLOC( hp ) )
					(void) free( (char *)hp ) ;
				return( SS_ERR ) ;
			}
			SS_OPS( hp ) = &__strs_bfops ;
		}
	}
}


/*
 * Finalize header
 */
PRIVATE void fini(header_s *hp)
{
	SS_DONE( hp ) ;
	if ( SS_MALLOC( hp ) )
		(void) free( SS_PATTERN( hp ) ) ;
}


/*
 * Create a search handle
 */
strs_h strs_setup( int flags, char *pattern, ... )
{
	header_s		*hp ;
	int			patlen ;
	va_list		ap ;

	hp = HP( malloc( sizeof( *hp ) ) ) ;
	if ( hp == NULL )
		return( NULL_HANDLE ) ;

	if ( flags & STRS_PATLEN )
	{
		va_start( ap, pattern ) ;
		patlen = va_arg( ap, int ) ;
		va_end( ap ) ;
	}
	else
		patlen = strlen( pattern ) ;

	if ( patlen == 0 )
		return( (strs_h) &empty_pattern_header ) ;

	if ( init( hp, flags, pattern, patlen ) == SS_OK )
		return( (strs_h)hp ) ;
	else
	{
		free( (char *)hp ) ;
		return( NULL_HANDLE ) ;
	}
}


/*
 * Destroy a search handle
 */
void strs_done(strs_h handle)
{
	header_s *hp = HP( handle ) ;

	if ( SS_PATLEN( hp ) != 0 )
	{
		fini( hp ) ;
		(void) free( (char *) handle ) ;
	}
}


char *strs_match(strs_h handle, char *str, int len)
{
	register header_s *hp = HP( handle ) ;

	if ( SS_PATLEN( hp ) == 0 )
		return( str ) ;
	if ( SS_PATLEN( hp ) > len )
		return( CHAR_NULL ) ;
	return( SS_MATCH( hp, str, len ) ) ;
}



char *strs_search( int flags, char *str, int len, char *pattern, ... )
{
	header_s		t_header ;
	char			*p ;
	int			patlen ;
	va_list		ap ;

	if ( flags & STRS_PATLEN )
	{
		va_start( ap, pattern ) ;
		patlen = va_arg( ap, int ) ;
		va_end( ap ) ;
	}
	else
		patlen = strlen( pattern ) ;

	if ( patlen == 0 )
		return( str ) ;

	if ( patlen > len )
		return( CHAR_NULL ) ;

	if ( init( &t_header, flags | STRS_NOMALLOC, pattern, patlen ) == SS_OK )
	{
		p = SS_MATCH( &t_header, str, len ) ;
		fini( &t_header ) ;
		return( p ) ;
	}
	else
		return( CHAR_NULL ) ;
}


