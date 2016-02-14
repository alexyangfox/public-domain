#!/usr/bin/make -f
# Demonstration of using the toc2 doxygen helper...
include toc2.make

ifeq (,$(toc2.bins.doxygen))
  toc2.bins.doxygen := $(call toc2.call.find-program,doxygen)
endif

ifeq (,$(toc2.bins.doxygen))
########################################################################
# If no doxygen, just warn the user gently...
  all:
	@echo "toc2.bins.doxygen not set - assuming doxygen is not available."
  doxygen: all
else
# end no-doxygen warning
########################################################################
# Start doxygen stuff...
  ifneq (1,$(toc2.flags.making_clean))
    ifeq (,$(wildcard $(toc2.top_srcdir)/include/s11n.net/s11n/*.hpp))
      $(warning You must build the source tree before you can build the doxygen docs.)
    endif
    include $(toc2.dirs.makefiles)/doxygen.make
    doxygen.inputs := $(toc2.top_srcdir)/include
    # doxygen.use_dot := 1
    all:
	@echo "Run the 'doxygen' target to create doxygen docs."

    DISTNAME = $(package.name)-API-$(package.version)
    ifeq (1,$(doxygen.use_dot))
      DISTNAME := $(DISTNAME)-with-charts
    endif
    tar: doxygen
	mv $(doxygen.dirs.out) $(DISTNAME)
	tar czf $(DISTNAME).tar.gz $(DISTNAME)
	tar cjf $(DISTNAME).tar.bz2 $(DISTNAME)
	rm -fr $(DISTNAME)
  endif
# end doxygen stuff.
########################################################################
endif
