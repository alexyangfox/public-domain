/*************************************************************************
|                                                                        |
|   MAIN.C                                                      04.10.89 |
|   PAMAKE Utility:  main program                                        |
|                                                                        |
*************************************************************************/

#define VV "1.80"
#define DD "04 Oct 89"

#ifdef VMS
#include stdio
#include "h.h"
#include string
#include errno
#endif

#ifdef LATTICE
#include <stdio.h>
#include <dos.h>
#include "h.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#endif

#ifdef __TURBOC__
#include <stdio.h>
#include <dos.h>
#include <io.h>
#include <dir.h>
#include "h.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#endif

#ifdef MSC
#include <stdio.h>
#include <dos.h>
#include <io.h>
#include <direct.h>
#include "h.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#endif

char *          myname;             /* Name of this program */
char *          makefile;           /* The make file  */

bool            confirm = FALSE;    /* Re-stat target after make */
bool            domake = TRUE;      /* Go through the motions option  */
bool            ignore = FALSE;     /* Ignore exit status option  */
bool            silent = FALSE;     /* Silent option  */
bool            print = FALSE;      /* Print debuging information  */
bool            rules = TRUE;       /* Use inbuilt rules  */
bool            dotouch = FALSE;    /* Touch files instead of making  */
bool            quest = FALSE;      /* Question up-to-dateness of file  */
bool            display = FALSE;    /* Display times */
uchar           macrotype = '\0';   /* Environment, Command-line, etc */

FILE *          ifile[4] = {0};             /* input files */
int             fln[4] = {0,0,0,0};         /* input file line numbers */
char            fname[4][80] = {"stdin"};   /* input file names */
int             nestlvl = 0;                /* nesting level */
char            ifmessage = '#';            /* 'if' statement lead-in */
unsigned char   pamakeos2 = 0;       /* 0 = DOS (or VMS), 1 = OS2 */

#ifdef PAMOS2
    extern far pascal DosGetMachineMode(unsigned char far *);
#define OMITBRK
#endif

#ifndef VMS
#ifndef OMITBRK
static union REGS regs;         /* registers for BIOS calls */
static unsigned int svbreak;    /* save break state */

void cleanbrk(void)
{
    regs.x.ax = 0x3301;         /* set break state */
    regs.x.dx = svbreak;        /* save state */
    intdos(&regs,&regs);        /* do the dos call */
}
#endif
#endif

void
usage(void)
{
    fprintf(stderr, "Usage: %s [-f makefile] [-deinpqrst] [macro=val ...] [target(s) ...]\n", myname);
    exit(PXERROR);
}

#ifndef __TSC__
void
fatal(msg, a1, a2, a3)
char *      msg;
{
    fprintf(stderr, "%s: ", myname);
    fprintf(stderr, msg, a1, a2, a3);
    fputc('\n', stderr);
    exit(PXERROR);
}
#endif

/* local strstr, some compiler libraries dont have it */

char *
pstrstr(char * source, char * target)
{
    register char *         s1;
    register char *         t;

    for ( ; *source ; ++source)         /* look along source */
    {
        for (s1 = source, t = target; *s1 == *t; ++s1)
        {
            if (*++t == '\0') return source;    /* match if hit end of targ */
        }
    }
    return (char *)0;
}

