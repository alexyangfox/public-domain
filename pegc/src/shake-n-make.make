#!/usr/bin/make -f
all:
SHELL=/bin/bash
MAKE_REQUIRED_VERSION := 380# MAKE_VERSION stripped of any dots
VERSION_CHECK := \
	$(shell \
	test $$(echo $(MAKE_VERSION) | sed -e 's/\.//g') -ge \
	"$(MAKE_REQUIRED_VERSION)" 2>/dev/null \
	&& echo 1 || echo 0)

ifneq (1,$(VERSION_CHECK))
$(error Your version of Make ($(MAKE_VERSION)) is too old to use this code!)
endif


########################################################################
# This file defines a set of basic targets for single-dir C++ projects
# trees, designed for GNU make and gcc. The code in this file is
# intended to remain project-neutral. Add your project-specific stuff
# in higher-level makefiles and include this one from there.
#
# The shell code in this makefile assumes GNU Make, GNU Bash and GNU
# versions several other common system tools, like mkdir, tar, sed,
# etc.
#
########################################################################
# Distribution policy:
#
# Public Domain, of course. No warranties at all: if it destroys your
# data or your marriage, it's your faul (or at least it's not MY fault).
#
# i ACTUALLY DO use this code in several source trees, so please send
# in improvements: http://wanderinghorse.net/computing/make/
#
########################################################################
# Main features:
#
# - Rules for compiling C/C++ sources.
#
# - Rules for building binaries from C/C++ sources.
#
# - Rules for building shared/static libraries and binaries.
#
# - Rules for building a tarred or zipped distribution file.
#
# - Rules for installation of arbitrary file sets.
#
# - Transparent and automatic (and *fast*) dependencies generation for
# C/C++ files.
#
########################################################################
# Ultra-quick usage overview:
#
# Include it from your Makefile like so:
#
#  PACKAGE.NAME = single-token name of your project (e.g., libfoo)
#  PACKAGE.VERSION = project version number, in an arbitrary format.
#  include shake-n-make.mk
#
# Using spaces or special shell characters in .NAME and .VERSION may
# cause problems in this code.
#
# PACKAGE.* are used when building a distribution tarball.
#
# You can optionally define the following variables:
#  CLEAN_FILES += list of files/dirs to delete during 'make clean'
#  DISTCLEAN_FILES += list of files/dirs to delete during 'make distclean'
#  PACKAGE.DIST_FILES += list of files/dirs to include in distribution
#          tarball/zip file.
#          
#
# Achtung:
# ***** Note the use of "+=" on several of those variables!!!! *****
# ***** BE CAREFUL WHEN USING [DIST]CLEAN_FILES!!!! *****
#
# The rules for bins, shared libs, etc., have to be installed by the
# user, but it's really simple. The subsections below explain how to
# use the various features.
#
# To install your package do:
#
#  make install prefix=/install/prefix
#
# where $(prefix) normally defaults to /usr/local. (Setting prefix
# to $HOME is often useful.)
#
########################################################################
# Compiling .o files from *.cpp and *.c:
#
# This file provides flexible rules for building these. See the %.o
# targets for all of the accepted flags.
#
########################################################################
# Building a binary executable:
#
#  mybin.BIN.OBJECTS = foo.o bar.o
#  mybin.BIN.LDFLAGS = -ldl -lstdc++
#  $(call ShakeNMake.CALL.RULES.BINS,mybin)
# 
# You may pass an arbitrary number of binary names to CALL.RULES.BINS.
# See ShakeNMake.EVAL.RULES.BINS for the full list of configuration
# vars it accepts.
#
# A target named mybin.BIN is created for building the binary and
# $(mybin.BIN) holds the name of the compiled binary (typically
# "mybin").
#
########################################################################
# Building a shared library:
#
#   myDLL.DLL.OBJECTS = foo.o bar.o
#   myDLL.LDFLAGS = -L/a/lib/path -lmylib
#   $(call ShakeNMake.CALL.RULES.DLLS,myDLL)
#
# Like CALL.RULES.BINS, you may pass an arbitrary number of DLL names to
# CALL.RULES.DLLS. See EVAL.RULES.DLLS for the full list of configuration
# vars it accepts.
#
# A target named myDLL.DLL is created for building the library and
# $(myDLL.DLL) holds the name of the compiled DLL (typically
# "myDLL.so").
#
########################################################################
# Building a static library is trival:
#
#   myLib.LIB.OBJECTS = foo.o bar.o
#   $(call ShakeNMake.CALL.RULES.LIBS,myLib)
#
# A target named myLib.LIB is created for building the library and
# $(myLib.LIB) holds the name of the compiled lib (typically
# "myLib.a").
#
########################################################################
# Installing files:
#
# To install files you have to define "install sets", where a "set" is
# simply a collection of files which all have the same destination.
# An example:
#
#  INSTALL.STUFF = $(myLib.LIB) $(myDLL.DLL) $(mybin.BIN)
#  INSTALL.STUFF.DEST = $(prefix)/$(PACKAGE.NAME)
#  $(call ShakeNMake.CALL.RULES.INSTALL,STUFF)
#
# That creates install rules named install-STUFF and uninstall-STUFF,
# which are called by the install and uninstall targets, respectively.
#
# You may pass an arbitrary number of install set names to
# CALL.RULES.INSTALL.
#
# Install notes:
#
# - There is nothing magical about the word STUFF: any single token can
# be used. If INSTALL.xxx.DEST is not set then [un]install-xxx will
# cause an error.
#
# - The installation rules are quite braindead, and simply use cp to
# install files.
#
########################################################################
# Distribution tarball:
#
# To build a dist tarball:
#
#     PACKAGE.DIST_FILES += list of files
#
# Then:
#   make dist
#
# That builds a file named $(PACKAGE.NAME)-$(PACKAGE.VERSION).tar.gz and,
# if 'zip' is found in your $(PATH) then a zip file is created as well.
#
# Note that GNUmakefile and shake-n-make.make are added to DIST_FILES by
# default, so you don't need to add those. If shake-n-make.make is using
# mkdep.c to generate C/C++ dependencies than that file is also
# automatically included in the distribution.
#
# If you want to use a non-GNU tar or change the compression type
# (defaults to gzip) then change the ShakeNMake.TARBALL.FLAGS variable
# in this file (not in your Makefile).
# The ShakeNMake.TARBALL.XFLAGS is a special case var which has flags
# for tar which go AFTER the target filename. e.g. you can set it to
# --exclude=MySubDir to exclude files from a certain dir.
#
########################################################################
# Eye candy:
#
# If you want the compiler/linker output to be less verbose, try:
#
#  ShakeNMake.QUIET = 1
#
# BEFORE including this file.
#
########################################################################
# Maintainer's/hacker's notes:
#
# Vars names starting with ShakeNMake are mostly internal to this
# makefile and are considered "private" unless documented otherwise.
# Notable exceptions are most of the ShakeNMake.CALL entries, which
# are $(call)able functions, and ShakeNMake.EVAL entries, which are
# $(eval)able code.
#
########################################################################

