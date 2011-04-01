/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */


#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sio.h"
#include "impl.h"

#ifdef EVENTS
#include "events.h"
#endif
#include "clchack.h"

#ifndef _WIN32
#ifdef NEED_PROTOTYPES
char *malloc(size_t) ;
char *realloc(void *, size_t) ;
int madvise(caddr_t addr, int len, int behav) ;
#else
#include <stdlib.h>
#endif
#else  /* WIN32 */
# include <malloc.h>
# include <string.h>
# ifndef __CYGWIN__
#  include <sys/fcntl.h>
#  include <sosnt.h>
# endif /* !__CYGWIN__ */
#endif /* _WIN32 */

static __sio_descriptor_t static_descriptor_array[ N_SIO_DESCRIPTORS ] ;
static int n_descriptors = N_SIO_DESCRIPTORS ;
__sio_descriptor_t *__sio_descriptors = static_descriptor_array ;

#ifdef EVENTS
static events_s static___sio_events[ N_SIO_DESCRIPTORS ] ;
events_s *__sio_events = static___sio_events ;
#endif

PRIVATE void terminate(char *);
PRIVATE status_e setup_read_buffer(register __sio_id_t *, unsigned);

/*
 * Code for finalization
 */
#ifdef HAS_FINALIZATION_FUNCTION
static int finalizer_installed ;

SIO_DEFINE_FIN( sio_cleanup )
{
   (void) Sflush( SIO_FLUSH_ALL ) ;
}
#endif /* HAS_FINALIZATION_FUNCTION */



#ifdef HAS_MMAP

#define CHAR_NULL				((char *)0)

/*
 * PAGES_MAPPED gives the size of each map unit in pages
 */
#define PAGES_MAPPED				2

static size_t map_unit_size = 0 ;			/* bytes */
static size_t page_size = 0 ;					/* bytes */

static mapd_s static_mapd_array[ N_SIO_DESCRIPTORS ] ;
static mapd_s *mmap_descriptors = static_mapd_array ;

#define MDP( fd )				( mmap_descriptors + (fd) )


/*
 * NOTES ON MEMORY MAPPING:
 *
 * 	1. Memory mapping works only for file descriptors opened for input
 *		2. Mapping an object to a part of the address space where another
 *			object is mapped will cause the old mapping to disappear (i.e. mmap
 *			will not fail)
 *
 * Memory mapping interface:
 *		SIO_MMAP : maps a file into a portion of the address space.
 *		SIO_MUNMAP: unmap a portion of the address space
 *		SIO_MNEED: indicate to the OS that we will need a portion of
 *						 our address space.
 *
 * The map_unit_size variable defines how much of the file is mapped at
 * a time. It is a multiple of the operating system page size. It is
 * not less than SIO_BUFFER_SIZE unless SIO_BUFFER_SIZE is not a
 * multiple of the page size (so the SIO_BUFFER_SIZE overrides
 * PAGES_MAPPED).
 *
 * NOTE: All memory mapping code is in this file only
 */


/*
 * Macros used by the memory mapping code
 */
#define FIRST_TIME( dp )					( dp->buf == NULL )
#define FATAL_ERROR( msg )					perror( msg ), exit( 1 )

/*
 * Functions to support memory mapping:
 *
 *			try_memory_mapping
 *			buffer_setup
 *			__sio_switch
 *			initial_map
 *			map_unit
 */

/*
 * try_memory_mapping attempts to setup the specified descriptor
 * for memory mapping.
 * It returns FAILURE if it fails and SUCCESS if it is successful.
 * If HAS_MMAP is not defined, the function is defined to be FAILURE.
 *
 * Sets fields:
 *		memory_mapped:			 TRUE or FALSE
 *
 * Also sets the following fields if memory_mapped is TRUE:
 *    file_offset, file_size, buffer_size
 *
 */
