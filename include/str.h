/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */


#ifndef __STR_H
#define __STR_H

/*
 * $Id: str.h,v 1.1 2001/05/26 22:04:52 seth Exp $
 */

#include <stdarg.h>


#ifdef __ARGS
#undef __ARGS
#endif

#ifdef PROTOTYPES
#   define __ARGS( s )               s
#else
#   define __ARGS( s )               ()
#endif


/*
 * strprint(3) functions
 */
char *str_sprint __ARGS( ( char *buf, char *fmt, ... ) ) ;
int str_nprint __ARGS( ( char *buf, char *fmt, ... ) ) ;
void str_print __ARGS( ( int *count, char *buf, char *fmt, ... ) ) ;

char *str_sprintv __ARGS( ( char *buf, char *fmt, va_list ) ) ;
int str_nprintv __ARGS( ( char *buf, char *fmt, va_list ) ) ;
void str_printv __ARGS( ( int *count, char *buf, char *fmt, va_list ) ) ;

char *strx_sprint __ARGS( ( char *buf, int len, char *fmt, ... ) ) ;
int strx_nprint __ARGS( ( char *buf, int len, char *fmt, ... ) ) ;
void strx_print __ARGS( ( int *count, char *buf, int len, char *fmt, ... ) ) ;

char *strx_sprintv __ARGS( ( char *buf, int len, char *fmt, va_list ) ) ;
int strx_nprintv __ARGS( ( char *buf, int len, char *fmt, va_list ) ) ;
void strx_printv __ARGS(( int *cnt, char *buf, int len, char *fmt, va_list )) ;


/*
 * strparse(3) functions
 */

/*
 * Return values
 */
#define STR_OK						0
#define STR_ERR					(-1)


/* 
 * Flags for the string parsing functions
 */
#define STR_NOFLAGS				0x0
#define STR_RETURN_ERROR		0x1
#define STR_NULL_START			0x2
#define STR_NULL_END				0x4
#define STR_MALLOC				0x8

extern int str_errno ;

/*
 * Error values
 */
#define STR_ENULLSEPAR			1
#define STR_ENULLSTRING			2
#define STR_ENOMEM				3

typedef void *str_h ;

str_h str_parse __ARGS( ( char *str, char *separ, int flags, int *errnop ) ) ;
void str_endparse __ARGS( ( str_h handle ) ) ;
char *str_component __ARGS( ( str_h handle ) ) ;
int str_setstr __ARGS( ( str_h handle, char *newstr ) ) ;
int str_separator __ARGS( ( str_h handle, char *separ ) ) ;
char *str_nextpos __ARGS( ( str_h handle ) ) ;

/*
 * For backwards compatibility
 */
#define str_process( s, sep, flags )	str_parse( s, sep, flags, (int *)0 )
#define str_endprocess( handle )			str_endparse( handle )


/*
 * strutil(3) functions
 */
char *str_find __ARGS( ( char *s1, char *s2 ) ) ;
char *str_casefind __ARGS( ( char *s1, char *s2 ) ) ;
void str_fill __ARGS( ( char *s, char c ) ) ;
char *str_lower __ARGS( ( char *s ) ) ;
char *str_upper __ARGS( ( char *s ) ) ;


/*
 * strsearch(3) functions
 */

/*
 * Methods
 */
#define STRS_BF								0			/* brute force				*/
#define STRS_RK								1			/* Rabin-Karp				*/
#define STRS_KMP								2			/* Knuth-Morris-Pratt	*/
#define STRS_SBM								3			/* Simple Boyer-Moore	*/
#define STRS_BMH								4			/* Boyer-Moore-Horspool */
#define STRS_SO								5			/* Shift-Or					*/

#define __STRS_METHOD_BITS					5
#define STRS_METHODS_MAX					( 1 << __STRS_METHOD_BITS )

/*
 * Flags
 */
#define __STRS_MAKEFLAG( v )				( (v) << __STRS_METHOD_BITS )
#define STRS_IGNCASE							__STRS_MAKEFLAG( 0x1 )
#define STRS_NOMALLOC						__STRS_MAKEFLAG( 0x2 )
#define STRS_NOSWITCH						__STRS_MAKEFLAG( 0x4 )
#define STRS_PATLEN							__STRS_MAKEFLAG( 0x8 )


typedef void *strs_h ;

char *strs_search __ARGS( ( int flags, char *str, int len, char *pat, ... ) ) ;
strs_h strs_setup __ARGS( ( int flags, char *pattern, ... ) ) ;
char *strs_match	__ARGS( ( strs_h handle, char *str, int len ) ) ;
void strs_done		__ARGS( ( strs_h handle ) ) ;

#endif 	/* __STR_H */

