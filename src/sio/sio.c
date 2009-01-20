/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */

static const char sio_version[] = VERSION;

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined (_WIN32) && !defined(__CYGWIN__)
# include <sosnt.h>
#endif /* _WIN32 && !__CYGWIN__ */

#include "sio.h"
#include "impl.h"

#ifdef EVENTS
#include "events.h"
#endif
#include "clchack.h"


/*
 * SIO WRITE FUNCTIONS: Swrite, Sputc
 */

/*
 * Stream write call: arguments same as those of write(2)
 */
int Swrite(int fd, register char *addr, register int nbytes)
{
	register __sio_descriptor_t *dp = &__sio_descriptors[ fd ] ;
	register __sio_od_t *odp = ODP( dp ) ;
	register int b_transferred ;
	register int b_avail ;
	int total_b_transferred ;
	int b_written ;
	int b_in_buffer ;

#ifdef EVENTS
	EVENT( fd, EV_SWRITE ) ;
#endif

	IO_SETUP( fd, dp, __SIO_OUTPUT_STREAM, SIO_ERR ) ;
	ASSERT( odp->start <= odp->nextb && odp->nextb <= odp->buf_end ) ;

	b_avail = odp->buf_end - odp->nextb ;
	b_transferred = MIN( nbytes, b_avail ) ;
	sio_memcopy( addr, odp->nextb, b_transferred ) ;
	odp->nextb += b_transferred ;

	/*
	 * check if we are done
	 */
	if ( b_transferred == nbytes )
		return( b_transferred ) ;

	/*
	 * at this point we know that the buffer is full
	 */
	b_in_buffer = odp->buf_end - odp->start ;
	b_written = __sio_writef( odp, fd ) ;
	if ( b_written != b_in_buffer )
		return( (b_written >= nbytes) ? nbytes : b_written ) ;

	total_b_transferred = b_transferred ;
	addr += b_transferred ;
	nbytes -= b_transferred ;

	for ( ;; )
	{
		b_transferred = MIN( nbytes, odp->buffer_size ) ;
		sio_memcopy( addr, odp->nextb, b_transferred ) ;
		odp->nextb += b_transferred ;
		nbytes -= b_transferred ;
		if ( nbytes == 0 )
		{
			total_b_transferred += b_transferred ;
			break ;
		}
		/*
		 * the buffer is full
		 */
		b_written = __sio_writef( odp, fd ) ;
		if ( b_written != odp->buffer_size )
		{
			if ( b_written != SIO_ERR )
			{
				total_b_transferred += b_written ;
				odp->nextb += b_written ;
			}
			break ;
		}
		/*
		 * everything is ok
		 */
		total_b_transferred += b_transferred ;
		addr += b_transferred ;
	}
	return( total_b_transferred ) ;
}


/*
 * Add a character to a file
 */
int Sputc(int fd, char c)
{
	register __sio_descriptor_t *dp = &__sio_descriptors[ fd ] ;
	register __sio_od_t *odp = ODP( dp ) ;

#ifdef EVENTS
	EVENT( fd, EV_SPUTC ) ;
#endif

	IO_SETUP( fd, dp, __SIO_OUTPUT_STREAM, SIO_ERR ) ;
	ASSERT( odp->start <= odp->nextb && odp->nextb <= odp->buf_end ) ;

	/*
	 * The following is a weak check since we should really be
	 * checking that nextb == buf_end (it would be an error for
	 * nextb to exceed buf_end; btw, the assertion above, when
	 * enabled makes sure this does not occur).
	 *
	 * NOTE: __sio_writef NEVER uses data beyond the end of buffer.
	 */
	if ( odp->nextb >= odp->buf_end )
	{
		int b_in_buffer = odp->buf_end - odp->start ;

		/*
		 * There is nothing we can do if __sio_writef does not manage
		 * to write the whole buffer
		 */
		if ( __sio_writef( odp, fd ) != b_in_buffer )
			return( SIO_ERR ) ;
	}
	*odp->nextb++ = c ;
	if ( __SIO_MUST_FLUSH( *odp, c ) && __sio_writef( odp, fd ) == SIO_ERR )
		return( SIO_ERR ) ;
	return ( c ) ;
}



