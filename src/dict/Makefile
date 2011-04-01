######################################################################
# $Id: Makefile,v 1.6 2002/05/16 16:02:20 dupuy Exp $
#
# ++Copyright BAKA++
#
# Copyright © 2001-2011 The Authors. All rights reserved.
#
# This source code is licensed to you under the terms of the file
# LICENSE.TXT in this release for further details.
#
# Send e-mail to <projectbaka@baka.org> for further information.
#
# - -Copyright BAKA- -
#
# libbk Makefile
#
BK_SIMPLE_INTERNALPROGS=dlltest htest bsttest bstcomp dlltest2

BK_LARGE_LIB=../libdict$(LIBEXT)
BK_LARGE_LIBSRC=dll.c ht.c bst.c rbt.c dict.c
BK_MAN=bst.3 dict.3 dll.3 ht.3

LOCAL_DEFINES=-DFAST_ACTIONS -DSAFE_ITERATE
LOCAL_LDS=-L..
LOCAL_LIBS=-ldict -lfsma

LT_LDLIBS=-lfsma

GROUPTOP=../..
GROUPSUBDIR=src/dict
##################################################
## BEGIN BKSTANDARD MAKEFILE
-include ./Make.preinclude
-include $(GROUPTOP)/Make.preinclude
-include $(GROUPTOP)/$(PKGTOP)/Make.preinclude
include $(GROUPTOP)/$(PKGTOP)/bkmk/Make.include
-include $(GROUPTOP)/$(PKGTOP)/Make.include
-include $(GROUPTOP)/Make.include
-include ./Make.include
## END BKSTANDARD MAKEFILE
##################################################
