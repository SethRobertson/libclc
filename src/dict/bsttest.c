/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

static char RCSid[] = "$Id: bsttest.c,v 1.2 2001/07/07 02:58:21 seth Exp $" ;

#ifndef NULL
#define NULL 0
#endif

#include <stdio.h>
#include "bst.h"
#include "clchack.h"


int int_comp( char *p1, char *p2 );

int int_comp( char *p1, char *p2 )
{
	int i1 = *(int *)p1 ;
	int i2 = *(int *)p2 ;

	return( i1 - i2 ) ;
}

void printval( int *ip );
void printval( int *ip )
{
	printf( "%d\n", *ip ) ;
}


#define N 100
int nums[ N ] ;

int main( int argc, char *argv[] );
int main( int argc, char *argv[] )
{
	dict_h bh ;
	int i ;
	int *ip ;
	int j ;
	int flags = DICT_NOFLAGS ;
#ifdef BST_DEBUG
	struct bst_depth d ;
#endif /* BST_DEBUG */

	if ( argc == 2 && argv[1][0] == 'b' )
		flags |= DICT_BALANCED_TREE ;

	bh = bst_create( int_comp, int_comp, flags ) ;

	for ( i = 0 ; i < N ; i++ )
	{
#ifdef notdef
		nums[ i ] = random() % 100 ;
#else
		nums[ i ] = N-i ;
#endif
		if ( bst_insert( bh, &nums[ i ] ) != DICT_OK )
		{
			printf( "Failed at %d\n", i ) ;
			exit( 1 ) ;
		}
	}

   /*
    * If the tree is balanced, this call will make sure that
    *    max_depth <= 2*min_depth
    */
#ifdef BST_DEBUG
   bst_getdepth( bh, &d ) ;

	bst_traverse( bh, BST_INORDER, printval ) ;
	putchar( '\n' ) ;
#endif /* BST_DEBUG */

	printf( "Successor test\n" ) ;
	for ( ip=(int *)bst_minimum( bh ) ; ip ; ip=(int *)bst_successor( bh, ip ) ) 
		printf( "%d ", *ip ) ;
	putchar( '\n' ) ;

	printf( "Predecessor test\n" ) ;
	for ( ip=(int *)bst_maximum( bh ) ;ip; ip=(int *)bst_predecessor( bh, ip ) ) 
		printf( "%d ", *ip ) ;
	putchar( '\n' ) ;

	printf( "Search/delete test\n" ) ;
	i = 7 ;
	ip = (int *) bst_search( bh, &i ) ;
	if ( ip == NULL )
		printf( "Search failed\n" ) ;
	else
		if ( bst_delete( bh, ip ) != DICT_OK )
		{
			printf( "Delete failed\n" ) ;
			exit( 0 ) ;
		}

	printf( "Successor test 2\n" ) ;
	for ( ip=(int *)bst_minimum( bh ) ; ip ; ip=(int *)bst_successor( bh, ip ) ) 
		printf( "%d ", *ip ) ;
	putchar( '\n' ) ;

	printf( "Predecessor test 2\n" ) ;
	for ( ip=(int *)bst_maximum( bh ) ;ip; ip=(int *)bst_predecessor( bh, ip ) ) 
		printf( "%d ", *ip ) ;
	putchar( '\n' ) ;
	exit( 0 ) ;
}
