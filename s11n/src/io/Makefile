include toc2.make

ifeq (1,1)
  package.subdirs = compact funtxt funxml js parens simplexml wesnoth
  # package.subdirs += js 
  ifeq (1,$(HAVE_LIBEXPAT))
      package.subdirs += expat
  endif
  subdirs: $(addprefix subdir-,$(package.subdirs))
endif

SOURCES = \
	data_node_io.cpp \
	strtool.cpp \
	url.cpp

HEADERS = \
	data_node_format.hpp \
	data_node_io.hpp \
	FlexLexer.hpp \
	reg_serializer.hpp \
	serializers.hpp \
	strtool.hpp \
	url.hpp

HEADERS_NOINST = \
	LexerTemplate.hpp

OBJECTS = data_node_io.o strtool.o url.o


IOINCLUDES_PATH = include/s11n.net/s11n/io
package.install.headers = $(HEADERS)
package.install.headers.dest = $(prefix)/$(IOINCLUDES_PATH)

package.dist_files += $(SOURCES) $(HEADERS) $(HEADERS_NOINST) \
	serializer_lex.make


symlink-files.list = $(package.install.headers)
symlink-files.dest = $(toc2.top_srcdir)/$(IOINCLUDES_PATH)
include $(toc2.dirs.makefiles)/symlink-files.make

#$(OBJECTS): symlink-files # causes always-rebuild
all: symlink-files $(OBJECTS) subdirs
