######################################################################
#
# CLC library
#
######################################################################

SUBDIR=src include
TOPDIR=.
CURDIR=.

include $(TOPDIR)/Make.include

ifneq ($(strip $(PUBINST)),)
INSTALL_DIR=/usr/local/lib
H_INSTALL_DIR=/usr/local/include
else
INSTALL_DIR=$(SOS)/lib/lib.$(ARCH_DIR)
H_INSTALL_DIR=$(SOS)/include
endif


depend: depends

meta-makeinstall:: install