PRIVATE status_e try_memory_mapping(int fd, register __sio_id_t *idp, struct stat *stp)
{
	int access ;

#ifdef EVENTS
	EVENT( fd, EV_TRY_MEMORY_MAPPING ) ;
#endif

	/*
	 * Do not try memory mapping if:
	 *		1) The file is not a regular file
	 *		2) The file is a regular file but has zero-length
	 *		3) The file pointer is not positioned at the beginning of the file
	 *		4) The fcntl to obtain the file descriptor flags fails
	 *		5) The access mode is not O_RDONLY or O_RDWR
	 *
	 * The operations are done in this order to avoid the system calls
	 * if possible.
	 */
	if ( ( ( stp->st_mode & S_IFMT ) != S_IFREG ) ||
		  ( stp->st_size == 0 ) ||
		  ( lseek( fd, (long)0, 1 ) != 0 ) ||
		  ( ( access = fcntl( fd, F_GETFL, 0 ) ) == -1 ) ||
		  ( ( access &= 0x3 ) != O_RDONLY && access != O_RDWR ) )
	{
		idp->memory_mapped = FALSE ;
		return( FAILURE ) ;
	}

	/*
	 * Determine page_size and map_unit_size.
	 * Note that the code works even if PAGES_MAPPED is 0.
	 */
	if ( page_size == 0 )
	{
		page_size = getpagesize() ;
		map_unit_size = page_size * PAGES_MAPPED ;
		if ( map_unit_size < SIO_BUFFER_SIZE )
		{
			if ( map_unit_size > 0 && SIO_BUFFER_SIZE % map_unit_size == 0 )
				map_unit_size = SIO_BUFFER_SIZE ;
			else
				map_unit_size = page_size ;
		}
	}

	MDP(fd)->file_offset = 0 ;
	MDP(fd)->file_size = stp->st_size ;
	idp->buffer_size = map_unit_size ;
	idp->buf = CHAR_NULL ;
	idp->memory_mapped = TRUE ;

	return( SUCCESS ) ;
}


/*
 * Copy the current_unit to the primary buffer
 *
 * Sets fields: start, end, nextb
 * Also sets the file pointer
 */
PRIVATE void buffer_setup( idp, fd, mu_cur, mu_next )
	__sio_id_t *idp ;
	int fd ;
	struct map_unit *mu_cur ;
	struct map_unit *mu_next ;
{
	off_t new_offset ;

	sio_memcopy( mu_cur->addr, idp->buf, mu_cur->valid_bytes ) ;
	idp->start = idp->buf ;
	idp->end = idp->buf + mu_cur->valid_bytes ;
	idp->nextb = idp->buf + ( idp->nextb - mu_cur->addr ) ;

	if ( mu_next->addr != CHAR_NULL )
		new_offset = MDP(fd)->file_offset - mu_next->valid_bytes ;
	else
		new_offset = MDP(fd)->file_offset ;
	(void) lseek( fd, new_offset, 0 ) ;
}


/*
 * Switch from memory mapping to buffered I/O
 * If any mapping has occured, then the current unit is
 * copied into the buffer that is allocated.
 * Any data in the next unit is ignored.
 * We rely on idp->buf to identify the current unit (so it
 * better be equal to the address of one of the units).
 *
 * Sets fields:
 *			start, end, nextb
 */
status_e __sio_switch(register __sio_id_t *idp, int fd)
{
	register mapd_s *mdp = MDP( fd ) ;
	struct map_unit *mu_cur, *mu_next ;
	unsigned buffer_size = idp->buffer_size ;
	char *buf_addr = idp->buf ;
	int first_time = FIRST_TIME( idp ) ;
	void buffer_setup() ;
	status_e setup_read_buffer() ;

#ifdef EVENTS
	EVENT( fd, EV_SIO_SWITCH ) ;
#endif

	/*
	 * Initialize stream for buffering
	 */
	if ( setup_read_buffer( idp, buffer_size ) == FAILURE )
		return( FAILURE ) ;

	if ( ! first_time )
	{
		/*
		 * Find current, next unit
		 */
		if ( buf_addr == mdp->first_unit.addr )
		{
			mu_cur = &mdp->first_unit ;
			mu_next = &mdp->second_unit ;
		}
		else
		{
			mu_cur = &mdp->second_unit ;
			mu_next = &mdp->first_unit ;
		}

		buffer_setup( idp, fd, mu_cur, mu_next ) ;
		/*
		 * Destroy all mappings
		 */
		(void) SIO_MUNMAP( mu_cur->addr, mu_cur->mapped_bytes ) ;
		if ( mu_next->addr != NULL )
			(void) SIO_MUNMAP( mu_next->addr, mu_next->mapped_bytes ) ;
	}
	else
		idp->start = idp->end = idp->nextb = idp->buf ;

	idp->memory_mapped = FALSE ;
	return( SUCCESS ) ;
}