#ifndef VMS
void
#endif
main(argc, argv, envp)
char * *        argv;
int             argc;
char * *        envp;
{
    register char *         p;          /*  For argument processing  */
    int                     estat = 0;  /*  For question  */
    register struct name *  np;
    int                     i;
    char *                  makeflags;
    int                     egiven = 0;

    myname = "pamake";

    macrotype = M_ENV;          /* doing enviroment macros */ 
    while(*envp) 
    {
        p = strchr(*envp,'=');  /* find = in environment string */
        if (p == NULL)          /* no '=' in environment string */
        {                       /* never happens with COMMAND.COM */
            envp++;             /* but needed for MKS Korn Shell */
            continue;
        }   
        i = (int)(p - *envp);
        strncpy(str1,*envp++,i);
        str1[i] = '\0';
        setmacro(str1,p+1);
    }

#ifdef PAMOS2
    DosGetMachineMode((uchar far *)&pamakeos2);
    if (pamakeos2) setmacro("PAMAKE_OS","OS2");
    else setmacro("PAMAKE_OS","DOS");
#endif
#ifdef PAMDOS
    setmacro("PAMAKE_OS","DOS");
#endif
#ifdef VMS
    setmacro("PAMAKE_OS","VMS");
#endif    

    macrotype = '\0';

    makeflags = getenv("MAKEFLAGS");
    if (makeflags) argv[0] = makeflags;
    else argv[0] = "";

    while (argc > 0) 
    {
        if (**argv == '\0')     /* empty makeflags */
        {
            argc--;             /*  One less to process  */
            argv++;  
        } 
        else if (**argv == '-')
        {
            argc--;             /*  One less to process  */
            p = *argv++;        /*  Now processing this one  */

            while (*++p != '\0')
            {
                switch(*p)
                {
                case 'c':       /*  confirm timestamp after make */
                    confirm++;
                    break;
                case 'd':       /*  debug with timestamps */
                    display++;
                    break;
                case 'e':       /*  Enviroment macros prevail */
                    egiven++;
                    break;
                case 'f':       /*  Alternate file name  */
                    if (*(argv-1) == makeflags) break;
                    if (argc-- <= 0) usage();
                    makefile = *argv++;
                    break;
                case 'n':       /*  Pretend mode  */
                    domake = FALSE;
                    break;
                case 'i':       /*  Ignore fault mode  */
                    ignore = TRUE;
                    break;
                case 's':       /*  Silent about commands  */
                    silent = TRUE;
                    break;
                case 'p':
                    if (*(argv-1) != makeflags) print = TRUE;
                    break;
                case 'r':
                    rules = FALSE;
                    break;
                case 't':
                    dotouch = TRUE;
                    break;
                case 'q':
                    quest = TRUE;
                    break;
                case 'v':
                    fprintf(stderr, "\nPAMAKE Version: %s Date: %s\n\n", VV, DD);
                    /* drop through */
                default:        /*  Wrong option  */
                    usage();
                }
            }
        }
        else if ((p = strchr(*argv, '=')) != NULL) 
        {
            char            c;

            c = *p;
            *p = '\0';
            macrotype = M_PERM;     /* doing command-line macros */ 
            setmacro(*argv, p+1);
            macrotype = '\0';
            *p = c;

            argv++;
            argc--;
        }
        else break;
    }

    if ((makefile != NULL) && (strcmp(makefile, "-") == 0))  
        ifile[0] = stdin;
    else if (!makefile)             /*  If no file, then use default */
    {
        if ((ifile[0] = fopen(DEFN1, "r")) == (FILE *)0)
        {
            fatal("Unable to open file %s", DEFN1);
        }
        strncpy(fname[0],DEFN1,80);
    }
    else
    {
        if ((ifile[0] = fopen(makefile, "r")) == (FILE *)0)
            fatal("Unable to open file %s", makefile);
        strncpy(fname[0],makefile,80);
    }

    if (egiven) markmacros();       /* make env macros perm */
    if (rules) makerules();         /* use builtin rules */
#ifndef VMS
    setmacro("\\","\\");
    setmacro("HOME",getcwd((char *)0,64));
    if (pamakeos2 == 0)
    {
#ifndef OMITBRK
        regs.x.ax = 0x3300;         /* get break state */
        intdos(&regs,&regs);        /* do the dos call */
        svbreak = regs.x.dx;        /* save state */
        regs.x.ax = 0x3301;         /* set break state */
        regs.x.dx = 0x0001;         /* turn it on */
        intdos(&regs,&regs);        /* do the dos call */
#endif
    }
#ifndef OMITBRK
    atexit(cleanbrk);
#endif
#endif
#ifdef DOLLAR
    setmacro("$", "\244");          /* special high bit set */
#else
    setmacro("$", "$");
#endif
    setmacro("MAKE",myname);
    setmacro("-", LIFNAME);

    input();            /*  Input all the gunga  */
    lineno = 0;         /*  Any calls to error now print no line number */
    ifmessage = '%';    /*  if statement lead-in now '%' */

    np = newname(".SILENT");
    if (np->n_flag & N_TARG) silent = TRUE;

    np = newname(".IGNORE");
    if (np->n_flag & N_TARG) ignore = TRUE;

    p = str1;
    *p++ = '-';
    if (egiven) *p++ = 'e';
    if (ignore) *p++ = 'i';
    if (!domake) *p++ = 'n';
    if (quest) *p++ = 'q';
    if (!rules) *p++ = 'r';
    if (silent) *p++ = 's';
    if (dotouch) *p++ = 't';
    if (p == (str1+1)) p = str1;
    *p = '\0';
    setmacro("+MAKEFLAGS",str1);

    if (print) prt();       /*  Print out structures  */
    precious();
    if (!domake) silent = FALSE;
    if (!firstname) fatal("No targets found to build");
    circh();                /*  Check circles in target definitions  */

    if (!argc) estat = make(firstname, 0);
    else while (argc--) estat |= make(newname(*argv++), 0);

    if (quest) exit((estat)?PXERROR:PXNORMAL);
    else exit(PXNORMAL);
}

