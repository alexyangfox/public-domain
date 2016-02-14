/*************************************************************************
|                                                                        |
|   MAKE.C                                                      30.09.89 |
|   PAMAKE Utility:  do the actual making                                |
|                                                                        |
*************************************************************************/

#ifdef VMS
#include stdio
#include processes
#include "h.h"
#include errno
#include ctype
#include string
#include stdlib
#include types
#include stat
#endif

#ifdef LATTICE
#include <stdio.h>
#include <fcntl.h>
#include <dos.h>
#include "h.h"
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#endif

#ifdef __TURBOC__
#include <stdio.h>
#include <fcntl.h>
#include <dos.h>
#include "h.h"
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <process.h>
#endif

#ifdef MSC
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <process.h>
#include <dos.h>
#include "h.h"
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#endif

#define ARGN 64

static int  lifmade = 0;
static char lifname[128] = "";

#ifdef DOLLAR
static char dollarbuf[1024];
#endif

#ifdef LATTICE
#define O_TEXT 0
#endif

#ifdef PAMDOS
#define TESTINTERNAL 8      /* number of internal commands to test for */ 
#endif
#ifdef PAMOS2
#define TESTINTERNAL 8      /* number of internal commands to test for */ 
#endif

                            /* Dos internal commands, augment list if reqd */
#ifdef TESTINTERNAL
static char * internal[] =
{
    "ECHO",
    "DEL",
    "ERASE",
    "MD",
    "MKDIR",
    "REN",
    "RENAME",
    "COPY",
};
#endif

/*****  remove special $ markers */

#ifdef DOLLAR

char *
dollar(p)
char *          p;
{
    register char *         q = dollarbuf;
    
    while (*p)
    {
        if ((*p & 0xff) == 0244) 
        {
            *q++ = '$';
            p++;
        }
        else *q++ = *p++;
    }
    *q = '\0';
    return dollarbuf;
}

#endif

/****   test for common DOS and OS/2 internal commands, true if found */

#ifdef TESTINTERNAL

int dos_internal(s)
char *          s;
{
    register int            i;
    char *                  p;
    char *                  q;

    while ((*s == ' ') && (*s != '\0')) s++;
    for (i = 0; i < TESTINTERNAL; i++)
    {
        p = s;
        q = internal[i];
        while ( (*p != ' ') && ( (*p & 0x5f) == *q )) p++,q++;
        if ((*p == ' ') && (*q == '\0')) return 1;
    }
    return 0;
}

#endif

/*****  do the command */

int
dosh(p, shell)
char *          p;
int             shell;
{
#ifdef VMS

    int result;

    result = 0xffff & system(dollar(p));
    if (result == 33384) return 0;      /* LINK: compilation warnings */
    if (result == PXNORMAL) return 0;
    return result;
    
#else

    int                     argc;
    char *                  argv[ARGN];
    char                    c;
    char *                  q;
    char                    token[64];
    register int            i;
    char *                  mode;
    int                     ret;
    int                     r0,r1,r2;     

    p = dollar(p);
    if (shell) return system(p);

#ifdef TESTINTERNAL         /* must be DOS or OS/2 */

    r0 = r1 = r2 = 0;       /* clear redirection switches */
    if ((q = pstrstr(p,"<")) != (char *)0)
    {
        i = 0;
        *q++ = ' ';
        while (*q == ' ') q++;
        while (!pspace(*q) && (*q != '\0') && (i < 63))
        {
            token[i++] = *q;
            *q++ = ' ';
        }
        token[i] = '\0';
        if (0 == freopen(token,"r",stdin)) return 999;
        r0 = 1;
    }
    if ((q = pstrstr(p,"2>")) != (char *)0)
    {
        i = 0;
        mode = "w";
        *q++ = ' ';
        *q++ = ' ';
        if (*q == '>')
        {
            mode = "a";
            *q++ = ' ';
        }
        while (*q == ' ') q++;
        while (!pspace(*q) && (*q != '\0') && (i < 63))
        {
            token[i++] = *q;
            *q++ = ' ';
        }
        token[i] = '\0';
        if (0 == freopen(token,mode,stderr)) return 999;
        r2 = 1;
    }
    if ((q = pstrstr(p,">")) != (char *)0)
    {
        i = 0;
        mode = "w";
        *q++ = ' ';
        if (*q == '>')
        {
            mode = "a";
            *q++ = ' ';
        }
        while (*q == ' ') q++;
        while (!pspace(*q) && (*q != '\0') && (i < 63))
        {
            token[i++] = *q;
            *q++ = ' ';
        }
        token[i] = '\0';
        if (0 == freopen(token,mode,stdout)) return 999;
        r1 = 1;
    }
#endif

    for (argc = 0; argc < ARGN-1; argc++) argv[argc] = 0;
    for (argc = 0; argc < ARGN-1; )
    {
        while(pspace(*p)) p++;
        if (*p == '\0') break;
        argv[argc++] = p;
        while ((*p != '\0') && (! pspace(*p))) p++;
        c = *p;
        *p++ = '\0';
        if (c == '\0') break;
    }

#ifdef LATTICE
    if (-1 == forkvp(argv[0],argv)) ret = -1;
    else
    {
        ret = wait();
        if ((ret >> 8) == 1) ret = 4;
        else if (ret) ret &= 0xff;
    }
#else
    ret = spawnvp(P_WAIT,argv[0],argv);
#endif

#ifdef TESTINTERNAL         /* must be DOS or OS/2 */

    if (r0) freopen("CON","r",stdin);
    if (r1) freopen("CON","w",stdout);
    if (r2) freopen("CON","w",stderr);

#endif
    return ret;
#endif
}