/*
 * initial_map does the first memory map on the file descriptor.
 * It attempts to map both units.
 * The mapping always starts at file offset 0.
 *
 * SETS FIELDS:
 *			first_unit.*, second_unit.*
 *			file_offset
 *
 * Returns:
 *			number of bytes mapped in first_unit
 *    or
 *			0 to indicate that mmap failed.
 */
PRIVATE int initial_map(register mapd_s *mdp, int fd)
{
	register caddr_t addr ;
	register size_t requested_length = 2 * map_unit_size ;
	register size_t mapped_length = MIN( mdp->file_size, requested_length ) ;
	size_t bytes_left ;
	register size_t bytes_in_unit ;

#ifdef EVENTS
	EVENT( fd, EV_INITIAL_MAP ) ;
#endif

	addr = SIO_MMAP( CHAR_NULL, mapped_length, fd, 0 ) ;
	if ( addr == (caddr_t)(-1) )
		return( 0 ) ;

	SIO_MNEED( addr, mapped_length ) ;

	/*
	 * Map as much as possible in the first unit
	 */
	bytes_in_unit = MIN( mapped_length, map_unit_size ) ;
	mdp->first_unit.addr 			= addr ;
	mdp->first_unit.mapped_bytes 	= bytes_in_unit ;
	mdp->first_unit.valid_bytes 	= bytes_in_unit ;

	/*
	 * If there is more, map it in the second unit.
	 */
	bytes_left = mapped_length - bytes_in_unit ;
	if ( bytes_left > 0 )
	{
		mdp->second_unit.addr 			= addr + bytes_in_unit ;
		mdp->second_unit.mapped_bytes = bytes_left ;
		mdp->second_unit.valid_bytes 	= bytes_left ;
	}
	else
		mdp->second_unit.addr 			= CHAR_NULL ;

	mdp->file_offset = mapped_length ;

	return( mdp->first_unit.valid_bytes ) ;
}


/*
 * ALGORITHM:
 *
 *		if ( there are more bytes in the file )
 *		{
 *			map them at the given unit
 *			update offset
 *			issue SIO_MNEED()
 *		}
 *		else
 *			unmap the unit
 */
PRIVATE status_e map_unit(register mapd_s *mdp, int fd, register struct map_unit *mup)
{
	register size_t bytes_left = mdp->file_size - mdp->file_offset ;
	register size_t bytes_to_map = MIN( bytes_left, map_unit_size ) ;

#ifdef EVENTS
	EVENT( fd, EV_MAP_UNIT ) ;
#endif

	if ( bytes_to_map > 0 )
	{
		if ( SIO_MMAP( mup->addr, bytes_to_map,
			       fd, mdp->file_offset ) == (caddr_t)(-1) )
			return( FAILURE ) ;			/* XXX: need to do more ? */

		mup->valid_bytes = bytes_to_map ;
		ASSERT( mup->valid_bytes <= mup->mapped_bytes ) ;
		mdp->file_offset += bytes_to_map ;
		SIO_MNEED( mup->addr, mup->valid_bytes ) ;
	}
	else
	{
		(void) SIO_MUNMAP( mup->addr, mup->mapped_bytes ) ;
		mup->addr = CHAR_NULL ;
	}
	return( SUCCESS ) ;
}

#else

#define try_memory_mapping( x, y, z )				FAILURE

#endif /* HAS_MMAP */


