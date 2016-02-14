include toc2.make

package.dist_files += \
	base64enc.c \
	base64dec.c \
	exception.cpp \
	s11n.cpp \
	s11n_node.cpp \
	vappendf.cpp \
	mutex.cpp \
	mutex.win32.cpp \
	mutex.pthreads.cpp \
	mutex.gnupth.cpp


HEADERS = \
	abstract_creator.hpp \
	algo.hpp \
	algo.tpp \
	base64.hpp \
	base64enc.h \
	base64enc.hpp \
	base64dec.h \
	base64dec.hpp \
	classload.hpp \
	classload.tpp \
	exception.hpp \
	export.hpp \
	factory.hpp \
	factory_reg.hpp \
	functional.hpp \
	memory.hpp \
	memory.tpp \
	phoenix.hpp \
	refcount.hpp \
	reg_s11n_traits.hpp \
	s11n_debuggering_macros.hpp \
	s11n.hpp \
	s11n_node.hpp \
	s11n_node_query.hpp \
	serialize.hpp \
	serialize.tpp \
	tags.hpp \
	traits.hpp \
	type_traits.hpp \
	variant.hpp \
	vappendf.hpp \
	mutex.hpp


OBJECTS = $(addsuffix .o,\
	exception \
	mutex \
	s11n \
	s11n_node \
	vappendf \
	base64enc \
	base64dec \
	)

package.dist_files += $(wildcard *.at) $(HEADERS)
package.distclean_files += s11n_config.hpp
HEADERS_RELPATH = include/s11n.net/s11n
package.install.headers = $(HEADERS) s11n_config.hpp
package.install.headers.dest = $(prefix)/$(HEADERS_RELPATH)
symlink-files.list := $(package.install.headers)
symlink-files.dest := $(toc2.top_srcdir)/$(HEADERS_RELPATH)
include $(toc2.dirs.makefiles)/symlink-files.make

#$(OBJECTS): symlink-files
#%.o: symlink-files
#all: $(OBJECTS)
########################################################################
# Reminder: set package.subdirs so that cleanup rules will work.
# And make sure subdirs are built AFTER everything else.
package.subdirs := 0x
all: symlink-files $(OBJECTS) $(addprefix subdir-,$(package.subdirs))
