include config.mk

DIST_DIR=dist
DIST_NAME=mpkg-0.1
DIST_STAGING=$(DIST_DIR)/$(DIST_NAME)
DIST_TARBALL=$(DIST_NAME).tar.gz

.PHONY: all clean dist install strip

all:
	$(MAKE) -C man all
	$(MAKE) -C src all

clean:
	$(MAKE) -C man clean
	$(MAKE) -C src clean
	$(RM) -f $(DIST_TARBALL)
	$(RM) -rf $(DIST_STAGING)

dist:
	$(MKDIR) -p $(DIST_STAGING) $(DIST_STAGING)/dist \
		$(DIST_STAGING)/include $(DIST_STAGING)/man \
		$(DIST_STAGING)/src
	$(LN) -f CREDITS ChangeLog INSTALL LICENSE $(DIST_STAGING)
	$(LN) -f Makefile.bsd Makefile config.mk.bsd config.mk $(DIST_STAGING)
	$(LN) -f include/*.h $(DIST_STAGING)/include
	$(LN) -f man/Makefile man/Makefile.bsd man/mpkg.1 $(DIST_STAGING)/man
	$(LN) -f src/Makefile src/Makefile.bsd src/*.c $(DIST_STAGING)/src
	$(TAR) -C $(DIST_DIR) -cvf - $(DIST_NAME) | $(GZIP) > \
		$(DIST_TARBALL)

strip:
	$(MAKE) -C src strip

install:
	$(MAKE) -C man install
	$(MAKE) -C src install
