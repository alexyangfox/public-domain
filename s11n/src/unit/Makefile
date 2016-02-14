default: all
include toc2.make

package.dist_files += $(wildcard in.* *.cpp *.hpp)

c-bins.list = unittest
unittest.BIN.OBJECTS = main.o
unittest.BIN.LDADD = -rdynamic $(LIBS11N_CLIENT_INTREE_LDADD)
c-bins.DO_INSTALL = 0
include $(toc2.dirs.makefiles)/c-bins.make
all: unittest.BIN


UNIT_TESTS = Casts \
	Exceptions \
	FormatsFrenzy \
	ListsTest \
	PodsTest \
	PointerDeser \
	URLParser \
	ValArray

UNIT_TESTS.NORUN = Thrower

package.dist_files += TestTemplate.cpp

package.clean_files += $(patsubst %,%.s11n,$(UNIT_TESTS)) \
	FormatsFrenzy.*.s11n


########################################################################
# As long as each test is made up of exactly 1 .cpp file which has
# exactly the same base name as the test, then the following will
# generate the .so rules:
define UNIT_TESTS_DEFAULT_SO_STUFF
c-dlls.list += $(1)
$(1).DLL.OBJECTS = $(1).o
endef
c-dlls.DO_INSTALL = 0
$(foreach proggy,$(UNIT_TESTS) $(UNIT_TESTS.NORUN),$(eval $(call UNIT_TESTS_DEFAULT_SO_STUFF,$(proggy))))
include $(toc2.dirs.makefiles)/c-dlls.make
# end .so generation
########################################################################


Makefile:
	@true

tests: libs
	@LD_LIBRARY_PATH=".:$(toc2.top_srcdir)/src:$${LD_LIBRARY_PATH}"; \
	./$(unittest.BIN) $(UNIT_TESTS)
	@echo 'Unit tests all seem to have passed! $(toc2.emoticons.okay)'
run: tests

all: bins libs