ifneq (,$(COMSPEC))
$(warning Setting ShakeNMake.SMELLS.LIKE.WINDOWS to 1)
ShakeNMake.SMELLS.LIKE.WINDOWS := 1
ShakeNMake.EXTENSIONS.DLL = .DLL# maintenance reminder: this must stay upper-case!
ShakeNMake.EXTENSIONS.EXE = .EXE# maintenance reminder: this must stay upper-case!
else
ShakeNMake.SMELLS.LIKE.WINDOWS := 0
ShakeNMake.EXTENSIONS.DLL = .so
ShakeNMake.EXTENSIONS.EXE =# no whitespace, please
endif


ShakeNMake.MAKEFILE = shake-n-make.make
$(ShakeNMake.MAKEFILE):# avoid breaking some deps checks if someone renames this file (been there, done that)

########################################################################
# Core information:

ifeq (,$(PACKAGE.NAME))
$(error You must set PACKAGE.NAME to a single-token name for your prject, e.g., libMyStuff)
endif

ifeq (,$(PACKAGE.VERSION))
$(error You must set PACKAGE.VERSION to a version number for your project, e.g., 1.3.5.7-beta9 or 1.0 or 2007-02-14)
endif


########################################################################
# auto-add the makefiles to DIST_FILES, filtering out any which start
# with a dot because we use such files for temp/volitile files which
# contain Make rules (C/C++ deps, for example).
PACKAGE.MAKEFILE = $(firstword $(MAKEFILE_LIST))# normally either Makefile or GNUmakefile
$(PACKAGE.MAKEFILE):
PACKAGE.DIST_FILES += $(filter-out .%,$(MAKEFILE_LIST))


