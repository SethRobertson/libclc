/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

UNUSED static char RCSid[] = "$Id: ostimer.c,v 1.2 2003/06/17 05:10:55 seth Exp $" ;

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

extern int errno ;

#include "timemacros.h"
#include "defs.h"
#include "impl.h"
#include "ostimer.h"
#include "clchack.h"

#ifdef DEBUG
#define DEBUG_REPORT_ERRORS
#endif

#define OSTIMER_SET( func, otp, itv )														\
		if ( setitimer( (otp)->ost_systype, &(itv), ITIMERVAL_NULL ) == -1 )		\
			report( "TIMER %s: setitimer failed. errno = %d\n", func, errno )

#define SET_OSTIMER( otp, itv, func )														\
	if ( TV_ISZERO( (itv).it_value ) )														\
		report( "TIMER %s: zero interval\n", func ) ;									\
	else																								\
	{																									\
		OSTIMER_SET( func, otp, itv ) ;														\
	}


/* ARGSUSED */
PRIVATE void report( char *fmt, ... )
{
#ifdef DEBUG_REPORT_ERRORS
   va_list ap ;

   va_start( ap, fmt ) ;
   (void) vfprintf( stderr, fmt, ap ) ;
#ifdef DEBUG
   abort() ;
   _exit( 1 ) ;
#endif
#endif   /* DEBUG_REPORT_ERRORS */
}


/*
 * Initialize the fields of struct timer that are used by the ostimer code
 */
int __ostimer_newtimer(timer_s *tp, enum timer_types type)
{
	ostimer_s			*otp = __ostimer_init( tp, type ) ;

	if ( otp == OSTIMER_NULL )
		return( TIMER_ERR ) ;

	tp->t_ostimer = otp ;
	TV_ZERO( tp->t_expiration ) ;
	TV_ZERO( tp->t_interval ) ;
	tp->t_count = 0 ;
	return( TIMER_OK ) ;
}




/*
 * The following variables are used to handle the following scenario:
 *
 *		1. INTERRUPT happens ==> ostimer_interrupt called
 *		2. Timers A and B expire.
 *		3. The function associated with A is invoked and running
 *		4. INTERRUPT happens ==> ostimer_interrupt called
 *		5. Timer C expires.
 *		6. Function of timer C invoked and returns with a jmp_buf.
 *
 * If we longjmp to the jmp_buf returned by the function of timer C the
 * function of timer B will never be called and the function of timer A
 * will never finish.
 * What we do instead is have ostimer_interrupt check the call_level
 * and if greater than 1, then just save the jmp_buf returned by the
 * function of timer C (only if there is no other ret_env) and simply return.
 *
 * Notice that there can be only one ret_env (since there is only 1 control
 * flow).
 *
 * XXX:  this complexity stems from the fact that we allow interrupts while
 *			the timer functions are running. Is there a good reason for this ?
 *			(probably because we have to allow interrupts of other timer types).
 */
static int call_level ;
static int have_env ;
static jmp_buf ret_env ;

#define MAX_EXPIRED					20

/*
 * ostimer_interrupt invokes the functions of the timers that expired.
 * Since we don't know in advance how many timers expired, we follow a
 * two-step procedure:
 *
 *		Step 1: collect all expired timers
 *		Step 2: invoke timer actions
 *
 * The expired timers are collected in an array stored on the stack.
 * If the array overflows, we arrange for another timer interrupt as
 * soon as possible to service remaining timers. The reason we don't
 * allocate the array using malloc is that malloc is not guaranteed
 * to be reentrant and tracking timing-related dynamic memory allocation
 * problems is guaranteed to be a nightmare.
 *
 * Notice that *all* timer interrupts are blocked during step 1.
 */
