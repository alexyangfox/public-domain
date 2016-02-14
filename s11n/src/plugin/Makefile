#!/usr/bin/make -f

include toc2.make

SOURCES = \
	path_finder.cpp \
	plugin.cpp

HEADERS = \
	path_finder.hpp \
	plugin.hpp \
	plugin_config.hpp

package.dist_files += \
	$(SOURCES) \
	path_finder.hpp \
	plugin.hpp \
	plugin.dl.cpp \
	plugin.win32.cpp \
	plugin_config.hpp.at

package.distclean_files += plugin_config.hpp

OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))

HEADERS_RELPATH = include/s11n.net/s11n/plugin

package.install.headers = $(HEADERS)
package.install.headers.dest = $(prefix)/$(HEADERS_RELPATH)

symlink-files.list := $(package.install.headers)
symlink-files.dest := $(toc2.top_srcdir)/$(HEADERS_RELPATH)
include $(toc2.dirs.makefiles)/symlink-files.make

all: symlink-files $(OBJECTS)