ShakeNMake.FORCE: ; @true

########################################################################
# DESTDIR is for GNU Autotools compatibility...
DESTDIR ?=
prefix ?= /usr/local
ShakeNMake.INSTALL.DESTDIR = $(DESTDIR)
ShakeNMake.INSTALL.PREFIX = $(prefix)
ShakeNMake.INSTALL_ROOT=$(DESTDIR)$(prefix)/

########################################################################
# ShakeNMake.CALL.FIND_BIN call()able function:
# $1 = app name
# $2 = optional path
ShakeNMake.CALL.FIND_BIN = $(firstword $(wildcard $(addsuffix /$(1),$(subst :, ,$(2) $(PATH)))))
########################################################################

########################################################################
# Find some common binaries...
ShakeNMake.BINS.TAR := $(call ShakeNMake.CALL.FIND_BIN,tar)
ifeq (,$(ShakeNMake.BINS.TAR))
  ShakeNMake.BINS.TAR := $(call ShakeNMake.CALL.FIND_BIN,gtar)
endif
ShakeNMake.BINS.ZIP := $(call ShakeNMake.CALL.FIND_BIN,zip)
ShakeNMake.BINS.RM := $(call ShakeNMake.CALL.FIND_BIN,rm)
ShakeNMake.BINS.GCC := $(call ShakeNMake.CALL.FIND_BIN,gcc)
#
########################################################################

########################################################################
# An internal hack to enable "quiet" output. $(1) is a string which
# is shown ONLY if ShakeNMake.QUIET!=1
ShakeNMake.QUIET ?= 0
define ShakeNMake.CALL.SETX
if [[ x1 = "x$(ShakeNMake.QUIET)" ]]; then echo $(1); else set -x; fi
endef
########################################################################

########################################################################
# PACKAGE.DIST_FILES stuff...
ifeq (,$(ShakeNMake.BINS.TAR))
dist:
	@echo "'tar' was not found in the PATH, so i cannot build a dist tarball :(."
else
ShakeNMake.TARBALL.FLAGS ?= czf
ShakeNMake.TARBALL.XFLAGS ?=
ShakeNMake.TARBALL.BASENAME = $(PACKAGE.NAME)-$(PACKAGE.VERSION)
ShakeNMake.TARBALL.FILE = $(ShakeNMake.TARBALL.BASENAME).tar.gz
ShakeNMake.TARBALL.ZIPFILE = $(ShakeNMake.TARBALL.BASENAME).zip
DISTCLEAN_FILES += $(ShakeNMake.TARBALL.FILE) $(ShakeNMake.TARBALL.ZIPFILE)

dist-cleanup:
	@rm -fr $(ShakeNMake.TARBALL.BASENAME); true
dist-target-implementation:
	@-if test -d $(ShakeNMake.TARBALL.BASENAME); then rm -fr $(ShakeNMake.TARBALL.BASENAME) || exit $$?; fi
	@echo "Creating $(ShakeNMake.TARBALL.FILE)..."
	@mkdir $(ShakeNMake.TARBALL.BASENAME)
	@cp --parents -r $(PACKAGE.DIST_FILES) $(ShakeNMake.TARBALL.BASENAME)
	@find $(ShakeNMake.TARBALL.BASENAME) -name CVS -o -name .svn -type d | xargs rm -fr; true
	@$(ShakeNMake.BINS.TAR) \
		$(ShakeNMake.TARBALL.FLAGS) \
		$(ShakeNMake.TARBALL.FILE) \
		$(ShakeNMake.TARBALL.XFLAGS) \
		$(ShakeNMake.TARBALL.BASENAME)
	@ls -la $(ShakeNMake.TARBALL.FILE)
