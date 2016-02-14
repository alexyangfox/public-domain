# (PD) 2001 The Bitzi Corporation
# Please see file COPYING or http://bitzi.com/publicdomain 
# for more info.
# 
# $Id: Makefile.am,v 1.14 2004/02/04 08:51:44 mayhemchaos Exp $
#
AUTOMAKE_OPTIONS = foreign

SUBDIRS = libltdl lib src @VORBIS_SUB@ wav image video

cvsclean: distclean
	@echo "additonal cleanup for cvs commits"
	rm -f *~ *.exe
	rm -f $(PACKAGE)-$(VERSION).tar.gz

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL NEWS README README.win32 \
    include/bc_version.h include/bitcollider.h include/bitprint.h include/cache.h include/defs.h \
    include/dirsearch.h include/ed2k_md4.h include/ftuuhash.h include/gui_win32.h include/kztree.h \
    include/list.h include/plugin.h include/sha1.h include/tiger.h include/tigertree.h

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

install-data-local:
	mkdir -p $(libdir)/bitcollider/plugins
	$(INSTALL) -m 0655 plugins/*.bcp $(libdir)/bitcollider/plugins
	mkdir -p $(DESTDIR)$(bindir)
	$(INSTALL) -s -m 0655 src/bitcollider $(DESTDIR)$(bindir)
	mkdir -p $(DESTDIR)$(includedir)/bitcollider
	$(INSTALL) -m 0644 include/bitcollider.h $(DESTDIR)$(includedir)/bitcollider
	$(INSTALL) -m 0644 include/plugin.h $(DESTDIR)$(includedir)/bitcollider
	$(INSTALL) -m 0644 include/sha1.h $(DESTDIR)$(includedir)/bitcollider
	$(INSTALL) -m 0644 include/tiger.h $(DESTDIR)$(includedir)/bitcollider
	$(INSTALL) -m 0644 include/tigertree.h $(DESTDIR)$(includedir)/bitcollider
	$(INSTALL) -m 0644 include/defs.h $(DESTDIR)$(includedir)/bitcollider
	$(INSTALL) -m 0644 include/cache.h $(DESTDIR)$(includedir)/bitcollider