/*
 * SIO READ FUNCTIONS
 */

/*
 * Stream read call: arguments same as those of read(2)
 */
int Sread(int fd, char *addr, int nbytes)
{
	register __sio_descriptor_t *dp = &__sio_descriptors[ fd ] ;
	register __sio_id_t *idp = IDP( dp ) ;
	register int b_transferred ;
	int b_read ;
	int total_b_transferred ;
	int b_left ;

#ifdef EVENTS
	EVENT( fd, EV_SREAD ) ;
#endif

	IO_SETUP( fd, dp, __SIO_INPUT_STREAM, SIO_ERR ) ;
	ASSERT( idp->start <= idp->nextb && idp->nextb <= idp->end ) ;

	b_left = idp->end - idp->nextb ;
	b_transferred = MIN( b_left, nbytes ) ;
	sio_memcopy( idp->nextb, addr, b_transferred ) ;
	idp->nextb += b_transferred ;
	if ( b_transferred == nbytes )
		return( b_transferred ) ;

	nbytes -= b_transferred ;
	total_b_transferred = b_transferred ;
	addr += b_transferred ;

	do
	{
		b_read = __sio_readf( idp, fd ) ;
		switch ( b_read )
		{
			case SIO_ERR:
				if ( total_b_transferred == 0 )
					return( SIO_ERR ) ;
				/* FALL THROUGH */
		
			case 0:
				return( total_b_transferred ) ;
		}
		
		b_transferred = MIN( b_read, nbytes ) ;
		sio_memcopy( idp->nextb, addr, b_transferred ) ;
		addr += b_transferred ;
		idp->nextb += b_transferred ;
		total_b_transferred += b_transferred ;
		nbytes -= b_transferred ;
	}
	while ( nbytes && b_read == idp->buffer_size ) ;
	return( total_b_transferred ) ;
}



/*
 * Read a line from a file
 * Returns a pointer to the beginning of the line or NULL
 */
char *Srdline(int fd)
{
	register __sio_descriptor_t *dp = &__sio_descriptors[ fd ] ;
	register __sio_id_t *idp = IDP( dp ) ;
	register char *cp ;
	register char *line_start ;
	register int b_left ;
	register int extension ;

#ifdef EVENTS
	EVENT( fd, EV_SRDLINE ) ;
#endif

	IO_SETUP( fd, dp, __SIO_INPUT_STREAM, NULL ) ;
	ASSERT( idp->start <= idp->nextb && idp->nextb <= idp->end ) ;

#ifdef HAS_MMAP
	if ( idp->memory_mapped && __sio_switch( idp, fd ) == FAILURE )
		return( NULL ) ;
#endif

	b_left = idp->end - idp->nextb ;
	/*
	 * Look for a '\n'. If the search fails, extend the buffer
	 * and search again (the extension is performed by copying the
	 * bytes that were searched to the auxiliary buffer and reading
	 * new input in the main buffer).
	 * If the new input still does not contain a '\n' and there is
	 * more space in the main buffer (this can happen with network
	 * connections), read more input until either the buffer is full
	 * or a '\n' is found.
	 * Finally, set cp to point to the '\n', and line_start to
	 * the beginning of the line
	 */
	if ( b_left && ( cp = sio_memscan( idp->nextb, b_left, '\n' ) ) != NULL )
	{
		line_start = idp->nextb ;
		idp->nextb = cp + 1 ;
	}
	else
	{
		extension = __sio_extend_buffer( idp, fd, b_left ) ;
		if ( extension > 0 )
		{
			ASSERT( idp->start <= idp->nextb && idp->nextb <= idp->end ) ;

			line_start = idp->start ;
			cp = sio_memscan( idp->nextb, extension, '\n' ) ;
			if ( cp != NULL )
				idp->nextb = cp + 1 ;
			else
				for ( ;; )
				{
					idp->nextb = idp->end ;
					extension = __sio_more( idp, fd ) ;
					if ( extension > 0 )
					{
						cp = sio_memscan( idp->nextb, extension, '\n' ) ;
						if ( cp == NULL )
							continue ;
						idp->nextb = cp + 1 ;
						break ;
					}
					else
					{
						/*
						 * If there is spare room in the buffer avoid trashing
						 * the last character
						 */
						if ( idp->end < &idp->buf[ idp->buffer_size ] )
							cp = idp->end ;
						else
							cp = &idp->buf[ idp->buffer_size - 1 ] ;
						break ;
					}
				}
		}
		else					/* buffer could not be extended */
			if ( b_left == 0 )
			{
				/*
				 * Set errno to 0 if EOF has been reached
				 */
				if ( extension == 0 )
					errno = 0 ;
				return( NULL ) ;
			}
			else
			{
				line_start = idp->start ;
				cp = idp->end ;
				/*
				 * By setting idp->nextb to be equal to idp->end,
				 * subsequent calls to Srdline will return NULL because
				 * __sio_extend_buffer will be invoked and it will return 0.
				 */
				idp->nextb = idp->end ;
			}
	}
	*cp = NUL ;
	idp->line_length = cp - line_start ;
	return( line_start ) ;
}


