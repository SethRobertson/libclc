/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

static char RCSid[] = "$Id: dlltest2.c,v 1.4 2001/11/05 19:31:45 seth Exp $" ;

struct foo
{
  int compare;
  int counter;
  char *text;
};


#ifndef NULL
#define NULL 0
#endif

#include <stdio.h>
#include <stdlib.h>
#include "dll.h"
#include "clchack.h"

void baz(dict_h lh);

static int int_comp(void *p1v, void *p2v)
{
  struct foo *p1=(struct foo *)p1v;
  struct foo *p2=(struct foo *)p2v;
  return(p1->compare - p2->compare);
}


static int int_kcomp(void *p1v, void *p2v)
{
  int *p1=(int *)p1v;
  struct foo *p2=(struct foo *)p2v;
  return(*p1 - p2->compare);
}


#define N 14
struct foo foos[N];

int main(void)
{
	dict_h lh ;

	printf("---- ordered test ---\n");
	lh = dll_create( int_comp, int_kcomp, 0 ) ;
	baz(lh);
	printf("------\n");
	printf("---- unordered test ---\n");
	lh = dll_create( NULL, NULL, DICT_UNORDERED ) ;
	baz(lh);
	printf("------\n");
	printf("---- unordered unique test ---\n");
	lh = dll_create( int_comp, int_kcomp, DICT_UNORDERED|DICT_UNIQUE_KEYS ) ;
	baz(lh);
	printf("------\n");

	exit( 0 ) ;
}


void baz(dict_h lh)
{
	int cntr;
	struct foo *bar;
	for(cntr=0;cntr<N;cntr++)
	  {
	    foos[cntr].counter = cntr;
	    foos[cntr].compare = 100;
	    foos[cntr].text = "insert";
	  }

	dll_insert(lh, &foos[0]);
	dll_insert(lh, &foos[1]);
	dll_insert(lh, &foos[2]);
	cntr = 3; foos[cntr].text = "append"; dll_append(lh, &foos[cntr]);
	cntr = 4; foos[cntr].text = "append"; dll_append(lh, &foos[cntr]);
	cntr = 5; foos[cntr].text = "append"; dll_append(lh, &foos[cntr]);
	cntr = 6; foos[cntr].compare = 10; foos[cntr].text = "append"; dll_append(lh, &foos[cntr]);
	cntr = 7; foos[cntr].compare = 10; dll_insert(lh, &foos[cntr]);
	cntr = 8; foos[cntr].compare = 200; dll_insert(lh, &foos[cntr]);
	cntr = 9; foos[cntr].compare = 200; dll_insert(lh, &foos[cntr]);
	dll_insert(lh, &foos[10]);
	cntr = 11; foos[cntr].text = "append"; dll_append(lh, &foos[cntr]);
	cntr = 12; foos[cntr].compare = 10; foos[cntr].text = "append"; dll_append(lh, &foos[cntr]);
	cntr = 13; foos[cntr].compare = 200; foos[cntr].text = "append"; dll_append(lh, &foos[cntr]);


	for(bar = (struct foo *)dll_minimum(lh);bar;bar = (struct foo *)dll_successor(lh,bar))
	  {
	    printf("%02d %3d %s\n",bar->counter,bar->compare,bar->text);
	  }
}