ifneq (,$(ShakeNMake.BINS.ZIP))
	@test -e $(ShakeNMake.TARBALL.ZIPFILE) && rm -f $(ShakeNMake.TARBALL.ZIPFILE); true
	@$(ShakeNMake.BINS.ZIP) -q -r $(ShakeNMake.TARBALL.ZIPFILE) $(ShakeNMake.TARBALL.BASENAME)
	@ls -la $(ShakeNMake.TARBALL.BASENAME).zip
endif
dist:
	@$(MAKE) --no-print-directory dist-target-implementation; err=$$?; \
	$(MAKE) --no-print-directory dist-cleanup; echo $$err
clean: dist-cleanup
endif # if $(ShakeNMake.BINS.TAR)
# end dist
########################################################################


########################################################################
# ShakeNMake.CALL.INSTALL: $(call)able function:
# $1 = destination dir.
# $2 = list of source files or directories
# Copies $2 to $1.
ShakeNMake.CALL.INSTALL = { \
	test x = "x$(1)" && { echo "Install path is empty!"; exit 1; }; \
	test -d $(1) || mkdir -p $(1) || exit; \
	for x in $(2); do \
		echo -e '\t--> '$$x; \
		cp -rp $$x $(1) || { err=$$?; echo "Copy failed!"; exit $$?; }; \
	done; \
	}
# ShakeNMake.CALL.UNINSTALL: $(call)able function:
# $1 = destination dir.
# $2 = list of files (not directories!) to delete
# Removes $2 from $1. It tries to rmdir $1 when it is done, but that will
# only work if $1 is empty. Such a failure is silently ignored.
ShakeNMake.CALL.UNINSTALL = { \
	test -d $(1) || exit 0; \
	for x in $(2); do \
		echo -e "\t<-- " $(1)/$$x; \
		rm -f $(1)/$$x || exit; \
	done; \
	}; \
	rmdir $(1) 2>/dev/null || true;
# Trivia: i actually did delete all files in my home dir
# once while testing the uninstall code. Luckily, -r wasn't
# in effect.
########################################################################

########################################################################
# install-% installs files listed in $(INSTALL.%) to $(INSTALL.%.DEST),
# which defaults to %. $(ShakeNMake.INSTALL.DESTDIR) is automatically
# prefixed to installation paths, for compatibility with the GNU
# Autotools and Debian-preferred install methods.
# When specifying install paths for client code, like so:
#
#  INSTALL.MYSTUFF = list of files
#  INSTALL.MYSTUFF.DEST = $(prefix)/$(PACKAGE.NAME)
#
# the path should always be relative to $(prefix), with the assumption that
# $(prefix) is an absolute path without a trailing backslash. The install
# path should NOT take into accont $(DESTDIR), because that is handled
# transparently at a deeper level.
ShakeNMake.CALL.GET_INSTALL_DEST = $(ShakeNMake.INSTALL.DESTDIR)$(INSTALL.$(1).DEST)
install-%:
	@echo "Installing \$$(INSTALL.$(*)) files to $(call ShakeNMake.CALL.GET_INSTALL_DEST,$(*))..."; \
	$(call ShakeNMake.CALL.INSTALL,$(call ShakeNMake.CALL.GET_INSTALL_DEST,$(*)),$(INSTALL.$(*)))
# Maintenance note: don't break the $(call) args onto separate lines because
# that introduces a whitespace char which can screw up the install code. :(
########################################################################
# uninstall-% deletes INSTALL.% from INSTALL.%.DEST, which defaults to
# $(ShakeNMake.INSTALL_ROOT)%.
uninstall-%:
	@echo "Uninstalling \$$(INSTALL.$(*)) files from $(call ShakeNMake.CALL.GET_INSTALL_DEST,$(*))..."; \
	$(call ShakeNMake.CALL.UNINSTALL,$(call ShakeNMake.CALL.GET_INSTALL_DEST,$(*)),$(INSTALL.$(*)))
# Maintenance note: don't break the $(call) args onto separate lines because
# that introduces a whitespace char which can screw up the uninstall code. :(
########################################################################

