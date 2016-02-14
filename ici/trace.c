#define ICI_CORE
/*
 * trace.c - tracing for ICI
 *
 *
 * Andy Newman (andy@research.canon.oz.au)
 *
 *
 * This is public domain code. Use how you wish.
 *
 */

#include "func.h"
#include "object.h"
#include "trace.h"
#include "file.h"
#include "set.h"
#include "struct.h"
#include "array.h"
#include "re.h"
#include "str.h"
#include "int.h"
#include "float.h"
#include "exec.h"
#include "op.h"

int      trace_flags = TRACE_ALL; /* What we are tracing */
int      trace_yes = 0;           /* Non-zero if tracing enabled */

static int
f_trace()
{
    register char **s;
    register int flags = 0;
    register int reset = 0;
    char **wds;
    char *str;
    if (NARGS() == 0)
        return ici_int_ret((long)trace_yes);
    if (ici_typecheck("s", &str))
        return ici_argerror(0);
    wds = smash(str, ' ');
    for (s = wds; *s != 0; ++s)
    {
        if (!strcmp(*s,"lexer"))
        {
            flags |= TRACE_LEXER;
            reset = 1;
        }
        else if (!strcmp(*s,"expr"))
        {
            flags |= TRACE_EXPR;
            reset = 1;
        }
        else if (!strcmp(*s,"calls"))
        {
            flags |= TRACE_INTRINSICS | TRACE_FUNCS;
            reset = 1;
        }
        else if (!strcmp(*s,"funcs"))
        {
            flags |= TRACE_FUNCS;
            reset = 1;
        }
        else if (!strcmp(*s,"all"))
        {
            flags |= TRACE_ALL;
            reset = 1;
        }
        else if (!strcmp(*s,"mem"))
        {
            flags |= TRACE_MEM;
            reset = 1;
        }
        else if (!strcmp(*s,"src"))
        {
            flags |= TRACE_SRC;
            reset = 1;
        }
        else if (!strcmp(*s,"gc"))
        {
            flags |= TRACE_GC;
            reset = 1;
        }
        else if (!strcmp(*s,"none"))
        {
            flags = 0;
            reset = 1;
        }
        else if (!strcmp(*s,"off"))
            trace_yes = 0;
        else if (!strcmp(*s,"on"))
            trace_yes = 1;
        else
        {
            ici_error = "unrecognised trace option";
            ici_free((char *)wds);
            return 1;
        }
    }
    if (reset)
        trace_flags = flags;
    ici_free((char *)wds);
    return ici_int_ret(trace_yes);
}

ici_cfunc_t trace_cfuncs[] =
{
    {CF_OBJ,    "trace",        f_trace},
    {CF_OBJ}
};

static char *
fixup(char *s)
{
    static char buffer[128]; /* kludge */
    register char *p;

    for (p = buffer; *s && ((p - buffer) < 125); ++s)
    {
        switch (*s)
        {
        case '\b':
            *p++ = '\\';
            *p++ = 'b';
            break;
        case '\f':
            *p++ = '\\';
            *p++ = 'f';
            break;
        case '\n':
            *p++ = '\\';
            *p++ = 'n';
            break;
        case '\r':
            *p++ = '\\';
            *p++ = 'r';
            break;
        case '\t':
            *p++ = '\\';
            *p++ = 't';
            break;
        case '\033':
            *p++ = '\\';
            *p++ = 'e';
            break;
        default:
            if (*s >= ' ' && *s <= '~')
                *p++ = *s;
            else
            {
                char num[8], *pp;
                *p++ = '\\';
                if ( *s < ' ')
                    sprintf(num, "%03o", *s);
                else
                    sprintf(num, "x%0x", *s);
                for (pp = num; *pp; *p++ = *pp++)
                    ;
            }
            break;
        }
    }
    *p = 0;
    return buffer;
}

static void
pcall_arg(ici_obj_t *ap)
{
    if (isint(ap))
        fprintf(stderr, "%ld", intof(ap)->i_value);
    else if (isstring(ap))
        fprintf(stderr, "\"%s\"", fixup(stringof(ap)->s_chars));
    else if (isfloat(ap))
        fprintf(stderr, "%.9g", floatof(ap)->f_value);
    else if (isarray(ap))
        fprintf(stderr, "[array]");
    else if (isset(ap))
        fprintf(stderr, "[set]");
    else if (isstruct(ap))
        fprintf(stderr, "[struct]");
    else if (isfile(ap))
        fprintf(stderr, "<file>");
    else if (isregexp(ap))
        fprintf(stderr, "#regexp#");
    else if (isfunc(ap))
        fprintf(stderr, "[func]");
}

void
trace_pcall(ici_obj_t *o)
{
    ici_func_t   *f = NULL;
    char     *s;
    int      n;
    ici_obj_t **ap;
    /* ### FIX for generalised callable objects. */
    if (iscfunc(o))
    {
        if (!(trace_flags & TRACE_INTRINSICS))
            return;
        s = ((ici_cfunc_t *)o)->cf_name;
    }
    else if (!(trace_flags & TRACE_FUNCS))
        return;
    else
    {
        f = (ici_func_t *)o;
        s = f->f_name->s_chars;
    }
    fprintf(stderr, "trace: %s(", s);
    n = NARGS();
    ap = ARGS();
    if (ap != 0)
    {
        if (f != NULL)
        {
            ici_obj_t **fp;
            for
            (
                fp = f->f_args->a_base;
                fp < f->f_args->a_top && n > 0;
                ++fp, --ap
            )
            {
                fprintf(stderr, "%s = ", stringof(*fp)->s_chars);
                pcall_arg(*ap);
                if ( --n > 0 )
                    fprintf(stderr, ", ");
            }
        }
        while ( n > 0 )
        {
            pcall_arg(*ap--);
            if ( --n > 0 )
                fprintf(stderr, ", ");
        }
    }
    fprintf(stderr, ")\n" );
}
