/*
 * (c) Copyright 1992 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

static char RCSid[] = "$Id: fsma.c,v 1.6 2001/11/29 17:39:56 jtt Exp $" ;
static char *version = VERSION ;

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fsma.h"
#include "impl.h"
#ifdef DEBUG
#include <stdio.h>
#endif
#include "clchack.h"



/*
 * Normally when using insight et al we want to debug
 * users of memory, not fsma itself.
 */
#ifndef DEBUG_FSMA_ITSELF
#if defined (__INSIGHT__) || defined (__INSURE__) || defined(USING_DMALLOC) || defined(USING_INSIGHT)
#	ifdef COALESCE
#		undef COALESCE
#	endif /* COALESCE */
#	define FSMA_USE_MALLOC
#endif /* memory debuggers */
#endif /* DEBUG_FSMA_ITSELF */


unsigned int fsma_slots_per_chunk = SLOTS_PER_CHUNK;
#ifdef COALESCE
int fsma_coalesce = TRUE;
static fsma_h *coalesce = NULL;
static int coalesce_size = 1;
#endif /* COALESCE */

PRIVATE void init_free_list  ( unsigned, unsigned, void * )  ;



/*
 * Allocator information
 *
 * Alignment:  in order for your data to be aligned properly
 * (e.g. doubles must be 8 byte aligned)
 *  Malloc must return memory aligned to worst case data type
 *  Requirements no worse than __FSMA_ALIGNMENT
 *  Chunk size must be aligned to worst case data type
 *	(fsma ensures it is aligned to sizeof(void *))
 *
 * An allocator manages a linked list of chunks:
 *  _____________         ______________         _____________
 * |   HEADER    |------>|              |------>|             |
 * |_____________|       |______________|       |_____________|
 * |             |       |              |       |             |
 * |             |       |              |       |             |
 * |             |       |              |       |             |
 * |   INITIAL   |       |              |       |             |
 * |    SLOTS    |       |              |       |             |
 * |             |       |              |       |             |
 * |             |       |              |       |             |
 * |             |       |              |       |             |
 * |             |       |              |       |             |
 * |_____________|       |______________|       |_____________|
 *
 * A slot is a chunk of memory aligned as specified above of at
 * least object_size bytes of memory--allocation requests will
 * return this chunk of memory.
 *
 * The allocator creates chunks as necessary and places all of the
 * newly retrieved slots onto the free list as a singly linked list
 *
 * The chunks are not otherwise used until the allocator is destroyed
 * when each chunk is freed.
 */




#ifndef FSMA_USE_MALLOC



/*
 * Create a memory allocator
 */