/*****  create the local input file */

void
makelif(cp)
struct cmd *    cp;
{
    FILE *                  lifile;
    struct lif *            lp;
    struct macro *          mp;
    char *                  p = lifname;
    char *                  q;
    static char *           ermsg = {"Unable to write local input file"};

    lifmade = 0;
    if ( cp->c_lif == (struct lif *)0 ) return;
    if ((mp = getmp("-")) != (struct macro *)0)
    {
        if (mp->m_sub) 
        {
            strncpy(lifname,mp->m_sub,127);
            free(mp->m_sub);
        }
        else 
            strncpy(lifname,mp->m_val,127);
    }
    else
        lifname[0] = '\0';
    while (*p == ' ') p++;
#ifdef DOLLAR
    strcpy(lifname,dollar(lifname));
#endif
    if (*p == '\0') strcpy(lifname,LIFNAME);
    if ( (lifile = fopen(lifname,"w")) == NULL) fatal(ermsg);
    for (lp = cp->c_lif; lp; lp = lp->f_next)
    {
        strcpy(str2, lp->f_lif);
        expand(str2);
        if (!strcmp(str2,"\\<")) strcpy(str2,"<");
        while ((q = pstrstr(str2,"\\n")) != (char *)0)
        {
            *q++ = '\n';
            while (*++q) *(q-1) = *q;
            *--q = '\0';
        }
        if (fputs(dollar(str2),lifile) == EOF) fatal(ermsg);
        if (fputs("\n",lifile) == EOF) fatal(ermsg);
    }
    if (fclose(lifile) == EOF) fatal(ermsg);
    lifmade = 1;
}

/*****  kill the local input file */

void
killlif()
{
    if (lifmade) unlink(lifname);
}

/*****  do the %set command */

void
dosetcmd(p)
char *          p;
{
    char *                  q;
    char *                  pp;

    while (pspace(*p)) p++;    /* find first target */

    pp = p;
    while (((q = strchr(p, '=')) != (char *)0) &&
        (p != q) && (q[-1] == '\\'))        /*  Find value */
    {
        register char * a;

        a = q - 1;              /* del \ chr; move rest back */
        p = q;
        while ((*a++ = *q++) != '\0') ;
    }

    if (q != (char *)0)
    {
        register char * a;

        *q++ = '\0';            /* separate name and val */
        while (pspace(*q)) q++;
        if ((a = gettok(&pp)) == (char *)0)
            error("Macro definition without macro name");
        setmacro(a, q);
    }
}

/*****  do the %stat command */

void
dostatcmd(p)
char *          p;
{
    struct name *           q;

    while (pspace(*p)) p++;    /* find filename */
    q = newname(p);
    if (q->n_flag & N_TARG)
    {
        modtime(q);
        q->n_flag &= ~N_DONE;
    }
}

/*****  do a set of commands to make a target */

void
docmds1(np,lp)
struct name *   np;
struct line *   lp;
{
    bool                    ssilent;
    bool                    signore;
    bool                    sdomake;
    int                     estat;
    register char *         q;
    register char *         p;
    int                     shell;
    register struct cmd *   cp;
    int                     specialhash;

