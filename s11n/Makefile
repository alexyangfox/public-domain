#!/usr/bin/make -f

toc2.dist.suffix_list := tar.bz2
include toc2.make
package.subdirs = src
ifneq (,$(strip $(filter dist testdist clean distclean uninstall install,$(MAKECMDGOALS))))
# only build these package.subdirs when running certain targets
      package.subdirs += include doc toc2
endif
subdirs: $(addprefix subdir-,$(package.subdirs))

package.install.bin-scripts = $(package.name)-config
package.install.pkgconfig = $(package.name).pc


package.dist_files += ChangeLog INSTALL INSTALL.nobuildfiles LICENSE.$(package.name) \
	README.src README.SVN README.WIN32 \
	configure configure.$(package.name) postconfig.$(package.name) \
	toc2.$(package.name).make.at toc2.$(package.name).help \
	find_toc2.sh


package.distclean_files += $(package.name)-config $(package.name).pc

all: subdirs



########################################################################
# Check if tree appears to be built, and do some stuff if it is...
ifneq (,$(wildcard src/lite/s11nlite.o))
S11NCONF = $(wildcard ./libs11n-config)
LIB_WITH_VERSION = libs11n-$(shell $(S11NCONF) --version)

NOBUILD_DIR = nobuildfiles# must match name used by ./create_generated_tree.sh
NOBUILD_PKG_DIR = $(LIB_WITH_VERSION)-$(NOBUILD_DIR)
NOBUILD_ZIP = $(NOBUILD_PKG_DIR).zip
##################################################
# The nobuild target creates a source zip file
# for cross-platform use.
.PHONY: nobuild
nobuild:
	@test -d $(NOBUILD_DIR) && rm -fr $(NOBUILD_DIR); exit 0
	./create_generated_tree.sh
	@test -d $(NOBUILD_PKG_DIR) && rm -fr $(NOBUILD_PKG_DIR); exit 0
	mv $(NOBUILD_DIR) $(NOBUILD_PKG_DIR)
	@test -f $(NOBUILD_ZIP) && rm $(NOBUILD_ZIP); exit 0
	zip -q -r $(NOBUILD_ZIP) $(NOBUILD_PKG_DIR)
	@ls -la $(NOBUILD_ZIP)
CLEAN_FILES += $(NOBUILD_PKG_DIR)
########################################################################
# Tree isn't built yet.
else
nobuild:
	@echo "Creating the fully generated source tree requires a configured and built source tree."; exit 1
endif
# end built-tree check.
########################################################################