PRIVATE status_e setup_read_buffer(register __sio_id_t *idp, unsigned int buf_size)
{
	register char *buf ;

	/*
	 * First allocate space for 2 buffers: primary and auxiliary
	 */
	buf = malloc( buf_size * 2 ) ;
	if ( buf == NULL )
		return( FAILURE ) ;

	/*
	 * The descriptor buf field should point to the start of the main buffer
	 */
	idp->buf = buf + buf_size ;
	idp->buffer_size = buf_size ;
	return( SUCCESS ) ;
}


PRIVATE status_e init_input_stream(register __sio_id_t *idp, int fd, struct stat *stp)
{
#ifdef EVENTS
	EVENT( fd, EV_INIT_INPUT_STREAM ) ;
#endif

	/*
	 * First initialize the fields relevant to buffering: buf, buffer_size
	 */
	if ( try_memory_mapping( fd, idp, stp ) == FAILURE )
	{
		/*
		 * Try to use normal buffering
		 */
		unsigned buf_size = (unsigned)
#ifdef _WIN32
		  SIO_BUFFER_SIZE ;
#else
		  ( stp->st_blksize ? stp->st_blksize : SIO_BUFFER_SIZE ) ;
#endif /* _WIN32 */
		if ( setup_read_buffer( idp, buf_size ) == FAILURE )
			return( FAILURE ) ;
	}

 	/*
	 * Initialize remaining descriptor fields
	 */
	idp->max_line_length = 2 * idp->buffer_size - 1 ;
	idp->start = idp->end = idp->nextb = idp->buf ;
	idp->tied_fd = SIO_NO_TIED_FD ;

	return( SUCCESS ) ;
}


PRIVATE status_e init_output_stream(register __sio_od_t *odp, int fd, struct stat *stp)
{
	register unsigned buf_size ;
	register char *buf ;

#ifdef EVENTS
	EVENT( fd, EV_INIT_OUTPUT_STREAM ) ;
#endif

	buf_size = (unsigned)
#ifdef _WIN32
	  SIO_BUFFER_SIZE ;
#else
	  ( stp->st_blksize ? stp->st_blksize : SIO_BUFFER_SIZE ) ;
#endif /* _WIN32 */

	buf = malloc( buf_size ) ;
	if ( buf == NULL )
		return( FAILURE ) ;

	/*
	 * Initialize buffering fields
	 */
	odp->buf = buf ;
	odp->buffer_size = buf_size ;
	odp->buf_end = odp->buf + buf_size ;

	/*
	 * Initialize remaining fields
	 */
	odp->start = odp->nextb = odp->buf ;
	if ( isatty( fd ) )
		odp->buftype = SIO_LINEBUF ;

	if ( fd == 2 )
		odp->buftype = SIO_NOBUF ;

	return( SUCCESS ) ;
}


#ifndef HAS_ISATTY

#ifdef HAS_SYSVTTY

#include <termio.h>

PRIVATE int isatty( fd )
	int fd ;
{
	struct termio t ;

	if ( ioctl( fd, TCGETA, &t ) == -1 && errno == ENOTTY )
		return( FALSE ) ;
	else
		return( TRUE ) ;
}
#endif	/* HAS_SYSVTTY */

#ifdef HAS_BSDTTY

#include <sgtty.h>

PRIVATE int isatty( fd )
	int fd ;
{
	struct sgttyb s ;

	if ( ioctl( fd, TIOCGETP, &s ) == -1 && errno == ENOTTY )
		return( FALSE ) ;
	else
		return( TRUE ) ;
}
#endif	/* HAS_BSDTTY */

#endif	/* ! HAS_ISATTY */


/*
 * Initialize stream I/O for a file descriptor.
 *
 * Arguments:
 *		fd:				file descriptor
 *		dp:				descriptor pointer
 *		stream_type: 	either __SIO_INPUT_STREAM or __SIO_OUTPUT_STREAM
 *
 * Returns
 *		0 			if successful
 *	  SIO_ERR	if the file descriptor is not valid (sets errno)
 *   exits		if stream_type is not __SIO_INPUT_STREAM or __SIO_OUTPUT_STREAM
 */