    for (cp = lp->l_cmd; cp; cp = cp->c_next)
    {
        shell = 0;
        ssilent = silent;
        signore = ignore;
        sdomake = domake;
        strcpy(str1, cp->c_cmd);
        if (!strncmp(str1,"%ifn",4)) specialhash = 2;
        else if (!strncmp(str1,"%if",3)) specialhash = 1;
        else if (!strncmp(str1,"%else",5)) specialhash = 3;
        else if (!strncmp(str1,"%endif",6)) specialhash = 4;
        else if (!strncmp(str1,"%set",4)) specialhash = 5;
        else if (!strncmp(str1,"%exit",5)) specialhash = 6;
        else if (!strncmp(str1,"%stat",5)) specialhash = 7;
        else specialhash = 0;

        if (ifproc(str1,specialhash)) continue;
        if (specialhash == 6) exit(atoi(str1+5));
        if (specialhash == 5)
        {
            dosetcmd(str1+4);
            continue;
        }
        if (specialhash == 7)
        {
            dostatcmd(str1+5);
            continue;
        }

        if (pstrstr(str1,"$(MAKE)")) sdomake = TRUE;
        expand(str1);
        q = str1;
        while ((*q == '@') || (*q == '-') || (*q == '+') || (*q == '\\'))
        {
            if (*q == '@') ssilent = TRUE;      /* specific silent */
            else if (*q == '-') signore = TRUE; /* specific ignore */
            else if (*q == '+') shell = 1;      /* specific shell */
            else                                /* must be \ */
            {
                q++;                            /* skip past \ */
                if ((*q == '@') || (*q == '-') || (*q == '+')) break;
                else                            /* \ is a real char */
                {
                    q--;                        /* make it start command */
                    break;                      /* no more specials */
                }
            }
            q++;                                /* skip past special char */
        }
#ifdef TESTINTERNAL
        if ( dos_internal(q) ) shell = 1;
#endif

        if (!ssilent) fputs("    ", stdout);

        for (p = q; *p; p++)
        {
            if (*p == '\n' && p[1] != '\0')
            {
                *p = ' ';
                if (!ssilent) fputs("\\\n", stdout);
            }
            else if (!ssilent)
            {
#ifdef DOLLAR
                if ((*p & 0xff) == 0244) 
                    putchar('$');
                else
#endif
                    putchar(*p);
            }
        }
        if (!ssilent) putchar('\n');

        if (sdomake)
        {                       /*  Get the shell to execute it  */
            makelif(cp);
            estat = dosh(q, shell);
            killlif();
#ifdef PAMDOS
            bdos(0xb,0,0);    /* check control break */
#endif
            if (estat != 0)
            {
                if (shell)
                {
                    if (pamakeos2 & signore) fputs("(Ignored)\n",stdout);
                    else
                        fatal("Couldn't execute shell");
                }
                else
                {
                    if (estat==4) printf("%s: Interrupted", myname);
                    else if (estat==-1) printf("%s: Couldn't execute command",myname);
                    else if (estat==999) printf("%s: Redirection error",myname);
                    else printf("%s: Error return %d", myname, estat);
                    if ( (signore || !domake) && (estat != 4))
                        fputs(" (Ignored)\n", stdout);
                    else 
                    {
                        putchar('\n');
                        if (!(np->n_flag & N_PREC) && (estat != -1))
                            if (unlink(dollar(np->n_name)) == 0)
                                printf("%s: '%s' removed\n", myname, dollar(np->n_name));
                        exit(estat);
                    }
                }
            }
        }
    }
    ifeoc();         /* check for unterminated if in cmds */
}

/***** do commands to make a target */

void
docmds(np)
struct name *   np;
{
    struct name *           dft;
    register struct line *  rp;
    struct line *           lp;
    bool                    hascmds = FALSE;

    for
        (
        rp = np->n_line;
        rp;
        rp = rp->l_next
        )
        if (rp->l_cmd) hascmds = TRUE;

    if (!hascmds)
    {
        dft = newname(".DEFAULT");
        if (dft->n_flag & N_TARG) np->n_line = dft->n_line;
    }

    for (lp = np->n_line; lp; lp = lp->l_next)
        docmds1(np,lp);
}



/*****  set $< and $* for the first dependancy name */
/*****  set $? to list of out-of-date dependency names */

