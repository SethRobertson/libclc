/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

#ifndef __TIMEMACROS_H
#define __TIMEMACROS_H

/*
 * $Id: timemacros.h,v 1.2 2003/06/17 05:10:50 seth Exp $
 */

#include <sys/types.h>
#include <sys/time.h>

/*
 * Define a few relevant NULL values
 */
#ifndef TIMEZONE_NULL
#define TIMEZONE_NULL		((struct timezone *)0)
#endif
#ifndef TIMEVAL_NULL
#define TIMEVAL_NULL			((struct timeval *)0)
#endif
#ifndef ITIMERVAL_NULL
#define ITIMERVAL_NULL		((struct itimerval *)0)
#endif

/*
 * The TV_* macros work with timeval structures.
 * The TVP_* macros work with pointers to timeval structures.
 *
 * We support the following operations:
 *
 *			EQ( tv1, tv2 )			: tv1 == tv2
 *			NE( tv1, tv2 )			: tv1 != tv2
 *			LT( tv1, tv2 )			: tv1 <  tv2
 *			LE( tv1, tv2 )			: tv1 <= tv2
 *			GT( tv1, tv2 )			: tv1 >  tv2
 *			GE( tv1, tv2 )			: tv1 >= tv2
 *			ISZERO( tv )			: tv == 0
 *			ZERO( tv )				: tv = 0
 *			ADD( res, tv1, tv2 ) : res = tv1 + tv2
 *			SUB( res, tv1, tv2 ) : res = tv1 = tv2
 *
 * We make sure the result is normalized after addition and subtraction.
 */

#define __ONE_MILLION					1000000

#define TV_ADD( tv_res, tv1, tv2 )									\
	{																			\
		(tv_res).tv_sec = (tv1).tv_sec + (tv2).tv_sec ;			\
		(tv_res).tv_usec = (tv1).tv_usec + (tv2).tv_usec ;		\
		if ( (tv_res).tv_usec >= __ONE_MILLION )					\
		{																		\
			(tv_res).tv_sec++ ;											\
			(tv_res).tv_usec -= __ONE_MILLION ;						\
		}																		\
	}

/*
 * We handle the possibility of underflow in advance in case
 * tv_usec is an unsigned integer.
 */
#define TV_SUB( tv_res, tv1, tv2 )									\
	{																			\
		if ( (tv1).tv_usec < (tv2).tv_usec )						\
		{																		\
			(tv1).tv_usec += __ONE_MILLION ;							\
			(tv1).tv_sec-- ;												\
		}																		\
		(tv_res).tv_sec = (tv1).tv_sec - (tv2).tv_sec ;			\
		(tv_res).tv_usec = (tv1).tv_usec - (tv2).tv_usec ;		\
	}

#define TV_LT( tv1, tv2 )																	\
		( (tv1).tv_sec < (tv2).tv_sec ||													\
		  ((tv1).tv_sec == (tv2).tv_sec && (tv1).tv_usec < (tv2).tv_usec ))

#define TV_LE( tv1, tv2 )																	\
		( (tv1).tv_sec < (tv2).tv_sec ||													\
		  ((tv1).tv_sec == (tv2).tv_sec && (tv1).tv_usec <= (tv2).tv_usec ))

#define TV_GT( tv1, tv2 )																	\
		( (tv1).tv_sec > (tv2).tv_sec ||													\
		  ((tv1).tv_sec == (tv2).tv_sec && (tv1).tv_usec > (tv2).tv_usec ))

#define TV_GE( tv1, tv2 )																	\
		( (tv1).tv_sec > (tv2).tv_sec ||													\
		  ((tv1).tv_sec == (tv2).tv_sec && (tv1).tv_usec >= (tv2).tv_usec ))

#define TV_EQ( tv1, tv2 )																	\
		( (tv1).tv_sec == (tv2).tv_sec && (tv1).tv_usec == (tv2).tv_usec )

#define TV_NE( tv1, tv2 )																	\
		( (tv1).tv_sec != (tv2).tv_sec || (tv1).tv_usec != (tv2).tv_usec )

#define TV_ISZERO( tv )			( (tv).tv_sec == 0 && (tv).tv_usec == 0 )

#define TV_ZERO( tv )			(tv).tv_sec = (tv).tv_usec = 0


/*
 * Pointer macros
 */

#define TVP_ADD( tvp_res, tvp1, tvp2 )	TV_ADD( *(tvp_res), *(tvp1), *(tvp2) )
#define TVP_SUB( tvp_res, tvp1, tvp2 )	TV_SUB( *(tvp_res), *(tvp1), *(tvp2) )
#define TVP_LT( tvp1, tvp2 )				TV_LT( *(tvp1), *(tvp2) )
#define TVP_LE( tvp1, tpv2 )				TV_LE( *(tvp1), *(tvp2) )
#define TVP_GT( tvp1, tvp2 )				TV_GT( *(tvp1), *(tvp2) )
#define TVP_GE( tvp1, tvp2 )				TV_GE( *(tvp1), *(tvp2) )
#define TVP_EQ( tvp1, tvp2 )				TV_EQ( *(tvp1), *(tvp2) )
#define TVP_NE( tvp1, tvp2 )				TV_NE( *(tvp1), *(tvp2) )
#define TVP_ISZERO( tvp )					TV_ISZERO( *(tvp) )
#define TVP_ZERO( tvp )						TV_ZERO( *(tvp) )

#endif	/* __TIMEMACROS_H */