int __sio_init(register __sio_descriptor_t *dp, int fd, enum __sio_stream stream_type)
{
	struct stat st ;
	void terminate() ;

#ifdef EVENTS
	EVENT( fd, EV_SIO_INIT ) ;
#endif

	if ( fstat( fd, &st ) == -1 )
		return( SIO_ERR ) ;

	switch ( stream_type )
	{
		case __SIO_INPUT_STREAM:
			if ( init_input_stream( IDP( dp ), fd, &st ) == FAILURE )
				return( SIO_ERR ) ;
			break ;

		case __SIO_OUTPUT_STREAM:
			if ( init_output_stream( ODP( dp ), fd, &st ) == FAILURE )
				return( SIO_ERR ) ;
			break ;

		default:
			terminate( "SIO __sio_init: bad stream type (internal error).\n" ) ;
			/* NOTREACHED */
	}
	dp->stream_type = stream_type ;
	dp->initialized = TRUE ;

#ifdef HAS_FINALIZATION_FUNCTION
	if ( ! finalizer_installed )
	{
		if ( ! SIO_FINALIZE( sio_cleanup ) )
		{
			char *s = "SIO __sio_init: finalizer installation failed\n" ;

			(void) write( 2, s, strlen( s ) ) ;
		}
		else
			finalizer_installed = TRUE ;
	}
#endif /* HAS_FINALIZATION_FUNCTION */

	return( 0 ) ;
}


/*
 * __sio_writef writes the data in the buffer to the file descriptor.
 *
 * It tries to write as much data as possible until either all data
 * are written or an error occurs. EINTR is the only error that is
 * ignored.
 * In case an error occurs but some data were written, that number
 * is returned instead of SIO_ERR.
 *
 * Fields modified:
 *		When successful: start, nextb
 *		When not successful: start
 *
 * Return value:
 *		Number of bytes written
 *		SIO_ERR, if write(2) fails and no data were written
 */
int __sio_writef(register __sio_od_t *odp, int fd)
{
	register int b_in_buffer ;
	register int cc_total = 0 ;

#ifdef EVENTS
	EVENT( fd, EV_SIO_WRITEF ) ;
#endif

	/*
	 * Make sure we don't exceed the buffer limits
	 *	Maybe we should log this ?			XXX
	 */
	if ( odp->nextb > odp->buf_end )
		odp->nextb = odp->buf_end ;

	b_in_buffer = odp->nextb - odp->start ;

	if ( b_in_buffer == 0 )
		return( 0 ) ;

	for ( ;; )
	{
		register int cc ;

		cc = write( fd, odp->start, b_in_buffer ) ;
		if ( cc == b_in_buffer )
		{
			odp->start = odp->nextb = odp->buf ;
			cc_total += cc ;
			break ;
		}
		else if ( cc == -1 )
		{
			if ( errno == EINTR )
				continue ;
			else
				/*
				 * If some bytes were written, return that number, otherwise
				 * return SIO_ERR
				 */
				return( ( cc_total != 0 ) ? cc_total : SIO_ERR ) ;
		}
		else			/* some bytes were written */
		{
			odp->start += cc ;			/* advance start of buffer */
			b_in_buffer -= cc ;			/* decrease number bytes left in buffer */
			cc_total += cc ;				/* count the bytes that were written */
		}
	}
	return( cc_total ) ;
}


/*
 * __sio_readf reads data from the file descriptor into the buffer.
 * Unlike __sio_writef it does NOT try to read as much data as will fit
 * in the buffer. It ignores EINTR.
 *
 * Returns: # of bytes read or SIO_ERR
 *
 * Fields set:
 * 		If it does not return SIO_ERR, it sets start, nextb, end
 *			If it returns SIO_ERR, it does not change anything
 */
