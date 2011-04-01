/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */



#include <ctype.h>

#include "sio.h"
#include "impl.h"

#ifdef _WIN32
#include <stdarg.h>
#endif /* _WIN32 */
#include "clchack.h"

#ifndef WIDE_INT
#define WIDE_INT		long
#endif

typedef WIDE_INT 		wide_int ;
typedef unsigned WIDE_INT 	u_wide_int ;
typedef int 			bool_int ;

#define S_NULL			"(null)"
#define S_NULL_LEN		6
#define FLOAT_DIGITS		6
#define EXPONENT_LENGTH		10

/*
 * NUM_BUF_SIZE is the size of the buffer used for arithmetic conversions
 *
 * XXX: this is a magic number; do not decrease it
 */
#define NUM_BUF_SIZE						512

PRIVATE char *conv_10( register wide_int, register bool_int, register bool_int *, char *, register int *);

/*
 * The INS_CHAR macro inserts a character in the buffer and writes
 * the buffer back to disk if necessary
 * It uses the char pointers sp and bep:
 * 	sp points to the next available character in the buffer
 * 	bep points to the end-of-buffer+1
 * While using this macro, note that the nextb pointer is NOT updated.
 *
 * No I/O is performed if fd is not positive. Negative fd values imply
 * conversion with the output directed to a string. Excess characters
 * are discarded if the string overflows.
 *
 * NOTE: Evaluation of the c argument should not have any side-effects
 */
#define INS_CHAR( c, sp, bep, odp, cc, fd )											\
			{																						\
				if ( sp < bep )																\
				{																					\
					*sp++ = c ;																	\
					cc++ ;																		\
				}																					\
				else																				\
				{																					\
					if ( fd >= 0 )																\
					{																				\
						odp->nextb = sp ;														\
						if ( __sio_writef( odp, fd ) != bep - odp->start )			\
							return( ( cc != 0 ) ? cc : SIO_ERR ) ;						\
						sp = odp->nextb ;														\
						*sp++ = c ;																\
						cc++ ;																	\
					}																				\
				}																					\
				if ( __SIO_MUST_FLUSH( *odp, c ) && fd >= 0 ) 						\
				{																					\
					int b_in_buffer = sp - odp->start ;									\
																									\
					odp->nextb = sp ;															\
					if ( __sio_writef( odp, fd ) != b_in_buffer )					\
						return( cc ) ;															\
					sp = odp->nextb ;															\
				}																					\
			}



#define NUM( c )			( c - '0' )

#define STR_TO_DEC( str, num )									\
									num = NUM( *str++ ) ;			\
									while ( isdigit( *str ) )		\
									{										\
										num *= 10 ;						\
										num += NUM( *str++ ) ;		\
									}

/*
 * This macro does zero padding so that the precision
 * requirement is satisfied. The padding is done by
 * adding '0's to the left of the string that is going
 * to be printed.
 */
#define FIX_PRECISION( adjust, precision, s, s_len )				\
					if ( adjust )												\
						while ( s_len < precision )						\
						{															\
							*--s = '0' ;										\
							s_len++ ;											\
						}

/*
 * Macro that does padding. The padding is done by printing
 * the character ch.
 */
#define PAD( width, len, ch )			do														\
												{														\
													INS_CHAR( ch, sp, bep, odp, cc, fd ) ;	\
													width-- ;										\
												}														\
												while ( width > len )

/*
 * Prefix the character ch to the string str
 * Increase length
 * Set the has_prefix flag
 */
#define PREFIX( str, length, ch )	*--str = ch ; length++ ; has_prefix = YES


/*
 * Sprint is the equivalent of printf for SIO.
 * It returns the # of chars written
 * Assumptions:
 *     - all floating point arguments are passed as doubles
 */
/* VARARGS2 */
int Sprint( int fd, char *fmt, ... )
{
	__sio_descriptor_t *dp = &__sio_descriptors[ fd ] ;
	register __sio_od_t *odp = ODP( dp ) ;
	register int cc ;
	va_list ap ;

	IO_SETUP( fd, dp, __SIO_OUTPUT_STREAM, SIO_ERR ) ;

	va_start( ap, fmt ) ;
	cc = __sio_converter( odp, fd, fmt, ap ) ;
	va_end( ap ) ;
	return( cc ) ;
}


