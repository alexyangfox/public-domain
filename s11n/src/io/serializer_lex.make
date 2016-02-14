#!/do/not/make
# Intented to be included by lex-based Serializer Makefiles.
# See the included serializers subdirs for sample usage.

########################################################################
# Requires:
#
#   SERILIZER_BASE = shortname

#   e.g.: compact, parens, funtxt
#
#   Required files:
#      $(SERIALIZER_BASE)_serializer.{c,h}pp
#      $(SERIALIZER_BASE).flex
#
#   In order to provide dynamic file handler lookup, client lexers must
#   register their FlexLexer concrete subclass with with the FlexLexer
#   classloader, as demonstrated in the included lexers.
########################################################################

ifeq (,$(SERIALIZER_BASE))
  $(error SERIALIZER_BASE must be set to the "short" form of the Serializer name before including this file.)
endif

SERIALIZER = $(SERIALIZER_BASE)_serializer

SERIALIZER_FLEX = $(SERIALIZER_BASE).flex
# SERIALIZER_FLEX_CPP might be locally generated or might come with the
# dist tarball. We can only rely on flex 2.5.4, so we ship
# the lexers generated on the dev machine with the tarball.
SERIALIZER_FLEX_CPP = $(SERIALIZER_FLEX).cpp

SERIALIZER_LEXER_PREFIX = $(SERIALIZER_BASE)_data_node
SERIALIZER_LEXER_CLASS = $(SERIALIZER_LEXER_PREFIX)FlexLexer
SERIALIZER_FlexLexer_hpp = $(SERIALIZER_LEXER_CLASS).hpp

SOURCES = $(SERIALIZER).cpp
HEADERS = $(SERIALIZER).hpp

package.dist_files += $(SOURCES) $(HEADERS) \
	$(SERIALIZER_FLEX) $(SERIALIZER_FLEX_CPP) \
	$(SERIALIZER_FlexLexer_hpp)


LexerTemplate_hpp = ../LexerTemplate.hpp
package.distclean_files += $(SERIALIZER_FlexLexer_hpp)
$(SERIALIZER_FlexLexer_hpp): $(LexerTemplate_hpp) Makefile
	sed -e 's/yyFlexLexer/$(SERIALIZER_LEXER_CLASS)/g' $(LexerTemplate_hpp) > $@

########################################################################
# headers stuff...
IOINCLUDES_PATH = include/s11n.net/s11n/io
package.install.headers.dest = $(prefix)/$(IOINCLUDES_PATH)
# $(SERIALIZER_FlexLexer_hpp) is only installed so that we can
# easily make a copy for a build on platforms with no flex, like MS Windows.
package.install.headers = $(HEADERS) $(SERIALIZER_FlexLexer_hpp)
symlink-files.list = $(package.install.headers)
symlink-files.dest = $(toc2.top_srcdir)/$(IOINCLUDES_PATH)
include $(toc2.dirs.makefiles)/symlink-files.make
########################################################################

OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES) $(SERIALIZER_FLEX_CPP))

########################################################################
# if we are using a local flex to build *.flex, the following block
# becomes active. Only flex 2.5.4[a] is known to generate working
# code for the serializers, thus we ship pre-flexed copies in the
# code distribution for use on systems where we don't trust flex.
ifneq (,$(FLEX_BIN))
  ifeq (1,$(SERIALIZERS_USE_LOCAL_FLEX))

  ########################################################################
  # THIS IS A HORRIBLE KLUDGE to work around to some inexplicable build bug.
  # See the comments below labeled 'WTF' to understand the [non]sense of this.
  $(shell test -f $(SERIALIZER_FLEX_CPP) -a $(SERIALIZER_FLEX) -nt $(SERIALIZER_FLEX_CPP) && rm $(SERIALIZER_FLEX_CPP))
  ########################################################################

  FlexLexer_h = s11n.net/s11n/io/FlexLexer.h
  FlexLexer_hpp = s11n.net/s11n/io/FlexLexer.hpp

  package.clean_files += $(SERIALIZER_FLEX_CPP)
  SERIALIZER_FLEX_FLAGS = -p -+ -B -P$(SERIALIZER_LEXER_PREFIX)

  ########################################################################
  # WTF!?!?! When i add SERIALIZER_FLEX as a dep for the
  # SERIALIZER_FLEX_CPP target i get warnings about circular deps being
  # dropped and it tries to compile $(FlexLexer_hpp)!!!
  # AND it DELETES $(SERIALIZER_FLEX)!!!!! Aaaarrrgggg!
  $(SERIALIZER_FLEX_CPP): $(SERIALIZER_FlexLexer_hpp) Makefile ../serializer_lex.make
	@$(call toc2.call.setx-unless-quiet,"FLEX [$(SERIALIZER_FLEX)] ..."); \
	$(FLEX_BIN) $(SERIALIZER_FLEX_FLAGS) -t $(SERIALIZER_FLEX)  > $@
	@echo -n "Patching $@ for recent C++ standards and $(FlexLexer_hpp)... "; \
		perl -i -p \
		-e 's|<FlexLexer\.h>|<$(FlexLexer_hpp)>|g;' \
		-e 's|class (std\::)?istream;|#include <iostream>|;' \
		-e 's/\biostream\.h\b/iostream/;' \
		-e 'next if m/\biostream\b/;' \
		-e 'next if m/::[io]stream/;' \
		-e 's/(\bostream\b|\bistream\b)([^\.])/std::$$1$$2/g;' $@ || exit ; \
		perl -i -ne 'next if $$_ =~ m|unistd\.h|; print;' $@ || exit; \
		echo
  # ^^^ reminder: we remove unistd.h so that the files will build under Windows (hopefully)

  endif # 1 == $(SERIALIZERS_USE_LOCAL_FLEX)
endif #have $(FLEX_BIN)
# ^^^^ local flex support
########################################################################

serializer:  symlink-files $(SERIALIZER_FlexLexer_hpp) $(OBJECTS)
