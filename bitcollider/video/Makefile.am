# (PD) 2001 The Bitzi Corporation
# Please see file COPYING or http://bitzi.com/publicdomain 
# for more info.
# 
# $Id: Makefile.am,v 1.5 2004/02/04 08:51:45 mayhemchaos Exp $
#
AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/.. -I$(top_srcdir)/ver -I$(top_srcdir)/include
lib_LTLIBRARIES = video.la
video_la_SOURCES = video.c mpeg.c quicktime.c avi.c video.h
video_la_LDFLAGS = -module -avoid-version

all:
	mkdir -p $(top_srcdir)/plugins
	rm -f $(top_srcdir)/plugins/video.bcp
	ln -s $(top_srcdir)/video/.libs/video.so $(top_srcdir)/plugins/video.bcp

install-exec-am:

clean distclean:
	rm -f $(top_srcdir)/plugins/video.bcp
