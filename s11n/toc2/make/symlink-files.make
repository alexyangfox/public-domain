#!/do/not/make
#
# snippet to symlink files to a common dir.
#
# Usage:
#   symlink-files.list = $(wildcard *.h)
#   symlink-files.dest = $(top_srcdir)/include/whereever
#   include /path/to/symlink-files.make
# run:
#  all: symlink-files.list
#
# The symlinks are cleaned up during clean/distclean.

ifeq (,$(symlink-files.list))
$(error You must define symlink-files.list before including this makefile.)
endif
ifeq (,$(symlink-files.dest))
$(error You must define both symlink-files.dest before including this makefile.)
endif

symlink-files.dest.list := $(addprefix $(symlink-files.dest)/,$(symlink-files.list))
symlink-files.makefile := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
$(symlink-files.makefile):
$(symlink-files.dest.list):
	@$(call toc2.call.install-symlink,$(notdir $@),$(symlink-files.dest),-m 0644)
#	@echo "Symlinking $@"; test -L "$@" && rm "$@"; ln -s "$${PWD}/$(notdir $@)" "$@"
symlink-files: $(symlink-files.dest.list)
#	@$(call toc2.call.install-symlink,$(symlink-files.list),$(symlink-files.dest),-m 0644)
symlink-files.list: symlink-files
symlink-files-clean: FORCE
	@echo "Cleaning symlinks."
	@-touch foo.cleanlocal; rm foo.cleanlocal $(wildcard $(symlink-files.dest.list))

.PHONY: symlink-files-clean symlink-files
clean-.: symlink-files-clean
distclean-.: symlink-files-clean
