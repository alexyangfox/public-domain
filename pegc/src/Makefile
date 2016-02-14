#!/usr/bin/make -f
# Requires GNU Make 3.80+!
default: all

ifeq (1,$(TCC))
  # If you have (want to use) tcc, try this:
  CC := tcc
  CXX := tcc
  CPPFLAGS += -gb -bt 10 \
	-Wwrite-strings \
	-Wunsupported \
	-Wall
else
  # Assume gcc-compatible flags:
  CXXFLAGS += -g
#  CXXFLAGS += -Os
  CFLAGS += \
	-Wimplicit-function-declaration \
	-Wall \
	-g
endif

# CPPFLAGS ?= -g
# -Wall -Werror

PACKAGE.NAME = pegc
PACKAGE.VERSION := $(shell date +%Y%m%d)
ShakeNMake.DOXYGEN.GENERATE_LATEX := NO
ShakeNMake.DOXYGEN.USE_DOT := 0
-include shake-n-make.make

#CFLAGS += -std=c99
#CFLAGS += -xc99=all # SUN CC

INCLUDES += -I.

########################################################################
# Hashtable lib:
HASH_OBJ := $(patsubst %,%.o,whhash)
libwhhash.LIB.OBJECTS = $(HASH_OBJ)
$(call ShakeNMake.CALL.RULES.LIBS,libwhhash)

########################################################################
# GC lib:
WHGC_OBJ := whgc.o
libwhgc.LIB.OBJECTS = $(WHGC_OBJ) $(HASH_OBJ)
$(call ShakeNMake.CALL.RULES.LIBS,libwhgc)
$(libwhgc.LIB): $(libwhhash.LIB)

########################################################################
# Clob lib
WHCLOB_OBJ := whclob.o vappendf.o
libwhclob.LIB.OBJECTS = $(WHCLOB_OBJ)
$(call ShakeNMake.CALL.RULES.LIBS,libwhclob)
libs: $(libwhclob.LIB)

########################################################################
# Reference counting lib
WHRC_OBJ := whrc.o
libwhrc.LIB.OBJECTS = $(WHRC_OBJ) $(libwhhash.LIB)
$(call ShakeNMake.CALL.RULES.LIBS,libwhrc)
libs: $(libwhrc.LIB)


########################################################################
# pegc lib:
libpegc.LIB.EXTRALIBS :=  $(libwhgc.LIB) $(libwhhash.LIB) $(libwhclob.LIB)
libpegc.LIB.EXTRAOBJ :=  $(libwhgc.LIB.OBJECTS) $(libwhclob.LIB.OBJECTS)
libpegc.LIB.OBJECTS := pegc.o
ifeq (1,1)
  libpegc.LIB.OBJECTS += $(libpegc.LIB.EXTRAOBJ)
else
  libpegc.LIB.OBJECTS += $(libpegc.LIB.EXTRALIBS)
endif
$(call ShakeNMake.CALL.RULES.LIBS,libpegc)
$(libpegc.LIB): $(libwhgc.LIB)
test.BIN.LDFLAGS := $(libpegc.LIB)
test.BIN.OBJECTS := test.o
$(call ShakeNMake.CALL.RULES.BINS,test)
$(test.BIN): $(libpegc.LIB)

pegcgen.BIN.LDFLAGS := -L. -lpegc
pegcgen.BIN.OBJECTS := pegcgen.o # $(libpegc.LIB.OBJECTS)
$(call ShakeNMake.CALL.RULES.BINS,pegcgen)
$(pegcgen.BIN): $(libpegc.LIB)

unittests.BIN.LDFLAGS := $(libpegc.LIB)  $(libwhrc.LIB)
unittests.BIN.OBJECTS := unittests.o # $(libpegc.LIB.OBJECTS)
$(call ShakeNMake.CALL.RULES.BINS,unittests)
$(unittests.BIN): $(libpegc.LIB) $(libwhrc.LIB)

bins: $(test.BIN)
bins: $(pegcgen.BIN)
bins: $(unittests.BIN)
libs: $(libpegc.LIB)
CLEAN_FILES += *~

PACKAGE.DIST_FILES += $(wildcard *.c *.h)

all: libs bins
