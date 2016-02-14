#!/usr/bin/make -f
include toc2.make

package.subdirs = manual
ifneq (,$(DOXYGEN_BIN))
  package.subdirs += doxygen
endif


all: subdirs

