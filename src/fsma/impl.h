/*
 * (c) Copyright 1992 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

/*
 * $Id: impl.h,v 1.1 2001/05/26 22:04:49 seth Exp $
 */

#ifndef NULL
#define NULL 0
#endif

#ifndef FALSE
#define FALSE     0
#define TRUE      1
#endif

#define PRIVATE                     static

#define SLOTS_PER_CHUNK             100

#define POINTER							__fsma_pointer
#define MINSIZE                     sizeof( POINTER )

#define CHUNK_HEADER( p )           ((union __fsma_chunk_header *)(p))

