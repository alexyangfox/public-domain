/* +++Date last modified: 05-Jul-1997 */

/*
**  NLCNVRT.C - A utility to convert newlines in text files
**
**  public domain by Bob Stout
**  Mac support by Norman Dodge and Bob Stout
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"
#include "sniptype.h"

FILE *infile;
FILE *outfile;

static void usage(void);
static void dos2unix(void);
static void dos2mac(void);
static void unix2dos(void);
static void unix2mac(void);
static void mac2dos(void);
static void mac2unix(void);
static void Cout(int ch, FILE *outfile);
static void Sout(char *str, FILE *outfile);

main(int argc, char *argv[])
{
      char outname[L_tmpnam] = "";

      if (argc < 3)
            usage();
      if (NULL == strchr("-/", argv[1][0]))
            usage();

      infile = cant(argv[2], "rb");

      if (3 == argc || NULL == (outfile = fopen(argv[3], "wb")))
      {
            tmpnam(outname);
            outfile = cant(outname, "wb");
      }

      switch (argv[1][1])
      {
      case 'd':
            unix2dos();
            break;

      case 'D':
            mac2dos();
            break;

      case 'u':
            dos2unix();
            break;

      case 'U':
            mac2unix();
            break;

      case 'm':
            dos2mac();
            break;

      case 'M':
            unix2mac();
            break;

      default:
            usage();
      }
      fclose(infile);
      fclose(outfile);
      if (*outname)
      {
            if (Success_ != remove(argv[2]))
            {
                  remove(outname);
                  ErrExit("Can't remove %s", argv[2]);
            }
            rename(outname, argv[2]);
      }
      return EXIT_SUCCESS;
}

void usage(void)
{
      puts("Usage: NLCNVRT -[d | u | m | D | U | M] infile [outfile]");
      puts("Switches: -d  Convert to DOS  from Unix - converts LF   => CRLF");
      puts("        : -u  Convert to Unix from DOS  - converts CRLF => LF");
      puts("        : -m  Convert to Mac  from DOS  - converts CRLF => CR");
      puts("        : -D  Convert to DOS  from Mac  - converts CR   => CRLF");
      puts("        : -U  Convert to Unix from Mac  - converts CR   => LF");
      puts("        : -M  Convert to Mac  from Unix - converts LF   => CR");
      exit(EXIT_FAILURE);
}

void unix2dos(void)
{
      int ch, lastch = 0;

      while (EOF != (ch = fgetc(infile)))
      {
            if ('\n' == ch && '\r' != lastch)
                  fputc('\r', outfile);
            fputc(lastch = ch, outfile);
      }
      if ('\n' != lastch)
            fputs("\r\n", outfile);
}

void dos2unix(void)
{
      int ch, lastch = 0;

      while (EOF != (ch = fgetc(infile)) && '\x1a' != ch)
      {
            if ('\r' != ch)
                  fputc(lastch = ch, outfile);
      }
      if ('\n' != lastch)
            fputc('\n', outfile);
}

void mac2dos(void)
{
      int ch, lastch = 0;

      while (EOF != (ch = fgetc(infile)))
      {
            fputc(lastch = ch, outfile);
            if ('\r' == ch)
                  fputc('\n', outfile);
      }
      if ('\r' != lastch)
            fputs("\r\n", outfile);
}

static void dos2mac(void)
{
      int ch, lastch = 0;

      while (EOF != (ch = fgetc(infile)) && '\x1a' != ch)
      {
            if ('\r' != ch)
            {
                  if ('\n' == ch)
                        ch = '\r';
                  fputc(lastch = ch, outfile);
            }
      }
      if ('\r' != lastch)
            fputc('\r', outfile);
}

static void unix2mac(void)
{
      int ch, lastch = 0;

      while (EOF != (ch = fgetc(infile)))
      {
            if ('\n' == ch)
                  ch = '\r';
            fputc(lastch = ch, outfile);
      }
      if ('\r' != lastch)
            fputc('\r', outfile);
}

static void mac2unix(void)
{
      int ch, lastch = 0;

      while (EOF != (ch = fgetc(infile)))
      {
            if ('\r' == ch)
                  ch = '\n';
            fputc(lastch = ch, outfile);
      }
      if ('\n' != lastch)
            fputc('\n', outfile);
}

void Cout(int ch, FILE *outfile)
{
      if (EOF == fputc(ch, outfile))
            exit(EXIT_FAILURE);
}

void Sout(char *str, FILE *outfile)
{
      if (EOF == fputs(str, outfile))
            exit(EXIT_FAILURE);
}
