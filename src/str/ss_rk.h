/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef SS_RK_H
#define SS_RK_H

/*
 * $Id: ss_rk.h,v 1.1 2001/05/26 22:04:51 seth Exp $
 */

struct rk_header
{
	u_wide_int rk_digit1 ;				/* in the appropriate radix */
	u_wide_int rk_patval ;
} ;

#if WIDE_INT_SIZE == 32
#	define PRIME						16777213
#else
#	if WIDE_INT_SIZE == 16
#		define PRIME					251
#	else
		int WIDE_INT_SIZE_has_bad_value = or_is_undefined ;
#	endif 
#endif

#endif	/* SS_RK_H */

