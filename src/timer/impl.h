/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

#ifndef IMPL_H
#define IMPL_H

/*
 * $Id: impl.h,v 1.2 2003/06/17 05:10:55 seth Exp $
 */

#include <sys/time.h>
#include <setjmp.h>


#include "pq.h"
#include "timer.h"

enum timer_state { INACTIVE, TICKING, DESTROYED } ; 
enum action_state { IDLE, PENDING, SCHEDULED, INVOKED } ;


struct timer
{
	enum timer_state 		t_state ;
	enum action_state 	t_act ;
	int						t_blocked ;

	int 						t_flags ;
	int						*t_errnop ;
	struct os_timer		*t_ostimer ;
	struct timer_action	t_action ;

	/*
	 * The following fields are managed by the ostimer code.
	 * t_expiration is the (absolute) time when the timer will expire.
	 * t_interval is the repeat interval for the timer.
	 * t_expirations is the number of expirations of the timer when
	 * the function associated with the timer is invoked.
	 * t_count is the number of times that the timer has expired before
	 * the function was invoked.
	 */
	struct timeval			t_expiration ;
	struct timeval			t_interval ;
	unsigned					t_count ;
	unsigned					t_expirations ;
} ;

typedef struct timer timer_s ;

#define TP( p )				( (struct timer *) (p) )

#ifdef _WIN32
# include <malloc.h>
#else
#ifdef NEED_PROTOTYPES
char *malloc(size_t) ;
char *realloc(void *, size_t) ;
#else
#include <stdlib.h>
#endif
#endif /* _WIN32 */

#define TIMER_ALLOC()		TP( malloc( sizeof( timer_s ) ) )
#define TIMER_FREE( tp )	(void) free( (char *)(tp) )

/*
 * The following are masks for the expected flags of timer_create and
 * timer_start
 */
#define TIMER_CREATE_FLAGS		TIMER_RETURN_ERROR
#define TIMER_START_FLAGS		\
			( TIMER_INC_VAR + TIMER_BLOCK_SAME + TIMER_BLOCK_ALL + TIMER_LONGJMP )


enum timer_state __timer_invoke(register timer_s *tp) ;
void __timer_terminate() ;

#include "ostimer.h"

#endif	/* IMPL_H */

