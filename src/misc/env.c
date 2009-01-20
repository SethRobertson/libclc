/*
 * (c) Copyright 1992 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

UNUSED static char RCSid[] = "$Id: env.c,v 1.2 2003/06/17 05:10:52 seth Exp $" ;

#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "misc.h"
#include "env.h"
#include "clchack.h"

typedef struct __env env_s ;

#define PRIVATE					static
#define INITIAL_VARS				20
#define INCREASE					10

#ifndef NULL
#define NULL						0
#endif

#if 0
#if !defined(__linux__) && !defined(__bsdi__)	/* do we really need this?... */
/* NO! It conflicts w/ std headers. -Erez, 7/27/00 */
char *malloc(size_t) ;
char *realloc(void *, size_t) ;
#endif
#endif

int env_errno ;

PRIVATE char **lookup(env_h env, char *var, register int len);


PRIVATE env_s *alloc_env(unsigned int max_vars)
{
	env_s *ep ;
	unsigned size ;
	char **pointers ;

	ep = (env_s *) malloc( sizeof( env_s ) ) ;
	if ( ep == ENV_NULL )
	{
		env_errno = ENV_ENOMEM ;
		return( ENV_NULL ) ;
	}

	size = ( max_vars + 1 ) * sizeof( char * ) ;
	pointers = (char **) malloc( size ) ;
	if ( pointers == NULL )
	{
		free( (char *)ep ) ;
		env_errno = ENV_ENOMEM ;
		return( ENV_NULL ) ;
	}
	(void) memset( (char *)pointers, 0, (int) size ) ;

	ep->vars = pointers ;
	ep->max_vars = max_vars ;
	ep->n_vars = 0 ;
	return( ep ) ;
}


env_h env_create(env_h init_env)
{
	unsigned u ;
	env_s *ep ;
	unsigned max_vars ;

	if ( init_env == ENV_NULL )
		max_vars = INITIAL_VARS ;
	else
		max_vars = init_env->n_vars + 5 ;

	ep = alloc_env( max_vars ) ;
	if ( ep == NULL )
	{
		env_errno = ENV_ENOMEM ;
		return( ENV_NULL ) ;
	}

	if ( init_env == ENV_NULL )
		return( ep ) ;

	for ( u = 0, ep->n_vars = 0 ; u < init_env->n_vars ; u++, ep->n_vars++ )
	{
		ep->vars[ ep->n_vars ] = make_string( 1, init_env->vars[ u ] ) ;
		if ( ep->vars[ ep->n_vars ] == NULL )
		{
			env_destroy( ep ) ;
			env_errno = ENV_ENOMEM ;
			return( ENV_NULL ) ;
		}
	}
	return( ep ) ;
}


void env_destroy(env_h env)
{
	unsigned u ;

	for ( u = 0 ; u < env->n_vars ; u++ )
		free( env->vars[ u ] ) ;
	free( (char *)env->vars ) ;
	free( (char *)env ) ;
}


env_h env_make(char **env_strings)
{
	env_s *ep ;
	char **pp ;

	for ( pp = env_strings ; *pp ; pp++ ) ;

	ep = alloc_env( (unsigned) (pp-env_strings) ) ;
	if ( ep == NULL )
	{
		env_errno = ENV_ENOMEM ;
		return( ENV_NULL ) ;
	}

	for ( pp = env_strings ; *pp ; pp++ )
	{
		char *p = make_string( 1, *pp ) ;

		if ( p == NULL )
		{
			env_destroy( ep ) ;
			env_errno = ENV_ENOMEM ;
			return( ENV_NULL ) ;
		}
		ep->vars[ ep->n_vars++ ] = p ;
	}
	return( ep ) ;
}


char *env_lookup(env_h env, char *var)
{
	char **lookup() ;
	char **pp = lookup( env, var, strlen( var ) ) ;

	return( ( pp == NULL ) ? NULL : *pp ) ;
}


PRIVATE char **lookup(env_h env, char *var, register int len)
{
	register char **pp ;

	for ( pp = env->vars ; *pp ; pp++ )
		if ( strncmp( *pp, var, len ) == 0 && (*pp)[ len ] == '=' )
			return( pp ) ;
	return( NULL ) ;
}


PRIVATE int grow(env_s *ep)
{
	char **new_vars ;
	unsigned new_max_vars ;
	unsigned new_size ;

	new_max_vars = ep->max_vars + INCREASE ;
	new_size = ( new_max_vars+1 ) * sizeof( char * ) ;
	new_vars = (char **) realloc( (char *)ep->vars, new_size ) ;
	if ( new_vars == NULL )
		return( ENV_ERR ) ;

	ep->vars = new_vars ;
	ep->max_vars = new_max_vars ;
	return( ENV_OK ) ;
}


/*
 * Add the variable string to the given environment.
 */
PRIVATE int addstring(env_s *ep, char *var_string, int len)
{
	char **pp ;
	char *p ;

	p = make_string( 1, var_string ) ;
	if ( p == NULL )
		return( ENV_ERR ) ;
	
	pp = lookup( ep, var_string, len ) ;
	if ( pp == NULL )
	{
		if ( ep->n_vars >= ep->max_vars && grow( ep ) == ENV_ERR )
		{
			free( p ) ;
			env_errno = ENV_ENOMEM ;
			return( ENV_ERR ) ;
		}
		ep->vars[ ep->n_vars++ ] = p ;
	}
	else
	{
		free( *pp ) ;
		*pp = p ;
	}
	return( ENV_OK ) ;
}


int env_addvar(env_h env, env_h from_env, char *var_name)
{
	char *var_string = env_lookup( from_env, var_name ) ;

	if ( var_string == NULL )
	{
		env_errno = ENV_EBADVAR ;
		return( ENV_ERR ) ;
	}

	return( addstring( env, var_string, strlen( var_name ) ) ) ;
}


int env_addstr(env_h env, char *var_string)
{
	char *p = strchr( var_string, '=' ) ;

	if ( p == NULL )
	{
		env_errno = ENV_EBADSTRING ;
		return( ENV_ERR ) ;
	}

	return( addstring( env, var_string, p-var_string ) ) ;
}


int env_remvar(env_h env, char *var)
{
	char **pp = lookup( env, var, strlen( var ) ) ;

	if ( pp == NULL )
	{
		env_errno = ENV_EBADVAR ;
		return( ENV_ERR ) ;
	}

	free( *pp ) ;
	*pp = env->vars[ --env->n_vars ] ;
	return( ENV_OK ) ;
}


#ifdef notdef
PRIVATE int comparator( p1, p2 )
	char **p1, **p2 ;
{
	return( strcmp( *p1, *p2 ) ) ;
}


void env_sort( env )
	env_h env ;
{
	qsort( (char *)env->vars, env->n_vars, sizeof( char * ), comparator ) ;
}
#endif
