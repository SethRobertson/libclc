/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

UNUSED static char RCSid[] = "$Id: tietest.c,v 1.2 2003/06/17 05:10:54 seth Exp $" ;

#include "sio.h"
#include <stdio.h>
#include "clchack.h"

main()
{
	char *s ;

	Stie( 0, 1 ) ;
	Sprint( 1, "Enter 1st command --> " ) ;
	s = Srdline( 0 ) ;
	Sprint( 1, "Received command: %s\n", s ) ;
	Sprint( 1, "Enter 2nd command --> " ) ;
	s = Srdline( 0 ) ;
	Sprint( 1, "Received command: %s\n", s ) ;
	Suntie( 0 ) ;
	Sprint( 1, "Enter 3rd command --> " ) ;
	s = Srdline( 0 ) ;
	Sprint( 1, "Received command: %s\n", s ) ;
	exit( 0 ) ;
}
