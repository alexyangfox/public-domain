#include "substdio.h"
#include "subfd.h"
#include "strerr.h"
#include "stralloc.h"
#include "getln.h"
#include "open.h"
#include "readwrite.h"

#define FATAL "setmaillist: fatal: "

void usage()
{
  strerr_die1x(100,"setmaillist: usage: setmaillist list.bin list.tmp");
}

stralloc line = {0};
int match;

char *fnbin;
char *fntmp;
int fd;
substdio ss;
char buf[1024];

void writeerr()
{
  strerr_die4sys(111,FATAL,"unable to write to ",fntmp,": ");
}

void out(s,len)
char *s;
int len;
{
  if (substdio_put(&ss,s,len) == -1) writeerr();
}

void main(argc,argv)
int argc;
char **argv;
{
  umask(033);

  fnbin = argv[1]; if (!fnbin) usage();
  fntmp = argv[2]; if (!fntmp) usage();

  fd = open_trunc(fntmp);
  if (fd == -1)
    strerr_die4sys(111,FATAL,"unable to create ",fntmp,": ");

  substdio_fdbuf(&ss,write,fd,buf,sizeof buf);

  do {
    if (getln(subfdinsmall,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,"unable to read input: ");

    while (line.len) {
      if (line.s[line.len - 1] != '\n')
        if (line.s[line.len - 1] != ' ')
          if (line.s[line.len - 1] != '\t')
            break;
      --line.len;
    }

    if (byte_chr(line.s,line.len,'\0') != line.len)
      strerr_die2x(111,FATAL,"NUL in input");

    if (line.len)
      if (line.s[0] != '#') {
        if ((line.s[0] == '.') || (line.s[0] == '/')) {
          out(line.s,line.len);
          out("",1);
        }
        else {
          if (line.len > 800)
            strerr_die2x(111,FATAL,"addresses must be under 800 bytes");
          if (line.s[0] != '&')
            out("&",1);
          out(line.s,line.len);
          out("",1);
        }
      }

  }
  while (match);

  if (substdio_flush(&ss) == -1) writeerr();
  if (fsync(fd) == -1) writeerr();
  if (close(fd) == -1) writeerr(); /* NFS stupidity */

  if (rename(fntmp,fnbin) == -1)
    strerr_die6sys(111,FATAL,"unable to move ",fntmp," to ",fnbin,": ");
  
  _exit(0);
}
