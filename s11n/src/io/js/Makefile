INSTALL_LIBEXECS_DEST = $(prefix)/lib/s11n

include toc2.make

HEADERS = \
	js_serializer.hpp

SOURCES = js_serializer.cpp

package.install.headers = $(HEADERS)
package.install.headers.dest = $(prefix)/include/s11n.net/s11n/io
package.dist_files += $(HEADERS) $(SOURCES)


symlink-files.list = $(package.install.headers)
symlink-files.dest = $(toc2.top_srcdir)/include/s11n.net/s11n/io
include $(toc2.dirs.makefiles)/symlink-files.make

#SHARED_LIBS = js_serializer
#INSTALL_LIBEXECS_DEST = $(prefix)/lib/s11n
#js_serializer_so_OBJECTS = js_serializer.o
#js_serializer_so_LDADD = -lexpat
#js_serializer_so_VERSION = $(package.version)
#include $(TOC_MAKESDIR)/SHARED_LIBS.make


all: symlink-files js_serializer.o
# $(js_serializer_so_OBJECTS)
# SHARED_LIBS
