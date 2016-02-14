/*************************************************************************
|                                                                        |
|   MACRO.C                                                     31.08.89 |
|   PAMAKE Utility:  macro control functions                             |
|                                                                        |
*************************************************************************/

#ifdef VMS
#include stdio
#include string
#include stdlib
#include "h.h"
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "h.h"
#endif

/*****  do not include <ctypes.h> */

struct macro *          macrohead;

/*****  get macro, do substitutions */

struct macro *
getmp(name)
char *          name;
{
    register struct macro * rp;
    char *                  p;
    char *                  q;
    char *                  qq;
    char *                  qqq;
    char *                  sub1 = NULL;
    char *                  sub2 = NULL;
    char *                  a;

    p = strchr(name,':');
    if (p)
    {
        sub1 = p+1;
        *p = '\0';
        sub2 = strchr(sub1,'=');
        if (sub2) *sub2++ = '\0';
        else sub2 = "";
    }

    for (rp = macrohead; rp; rp = rp->m_next)
        if (strcmp(name, rp->m_name) == 0)
        {
            rp->m_sub = NULL;
            if (sub1)
            {
                a = malloc(LZ);
                if (!a) fatal("No memory for macro substitution");
                p = a;
                q = rp->m_val;
                while ((qq = pstrstr(q,sub1)) != (char *)0)
                {
                    for (qqq = q; qqq < qq;) *p++ = *qqq++;
                    q = qq + strlen(sub1);
                    qqq = sub2;
                    while (*qqq) *p++ = *qqq++;
                }
                if (q != rp->m_val)
                {
                    strcpy(p,q);
                    rp->m_sub = a;
                }
                else
                    free(a);
            }
            return rp;
        }
    return (struct macro *)0;
}

/*****  Do the dirty work for expand */

void
doexp(to, from, len, buf)
char * *        to;
char *          from;
int *           len;
char *          buf;
{
    register char *         rp;
    register char *         p;
    register char *         q;
    register struct macro * mp;

    rp = from;
    p = *to;
    while (*rp)
    {
        if (*rp != '$')
        {
            *p++ = *rp++;
            (*len)--;
        }
        else
        {
            q = buf;
            if (*++rp == '{')
                while (*++rp && *rp != '}')
                    *q++ = *rp;
            else if (*rp == '(')
                while (*++rp && *rp != ')')
                    *q++ = *rp;
            else if (!*rp)
            {
                *p++ = '$';
                break;
            }
            else
                *q++ = *rp;
            *q = '\0';
            if (*rp)
                rp++;
            if (!(mp = getmp(buf)))
                mp = setmacro(buf, "");
            if (mp->m_flag & M_LOOPCHK)
                fatal("Macro %s cannot be expanded: infinite recursion", mp->m_name);
            mp->m_flag = M_LOOPCHK;
            *to = p;
            if (mp->m_sub)
            {
                doexp(to, mp->m_sub, len, buf);
                free(mp->m_sub);
            }
            else
                doexp(to, mp->m_val, len, buf);
            p = *to;
            mp->m_flag &= ~M_LOOPCHK;
        }
        if (*len <= 0)
            error("Macro expansion too long");
    }
    *p = '\0';
    *to = p;
}

/*****  expand any macros in str */

void
expand(str)
char *          str;
{
    static char             a[LZ];
    static char             b[LZ];
    char *                  p = str;
    int                     len = LZ-1;

    strcpy(a, str);
    doexp(&p, a, &len, b);
}


/*****  mark all M_ENV macros as permanent (M_PERM) */

void
markmacros()
{
    register struct macro * rp;

    for (rp = macrohead; rp; rp = rp->m_next)
    {
        if (rp->m_flag & M_ENV) rp->m_flag |= M_PERM;
    }
}

/*****  set macro value */

struct macro *
setmacro(name, val)
char *          name;
char *          val;
{
    register struct macro * rp;
    register char *         cp;
    register int            i,j;
    static char             buf[LZ];
    static char             macerr[] = {"Cannot allocate memory for macro"};

    if (*name == '+') 
    {
        name++;
        if ( (strlen(name) + strlen(val)) < 126)
        {
            for (i = 0, j = 0; name[j]; ) buf[i++] = (char)(toupper(*(name+(j++))));
            buf[i++] = '=';
            for (j = 0; *(val+j); ) buf[i++] = *(val+(j++));
            buf[i++] = '\0';
            expand(buf);
#ifndef VMS
            putenv(strdup(buf));
#endif            
        }
    }

/*****  replace macro definition if it exists  */
/*****  don't let macros derived from external env. be replaced if -e given */

    for (rp = macrohead; rp; rp = rp->m_next)
        if (strcmp(name, rp->m_name) == 0)
        {
            if (rp->m_flag & M_PERM) return rp;
            free(rp->m_val);        /*  Free space from old  */
            break;
        }

    if (!rp)                /*  If not defined, allocate space for new  */
    {
        if ((rp = (struct macro *)malloc(sizeof (struct macro)))
            == (struct macro *)0) fatal(macerr);
        rp->m_next = macrohead;
        macrohead = rp;
        rp->m_sub = NULL;
        if ((cp = malloc(strlen(name)+1)) == (char *)0) fatal(macerr);
        strcpy(cp, name);
        rp->m_name = cp;
    }

    if ((cp = malloc(strlen(val)+1)) == (char *)0) fatal(macerr);
    strcpy(cp, val);                /*  Copy in new value  */
    rp->m_val = cp;
    rp->m_flag = macrotype;

    return rp;
}
