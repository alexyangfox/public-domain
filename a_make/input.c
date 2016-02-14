/*************************************************************************
|                                                                        |
|   INPUT.C                                                     31.08.89 |
|   PAMAKE Utility:  parse a makefile                                    |
|                                                                        |
*************************************************************************/

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

struct name     namehead;
struct name *   firstname;

char            str1[LZ];               /* general store */
char            str2[LZ];

/*****  intern a name - return a pointer to the name struct */

struct name *
newname(name)
char *          name;
{
    struct name *           rp;
    struct name *           rrp;
    char *                  cp;
    register int            i;
    register char *         q;

    static char namerr[] = {"Cannot allocate memory for name"}; 

    for
        (
        rp = namehead.n_next, rrp = &namehead;
        rp;
        rp = rp->n_next, rrp = rrp->n_next
        )
    {
        i = 0;
        q = rp->n_name;
        while (name[i] == *(q+i))
            if (name[i++] == '\0') return rp;
    }

    if ((rp = (struct name *)malloc(sizeof(struct name))) == (struct name *)0)
        fatal(namerr);
    rrp->n_next = rp;
    rp->n_next = (struct name *)0;
    if ((cp = malloc(strlen(name)+1)) == (char *)0) fatal(namerr);
    strcpy(cp, name);
    rp->n_name = cp;
    rp->n_line = (struct line *)0;
    rp->n_time = (long)0;
    rp->n_flag = 0;

    return rp;
}


/*****  add a dependant to the end of the supplied list of dependants */
/*****  return the new head pointer for that list */

struct depend *
newdep(np, dp)
struct name *       np;
struct depend *     dp;
{
    register struct depend *rp;
    register struct depend *rrp;
    static char deperr[] = {"Cannot allocate memory for dependant"}; 

    if ((rp = (struct depend *)malloc(sizeof(struct depend)))
        == (struct depend *)0)
        fatal(deperr);
    rp->d_next = (struct depend *)0;
    rp->d_name = np;

    if (dp == (struct depend *)0)
        return rp;

    for (rrp = dp; rrp->d_next; rrp = rrp->d_next)
        ;

    rrp->d_next = rp;
    return dp;
}


/*****  add a command to the end of the supplied list of commands */
/*****  return the new head pointer for that list, and save the */
/*****  actual command structure created in case lif lines are found */

struct cmd *
newcmd(str, cp, crp)
char *              str;
struct cmd *        cp;
struct cmd * *      crp;
{
    register struct cmd *   rp;
    register struct cmd *   rrp;
    register char *         rcp;
    static char comerr[] = {"Cannot allocate memory for command"}; 

    if ((rcp = strrchr(str, '\n')) != (char *)0) *rcp = '\0';   /* lose nl */
    while (pspace(*str)) str++;

    if (*str == '\0')               /* if nothing left, then exit */
    {
        *crp = (struct cmd *)0;
        return cp;
    }

    if ((rp = (struct cmd *)malloc(sizeof(struct cmd))) == (struct cmd *)0)
        fatal(comerr);
    rp->c_next = (struct cmd *)0;
    if ((rcp = malloc(strlen(str)+1)) == (char *)0) fatal(comerr);
    strcpy(rcp, str);
    rp->c_cmd = rcp;
    rp->c_lif = (struct lif *)0;
    *crp = rp;

    if (cp == (struct cmd *)0) return rp;

    for (rrp = cp; rrp->c_next; rrp = rrp->c_next)
        ;

    rrp->c_next = rp;
    return cp;
}

/*****  add a lif line to the end of the supplied list of lif lines */
/*****  return the new head pointer for that list */

struct lif *
newlif(str, lp)
char *          str;
struct lif *    lp;
{
    register struct lif *   rp;
    register struct lif *   rrp;
    register char *         rlp;
    static char liferr[] = {"Cannot allocate memory for local input file"}; 

    if ((rlp = strrchr(str, '\n')) != (char *)0) *rlp = '\0';   /* lose nl */

    if (*str == '\0') return lp;    /* if nothing left, then exit */

    if ((rp = (struct lif *)malloc(sizeof(struct lif))) == (struct lif *)0)
        fatal(liferr);
    rp->f_next = (struct lif *)0;
    if ((rlp = malloc(strlen(str)+1)) == (char *)0) fatal(liferr);
    strcpy(rlp, str);
    rp->f_lif = rlp;

    if (lp == (struct lif *)0) return rp;
    for (rrp = lp; rrp->f_next; rrp = rrp->f_next)
        ;

    rrp->f_next = rp;
    return lp;
}


/*****  add a new 'line' of stuff to a target */
/*****  check to see if commands already exist for the target */

void
newline(np, dp, cp, flag)
struct name *   np;
struct depend * dp;
struct cmd *    cp;
int             flag;
{
    bool                    hascmds = FALSE;  /*  Target has commands  */
    register struct line *  rp;
    register struct line *  rrp;
    static char linerr[] = {"Cannot allocate memory for line"}; 

    if ( (!dp && !cp && !strcmp(np->n_name,".SUFFIXES")))
    {
        for (rp = np->n_line; rp; rp = rrp)
        {
            rrp = rp->l_next;
            free(rp);
        }
        np->n_line = (struct line *)0;
        np->n_flag &= ~N_TARG;
        return;
    }

    for
        (
        rp = np->n_line, rrp = (struct line *)0;
        rp;
        rrp = rp, rp = rp->l_next
        )
        if (rp->l_cmd)
            hascmds = TRUE;

/*****  handle the implicit rules redefinition case */

    if (hascmds && cp && !(np->n_flag & N_DOUBLE))
        if (np->n_name[0] == '.' && dp == (struct depend *)0)
        {
            np->n_line->l_cmd = cp;
            return;
        }
        else
            error("Multiple sets of commands for target %s", np->n_name);

    if (np->n_flag & N_TARG)
        if (!(np->n_flag & N_DOUBLE) != !flag)          /* like xor */
            error("Inconsistent rules for target %s", np->n_name);

    if ((rp = (struct line *)malloc(sizeof (struct line)))
        == (struct line *)0)
        fatal(linerr);
    rp->l_next = (struct line *)0;
    rp->l_dep = dp;
    rp->l_cmd = cp;

    if (rrp)
        rrp->l_next = rp;
    else
        np->n_line = rp;

    np->n_flag |= N_TARG;

    if (flag)
        np->n_flag |= N_DOUBLE;
}

