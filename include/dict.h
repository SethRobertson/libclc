/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef __DICT_H
#define __DICT_H

/*
 * $Id: dict.h,v 1.1 2001/05/26 22:04:51 seth Exp $
 */

/*
 * Return values
 */
#define DICT_OK						0
#define DICT_ERR						(-1)

/*
 * Flags
 */
#define DICT_NOFLAGS				0x0
#define DICT_RETURN_ERROR			0x1
#define DICT_UNIQUE_KEYS			0x2
#define DICT_UNORDERED				0x4
#define DICT_ORDERED				0x8
#define DICT_BALANCED_TREE			0x10
#define DICT_HT_STRICT_HINTS			0x20

/*
 * Error values
 */
#define DICT_ENOERROR				0
#define DICT_ENOMEM					1
#define DICT_ENOTFOUND				2
#define DICT_ENOOOCOMP				3
#define DICT_ENOKOCOMP				4
#define DICT_ENULLOBJECT			5
#define DICT_EEXISTS					6
#define DICT_ENOHVFUNC				7
#define DICT_EBADOBJECT				8
#define DICT_EORDER					9
#define DICT_EBADORDER				10

#ifdef __ARGS
#undef __ARGS
#endif

#ifdef PROTOTYPES
#   define __ARGS( s )           s
#else
#   define __ARGS( s )           ()
#endif

typedef int (*dict_function)() ;
typedef void *dict_obj ;
typedef void *dict_key ;
typedef void *dict_h ;

enum dict_direction { DICT_FROM_START, DICT_FROM_END } ;

extern int dict_errno ;

#endif	/* __DICT_H */