/*
 * Get a character from a file
 */
int Sgetc(int fd)
{
	register __sio_descriptor_t *dp = &__sio_descriptors[ fd ] ;
	register __sio_id_t *idp = IDP( dp ) ;

#ifdef EVENTS
	EVENT( fd, EV_SGETC ) ;
#endif

	IO_SETUP( fd, dp, __SIO_INPUT_STREAM, SIO_ERR ) ;
	ASSERT( idp->start <= idp->nextb && idp->nextb <= idp->end ) ;
	if ( idp->nextb >= idp->end )
	{
		register int b_read = __sio_readf( idp, fd ) ;

		if ( b_read == 0 )
			return( SIO_EOF ) ;
		else if ( b_read == SIO_ERR )
			return( SIO_ERR ) ;
	}
	return( (int) *idp->nextb++ ) ;
}


char *Sfetch(int fd, long int *lenp)
{
	register __sio_descriptor_t *dp = &__sio_descriptors[ fd ] ;
	register __sio_id_t *idp = IDP( dp ) ;
	register int b_read ;
	register char *p ;

#ifdef EVENTS
	EVENT( fd, EV_SFETCH ) ;
#endif

	IO_SETUP( fd, dp, __SIO_INPUT_STREAM, NULL ) ;
	ASSERT( idp->start <= idp->nextb && idp->nextb <= idp->end ) ;
	if ( idp->nextb >= idp->end )
	{
		b_read = __sio_readf( idp, fd ) ;
		if ( b_read == SIO_ERR )
			return( NULL ) ;
		if ( b_read == 0 )
		{
			errno = 0 ;
			return( NULL ) ;
		}
	}
	*lenp = idp->end - idp->nextb ;
	p = idp->nextb ;
	idp->nextb = idp->end ;
	return( p ) ;
}



/*
 * SIO CONTROL FUNCTIONS
 */

/*
 * Undo the last Srdline or Sgetc
 */
