/*************************************************************************
|                                                                        |
|   IFPROC.C                                                    31.08.89 |
|   PAMAKE Utility:  process conditionals                                |
|                                                                        |
*************************************************************************/

#define MAXIF 10

#ifdef VMS
#include stdio
#include string
#include stdlib
#include ctype
#include "h.h"
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "h.h"
#endif

int iftable[MAXIF+1] = {0};
int iflevel = 0;
extern char ifmessage;

#define NORMAL 0    /* No conditionals in force */
#define OKCOND 1    /* Processing lines, waiting for else or endif */
#define SKIP   2    /* Skipping lines, waiting for else or endif */
#define HADELSE 4   /* Had else, must be waiting for endif */

/*****  check clean eof */

void
ifeof()
{                     
    if (iflevel == 1) fatal("End of file - expected #endif");
    if (iflevel > 1) fatal("End of file - expected %d #endif's",iflevel);
}

/*****  check clean end of commands */

void
ifeoc()
{                     
    if (iflevel == 1) fatal("End of command set - expected %%endif");
    if (iflevel > 1) fatal("End of command set - expected %d %%endif's",iflevel);
}

/*****  check each line, return true if to be ignored */

int
ifproc(s,n)
char *          s;
int             n;
{
    register int            i;
    register int            test;

    switch (n) 
    {
        case 1:                                 /* IF */
        case 2:                                 /* IFN */
            if (iflevel >= (MAXIF))
                error("%cif and %cifn statements nested too deeply",
                    ifmessage,ifmessage);
            s += 4;
            while (pspace(*s)) s++;
            i = istrue(s);
            if (n == 2) i = !i;
            iftable[++iflevel] = 2 - i;
            return 1;
        case 3:                                 /* ELSE */
            i = iftable[iflevel];
            if ((i == NORMAL) || (i & HADELSE))
                error("%celse without %cif",ifmessage,ifmessage);
            if (i & OKCOND) i = SKIP | HADELSE;
            else i = OKCOND | HADELSE;
            iftable[iflevel] = i;
            return 1;
        case 4:
            if (iftable[iflevel])
            {
                iftable[iflevel] = NORMAL;
                iflevel--;
                return 1;
            }
            error("%cendif without %cif",ifmessage,ifmessage);
        default:
            test = 0;
            for (i = 0; i <= iflevel; i++) test |= (iftable[i] & SKIP);
            return (test);              /* true if skipping */
    }
}

/*****  check whether if statement is true */

int
istrue(s)
char *          s;
{
    char *                  p;              /*  General  */
    char *                  q;              /*  General  */
    struct macro *          mp;
    int                     r;

    p = s;

    while (((q = strchr(p, '=')) != (char *)0) &&
        (p != q) && (q[-1] == '\\'))        /*  Find value */
    {
        register char *         a;
        a = q - 1;      /*  Del \ chr; move rest back  */
        p = q;
        while ((*a++ = *q++) != '\0')
            ;
    }
    if (q != (char *)0)
    {
        register char *         a;

        *q++ = '\0';            /*  Separate name and val  */
        while (pspace(*q))
            q++;
        if ((p = strrchr(q, '\n')) != (char *)0)
            *p = '\0';
        p = s;
        if ((a = gettok(&p)) == (char *)0)
            error("Bad conditional on %cif statement",ifmessage);
        expand(q);
        if ((mp = getmp(a)) != (struct macro *)0)
        {
            if (mp->m_sub) 
            {
                r = strcmp(q,mp->m_sub);
                free(mp->m_sub);
            }
            else 
                r = strcmp(q,mp->m_val);
        }
        else
            r = strcmp(q,"");
        return (r == 0);
    }
    error("No conditional on %cif statement",ifmessage);
    return 0;                   /* not reached */
}
