#include <sys/types.h>
#include <sys/time.h>
#include "sgetopt.h"
#include "stralloc.h"
#include "strerr.h"
#include "exit.h"
#include "readwrite.h"
#include "open.h"
#include "substdio.h"
#include "str.h"
#include "auto_bin.h"

#define FATAL "ezmlm-make: fatal: "

void die_usage()
{
  strerr_die1x(100,"ezmlm-make: usage: ezmlm-make [ -aApP ] dir dot local host");
}
void die_relative()
{
  strerr_die2x(100,FATAL,"dir must start with slash");
}
void die_newline()
{
  strerr_die2x(100,FATAL,"newlines not allowed");
}
void die_quote()
{
  strerr_die2x(100,FATAL,"quotes not allowed");
}
void die_nomem()
{
  strerr_die2x(111,FATAL,"out of memory");
}

stralloc key = {0};
struct timeval tv;

void keyadd(u)
unsigned long u;
{
  char ch;
  ch = u; if (!stralloc_append(&key,&ch)) die_nomem(); u >>= 8;
  ch = u; if (!stralloc_append(&key,&ch)) die_nomem(); u >>= 8;
  ch = u; if (!stralloc_append(&key,&ch)) die_nomem(); u >>= 8;
  ch = u; if (!stralloc_append(&key,&ch)) die_nomem();
}

void keyaddtime()
{
  gettimeofday(&tv,(struct timezone *) 0);
  keyadd(tv.tv_usec);
}

char *dir;
char *dot;
char *local;
char *host;

stralloc dotplus = {0};
stralloc dirplus = {0};

void dirplusmake(slash)
char *slash;
{
  if (!stralloc_copys(&dirplus,dir)) die_nomem();
  if (!stralloc_cats(&dirplus,slash)) die_nomem();
  if (!stralloc_0(&dirplus)) die_nomem();
}

void linkdotdir(dash,slash)
char *dash;
char *slash;
{
  if (!stralloc_copys(&dotplus,dot)) die_nomem();
  if (!stralloc_cats(&dotplus,dash)) die_nomem();
  if (!stralloc_0(&dotplus)) die_nomem();
  dirplusmake(slash);
  if (symlink(dirplus.s,dotplus.s) == -1)
    strerr_die4sys(111,FATAL,"unable to create ",dotplus.s,": ");
  keyaddtime();
}

void dcreate(slash)
char *slash;
{
  dirplusmake(slash);
  if (mkdir(dirplus.s,0755) == -1)
    strerr_die4sys(111,FATAL,"unable to create ",dirplus.s,": ");
  keyaddtime();
}

substdio ss;
char ssbuf[SUBSTDIO_OUTSIZE];

void fopen(slash)
char *slash;
{
  int fd;

  dirplusmake(slash);
  fd = open_trunc(dirplus.s);
  if (fd == -1)
    strerr_die4sys(111,FATAL,"unable to create ",dirplus.s,": ");

  substdio_fdbuf(&ss,write,fd,ssbuf,sizeof(ssbuf));
}

void fput(buf,len)
char *buf;
unsigned int len;
{
  if (substdio_bput(&ss,buf,len) == -1)
    strerr_die4sys(111,FATAL,"unable to write to ",dirplus.s,": ");
}
void fputs(buf)
char *buf;
{
  if (substdio_bputs(&ss,buf) == -1)
    strerr_die4sys(111,FATAL,"unable to write to ",dirplus.s,": ");
}

void fclose()
{
  if (substdio_flush(&ss) == -1)
    strerr_die4sys(111,FATAL,"unable to write to ",dirplus.s,": ");
  if (fsync(ss.fd) == -1)
    strerr_die4sys(111,FATAL,"unable to write to ",dirplus.s,": ");
  if (close(ss.fd) == -1) /* NFS stupidity */
    strerr_die4sys(111,FATAL,"unable to write to ",dirplus.s,": ");
  keyaddtime();
}

