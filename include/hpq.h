/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef __HPQ_H
#define __HPQ_H

/*
 * $Id: hpq.h,v 1.3 2001/07/06 00:57:30 seth Exp $
 */

/*
 * Implementation of the PQ interface
 */

#define pq_create		__hpq_create
#define pq_destroy		__hpq_destroy
#define pq_insert		__hpq_insert
#define pq_delete		__hpq_delete
#define pq_head			__hpq_head
#define pq_extract_head		__hpq_extract_head
#define pq_iterate		__hpq_iterate
#define pq_nextobj		__hpq_nextobj

/*
 * The is_better function takes 2 arguments which are pointers to objects
 * and returns:
 *       1     if the first object is "better" than the second
 *       0     otherwise
 */

struct __hpq_header
{
	int (*is_better)(void *, void *) ;
	int *errnop ;
	int flags ;
	pq_obj *objects ;			/* array of objects */
	unsigned cur_size ;			/* # of objects in array */
	unsigned max_size ;			/* max # of objects that can fit in array */
	unsigned int iter ;			/* current iteration index */
} ;
typedef struct __hpq_header header_s ;


pq_h __hpq_create		 ( int (*func)(pq_obj, pq_obj), int flags, int *errnop )  ;
void __hpq_destroy		 ( pq_h handle )  ;
int  __hpq_insert		 ( pq_h handle, pq_obj object )  ;
pq_obj __hpq_extract_head	 ( pq_h handle )  ;
int  __hpq_delete 		 ( pq_h handle, pq_obj object )  ;
void __hpq_iterate		 ( pq_h handle )  ;
pq_obj __hpq_nextobj		 ( pq_h handle )  ;
int __hpq_verify		 (header_s *hp, unsigned int current)  ;


#define __hpq_head( handle ) \
     ( \
      ((struct __hpq_header *)(handle))->cur_size \
      ? ((struct __hpq_header *)(handle))->objects[ 0 ] \
      : (pq_obj) 0 \
     )

#endif /* __HPQ_H */

