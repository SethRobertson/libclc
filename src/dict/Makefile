######################################################################
# $Id: Makefile,v 1.2 2001/06/18 20:47:28 seth Exp $
#
# ++Copyright LIBBK++
#
# Copyright (c) 2001 The Authors.  All rights reserved.
#
# This source code is licensed to you under the terms of the file
# LICENSE.TXT in this release for further details.
#
# Mail <projectbaka@baka.org> for further information
#
# --Copyright LIBBK--
#
# libbk Makefile
#
BK_SIMPLE_PROGS=dlltest htest bsttest bstcomp dlltest2

BK_LARGE_LIB=../libdict.a
BK_LARGE_LIBSRC=dll.c ht.c bst.c rbt.c dict.c
BK_MAN=bst.3 dict.3 dll.3 ht.3

LOCAL_DEFINES=-DFAST_ACTIONS -DSAFE_ITERATE
LOCAL_LDS=-L..
LOCAL_LIBS=-ldict -lfsma

GROUPTOP=../..
GROUPSUBDIR=src/dict
##################################################
## BEGIN BKSTANDARD MAKEFILE
-include $(GROUPTOP)/Make.preinclude
include $(GROUPTOP)/$(PKGTOP)/bkmk/Make.include
-include $(GROUPTOP)/$(PKGTOP)/Make.include
-include $(GROUPTOP)/Make.include
-include ./Make.include
## END BKSTANDARD MAKEFILE
##################################################