########################################################################
# ShakeNMake.EVAL.RULES.INSTALL adds [un]install-$(1) as prerequisites
# of the [un]install targets, so that they get called by
# 'make [un]install'.
define ShakeNMake.EVAL.RULES.INSTALL
INSTALL.$(1).DEST ?= $(prefix)/$(1)
install: install-$(1)
uninstall: uninstall-$(1)
endef
########################################################################
# $(call ShakeNMake.CALL.RULES.INSTALL,[list]) calls and $(eval)s
# ShakeNMake.EVAL.RULES.INSTALL one time for each item in $(1).
define ShakeNMake.CALL.RULES.INSTALL
$(foreach proggy,$(1),$(eval $(call ShakeNMake.EVAL.RULES.INSTALL,$(proggy))))
endef
# end ShakeNMake.EVAL.RULES.INSTALL
########################################################################


########################################################################
# builds %.o from %.c using $(CC).
# Passes on flags from these vars:
#   CFLAGS, %.CFLAGS
#   INCLUDES, %.OBJ.INCLUDES
#   CPPFLAGS, %.OBJ.CPPFLAGS
%.o: %.c $(ShakeNMake.MAKEFILE) $(PACKAGE.MAKEFILE)
	@$(call ShakeNMake.CALL.SETX,"CC [$@] ..."); \
	$(CC) $(CFLAGS) $($(*).OBJ.CFLAGS) \
		$(INCLUDES) $($(*).OBJ.INCLUDES) \
		$(CPPFLAGS) $($(*).OBJ.CPPFLAGS) \
		-c -o $@ $<
########################################################################

########################################################################
# build %.o from %.cpp using $(CXX).
# Passes on flags from these vars:
#   CXXFLAGS, %.OBJ.CXXFLAGS
#   INCLUDES, %.OBJ.INCLUDES
#   CPPFLAGS, %.OBJ.CPPFLAGS
%.o: %.cpp $(ShakeNMake.MAKEFILE) $(PACKAGE.MAKEFILE)
	@$(call ShakeNMake.CALL.SETX,"CXX [$@] ..."); \
	 $(CXX) $(CXXFLAGS) $($(*).OBJ.CXXFLAGS) \
		$(INCLUDES) $($(*).OBJ.INCLUDES) \
		$(CPPFLAGS) $($(*).OBJ.CPPFLAGS) \
		-c -o $@ $<
# end %.o: %.cpp rules
########################################################################


########################################################################
# ShakeNMake.EVAL.RULES.BIN is intended to be called like so:
# $(eval $(call ShakeNMake.EVAL.RULES.BIN,MyApp))
#
# It builds a binary named $(1) by running $(CC) and passing it:
#
# INCLUDES, $(1).BIN.INCLUDES
# CFLAGS, $(1).BIN.CFLAGS
# CXXFLAGS, $(1).BIN.CXXFLAGS
# CPPFLAGS, $(1).BIN.CPPFLAGS
# LDFLAGS, $(1).BIN.LDFLAGS
# $(1).BIN.OBJECTS $(1).BIN.SOURCES
#
# Note that we have to pass both CFLAGS and CPPFLAGS because .SOURCES might
# contain either of C or C++ files.
define ShakeNMake.EVAL.RULES.BIN
$(1).BIN = $(1)$(ShakeNMake.EXTENSIONS.EXE)
$(1).BIN: $$($(1).BIN)
# Many developers feel that bins should not be cleaned by 'make
# clean', but instead by distclean, but i'm not one of those
# developers. i subscribe more to the school of thought that distclean
# is for cleaning up configure-created files. That said, shake-n-make
# isn't designed to use a configure-like process, so that is probably
# moot here and we probably (maybe?) should clean up bins only in
# distclean. As always: hack it to suit your preference:
CLEAN_FILES += $$($(1).BIN)
$$($(1).BIN): $$($(1).BIN.OBJECTS) $$($(1).BIN.SOURCES)
	@test x = "x$$($(1).BIN.OBJECTS)$$($(1).BIN.SOURCES)" && { \
	echo "$(1).BIN.OBJECTS and/or $(1).BIN.SOURCES is undefined!"; exit 1; }; \
	$(call ShakeNMake.CALL.SETX,"CXX [$$@] ..."); \
	$$(CXX) -o $$@ \
		$$(INCLUDES) $$($(1).BIN.INCLUDES) \
		$$(CFLAGS) $$($(1).BIN.CFLAGS) \
		$$(CXXFLAGS) $$($(1).BIN.CXXFLAGS) \
		$$(CPPFLAGS) $$($(1).BIN.CPPFLAGS) \
		$$($(1).BIN.OBJECTS) $$($(1).BIN.SOURCES) \
		$$(LDFLAGS) $$($(1).BIN.LDFLAGS)
