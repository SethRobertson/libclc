/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */


#include "pset.h"
#include "clchack.h"

#define PRIVATE				static

#ifndef NULL
#define NULL					0
#endif


/*
 * Remove all NULL pointers from a pset
 */
void pset_compact(register pset_h pset)
{
	register unsigned u ;

	for ( u = 0 ; u < pset_count( pset ) ; )
		if ( pset_pointer( pset, u ) != NULL )
			u++ ;
		else
			pset_remove_index( pset, u ) ;
}


/*
 * Apply a function to all pointers of a pset
 */
void pset_apply(register pset_h pset, register void (*func) (/* ??? */), register __pset_pointer arg)
{
	register unsigned u ;

	for ( u = 0 ; u < pset_count( pset ) ; u++ )
		if ( arg )
			(*func)( arg, pset_pointer( pset, u ) ) ;
		else
			(*func)( pset_pointer( pset, u ) ) ;
}

