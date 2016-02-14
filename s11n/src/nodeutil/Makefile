include toc2.make

HEADERS = node_tree.hpp
package.dist_files += $(HEADERS)
package.install.headers = $(HEADERS)
package.install.headers.dest = $(prefix)/$(HEADER_PATH)
symlink-files.list = $(package.install.headers)
HEADER_PATH = include/s11n.net/s11n/nodeutil
symlink-files.dest = $(toc2.top_srcdir)/$(HEADER_PATH)
include $(toc2.dirs.makefiles)/symlink-files.make

all: symlink-files
