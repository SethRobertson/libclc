/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef __PQ_H
#define __PQ_H

/*
 * $Id: pq.h,v 1.1 2001/05/26 22:04:51 seth Exp $
 */

/*
 * Return values
 */
#define PQ_OK			0
#define PQ_ERR			(-1)

extern int pq_errno ;

/*
 * pq_errno values
 */
#define PQ_ENOERROR				0
#define PQ_ENOFUNC				1
#define PQ_ENOMEM					2
#define PQ_ENULLOBJECT			3
#define PQ_ENOTFOUND				4
#define PQ_ENOTSUPPORTED		5

/*
 * flag values
 */
#define PQ_NOFLAGS				0x0
#define PQ_RETURN_ERROR			0x1

typedef void *pq_h ;
typedef void *pq_obj ;

#include "hpq.h"

#endif	/* __PQ_H */