fsma_h fsm_create(unsigned int object_size, unsigned int slots_per_chunk, int flags)
{
  register fsma_h		fp ;
  union __fsma_chunk_header	*chp ;
  void				*slots ;
  unsigned			nslots ;
  unsigned			chunk_size ;
  unsigned			slot_size ;
  int				header_inlined ;
  unsigned			tmp1;

  nslots = ( slots_per_chunk == 0 ) ? fsma_slots_per_chunk : slots_per_chunk ;
  slot_size = ( object_size < MINSIZE ) ? MINSIZE : object_size ;

  /*
   * Make sure that the slot_size is a multiple of the pointer size
   *
   * XXX - should this be a multiple of __FSMA_ALIGNMENT?
   */
  if ( slot_size % sizeof( __fsma_pointer ) != 0 )
    slot_size += sizeof( __fsma_pointer ) -
      slot_size % sizeof( __fsma_pointer ) ;

  chunk_size = sizeof( union __fsma_chunk_header ) + nslots * slot_size ;


#ifdef COALESCE
  /*
   * See if another allocator already in use might be suitable
   * (same size, flags, etc)
   */
  fp = NULL;
  if (fsma_coalesce && !(flags & FSM_NOCOALESCE))
  {
    fsma_h *fpp;
    for (fpp = coalesce;fpp && *fpp;fpp++)
    {
      if ((*fpp)->slot_size == slot_size &&
	  flags == (*fpp)->flags)
	break;
    }
    if (fpp && *fpp)
    {
      (*fpp)->references++;
      return(*fpp);
    }
  }
#endif /* COALESCE */

  /*
   * Adjust memory allocation to prevent wasting space due to most
   * malloc allocation of power-of-two memory sizes.  Of course,
   * many use a bit of memory in addition, so it really should
   * be power of two minus malloc overhead, but lets not go
   * overboard especially since we cannot find the right offset.
   */
  for(tmp1=16;tmp1<chunk_size;tmp1*=2) ;
  tmp1 -= sizeof( union __fsma_chunk_header );
  nslots = tmp1/slot_size;
  chunk_size = sizeof( union __fsma_chunk_header ) + nslots * slot_size ;

  chp = CHUNK_HEADER( malloc( chunk_size ) ) ;
#ifdef DEBUG
  fprintf(stderr,"Mallocing new chunk at FSM creation time: %d\n", chunk_size);
#endif /* DEBUG */
  if ( chp == NULL )
  {
    return( NULL ) ;
  }

  /* Set up free list starting after chunk list pointer */
  chp->next_chunk = NULL ;
  slots = (void *) &chp[ 1 ] ;
  init_free_list( nslots, slot_size, slots ) ;

#ifdef DEBUG
  fprintf( stderr, "Size = %d, nslots = %d\n", slot_size, nslots );
  for ( tmp1 = 0 ; tmp1 < nslots ; tmp1++ )
    fprintf( stderr, "slot[ %d ] = %p\n", tmp1, (*(char **) (slots + tmp1 * slot_size )));
#endif /* DEBUG */


  /*
   * Check if we can fit the header in an object slot
   */
  if ( slot_size >= sizeof( struct __fsma_header ) )
  {
    /*
     * We can do it.
     * Allocate the first slot
     */
    fp = (fsma_h) slots ;
    slots = *(POINTER *) slots ;		/* same as slots++ */
    header_inlined = TRUE ;
  }
  else
  {
    fp = (fsma_h) malloc( sizeof( struct __fsma_header ) ) ;
#ifdef DEBUG
    fprintf(stderr,"Mallocing a new fsma header: %d\n", 
	    sizeof(struct __fsma_header));
#endif /* DEBUG */
    if ( fp == NULL )
    {
      free( chp ) ;
      return( NULL ) ;
    }
    header_inlined = FALSE ;
  }


  fp->next_free = (POINTER) slots ;
  fp->chunk_chain = chp ;
  fp->slots_in_chunk = nslots ;
  fp->slot_size = slot_size ;
  fp->chunk_size = chunk_size ;
  fp->flags = flags ;
  fp->is_inlined = header_inlined ;
#ifdef COALESCE
  fp->references = 1;
#endif /* COALESCE */
	
#ifdef DEBUG
  fprintf( stderr, "Slots/chunk = %d\n", nslots ) ;
  fprintf( stderr, "Allocating chunk %p\n", chp ) ;
#endif

#ifdef COALESCE
  if (fsma_coalesce && !(flags & FSM_NOCOALESCE))
  {
    /* Add memory to hold this fsma size/flags allocator */
    if (!coalesce)
      coalesce = (fsma_h *)malloc(sizeof(fsma_h)*2);
    else
      coalesce = (fsma_h *)realloc(coalesce,(coalesce_size+1)*sizeof(fsma_h));

    if (!coalesce)
    {
      /* Something drastic, but what? */
    }
    else
    {
      coalesce[coalesce_size-1] = fp;
      coalesce[coalesce_size++] = NULL;
    }
  }
#endif /* COALESCE */

      return( (fsma_h) fp ) ;
      }



  /*
 * Done with allocator--if everyone done, destroy it.
 */
  void fsm_destroy(register fsma_h fp)
    {
      int header_inlined = fp->is_inlined ;
      register union __fsma_chunk_header *chp, *next_chunk ;
      register int zero_memory = fp->flags & FSM_ZERO_DESTROY ;
      register int chunk_size = fp->chunk_size ;

#ifdef COALESCE
      if (fp->references > 1)
      {
	fp->references--;
	return;
      }

      if (fsma_coalesce && !(fp->flags & FSM_NOCOALESCE))
      {
	fsma_h *fpp;
	int cntr = 0;

	/* Remove this allocator from coalesce list */
	for(fpp = coalesce;fpp && *fpp && *fpp != fp;cntr++,fpp++) ;
	if (fpp && *fpp == fp)
	{
	  *fpp = coalesce[(--coalesce_size)-1];
	  coalesce[coalesce_size-1] = NULL;
	  coalesce = (fsma_h *)realloc(coalesce, coalesce_size * sizeof(fsma_h));
	  if (!coalesce)
	  {
	    /* XXX - something drastic, but what? How can this fail anyway? */
	  }
	}
      }
#endif /* COALESCE */

      /*
       * Free all chunks in the chunk chain
       */
      for ( chp = fp->chunk_chain ; chp != NULL ; chp = next_chunk )
      {
	next_chunk = chp->next_chunk ;
	if ( zero_memory )
	  (void) memset( chp, 0, chunk_size ) ;

#ifdef DEBUG
	fprintf( stderr, "Freeing chunk %p\n", chp ) ;
#endif
	free( chp ) ;
      }

      /*
       * If fp->inlined is NO, we have to free the handle.
       * Note that we copied fp->inlined in case it is YES.
       */
      if ( ! header_inlined )
	free( fp ) ;
    }



  /*
 * Slow case allocator -- handle memory clears, new
 * allocations, etc
 */
  void *_fsm_alloc(register fsma_h fp)
    {
      register POINTER object ;

      /*
       * Check if there are any slots on the free list
       */
      if ( fp->next_free == NULL )
      {
	/*
	 * Free list exhausted; allocate a new chunk
	 */
	void *slots ;
	union __fsma_chunk_header *chp ;

	chp = CHUNK_HEADER( malloc( fp->chunk_size ) ) ;
#ifdef DEBUG
	fprintf(stderr,"Mallocing a new chunk: %d\n", fp->chunk_size);
#endif /* DEBUG */
	if ( chp == NULL )
	{
	  return( NULL ) ;
	}

#ifdef DEBUG
	fprintf( stderr, "Allocating chunk %p\n", chp ) ;
#endif
	/*
	 * Put the slots in this chunk in a linked list
	 * and add this list to the free list
	 */
	slots = (void *) &chp[ 1 ] ;
	init_free_list( fp->slots_in_chunk, fp->slot_size, slots ) ;
	fp->next_free = (POINTER) slots ;

	/*
	 * Link this chunk in at the head of the chunk chain
	 */
	chp->next_chunk = fp->chunk_chain ;
	fp->chunk_chain = chp ;
      }

      object = fp->next_free ;
      fp->next_free = *(POINTER *)object ;

      if ( fp->flags & FSM_ZERO_ALLOC )
	(void) memset( object, 0, fp->slot_size ) ;

      return( object ) ;
    }



  /*
 * Done with slot slow case
 */
  void _fsm_free(fsma_h fp, void *object)
    {
      if ( fp->flags & FSM_ZERO_FREE )
	(void) memset( object, 0, fp->slot_size ) ;

      *(POINTER *)object = fp->next_free ;
      fp->next_free = object ;
    }



  /*
 * Ensure the free list is nice and initialized and that each element
 * points to the next.
 */
  PRIVATE void init_free_list(unsigned int nslots, register unsigned int size, void *slots)
    {
      register unsigned int i ;
      register void *next ;
      register POINTER current ;

      for ( i = 0, current = slots, next = (char *)slots + size ; i < nslots - 1 ;
	    i++, current = next, next = (char *)next + size )
	*(POINTER *)current = next ;
      *(POINTER *)current = NULL ;
    }



