/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */


#ifndef SS_SBM_H
#define SS_SBM_H

/*
 * $Id: ss_sbm.h,v 1.2 2003/06/17 05:10:55 seth Exp $
 */

typedef int last_int ;			/* must be signed */

struct sbm_header
{
	last_int *last_occurrence ;
} ;

#endif	/* SS_SBM_H */

