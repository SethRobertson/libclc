/*
 * (c) Copyright 1992 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef __FSMA_H
#define __FSMA_H

/*
 * $Id: fsma.h,v 1.3 2001/07/07 02:58:20 seth Exp $
 */

#define __FSMA_ALIGNMENT	8

union __fsma_chunk_header
{
  union __fsma_chunk_header *next_chunk ;
  char bytes[ __FSMA_ALIGNMENT ] ;
} ;

typedef void *__fsma_pointer ;

struct __fsma_header
{
  union __fsma_chunk_header *chunk_chain ;
  __fsma_pointer next_free ;
  __fsma_pointer temp ;
  unsigned slots_in_chunk ;
  unsigned slot_size ;
  unsigned chunk_size ;
  unsigned short flags;
  unsigned short references;
  int is_inlined ;						/* header is inlined (boolean)	*/
} ;

typedef struct __fsma_header *fsma_h ;


/*
 * Flags
 */
#define FSM_NOFLAGS				0x0
#define FSM_NOCOALESCE				0x1
#define FSM_ZERO_ALLOC				0x2
#define FSM_ZERO_FREE				0x4
#define FSM_ZERO_DESTROY			0x8


fsma_h	fsm_create	( unsigned size, unsigned slots, int flags )  ;
void	fsm_destroy	( fsma_h handle )  ;
void	*_fsm_alloc	( fsma_h handle )  ;
void	_fsm_free	( fsma_h handle, void *ptr )  ;

#define fsm_alloc( fsma ) \
     ( \
      (!(fsma)->next_free || (fsma)->flags & FSM_ZERO_ALLOC) \
      ? _fsm_alloc( fsma ) \
      : ((fsma)->temp = (fsma)->next_free, \
	 (fsma)->next_free = *(__fsma_pointer *) (fsma)->next_free, (char *) (fsma)->temp) \
      )

#define fsm_free( fsma, p ) \
     if ( (fsma)->flags & FSM_ZERO_FREE ) \
       _fsm_free( fsma, p ); \
     else \
       (fsma)->temp = (p), \
	 *(__fsma_pointer *) (fsma)->temp = (fsma)->next_free,  \
	 (fsma)->next_free = (fsma)->temp

#define fsm_size( fsma )	(fsma)->slot_size

extern int fsma_slots_per_chunk;

#endif 	/* __FSMA_H */