/*
 * This is the equivalent of vfprintf for SIO
 */
int Sprintv( int fd, char *fmt, va_list ap)
{
	__sio_descriptor_t *dp = &__sio_descriptors[ fd ] ;
	register __sio_od_t *odp = ODP( dp ) ;

	IO_SETUP( fd, dp, __SIO_OUTPUT_STREAM, SIO_ERR ) ;
	return( __sio_converter( odp, fd, fmt, ap ) ) ;
}


/*
 * Convert a floating point number to a string formats 'f', 'e' or 'E'.
 * The result is placed in buf, and len denotes the length of the string
 * The sign is returned in the is_negative argument (and is not placed
 * in buf).
 */
PRIVATE char *conv_fp(register char format, register double num, boolean_e add_dp, int precision, bool_int *is_negative, char *buf, int *len)
				/* always add decimal point if YES */
{
	register char *s = buf ;
	register char *p ;
	int decimal_point ;
	char *ecvt(double, int, int *, int *), *fcvt(double, int, int *, int *) ;
	char *conv_10() ;
#ifndef __linux__				/* necessary kludge */
	char *strcpy(char *, const char *) ;
#endif

	if ( format == 'f' )
		p = fcvt( num, precision, &decimal_point, is_negative ) ;
	else /* either e or E format */
		p = ecvt( num, precision+1, &decimal_point, is_negative ) ;

	/*
	 * Check for Infinity and NaN
	 */
	if ( isalpha( *p ) )
	{
		*len = strlen( strcpy( buf, p ) ) ;
		*is_negative = FALSE ;
		return( buf ) ;
	}

	if ( format == 'f' )
		if ( decimal_point <= 0 )
		{
			*s++ = '0' ;
			if ( precision > 0 )
			{
				*s++ = '.' ;
				while ( decimal_point++ < 0 )
					*s++ = '0' ;
			}
			else if ( add_dp )
				*s++ = '.' ;
		}
		else
		{
			while ( decimal_point-- > 0 )
				*s++ = *p++ ;
			if ( precision > 0 || add_dp ) *s++ = '.' ;
		}
	else
	{
		*s++ = *p++ ;
		if ( precision > 0 || add_dp ) *s++ = '.' ;
	}

	/*
	 * copy the rest of p, the NUL is NOT copied
	 */
	while ( *p ) *s++ = *p++ ;

	if ( format != 'f' )
	{
		char temp[ EXPONENT_LENGTH ] ;				/* for exponent conversion */
		int t_len ;
		bool_int exponent_is_negative ;

		*s++ = format ;		/* either e or E */
		decimal_point-- ;
		if ( decimal_point != 0 )
		{
			p = conv_10( (wide_int)decimal_point, FALSE, &exponent_is_negative,
												&temp[ EXPONENT_LENGTH ], &t_len ) ;
			*s++ = exponent_is_negative ? '-' : '+' ;

			/*
			 * Make sure the exponent has at least 2 digits
			 */
			if ( t_len == 1 )
				*s++ = '0' ;
			while ( t_len-- ) *s++ = *p++ ;
		}
		else
		{
			*s++ = '+' ;
			*s++ = '0' ;
			*s++ = '0' ;
		}
	}

	*len = s - buf ;
	return( buf ) ;
}


/*
 * Convert num to a base X number where X is a power of 2. nbits determines X.
 * For example, if nbits is 3, we do base 8 conversion
 * Return value:
 *			a pointer to a string containing the number
 *
 * The caller provides a buffer for the string: that is the buf_end argument
 * which is a pointer to the END of the buffer + 1 (i.e. if the buffer
 * is declared as buf[ 100 ], buf_end should be &buf[ 100 ])
 */
PRIVATE char *conv_p2(register u_wide_int num, register int nbits, char format, char *buf_end, register int *len)
{
	register int mask = ( 1 << nbits ) - 1 ;
	register char *p = buf_end ;
	static char low_digits[] = "0123456789abcdef" ;
	static char upper_digits[] = "0123456789ABCDEF" ;
	register char *digits = ( format == 'X' ) ? upper_digits : low_digits ;

	do
	{
		*--p = digits[ num & mask ] ;
		num >>= nbits ;
	}
	while( num ) ;

	*len = buf_end - p ;
	return( p ) ;
}



