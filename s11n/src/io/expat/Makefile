include toc2.make


HEADERS = \
	expat_serializer.hpp

SOURCES = expat_serializer.cpp

HEADER_PATH_REL := include/s11n.net/s11n/io
package.install.headers = $(HEADERS)
package.install.headers.dest = $(prefix)/$(HEADER_PATH_REL)
package.dist_files += $(HEADERS) $(SOURCES)


symlink-files.list = $(package.install.headers)
symlink-files.dest = $(toc2.top_srcdir)/$(HEADER_PATH_REL)
include $(toc2.dirs.makefiles)/symlink-files.make

#SHARED_LIBS = expat_serializer
#INSTALL_LIBEXECS_DEST = $(prefix)/lib/s11n
#expat_serializer_so_OBJECTS = expat_serializer.o
#expat_serializer_so_LDADD = -lexpat
#expat_serializer_so_VERSION = $(package.version)
#include $(TOC_MAKESDIR)/SHARED_LIBS.make

all: symlink-files expat_serializer.o 
# $(expat_serializer_so_OBJECTS)
# SHARED_LIBS