void __ostimer_interrupt(register ostimer_s *otp)
{
	struct timeval		current_time ;
	register timer_s	*tp ;
	timer_s				*expired_timers[ MAX_EXPIRED ] ;
	unsigned				n_expired = 0 ;
	int					i ;

	if ( timer_pq_head( otp->ost_timerq.tq_handle ) == TIMER_NULL )
		return ;

	call_level++ ;

	(*otp->ost_get_current_time)( &current_time ) ;

	/*
	 * Get all timers that expired
	 */
	for ( ;; )
	{
		tp = timer_pq_head( otp->ost_timerq.tq_handle ) ;

		if ( tp == TIMER_NULL || TV_GT( tp->t_expiration, current_time ) ||
																	n_expired == MAX_EXPIRED )
			break ;
	
		tp = timer_pq_extract_head( otp->ost_timerq.tq_handle ) ;
		if ( tp->t_state == TICKING )
		{
			tp->t_state = INACTIVE ;

			tp->t_count++ ;
			if ( tp->t_blocked )
			{
				if ( tp->t_act == IDLE )
					tp->t_act = PENDING ;
				else if ( tp->t_act == INVOKED )
					tp->t_act = SCHEDULED ;
			}
			else		/* not blocked */
			{
				if ( tp->t_act == IDLE || tp->t_act == INVOKED )
				{
					tp->t_act = SCHEDULED ;
					expired_timers[ n_expired++ ] = tp ;
				}
			}
		}
	}

	/*
	 * Check which timers have regular expiration intervals
	 */
	for ( i = 0 ; i < n_expired ; i++ )
	{
		tp = expired_timers[ i ] ;

		if ( ! TV_ISZERO( tp->t_interval ) )
		{
			tp->t_state = TICKING ;
			TV_ADD( tp->t_expiration, tp->t_expiration, tp->t_interval ) ;
			/*
			 * We shouldn't have an insertion problem since we just extracted
			 * the timers from the queue (i.e. there should be enough room)
			 */
			(void) timer_pq_insert( otp->ost_timerq.tq_handle, tp ) ;
		}
	}

	tp = timer_pq_head( otp->ost_timerq.tq_handle ) ;

	if ( tp != TIMER_NULL )
	{
		struct itimerval itv ;

		TV_ZERO( itv.it_interval ) ;
		/*
		 * Check if we had too many expired timers
		 */
		if ( TV_LE( tp->t_expiration, current_time ) )
		{
			itv.it_value.tv_sec = 0 ;
			itv.it_value.tv_usec = 1 ;		/* schedule an interrupt ASAP */
			/* XXX:	this trick will result in another call to
			 *			ostimer_interrupt. So why don't we just call it
			 *			recursively, instead of taking another timer interrupt ?
			 */
		}
		else
			TV_SUB( itv.it_value, tp->t_expiration, current_time ) ;

		SET_OSTIMER( otp, itv, "ostimer_interrupt" ) ;
	}

#ifdef DEBUG_MSGS
   printf( "\t\t%d timers expired\n", n_expired ) ;
#endif

   /*
    * Invoke the functions of all expired timers
	 */
   for ( i = 0 ; i < n_expired ; i++ )
   {
      tp = expired_timers[ i ] ;
		if ( __timer_invoke( tp ) != DESTROYED &&
					! have_env && ( tp->t_action.ta_flags & TIMER_LONGJMP ) )
		{
			(void) memcpy( (char *)ret_env,
						(char *)tp->t_action.ta_env, sizeof( jmp_buf ) ) ;
			have_env = TRUE ;
		}
   }

	call_level-- ;

	/*
	 * If this is not a recursive call, and there is a jmp_buf, use it.
	 */
	if ( call_level == 0 && have_env )
	{
		have_env = FALSE ;
		longjmp( ret_env, 1 ) ;
	}
}


/*
 * Carry out the timer action.
 * If			the call level is 0
 *		AND	there is not already an environment to longjmp to
 *	 	AND	this timer has such an environment
 *	then
 *		longjmp to that environment
 *
 * Notice that all timer interrupts are blocked while this function is running.
 * Therefore, none of the global variables checked can change.
 */
PRIVATE void invoke_protocol(timer_s *tp)
{
	if ( __timer_invoke( tp ) != DESTROYED &&
					call_level == 0 && ! have_env &&
									( tp->t_action.ta_flags & TIMER_LONGJMP ) )
		longjmp( tp->t_action.ta_env, 1 ) ;
}


/*
 * Add a timer to the specified OS timer's queue
 * We assume that the timer already has a valid state and action
 * associated with it.
 * We also assume that no operations (block etc) are applied to the
 * timer while this function is running.
 */