/*
 * Convert num to its decimal format.
 * Return value:
 *       - a pointer to a string containing the number (no sign)
 *			- len contains the length of the string
 *			- is_negative is set to TRUE or FALSE depending on the sign
 *			  of the number (always set to FALSE if is_unsigned is TRUE)
 *
 * The caller provides a buffer for the string: that is the buf_end argument
 * which is a pointer to the END of the buffer + 1 (i.e. if the buffer
 * is declared as buf[ 100 ], buf_end should be &buf[ 100 ])
 */
PRIVATE char *conv_10(register wide_int num, register bool_int is_unsigned, register bool_int *is_negative, char *buf_end, register int *len)
{
	register char *p = buf_end ;
	register u_wide_int magnitude ;

	if ( is_unsigned )
	{
		magnitude = (u_wide_int) num ;
		*is_negative = FALSE ;
	}
	else
	{
		*is_negative = ( num < 0 ) ;

		/*
		 * On a 2's complement machine, negating the most negative integer
		 * results in a number that cannot be represented as a signed integer.
		 * Here is what we do to obtain the number's magnitude:
		 *		a. add 1 to the number
		 *		b. negate it (becomes positive)
		 *		c. convert it to unsigned
		 *		d. add 1
		 */
		if ( *is_negative )
		{
			wide_int t = num + 1 ;

			magnitude = ( (u_wide_int) -t ) + 1 ;
		}
		else
			magnitude = (u_wide_int) num ;
	}

	/*
	 * We use a do-while loop so that we write at least 1 digit
	 */
	do
	{
		register u_wide_int new_magnitude = magnitude / 10 ;

		*--p = magnitude - new_magnitude*10 + '0' ;
		magnitude = new_magnitude ;
	}
	while ( magnitude ) ;

	*len = buf_end - p ;
	return( p ) ;
}


/*
 * Do format conversion placing the output in odp
 */
