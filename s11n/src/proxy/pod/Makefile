#!/usr/bin/make -f

include toc2.make

HEADERS = $(wildcard *.hpp)

package.dist_files += \
	$(HEADERS)

symlink-files.list = $(HEADERS)
HEADER_DIR = include/s11n.net/s11n/proxy/pod
symlink-files.dest = $(toc2.top_srcdir)/$(HEADER_DIR)
include $(toc2.dirs.makefiles)/symlink-files.make

package.install.headers = $(HEADERS)
package.install.headers.dest = $(prefix)/$(HEADER_DIR)
all: symlink-files