void main(argc,argv)
int argc;
char **argv;
{
  int opt;
  int flagarchived;
  int flagpublic;

  keyadd(getpid());
  keyadd(getppid());
  keyadd(getuid());
  keyadd(getgid());
  gettimeofday(&tv,(struct timezone *) 0);
  keyadd(tv.tv_sec);

  umask(077);

  flagarchived = 1;
  flagpublic = 1;

  while ((opt = getopt(argc,argv,"aApP")) != opteof)
    switch(opt) {
      case 'a': flagarchived = 1; break;
      case 'A': flagarchived = 0; break;
      case 'p': flagpublic = 1; break;
      case 'P': flagpublic = 0; break;
      default:
	die_usage();
    }
  argv += optind;

  if (!(dir = *argv++)) die_usage();
  if (!(dot = *argv++)) die_usage();
  if (!(local = *argv++)) die_usage();
  if (!(host = *argv++)) die_usage();

  if (dir[0] != '/') die_relative();
  if (dir[str_chr(dir,'\'')]) die_quote();
  if (dir[str_chr(dir,'\n')]) die_newline();
  if (local[str_chr(local,'\n')]) die_newline();
  if (host[str_chr(host,'\n')]) die_newline();

  dcreate("");
  dcreate("/archive");
  dcreate("/subscribers");
  dcreate("/bounce");
  dcreate("/text");


  linkdotdir("-owner","/owner");
  linkdotdir("-default","/manager");
  linkdotdir("-return-default","/bouncer");
  linkdotdir("","/editor");

  fopen("/lock"); fclose();
  fopen("/lockbounce"); fclose();
  if (flagpublic) {
    fopen("/public"); fclose();
  }
  if (flagarchived) {
    fopen("/archived"); fclose();
  }
  fopen("/num"); fputs("0\n"); fclose();
  fopen("/inhost"); fputs(host); fputs("\n"); fclose();
  fopen("/outhost"); fputs(host); fputs("\n"); fclose();
  fopen("/inlocal"); fputs(local); fputs("\n"); fclose();
  fopen("/outlocal"); fputs(local); fputs("\n"); fclose();

  fopen("/mailinglist");
  fputs("contact ");
  fputs(local); fputs("-help@"); fputs(host); fputs("; run by ezmlm\n");
  fclose();

  fopen("/owner");
  fputs(dir); fputs("/Mailbox\n");
  fputs("|"); fputs(auto_bin); fputs("/ezmlm-warn '"); fputs(dir);
  fputs("' || exit 0\n");
  fclose();

  fopen("/manager");
  fputs("|"); fputs(auto_bin); fputs("/ezmlm-manage '"); fputs(dir); fputs("'\n");
  fputs("|"); fputs(auto_bin); fputs("/ezmlm-warn '"); fputs(dir);
  fputs("' || exit 0\n");
  fclose();

  fopen("/editor");
  fputs("|"); fputs(auto_bin); fputs("/ezmlm-reject\n");
  fputs("|"); fputs(auto_bin); fputs("/ezmlm-send '"); fputs(dir); fputs("'\n");
  fputs("|"); fputs(auto_bin); fputs("/ezmlm-warn '"); fputs(dir);
  fputs("' || exit 0\n");
  fclose();

  fopen("/bouncer");
  fputs("|"); fputs(auto_bin); fputs("/ezmlm-warn '"); fputs(dir);
  fputs("' || exit 0\n");
  fputs("|"); fputs(auto_bin); fputs("/ezmlm-weed\n");
  fputs("|"); fputs(auto_bin); fputs("/ezmlm-return '"); fputs(dir); fputs("'\n");
  fclose();

  fopen("/headerremove");
  fputs("\
return-path\n\
return-receipt-to\n\
content-length\n\
");
  fclose();

  fopen("/headeradd");
  fclose();


  fopen("/text/top");
  fputs("Hi! This is the ezmlm program. I'm managing the\n");
  fputs(local); fputs("@"); fputs(host); fputs(" mailing list.\n\n");
  fclose();

  fopen("/text/bottom");
  fputs("\n--- Here are the ezmlm command addresses.\n\
\n\
I can handle administrative requests automatically.\n\
Just send an empty note to any of these addresses:\n\n   <");
  fputs(local); fputs("-subscribe@"); fputs(host); fputs(">:\n");
  fputs("   Receive future messages sent to the mailing list.\n\n   <");
  fputs(local); fputs("-unsubscribe@"); fputs(host); fputs(">:\n");
  fputs("   Stop receiving messages.\n\n   <");
  fputs(local); fputs("-get.12345@"); fputs(host); fputs(">:\n");
  fputs("   Retrieve a copy of message 12345 from the archive.\n\
\n\
DO NOT SEND ADMINISTRATIVE REQUESTS TO THE MAILING LIST!\n\
If you do, I won't see them, and subscribers will yell at you.\n\
\n\
To specify God@heaven.af.mil as your subscription address, send mail\n\
to <");
  fputs(local); fputs("-subscribe-God=heaven.af.mil@"); fputs(host);
  fputs(">.\n\
I'll send a confirmation message to that address; when you receive that\n\
message, simply reply to it to complete your subscription.\n\
\n");
  fputs("\n--- Below this line is a copy of the request I received.\n\n");
  fclose();

  fopen("/text/sub-confirm");
  fputs("To confirm that you would like\n\
\n\
!A\n\
\n\
added to this mailing list, please send an empty reply to this address:\n\
\n\
!R\n\
\n\
Your mailer should have a Reply feature that uses this address automatically.\n\
\n\
This confirmation serves two purposes. First, it verifies that I am able\n\
to get mail through to you. Second, it protects you in case someone\n\
forges a subscription request in your name.\n\
\n");
  fclose();

  fopen("/text/unsub-confirm");
  fputs("To confirm that you would like\n\
\n\
!A\n\
\n\
removed from this mailing list, please send an empty reply to this address:\n\
\n\
!R\n\
\n\
Your mailer should have a Reply feature that uses this address automatically.\n\
\n\
I haven't checked whether your address is currently on the mailing list.\n\
To see what address you used to subscribe, look at the messages you are\n\
receiving from the mailing list. Each message has your address hidden\n\
inside its return path; for example, God@heaven.af.mil receives messages\n\
with return path ...-God=heaven.af.mil.\n\
\n");
  fclose();

  fopen("/text/sub-ok");
  fputs("Acknowledgment: I have added the address\n\
\n\
!A\n\
\n\
to this mailing list.\n\
\n");
  fclose();

  fopen("/text/unsub-ok");
  fputs("Acknowledgment: I have removed the address\n\
\n\
!A\n\
\n\
from this mailing list.\n\
\n");
  fclose();

  fopen("/text/sub-nop");
  fputs("Acknowledgment: The address\n\
\n\
!A\n\
\n\
is on this mailing list.\n\
\n");
  fclose();

  fopen("/text/unsub-nop");
  fputs("Acknowledgment: The address\n\
\n\
!A\n\
\n\
is not on this mailing list.\n\
\n");
  fclose();

  fopen("/text/sub-bad");
  fputs("Oops, that confirmation number appears to be invalid.\n\
\n\
The most common reason for invalid numbers is expiration. I have to\n\
receive confirmation of each request within ten days.\n\
\n\
I've set up a new confirmation number. To confirm that you would like\n\
\n\
!A\n\
\n\
added to this mailing list, please send an empty reply to this address:\n\
\n\
!R\n\
\n\
Sorry for the trouble.\n\
\n");
  fclose();

  fopen("/text/unsub-bad");
  fputs("Oops, that confirmation number appears to be invalid.\n\
\n\
The most common reason for invalid numbers is expiration. I have to\n\
receive confirmation of each request within ten days.\n\
\n\
I've set up a new confirmation number. To confirm that you would like\n\
\n\
!A\n\
\n\
removed from this mailing list, please send an empty reply to this address:\n\
\n\
!R\n\
\n\
Sorry for the trouble.\n\
\n");
  fclose();

  fopen("/text/get-bad");
  fputs("Sorry, I don't see that message.\n\n");
  fclose();

  fopen("/text/bounce-bottom");
  fputs("\n\
--- Below this line is a copy of the bounce message I received.\n\n");
  fclose();

  fopen("/text/bounce-warn");
  fputs("\n\
Messages to you seem to have been bouncing. I've attached a copy of\n\
the first bounce message I received.\n\
\n\
If this message bounces too, I will send you a probe. If the probe bounces,\n\
I will remove your address from the mailing list, without further notice.\n\
\n");
  fclose();

  fopen("/text/bounce-probe");
  fputs("\n\
Messages to you seem to have been bouncing. I sent you a warning\n\
message, but it bounced. I've attached a copy of the bounce message.\n\
\n\
This is a probe to check whether your address is reachable. If this\n\
probe bounces, I will remove your address from the mailing list, without\n\
further notice.\n\
\n");
  fclose();

  fopen("/text/bounce-num");
  fputs("\n\
I've kept a list of which messages bounced from your address. Copies of\n\
these messages may be in the archive. To get message 12345 from the\n\
archive, send an empty note to ");
  fputs(local); fputs("-get.12345@"); fputs(host); fputs(".\n\
Here are the message numbers:\n\
\n");
  fclose();

  fopen("/text/help");
  fputs("\
This is a generic help message. The message I received wasn't sent to\n\
any of my command addresses.\n\
\n");
  fclose();

  fopen("/key");
  fput(key.s,key.len);
  fclose();

  _exit(0);
}
