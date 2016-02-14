default: all
include toc2.make

package.dist_files += $(wildcard in.* *.cpp *.hpp)

c-bins.list = \
		arrays \
		classname \
		cleanup \
		compspeed \
		demo_coord \
		exceptions \
		experimental \
		gene \
		graph \
		mylite \
		patrick \
		pointerdeser \
		pool \
		radford \
		rattlemouse \
		sdj \
		sdj2 \
		statics \
		templates \
		urlparse

ifeq (1,$(LIBS11N_HAS_CPP0X))
  c-bins.list += 0x
endif

define BIN_TEMPLATE
  $(1).BIN.OBJECTS += $(1).o
endef
$(foreach BIN,$(c-bins.list),$(eval $(call BIN_TEMPLATE,$(BIN))))

c-bins.LDADD = -rdynamic $(LIBS11N_CLIENT_INTREE_LDADD)
ifeq (1,$(s11n_CONFIG_ENABLE_PLUGINS))
  c-dlls.list = SerializableOne SerializableTwo
  SerializableOne.DLL.OBJECTS = SerializableOne.o
  SerializableOne.DLL.INSTALL = 0
  SerializableTwo.DLL.OBJECTS = SerializableTwo.o
  SerializableTwo.DLL.INSTALL = 0
  include $(toc2.dirs.makefiles)/c-dlls.make
  plugins: $(SerializableOne.DLL) $(SerializableTwo.DLL)
  c-bins.list += plugins
  plugins.BIN.OBJECTS = plugins.o SerializableBase.o SerializableFinal.o
endif
######## ^^^^ $(s11n_CONFIG_ENABLE_PLUGINS))
include $(toc2.dirs.makefiles)/c-bins.make

Makefile:
	@true

package.clean_files += graph.s11n graph.s11n.2 

run: $(c-bins.list)
	@for i in $(c-bins.list); do \
		LD_LIBRARY_PATH=$(toc2.top_srcdir)/src:$$LD_LIBRARY_PATH ./$$i || { \
			err=$$?; echo "$$i failed with code $$err $(toc2.emoticons.error)"; exit $$err; \
		}; \
	done; \
	echo "All bins returned exit code 0. $(toc2.emoticons.okay)"

all: bins
