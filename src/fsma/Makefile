######################################################################
#
# PLEX utility library
#
######################################################################

SRC=	fsma.c
MAN3=	fsma.3

OBJ=${SRC:.c=.o}

######################################################################
##
## Private definitions
##
PRIVATE_INCLUDES= -I$(TOPDIR)/include -I$(TOPDIR)/$(BUILD_UPDIR)/include
LIBS=../libfsma.a

TOPDIR=../..
CURDIR=src/fsma

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
	mkdir -p $(INSTALL_LIB_DIR)
	$(INSTALL) $(INSTALL_OTHERFLAGS) $(LIBS) $(INSTALL_LIB_DIR)
	$(RANLIB) $(INSTALL_LIB_DIR)/libfsma.a

install:: $(MAN3)
	mkdir -p $(INSTALL_MAN3_DIR)
	$(INSTALL) $(INSTALL_OTHERFLAGS) $^ $(INSTALL_MAN3_DIR)
