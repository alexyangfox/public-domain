#!/usr/bin/make -f
########################################################################
#
# This is a platform-neutral[ish] Makefile for libs11n. It requires GNU
# Make on a Unix-like platform hosting gcc. (How's that for platform
# neutral?)
#
# This makefile does nothing fancy like full depency tracking. It is
# mainly provided to demonstrate what sources go to what parts, to
# assist users who want to get it running under their own build
# environments.
#
# libs11n home page: http://s11n.net
#
########################################################################

PACKAGE.NAME = libs11n
PACKAGE.VERSION = 1.3.0
PACKAGE.URL = http://s11n.net

include shake-n-make.make

########################################################################
# You only need to change INCLUDES if you enable some external support,
# like libzfstream (see s11n_config.hpp).
LIBS11N_INCLUDES_DIR = ./include
INCLUDES += -I$(LIBS11N_INCLUDES_DIR)

########################################################################
# CXXFLAGS = flags for the C++ compiler.
CXXFLAGS = -Wall -Werror
ifeq (,$(shell uname -a | grep -i cygw))
# Per request from a user: cygwin/mingw doesn't support -fPIC, apparently.
# Debian requires it from compliant apps.
CXXFLAGS += -fPIC
endif


########################################################################
# SOURCES_CORE = the code for the core library.
SOURCES_CORE = \
	exception.cpp \
	s11n.cpp \
	s11n_node.cpp \
	mutex.cpp \
	vappendf.cpp
# Note that the mutex.PLATFORM.cpp files may be included by mutex.cpp,
# depending on the contents of s11n_config.hpp.

########################################################################
## If you know that s11n has a DLL handler for your platform, set
## the related #defines in the config headers:
##  <s11n.net/s11n/plugin/plugin_config.hpp>
##  <s11n.net/s11n/s11n_config.hpp>
## and link with your appropriate DLL loader library (e.g., libdl or
## libltdl).
##
## By default a no-op handler is used except when WIN32 is defined,
## in which case a LoadModule()-based implementation is used.
## 
## Do not add plugin.PLATFORM.cpp to SOURCES_PLUGIN, as it is included
## directly by plugin.cpp depending on defines from *_config.hpp.
## If you completely disable plugins support (s11n_config.hpp) you can
## unset SOURCES_PLUGIN and build without plugins support.
SOURCES_PLUGIN = \
	path_finder.cpp \
	plugin.cpp


########################################################################
# SOURCES_IO = the code for the base i/o code
SOURCES_IO = \
	data_node_io.cpp \
	strtool.cpp \
	url.cpp

########################################################################
# SOURCES_IO_SERIALIZERS = the code for the i/o handlers
SOURCES_IO_SERIALIZERS = \
	compact.flex.cpp \
	compact_serializer.cpp \
	funtxt.flex.cpp \
	funtxt_serializer.cpp \
	funxml.flex.cpp \
	funxml_serializer.cpp \
	parens.flex.cpp \
	parens_serializer.cpp \
	simplexml.flex.cpp \
	simplexml_serializer.cpp \
	wesnoth.flex.cpp \
	wesnoth_serializer.cpp

########################################################################
# ONLY add expat_serializer.cpp if you have libexpat:
#   SOURCES_IO_SERIALIZERS += expat_serializer.cpp
#   libs11n.DLL.LDFLAGS += -lexpat
# If enabled, you must link your project against expat and add any
# necessary INCLUDES when compiling THIS code.
########################################################################

########################################################################
# SOURCES_LITE = s11nlite
SOURCES_LITE = \
	s11nlite.cpp


########################################################################
# LIB_SOURCES lists the .cpp files making up the core library, the i/o
# handlers, and s11nlite:
LIB_SOURCES = $(SOURCES_PLUGIN) $(SOURCES_CORE) \
		$(SOURCES_IO) $(SOURCES_IO_SERIALIZERS) \
		$(SOURCES_LITE)
LIB_OBJECTS = $(patsubst %.cpp,%.o,$(LIB_SOURCES))
LIB_OBJECTS: $(LIB_OBJECTS)
CLEAN_FILES = *.o

########################################################################
# build shared libs11n
libs11n.DLL.OBJECTS = $(LIB_OBJECTS)
# libs11n.DLL.LDFLAGS = -export-all-symbols
#  -export-all-symbols:  http://www.redhat.com/docs/manuals/enterprise/RHEL-4-Manual/gnu-linker/win32.html
$(call ShakeNMake.CALL.RULES.DLLS,libs11n)
all: libs11n.DLL

########################################################################
# build s11nconvert...
#  s11nconvert.BIN.MONOLITHIC=1 == compile in all libs11n objects
#  s11nconvert.BIN.MONOLITHIC=0 == link to libs11n.dll (doesn't yet work on Cygwin, MinGW)
ifeq (1,$(ShakeNMake.SMELLS.LIKE.WINDOWS))
s11nconvert.BIN.MONOLITHIC = 1
else
s11nconvert.BIN.MONOLITHIC ?= 0
# s11nconvert.BIN.LDFLAGS += -lstdc++
endif
s11nconvert.BIN.OBJECTS = main.o argv_parser.o
s11nconvert.BIN.LDFLAGS = -export-dynamic
ifeq (1,$(s11nconvert.BIN.MONOLITHIC))
s11nconvert.BIN.OBJECTS += $(LIB_OBJECTS)
else
s11nconvert.BIN.LDFLAGS += -L. -ls11n
endif
$(call ShakeNMake.CALL.RULES.BINS,s11nconvert)
$(s11nconvert.BIN): libs11n.DLL
all: s11nconvert.BIN
########################################################################


########################################################################
# build a statically-linked test/demo app...
#test.BIN = test
#test.BIN.OBJECTS = test.o $(LIB_OBJECTS)
# test.BIN.LDFLAGS = -export-dynamic -lstdc++
#$(call ShakeNMake.CALL.RULES.BINS,test)
#all: test.BIN


########################################################################
# [un]install rules
INSTALL.BINS = $(s11nconvert.BIN)
INSTALL.BINS.DEST = $(prefix)/bin
INSTALL.LIBS = $(libs11n.DLL)
INSTALL.LIBS.DEST = $(prefix)/lib
INSTALL.HEADERS.DEST = $(prefix)/include/s11n.net
INSTALL.HEADERS = include/s11n.net/s11n
install-HEADERS-kludge:
	@test -e $(INSTALL.HEADERS.DEST)/s11n || exit 0; \
	echo "Removing old s11n headers..."; \
	/bin/rm -fr $(INSTALL.HEADERS.DEST)/s11n
install-HEADERS: install-HEADERS-kludge
$(call ShakeNMake.CALL.RULES.INSTALL,BINS LIBS HEADERS)

########################################################################
# 
all:
	@echo "Don't forget: $(PACKAGE.URL)"