int __sio_readf(register __sio_id_t *idp, int fd)
{
	register int cc ;

#ifdef EVENTS
	EVENT( fd, EV_SIO_READF ) ;
#endif

	/*
	 * First check for a tied fd and flush the stream if necessary
	 *
	 * 		XXX	the return value of __sio_writef is not checked.
	 *					Is that right ?
	 */
	if ( idp->tied_fd != SIO_NO_TIED_FD )
		(void) __sio_writef( &__SIO_OD( idp->tied_fd ), idp->tied_fd ) ;

#ifdef HAS_MMAP
	if ( idp->memory_mapped )
	{
		register mapd_s *mdp = MDP( fd ) ;

		/*
		 * The functions initial_map and map_unit may fail.
		 * In either case, we switch to buffered I/O.
		 * If initial_map fails, we have read no data, so we
		 * should perform a read(2).
		 * If map_unit fails (for the next unit), we still have
		 * the data in the current unit, so we can return.
		 */
		if ( FIRST_TIME( idp ) )
		{
			cc = initial_map( mdp, fd ) ;
			if ( cc > 0 )
				idp->buf = mdp->first_unit.addr ;
			else
			{
				if ( __sio_switch( idp, fd ) == FAILURE )
					return( SIO_ERR ) ;
				cc = -1 ;
			}
		}
		else
		{
			register struct map_unit *mu_cur, *mu_next ;

			if ( idp->buf == mdp->first_unit.addr )
			{
				mu_cur = &mdp->first_unit ;
				mu_next = &mdp->second_unit ;
			}
			else
			{
				mu_cur = &mdp->second_unit ;
				mu_next = &mdp->first_unit ;
			}

			if ( mu_next->addr != NULL )
			{
				idp->buf = mu_next->addr ;
				cc = mu_next->valid_bytes ;
				/*
				 * XXX:  Here we may return SIO_ERR even though there
				 *		   are data in the current unit because the switch
				 *		   fails (possibly because malloc failed).
				 */
				if ( map_unit( mdp, fd, mu_cur ) == FAILURE &&
										__sio_switch( idp, fd ) == FAILURE )
					return( SIO_ERR ) ;
			}
			else
				cc = 0 ;
		}
		if ( cc >= 0 )
		{
			idp->end = idp->buf + cc ;
			idp->start = idp->nextb = idp->buf ;
			return( cc ) ;
		}
	}
#endif /* HAS_MMAP */

	for ( ;; )
	{
		cc = read( fd, idp->buf, (int) idp->buffer_size ) ;
		if ( cc == -1 )
			if ( errno == EINTR )
				continue ;
			else
				return( SIO_ERR ) ;
		else
			break ;
	}

	idp->end = idp->buf + cc ;
	idp->start = idp->nextb = idp->buf ;
	return( cc ) ;
}


/*
 * __sio_extend_buffer is used by Srdline to extend the buffer
 * If successful, it returns the number of bytes that have been read.
 * If it fails (because of end-of-file or I/O error), it returns 0 or -1.
 *
 * Fields modified:
 * 	idp->start points to the start of the buffer area (which is in the
 * 	auxiliary buffer)
 *		Also, if successful, idp->nextb is set to idp->buf, idp->end is modified.
 */
int __sio_extend_buffer(register __sio_id_t *idp, int fd, register int b_left)
{
	register int b_read ;

#ifdef EVENTS
	EVENT( fd, EV_SIO_EXTEND_BUFFER ) ;
#endif

	/*
	 * copy to auxiliary buffer
	 */
	if ( b_left )
		sio_memcopy( idp->nextb, idp->buf - b_left, b_left ) ;
	b_read = __sio_readf( idp, fd ) ;
	idp->start = idp->buf - b_left ;
	return( b_read ) ;
}


/*
 * __sio_more tries to read more data from the given file descriptor iff
 * there is free space in the buffer.
 * __sio_more is used only by Srdline and only AFTER __sio_extend_buffer
 * has been called. This implies that
 *		a) this is not a memory mapped file
 *		b) __sio_readf has been called (so we don't need to check for tied fd's
 *
 * Fields modified (only if successful):
 *			idp->end
 *
 * Return value: the number of bytes read.
 */