void
setdmacros(np,qdp)
struct name *   np;
struct depend * qdp;
{
    char *                  p;
    char *                  q;
    char *                  suff;
    char *                  suffix();
    register struct depend *dp;
    register struct line *  lp;
    int                     found = 0;

    strcpy(str1, "");
    for (dp = qdp; dp; dp = qdp)
    {
        if ((strlen(str1) + strlen(dp->d_name->n_name) + 2) > LZ)
            fatal("$? macro is too long");
        if (strlen(str1)) strcat(str1, " ");
        strcat(str1, dp->d_name->n_name);
        qdp = dp->d_next;
        free(dp);
    }
    setmacro("?", str1);

    for (lp = np->n_line; lp; lp = lp->l_next)
        for (dp = lp->l_dep; dp; dp = dp->d_next)
        {
            if ( (!found) || (dp->d_name->n_flag & N_DYND) )
            {
                q = dp->d_name->n_name;
                setmacro("<",q);
                p = str1;
                suff = suffix(q);
                while (q < suff) *p++ = *q++;
                *p = '\0';
                setmacro("*",str1);
                if (found) return;
                found++;
            }
        }
}

/*****  clear the dynamic dep flag for all deps of this target */
/*****  (in case this dep is used again for another target) */

void
cleardynflag(np)
struct name *   np;
{
    register struct depend *dp;
    register struct line *  lp;

    for (lp = np->n_line; lp; lp = lp->l_next)
        for (dp = lp->l_dep; dp; dp = dp->d_next)
            dp->d_name->n_flag &= ~N_DYND;
}

/*****  make one thing */

void            
make1(np, lp, qdp)
struct depend *                qdp;
struct line *                  lp;
struct name *                  np;
{
    if (dotouch) touch(np);
    else
    {
        setmacro("@", np->n_name);  /* NOTE: *not* dollar() */
        setdmacros(np,qdp);
        if (lp) docmds1(np, lp);    /* lp set if doing a :: rule */
        else docmds(np);
    }
}

/*****  recursive routine to make a target */

int
make(np, level)
struct name *   np;
int             level;
{
    register struct depend *dp;
    register struct line *  lp;
    long                    dtime = 1;
    register struct depend *qdp;
    int                     didsomething = 0;

    if (np->n_flag & N_DONE)
    {
        if ( display ) dodisp(dollar(np->n_name),np->n_time);
        return 0;
    }

    if (!np->n_time)
        modtime(np);            /*  Gets modtime of this file  */

    if ( display ) dodisp(dollar(np->n_name),np->n_time);

    for (lp = np->n_line; lp; lp = lp->l_next)
        if (lp->l_cmd) break;
    if (!lp) dyndep(np);

    if (!(np->n_flag & N_TARG) && np->n_time == 0L)
        fatal("Don't know how to make %s", dollar(np->n_name));

    for (qdp = (struct depend *)0, lp = np->n_line; lp; lp = lp->l_next)
    {
        for (dp = lp->l_dep; dp; dp = dp->d_next)
        {
            make(dp->d_name, level+1);
#ifdef DEBUG
            printf("%s, %lx, %s, %lx\n",
                np->n_name,np->n_time, dp->d_name->n_name, dp->d_name->n_time);
#endif
            if (np->n_time < dp->d_name->n_time)
                qdp = newdep(dp->d_name, qdp);
            if (dtime < dp->d_name->n_time) dtime = dp->d_name->n_time; 
        }
        if (!quest && (np->n_flag & N_DOUBLE) && (np->n_time < dtime))
        {
            make1(np, lp, qdp);     /* free()'s qdp */
            dtime = 1;
            qdp = (struct depend *)0;
            didsomething++;
        }
    }

    np->n_flag |= N_DONE;

    if (quest)
    {
        long t;
        t = np->n_time;
        np->n_time = curtime();
        cleardynflag(np);
        return t < dtime;
    }
    else if (np->n_time < dtime && !(np->n_flag & N_DOUBLE))
    {
#ifdef DEBUG
            printf("%s, %lx, dtime, %lx\n",
                np->n_name,np->n_time, dtime);
#endif
        make1(np, (struct line *)0, qdp);
        if (confirm)
            modtime(np);
        else
        {
            np->n_time = curtime();
        }
    }
    else if (level == 0 && !didsomething)
        printf("%s: '%s' is up to date\n", myname, dollar(np->n_name));
    cleardynflag(np);
    return 0;
}
