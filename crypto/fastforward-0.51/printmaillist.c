#include "substdio.h"
#include "subfd.h"
#include "strerr.h"
#include "stralloc.h"
#include "getln.h"

#define FATAL "printmaillist: fatal: "

void badformat()
{
  strerr_die2x(100,FATAL,"bad mailing list format");
}

stralloc line = {0};
int match;

void main()
{
  for (;;) {
    if (getln(subfdinsmall,&line,&match,'\0') == -1)
      strerr_die2sys(111,FATAL,"unable to read input: ");
    if (!match) {
      if (line.len)
        badformat();
      if (substdio_flush(subfdoutsmall) == -1)
        strerr_die2sys(111,FATAL,"unable to write output: ");
      _exit(0);
    }

    if (line.s[str_chr(line.s,'\n')]) badformat();
    if (line.s[line.len - 1] == ' ') badformat();
    if (line.s[line.len - 1] == '\t') badformat();

    if ((line.s[0] == '.') || (line.s[0] == '/')) {
      if (substdio_puts(subfdoutsmall,line.s) == -1)
        strerr_die2sys(111,FATAL,"unable to write output: ");
      if (substdio_puts(subfdoutsmall,"\n") == -1)
        strerr_die2sys(111,FATAL,"unable to write output: ");
      continue;
    }
    if (line.s[0] == '&') {
      if (line.len > 900) badformat();
      if (substdio_puts(subfdoutsmall,line.s) == -1)
        strerr_die2sys(111,FATAL,"unable to write output: ");
      if (substdio_puts(subfdoutsmall,"\n") == -1)
        strerr_die2sys(111,FATAL,"unable to write output: ");
      continue;
    }

    badformat();
  }
}
