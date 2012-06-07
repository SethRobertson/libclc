/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

#ifndef __HPQ_H
#define __HPQ_H

/*
 * $Id: hpq.h,v 1.10 2003/06/17 05:10:50 seth Exp $
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
#define pq_iterate_done		__hpq_iterate_done
#define pq_verify		__hpq_verify


typedef int (*pq_compfun)(void *, void *);
typedef int *pq_iter;


/*
 * The is_better function takes 2 arguments which are pointers to objects
 * and returns:
 *       1     if the first object is "better" than the second
 *       0     otherwise
 */
struct __hpq_header
{
  pq_compfun is_better;
  int dicterrno ;
  int flags ;
  pq_obj *objects ;				/* array of objects */
  int cur_size ;				/* # of objects in array */
  int max_size ;				/* max # of objects that can fit in array */
#ifdef BK_USING_PTHREADS
  int iter_cnt;					/* Number of iterators allocated */
  pq_iter *iter;				/* iterator list */
  pthread_mutex_t lock;				/* Threading lock */
#else /* BK_USING_PTHREADS */
  unsigned int iter ;				/* current iteration index */
#endif /* BK_USING_PTHREADS */
} ;
typedef struct __hpq_header header_s ;


pq_h __hpq_create		( int (*func)(pq_obj, pq_obj), int flags )  ;
void __hpq_destroy		( pq_h handle )  ;
int  __hpq_insert		( pq_h handle, pq_obj object )  ;
pq_obj __hpq_extract_head	( pq_h handle )  ;
int  __hpq_delete 		( pq_h handle, pq_obj object )  ;
pq_iter __hpq_iterate		( pq_h handle )  ;
pq_obj __hpq_nextobj		( pq_h handle, pq_iter iter )  ;
void __hpq_iterate_done		( pq_h handle, pq_iter iter ) ;
int __hpq_verify		(header_s *hp, int current)  ;
char *__hpq_error_reason	( int dicterrno ) ;
char *pq_error_reason		( pq_h handle, int *errnop ) ;


#define __hpq_head( handle ) \
     ( \
      ((struct __hpq_header *)(handle))->cur_size \
      ? ((struct __hpq_header *)(handle))->objects[ 0 ] \
      : (pq_obj) 0 \
     )



/*
 * Delete all of the elements in a PQ datastructure
 * For use only by PQ_NUKE_CONTENTS and PQ_NUKE.
 *
 * THREADS: REENTRANT (assuming different dictionaries)
 *
 * <TODO>Can we figure out this is a bst and turn off balanced_tree?</TODO>
 *
 * <TRICKY>Do not change this macro, you will surely regret it.</TRICKY>
 */
#define _PQ_NUKE_WORK_LOOP(Q, ptr, code)			\
 while (((ptr) = pq_extract_head(Q)))				\
 {								\
   code;							\
 }								\


/**
 * Delete all of the elements in a CLC pq datastructure
 * User supplies the code to actually free the objects in the pq.
 * Errcode is unlikely to do anything very useful--just log messages--but is
 * also not likely to ever execute.
 *
 * THREADS: REENTRANT (assuming different pqionaries)
 *
 */
#define PQ_NUKE_CONTENTS(Q, ptr, code)	\
 do					\
 {					\
   if (Q)				\
     _PQ_NUKE_WORK_LOOP(Q, ptr, code)	\
 } while (0)


/**
 * Delete a CLC pq datastructure and all of its elements.
 * User supplies the code to actually free the objects in the pq.
 * Errcode is unlikely to do anything very useful--just log messages--but is
 * also not likely to ever execute.
 *
 * THREADS: REENTRANT (assuming different Qs)
 *
 */
#define PQ_NUKE(Q, ptr, code)		\
 do					\
 {					\
   if (!(Q)) break;			\
   _PQ_NUKE_WORK_LOOP(Q, ptr, code);	\
   pq_destroy(Q);			\
 } while (0)


#endif /* __HPQ_H */
