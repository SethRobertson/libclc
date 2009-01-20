/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

static const char version[] = VERSION;

#include "pset.h"
#include <stdlib.h>
#include "clchack.h"

#ifndef NULL
#define NULL						0
#endif

#define ALLOC_START				20
#define ALLOC_STEP				10

#define POINTER					__pset_pointer



/*
 * Create a pointer set and return a handle to it.
 * Some space is initially allocated for the set.
 */
pset_h pset_create(unsigned int alloc_start, unsigned int alloc_step)
{
	pset_h pset ;
	unsigned start ;

	pset = (pset_h) malloc( sizeof( struct __pset ) ) ;
	if ( pset == NULL )
		return( NULL ) ;

	start = ( alloc_start == 0 ) ? ALLOC_START : alloc_start ;
	pset->ptrs = (POINTER *) malloc( start * sizeof( POINTER ) ) ;
	if ( pset->ptrs == NULL )
	{
		free( (char *) pset ) ;
		return( NULL ) ;
	}

	pset->max = start ;
	pset->count = 0 ;
	pset->alloc_step = ( alloc_step == 0 ) ? ALLOC_STEP : alloc_step ;
	return( pset ) ;
}


/*
 * Destroy a pset
 */
void pset_destroy(pset_h pset)
{
	free( (char *) pset->ptrs ) ;
	free( (char *) pset ) ;
}


/*
 * Insert a pointer to a pset
 */
POINTER pset_insert(pset_h pset, __pset_pointer p)
{
	if ( pset->count >= pset->max )
	{
		unsigned new_max = pset->max + pset->alloc_step ;
		POINTER *new_ptrs ;

		new_ptrs = (POINTER *) realloc(
								(char *)pset->ptrs, new_max * sizeof( POINTER ) ) ;
		if ( new_ptrs == NULL )
			return( NULL ) ;
		pset->max = new_max ;
		pset->ptrs = new_ptrs ;
	}
	return( pset->ptrs[ pset->count++ ] = p ) ;
}


/*
 * Remove a pointer from a pset
 */
void pset_delete(register pset_h pset, register __pset_pointer ptr)
{
	register unsigned u = pset->count ;

	if ( u == 0 )
		return ;

	do
	{
		u-- ;
		if ( pset->ptrs[ u ] == ptr )
		{
			pset->ptrs[ u ] = pset->ptrs[ --pset->count ] ;
			return ;
		}
	}
	while ( u ) ;
}


/*
 * Create a pset iterator
 */
psi_h psi_create(pset_h pset)
{
	psi_h iter = (psi_h) malloc( sizeof( struct __pset_iterator ) ) ;

	if ( iter == NULL )
		return( NULL ) ;
	iter->pset = pset ;
	return( iter ) ;
}


