# (PD) 2001 The Bitzi Corporation
# Please see file COPYING or http://bitzi.com/publicdomain 
# for more info.
# 
# $Id: Makefile.am,v 1.7 2004/02/04 08:51:45 mayhemchaos Exp $
#
AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/.. -I$(top_srcdir)/ver -I$(top_srcdir)/include
lib_LTLIBRARIES = vorbisplugin.la
vorbisplugin_la_SOURCES = vorbis.c vorbis.h
vorbisplugin_la_LDFLAGS = -module -avoid-version
vorbisplugin_la_LIBADD = @VORBIS_LIBS@

noinst_HEADERS = vorbis.h

all:
	mkdir -p $(top_srcdir)/plugins
	rm -f $(top_srcdir)/plugins/vorbis.bcp
	ln -s $(top_srcdir)/vorbis/.libs/vorbisplugin.so $(top_srcdir)/plugins/vorbis.bcp

install-exec-am:

clean distclean:
	rm -f $(top_srcdir)/plugins/vorbis.bcp