# note about 'set -x': i do this because it normalizes backslashed
# newline, extra spaces, and other oddities of formatting.
endef
########################################################################
# $(call ShakeNMake.CALL.RULES.BINS,[list]) calls and $(eval)s
# ShakeNMake.EVAL.RULES.BIN for each entry in $(1)
define ShakeNMake.CALL.RULES.BINS
$(foreach bin,$(1),$(eval $(call ShakeNMake.EVAL.RULES.BIN,$(bin))))
endef
# end ShakeNMake.CALL.RULES.BIN and friends
########################################################################


########################################################################
# ShakeNMake.EVAL.RULES.DLL builds builds $(1)$(ShakeNMake.EXTENSIONS.DLL) from object files
# defined by $(1).DLL.OBJECTS and $(1).DLL.SOURCES. Flags passed on
# to the linker include:
#   LDFLAGS, $(1).DLL.LDFLAGS, LDADD, -shared -export-dynamic
#   $(1).DLL.CPPFLAGS
#
# Also defines the var $(1).DLL, which expands to the filename of the DLL,
# (normally $(1)$(ShakeNMake.EXTENSIONS.DLL)).
define ShakeNMake.EVAL.RULES.DLL
$(1).DLL = $(1)$(ShakeNMake.EXTENSIONS.DLL)
ifneq (.DLL,$(ShakeNMake.EXTENSIONS.DLL))
$(1).DLL: $$($(1).DLL)
endif
CLEAN_FILES += $$($(1).DLL)
$$($(1).DLL): $$($(1).DLL.SOURCES) $$($(1).DLL.OBJECTS)
	@test x = "x$$($(1).DLL.OBJECTS)$$($(1).DLL.SOURCES)" && { \
	echo "$(1).DLL.OBJECTS and/or $(1).DLL.SOURCES are/is undefined!"; exit 1; }; \
	$(call ShakeNMake.CALL.SETX,"CXX [$$@] ..."); \
	 $$(CXX) -o $$@ -shared -export-dynamic $$(LDFLAGS) \
		$$($(1).DLL.LDFLAGS) $$($(1).DLL.OBJECTS) $$($(1).DLL.SOURCES) \
		$$($(1).DLL.CPPFLAGS)
endef
########################################################################
# $(call ShakeNMake.CALL.RULES.DLLS,[list]) calls and $(eval)s
# ShakeNMake.EVAL.RULES.DLL for each entry in $(1)
define ShakeNMake.CALL.RULES.DLLS
$(foreach dll,$(1),$(eval $(call ShakeNMake.EVAL.RULES.DLL,$(dll))))
endef
# end ShakeNMake.CALL.RULES.DLLS and friends
########################################################################

########################################################################
# ShakeNMake.EVAL.RULES.LIB creates rules to build static library
# $(1).a
define ShakeNMake.EVAL.RULES.LIB
$(1).LIB = $(1).a
$(1).LIB: $$($(1).LIB)
CLEAN_FILES += $$($(1).LIB)
$$($(1).LIB): $$($(1).LIB.OBJECTS)
	@$(call ShakeNMake.CALL.SETX,"AR [$$@] ..."); \
		$$(AR) crs $$@ $$($(1).LIB.OBJECTS)
endef
define ShakeNMake.CALL.RULES.LIBS
$(foreach liba,$(1),$(eval $(call ShakeNMake.EVAL.RULES.LIB,$(liba))))
endef
# end ShakeNMake.EVAL.RULES.LIB
########################################################################

########################################################################
# [DIST]CLEAN_FILES support...
########################################################################
CLEAN_FILES += *.o
DISTCLEAN_FILES += *~
clean: ShakeNMake.FORCE
	@fl="$(sort $(wildcard $(CLEAN_FILES)))"; \
		test x = "x$$fl" && { echo "Nothing to clean!"; exit 0; }; \
		$(call ShakeNMake.CALL.SETX,"Cleaning up ..."); \
		$(ShakeNMake.BINS.RM) -fr $$fl
