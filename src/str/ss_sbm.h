/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */


#ifndef SS_SBM_H
#define SS_SBM_H

/*
 * $Id: ss_sbm.h,v 1.1 2001/05/26 22:04:51 seth Exp $
 */

typedef int last_int ;			/* must be signed */

struct sbm_header
{
	last_int *last_occurrence ;
} ;

#endif	/* SS_SBM_H */

