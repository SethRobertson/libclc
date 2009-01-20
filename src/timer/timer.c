/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

static const char version[] = VERSION;

#include <unistd.h>
#include <string.h>
#include "timemacros.h"
#include "impl.h"
#include "defs.h"
#include "clchack.h"

#define TIMER_H_NULL					((timer_h)NULL)


int timer_errno ;



/*
 * Create a timer of the specified type.
 * Returns a timer handle
 */
timer_h clc_timer_create(enum timer_types type, int flags, int *errnop)
{
	int		*errp = ( errnop != NULL ) ? errnop : &timer_errno ;
	timer_s	*tp ;

	if ( type != TIMER_REAL && type != TIMER_VIRTUAL && type != TIMER_PROF )
			HANDLE_ERROR( flags, TIMER_H_NULL, errp, TIMER_EBADTYPE,
							"TIMER timer_create: bad timer type\n" ) ;
		
	tp = TIMER_ALLOC() ;
	if ( tp == NULL )
	{
		*errp = TIMER_ENOMEM ;
		return( TIMER_H_NULL ) ;
	}

	tp->t_state = INACTIVE ;
	tp->t_act = IDLE ;
	tp->t_blocked = FALSE ;

	tp->t_errnop = errp ;
	tp->t_flags = flags & TIMER_CREATE_FLAGS ;
	tp->t_action.ta_func = NULL ;
	tp->t_action.ta_arg = NULL ;
	tp->t_action.ta_flags = TIMER_NOFLAGS ;
	if ( __ostimer_newtimer( tp, type ) == TIMER_ERR )
	{
		TIMER_FREE( tp ) ;
		return( TIMER_H_NULL ) ;
	}
	return( (timer_h) tp ) ;
}


void clc_timer_destroy(timer_h handle)
{
	timer_s	*tp = TP( handle ) ;

	__ostimer_blockall() ;

	if ( tp->t_state == TICKING )
	{
		__ostimer_remove( tp->t_ostimer, tp ) ;
		tp->t_state = DESTROYED ;
	}

	if ( tp->t_act == IDLE || tp->t_act == PENDING )
		TIMER_FREE( tp ) ;

	__ostimer_unblockall() ;
}


int clc_timer_start(timer_h handle, struct itimerval *itvp, enum timer_timetypes time_type, struct timer_action *ap)
{
	int					result ;
	int					ok_to_start ;
	timer_s				*tp	= TP( handle ) ;
	struct os_timer	*otp	= tp->t_ostimer ;

	__ostimer_blockall() ;

	/*
	 * We allow invoking timer_start from within the user-specified action
	 * after the timer has expired. However, we do not allow this for
	 * timers that have a t_interval (these timers stay at the TICKING state).
	 */
	ok_to_start = tp->t_state == INACTIVE &&
							( tp->t_act == IDLE || tp->t_act == INVOKED ) ;

	if ( ! ok_to_start )
	{
		__ostimer_unblockall() ;
		HANDLE_ERROR( tp->t_flags, TIMER_ERR, tp->t_errnop, TIMER_EBADSTATE,
			"TIMER timer_start: timer state does not allow this operation\n" ) ;
	}

	if ( itvp->it_value.tv_sec < 0 || itvp->it_value.tv_usec < 0 )
	{
		__ostimer_unblockall() ;
		HANDLE_ERROR( tp->t_flags, TIMER_ERR, tp->t_errnop, TIMER_EBADTIME,
			"TIMER timer_start: neg time value)\n" ) ;
	}

	tp->t_action = *ap ;
	tp->t_action.ta_flags &= TIMER_START_FLAGS ;

	result = __ostimer_add( otp, tp, itvp, time_type ) ;
	__ostimer_unblockall() ;
	return( result ) ;
}


void clc_timer_stop(timer_h handle)
{
	timer_s *tp = TP( handle ) ;

	__ostimer_blockall() ;

	if ( tp->t_state == TICKING )
	{
		__ostimer_remove( tp->t_ostimer, tp ) ;
		tp->t_state = INACTIVE ;
	}

	if ( tp->t_act == SCHEDULED )
		tp->t_act = INVOKED ;		/* to avoid the invocation */
	else if ( tp->t_act == PENDING )
		tp->t_act = IDLE ;

	tp->t_blocked = FALSE ;

	__ostimer_unblockall() ;
}


void clc_timer_block(timer_h handle)
{
	timer_s *tp = TP( handle ) ;

	__ostimer_blockall() ;

	if (tp->t_state == TICKING ||
	    (tp->t_state == INACTIVE &&
	     (tp->t_act == PENDING || tp->t_act == SCHEDULED)))
		tp->t_blocked = TRUE ;

	__ostimer_unblockall() ;
}


void clc_timer_unblock(timer_h handle)
{
	timer_s *tp = TP( handle ) ;

	__ostimer_blockall() ;

	if ( tp->t_blocked )
	{
		tp->t_blocked = FALSE ;
		if ( tp->t_act == PENDING )
		{
			tp->t_act = SCHEDULED ;
			(void) __timer_invoke( tp ) ;
		}
	}

	__ostimer_unblockall() ;
}


unsigned clc_timer_expirations(timer_h handle)
{
	return( TP( handle )->t_count ) ;
}



/*
 * Invoke the action of the specified timer
 * All timer interrupts should be blocked when this function is invoked
 * Returns TRUE if
 */
enum timer_state __timer_invoke(register timer_s *tp)
{
	enum timer_state state ;

	/*
	 * The reason for the infinite loop is that the timer may reexpire
	 * while its function is being invoked.
	 */
	for ( ;; )
	{
		/*
		 * This is the INVOKE part
		 */
		if ( tp->t_blocked )
			tp->t_act = PENDING ;
		else
		{
			if ( tp->t_state != DESTROYED && tp->t_act == SCHEDULED )
			{
				void	(*func)()	= tp->t_action.ta_func ;
				void	*arg			= tp->t_action.ta_arg ;
				int	flags 		= tp->t_action.ta_flags ;

				tp->t_act = INVOKED ;
				tp->t_expirations = tp->t_count ;
				tp->t_count = 0 ;
				if ( func != NULL )
				{
					int unblock_all_intrs = ! ( flags & TIMER_BLOCK_ALL ) ;
					int unblock_all_but_same_intr = ! ( flags & TIMER_BLOCK_SAME ) ;
				
					if ( unblock_all_intrs )
						__ostimer_unblockall() ;
					else if ( unblock_all_but_same_intr )
						__ostimer_unblockall_except( tp->t_ostimer ) ;

					(*func)( tp, arg ) ;

					if ( unblock_all_intrs || unblock_all_but_same_intr )
						__ostimer_blockall() ;
				}
				else if ( arg != NULL )
				{
					int *ip = (int *) arg ;

					if ( flags & TIMER_INC_VAR )
						*ip += tp->t_expirations ;
					else
						*ip = 1 ;
				}
			}
		}

		state = tp->t_state ;

		/*
		 * This is the RETURN part
		 */
		if ( tp->t_state == DESTROYED )
			TIMER_FREE( tp ) ;
		else
		{
			if ( tp->t_act == INVOKED )
				tp->t_act = IDLE ;
			else if ( tp->t_act == SCHEDULED )
				continue ;
		}
		break ;
	}
	return( state ) ;
}

