######################################################################
#
# PLEX utility library
#
######################################################################

SRC=	strutil.c strprint.c strparse.c strs.c ss_rk.c ss_kmp.c ss_bf.c ss_sbm.c ss_bmh.c ss_so.c
MAN3=	strparse.3 strprint.3 strs.3 strutil.3

OBJ=${SRC:.c=.o}

######################################################################
##
## Private definitions
##
PRIVATE_INCLUDES= -I$(TOPDIR)/include -I$(TOPDIR)/$(BUILD_UPDIR)/include
LIBS=../libstr.a

TOPDIR=../..
CURDIR=src/str

ifneq ($(strip $(PUBINST)),)
INSTALL_LIB_DIR=/usr/local/lib
INSTALL_MAN3_DIR=/usr/local/man/man3
else
INSTALL_LIB_DIR=$(SOS)/lib/lib.$(ARCH_DIR)
INSTALL_MAN3_DIR=$(SOS)/man/man3
endif

#VPATH=.:
include $(TOPDIR)/Make.include
VPATH += .:../../$(TOPDIR)/$(CURDIR)

$(LIBS): $(OBJ)
	$(AR) $(ARFLAGS) $@ $?
	$(RANLIB) $@

depend: depends

install:: $(LIBS)
	$(INSTALL) $(INSTALL_OTHERFLAGS) $(LIBS) $(INSTALL_LIB_DIR)
	$(RANLIB) $(INSTALL_LIB_DIR)/libstr.a

install:: $(MAN3)
	$(INSTALL) $(INSTALL_OTHERFLAGS) $^ $(INSTALL_MAN3_DIR)
