/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

static char RCSid[] = "$Id: dlltest.c,v 1.3 2001/08/18 18:15:56 jtt Exp $" ;


#ifndef NULL
#define NULL 0
#endif

#include <stdio.h>
#include "dll.h"
#include "clchack.h"


#define COMPARE( i1, i2 )												\
										if ( i1 < i2 )						\
											return( -1 ) ;					\
										else if ( i1 == i2 )				\
											return( 0 ) ;					\
										else									\
											return( 1 ) ;

#define INTP( p )					((int *)(p))

static int int_comp(void *p1v, void *p2v)
{
  char *p1=(char *)p1v;
  char *p2=(char *)p2v;

	int i1 = *INTP( p1 ) ;
	int i2 = *INTP( p2 ) ;

	COMPARE( i1, i2 ) ;
}


static int int_kcomp(void *p1v, void *p2v)
{
  char *p1=(char *)p1v;
  char *p2=(char *)p2v;

	int k = *INTP( p1 ) ;
	int obj = *INTP( p2 ) ;

	COMPARE( k, obj ) ;
}


#define N 10
int nums[ N ] ;

int main(void)
{
	dict_h lh ;
	int i ;
	int *ip ;

	lh = dll_create( int_comp, int_kcomp, 0 ) ;

	for ( i = 0 ; i < N ; i++ )
	{
		nums[ i ] = 10-i ;
		if ( dll_insert( lh, &nums[ i ] ) != DICT_OK )
		{
			printf( "Failed at %d\n", i ) ;
			exit( 1 ) ;
		}
	}
		
	printf( "Successor test\n" ) ;
	for ( ip=INTP(dll_minimum( lh )) ; ip ; ip=INTP(dll_successor( lh, ip )) ) 
		printf( "%d\n", *ip ) ;
	printf( "Predecessor test\n" ) ;
	for ( ip=INTP(dll_maximum( lh )) ; ip ; ip=INTP(dll_predecessor( lh, ip )) ) 
		printf( "%d\n", *ip ) ;
	
	printf( "Search/delete test\n" ) ;
	i = 7 ;
	ip = INTP( dll_search( lh, &i ) ) ;
	if ( ip == NULL )
		printf( "Search failed\n" ) ;
	else
		if ( dll_delete( lh, ip ) != DICT_OK )
		{
			printf( "Delete failed\n" ) ;
			exit( 0 ) ;
		}

	printf( "Successor test 2\n" ) ;
	for ( ip=INTP(dll_minimum( lh )) ; ip ; ip=INTP(dll_successor( lh, ip )) ) 
		printf( "%d\n", *ip ) ;
	printf( "Predecessor test 2\n" ) ;
	for ( ip=INTP(dll_maximum( lh )) ; ip ; ip=INTP(dll_predecessor( lh, ip )) ) 
		printf( "%d\n", *ip ) ;

	printf( "Iteration test\n" ) ;
	dll_iterate( lh, DICT_FROM_START ) ;
	while (( ip = INTP( dll_nextobj( lh ) ) ))
		if ( *ip == 5 )
			(void) dll_delete( lh, ip ) ;
		else
			printf( "%d\n", *ip ) ;
		
	exit( 0 ) ;
}
