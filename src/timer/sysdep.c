/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "defs.h"
#include "impl.h"
#include "ostimer.h"
#include "timemacros.h"
#include "timer.h"
#include "clchack.h"

extern int getrusage(int, struct rusage *);

PRIVATE void sigalrm_handler(void) ;
PRIVATE void sigvtalrm_handler(void) ;
PRIVATE void sigprof_handler(void) ;

PRIVATE void get_real_time(struct timeval *tvp) ;
PRIVATE void get_virtual_time(struct timeval *tvp) ;
PRIVATE void get_prof_time(struct timeval *tvp) ;

#define SIGNOSIG							0

/*
 * The reason for having the AVAILABLE/UNAVAILABLE fields instead of
 * simply not having entries if a specified timer type is not supported,
 * is that we use the timer type as an index into the os_timers[] array.
 */
static struct os_timer os_timers[] =
   {
		{
#if defined( ITIMER_REAL ) && ! defined( NO_ITIMER_REAL )
			AVAILABLE,	 ITIMER_REAL,		SIGALRM,
#else
			UNAVAILABLE,	0,						SIGNOSIG,
#endif
			TIMER_REAL,		sigalrm_handler,     get_real_time,	{NULL,0},
#if defined(NSIG) && NSIG > 32
			{{0,}},
#else  /* NSIG <= 32 */
			0,
#endif /* NSIG <= 32 */
		},

		{
#if defined( ITIMER_VIRTUAL ) && ! defined( NO_ITIMER_VIRTUAL )
			AVAILABLE,	ITIMER_VIRTUAL,	SIGVTALRM,
#else
			UNAVAILABLE,	0,						SIGNOSIG,
#endif
      	TIMER_VIRTUAL,	sigvtalrm_handler,   get_virtual_time,	{NULL,0},
#if defined(NSIG) && NSIG > 32
			{{0,}},
#else  /* NSIG <= 32 */
			0,
#endif /* NSIG <= 32 */
		},

		{
#if defined( ITIMER_PROF ) && ! defined( NO_ITIMER_PROF )
			AVAILABLE,	ITIMER_PROF,		SIGPROF,
#else
			UNAVAILABLE,	0,					SIGNOSIG,
#endif
			TIMER_PROF,   	sigprof_handler,     get_prof_time,	{NULL,0},
#if defined(NSIG) && NSIG > 32
			{{0,}},
#else  /* NSIG <= 32 */
			0,
#endif /* NSIG <= 32 */
		},

		{	UNAVAILABLE,	0,						SIGNOSIG,
      	TIMER_REAL,		NULL,                NULL,	{NULL,0},
#if defined(NSIG) && NSIG > 32
			{{0,}},
#else  /* NSIG <= 32 */
			0,
#endif /* NSIG <= 32 */
		}
   } ;


/*
 * The timer_block_mask blocks all timer signals when a timer signal
 * happens (using the sa_mask field of struct sigaction). This is necessary
 * when the user function associated with the timer does not return.
 * Consider the following scenario:
 *    The user creates 2 timers, one TIMER_REAL (signal: SIGALRM), one
 *    TIMER_VIRTUAL (signal: SIGVTALRM).
 *    SIGALRM occurs first but before it is handled, SIGVTALRM happens.
 *    At this point both SIGARLM and SIGVTALRM are blocked.
 *    SIGVTALRM gets unblocked and the function for the TIMER_VIRTUAL is
 *    invoked and never returns. The function for the TIMER_REAL is never
 *    invoked (and any TIMER_REAL timers never expire).
 */
#ifndef NO_POSIX_SIGS
static sigset_t timer_block_mask ;
#else
static int timer_block_mask ;
#endif

static int timer_block_mask_set ;         /* flag */


/*
 * Initialize the timer_block_mask.
 * As a side-effect it also initializes the block_mask of each ostimer.
 */
PRIVATE void set_timer_block_mask(void)
{
   ostimer_s *otp ;

#ifndef NO_POSIX_SIGS
   (void) sigemptyset( &timer_block_mask ) ;
#else
    /* timer_block_mask is a global variable so it is initialized to 0 */
#endif

   for ( otp = &os_timers[ 0 ] ; otp->ost_handler ; otp++ )
   {
#ifndef NO_POSIX_SIGS
      (void) sigemptyset( &otp->ost_block_mask ) ;
      (void) sigaddset( &otp->ost_block_mask, otp->ost_signal ) ;
      (void) sigaddset( &timer_block_mask, otp->ost_signal ) ;
#else
      otp->ost_block_mask = sigmask( otp->ost_signal ) ;
      timer_block_mask |= otp->ost_block_mask ;
#endif
   }

   timer_block_mask_set = TRUE ;
}


PRIVATE ostimer_s *ostimer_find(enum timer_types type)
{
	register ostimer_s *otp ;

	for ( otp = os_timers ; otp->ost_handler ; otp++ )
		if ( otp->ost_timertype == type )
			return( otp->ost_availability == AVAILABLE ? otp : OSTIMER_NULL ) ;
	return( OSTIMER_NULL ) ;
}


PRIVATE int time_compare(pq_obj p1, pq_obj p2)
{
   return( TV_LT( TP( p1 )->t_expiration, TP( p2 )->t_expiration ) ) ;
}


/*
 * Initialize an OS timer. The initialization steps are:
 *
 *    create priority queue
 *    install signal handler
 *
 * We also initialize the timer_block_mask if it has not been initialized yet.
 */
