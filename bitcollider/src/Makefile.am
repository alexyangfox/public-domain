# (PD) 2001 The Bitzi Corporation
# Please see file COPYING or http://bitzi.com/publicdomain 
# for more info.
# 
# $Id: Makefile.am,v 1.9 2001/10/10 23:25:48 mayhemchaos Exp $
#
INCLUDES = $(INCLTDL) -I$(top_srcdir)/lib -I$(top_srcdir)/include

noinst_PROGRAMS = bitcollider
bitcollider_SOURCES = bitcollider.c 
bitcollider_LDFLAGS = -export-dynamic 
bitcollider_LDADD = $(top_builddir)/lib/.libs/libbitcollider.a @BDB_LIBS@ @LIBS@ $(LIBLTDL)
