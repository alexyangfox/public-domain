include Make.cfg

.PHONY : all
all : demos tests

include src/Makefile.mk

include demos/gxpmedit/Makefile.mk
include demos/gxte/Makefile.mk

include tests/Makefile.mk

.PHONY : demos
demos : $(R_OFFSET_GXPMEDIT)/gxpmedit $(R_OFFSET_GXTE)/gxte

.PHONY : tests
tests : $(test_bins)

.PHONY : clean
clean :
	$(MAKE) gxpmedit_clean
	$(MAKE) gxte_clean
	$(MAKE) tests_clean
	$(MAKE) src_clean
	$(RM) lib/*

lib/libGx.a  : src/libGx.a
	$(CP) src/libGx.a lib/libGx.a

.PHONY : real_clean
real_clean : clean
	$(RM) src/*.d
	$(RM) Make.cfg

.PHONY : all
all : lib/libGx.a tests demos