int __ostimer_add(ostimer_s *otp, register timer_s *tp, struct itimerval *itvp, enum timer_timetypes time_type)
{
	struct timeval		current_time ;
	int					expired ;

	/*
	 * While this function (__ostimer_add) is running, this will be our
	 * notion of the current time.
	 * In reality, there may be some time skew as this function
	 * is running, possibly because of swapping.
	 */
	(*otp->ost_get_current_time)( &current_time ) ;

   /*
    * Since we use absolute time for our calculations,
    * convert the user-specified time to that, if necessary
    *
    * Also check if the timer has already expired. There are 2 possibilities:
    *
    *    1. a zero interval for TIMER_RELATIVE
    *    2. a time before current time for TIMER_ABSOLUTE
	 *
	 * Note that we always calculate t_expiration in case the user has
	 * specified an it_interval.
    */

	if ( time_type == TIMER_RELATIVE )
	{
		/*
		 * timer_start has verified that it_value is not negative
		 */
		TV_ADD( tp->t_expiration, current_time, itvp->it_value ) ;
		expired = TV_ISZERO( itvp->it_value ) ;
	}
	else
	{
		tp->t_expiration = itvp->it_value ;
		expired = TV_LE( tp->t_expiration, current_time ) ;
	}

	tp->t_interval = itvp->it_interval ;

	if ( expired )
	{
		tp->t_count++ ;
		tp->t_act = SCHEDULED ;

		if ( TV_ISZERO( tp->t_interval ) )
		{
			invoke_protocol( tp ) ;
			return( TIMER_OK ) ;
		}
	
		/*
		 * Keep expiring the timer until it exceeds the current time
		 */
		time_type = TIMER_ABSOLUTE ;
		for ( ;; )
		{
			TV_ADD( tp->t_expiration, tp->t_expiration, tp->t_interval ) ;
			expired = TV_LE( tp->t_expiration, current_time ) ;
			if ( ! expired )
				break ;
			tp->t_act = SCHEDULED ;
			tp->t_count++ ;
		}
		invoke_protocol( tp ) ;
	}

	if ( timer_pq_insert( otp->ost_timerq.tq_handle, tp ) == PQ_ERR )
		HANDLE_ERROR( tp->t_flags, TIMER_ERR, tp->t_errnop, TIMER_ECANTINSERT,
			"TIMER __ostimer_add: can't insert timer in priority queue\n" ) ;

	tp->t_state = TICKING ;

	/*
	 * Now check if we will need to set the timer again
	 */
	if ( tp == timer_pq_head( otp->ost_timerq.tq_handle ) )
	{
      /*
       * Check if the user specified relative time.
       * If so, use the value given (for better accuracy), otherwise compute
       * a new value.
       * It is possible that by now the timer that was at the head
       * of the queue expired and a signal is pending. This can happen
       * if we are swapped out. The signal handling function will
       * obviously expire both the new timer and the old one, so our
       * setting a new timer value may cause a signal at a later time
       * when the queue is empty. The signal handling function should
       * be able to deal with an empty queue.
       */
		struct itimerval itv ;

		TV_ZERO( itv.it_interval ) ;
		if ( time_type == TIMER_RELATIVE )
			itv.it_value = itvp->it_value ;
		else
			TV_SUB( itv.it_value, tp->t_expiration, current_time ) ;
		SET_OSTIMER( otp, itv, "__ostimer_add" ) ;
	}

	return( TIMER_OK ) ;
}


/*
 * Remove the specified timer from the OS timer's queue
 * Timer interrupts should be blocked.
 */
void __ostimer_remove(ostimer_s *otp, timer_s *tp)
{
	struct itimerval	itv ;
	struct timeval		current_time ;
	timer_s				*new_head_timer, *old_head_timer ;

	old_head_timer = timer_pq_head( otp->ost_timerq.tq_handle ) ;
	timer_pq_delete( otp->ost_timerq.tq_handle, tp ) ;
	new_head_timer = timer_pq_head( otp->ost_timerq.tq_handle ) ;

	if ( old_head_timer != new_head_timer )
	{
		int do_setitimer ;

		if ( new_head_timer != TIMER_NULL )
		{
			(*otp->ost_get_current_time)( &current_time ) ;

			/*
			 * If the head_timer is less than or equal to the current time,
			 * the interrupt must be pending, so we leave the OS timer running.
			 * Otherwise, we restart the OS timer.
			 */
			if ( TV_LE( current_time, new_head_timer->t_expiration ) )
				do_setitimer = FALSE ;
			else
			{
				do_setitimer = TRUE ;
				TV_SUB( itv.it_value, new_head_timer->t_expiration, current_time ) ;
			}
		}
		else				/* queue is empty */
		{
			do_setitimer = TRUE ;
			TV_ZERO( itv.it_value ) ;
		}

		if ( do_setitimer )
		{
			TV_ZERO( itv.it_interval ) ;
			OSTIMER_SET( "__ostimer_remove", otp, itv ) ;
		}
	}
}

