######################################################################
#
# CLC utility library
#
######################################################################

SRC=	dll.c ht.c bst.c rbt.c dict.c
MAN3=	bst.3 dict.3 dll.3 ht.3

OBJ=${SRC:.c=.o}

######################################################################
##
## Private definitions
##
PRIVATE_INCLUDES= -I$(TOPDIR)/include -I$(TOPDIR)/$(BUILD_UPDIR)/include
PRIVATE_LIBS=${LIBS} ../libfsma.a
LIBS=../libdict.a
SIMPLE_PROGS=dlltest htest bsttest bstcomp dlltest2


ifneq ($(strip $(PUBINST)),)
INSTALL_LIB_DIR=/usr/local/lib
INSTALL_MAN3_DIR=/usr/local/man/man3
else
INSTALL_LIB_DIR=$(SOS)/lib/lib.$(ARCH_DIR)
INSTALL_MAN3_DIR=$(SOS)/man/man3
endif


TOPDIR=../..
CURDIR=src/dict

#VPATH=.:
include $(TOPDIR)/Make.include
VPATH += .:../../$(TOPDIR)/$(CURDIR)
PRIVATE_DEFINES += -DFAST_ACTIONS -DSAFE_ITERATE



$(LIBS): $(OBJ) 
	$(AR) $(ARFLAGS) $@ $?
	$(RANLIB) $@

depend: depends

install:: $(LIBS)
	$(INSTALL) $(INSTALL_OTHERFLAGS) $(LIBS) $(INSTALL_LIB_DIR)
	$(RANLIB) $(INSTALL_LIB_DIR)/libdict.a

install:: $(MAN3)
	$(INSTALL) $(INSTALL_OTHERFLAGS) $^ $(INSTALL_MAN3_DIR)