int Sundo(int fd, int type)
{
	register __sio_descriptor_t *dp = &__sio_descriptors[ fd ] ;
	register __sio_id_t *idp = IDP( dp ) ;
	int retval = 0 ;

#ifdef EVENTS
	EVENT( fd, EV_SUNDO ) ;
#endif

	CONTROL_SETUP( dp, __SIO_INPUT_STREAM, SIO_ERR ) ;

	/*
	 * Undo works only for fd's used for input
	 */
	if ( dp->stream_type != __SIO_INPUT_STREAM )
		return( SIO_ERR ) ;

	/*
	 * Check if the operation makes sense; if so, do it, otherwise ignore it
	 */
	switch ( type )
	{
		case SIO_UNDO_LINE:
			if ( idp->nextb - idp->line_length > idp->start )
			{
				*--idp->nextb = '\n' ;
				idp->nextb -= idp->line_length ;
			}
			ASSERT( idp->start <= idp->nextb && idp->nextb <= idp->end ) ;
			break ;
	
		case SIO_UNDO_CHAR:
			if ( idp->nextb > idp->start )
				idp->nextb-- ;
			ASSERT( idp->start <= idp->nextb && idp->nextb <= idp->end ) ;
			break ;
	
		default:
			retval = SIO_ERR ;
			break ;
	}
	return( retval ) ;
}


/*
 * Flush the buffer associated with the given file descriptor
 * The special value, SIO_FLUSH_ALL flushes all buffers
 *
 * Return value:
 *		0 :  if fd is SIO_FLUSH_ALL or if the flush is successful
 *		SIO_ERR: if fd is not SIO_FLUSH_ALL and
 *		the flush is unsuccessful
 *		or the descriptor is not initialized or it is not
 *		an output descriptor
 */
int Sflush(int fd)
{
   register __sio_descriptor_t *dp ;
   int b_in_buffer ;

#ifdef EVENTS
	EVENT( fd, EV_SFLUSH ) ;
#endif

   if ( fd == SIO_FLUSH_ALL )
   {
      for ( fd = 0, dp = __sio_descriptors ;
				fd < N_SIO_DESCRIPTORS ;
				dp++, fd++ )
         if ( DESCRIPTOR_INITIALIZED( dp ) &&
	      dp->stream_type == __SIO_OUTPUT_STREAM )
            (void) __sio_writef( ODP( dp ), fd ) ;
      return( 0 ) ;
   }
   else
   {
      dp = &__sio_descriptors[ fd ] ;

		CONTROL_SETUP( dp, __SIO_OUTPUT_STREAM, SIO_ERR ) ;
      b_in_buffer = ODP( dp )->nextb - ODP( dp )->start ;
      if ( __sio_writef( ODP( dp ), fd ) != b_in_buffer )
         return( SIO_ERR ) ;
      else
         return( 0 ) ;
   }
}


/*
 * Close the file descriptor. This call is provided because
 * a file descriptor may be closed and then reopened. There is
 * no easy way for SIO to identify such a situation, so Sclose
 * must be used.
 *
 * Sclose invokes Sdone which finalizes the buffer.
 * There is no SIO_CLOSE_ALL value for fd because such a thing
 * would imply that the program will exit very soon, therefore
 * the closing of all file descriptors will be done in the kernel
 * (and the finalization will be done by the finalization function
 * NOTE: not true if the OS does not support a finalization function)
 *
 * There is no need to invoke SETUP; Sdone will do it.
 */
int Sclose(int fd)
{
#ifdef EVENTS
	EVENT( fd, EV_SCLOSE ) ;
#endif

	if ( __SIO_FD_INITIALIZED( fd ) )
		if ( Sdone( fd ) == SIO_ERR )
			return( SIO_ERR ) ;
	return( close( fd ) ) ;
}



/*
 * Tie the file descriptor in_fd to the file descriptor out_fd
 * This means that whenever a read(2) is done on in_fd, it is
 * preceded by a write(2) on out_fd.
 * Why this is nice to have:
 * 	1) When used in filters it maximizes concurrency
 *		2) When the program prompts the user for something it forces
 *			the prompt string to be displayed even if it does not
 *			end with a '\n' (which would cause a flush).
 * In this implementation, out_fd cannot be a regular file.
 * This is done to avoid non-block-size write's.
 * The file descriptors are initialized if that has not been done
 * already. If any of them is initialized, it must be for the appropriate
 * stream type (input or output).
 *
 * NOTE: the code handles correctly the case when in_fd == out_fd
 */
