/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef OSTIMER_H
#define OSTIMER_H

/*
 * Include signal.h to get sigset_t
 */
#ifndef NO_POSIX_SIGS
#include <signal.h>
#endif

#include "pq.h"

/*
 * Holds a list of timers ordered by expiration time (the one at the
 * head of the list is the one that will expire first).
 */
struct timer_q
{
	pq_h	tq_handle ;
	int	tq_errno ;
} ;

/*
 * Macros to handle the priority queue
 */
#define timer_pq_insert( tpq, tp )     pq_insert( tpq, (char *) tp )
#define timer_pq_head( tpq )           (struct timer *) pq_head( tpq )
#define timer_pq_extract_head( tpq )   (struct timer *) pq_extract_head( tpq )
#define timer_pq_delete( tpq, tp )     pq_delete( tpq, (char *) tp )


typedef enum { AVAILABLE, UNAVAILABLE } availability_e ;

/*
 * Description of a timer provided by the operating system
 */
struct os_timer
{
	availability_e		ost_availability ;
	int					ost_systype ;			/* e.g. ITIMER_REAL				 		*/
	int					ost_signal ;			/* what signal is generated 			*/
														/* upon expiration						*/
	enum timer_types	ost_timertype ;		/* e.g. TIMER_REAL						*/
	void					(*ost_handler)() ;	/* what function to invoke				*/
	void					(*ost_get_current_time)() ;
													/* how to find the current time			*/
	struct timer_q		ost_timerq ;			/* list of timers of this type		*/
#ifndef NO_POSIX_SIGS
	sigset_t				ost_block_mask ;		/* signal mask to blocks this timer */
#else
	int 					ost_block_mask ;
#endif
} ;

typedef struct os_timer ostimer_s ;

#define OSTIMER_NULL					((ostimer_s *)0)

#define SIGSET_NULL					((sigset_t *)0)

/*
 * Public functions
 */
ostimer_s	*__ostimer_init(timer_s *tp, enum timer_types type) ;
int			__ostimer_newtimer(timer_s *tp, enum timer_types type) ;
int			__ostimer_add(ostimer_s *otp, register timer_s *tp, struct itimerval *itvp, enum timer_timetypes time_type) ;
void			__ostimer_interrupt(register ostimer_s *otp) ;
void			__ostimer_remove(ostimer_s *otp, timer_s *tp) ;
void			__ostimer_blockall(void) ;
void			__ostimer_unblockall(void) ;
void			__ostimer_unblockall_except(ostimer_s *otp) ;

#endif	/* OSTIMER_H */

