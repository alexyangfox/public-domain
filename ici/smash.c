#define ICI_CORE
#include "fwd.h"

/*
 * Return an argv style array of pointers to each of the elemnts of
 * str where each element is delimited by delim.  The return'ed array has
 * been malloced and may be free'd with a single free.
 */
char **
smash(char *str, int delim)
{
    register char       *p;
    register int        i;
    register char       **ptrs;
    int                 n;

    i = 0;
    for (p = str; (p = strchr(p, delim)) != NULL; p++)
        i++;
    /*
     * XENIX compiler bug workaround:
     */
    n = strlen(str);
    n += (i + 2) * sizeof(char *) + 1;
    if ((ptrs = (char **)ici_alloc(n)) == NULL)
        return NULL;

    p = (char *)ptrs + (i + 2) * sizeof(char *);
    strcpy(p, str);
    ptrs[0] = p;
    i = 1;
    while ((p = strchr(p, delim)) != NULL)
    {
        *p++ = '\0';
        ptrs[i++] = p;
    }
    ptrs[i] = NULL;
    return ptrs;
}

/*
 * Just like smash(), but allow delim to be a set of delimiters.
 */
char **
ssmash(char *str, char *delims)
{
    register char       *p;
    register int        i;
    register char       **ptrs;
    int                 n;

    i = 0;
    for (p = str; (p = strpbrk(p, delims)) != NULL; p++)
        i++;
    /*
     * XENIX compiler bug workaround:
     */
    n = strlen(str);
    n += (i + 2) * sizeof(char *) + 1;
    if ((ptrs = (char **)ici_alloc(n)) == NULL)
        return NULL;

    p = (char *)ptrs + (i + 2) * sizeof(char *);
    strcpy(p, str);
    ptrs[0] = p;
    i = 1;
    while ((p = strpbrk(p, delims)) != NULL)
    {
        *p++ = '\0';
        ptrs[i++] = p;
    }
    ptrs[i] = NULL;
    return ptrs;
}
