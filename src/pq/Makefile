######################################################################
# $Id: Makefile,v 1.6 2005/02/07 18:17:14 dupuy Exp $
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
# libpqueue convenience library Makefile
#

# NOTE: this does *NOT* need to be built as a separate shared library;
# it is compiled PIC for sharing, and is incorporated into the shared libbk.

BK_LARGE_LIB=../libpqueue.a
BK_LARGE_LIBSRC=pq.c hpq.c
BK_MAN=pq.3

GROUPTOP=../..
GROUPSUBDIR=src/pq
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
