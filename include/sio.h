/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

/*
 * $Id: sio.h,v 1.1 2001/05/26 22:04:51 seth Exp $
 */

#ifndef __SIO_H
#define __SIO_H

#include <errno.h>
#include <stdarg.h>

/*
 * Naming conventions:
 *		1) SIO functions and macros have names starting with a capital S
 *		2) SIO constants meant to be used by user programs have 
 *			names starting with SIO_
 *		3) Internal functions, struct identifiers, enum identifiers 
 *			etc. have names starting with __sio
 *		4) Internal constants and macros have names starting with __SIO
 */


/*
 * external constants
 *
 * SIO_FLUSH_ALL: flush all output streams
 * SIO_EOF:	eof on stream
 * SIO_ERR: operation failed
 */
#define SIO_FLUSH_ALL				(-1)
#define SIO_EOF						(-2)
#define SIO_ERR						(-1)

/*
 * Undo types
 */
#define SIO_UNDO_LINE		0
#define SIO_UNDO_CHAR		1

/*
 * Buffering types
 */
#define SIO_FULLBUF			0
#define SIO_LINEBUF			1
#define SIO_NOBUF				2

/*
 * Descriptor for an input stream
 */
struct __sio_input_descriptor
{
	/*
	 * buf:		points to the buffer area.
	 *				When doing memory mapping, it is equal to the unit 
	 *				from which we are reading. When doing buffered I/O
	 *				it points to the primary buffer.
	 */
	char *buf ;
	unsigned buffer_size ;

	char *start ;                 /* start of valid buffer contents   	*/
	char *end ;                   /* end of valid buffer contents + 1 	*/
	char *nextb ;                 /* pointer to next byte to read/write 	*/
											/* Always:  start <= nextb < end			*/

	unsigned line_length ;
	int max_line_length ;
	int tied_fd ;

	int memory_mapped ;				/* flag to denote if we use				*/
											/* memory mapping								*/
} ;

typedef struct __sio_input_descriptor __sio_id_t ;


/*
 * Descriptor for an output stream
 */
struct __sio_output_descriptor
{
	/*
	 * buf:		points to the buffer area.
	 * buf_end: is equal to buf + buffer_size
	 */
	char *buf ;
	char *buf_end ;

	unsigned buffer_size ;

	char *start ;                 /* start of valid buffer contents   	*/
											/* (used by the R and W functions) 		*/
	char *nextb ;                 /* pointer to next byte to read/write  */
											/* Always:  start <= nextb < buf_end	*/
	int buftype ;						/* type of buffering 						*/
} ;

typedef struct __sio_output_descriptor __sio_od_t ;



/*
 * Stream types
 */
enum __sio_stream { __SIO_INPUT_STREAM, __SIO_OUTPUT_STREAM } ;


/*
 * General descriptor
 */
struct __sio_descriptor
{
	union
	{
		__sio_id_t input_descriptor ;
		__sio_od_t output_descriptor ;
	} descriptor ;
	enum __sio_stream stream_type ;
	int initialized ;
} ;

typedef struct __sio_descriptor __sio_descriptor_t ;


/*
 * The array of descriptors (as many as available file descriptors)
 */
extern __sio_descriptor_t *__sio_descriptors ;

extern int errno ;


/*
 * Internally used macros
 */
#define __SIO_FD_INITIALIZED( fd )		(__sio_descriptors[ fd ].initialized)
#define __SIO_ID( fd )	(__sio_descriptors[ fd ].descriptor.input_descriptor)
#define __SIO_OD( fd )	(__sio_descriptors[ fd ].descriptor.output_descriptor)
#define __SIO_MUST_FLUSH( od, ch )													\
					( (od).buftype != SIO_FULLBUF &&									\
						( (od).buftype == SIO_NOBUF || ch == '\n' ) )


