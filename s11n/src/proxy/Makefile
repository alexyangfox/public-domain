#!/usr/bin/make -f

package.subdirs = pod std
include toc2.make

HEADERS = $(wildcard *.hpp *.tpp)

package.dist_files += \
	$(HEADERS) \
	createRegSerTraits.sh

symlink-files.list = $(HEADERS)
HEADER_DIR = include/s11n.net/s11n/proxy
symlink-files.dest = $(toc2.top_srcdir)/$(HEADER_DIR)
include $(toc2.dirs.makefiles)/symlink-files.make

package.install.headers = $(HEADERS)
package.install.headers.dest = $(prefix)/$(HEADER_DIR)
all: symlink-files subdirs
