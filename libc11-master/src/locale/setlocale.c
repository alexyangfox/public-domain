/* This is free and unencumbered software released into the public domain. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <locale.h>

/**
 * @date   2014-11-12
 * @author Arto Bendiken
 * @see    http://libc11.org/locale/setlocale.html
 */
char*
setlocale(const int category,
          const char* const locale) {
  (void)category;
  (void)locale;
  return NULL; // TODO: implementation.
}
