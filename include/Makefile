######################################################################
##
## plex makefile
##
######################################################################

INCLUDES=timer.h timemacros.h pq.h hpq.h bst.h ht.h dll.h dict.h fsma.h pset.h misc.h ftwx.h env.h xlog.h str.h sio.h

TOPDIR=..
CURDIR=include

ifneq ($(strip $(PUBINST)),)
INSTALL_DIR=/usr/local/lib
H_INSTALL_DIR=/usr/local/include
else
INSTALL_DIR=$(SOS)/lib/lib.$(ARCH_DIR)
H_INSTALL_DIR=$(SOS)/include
endif

include $(TOPDIR)/Make.include

install:: $(INCLUDES)
	if test -d $(H_INSTALL_DIR); then \
		: ; \
	else \
		mkdir -p $(H_INSTALL_DIR) ; \
	fi
	for f in $^; do if cmp $$f ${H_INSTALL_DIR}/`basename $$f` > /dev/null 2>&1; then :; else $(INSTALL) $(INSTALL_OTHERFLAGS) $$f $(H_INSTALL_DIR); fi; done
