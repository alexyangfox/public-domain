# import a macro to prefix config.h defines with a package name
# http://ac-archive.sourceforge.net/Miscellaneous/ax_prefix_config_h.html
m4_include(cfgaux/ax_prefix_config_h.m4)

m4_include(cfgaux/libtool.m4)

# backport AC_LIBTOOL_TAGS from Libtool 1.6
# http://www.mail-archive.com/libtool@gnu.org/msg04504.html
m4_include(cfgaux/libtooltags.m4)
