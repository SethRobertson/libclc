/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */


#define NULL 0

#include <stdio.h>
#include <stdlib.h>
#include "bst.h"
#include "clchack.h"


int int_comp(void *p1v, void *p2v);
int int_comp(void *p1v, void *p2v)
{
  char *p1=(char *)p1v;
  char *p2=(char *)p2v;
	int i1 = *(int *)p1 ;
	int i2 = *(int *)p2 ;

	return( i1 - i2 ) ;
}

#define N 1000
int nums[ N ] ;

int main(int argc, char **argv)
{
	dict_h bh ;
	int i ;
	int *ip ;
	int j ;
	int flags = DICT_NOFLAGS ;
#ifdef BST_DEBUG
	struct bst_depth d ;
#endif /* BST_DEBUG */

	if ( argc == 3 && argv[1][0] == 'b' )
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

#ifdef BST_DEBUG
	bst_getdepth( bh, &d ) ;
	printf( "min tree depth=%d, max tree depth=%d\n",
		d.depth_min, d.depth_max ) ;
#endif /* BST_DEBUG */

	for ( j = atoi( argv[ argc-1 ] ) ; j ; j-- )
	{
		i = 1 ;
		ip = (int *) bst_search( bh, &i ) ;
		if ( ip == NULL )
		{
			printf( "Search failed at %d\n", j ) ;
			exit( 0 ) ;
		}
	}

	exit( 0 ) ;
}
