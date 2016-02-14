include toc2.make

HEADERS = ohex.hpp
package.dist_files += $(HEADERS)
HEADERS_RELPATH = include/s11n.net/s11n/0x
package.install.headers = $(HEADERS)
package.install.headers.dest = $(prefix)/$(HEADERS_RELPATH)
symlink-files.list := $(package.install.headers)
symlink-files.dest := $(toc2.top_srcdir)/$(HEADERS_RELPATH)
include $(toc2.dirs.makefiles)/symlink-files.make

all: symlink-files
