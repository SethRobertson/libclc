/*
 * (c) Copyright 1992 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

#ifndef __FSMA_H
#define __FSMA_H

/*
 * $Id: fsma.h,v 1.10 2003/06/06 03:34:51 seth Exp $
 */

#define __FSMA_ALIGNMENT	8

union __fsma_chunk_header
{
  union __fsma_chunk_header *next_chunk ;
  char bytes[ __FSMA_ALIGNMENT ] ;
} ;

typedef void *__fsma_pointer ;

struct __fsma_header
{
  union __fsma_chunk_header *chunk_chain ;
  __fsma_pointer next_free ;
  __fsma_pointer temp ;
  unsigned slots_in_chunk ;
  unsigned slot_size ;
  unsigned chunk_size ;
  unsigned short flags;
  unsigned short is_inlined;			/* header is inlined (boolean) (should be converted to a flag) */
  unsigned int references;			// Number of people using this allocator.  Yes, Virginia, we can have more than 2^16.
#ifdef BK_USING_PTHREADS
  pthread_mutex_t lock;
#endif /* BK_USING_PTHREADS */
} ;

typedef struct __fsma_header *fsma_h ;


/*
 * Flags
 */
#define FSM_NOFLAGS				0x0
#define FSM_NOCOALESCE				0x1
#define FSM_ZERO_ALLOC				0x2
#define FSM_ZERO_FREE				0x4
#define FSM_ZERO_DESTROY			0x8

#ifdef BK_USING_PTHREADS
#define FSM_THREADED				0x10
#else
#define FSM_THREADED				0
#endif /* BK_USING_PTHREADS */

#define FSM_FREE_USEFUN				0x20 // Only for internal FSMA use--otherwise infinite loop


extern void fsm_threaded_makeready(int preference);
#define FSM_PREFER_SAFE		0x1
#define FSM_PREFER_NOCOALESCE	0x2

fsma_h	fsm_create	( unsigned size, unsigned slots, int flags )  ;
void	fsm_destroy	( fsma_h handle )  ;
void	*_fsm_alloc	( fsma_h handle, u_int flags )  ;
#define FSM_LOCKED	1
void	_fsm_free	( fsma_h handle, void *ptr )  ;

#ifdef BK_USING_PTHREADS
#ifdef __GNUC__
#define fsm_alloc( fsma )									\
    ({												\
       void *_fsm_ret = NULL;									\
												\
       if (((fsma)->flags & FSM_THREADED) && (pthread_mutex_lock(&(fsma)->lock) != 0))		\
       {											\
	 /* Complain, somehow--locking failed */						\
	 _fsm_ret = NULL;									\
       }											\
       else											\
       {											\
	 if (!(fsma)->next_free)								\
	 { /* No fast path available */								\
	   _fsm_ret = _fsm_alloc(fsma, FSM_LOCKED);						\
	 }											\
	 else											\
	 {											\
	   _fsm_ret = (fsma)->next_free;							\
	   (fsma)->next_free = *(__fsma_pointer *)(fsma)->next_free;				\
												\
	   if ((fsma)->flags & FSM_ZERO_ALLOC)							\
	     memset(_fsm_ret, 0, (fsma)->slot_size);						\
	 }											\
												\
	 if (((fsma)->flags & FSM_THREADED) && (pthread_mutex_unlock(&(fsma)->lock) != 0))	\
	 {											\
	   /* Complain, somehow--locking failed */						\
	 }											\
       }											\
       _fsm_ret;										\
    })

#else /* GNUC */

#define fsm_alloc( fsma )									\
     (												\
      (!(fsma)->next_free || (fsma)->flags & (FSM_ZERO_ALLOC|FSM_THREADED))			\
      ? _fsm_alloc( fsma, 0 )									\
      : ((fsma)->temp = (fsma)->next_free,							\
	 (fsma)->next_free = *(__fsma_pointer *) (fsma)->next_free, (char *) (fsma)->temp)	\
      )

#endif /* GNUC */

#define fsm_free( fsma, p )										\
    do													\
      {													\
	__fsma_pointer *_fsm_next = (void *)(p);							\
													\
	if ((fsma)->flags & FSM_FREE_USEFUN)								\
	{												\
	  _fsm_free(fsma, _fsm_next);									\
	  break;											\
	}												\
													\
	if ((fsma)->flags & FSM_ZERO_FREE)								\
	  memset((void *)_fsm_next, 0, (fsma)->slot_size);						\
													\
	if (((fsma)->flags & FSM_THREADED) && (pthread_mutex_lock(&(fsma)->lock) != 0))			\
	{												\
	  /* Complain, somehow--locking failed and we did not free pointer */				\
	  break;											\
	}												\
													\
	*_fsm_next = (fsma)->next_free;									\
	(fsma)->next_free = _fsm_next;									\
													\
	if (((fsma)->flags & FSM_THREADED) && (pthread_mutex_unlock(&(fsma)->lock) != 0))		\
	{												\
	  /* Complain, somehow--locking failed */							\
	}												\
      } while (0)

#else /* BK_USING_PTHREADS */

#define fsm_alloc( fsma )									\
     (												\
      (!(fsma)->next_free || (fsma)->flags & (FSM_ZERO_ALLOC))					\
      ? _fsm_alloc( fsma )									\
      : ((fsma)->temp = (fsma)->next_free,							\
	 (fsma)->next_free = *(__fsma_pointer *) (fsma)->next_free, (char *) (fsma)->temp)	\
      )

#define fsm_free( fsma, p )					\
    do								\
      {								\
	__fsma_pointer *_fsm_next = (void *)(p);		\
								\
	if ((fsma)->flags & FSM_FREE_USEFUN)			\
	{							\
	  _fsm_free(fsma, _fsm_next);				\
	  break;						\
	}							\
								\
	if ((fsma)->flags & FSM_ZERO_FREE)			\
	  memset((void *)_fsm_next, 0, (fsma)->slot_size);	\
								\
	*_fsm_next = (fsma)->next_free;				\
	(fsma)->next_free = _fsm_next;				\
      } while (0)

#endif /* BK_USING_PTHREADS */

#define fsm_size( fsma )	(fsma)->slot_size

#endif	/* __FSMA_H */
