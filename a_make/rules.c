/*************************************************************************
|                                                                        |
|   RULES.C                                                     31.08.89 |
|   PAMAKE Utility:  control of implicit suffix rules                    |
|                                                                        |
*************************************************************************/

#ifdef VMS
#include stdio
#include string
#include "h.h"
#else
#include <stdio.h>
#include <string.h>
#include "h.h"
#endif

/*****  return a pointer to the suffix of a name */

char *
suffix(name)
char *          name;
{
    register int i;
    int l;
    l = strlen(name);
    for (i = l - 1; i >= 0; i--)
    {
        if (name[i] == '.') return name + i;
    }
    return name + l - 1;
 }


/*****  dynamic dependency */

/*************************************************************************
|                                                                        |
|   This routine applies the suffix rules to try and find a source and   |
|   a set of rules for a missing target.  If found, np is made into a    |
|   target with the implicit source name and rules.  Returns TRUE if np  |
|   was made into a target.                                              |
|                                                                        |
*************************************************************************/

bool
dyndep(np)
struct name *   np;
{
    register char *         p;
    register char *         q;
    register char *         suff;           /*  Old suffix  */
    register char *         basename;       /*  Name without suffix  */
    struct name *           op;             /*  New dependent  */
    struct name *           sp;             /*  Suffix  */
    struct line *           lp;
    struct depend *         dp;
    char *                  newsuff;

    p = str1;
    q = np->n_name;
    suff = suffix(q);
    while (q < suff)
        *p++ = *q++;
    *p = '\0';
    basename = strcpy(str2,str1);

    if (!((sp = newname(".SUFFIXES"))->n_flag & N_TARG))
        return FALSE;

    for (lp = sp->n_line; lp; lp = lp->l_next)
        for (dp = lp->l_dep; dp; dp = dp->d_next)
        {
            newsuff = dp->d_name->n_name;
            if (strlen(suff)+strlen(newsuff)+1 >= LZ)
                fatal("Inference rule too long");
            p = str1;
            q = newsuff;
            while ((*p++ = *q++) != '\0') 
                ;
            p--;
            q = suff;
            while ((*p++ = *q++) != '\0') 
                ;
            sp = newname(str1);
            if (sp->n_flag & N_TARG)
            {
                p = str1;
                q = basename;
                if (strlen(basename) + strlen(newsuff)+1 >= LZ)
                    fatal("Inferred name too long");
                while ((*p++ = *q++) != '\0') 
                    ;
                p--;
                q = newsuff;
                while ((*p++ = *q++) != '\0') 
                    ;
                if ((np->n_line) && 
                    (pstrstr(q=np->n_line->l_dep->d_name->n_name,str1))) 
                    op=newname(q);
                else
                    op = newname(str1);
                if (!op->n_time) modtime(op);
                if ((op->n_time) || (op->n_flag & N_TARG) || dyndep(op))
                {
                    dp = newdep(op, 0);
                    newline(np, dp, sp->n_line->l_cmd,0);
                    op->n_flag |= N_DYND;
                    return TRUE;
                }
            }
        }
    return FALSE;
}


/***** make the default rules */

void
makerules()
{
    struct cmd *            cp;
    struct name *           np;
    struct depend *         dp;
    struct cmd *            rcp;

    cp = newcmd("$(CC) $(CFLAGS) $(CFILES)", 0,&rcp);
    np = newname(".c.obj");
    newline(np, 0, cp, 0);
    cp = newcmd("$(CC) $(CFLAGS) $(CFILES)", 0,&rcp);
    np = newname(".c.o");
    newline(np, 0, cp, 0);

    cp = newcmd("$(ASM) $(AFLAGS) $(AFILES)", 0,&rcp);
    np = newname(".asm.obj");
    newline(np, 0, cp, 0);
    cp = newcmd("$(ASM) $(AFLAGS) $(AFILES)", 0,&rcp);
    np = newname(".asm.o");
    newline(np, 0, cp, 0);

    cp = newcmd("$(FOR) $(FFLAGS) $(FFILES)", 0,&rcp);
    np = newname(".for.obj");
    newline(np, 0, cp, 0);

    cp = newcmd("$(BAS) $(BFLAGS) $(BFILES)", 0,&rcp);
    np = newname(".bas.obj");
    newline(np, 0, cp, 0);

    cp = newcmd("$(PAS) $(PFLAGS) $(PFILES)", 0,&rcp);
    np = newname(".pas.obj");
    newline(np, 0, cp, 0);

    cp = newcmd("$(GEN) $(GFLAGS) $(GFILES)", 0,&rcp);
    np = newname(".pnl.c");
    newline(np, 0, cp, 0);

    cp = newcmd("$(GEN) $(GFLAGS) $(GFILES)", 0,&rcp);
    np = newname(".pnl.for");
    newline(np, 0, cp, 0); 

    cp = newcmd("$(GEN) $(GFLAGS) $(GFILES)", 0,&rcp);
    np = newname(".pnl.pas");
    newline(np, 0, cp, 0); 

    np = newname(".c");
    dp = newdep(np, 0);
    np = newname(".asm");
    dp = newdep(np, dp);
    np = newname(".pnl");
    dp = newdep(np, dp);
    np = newname(".SUFFIXES");
    newline(np, dp, 0, 0);
}