ostimer_s *__ostimer_init(timer_s *tp, enum timer_types type)
{
#ifndef NO_POSIX_SIGS
   struct sigaction  sa ;
#else
   struct sigvec     sv ;
#endif
   ostimer_s   		*otp ;
   struct timer_q    *tqp ;

	/*
	 * Find the corresponding ostimer
	 */
	if ( ( otp = ostimer_find( type ) ) == OSTIMER_NULL )
		HANDLE_ERROR( tp->t_flags, OSTIMER_NULL,
			tp->t_errnop, TIMER_ENOTAVAILABLE,
				"TIMER __ostimer_init: requested timer type not available\n" ) ;

	/*
	 * We use the value of ost_timerq to determine if the os_timer
	 * has been initialized.
	 */
   tqp = &otp->ost_timerq ;
	if ( tqp->tq_handle )
		return( otp ) ;

   tqp->tq_handle = pq_create( time_compare,
				tp->t_flags & TIMER_RETURN_ERROR ? PQ_RETURN_ERROR : PQ_NOFLAGS,
										&tqp->tq_errno ) ;
   if ( tqp->tq_handle == NULL )
   {
      *tp->t_errnop = TIMER_ENOMEM ;
      return( OSTIMER_NULL ) ;
   }

   if ( ! timer_block_mask_set )
      set_timer_block_mask() ;

#ifndef NO_POSIX_SIGS
   sa.sa_handler = otp->ost_handler ;
   sa.sa_mask = timer_block_mask ;
   sa.sa_flags = 0 ;
   if ( sigaction( otp->ost_signal, &sa, SIGACTION_NULL ) == -1 )
#else
   sv.sv_handler = otp->ost_handler ;
   sv.sv_mask = timer_block_mask ;
   sv.sv_flags = 0 ;
   if ( sigvec( otp->ost_signal, &sv, SIGVEC_NULL ) == -1 )
#endif
      HANDLE_ERROR( tp->t_flags, OSTIMER_NULL, tp->t_errnop, TIMER_ESIGPROBLEM,
         "TIMER __ostimer_init: signal handler installation failed\n" ) ;
   return( otp ) ;
}


/*
 * timer_* functions that need access to private data of ostimer
 */
void clc_timer_block_type(enum timer_types type)
{
   ostimer_s			*otp = ostimer_find( type ) ;

	if ( otp == OSTIMER_NULL )
		return ;

#ifndef NO_POSIX_SIGS
   (void) sigprocmask( SIG_BLOCK, &otp->ost_block_mask, SIGSET_NULL ) ;
#else
   (void) sigblock( otp->ost_block_mask ) ;
#endif
}


void clc_timer_unblock_type(enum timer_types type)
{
   ostimer_s			*otp = ostimer_find( type ) ;

	if ( otp == OSTIMER_NULL )
		return ;

#ifndef NO_POSIX_SIGS
   (void) sigprocmask( SIG_UNBLOCK, &otp->ost_block_mask, SIGSET_NULL ) ;
#else
	{
		int old_mask = sigblock( ~0 ) ;

		(void) sigsetmask( old_mask & ~otp->ost_block_mask ) ;
	}
#endif
}


void __ostimer_blockall(void)
{
#ifndef NO_POSIX_SIGS
   (void) sigprocmask( SIG_BLOCK, &timer_block_mask, SIGSET_NULL ) ;
#else
   (void) sigblock( timer_block_mask ) ;
#endif
}


void __ostimer_unblockall(void)
{
#ifndef NO_POSIX_SIGS
   (void) sigprocmask( SIG_UNBLOCK, &timer_block_mask, SIGSET_NULL ) ;
#else
   int old_mask = sigblock( ~0 ) ;

   (void) sigsetmask( old_mask & ~timer_block_mask ) ;
#endif
}


void __ostimer_unblockall_except(ostimer_s *otp)
{
#ifndef NO_POSIX_SIGS
   sigset_t new_mask = timer_block_mask ;

   (void) sigdelset( &new_mask, otp->ost_signal ) ;
   (void) sigprocmask( SIG_UNBLOCK, &new_mask, SIGSET_NULL ) ;
#else
   int old_mask = sigblock( ~0 ) ;

   (void) sigsetmask( ( old_mask & ~timer_block_mask )
                                          | otp->ost_block_mask ) ;
#endif
}


PRIVATE void sigalrm_handler(void)
{
#ifdef DEBUG_MSGS
   printf( "\tSIGALRM happened\n" ) ;
#endif
   __ostimer_interrupt( &os_timers[ (int)TIMER_REAL ] ) ;
}


PRIVATE void sigvtalrm_handler(void)
{
#ifdef DEBUG_MSGS
   printf( "\tSIGVTALRM happened\n" ) ;
#endif
   __ostimer_interrupt( &os_timers[ (int)TIMER_VIRTUAL ] ) ;
}


PRIVATE void sigprof_handler(void)
{
#ifdef DEBUG_MSGS
   printf( "\tSIGPROF happened\n" ) ;
#endif
   __ostimer_interrupt( &os_timers[ (int)TIMER_PROF ] ) ;
}


PRIVATE void get_real_time(struct timeval *tvp)
{
#if defined( ITIMER_REAL ) && ! defined( NO_ITIMER_REAL )
   (void) gettimeofday( tvp, TIMEZONE_NULL ) ;
#endif
}


PRIVATE void get_virtual_time(struct timeval *tvp)
{
#if defined( ITIMER_VIRTUAL ) && ! defined( NO_ITIMER_VIRTUAL )
   struct rusage ru ;

   (void) getrusage( RUSAGE_SELF, &ru ) ;
   *tvp = ru.ru_utime ;
#endif
}


PRIVATE void get_prof_time(struct timeval *tvp)
{
#if defined( ITIMER_PROF ) && ! defined( NO_ITIMER_PROF )
   struct rusage ru ;

   (void) getrusage( RUSAGE_SELF, &ru ) ;
   TV_ADD( *tvp, ru.ru_utime, ru.ru_stime ) ;
#endif
}