int __sio_more(register __sio_id_t *idp, int fd)
{
	register int b_left = &idp->buf[ idp->buffer_size ] - idp->end ;
	register int cc ;

#ifdef EVENTS
	EVENT( fd, EV_SIO_MORE ) ;
#endif

	if ( b_left <= 0 )
		return( 0 ) ;

	for ( ;; )
	{
		cc = read( fd, idp->end, b_left ) ;
		if ( cc >= 0 )
		{
			idp->end += cc ;
			return( cc ) ;
		}
		else
			if ( errno == EINTR )
				continue ;
			else
				return( SIO_ERR ) ;
	}
}


/*
 * Finalize a buffer by unmapping the file or freeing the malloc'ed memory
 */
int Sdone(int fd)
{
	register __sio_descriptor_t *dp = &__sio_descriptors[ fd ] ;

#ifdef EVENTS
	EVENT( fd, EV_SDONE ) ;
#endif

	if ( ! DESCRIPTOR_INITIALIZED( dp ) )
	{
		errno = EBADF ;
		return( SIO_ERR ) ;
	}

	switch ( dp->stream_type )
	{
		case __SIO_INPUT_STREAM:
			{
				register __sio_id_t *idp = IDP( dp ) ;

#ifdef HAS_MMAP
				if ( idp->memory_mapped )
				{
					register mapd_s *mdp = MDP( fd ) ;

					if ( mdp->first_unit.addr != CHAR_NULL )
						(void) SIO_MUNMAP( mdp->first_unit.addr,
														mdp->first_unit.mapped_bytes ) ;
					if ( mdp->second_unit.addr != CHAR_NULL )
						(void) SIO_MUNMAP( mdp->second_unit.addr,
														mdp->second_unit.mapped_bytes ) ;
					idp->memory_mapped = FALSE ;
				}
				else
#endif	/* HAS_MMAP */
					free( idp->buf - idp->buffer_size ) ;
					idp->nextb = idp->end = NULL ;
			}
			break ;

		case __SIO_OUTPUT_STREAM:
			{
				register __sio_od_t *odp = ODP( dp ) ;

				if ( Sflush( fd ) == SIO_ERR )
					return( SIO_ERR ) ;
				free( odp->buf ) ;
				odp->nextb = odp->buf_end = NULL ;
			}
			break ;

		default:
			terminate( "SIO Sdone: bad stream type\n" ) ;
	}

	dp->initialized = FALSE ;
	return( 0 ) ;
}


PRIVATE char *expand(char *area, unsigned int old_size, unsigned int new_size, int is_static)
{
	char *new_area ;

	if ( is_static )
	{
		if ( ( new_area = malloc( new_size ) ) == NULL )
			return( NULL ) ;
		sio_memcopy( area, new_area, old_size ) ;
	}
	else
		if ( ( new_area = realloc( area, new_size ) ) == NULL )
			return( NULL ) ;
	return( new_area ) ;
}


#if !defined(_WIN32) || defined(__CYGWIN__)
#include <sys/time.h>
#include <sys/resource.h>
#endif /* !_WIN32 || __CYGWIN__ */

PRIVATE int get_fd_limit(void)
{
#ifdef RLIMIT_NOFILE

	struct rlimit rl ;

	(void) getrlimit( RLIMIT_NOFILE, &rl ) ;
	return( rl.rlim_cur ) ;

#else

	return( N_SIO_DESCRIPTORS ) ;

#endif
}

/*
 * Expand the descriptor array (and if we use memory mapping the
 * memory mapping descriptors). We first expand the memory mapping
 * descriptors.
 * There is no problem if the expansion of the SIO descriptors fails
 * (i.e. there is no need to undo anything).
 */
