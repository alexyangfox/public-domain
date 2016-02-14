
default: all
include toc2.make

package.dist_files += $(wildcard in.* *.cpp *.hpp) s11nconvert.1

SOURCES = main.cpp

ifeq (1,0)
  s11nconvert.BIN.OBJECTS = main.o argv_parser.o
  s11nconvert.BIN.LDADD = -rdynamic $(LIBS11N_CLIENT_INTREE_LDADD)
  $(call toc2.call.rules.c-bin,s11nconvert)
  all: $(s11nconvert.BIN)
else
  c-bins.list := s11nconvert
  s11nconvert.BIN.OBJECTS = main.o argv_parser.o
  s11nconvert.BIN.LDADD = -rdynamic $(LIBS11N_CLIENT_INTREE_LDADD)
  include $(toc2.dirs.makefiles)/c-bins.make
  all: $(s11nconvert.BIN)
endif
package.install.bins = $(s11nconvert.BIN)
package.install.man1 = s11nconvert.1

########################################################################
# start conditional demo/test stuff...
SC = $(wildcard ./$(s11nconvert.BIN))
SCARGS += -DL mysql
ifneq (,$(SC))
    SC := $(SC) $(SCARGS)
    SERIALIZERS = $(shell $(SC) --known-serializers)
    DEMO_IN = in.demo
showser: s11nconvert
	@echo -n "Available Serializers: "; $(SC) --known-serializers

demo:
	LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:.:$(toc2.top_srcdir)/lib"; \
		for s in $(SERIALIZERS); do \
			cmd="$(SC) -f $(DEMO_IN) -s $$s -o out.$$s"; \
			echo $$cmd; $$cmd || exit; \
		done
	ls -la out.{$(shell echo $(SERIALIZERS) | sed -e 's/ /,/g')} || true
        #          ^^^ i am SUCH a lazy bastard!
package.clean_files += $(wildcard my.* out.*)
endif
# ^^^ $(SC)
# end conditional demo/test stuff.
########################################################################
