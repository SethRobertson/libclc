/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

#ifndef __TIMER_H
#define __TIMER_H

/*
 * $Id: timer.h,v 1.2 2003/06/17 05:10:50 seth Exp $
 */

#include <sys/time.h>
#include <setjmp.h>

/*
 * Return values
 */
#define TIMER_OK			0
#define TIMER_ERR			(-1)

/*
 * errno values
 */
#define TIMER_ENOMEM				1
#define TIMER_EBADTYPE			2
#define TIMER_EBADSTATE			3
#define TIMER_EBADTIME			4
#define TIMER_ESIGPROBLEM		5
#define TIMER_ECANTINSERT		6
#define TIMER_ENOTAVAILABLE	7

/*
 * flags
 */
#define TIMER_NOFLAGS				0x0
#define TIMER_RETURN_ERROR			0x1
#define TIMER_INC_VAR				0x2
#define TIMER_BLOCK_SAME			0x4
#define TIMER_BLOCK_ALL				0x8
#define TIMER_LONGJMP				0x10

enum timer_types { TIMER_REAL = 0, TIMER_VIRTUAL = 1, TIMER_PROF = 2 };
enum timer_timetypes { TIMER_ABSOLUTE, TIMER_RELATIVE };

struct timer_action
{
	int		ta_flags ;
	void		(*ta_func)() ;
	void		*ta_arg ;
	jmp_buf	ta_env ;
} ;

#ifdef __ARGS
#undef __ARGS
#endif

#ifdef PROTOTYPES
#   define __ARGS( s )           s
#else
#   define __ARGS( s )           ()
#endif

typedef void *timer_h ;

timer_h clc_timer_create		__ARGS( (
											enum timer_types type,
											int flags,
											int *errnop
										) ) ;
void clc_timer_destroy 		__ARGS( ( timer_h handle ) ) ;

int clc_timer_start 			__ARGS( (
											timer_h handle,
											struct itimerval *itvp,
											enum timer_timetypes time_type,
											struct timer_action *ap
										) ) ;
void clc_timer_stop 			__ARGS( ( timer_h handle ) ) ;

void clc_timer_block 			__ARGS( ( timer_h handle ) ) ;
void clc_timer_unblock 		__ARGS( ( timer_h handle ) ) ;
void clc_timer_block_type(enum timer_types type);
void clc_timer_unblock_type(enum timer_types type);

unsigned clc_timer_expirations __ARGS( ( timer_h handle ) ) ;

void clc_timer_block_all		__ARGS( ( enum timer_types type ) ) ;
void clc_timer_unblock_all	__ARGS( ( enum timer_types type ) ) ;

char *clc_timer_strerr		__ARGS( ( timer_h handle ) ) ;

extern int clc_timer_errno ;

#endif	/* __TIMER_H */

