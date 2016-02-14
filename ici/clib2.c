/*
 * Process environment variable support - getenv() and putenv().
 */
#define ICI_CORE
#include "str.h"
#include "func.h"
#include "cfunc.h"
#include "int.h"
#include "array.h"

#ifndef environ
    /*
     * environ is sometimes mapped to be a function, so only extern it
     * if it is not already defined.
     */
    extern char         **environ;
#endif

/*
 * Return the value of an environment variable.
 */
static int
f_getenv(void)
{
    ici_str_t           *n;
    char                **p;

    if (NARGS() != 1)
        return ici_argcount(1);
    if (!isstring(ARG(0)))
        return ici_argerror(0);
    n = stringof(ARG(0));

    for (p = environ; *p != NULL; ++p)
    {
        if
        (
#           if _WIN32
                /*
                 * Some versions of Windows (NT and 2000 at least)
                 * gratuitously change to case of some environment variables
                 * on boot.  So on Windows we do a case-insensitive
                 * compations. strnicmp is non-ANSI, but exists on Windows.
                 */
                strnicmp(*p, n->s_chars, n->s_nchars) == 0
#           else
                strncmp(*p, n->s_chars, n->s_nchars) == 0
#           endif
            &&
            (*p)[n->s_nchars] == '='
        )
        {
            return ici_str_ret(&(*p)[n->s_nchars + 1]);
        }
    }
    return ici_null_ret();
}

/*
 * Set an environment variable.
 */
static int
f_putenv(void)
{
    char        *s;
    char        *t;
    char        *e;
    char        *f;
    int         i;

    if (ici_typecheck("s", &s))
        return 1;
    if ((e = strchr(s, '=')) == NULL)
    {
        ici_error = "putenv argument not in form \"name=value\"";
        return 1;
    }
    i = strlen(s) + 1;
    /*
     * Some implementations of putenv retain a pointer to the supplied string.
     * To avoid the environment becoming corrupted when ICI collects the
     * string passed, we allocate a bit of memory to copy it into.  We then
     * forget about this memory.  It leaks.  To try to mitigate this a bit, we
     * check to see if the value is alread in the environment, and free the
     * memory if it is.
     */
    if ((t = malloc(i)) == NULL)
    {
        ici_error = "ran out of memmory";
        return 1;
    }
    strcpy(t, s);
    t[e - s] = '\0';
    f = getenv(t);
    if (f != NULL && strcmp(f, e + 1) == 0)
    {
        free(t);
    }
    else
    {
        strcpy(t, s);
        putenv(t);
    }
    return ici_null_ret();
}

ici_cfunc_t extra_cfuncs[] =
{
    {CF_OBJ,    "getenv",       f_getenv},
    {CF_OBJ,    "putenv",       f_putenv},
    {CF_OBJ}
};
