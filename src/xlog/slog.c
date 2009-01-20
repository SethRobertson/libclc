/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

UNUSED static char RCSid[] = "$Id: slog.c,v 1.2 2003/06/17 05:10:56 seth Exp $" ;

#include <stdarg.h>

#ifndef NO_SYSLOG
#	ifndef SYSLOG_IN_SYS
#		include <syslog.h>
#	else
#		include <sys/syslog.h>
#	endif
#endif

#include "xlog.h"
#include "impl.h"
#include "slog.h"
#include "clchack.h"

#define MSGBUFSIZE			2048

int strx_nprint( char *buf, int len, char *format, ... );

PRIVATE int syslog_init(xlog_s *xp, va_list ap) ;
PRIVATE void syslog_fini(xlog_s *xp) ;
PRIVATE int syslog_control(xlog_s *xp, xlog_cmd_e cmd, va_list ap) ;
PRIVATE int syslog_write(xlog_s *xp, char *buf, int len, int flags, va_list ap) ;
PRIVATE int syslog_parms(va_list ap) ;

static struct syslog_parms parms =
   {
      0,
#ifndef NO_SYSLOG

#ifdef LOG_PID
		LOG_PID +
#endif
#ifdef LOG_NOWAIT
		LOG_NOWAIT +
#endif
		0,
      LOG_USER,

#else		/* NO_SYSLOG */

		0,
		0,

#endif	/* ! NO_SYSLOG */
      "XLOG"
   } ;


struct xlog_ops __xlog_syslog_ops =
	{
		syslog_init,
		syslog_fini,
		syslog_write,
		syslog_control,
		syslog_parms
	} ;

#ifdef NO_SYSLOG

/*
 * Notice that the following functions will never be invoked since
 * the xlog_* functions will not call them. However, we need to define
 * them so that we don't have any unresolved references; and we define
 * them without any arguments.
 */
PRIVATE void syslog()
{
}

PRIVATE void openlog()
{
}

PRIVATE void closelog()
{
}

#else

void closelog() ;

#endif	/* NO_SYSLOG */


/*
 * Expected arguments:
 *		facility, level
 */
PRIVATE int syslog_init(xlog_s *xp, va_list ap)
{
	register struct syslog_parms	*slp = &parms ;
	struct syslog						*sp ;

	sp = NEW( struct syslog ) ;
	if ( sp == NULL )
		return( XLOG_ENOMEM ) ;

	sp->sl_facility = va_arg( ap, int ) ;
	sp->sl_default_level = va_arg( ap, int ) ;
	if ( slp->slp_n_xlogs++ == 0 )
		openlog( slp->slp_ident, slp->slp_logopts, slp->slp_facility ) ;
	xp->xl_data = sp ;
	return( XLOG_ENOERROR ) ;
}


PRIVATE void syslog_fini(xlog_s *xp)
{
	if ( --parms.slp_n_xlogs == 0 )
		closelog() ;
	FREE( SYSLOG( xp ) ) ;
	xp->xl_data = NULL ;
}


PRIVATE int syslog_control(xlog_s *xp, xlog_cmd_e cmd, va_list ap)
{
	switch ( cmd )
	{
		case XLOG_LEVEL:
			SYSLOG( xp )->sl_default_level = va_arg( ap, int ) ;
			break ;

		case XLOG_FACILITY:
			SYSLOG( xp )->sl_facility = va_arg( ap, int ) ;
			break ;
	
		case XLOG_PREEXEC:
			closelog() ;
			break ;

		case XLOG_POSTEXEC:
			if ( parms.slp_n_xlogs )
				openlog( parms.slp_ident, parms.slp_logopts, parms.slp_facility ) ;
			break ;
	
		case XLOG_GETFD:
			return( XLOG_EBADOP ) ;
		default:
			break;
	}
	return( XLOG_ENOERROR ) ;
}


PRIVATE int syslog_write(xlog_s *xp, char *buf, int len, int flags, va_list ap)
{
	int	level ;
	int	syslog_arg ;
	char	prefix[ MSGBUFSIZE ] ;
	int	prefix_size = sizeof( prefix ) ;
	int	prefix_len = 0 ;
	int	cc ;
	char	*percent_m_pos ;
	int	action_flags = ( flags | xp->xl_flags ) ;

	if ( flags & XLOG_SET_LEVEL )
		level = va_arg( ap, int ) ;
	else
		level = SYSLOG( xp )->sl_default_level ;
	syslog_arg = SYSLOG( xp )->sl_facility + level ;

	if ( action_flags & XLOG_PRINT_ID )
	{
		cc = strx_nprint( &prefix[ prefix_len ], prefix_size, "%s: ",
							xp->xl_id ) ;
		prefix_len += cc ;
		prefix_size -= cc ;
	}

	if ( ( action_flags & XLOG_NO_ERRNO ) ||
						( percent_m_pos = __xlog_add_errno( buf, len ) ) == NULL )
		syslog( syslog_arg, "%.*s%.*s", prefix_len, prefix, len, buf ) ;
	else
	{
		int cc_before_errno = percent_m_pos - buf ;
		int cc_after_errno = len - cc_before_errno - 2 ;
		char *ep ;
		char errno_buf[ 100 ] ;
		unsigned size = sizeof( errno_buf ) ;
	
		ep = __xlog_explain_errno( errno_buf, &size ) ;
		syslog( syslog_arg, "%.*s%.*s%.*s%.*s",
				prefix_len, prefix,
					cc_before_errno, buf,
						(int)size, ep,
							cc_after_errno, &percent_m_pos[ 2 ] ) ;
	}
	return( XLOG_ENOERROR ) ;
}


PRIVATE int syslog_parms(va_list ap)
{
	char *__xlog_new_string(char *s) ;
	char *id = __xlog_new_string( va_arg( ap, char * ) ) ;

	if ( id == NULL )
		return( XLOG_ENOMEM ) ;
	parms.slp_ident = id ;
	parms.slp_logopts = va_arg( ap, int ) ;
	parms.slp_facility = va_arg( ap, int ) ;
	return( XLOG_ENOERROR ) ;
}