#else /* !FSMA_USE_MALLOC */



  /*
 * Trivial memory allocator--wrapper around malloc
 */
  fsma_h fsm_create(unsigned int object_size, unsigned int slots_per_chunk, int flags)
    {
      register fsma_h		fp ;
      unsigned			slot_size ;

      fp = (fsma_h) malloc( sizeof( struct __fsma_header ) ) ;
      if ( fp == NULL )
      {
	return( NULL ) ;
      }

      /* force fsm_alloc macro to invoke _fsm_alloc function */
      fp->next_free = NULL ;

      /* FSM_ZERO_FREE forces fsm_free macro to invoke _fsm_free function */
      fp->flags = flags | FSM_ZERO_FREE;

      /*
       * Make sure that the slot_size is a multiple of the pointer size
       *
       * XXX - should this be a multiple of __FSMA_ALIGNMENT?
       */
      slot_size = ( object_size < MINSIZE ) ? MINSIZE : object_size ;
      if ( slot_size % sizeof( __fsma_pointer ) != 0 )
	slot_size += sizeof( __fsma_pointer ) -
	  slot_size % sizeof( __fsma_pointer ) ;

      /* just in case some caller gets snoopy about these */
      fp->slots_in_chunk = 1 ;
      fp->slot_size = slot_size ;
      fp->chunk_size = slot_size ;
#ifdef COALESCE
      fp->references = 1;
#endif /* COALESCE */
      fp->is_inlined = FALSE ;
	
      return( (fsma_h) fp ) ;
    }



  /*
 * Trivial memory allocator destruction function
 */
  void fsm_destroy(register fsma_h fp)
    {
      /* may leak memory if people are lazy and use destroy to free each elem */
      free( fp ) ;
    }



  /*
 * Trivial memory allocator - allocate memory via malloc
 */
  void *_fsm_alloc(register fsma_h fp)
    {
      register POINTER object ;

      object = malloc( fp->slot_size ) ;
      if ( object == NULL )
      {
	return( NULL ) ;
      }

      if ( fp->flags & FSM_ZERO_ALLOC )
	(void) memset( object, 0, fp->slot_size ) ;

      return( object ) ;
    }



  /*
 * Trivial memory allocator -- free memory
 */
  void _fsm_free(fsma_h fp, void *object)
    {
#ifdef REALLY_WANT_FREE_ZERO
      if ( fp->flags & FSM_ZERO_FREE )
	(void) memset( object, 0, fp->slot_size ) ;
#endif /* REALLY_WANT_FREE_ZERO */

      free ( object ) ;
    }
#endif /* FSMA_USE_MALLOC */
