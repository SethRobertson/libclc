/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

#ifndef SS_KMP_H
#define SS_KMP_H

/*
 * $Id: ss_kmp.h,v 1.2 2003/06/17 05:10:55 seth Exp $
 */

typedef int next_int ;			/* must be signed */

struct kmp_header
{
	next_int *next ;
} ;

#endif	/* SS_KMP_H */

