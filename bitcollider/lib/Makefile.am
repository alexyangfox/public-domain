# (PD) 2001 The Bitzi Corporation
# Please see file COPYING or http://bitzi.com/publicdomain 
# for more info.
# 
# $Id: Makefile.am,v 1.18 2004/02/03 01:41:08 mayhemchaos Exp $
#
AUTOMAKE_OPTIONS = foreign

INCLUDES = $(INCLTDL) -I$(top_srcdir)/.. -I$(top_srcdir)/ver -I$(top_srcdir)/include
lib_LTLIBRARIES = libbitcollider.la
libbitcollider_la_SOURCES = main.c sha1.c tiger.c tigertree.c sboxes.c browser.c mp3.c id3.c plugin_man.c plugin_man.h dirsearch.c bitprint.c md5.c md5.h cache.c ed2k_md4.c ftuuhash.c kztree.c
libbitcollider_la_LDFLAGS = -version-info 1:0:0
libbitcollider_la_LIBADD = $(BDB_LIBS)

noinst_HEADERS = browser.h mp3.h id3.h
