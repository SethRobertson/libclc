/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

static char RCSid[] = "$Id: htest.c,v 1.3 2001/08/18 18:15:56 jtt Exp $" ;

#ifndef NULL
#define NULL 0
#endif

#include <stdio.h>
#include "dict.h"
#include "ht.h"
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


static ht_val getval(void *p)
{
	return ( *INTP( (char *)p ) ) ;
}



#define N 10
int nums[ N ] ;

int main(void)
{
	dict_h lh ;
	int i ;
	int *ip ;
	struct ht_args args ;

	args.ht_bucket_entries = 2 ;
	args.ht_table_entries = 2 ;
	args.ht_objvalue = getval ;
	args.ht_keyvalue = getval ;

	lh = ht_create( int_comp, int_comp, 0, &args ) ;

	for ( i = 0 ; i < N ; i++ )
	{
		nums[ i ] = 10-i ;
		if ( ht_insert( lh, &nums[ i ] ) != DICT_OK )
		{
			printf( "Failed at %d\n", i ) ;
			exit( 1 ) ;
		}
	}
		
	printf( "Search/delete test\n" ) ;
	i = 7 ;
	ip = INTP( ht_search( lh, &i ) ) ;
	if ( ip == NULL )
		printf( "Search failed\n" ) ;
	else
		if ( ht_delete( lh, ip ) != DICT_OK )
		{
			printf( "Delete failed\n" ) ;
			exit( 0 ) ;
		}
	
	for ( i = 0 ; i < N ; i++ )
		if (( ip = INTP( ht_search( lh, &nums[ i ] ) ) ))
			printf( "%d found\n", nums[ i ] ) ;
		else
			printf( "%d not found\n", nums[ i ] ) ;
		
	ht_iterate( lh , DICT_FROM_START ) ;
	while (( ip = INTP( ht_nextobj( lh ) ) ))
		printf( "Object = %d\n", *ip ) ;
	
	for ( ip = INTP(ht_minimum( lh )) ; ip ; ip = INTP(ht_successor( lh, ip )) )
		printf( "Object = %d\n", *ip ) ;

	for ( ip=INTP(ht_maximum( lh )) ; ip ; ip=INTP(ht_predecessor( lh, ip )) )
		printf( "Object = %d\n", *ip ) ;

	exit( 0 ) ;
}