int Stie(int in_fd, int out_fd)
{
	struct stat st ;
	register __sio_descriptor_t *dp ;
	int was_initialized ;
	boolean_e failed = NO ;

#ifdef EVENTS
	EVENT( in_fd, EV_STIE ) ;
#endif

	/*
	 * Check if the out_fd is open
	 */
	if ( fstat( out_fd, &st ) == -1 )
		return( SIO_ERR ) ;

	/*
	 * If the out_fd refers to a regular file, the request is silently ignored
	 */
	if ( ( st.st_mode & S_IFMT ) == S_IFREG )
		return( 0 ) ;

	dp = &__sio_descriptors[ in_fd ] ;
	was_initialized = dp->initialized ;		/* remember if it was initialized */
	IO_SETUP( in_fd, dp, __SIO_INPUT_STREAM, SIO_ERR ) ;

	/*
	 * Perform manual initialization of out_fd to avoid leaving in_fd
	 * initialized if the initialization of out_fd fails.
	 * If out_fd is initialized, check if it is used for output.
	 * If it is not initialized, initialize it for output.
	 */
	dp = &__sio_descriptors[ out_fd ] ;
	if ( DESCRIPTOR_INITIALIZED( dp ) )
	{
		if ( dp->stream_type != __SIO_OUTPUT_STREAM )
		{
			failed = YES ;
			errno = EBADF ;
		}
	}
	else
		if ( __sio_init( dp, out_fd, __SIO_OUTPUT_STREAM ) == SIO_ERR )
			failed = YES ;

	if ( failed == NO )
	{
		__SIO_ID( in_fd ).tied_fd = out_fd ;
		return( 0 ) ;
	}
	else
	{
		/*
		 * We failed. If we did any initialization, undo it
		 */
		if ( ! was_initialized )
		{
			int save_errno = errno ;

			(void) Sdone( in_fd ) ;
			errno = save_errno ;
		}
		return( SIO_ERR ) ;
	}
}


/*
 * Untie a file descriptor
 */
int Suntie(int fd)
{
	register __sio_descriptor_t *dp = &__sio_descriptors[ fd ] ;

#ifdef EVENTS
	EVENT( fd, EV_SUNTIE ) ;
#endif

	CONTROL_SETUP( dp, __SIO_INPUT_STREAM, SIO_ERR ) ;

	if ( IDP( dp )->tied_fd != SIO_NO_TIED_FD )
	{
		IDP( dp )->tied_fd = SIO_NO_TIED_FD ;
		return( 0 ) ;
	}
	else
	{
		errno = EBADF ;
		return( SIO_ERR ) ;
	}
}


/*
 * Changes the type of buffering on the specified descriptor.
 * As a side-effect, it initializes the descriptor as an output stream.
 */
int Sbuftype(int fd, int type)
{
	register __sio_descriptor_t *dp = &__sio_descriptors[ fd ] ;

#ifdef EVENTS
	EVENT( fd, EV_SBUFTYPE ) ;
#endif

	/*
	 * Check for a valid type
	 */
	if ( type != SIO_LINEBUF && type != SIO_FULLBUF && type != SIO_NOBUF )
	{
		errno = EINVAL ;
		return( SIO_ERR ) ;
	}

	IO_SETUP( fd, dp, __SIO_OUTPUT_STREAM, SIO_ERR ) ;
	ODP( dp )->buftype = type ;
	return( 0 ) ;
}


#ifndef sio_memscan

PRIVATE char *sio_memscan( from, how_many, ch )
   char *from ;
   int how_many ;
   register char ch ;
{
   register char *p ;
   register char *last = from + how_many ;

   for ( p = from ; p < last ; p++ )
      if ( *p == ch )
         return( p ) ;
      return( 0 ) ;
}

#endif	/* sio_memscan */


#ifdef NEED_MEMCOPY

void __sio_memcopy( from, to, nbytes )
   register char *from, *to ;
   register int nbytes ;
{
   while ( nbytes-- )
      *to++ = *from++ ;
}

#endif /* NEED_MEMCOPY */