int Smorefds(void)
{
	char *p ;
	int is_static ;
	unsigned new_size, old_size ;
	int n_fds = get_fd_limit() ;

	if ( n_fds <= n_descriptors )
		return( 0 ) ;

#ifdef EVENTS
	old_size = n_descriptors * sizeof( events_s ) ;
	new_size = n_fds * sizeof( events_s ) ;
	is_static = ( __sio_events == static___sio_events ) ;
	p = expand( (char *)__sio_events, old_size, new_size, is_static ) ;
	if ( p == NULL )
		return( SIO_ERR ) ;
	__sio_events = (events_s *) p ;

	/*
	 * Clear the codes field of the extra events structs.
	 * We have to do this because a non-null codes field implies that
	 * events recording is on for that fd
	 */
	{
		int i ;

		for ( i = n_descriptors ; i < n_fds ; i++ )
			__sio_events[i].codes = NULL ;
	}
#endif	/* EVENTS */

#ifdef HAS_MMAP
	old_size = n_descriptors * sizeof( mapd_s ) ;
	new_size = n_fds * sizeof( mapd_s ) ;
	is_static = ( mmap_descriptors == static_mapd_array ) ;
	p = expand( (char *)mmap_descriptors, old_size, new_size, is_static ) ;
	if ( p == NULL )
		return( SIO_ERR ) ;
	mmap_descriptors = (mapd_s *) p ;
#endif	/* HAS_MMAP */

	old_size = n_descriptors * sizeof( __sio_descriptor_t ) ;
	new_size = n_fds * sizeof( __sio_descriptor_t ) ;
	is_static =  ( __sio_descriptors == static_descriptor_array ) ;
	p = expand( (char *)__sio_descriptors, old_size, new_size, is_static ) ;
	if ( p == NULL )
		return( SIO_ERR ) ;
	__sio_descriptors = (__sio_descriptor_t *) p ;

	n_descriptors = n_fds ;
	return( 0 ) ;
}


#ifdef EVENTS

/*
 * Enable recording of events for the specified file descriptor
 */
int __sio_enable_events( fd )
	int fd ;
{
	char *p = malloc( EVENT_ENTRIES * sizeof( short ) ) ;

	if ( p == NULL )
		return( SIO_ERR ) ;

	__sio_events[ fd ].codes = (short *) p ;
	return( 0 ) ;
}


/*
 * Disable recording of events for the specified file descriptor
 */
void __sio_disable_events( fd )
	int fd ;
{
	if ( __sio_events[ fd ].codes != NULL )
	{
		free( (char *) __sio_events[ fd ].codes ) ;
		__sio_events[ fd ].codes = NULL ;
	}
}


/*
 * Move stored events to buf
 */
int __sio_get_events( fd, buf, size )
	int fd ;
	char *buf ;
	int size ;
{
	events_s *evp = &__sio_events[ fd ] ;
	int bufentries ;
	int range1, range2 ;
	int diff ;
	char *p ;
	int cc ;
	int cc_total ;
	int move_entries ;

	if ( evp->codes == NULL )
		return( 0 ) ;

	diff = evp->next - evp->start ;
	if ( diff == 0 )
		return( 0 ) ;

	if ( diff > 0 )
	{
		range1 = diff ;
		range2 = 0 ;
	}
	else
	{
		range1 = EVENT_ENTRIES - evp->start ;
		range2 = evp->next ;
	}

	bufentries = size / sizeof( short ) ;
	p = buf ;
	cc_total = 0 ;

	move_entries = MIN( range1, bufentries ) ;
	cc = move_entries * sizeof( short ) ;
	sio_memcopy( (char *) &evp->codes[ evp->start ], p, cc ) ;
	cc_total += cc ;
	p += cc ;
	bufentries -= range1 ;
	ADD( evp->start, move_entries ) ;

	if ( bufentries == 0 || range2 == 0 )
		return( cc_total ) ;

	move_entries = MIN( range2, bufentries ) ;
	cc = move_entries * sizeof( short ) ;
	sio_memcopy(  (char *) &evp->codes[ evp->start ], p, cc ) ;
	cc_total += cc ;
	ADD( evp->start, move_entries ) ;

	return( cc_total ) ;
}

#endif 	/* EVENTS */


/*
 * Simple function that prints the string s at stderr and then calls
 * exit
 */
PRIVATE void terminate(char *s)
{
	(void) write( 2, s, strlen( s ) ) ;
	(void) abort() ;
	exit( 1 ) ;				/* in case abort fails */
}