/*****  check for the presence of a local input file */

void
checklif(cp)
struct cmd *    cp;
{
    register struct lif *   lp;

    lp = (struct lif *)0;
    if (pstrstr(str2,"$-"))    /* lif follows */
    {
        for (;;)
        {
            if (getline(str2))
                fatal("Unterminated local input file");
            if (str2[0] == '<') break;
            lp = newlif(&str2[0],lp);
        }
    }
    if (cp) cp->c_lif = lp;
}

/* expand $* in dependencies, by creating a new dep for each */

void
expast(struct name * np)
{
    register struct depend *dp;
    register struct line *  lp;
    struct name *           nnp;
    struct depend * *       dpp;
    char *                  p;
    char *                  q;
    char *                  r;
    char *                  rr;
    char *                  suff;
    char                    expb[64];
    static char             experr[] = {"Unable to expand $*"};

    for (lp = np->n_line; lp; lp = lp->l_next)
    {
        for (dp = lp->l_dep; dp; dp = dp->d_next)
        {
            rr = (char *)0;
            p = r = dp->d_name->n_name;
            while (*p)
            {
                if ( ((*p & 0xff) == 0244) && ((*(p + 1) & 0xff) == 0244) )
                {
                    rr = p;
                    break;
                }
                p++;
            }
            if (rr != (char *)0)
            {
                q = np->n_name;                 /* get target name */
                if ((strlen(q) + strlen(r)) > 60) fatal(experr);
                p = expb;
                while (r < rr) *p++ = *r++;
                suff = suffix(q);
                while (q < suff) *p++ = *q++;
                r += 2;                         /* skip special marker */
                while (*r) *p++ = *r++;
                *p = '\0';
                nnp = newname(expb);            /* make a new dependent */
                for (dpp = &(lp->l_dep); *dpp; dpp = &((*dpp)->d_next))
                {
                    if (*dpp == dp)
                    {
                        *dpp = (struct depend *)malloc(sizeof(struct depend));
                        if (*dpp == (struct depend *)0) fatal(experr);
                        (*dpp)->d_next = dp->d_next;
                        (*dpp)->d_name = nnp;
                    }
                }
            }
        }
    }
}

/*****  parse input from the makefile, and construct a tree structure of it */

void
input()
{
    char *                  p;              /*  General  */
    char *                  q;
    struct name *           np;
    struct depend *         dp;
    struct cmd *            cp;
    struct cmd *            crp;
    bool                    dbl;

    if (getline(str1)) return;      /* read the first line */
    setmacro("*","\244\244");       /* special marker for $* */

    for(;;)
    {
        if (*str1 == '\t')          /* rules without targets */
            error("Command found with no target/dependancy line");

        p = str1;
        while (pspace(*p)) p++;    /* find first target */

        while (((q = strchr(p, '=')) != (char *)0) &&
            (p != q) && (q[-1] == '\\'))        /*  Find value */
        {
            register char * a;

            a = q - 1;              /* del \ chr; move rest back */
            p = q;
            while ((*a++ = *q++) != '\0')
                ;
        }

        if (q != (char *)0)
        {
            register char * a;

            *q++ = '\0';            /* separate name and val */
            while (pspace(*q)) q++;
            if ((p = strrchr(q, '\n')) != (char *)0) *p = '\0';
            p = str1;
            if ((a = gettok(&p)) == (char *)0)
                error("Macro definition without macro name");
            setmacro(a, q);
            if (getline(str1)) return;
            continue;
        }

        expand(str1);
        p = str1;

        while (((q = strchr(p, ':')) != (char *)0) &&
            (p != q) && (q[-1] == '\\'))        /*  Find dependents  */
        {
            register char * a;

            a = q - 1;      /* del \ chr; move rest back */
            p = q;
            while ((*a++ = *q++) != '\0')
                ;
        }

        if (q == (char *)0) error("Cannot find any targets");

        *q++ = '\0';                /* separate targets and dependents */

        if (*q == ':')              /* double colon */
        {
            dbl = 1;
            q++;
        }
        else
            dbl = 0;

        for (dp = (struct depend *)0; ((p = gettok(&q)) != (char *)0);)
         
        {                           /* get list of dep's */
            np = newname(p);        /* intern name */
            dp = newdep(np, dp);    /* add to dep list */
        }

        *((q = str1) + strlen(str1) + 1) = '\0';    /* two nulls for gettok */
        cp = (struct cmd *)0;
        if (getline(str2) == FALSE) /* get commands */
        {
            while (*str2 == '\t')
            {
                cp = newcmd(&str2[0], cp, &crp);
                checklif(crp); 
                if (getline(str2)) break;
            }
        }

        while ((p = gettok(&q)) != (char *)0)   /* get list of targ's */
        {
            np = newname(p);                    /* intern name */
            newline(np, dp, cp, dbl);
            expast(np);                         /* expand $* in dep */
            if (!firstname && p[0] != '.') firstname = np;
        }

        if (nestlvl < 0) return;                /* eof? */
        strcpy(str1, str2);
    }
}
