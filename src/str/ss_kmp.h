/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef SS_KMP_H
#define SS_KMP_H

/*
 * $Id: ss_kmp.h,v 1.1 2001/05/26 22:04:51 seth Exp $
 */

typedef int next_int ;			/* must be signed */

struct kmp_header
{
	next_int *next ;
} ;

#endif	/* SS_KMP_H */

