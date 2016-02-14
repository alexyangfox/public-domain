#!/usr/bin/make -f

include toc2.make

ifeq (1,$(s11n_CONFIG_ADDON_PSTREAMS_ENABLE))
  package.subdirs += pstreams
  subdirs: subdir-pstreams
endif

all: subdirs