int __sio_converter(register __sio_od_t *odp, int fd, register char *fmt, va_list ap)
{
	register char *sp ;
	register char *bep ;
	register int cc = 0 ;
	register int i ;

	register char *s = NULL ;
	char *q ;
	int s_len ;

	register int min_width = 0 ;
	int precision = 0 ;
	enum { LEFT, RIGHT } adjust ;
	char pad_char ;
	char prefix_char ;

	double fp_num ;
	wide_int i_num = 0 ;
	u_wide_int ui_num ;

	char num_buf[ NUM_BUF_SIZE ] ;
	char char_buf[ 2 ] ;		/* for printing %% and %<unknown> */

	/*
	 * Flag variables
	 */
	boolean_e is_long ;
	boolean_e alternate_form ;
	boolean_e print_sign ;
	boolean_e print_blank ;
	boolean_e adjust_precision ;
	boolean_e adjust_width ;
	bool_int is_negative ;

	char *conv_10();
	char *gcvt(double, int, char *) ;
#ifndef __linux__				/* necessary kludge */
	char *strchr(const char *, int) ;
#endif

	sp = odp->nextb ;
	bep = odp->buf_end ;

	while ( *fmt )
	{
		if ( *fmt != '%' )
		{
			INS_CHAR( *fmt, sp, bep, odp, cc, fd ) ;
		}
		else
		{
			/*
			 * Default variable settings
			 */
			adjust = RIGHT ;
			alternate_form = print_sign = print_blank = NO ;
			pad_char = ' ' ;
			prefix_char = NUL ;

			fmt++ ;

			/*
			 * Try to avoid checking for flags, width or precision
			 */
			if ( isascii( *fmt ) && ! islower( *fmt ) )
			{
				/*
				 * Recognize flags: -, #, BLANK, +
				 */
				for ( ;; fmt++ )
				{
					if ( *fmt == '-' )
						adjust = LEFT ;
					else if ( *fmt == '+' )
						print_sign = YES ;
					else if ( *fmt == '#' )
						alternate_form = YES ;
					else if ( *fmt == ' ' )
						print_blank = YES ;
					else if ( *fmt == '0' )
						pad_char = '0' ;
					else
						break ;
				}

				/*
				 * Check if a width was specified
				 */
				if ( isdigit( *fmt ) )
				{
					STR_TO_DEC( fmt, min_width ) ;
					adjust_width = YES ;
				}
				else if ( *fmt == '*' )
				{
					min_width = va_arg( ap, int ) ;
					fmt++ ;
					adjust_width = YES ;
					if ( min_width < 0 )
					{
						adjust = LEFT ;
						min_width = -min_width ;
					}
				}
				else
					adjust_width = NO ;

				/*
				 * Check if a precision was specified
				 *
				 * XXX: an unreasonable amount of precision may be specified
				 *		  resulting in overflow of num_buf. Currently we
				 *		  ignore this possibility.
				 */
				if ( *fmt == '.' )
				{
					adjust_precision = YES ;
					fmt++ ;
					if ( isdigit( *fmt ) )
					{
						STR_TO_DEC( fmt, precision ) ;
					}
					else if ( *fmt == '*' )
					{
						precision = va_arg( ap, int ) ;
						fmt++ ;
						if ( precision < 0 )
							precision = 0 ;
					}
					else
						precision = 0 ;
				}
				else
					adjust_precision = NO ;
			}
			else
				adjust_precision = adjust_width = NO ;

			/*
			 * Modifier check
			 */
			if ( *fmt == 'l' )
			{
				is_long = YES ;
				fmt++ ;
			}
			else
				is_long = NO ;

			/*
			 * Argument extraction and printing.
			 * First we determine the argument type.
			 * Then, we convert the argument to a string.
			 * On exit from the switch, s points to the string that
			 * must be printed, s_len has the length of the string
			 * The precision requirements, if any, are reflected in s_len.
			 *
			 * NOTE: pad_char may be set to '0' because of the 0 flag.
			 *			It is reset to ' ' by non-numeric formats
			 */
			switch( *fmt )
			{
				case 'd':
				case 'i':
				case 'u':
					if ( is_long )
						i_num = va_arg( ap, wide_int ) ;
					else
						i_num = (wide_int) va_arg( ap, int ) ;
					s = conv_10( i_num, (*fmt) == 'u', &is_negative,
												&num_buf[ NUM_BUF_SIZE ], &s_len ) ;
					FIX_PRECISION( adjust_precision, precision, s, s_len ) ;

					if ( *fmt != 'u' )
					{
						if ( is_negative )
							prefix_char = '-' ;
						else if ( print_sign )
							prefix_char = '+' ;
						else if ( print_blank )
							prefix_char = ' ' ;
					}
					break ;


				case 'o':
					if ( is_long )
						ui_num = va_arg( ap, u_wide_int ) ;
					else
						ui_num = (u_wide_int) va_arg( ap, unsigned int ) ;
					s = conv_p2( ui_num, 3, *fmt,
												&num_buf[ NUM_BUF_SIZE ], &s_len ) ;
					FIX_PRECISION( adjust_precision, precision, s, s_len ) ;
					if ( alternate_form && *s != '0' )
					{
						*--s = '0' ;
						s_len++ ;
					}
					break ;


				case 'x':
				case 'X':
					if ( is_long )
						ui_num = (u_wide_int) va_arg( ap, u_wide_int ) ;
					else
						ui_num = (u_wide_int) va_arg( ap, unsigned int ) ;
					s = conv_p2( ui_num, 4, *fmt,
												&num_buf[ NUM_BUF_SIZE ], &s_len ) ;
					FIX_PRECISION( adjust_precision, precision, s, s_len ) ;
					if ( alternate_form && i_num != 0 )
					{
						*--s = *fmt ;			/* 'x' or 'X' */
						*--s = '0' ;
						s_len += 2 ;
					}
					break ;


				case 's':
					s = va_arg( ap, char * ) ;
					if ( s != NULL )
					{
						s_len = strlen( s ) ;
						if ( adjust_precision && precision < s_len )
							s_len = precision ;
					}
					else
					{
						s = S_NULL ;
						s_len = S_NULL_LEN ;
					}
					pad_char = ' ' ;
					break ;


				case 'f':
				case 'e':
				case 'E':
					fp_num = va_arg( ap, double ) ;

					s = conv_fp( *fmt, fp_num, alternate_form,
							( adjust_precision == NO ) ? FLOAT_DIGITS : precision,
													&is_negative, &num_buf[ 1 ], &s_len ) ;
					if ( is_negative )
						prefix_char = '-' ;
					else if ( print_sign )
						prefix_char = '+' ;
					else if ( print_blank )
						prefix_char = ' ' ;
					break ;


				case 'g':
				case 'G':
					if ( adjust_precision == NO )
						precision = FLOAT_DIGITS ;
					else if ( precision == 0 )
						precision = 1 ;
					/*
					 * We use &num_buf[ 1 ], so that we have room for the sign
					 */
					s = gcvt( va_arg( ap, double ), precision, &num_buf[ 1 ] ) ;
					if ( *s == '-' )
						prefix_char = *s++ ;
					else if ( print_sign )
						prefix_char = '+' ;
					else if ( print_blank )
						prefix_char = ' ' ;

					s_len = strlen( s ) ;

					if ( alternate_form && ( q = strchr( s, '.' ) ) == NULL )
						s[ s_len++ ] = '.' ;
					if ( *fmt == 'G' && ( q = strchr( s, 'e' ) ) != NULL )
						*q = 'E' ;
					break ;


				case 'c':
					char_buf[ 0 ] = (char) (va_arg( ap, int )) ;
					s = &char_buf[ 0 ] ;
					s_len = 1 ;
					pad_char = ' ' ;
					break ;


				case '%':
					char_buf[ 0 ] = '%' ;
					s = &char_buf[ 0 ] ;
					s_len = 1 ;
					pad_char = ' ' ;
					break ;


				case 'n':
					*(va_arg( ap, int * )) = cc ;
					break ;

				/*
				 * Always extract the argument as a "char *" pointer. We
				 * should be using "void *" but there are still machines
				 * that don't understand it.
				 * If the pointer size is equal to the size of an unsigned
				 * integer we convert the pointer to a hex number, otherwise
				 * we print "%p" to indicate that we don't handle "%p".
				 */
				case 'p':
					ui_num = (u_wide_int) va_arg( ap, char * ) ;

					if ( sizeof( char * ) <= sizeof( u_wide_int ) )
						s = conv_p2( ui_num, 4, 'x',
												&num_buf[ NUM_BUF_SIZE ], &s_len ) ;
					else
					{
						s = "%p" ;
						s_len = 2 ;
					}
					pad_char = ' ' ;
					break ;


				case NUL:
					/*
					 * The last character of the format string was %.
					 * We ignore it.
					 */
					continue ;


					/*
				 	 * The default case is for unrecognized %'s.
					 * We print %<char> to help the user identify what
					 * option is not understood.
					 * This is also useful in case the user wants to pass
					 * the output of __sio_converter to another function
					 * that understands some other %<char> (like syslog).
					 * Note that we can't point s inside fmt because the
					 * unknown <char> could be preceded by width etc.
					 */
				default:
					char_buf[ 0 ] = '%' ;
					char_buf[ 1 ] = *fmt ;
					s = char_buf ;
					s_len = 2 ;
					pad_char = ' ' ;
					break ;
			}

			if ( prefix_char != NUL )
			{
				*--s = prefix_char ;
				s_len++ ;
			}

			if ( adjust_width && adjust == RIGHT && min_width > s_len )
			{
				if ( pad_char == '0' && prefix_char != NUL )
				{
					INS_CHAR( *s, sp, bep, odp, cc, fd )
					s++ ;
					s_len-- ;
					min_width-- ;
				}
				PAD( min_width, s_len, pad_char ) ;
			}

			/*
			 * Print the string s.
			 */
			for ( i = s_len ; i != 0 ; i-- )
			{
				INS_CHAR( *s, sp, bep, odp, cc, fd ) ;
				s++ ;
			}

			if ( adjust_width && adjust == LEFT && min_width > s_len )
				PAD( min_width, s_len, pad_char ) ;
		}
		fmt++ ;
	}
	odp->nextb = sp ;
	return( cc ) ;
}