distclean: ShakeNMake.FORCE
	@fl="$(sort $(wildcard $(CLEAN_FILES) $(DISTCLEAN_FILES)))"; \
		test x = "x$$fl" && { echo "Nothing to clean!"; exit 0; }; \
		$(call ShakeNMake.CALL.SETX,"Cleaning up ..."); \
		$(ShakeNMake.BINS.RM) -fr $$fl
# end [DIST]CLEAN_FILES
########################################################################


########################################################################
# Automatic dependencies generation for C/C++ code...
# To disable deps generation, set ShakeNMake.USE_MKDEPS=0 *before*
# including this file.
ifeq (,$(ShakeNMake.BINS.GCC))
ShakeNMake.USE_MKDEPS ?= 0
else
ShakeNMake.USE_MKDEPS ?= 1
endif
#$(warning ShakeNMake.USE_MKDEPS=$(ShakeNMake.USE_MKDEPS));
ifeq (1,$(ShakeNMake.USE_MKDEPS))
ShakeNMake.CISH_SOURCES += $(wildcard *.cpp *.c *.c++ *.h *.hpp *.h++ *.hh)
#$(warning ShakeNMake.CISH_SOURCES=$(ShakeNMake.CISH_SOURCES))
ifneq (,$(ShakeNMake.CISH_SOURCES))
ShakeNMake.CISH_DEPS_FILE := .make.c_deps
ShakeNMake.BINS.MKDEP = gcc -E -MM $(INCLUDES)
CLEAN_FILES += $(ShakeNMake.CISH_DEPS_FILE)
$(ShakeNMake.CISH_DEPS_FILE): $(PACKAGE.MAKEFILE) $(ShakeNMake.MAKEFILE) $(ShakeNMake.CISH_SOURCES)
	@$(ShakeNMake.BINS.MKDEP) $(ShakeNMake.CISH_SOURCES) 2>/dev/null > $@ || \
	$(ShakeNMake.BINS.RM) -f $@ 2>/dev/null
# ^^^^ We rm -f the deps file if mkdep fails because we don't want a bad generated makefile
# to kill the build.

ifneq (,$(strip $(filter distclean clean,$(MAKECMDGOALS))))
#$(warning Skipping C/C++ deps generation.)
ABSOLUTEBOGO := $(shell $(ShakeNMake.BINS.RM) -f $(ShakeNMake.CISH_DEPS_FILE))
else
#$(warning Including C/C++ deps.)
-include $(ShakeNMake.CISH_DEPS_FILE)
endif

endif
# ^^^^ ifneq(,$(ShakeNMake.CISH_SOURCES))
endif
# ^^^^ end $(ShakeNMake.USE_MKDEPS)
########################################################################

########################################################################
# Doxygen
ShakeNMake.BINS.SED := $(call ShakeNMake.CALL.FIND_BIN,sed)
ShakeNMake.BINS.PERL := $(call ShakeNMake.CALL.FIND_BIN,perl)
ShakeNMake.BINS.LATEX := $(call ShakeNMake.CALL.FIND_BIN,latex)
ifneq (,$(ShakeNMake.BINS.PERL))
ShakeNMake.BINS.DOXYGEN := $(call ShakeNMake.CALL.FIND_BIN,doxygen)
ifneq (,$(ShakeNMake.BINS.DOXYGEN))
ShakeNMake.DOXYGEN.DOXYFILE_TEMPLATE = Doxyfile.at
ifneq (,$(wildcard $(ShakeNMake.DOXYGEN.DOXYFILE_TEMPLATE)))
ShakeNMake.DOXYGEN.INDEX := $(wildcard Doxygen-index.txt)
#ifneq (,$(wildcard $(ShakeNMake.DOXYGEN.INDEX)))
########################################################################
# let's try to do doxygen stuff...
########################################################################
# Set ShakeNMake.DOXYGEN.USE_DOT to 1 if you have 'dot' and want to
# use it in the doxygen stuff. It slows down the doc gen process
# significantly, but it looks nice.
ShakeNMake.DOXYGEN.USE_DOT ?= 0
##################################################
PACKAGE.DIST_FILES += $(ShakeNMake.DOXYGEN.DOXYFILE_TEMPLATE) $(ShakeNMake.DOXYGEN.INDEX)

