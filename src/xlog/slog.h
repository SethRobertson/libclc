/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */


/*
 * $Id: slog.h,v 1.2 2003/06/17 05:10:56 seth Exp $
 */


struct syslog
{
	int sl_facility ;
	int sl_default_level ;
} ;


struct syslog_parms
{
   int 		slp_n_xlogs ;              /* # of xlogs using syslog */
   int 		slp_logopts ;              /* used at openlog */
   int 		slp_facility ;
   char		*slp_ident ;					/* used at openlog */
} ;

#define SYSLOG( xp )         ((struct syslog *)(xp)->xl_data)

