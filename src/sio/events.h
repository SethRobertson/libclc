/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms
 * and conditions for redistribution.
 */


/*
 * $Id: events.h,v 1.2 2003/06/17 05:10:54 seth Exp $
 */

/*
 * Event codes
 *
 * We use a 2 letter code so that events that accumulate in a buffer
 * can be displayed as a string
 * We follow the convention that the first event letter is a capitalized
 * and the second letter is in lower case. This allows one to easily
 * recognize events in an event string.
 */

/*
 * The ENCODE macro takes 2 characters and creates a short integer
 * The size of the short integer is assumed to be 16-bits.
 * The macro makes sure that regardless of the endianess of the machine,
 * the low order byte contains the 1st character and the high order byte
 * contains the 2nd character.
 */
#ifdef LITTLE_ENDIAN
#define ENCODE( c1, c2 )					( (c1) + ( (c2) << 8 ) )
#else		/* BIG_ENDIAN */
#define ENCODE( c1, c2 )					( (c2) + ( (c1) << 8 ) )
#endif

/*
 * Event codes for SIO interface functions
 * We use the first 2 lettes after the initial 'S'
 */
#define EV_SGETC						ENCODE( 'G', 'e' )
#define EV_SPUTC						ENCODE( 'P', 'u' )
#define EV_SREAD						ENCODE( 'R', 'e' )
#define EV_SWRITE						ENCODE( 'W', 'r' )
#define EV_SRDLINE					ENCODE( 'R', 'd' )
#define EV_SFETCH						ENCODE( 'F', 'e' )
#define EV_SUNDO						ENCODE( 'U', 'n' )
#define EV_SDONE						ENCODE( 'D', 'o' )
#define EV_SFLUSH						ENCODE( 'F', 'l' )
#define EV_SCLOSE						ENCODE( 'C', 'l' )
#define EV_STIE						ENCODE( 'T', 'i' )
#define EV_SUNTIE						ENCODE( 'U', 't' )
#define EV_SBUFTYPE					ENCODE( 'B', 'u' )

/*
 * Event codes for internal functions
 * For the __sio_<name> functions we use 'S' and the first letter of <name>
 * For the rest we use the first letter from the first two components of
 * their name, for example for try_memory_mapping we use Tm.
 */
#define EV_SIO_INIT					ENCODE( 'S', 'i' )
#define EV_SIO_SWITCH				ENCODE( 'S', 's' )
#define EV_SIO_READF					ENCODE( 'S', 'r' )
#define EV_SIO_WRITEF				ENCODE( 'S', 'w' )
#define EV_SIO_EXTEND_BUFFER		ENCODE( 'S', 'e' )
#define EV_SIO_MORE					ENCODE( 'S', 'm' )
#define EV_TRY_MEMORY_MAPPING		ENCODE( 'T', 'm' )
#define EV_INITIAL_MAP				ENCODE( 'I', 'm' )
#define EV_MAP_UNIT					ENCODE( 'M', 'u' )
#define EV_INIT_INPUT_STREAM		ENCODE( 'I', 'i' )
#define EV_INIT_OUTPUT_STREAM		ENCODE( 'I', 'o' )

/*
 * The # of entries must be a power of 2
 */
#define EVENT_ENTRIES		512

struct events
{
	short next ;
	short start ;
	short *codes ;			/* malloc'ed memory */
} ;

typedef struct events events_s ;

extern events_s *__sio_events ;


#define ADD( index, x )				(index) += x ;								\
											(index) &= ( EVENT_ENTRIES - 1 )

#define EVENT( fd, code )															\
					{																		\
						events_s *evp = &__sio_events[ fd ] ;					\
																							\
						if ( evp->codes != NULL )									\
						{																	\
							evp->codes[ evp->next ] = code ;						\
							ADD( evp->next, 1 ) ;									\
							if ( evp->next == evp->start )						\
							{ ADD( evp->start, 1 ) ; }								\
						}																	\
					}

