/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

/*
 * $Id: impl.h,v 1.2 2003/06/17 05:10:54 seth Exp $
 */

#ifndef SIO_BUFFER_SIZE

#include "sioconf.h"

#ifdef HAS_MMAP
#include <sys/types.h>


/*
 * A struct map_unit describes a memory mapped area of a file.
 *
 * addr is the address where the file is mapped. If addr is NULL
 * the other fields are meaningless.
 * valid_bytes indicates how many bytes are _valid_ in that unit
 * mapped_bytes of a unit is how many bytes are mapped in that
 * unit ( valid <= mapped ).
 * Normally mapped_bytes will be equal to valid_bytes until
 * we reach the end of the file. Then if the file size is not a multiple
 * of the unit size, only the rest of the file will be mapped at the
 * unit, leaving part of what was mapped at the unit before still
 * visible (this happens because I chose not to unmap a unit before
 * remapping it). In that case valid_bytes shows the size of the "new"
 * mapping and mapped_bytes shows how many bytes are really mapped.
 * mapped_bytes is used in Sdone() to unmap the units.
 */
struct map_unit
{
	caddr_t addr ;
	size_t valid_bytes ;
	size_t mapped_bytes ;
} ;


/*
 * Meaning of fields used when memory mapping:
 *
 *    file_offset:      it is the offset in the file where the next
 *                      mapping should be done
 *
 *    file_size:        size of the file (obtained from stat(2))
 */
struct mmap_descriptor
{
   off_t file_offset ;
   off_t file_size ;
	struct map_unit first_unit ;
	struct map_unit second_unit ;
} ;

typedef struct mmap_descriptor mapd_s ;

#endif /* HAS_MMAP */

typedef enum { FAILURE, SUCCESS } status_e ;

/*
 * Descriptor management: convert a descriptor pointer to an input or
 * output descriptor pointer
 */
#define IDP( dp )						(&(dp)->descriptor.input_descriptor)
#define ODP( dp )						(&(dp)->descriptor.output_descriptor)

#define DESCRIPTOR_INITIALIZED( dp )	((dp)->initialized)

/*
 * Internal constants
 */
#define SIO_BUFFER_SIZE       	8192
#define SIO_NO_TIED_FD				(-1)

typedef enum { NO = 0, YES = 1 } boolean_e ;

#ifndef FALSE
#define FALSE			0
#define TRUE			1
#endif

#ifndef NULL
#define NULL			0
#endif

#ifdef MIN
#undef MIN
#endif
#define MIN( a, b )					( (a) < (b) ? (a) : (b) )

#define NUL								'\0'

#ifdef PRIVATE
#undef PRIVATE
#endif
#define PRIVATE						static

#ifdef DEBUG

static char *itoa( num )
	unsigned num ;
{
#define NUMBUF_SIZE		15
	static char numbuf[ NUMBUF_SIZE ] ;
	register char *p = &numbuf[ NUMBUF_SIZE ] ;

	*--p = '\0' ;
	do
	{
		*--p = num % 10 + '0' ;
		num /= 10 ;
	}
	while ( num ) ;
	return( p ) ;
}

#	define ASSERT( expr )														\
		if ( ! (expr) )															\
		{																				\
			char *s1 = "SIO assertion << expr >> failed: File: " ;	\
			char *s2 = __FILE__ ;												\
			char *s3 = ", line: " ;												\
			char *s4 = itoa( __LINE__ ) ;										\
			char *s5 = "\n" ;														\
			(void) write( 2, s1, strlen( s1 ) ) ;							\
			(void) write( 2, s2, strlen( s2 ) ) ;							\
			(void) write( 2, s3, strlen( s3 ) ) ;							\
			(void) write( 2, s4, strlen( s4 ) ) ;							\
			(void) write( 2, s5, strlen( s5 ) ) ;							\
			exit ( 1 ) ;															\
		}
#else
#	define ASSERT( expr )
#endif


#include <errno.h>
extern int errno ;

/*
 * IO_SETUP initializes a descriptor if it is not already initialized.
 * It checks if the stream is of the right type (input or output).
 * CONTROL_SETUP checks if the descriptor is initialized and if the
 * stream is of the right type (input or output).
 *
 * 	fd: file descriptor
 * 	dp: descriptor pointer
 * 	op: operation
 * 	ev: error value (if __sio_init fails; __sio_init should set errno)
 *
 * IO_SETUP will call __sio_init if the descriptor is not initialized.
 * Possible errors:
 *		1. Using CONTROL_SETUP on an uninitialized descriptor.
 *		2. The operation is not appropriate for the descriptor (e.g.
 *			a read operation on an descriptor used for writing).
 * Both errors set errno to EBADF.
 */
#define CONTROL_SETUP( dp, type, ev )														\
			{																							\
				if ( ! DESCRIPTOR_INITIALIZED( dp ) || dp->stream_type != type )	\
				{																						\
					errno = EBADF ;																\
					return( ev ) ;																	\
				}																						\
			}


#define IO_SETUP( fd, dp, type, ev )														\
			{																							\
				if ( DESCRIPTOR_INITIALIZED( dp ) ) 										\
				{																						\
					if ( dp->stream_type != type )											\
					{																					\
						errno = EBADF ;															\
						return( ev ) ;																\
					}																					\
				}																						\
				else if ( __sio_init( dp, fd, type ) == SIO_ERR )						\
					return( ev ) ;																	\
			}


/*
 * Internal functions that are visible
 */
int __sio_readf(register __sio_id_t *idp, int fd);
int __sio_writef(register __sio_od_t *odp, int fd);
int __sio_extend_buffer(register __sio_id_t *idp, int fd, register int b_left);
int __sio_init(register __sio_descriptor_t *dp, int fd, enum __sio_stream stream_type);
int __sio_converter(register __sio_od_t *odp, int fd, register char *fmt, va_list ap);
int __sio_more(register __sio_id_t *idp, int fd);
int __sio_extend_buffer(register __sio_id_t *idp, int fd, register int b_left);
status_e __sio_switch(register __sio_id_t *idp, int fd) ;


#ifdef HAS_MEMOPS
#include <memory.h>
#define sio_memcopy( from, to, nbytes )   	(void) memcpy( to, from, nbytes )
#define sio_memscan( from, nbytes, ch )      memchr( from, ch, nbytes )
#endif

#ifdef HAS_BCOPY
#define sio_memcopy( from, to, nbytes )      (void) bcopy( from, to, nbytes )
#endif

#ifndef sio_memcopy
#define sio_memcopy		__sio_memcopy
#define NEED_MEMCOPY
void __sio_memcopy() ;
#endif

#ifndef sio_memscan
char *sio_memscan() ;
#endif

#endif /* SIO_BUFFER_SIZE */