ShakeNMake.DOXYGEN.INCLUDE_DIRS = .

ShakeNMake.DOXYGEN.OUTPUT_DIR.HTML = $(PACKAGE.NAME)-$(PACKAGE.VERSION)-doxygen-html
ShakeNMake.DOXYGEN.OUTPUT_DIR.LATEX = $(PACKAGE.NAME)-$(PACKAGE.VERSION)-doxygen-latex


ifneq (,$(ShakeNMake.BINS.LATEX))
  ShakeNMake.DOXYGEN.GENERATE_LATEX ?= YES
else
  ShakeNMake.DOXYGEN.GENERATE_LATEX ?= NO
endif

Doxyfile: $(ShakeNMake.DOXYGEN.DOXYFILE_TEMPLATE) $(ShakeNMake.MAKEFILE) $(PACKAGE.MAKEFILE)
	@$(ShakeNMake.BINS.SED) -e 's,@PACKAGE_NAME@,$(PACKAGE.NAME),' \
		-e 's,@PACKAGE_VERSION@,$(PACKAGE.VERSION),' \
		-e 's,@DOXYGEN_INPUT@,$(ShakeNMake.DOXYGEN.INDEX) $(ShakeNMake.DOXYGEN.INCLUDE_DIRS),' \
		-e 's,@HTML_OUTPUT@,$(ShakeNMake.DOXYGEN.OUTPUT_DIR.HTML),' \
		-e 's,@LATEX_OUTPUT@,$(ShakeNMake.DOXYGEN.OUTPUT_DIR.LATEX),' \
		-e 's,@GENERATE_LATEX@,$(ShakeNMake.DOXYGEN.GENERATE_LATEX),' \
		-e 's,@PERL@,$(ShakeNMake.BINS.PERL),' \
		-e 's,@USE_DOT@,$(ShakeNMake.DOXYGEN.USE_DOT),' \
	< $(ShakeNMake.DOXYGEN.DOXYFILE_TEMPLATE) > $@

doxygen-clean:
	@test -d $(ShakeNMake.DOXYGEN.OUTPUT_DIR.HTML) && rm -fr $(ShakeNMake.DOXYGEN.OUTPUT_DIR.HTML); \
	rm -f Doxyfile; \
	true

.PHONY: $(ShakeNMake.DOXYGEN.OUTPUT_DIR.HTML)
$(ShakeNMake.DOXYGEN.OUTPUT_DIR.HTML): Doxyfile
	@echo "Building docs from headers"
	$(ShakeNMake.BINS.DOXYGEN)
	@echo "Output should be in the directory '$@'."
ifneq (NO,$(ShakeNMake.DOXYGEN.GENERATE_LATEX))
	@echo "Latex output (if any) is in '$(ShakeNMake.DOXYGEN.OUTPUT_DIR.LATEX)'."
endif

doxygen: $(ShakeNMake.DOXYGEN.OUTPUT_DIR.HTML)

doxygen-dist: doxygen
	tar czf $(ShakeNMake.DOXYGEN.OUTPUT_DIR.HTML).tar.gz $(ShakeNMake.DOXYGEN.OUTPUT_DIR.HTML)
	@ls -la $(ShakeNMake.DOXYGEN.OUTPUT_DIR.HTML).tar.gz

dist: doxygen-dist
CLEAN_FILES += Doxyfile $(ShakeNMake.DOXYGEN.OUTPUT_DIR.HTML) latex


INSTALL_DOXYGEN = $(ShakeNMake.DOXYGEN.OUTPUT_DIR.HTML)
INSTALL_DOXYGEN_DEST = $(prefix)/share/doc/$(PACKAGE.NAME)
install: doxygen
$(call ShakeNMake.CALL.RULES.INSTALL,DOXYGEN)

##################################################
#endif # $(ShakeNMake.DOXYGEN.INDEX)
endif # $(ShakeNMake.DOXYGEN.DOXYFILE_TEMPLATE)
endif # $(ShakeNMake.BINS.DOXYGEN)
endif # $(ShakeNMake.BINS.PERL)
# end Doxygen
########################################################################

