#!/usr/bin/make -f

include toc2.make

package.subdirs_SECOND = client
ifeq (1,$(s11n_CONFIG_ENABLE_PLUGINS))
      package.subdirs_FIRST += plugin
      package.subdirs_SECOND += unit
else
subdir-plugin:
endif
package.subdirs_FIRST += s11n io proxy lite nodeutil

########################################################################
# Reminder: set package.subdirs so that cleanup rules will work:
package.subdirs = $(package.subdirs_FIRST) $(package.subdirs_SECOND)
subdir-s11n: subdir-plugin
subdir-io: subdir-s11n
subdir-proxy: subdir-s11n
subdir-lite: subdir-io subdir-proxy
subdir-addons subdir-unit subdir-client subdir-nodeutil: subdir-lite
subdirs: $(addprefix subdir-,$(package.subdirs))

package.dist_files += $(wildcard in.*) test.cpp
# Makefile.for.generated.src.tree

########################################################################
# Build DLL...
c-dlls.list := libs11n
libs11n.DLL.VERSION := $(package.version)
libs11n.DLL.OBJECTS = \
	s11n/base64enc.o \
	s11n/base64dec.o \
	s11n/exception.o \
	s11n/mutex.o \
	s11n/s11n.o \
	s11n/s11n_node.o \
	s11n/vappendf.o \
	lite/s11nlite.o	\
	lite/serializable_base.o \
	$(shell find io -type f -name '*.o')
libs11n.DLL.LDADD = $(LIBS11N_LDADD) 
ifeq (1,$(s11n_CONFIG_ENABLE_PLUGINS))
  libs11n.DLL.OBJECTS += plugin/plugin.o plugin/path_finder.o
endif
include $(toc2.dirs.makefiles)/c-dlls.make
$(libs11n.DLL.OBJECTS):
	@true # huge kludge for parallel builds
# end DLL rules
########################################################################
#$(libs11n.DLL): subdirs
$(libs11n.DLL): | $(addprefix subdir-,$(package.subdirs_FIRST))
$(addprefix subdir-,$(package.subdirs_SECOND)): | $(libs11n.DLL)

########################################################################
# Reminder: Linking against a static s11n doesn't work because factory
# registrations done via the backwards approach we take are never
# triggered when linked this way :(
########################################################################

########################################################################
# Reminder: the strange 'all' target ordering here is necessary
# so that the DLL (built in ./) is in place for the client code
# which uses it.
########################################################################
#all: $(addprefix subdir-,$(package.subdirs_FIRST)) \
#	$(libs11n.DLL) \
#	$(addprefix subdir-,$(package.subdirs_SECOND))
#all: $(addprefix subdir-,$(package.subdirs_FIRST))
#all: $(libs11n.DLL) $(addprefix subdir-,$(package.subdirs_SECOND))
#all: subdirs $(libs11n.DLL)
all: $(libs11n.DLL)