/*
 * SIO Macros:
 *
 *		SIOLINELEN( fd )
 *		SIOMAXLINELEN( fd )
 *		Sputchar( fd, c )
 *		Sgetchar( fd )
 *
 * NOTE: The maximum line size depends on whether the descriptor
 *			was originally memory mapped. If it was, then the maximum
 *			line size will be the map_unit_size (a function of the system
 *			page size and PAGES_MAPPED). Otherwise, it will be either the
 *			optimal block size as reported by stat(2) or SIO_BUFFER_SIZE.
 */

#define SIOLINELEN( fd )      __SIO_ID( fd ).line_length
#define SIOMAXLINELEN( fd )																	\
	(																									\
		__SIO_FD_INITIALIZED( fd )																\
			? ( 																						\
				 (__sio_descriptors[ fd ].stream_type == __SIO_INPUT_STREAM)		\
					? __SIO_ID( fd ).max_line_length											\
					: ( errno = EBADF, SIO_ERR )												\
			  )																						\
			: (		/* not initialized; initialize it for input */					\
				 (__sio_init( &__sio_descriptors[ fd ], fd, __SIO_INPUT_STREAM )	\
																					== SIO_ERR)		\
					? SIO_ERR																		\
					: __SIO_ID( fd ).max_line_length											\
			  )																						\
	)



/*
 * Adds a character to a buffer, returns the character or SIO_ERR
 */
#define  __SIO_ADDCHAR( od, fd, c )                                  \
     ( od.buftype == SIO_FULLBUF )                                   \
         ? (int) ( *(od.nextb)++ = (unsigned char) (c) )             \
         : ( od.buftype == SIO_LINEBUF )                             \
               ? ( ( *(od.nextb) = (unsigned char) (c) ) != '\n' )   \
                     ? (int) *(od.nextb)++                           \
                     : Sputc( fd, *(od.nextb) )                      \
               : Sputc( fd, c )


/*
 * The Sgetchar/Sputchar macros depend on the fact that the fields 
 * 				nextb, buf_end, end
 * are 0 if a stream descriptor is not being used or has not yet been
 * initialized.
 * This is true initially because of the static allocation of the
 * descriptor array, and Sdone must make sure that it is true
 * after I/O on a descriptor is over.
 */
#define Sputchar( fd, c )														\
		(																				\
			( __SIO_OD( fd ).nextb < __SIO_OD( fd ).buf_end )			\
				? ( __SIO_ADDCHAR( __SIO_OD( fd ), fd, c ) )				\
				: Sputc( fd, c )													\
		)

#define Sgetchar( fd )													\
		(																		\
			( __SIO_ID( fd ).nextb < __SIO_ID( fd ).end )		\
				? (int) *__SIO_ID( fd ).nextb++ 						\
				: Sgetc( fd )												\
		)


#ifdef __ARGS
#undef __ARGS
#endif

#ifdef PROTOTYPES
#	define __ARGS( s )					s
#else
#	define __ARGS( s )					()
#endif

/*
 * The Read functions
 */
int Sread __ARGS( ( int fd, char *buf, int nbytes ) ) ;
int Sgetc __ARGS( ( int fd ) ) ;
char *Srdline __ARGS( ( int fd ) ) ;
char *Sfetch __ARGS( ( int fd, long *length ) ) ;

/*
 * The Write functions
 */
int Swrite __ARGS( ( int fd, char *buf, int nbytes ) ) ;
int Sputc __ARGS( ( int fd, char c ) ) ;
int Sprint __ARGS( ( int fd, char *format, ... ) ) ;
int Sprintv __ARGS( ( int fd, char *format, va_list ) ) ;

/*
 * other functions
 */
int Sdone __ARGS( ( int fd ) ) ;
int Sundo __ARGS( ( int fd, int type ) ) ;
int Sflush __ARGS( ( int fd ) ) ;
int Sclose __ARGS( ( int fd ) ) ;
int Suntie(int fd);
int Stie(int in_fd, int out_fd);
int Sbuftype __ARGS( ( int fd, int type ) ) ;
int Smorefds(void);

#endif /* __SIO_H */

