/*
 * (c) Copyright 1992 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef __ENV_H
#define __ENV_H

/*
 * $Id: env.h,v 1.1 2001/05/26 22:04:51 seth Exp $
 */

struct __env
{
	unsigned max_vars ;
	unsigned n_vars ;
	char **vars ;
} ;

typedef struct __env *env_h ;

#define ENV_NULL						((env_h)0)

/*
 * Return values
 */
#define ENV_ERR						(-1)
#define ENV_OK							0

/*
 * Error codes
 */
#define ENV_ENOMEM					1
#define ENV_EBADVAR					2
#define ENV_EBADSTRING				3


#ifdef __ARGS
#undef __ARGS
#endif

#ifdef PROTOTYPES
#  define __ARGS( s )               s
#else
#  define __ARGS( s )               ()
#endif

env_h env_create __ARGS( ( env_h ) ) ;
void env_destroy __ARGS( ( env_h ) ) ;
env_h env_make __ARGS( ( char **env_strings ) ) ;
int env_addvar __ARGS( ( env_h, env_h from_env, char *var ) ) ;
int env_addstr __ARGS( ( env_h, char *str ) ) ;
int env_remvar __ARGS( ( env_h, char *var ) ) ;
char *env_lookup __ARGS( ( env_h, char *var ) ) ;

#define env_getvars( env )				(env)->vars

extern int env_errno ;

#endif	/* __ENV_H */

