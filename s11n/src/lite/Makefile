include toc2.make

SOURCES = s11nlite.cpp \
	serializable_base.cpp


HEADERS = client_api.hpp \
	micro_api.hpp \
	s11nlite.hpp \
	simple_config.hpp \
	serializable_base.hpp

OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))
objects: $(OBJECTS)

package.install.headers = $(HEADERS)


# package.dist_files += $(sort $(wildcard *.cpp) $(wildcard *.h) $(wildcard *.make))
package.dist_files += $(SOURCES) $(HEADERS)

symlink-files.list = $(package.install.headers)
symlink-files.dest = $(toc2.top_srcdir)/include/s11n.net/s11n
include $(toc2.dirs.makefiles)/symlink-files.make
$(OBJECTS): | symlink-files
all: $(OBJECTS)
