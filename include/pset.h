/*
 * (c) Copyright 1992, 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

#ifndef __PSET_H
#define __PSET_H

/*
 * $Id: pset.h,v 1.1 2001/05/26 22:04:51 seth Exp $
 */

typedef void *__pset_pointer ;

struct __pset
{
	unsigned			alloc_step ;
	__pset_pointer *ptrs ;
	unsigned			max ;
	unsigned			count ;
} ;


/*
 * INTERFACE
 */

#ifdef __ARGS
#undef __ARGS
#endif

#ifdef PROTOTYPES
#  define __ARGS( s )               s
#else
#  define __ARGS( s )               ()
#endif

typedef struct __pset *pset_h ;

pset_h pset_create			__ARGS( ( unsigned start, unsigned step ) ) ;
void pset_destroy				__ARGS( ( pset_h ) ) ;

__pset_pointer pset_insert __ARGS( ( pset_h, __pset_pointer ) ) ;
void pset_delete				__ARGS( ( pset_h, __pset_pointer ) ) ;

void pset_compact 			__ARGS( ( pset_h ) ) ;
void pset_uniq 				__ARGS( ( pset_h, void (*func)(), void *arg ) ) ;
void pset_apply 				__ARGS( ( pset_h, void (*func)(), void *arg ) ) ;

/*
 * Macros
 */
#define pset_add( pset, ptr )                                       		\
      (                                                                 \
         ( (pset)->count < (pset)->max )                                \
            ? (pset)->ptrs[ (pset)->count++ ] = (__pset_pointer) ptr    \
            : pset_insert( (pset), (__pset_pointer) ptr )					\
      )

#define pset_remove( pset, ptr )			pset_delete( pset, (__pset_pointer)ptr )

#define pset_remove_index( pset, i )												\
		{																						\
			if ( (i) < (pset)->count )													\
				 (pset)->ptrs[ i ] = (pset)->ptrs[ --(pset)->count ] ;		\
		}

#define pset_clear( pset )				(pset)->count = 0
#define pset_count( pset )				(pset)->count
#define pset_pointer( pset, i )		(pset)->ptrs[ i ]

#define pset_sort( pset, compfunc )													\
		(void) qsort( (char *) &pset_pointer( pset, 0 ),						\
				pset_count( pset ), sizeof( __pset_pointer ), compfunc )

/*
 * PSET iterators
 * 
 * Note that the iterators do NOT use any knowledge about the internals
 * of pset's.
 */
struct __pset_iterator
{
	pset_h pset ;
	unsigned current ;
	int step ;
} ;

typedef struct __pset_iterator *psi_h ;


#define __psi_current( iter )															\
						( (iter)->current < pset_count( (iter)->pset )			\
							? pset_pointer( (iter)->pset, (iter)->current )		\
							: NULL )

#define psi_start( iter )															\
					( (iter)->current = 0, (iter)->step = 1,						\
					__psi_current( iter ) )

#define psi_next( iter ) 															\
					( (iter)->current += (iter)->step, (iter)->step = 1,		\
					__psi_current( iter ) )

#define psi_remove( iter )															\
				{																				\
					if ( (iter)->current < pset_count( (iter)->pset ) )		\
					{																			\
						pset_remove_index( (iter)->pset, (iter)->current ) ;	\
						(iter)->step = 0 ;												\
					}																			\
				}

#define psi_reset( iter, newpset )		(iter)->pset = (newpset)

#define psi_destroy( iter )				free( (char *) iter )

psi_h psi_create __ARGS( ( pset_h ) ) ;

#endif	/* __PSET_H */

