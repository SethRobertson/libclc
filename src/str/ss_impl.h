/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef SS_IMPL_H
#define SS_IMPL_H

/*
 * $Id: ss_impl.h,v 1.1 2001/05/26 22:04:51 seth Exp $
 */

/*
 * NBIC is the Number-of-Bits-In-a-Char
 */
#ifndef NBIC
#define NBIC								8
#endif

#define ALPHABET_SIZE					( 1 << NBIC )

#ifndef WIDE_INT
#define WIDE_INT                 long
#define WIDE_INT_SIZE            32       /* bits */
#endif

typedef WIDE_INT wide_int ;
typedef unsigned WIDE_INT u_wide_int ;


#include "ss_rk.h"
#include "ss_kmp.h"
#include "ss_sbm.h"
#include "ss_bmh.h"
#include "ss_so.h"

#include "str.h"

struct ss_ops
{
	int	(*so_setup)() ;
	char	*(*so_match)() ;
	void	(*so_done)() ;
} ;


struct ss_header
{
	char 				*ss_pattern ;
	int				ss_patlen ;
	int				ss_flags ;
	char				*ss_map ;				/* either identity or upper->lower */
	struct ss_ops	*ss_ops ;
	union ss_headers
	{
		struct rk_header	rkh ;
		struct kmp_header kmph ;
		struct sbm_header sbmh ;
		struct bmh_header bmhh ;
		struct so_header	soh ;
	} ss_h ;
} ;

typedef struct ss_header header_s ;

#define HP( p )							((header_s *)(p))

/*
 * Structure field access
 */
#define SS_PATTERN( hp )				(hp)->ss_pattern
#define SS_PATLEN( hp )					(hp)->ss_patlen
#define SS_FLAGS( hp )					(hp)->ss_flags
#define SS_OPS( hp )						(hp)->ss_ops
#define SS_SETMAP( hp, map )			(hp)->ss_map = map
#define SS_MAP( hp, c )					(hp)->ss_map[ (unsigned char) (c) ]

/*
 * Predicates
 */
#define SS_MALLOC( hp )				( ! ( SS_FLAGS( hp ) & STRS_NOMALLOC ) )
#define SS_IGNCASE( hp )			( SS_FLAGS( hp ) & STRS_IGNCASE )
#define SS_SWITCH( hp )				( ! ( SS_FLAGS( hp ) & STRS_NOSWITCH ) )
#define SS_SETMALLOC( hp )			SS_FLAGS( hp ) &= ~ STRS_NOMALLOC

/*
 * Indirect op invocation
 */
#define SS_SETUP( hp )					(*SS_OPS( hp )->so_setup)( hp )
#define SS_MATCH( hp, str, len )		(*SS_OPS( hp )->so_match)( hp, str, len )
#define SS_DONE( hp )					(*SS_OPS( hp )->so_done)( hp )

/*
 * Header extraction
 */
#define RK_HEADER( hp )					(&(hp)->ss_h.rkh)
#define KMP_HEADER( hp )				(&(hp)->ss_h.kmph)
#define SBM_HEADER( hp )				(&(hp)->ss_h.sbmh)
#define BMH_HEADER( hp )				(&(hp)->ss_h.bmhh)
#define SO_HEADER( hp )					(&(hp)->ss_h.soh)

/*
 * Macros to extract method and flags from the 'flags' argument
 */
#define METHOD_BITS						5		/* flag bits devoted to methods */
#define METHOD_MASK						( ( 1 << METHOD_BITS ) - 1 )
#define SS_GETMETHOD( x )				( (x) & METHOD_MASK )
#define SS_GETFLAGS( x )				( (x) & ~METHOD_MASK )


struct ss_select
{
	int				sel_method ;
	struct ss_ops	*sel_ops ;
} ;


#ifndef NULL
#define NULL								0
#endif

#ifndef FALSE
#define FALSE								0
#define TRUE								1
#endif

#define CHAR_NULL							((char *)0)
#define NULL_HANDLE						((strs_h)0)

#define PRIVATE							static


/*
 * Return values
 */
#define SS_OK								0
#define SS_ERR								(-1)

#endif	/* SS_IMPL_H */

