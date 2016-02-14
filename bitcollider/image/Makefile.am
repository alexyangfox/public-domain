# (PD) 2001 The Bitzi Corporation
# Please see file COPYING or http://bitzi.com/publicdomain 
# for more info.
# 
# $Id: Makefile.am,v 1.4 2004/02/04 08:51:45 mayhemchaos Exp $
#
AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/.. -I$(top_srcdir)/ver -I$(top_srcdir)/include
lib_LTLIBRARIES = image.la
image_la_SOURCES = image.c 
image_la_LDFLAGS = -module -avoid-version

all:
	mkdir -p $(top_srcdir)/plugins
	rm -f $(top_srcdir)/plugins/image.bcp
	ln -s $(top_srcdir)/image/.libs/image.so $(top_srcdir)/plugins/image.bcp

install-exec-am:

clean distclean:
	rm -f $(top_srcdir)/plugins/image.bcp
