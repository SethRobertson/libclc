/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef __PQ_H
#define __PQ_H

/*
 * $Id: pq.h,v 1.5 2003/04/01 04:40:56 seth Exp $
 */



/*
 * Return values
 */
#define PQ_OK			0
#define PQ_ERR			(-1)



/*
 * pq_errno values
 */
#define PQ_ENOERROR		0
#define PQ_ENOFUNC		1
#define PQ_ENOMEM		2
#define PQ_ENULLOBJECT		3
#define PQ_ENOTFOUND		4
#define PQ_ENOTSUPPORTED	5



/*
 * flag values
 */
#define PQ_NOFLAGS		0x0		// Naught
#define PQ_THREADED_SAFE	0x1		// Protect against multiple threads on same PQ

typedef void *pq_h ;
typedef void *pq_obj ;

#include "hpq.h"

#endif	/* __PQ_H */

