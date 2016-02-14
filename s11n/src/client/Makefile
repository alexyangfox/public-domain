package.subdirs = s11nconvert
ifneq (,$(strip $(filter dist testdist clean distclean,$(MAKECMDGOALS))))
      # only build these package.subdirs when running certain targets
      package.subdirs += sample
endif
include toc2.make
all: subdirs
