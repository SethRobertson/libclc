/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

static char RCSid[] = "$Id: dict.c,v 1.1 2001/05/26 22:04:49 seth Exp $" ;
static char version[] = VERSION ;

#include <unistd.h>
#include <string.h>
#include "dictimpl.h"
#include "clchack.h"

int dict_errno ;

struct name_value
{
	int nv_value ;
	char *nv_name ;
} ;


static struct name_value error_codes[] =
	{
		{	DICT_ENOMEM,			"out of memory"											},
		{	DICT_ENOTFOUND,		"object not found"										},
		{	DICT_ENOOOCOMP,		"object-object comparison function is missing"	},
		{	DICT_ENOKOCOMP,		"key-object comparison function is missing"		},
		{	DICT_ENULLOBJECT,		"object is NULL"											},
		{	DICT_EEXISTS,			"object exists"											},
		{	DICT_ENOHVFUNC,		"hashvalue extraction function is missing"		},
		{	DICT_EBADOBJECT,		"object does not exist"									},
		{	DICT_EBADORDER,		"bad order flag"											},
		{	DICT_EORDER,			"specified order not supported"						},
		{	DICT_ENOERROR,			NULL															}
	} ;


void __dict_terminate(char *prefix, char *msg)
{
	static char buf[ 80 ] ;
#ifndef __linux__				/* necessary kludge */
	char *strcat(char *, const char *) ;
	char *strcpy(char *, const char *) ;
	void abort(void) ;
#endif

	(void) strcpy( buf, "DICT " ) ;
	(void) strcat( buf, prefix ) ;
	(void) strcat( buf, ": " ) ;
	(void) strcat( buf, msg ) ;
	(void) strcat( buf, "\n" ) ;
	(void) write( 2, buf, strlen( buf ) ) ;
	abort() ;
	_exit( 1 ) ;
	/* NOTREACHED */
}


void __dict_fatal_error(char *caller, int error_code)
{
	struct name_value *nvp ;
	char *msg ;

	/*
	 * Lookup error message
	 */
	msg = "unknown error" ;
	for ( nvp = error_codes ; nvp->nv_name ; nvp++ )
		if ( nvp->nv_value == error_code )
		{
			msg = nvp->nv_name ;
			break ;
		}
	__dict_terminate( caller, msg ) ;
}


dict_h __dict_create_error(char *caller, int flags, int *errp, int error_code)
{
	dheader_s dh ;

	dh.flags = flags ;
	dh.errnop = ( errp == INT_NULL ) ? &dict_errno : errp ;
	HANDLE_ERROR( &dh, caller, error_code, NULL_HANDLE ) ;
	/* NOTREACHED */
	return NULL;
}


int __dict_args_ok(char *caller, int flags, int *errp, dict_function oo_comp, dict_function ko_comp, int allowed_orders)
{
	int requested_order ;

	if ( BAD_ORDER( flags ) )
	{
		(void) __dict_create_error( caller, flags, errp, DICT_EBADORDER ) ;
		return( FALSE ) ;
	}

	/*
	 * If the user provided an object-object comparator, we can pretend
	 * that the library supports the DICT_UNORDERED flag.
	 */
	if ( oo_comp )
		allowed_orders |= DICT_UNORDERED ;

	requested_order = ( flags & ORDER_FLAGS ) ;
	if ( requested_order && ! ( allowed_orders & requested_order ) )
	{
		(void) __dict_create_error( caller, flags, errp, DICT_EORDER ) ;
		return( FALSE ) ;
	}

	/*
	 * An object-object comparator is required if
	 *		order is requested,
	 * or
	 *		key uniqueness is requested
	 * or
	 *		the library requires it.
	 */
	if ( oo_comp == NULL_FUNC && ( flags & (DICT_ORDERED | DICT_UNIQUE_KEYS) ) )
	{
		(void) __dict_create_error( caller, flags, errp, DICT_ENOOOCOMP ) ;
		return( FALSE ) ;
	}

#ifdef notdef
	if ( ko_comp == NULL )
	{
		(void) __dict_create_error( caller, flags, errp, DICT_ENOKOCOMP ) ;
		return( FALSE ) ;
	}
#endif

	return( TRUE ) ;
}


void __dict_init_header(dheader_s *dhp, dict_function oo_comp, dict_function ko_comp, int flags, int *errnop)
{
	dhp->oo_comp = oo_comp ;
	dhp->ko_comp = ko_comp ;
	dhp->flags = flags ;
	dhp->errnop = ( errnop == INT_NULL ) ? &dict_errno : errnop ;
}

