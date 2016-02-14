/*
 * cfunc.c
 *
 * Implementations of many (not all) of the core language intrinsic functions.
 * For historical reasons this is *NOT* the code associated with the ici_cfunc_t
 * type. That's in cfunco.c
 *
 * This is public domain source. Please do not add any copyrighted material.
 */
#define ICI_CORE
#include "exec.h"
#include "func.h"
#include "str.h"
#include "int.h"
#include "float.h"
#include "struct.h"
#include "set.h"
#include "op.h"
#include "ptr.h"
#include "buf.h"
#include "file.h"
#include "re.h"
#include "null.h"
#include "parse.h"
#include "mem.h"
#include "handle.h"
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>

#ifdef  _WIN32
#include <windows.h>
#endif

#ifndef NOWAITFOR
/*
 * For select() for waitfor().
 */
#include <sys/types.h>

#ifdef  BSD
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#else
#ifdef mips
#include <bsd/sys/types.h>
#include <bsd/sys/time.h>
#else
#ifdef hpux
#include <time.h>
#else
#ifndef _R3000
#include <sys/select.h>
#include <sys/times.h>
#endif
#endif
#endif
#endif
#ifndef bzero
#define bzero(p,n)      (memset((p), 0, (n)))
#endif
#endif

#if !defined(_WIN32) || (defined(sun) && (defined(__GNUC__) || !__STDC__))
extern int      fgetc();
#endif

/*
 * Marshall function arguments in a call from ICI to C.  This function may
 * only be called from the implementation of an intrinsic function.
 *
 * 'types' is a character string.  Each character corresponds to an actual
 * argument in the ICI side of the call.  Each is checked according to the
 * particular letter, and possibly converted and/or assigned through a
 * corresponing pointer to a C-side data item provided in the vargars argument
 * list to this function.
 *
 * Any detected type mismatches result in a non-zero return.  If all types
 * match, all assignments will be made and zero will be returned.
 *
 * The key letters that may be used in 'types', and their meaning, are:
 *
 * o        Any ICI object is required in the ICI actuals, the corresponding
 *          vararg must be a pointer to an '(ici_obj_t *)'; which will be set
 *          to the actual argument.
 *
 * h        An ICI handle object.  The next available vararg must be an ICI
 *          string object.  The corresponding ICI argument must be a handle
 *          with that name.  The next (again) available vararg after that is a
 *          pointer to store the '(ici_handle_t *)' through.
 *
 * p        An ICI ptr object is required in the actuals, then as for 'o'.
 *
 * d        An ICI struct object is required in the actuals, then as for 'o'.
 *
 * a        An ICI array object is required in the actuals, then as for 'o'.
 *
 * u        An ICI file object is required in the actuals, then as for 'o'.
 *
 * r        An ICI regexp object is required in the actuals, then as for 'o'.
 *
 * m        An ICI mem object is required in the actuals, then as for 'o'.
 *
 * i        An ICI int object is required in the actuals, the value of this
 *          int will be stored through the corresponding pointer which must be
 *          a '(long *)'.
 *
 * f        An ICI float object is required in the actuals, the value of this
 *          float will be stored through the corresponding pointer which must
 *          be a '(double *)'.
 *
 * n        An ICI float or int object is required in the actuals, the value
 *          of this float or int will be stored through the corresponding
 *          pointer which must be a '(double *)'.
 *
 * s        An ICI string object is required in the actuals, the corresponding
 *          pointer must be a (char **).  A pointer to the raw characters of
 *          the string will be stored through this (this will be '\0'
 *          terminated by virtue of all ICI strings having a gratuitous '\0'
 *          just past their real end).  These characters can be assumed to
 *          remain available until control is returned back to ICI because the
 *          string is still on the ICI operand stack and can't be collected.
 *          Once control has reurned to ICI, they could be collected at any
 *          time.
 *
 * -        The acutal parameter at this position is skipped, but it must be
 *          present.
 *
 * *        All remaining actual parametes are ignored (even if there aren't
 *          any).
 *
 * The capitalisation of any of the alphabetic key letters above changes
 * their meaning.  The acutal must be an ICI ptr type.  The value this
 * pointer points to is taken to be the value which the above descriptions
 * concern themselves with (i.e. in place of the raw actual parameter).
 *
 * There must be exactly as many actual arguments as key letters unless
 * the last key letter is a *.
 *
 * Error returns have the usual ICI error conventions.
 *
 * This --func-- forms part or the --ici-api--.
 */
int
ici_typecheck(char *types, ...)
{
    va_list             va;
    register ici_obj_t  **ap;   /* Argument pointer. */
    register int        nargs;
    register int        i;
    char                *ptr;   /* Subsequent things from va_alist. */
    register int        tcode;
    register ici_obj_t  *o;

    va_start(va, types);
    nargs = NARGS();
    ap = ARGS();
    for (i = 0; types[i] != '\0'; ++i, --ap)
    {
        if (types[i] == '*')
        {
            va_end(va);
            return 0;
        }

        if (i == nargs)
        {
            va_end(va);
            return ici_argcount(strlen(types));
        }

        if ((tcode = types[i]) == '-')
            continue;

        ptr = va_arg(va, char *);
        if (tcode >= 'A' && tcode <= 'Z')
        {
            if (!isptr(*ap))
                goto fail;
            if ((o = ici_fetch(*ap, objof(ici_zero))) == NULL)
                goto fail;
            tcode += 'a' - 'A';
        }
        else
        {
            o = *ap;
        }

        switch (tcode)
        {
        case 'o': /* Any object. */
            *(ici_obj_t **)ptr = o;
            break;

        case 'h': /* A handle with a particular name. */
            if (!ishandleof(o, (ici_str_t *)ptr))
                goto fail;
            ptr = va_arg(va, char *);
            *(ici_handle_t **)ptr = handleof(o);
            break;

        case 'p': /* Any pointer. */
            if (!isptr(o))
                goto fail;
            *(ici_ptr_t **)ptr = ptrof(o);
            break;

        case 'i': /* An int -> long. */
            if (!isint(o))
                goto fail;
            *(long *)ptr = intof(o)->i_value;
            break;

        case 's': /* A string -> (char *). */
            if (!isstring(o))
                goto fail;
            *(char **)ptr = stringof(o)->s_chars;
            break;

        case 'f': /* A float -> double. */
            if (!isfloat(o))
                goto fail;
            *(double *)ptr = floatof(o)->f_value;
            break;

        case 'n': /* A number, int or float -> double. */
            if (isint(o))
                *(double *)ptr = intof(o)->i_value;
            else if (isfloat(o))
                *(double *)ptr = floatof(o)->f_value;
            else
                goto fail;
            break;

        case 'd': /* A struct ("dict") -> (ici_struct_t *). */
            if (!isstruct(o))
                goto fail;
            *(ici_struct_t **)ptr = structof(o);
            break;

        case 'a': /* An array -> (ici_array_t *). */
            if (!isarray(o))
                goto fail;
            *(ici_array_t **)ptr = arrayof(o);
            break;

        case 'u': /* A file -> (ici_file_t *). */
            if (!isfile(o))
                goto fail;
            *(ici_file_t **)ptr = fileof(o);
            break;

        case 'r': /* A regular expression -> (regexpr_t *). */
            if (!isregexp(o))
                goto fail;
            *(ici_regexp_t **)ptr = regexpof(o);
            break;

        case 'm': /* A mem -> (ici_mem_t *). */
            if (!ismem(o))
                goto fail;
            *(ici_mem_t **)ptr = memof(o);
            break;

        default:
            assert(0);
        }
    }
    va_end(va);
    if (i != nargs)
        return ici_argcount(i);
    return 0;

fail:
    return ici_argerror(i);
}

/*
 * Perform storage of values through pointers in the actual arguments to
 * an ICI to C function. Typically in preparation for returning values
 * back to the calling ICI code.
 *
 * 'types' is a character string consisting of key letters which
 * correspond to actual arguments of the current ICI/C function.
 * Each of the characters in the retspec has the following meaning.
 *
 * o        The actual ICI argument must be a ptr, the corresponding
 *                      pointer is assumed to be an (ici_obj_t **).  The
 *                      location indicated by the ptr object is updated with
 *                      the (ici_obj_t *).
 *
 * d
 * a
 * u    Likwise for types as per ici_typecheck() above.
 * ...
 * -    The acutal argument is skipped.
 * *    ...
 *
 */
int
ici_retcheck(char *types, ...)
{
    va_list             va;
    register int        i;
    register int        nargs;
    register ici_obj_t  **ap;
    char                *ptr;
    register int        tcode;
    register ici_obj_t  *o;
    register ici_obj_t  *s;

    va_start(va, types);
    nargs = NARGS();
    ap = ARGS();
    for (i = 0; types[i] != '\0'; ++i, --ap)
    {
        if ((tcode = types[i]) == '*')
        {
            va_end(va);
            return 0;
        }

        if (i == nargs)
        {
            va_end(va);
            return ici_argcount(strlen(types));
        }

        if (tcode == '-')
            continue;

        o = *ap;
        if (!isptr(o))
            goto fail;

        ptr = va_arg(va, char *);

        switch (tcode)
        {
        case 'o': /* Any object. */
            *(ici_obj_t **)ptr = o;
            break;

        case 'p': /* Any pointer. */
            if (!isptr(o))
                goto fail;
            *(ici_ptr_t **)ptr = ptrof(o);
            break;

        case 'i':
            if ((s = objof(ici_int_new(*(long *)ptr))) == NULL)
                goto ret1;
            if (ici_assign(o, ici_zero, s))
                goto ret1;
            ici_decref(s);
            break;

        case 's':
            if ((s = objof(ici_str_new_nul_term(*(char **)ptr))) == NULL)
                goto ret1;
            if (ici_assign(o, ici_zero, s))
                goto ret1;
            ici_decref(s);
            break;

        case 'f':
            if ((s = objof(ici_float_new(*(double *)ptr))) == NULL)
                goto ret1;
            if (ici_assign(o, ici_zero, s))
                goto ret1;
            ici_decref(s);
            break;

        case 'd':
            if (!isstruct(o))
                goto fail;
            *(ici_struct_t **)ptr = structof(o);
            break;

        case 'a':
            if (!isarray(o))
                goto fail;
            *(ici_array_t **)ptr = arrayof(o);
            break;

        case 'u':
            if (!isfile(o))
                goto fail;
            *(ici_file_t **)ptr = fileof(o);
            break;

        case '*':
            return 0;

        }
    }
    va_end(va);
    if (i != nargs)
        return ici_argcount(i);
    return 0;

ret1:
    va_end(va);
    return 1;

fail:
    va_end(va);
    return ici_argerror(i);
}

/*
 * Generate a generic error message to indicate that argument i of the current
 * intrinsic function is bad.  Despite being generic, this message is
 * generally pretty informative and useful.  It has the form:
 *
 *   argument %d of %s incorrectly supplied as %s
 *
 * The argument number is base 0.  I.e.  ici_argerror(0) indicates the 1st
 * argument is bad.
 *
 * The function returns 1, for use in a direct return from an intrinsic
 * function.
 *
 * This function may only be called from the implementation of an intrinsic
 * function.  It takes the function name from the current operand stack, which
 * therefore should not have been distured (which is normal for intrincic
 * functions).  This function is typically used from C coded functions that
 * are not using ici_typecheck() to process arguments.  For example, a
 * function that just takes a single mem object as an argument might start:
 *
 *  static int
 *  myfunc()
 *  {
 *      ici_obj_t  *o;
 *
 *      if (NARGS() != 1)
 *          return ici_argcount(1);
 *      if (!ismem(ARG(0)))
 *          return ici_argerror(0);
 *      . . .
 *
 * This --func-- forms part of ICI's exernal API --ici-api-- 
 */
int
ici_argerror(int i)
{
    char        n1[30];
    char        n2[30];

    sprintf(buf, "argument %d of %s incorrectly supplied as %s",
        i + 1,
        ici_objname(n1, ici_os.a_top[-1]),
        ici_objname(n2, ARG(i)));
    ici_error = buf;
    return 1;
}

/*
 * Generate a generic error message to indicate that the wrong number of
 * arguments have been supplied to an intrinsic function, and that it really
 * (or most commonly) takes 'n'.  This function sets the error descriptor
 * (ici_error) to a message like:
 *
 *      %d arguments given to %s, but it takes %d
 *
 * and then returns 1.
 *
 * This function may only be called from the implementation of an intrinsic
 * function.  It takes the number of actual argument and the function name
 * from the current operand stack, which therefore should not have been
 * distured (which is normal for intrincic functions).  It takes the number of
 * arguments the function should have been supplied with (or typically is)
 * from 'n'.  This function is typically used from C coded functions that are
 * not using ici_typecheck() to process arguments.  For example, a function
 * that just takes a single object as an argument might start:
 *
 *      static int
 *      myfunc()
 *      {
 *          ici_obj_t  *o;
 *
 *          if (NARGS() != 1)
 *              return ici_argcount(1);
 *          o = ARG(0);
 *          . . .
 *
 * This function forms part of ICI's exernal API --ici-api-- --func--
 */
int
ici_argcount(int n)
{
    char        n1[30];

    sprintf(buf, "%d arguments given to %s, but it takes %d",
        NARGS(), ici_objname(n1, ici_os.a_top[-1]), n);
    ici_error = buf;
    return 1;
}

/*
 * General way out of an intrinsic function returning the object 'o', but
 * the given object has a reference count which must be decref'ed on the
 * way out. Return 0 unless the given 'o' is NULL, in which case it returns
 * 1 with no other action.
 *
 * This is suitable for using as a return from an intrinsic function
 * as say:
 *
 *      return ici_ret_with_decref(objof(ici_int_new(2)));
 *
 * (Although see ici_int_ret().) If the object you wish to return does
 * not have an extra reference, use ici_ret_no_decref().
 *
 * This function forms part of ICI's exernal API --ici-api-- --func--
 */
int
ici_ret_with_decref(ici_obj_t *o)
{
    if (o == NULL)
        return 1;
    ici_os.a_top -= NARGS() + 1;
    ici_os.a_top[-1] = o;
    ici_decref(o);
    --ici_xs.a_top;
    return 0;
}

/*
 * General way out of an intrinsic function returning the object o where
 * the given object has no extra refernce count. Returns 0 indicating no
 * error.
 *
 * This is suitable for using as a return from an intrinsic function
 * as say:
 *
 *      return ici_ret_no_decref(o);
 *
 * If the object you are returning has an extra reference which must be
 * decremented as part of the return, use ici_ret_with_decref() (above).
 *
 * This function forms part of ICI's exernal API --ici-api-- --func--
 */
int
ici_ret_no_decref(ici_obj_t *o)
{
    if (o == NULL)
        return 1;
    ici_os.a_top -= NARGS() + 1;
    ici_os.a_top[-1] = o;
    --ici_xs.a_top;
    return 0;
}

/*
 * Use 'return ici_int_ret(ret);' to return an integer (i.e. a C long) from
 * an intrinsic fuction.
 *
 * This function forms part of ICI's exernal API --ici-api-- --func--
 */
int
ici_int_ret(long ret)
{
    return ici_ret_with_decref(objof(ici_int_new(ret)));
}

/*
 * Use 'return ici_float_ret(ret);' to return a float (i.e. a C double)
 * from an intrinsic fuction. The double will be converted to an ICI
 * float.
 * 
 *
 * This function forms part of ICI's exernal API --ici-api-- --func--
 */
int
ici_float_ret(double ret)
{
    return ici_ret_with_decref(objof(ici_float_new(ret)));
}

/*
 * Use 'return ici_str_ret(str);' to return a nul terminated string from
 * an intrinsic fuction. The string will be converted into an ICI string.
 *
 * This function forms part of ICI's exernal API --ici-api-- --func--
 */
int
ici_str_ret(char *str)
{
    return ici_ret_with_decref(objof(ici_str_new_nul_term(str)));
}

/*
 * Return the array object that is the current value of "path" in the
 * current scope. The array is not increfed - it is assumed to be still
 * referenced from the scope until the caller has finished with it.
 */
ici_array_t *
ici_need_path(void)
{
    ici_obj_t           *o;

    o = ici_fetch(ici_vs.a_top[-1], SSO(path));
    if (!isarray(o))
    {
        ici_error = "path is not an array";
        return NULL;
    }
    return arrayof(o);
}

/*
 * Return the ICI file object that is the current value of the 'stdin'
 * name in the current scope. Else NULL, usual conventions. The file
 * has not increfed (it is referenced from the current scope, until
 * that assumption is broken, it is known to be uncollectable).
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_file_t *
ici_need_stdin(void)
{
    ici_file_t          *f;

    f = fileof(ici_fetch(ici_vs.a_top[-1], SSO(_stdin)));
    if (!isfile(f))
    {
        ici_error = "stdin is not a file";
        return NULL;
    }
    return f;
}

/*
 * Return the file object that is the current value of the 'stdout'
 * name in the current scope. Else NULL, usual conventions.  The file
 * has not increfed (it is referenced from the current scope, until
 * that assumption is broken, it is known to be uncollectable).
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_file_t *
ici_need_stdout(void)
{
    ici_file_t          *f;

    f = fileof(ici_fetch(ici_vs.a_top[-1], SSO(_stdout)));
    if (!isfile(f))
    {
        ici_error = "stdout is not a file";
        return NULL;
    }
    return f;
}

#ifndef NOMATH

#if !__STDC__
# ifdef sun
/*
 * Math exception handler for SVID compliant systems. We just set errno
 * to the appropriate value and return non-zero. This stops the output
 * of a message to stderr and allows normal error handling to control
 * the behaviour.
 */
int
matherr(struct exception *exc)
{
    switch (exc->type)
    {
    case DOMAIN:
    case SING:
        errno = EDOM;
        break;
    case OVERFLOW:
    case UNDERFLOW:
        errno = ERANGE;
        break;
    }
    return 1;
}
# endif
#endif

/*
 * For any C functions that return a double and take 0, 1, or 2 doubles as
 * arguments.
 */
int
f_math()
{
    double      av[2];
    double      r;
    char        n1[30];
    char        n2[80];

    if (ici_typecheck((char *)CF_ARG2() + 2, &av[0], &av[1]))
        return 1;
    errno = 0;
    r = (*(double (*)())CF_ARG1())(av[0], av[1]);
    if (errno != 0)
    {
        sprintf(n2, "%g", av[0]);
        if (NARGS() == 2)
            sprintf(n2 + strlen(n2), ", %g", av[1]);
         return ici_get_last_errno(ici_objname(n1, ici_os.a_top[-1]), n2);
    }
    return ici_float_ret(r);
}
#endif

/*
 * Stand-in function for core functions that are coded in ICI.  The real
 * function is loaded on first call, and replaces this one, then we transfer
 * to it.  On subsequent calls, this code is out of the picture.
 *
 * cf_arg1              The name (an ICI string) of the function to be loaded
 *                      and transfered to.
 *
 * cf_arg2              The name (an ICI string) of the core ICI extension
 *                      module that defines the function.  (Eg "core1",
 *                      meaning the function is in "ici4core1.ici".  Only
 *                      "ici4core.ici" is always parsed.  Others are
 *                      on-demand.)
 */
static int
f_coreici(ici_obj_t *s)
{
    ici_obj_t           *c;
    ici_obj_t           *f;

    /*
     * Use the execution engine to evaluate the name of the core module
     * this function is in. It will auto-load if necessary.
     */
    if ((c = ici_evaluate((ici_obj_t *)CF_ARG2(), 0)) == NULL)
        return 1;
    /*
     * Fetch the real function from that module and verify it is callable.
     */
    f = fetch_base(c, (ici_obj_t *)CF_ARG1());
    ici_decref(c);
    if (f == NULL)
        return 1;
    if (ici_typeof(f)->t_call == NULL)
    {
        char    n1[30];

        sprintf(buf, "attempt to call %s", ici_objname(n1, f));
        ici_error = buf;
        return 1;
    }
    /*
     * Over-write the definition of the function (which was us) with the
     * real function.
     */
    if (ici_assign(ici_vs.a_top[-1], (ici_obj_t *)CF_ARG1(), f))
        return 1;
    /*
     * Replace us with the new callable object on the operand stack
     * and transfer to it.
     */
    ici_os.a_top[-1] = f;
    return (*ici_typeof(f)->t_call)(f, s);
}

static int
f_struct()
{
    ici_obj_t           **o;
    int                 nargs;
    ici_struct_t        *s;
    ici_objwsup_t       *super;

    nargs = NARGS();
    o = ARGS();
    super = NULL;
    if (nargs & 1)
    {
        super = objwsupof(*o);
        if (!hassuper(super) && !isnull(objof(super)))
            return ici_argerror(0);
        if (isnull(objof(super)))
            super = NULL;
        --nargs;
        --o;
    }
    if ((s = ici_struct_new()) == NULL)
        return 1;
    for (; nargs >= 2; nargs -= 2, o -= 2)
    {
        if (ici_assign(s, o[0], o[-1]))
        {
            ici_decref(s);
            return 1;
        }
    }
    s->o_head.o_super = super;
    return ici_ret_with_decref(objof(s));
}

static int
f_set()
{
    register int        nargs;
    register ici_set_t  *s;
    register ici_obj_t  **o;

    if ((s = ici_set_new()) == NULL)
        return 1;
    for (nargs = NARGS(), o = ARGS(); nargs > 0; --nargs, --o)
    {
        if (ici_assign(s, *o, ici_one))
        {
            ici_decref(s);
            return 1;
        }
    }
    return ici_ret_with_decref(objof(s));
}

static int
f_array()
{
    register int        nargs;
    register ici_array_t    *a;
    register ici_obj_t  **o;

    nargs = NARGS();
    if ((a = ici_array_new(nargs)) == NULL)
        return 1;
    for (o = ARGS(); nargs > 0; --nargs)
        *a->a_top++ = *o--;
    return ici_ret_with_decref(objof(a));
}

static int
f_keys()
{
    ici_struct_t        *s;
    register ici_array_t    *k;
    register ici_sslot_t *sl;

    if (ici_typecheck("d", &s))
        return 1;
    if ((k = ici_array_new(s->s_nels)) == NULL)
        return 1;
    for (sl = s->s_slots; sl < s->s_slots + s->s_nslots; ++sl)
    {
        if (sl->sl_key != NULL)
            *k->a_top++ = sl->sl_key;
    }
    return ici_ret_with_decref(objof(k));
}

static int
f_copy(ici_obj_t *o)
{
    if (o != NULL)
        return ici_ret_with_decref(copy(o));
    if (NARGS() != 1)
        return ici_argcount(1);
    return ici_ret_with_decref(copy(ARG(0)));
}

static int
f_typeof()
{
    if (NARGS() != 1)
        return ici_argcount(1);
    if (ishandle(ARG(0)))
        return ici_ret_no_decref(objof(handleof(ARG(0))->h_name));
    if (ici_typeof(ARG(0))->t_ici_name == NULL)
        ici_typeof(ARG(0))->t_ici_name = ici_str_new_nul_term(ici_typeof(ARG(0))->t_name);
    return ici_ret_no_decref(objof(ici_typeof(ARG(0))->t_ici_name));
}

static int
f_nels()
{
    register ici_obj_t  *o;
    long                size;

    if (NARGS() != 1)
        return ici_argcount(1);
    o = ARG(0);
    if (isstring(o))
        size = stringof(o)->s_nchars;
    else if (isarray(o))
        size = ici_array_nels(arrayof(o));
    else if (isstruct(o))
        size = structof(o)->s_nels;
    else if (isset(o))
        size = setof(o)->s_nels;
    else if (ismem(o))
        size = memof(o)->m_length;
    else
        size = 1;
    return ici_int_ret(size);
}

static int
f_int()
{
    register ici_obj_t  *o;
    register long       v;

    if (NARGS() < 1)
        return ici_argcount(1);
    o = ARG(0);
    if (isint(o))
        return ici_ret_no_decref(o);
    else if (isstring(o))
    {
        int             base = 0;

        if (NARGS() > 1)
        {
            if (!isint(ARG(1)))
                return ici_argerror(1);
            base = intof(ARG(1))->i_value;
            if (base != 0 && (base < 2 || base > 36))
                return ici_argerror(1);
        }
        v = ici_strtol(stringof(o)->s_chars, NULL, base);
    }
    else if (isfloat(o))
        v = (long)floatof(o)->f_value;
    else
        v = 0;
    return ici_int_ret(v);
}

static int
f_float()
{
    register ici_obj_t  *o;
    register double     v;
    extern double       strtod();

    if (NARGS() != 1)
        return ici_argcount(1);
    o = ARG(0);
    if (isfloat(o))
        return ici_ret_no_decref(o);
    else if (isstring(o))
        v = strtod(stringof(o)->s_chars, NULL);
    else if (isint(o))
        v = (double)intof(o)->i_value;
    else
        v = 0;
    return ici_float_ret(v);
}

static int
f_num()
{
    register ici_obj_t  *o;
    register double     f;
    register long       i;
    char                *s;
    extern double       strtod();
    char                n[30];

    if (NARGS() != 1)
        return ici_argcount(1);
    o = ARG(0);
    if (isfloat(o) || isint(o))
        return ici_ret_no_decref(o);
    else if (isstring(o))
    {
        int             base = 0;

        if (NARGS() > 1)
        {
            if (!isint(ARG(1)))
                return ici_argerror(1);
            base = intof(ARG(1))->i_value;
            if (base != 0 && (base < 2 || base > 36))
                return ici_argerror(1);
        }
        i = ici_strtol(stringof(o)->s_chars, &s, base);
        if (*s == '\0')
            return ici_int_ret(i);
        f = strtod(stringof(o)->s_chars, &s);
        if (*s == '\0')
            return ici_float_ret(f);
    }
    sprintf(buf, "%s is not a number", ici_objname(n, o));
    ici_error = buf;
    return 1;
}

static int
f_string()
{
    register ici_obj_t  *o;

    if (NARGS() != 1)
        return ici_argcount(1);
    o = ARG(0);
    if (isstring(o))
        return ici_ret_no_decref(o);
    if (isint(o))
        sprintf(buf, "%ld", intof(o)->i_value);
    else if (isfloat(o))
        sprintf(buf, "%g", floatof(o)->f_value);
    else if (isregexp(o))
        return ici_ret_no_decref(objof(regexpof(o)->r_pat));
    else
        sprintf(buf, "<%s>", ici_typeof(o)->t_name);
    return ici_str_ret(buf);
}

static int
f_eq()
{
    ici_obj_t   *o1;
    ici_obj_t   *o2;

    if (ici_typecheck("oo", &o1, &o2))
        return 1;
    if (o1 == o2)
        return ici_ret_no_decref(objof(ici_one));
    return ici_ret_no_decref(objof(ici_zero));
}

static int
f_push()
{
    ici_array_t *a;
    ici_obj_t   *o;

    if (ici_typecheck("ao", &a, &o))
        return 1;
    if (ici_array_push(a, o))
        return 1;
    return ici_ret_no_decref(o);
}

static int
f_rpush()
{
    ici_array_t *a;
    ici_obj_t   *o;

    if (ici_typecheck("ao", &a, &o))
        return 1;
    if (ici_array_rpush(a, o))
        return 1;
    return ici_ret_no_decref(o);
}

static int
f_pop()
{
    ici_array_t *a;
    ici_obj_t   *o;

    if (ici_typecheck("a", &a))
        return 1;
    if ((o = ici_array_pop(a)) == NULL)
        return 1;
    return ici_ret_no_decref(o);
}

static int
f_rpop()
{
    ici_array_t *a;
    ici_obj_t   *o;

    if (ici_typecheck("a", &a))
        return 1;
    if ((o = ici_array_rpop(a)) == NULL)
        return 1;
    return ici_ret_no_decref(o);
}

static int
f_top()
{
    ici_array_t *a;
    long        n = 0;

    switch (NARGS())
    {
    case 1:
        if (ici_typecheck("a", &a))
            return 1;
        break;

    default:
        if (ici_typecheck("ai", &a, &n))
            return 1;
    }
    n += ici_array_nels(a) - 1;
    return ici_ret_no_decref(ici_array_get(a, n));
}

static int
f_parse()
{
    ici_obj_t   *o;
    ici_file_t  *f;
    ici_struct_t    *s;     /* Statics. */
    ici_struct_t    *a;     /* Autos. */

    switch (NARGS())
    {
    case 1:
        if (ici_typecheck("o", &o))
            return 1;
        if ((a = ici_struct_new()) == NULL)
            return 1;
        if ((a->o_head.o_super = objwsupof(s = ici_struct_new())) == NULL)
        {
            ici_decref(a);
            return 1;
        }
        ici_decref(s);
        s->o_head.o_super = objwsupof(ici_vs.a_top[-1])->o_super;
        break;

    default:
        if (ici_typecheck("od", &o, &a))
            return 1;
        ici_incref(a);
        break;
    }

    if (isstring(o))
    {
        if ((f = ici_sopen(stringof(o)->s_chars, stringof(o)->s_nchars, o)) == NULL)
        {
            ici_decref(a);
            return 1;
        }
        f->f_name = SS(empty_string);
    }
    else if (isfile(o))
        f = fileof(o);
    else
    {
        ici_decref(a);
        return ici_argerror(0);
    }

    if (ici_parse(f, objwsupof(a)) < 0)
        goto fail;

    if (isstring(o))
        ici_decref(f);
    return ici_ret_with_decref(objof(a));

fail:
    if (isstring(o))
        ici_decref(f);
    ici_decref(a);
    return 1;
}

static int
f_include()
{
    ici_str_t   *filename;
    ici_struct_t    *a;
    int         rc;
    ici_file_t  *f;

    switch (NARGS())
    {
    case 1:
        if (ici_typecheck("o", &filename))
            return 1;
        a = structof(ici_vs.a_top[-1]);
        break;

    case 2:
        if (ici_typecheck("od", &filename, &a))
            return 1;
        break;

    default:
        return ici_argcount(2);
    }
    if (!isstring(objof(filename)))
        return ici_argerror(0);
#ifndef NODEBUGGING
    ici_debug_ignore_errors();
#endif
    if (ici_call(SS(fopen), "o=o", &f, filename))
    {
        char    fname[1024];

        strncpy(fname, filename->s_chars, 1023);
        if (!ici_find_on_path(fname, NULL))
        {
#ifndef NODEBUGGING
            ici_debug_respect_errors();
#endif
            if (ici_chkbuf(filename->s_nchars + 80))
                return 1;
            sprintf(ici_buf, "could not find \"%s\" on path", fname);
            ici_error = ici_buf;
            return 1;
        }
        if (ici_call(SS(fopen), "o=s", &f, fname))
        {
#ifndef NODEBUGGING
            ici_debug_respect_errors();
#endif
            return 1;
        }
    }
#ifndef NODEBUGGING
    ici_debug_respect_errors();
#endif
    rc = ici_parse(f, objwsupof(a));
    ici_call(SS(close), "o", f);
    ici_decref(f);
    return rc < 0 ? 1 : ici_ret_no_decref(objof(a));
}

static int
f_call(void)
{
    ici_array_t *aa;        /* The array with extra arguments, or NULL. */
    int         nargs;      /* Number of args to target function. */
    int         naargs;     /* Number of args comming from the array. */
    ici_obj_t   **base;
    ici_obj_t   **e;
    int         i;
    ici_int_t   *nargso;
    ici_obj_t   *func;

    if (NARGS() < 2)
        return ici_argcount(2);
    nargso = NULL;
    base = &ARG(NARGS() - 1);
    if (isarray(*base))
        aa = arrayof(*base);
    else if (isnull(*base))
        aa = NULL;
    else
        return ici_argerror(NARGS() - 1);
    if (aa == NULL)
        naargs = 0;
    else
        naargs = ici_array_nels(aa);
    nargs = naargs + NARGS() - 2;
    func = ARG(0);
    ici_incref(func);
    /*
     * On the operand stack, we have...
     *    [aa] [argn]...[arg2] [arg1] [func] [nargs] [us] [    ]
     *      ^                                               ^
     *      +-base                                          +-ici_os.a_top
     *
     * We want...
     *    [aa[n]]...[aa[1]] [aa[0]] [argn]...[arg2] [arg1] [nargs] [func] [    ]
     *      ^                                                               ^
     *      +-base                                                          +-ici_os.a_top
     *
     * Do everything that can get an error first, before we start playing with
     * the stack.
     *
     * We include an extra 80 in our ici_stk_push_chk, see start of
     * ici_evaluate().
     */
    if (ici_stk_push_chk(&ici_os, naargs + 80))
        goto fail;
    if ((nargso = ici_int_new(nargs)) == NULL)
        goto fail;
    /*
     * First move the arguments that we want to keep up to the stack
     * to their new position (all except the func and the array).
     */
    memmove(base + naargs, base + 1, (NARGS() - 2) * sizeof(ici_obj_t *));
    ici_os.a_top += naargs - 2;
    if (naargs > 0)
    {
        i = naargs;
        for (e = ici_astart(aa); e != ici_alimit(aa); e = ici_anext(aa, e))
            base[--i] = *e;
    }
    /*
     * Push the count of actual args and the target function.
     */
    ici_os.a_top[-2] = objof(nargso);
    ici_decref(nargso);
    ici_os.a_top[-1] = func;
    ici_decref(func);
    ici_xs.a_top[-1] = objof(&o_call);
    /*
     * Very special return. Drops back into the execution loop with
     * the call on the execution stack.
     */
    return 0;

fail:
    ici_decref(func);
    if (nargso != NULL)
        ici_decref(nargso);
    return 1;
}

static int
f_fail()
{
    char        *s;

    if (ici_typecheck("s", &s))
        return 1;
    if (ici_chkbuf(strlen(s)))
        return 1;
    strcpy(buf, s);
    ici_error = buf;
    return 1;
}

static int
f_exit()
{
    ici_obj_t   *rc;
    long        status;

    switch (NARGS())
    {
    case 0:
        rc = objof(&o_null);
        break;

    case 1:
        if (ici_typecheck("o", &rc))
            return 1;
        break;

    default:
        return ici_argcount(1);
    }
    if (isint(rc))
        status = (int)intof(rc)->i_value;
    else if (rc == objof(&o_null))
        status = 0;
    else if (isstring(rc))
    {
        if (stringof(rc)->s_nchars == 0)
            status = 0;
        else
        {
            if (strchr(stringof(rc)->s_chars, ':') != NULL)
                fprintf(stderr, "%s\n", stringof(rc)->s_chars);
            else
                fprintf(stderr, "exit: %s\n", stringof(rc)->s_chars);
            status = 1;
        }
    }
    else
    {
        return ici_argerror(0);
    }
    ici_uninit();
    exit((int)status);
    /*NOTREACHED*/
}

static int
f_vstack()
{
    int                 depth;

    if (NARGS() == 0)
        return ici_ret_with_decref(copy(objof(&ici_vs)));

    if (!isint(ARG(0)))
        return ici_argerror(0);
    depth = intof(ARG(0))->i_value;
    if (depth < 0)
        return ici_argerror(0);
    if (depth >= ici_vs.a_top - ici_vs.a_bot)
        return ici_null_ret();
    return ici_ret_no_decref(ici_vs.a_top[-depth - 1]);
}

static int
f_tochar()
{
    long        i;

    if (ici_typecheck("i", &i))
        return 1;
    buf[0] = (unsigned char)i;
    return ici_ret_with_decref(objof(ici_str_new(buf, 1)));
}

static int
f_toint()
{
    char        *s;

    if (ici_typecheck("s", &s))
        return 1;
    return ici_int_ret((long)(s[0] & 0xFF));
}

static int
f_rand()
{
    static long seed    = 1;

    if (NARGS() >= 1)
    {
        if (ici_typecheck("i", &seed))
            return 1;
        srand(seed);
    }
#ifdef ICI_RAND_IS_C_RAND
    return ici_int_ret(rand());
#else
    seed = seed * 1103515245 + 12345;
    return ici_int_ret((seed >> 16) & 0x7FFF);
#endif
}

static int
f_interval()
{
    ici_obj_t           *o;
    long                start;
    long                length;
    long                nel;
    ici_str_t           *s = 0; /* init to shut up compiler */
    ici_array_t         *a = 0; /* init to shut up compiler */
    ici_array_t         *a1;


    if (ici_typecheck("oi*", &o, &start))
        return 1;
    switch (o->o_tcode)
    {
    case TC_STRING:
        s = stringof(o);
        nel = s->s_nchars;
        break;

    case TC_ARRAY:
        a = arrayof(o);
        nel = ici_array_nels(a);
        break;

    default:
        return ici_argerror(0);
    }

    length = nel;
    if (NARGS() > 2)
    {
        if (!isint(ARG(2)))
            return ici_argerror(2);
        if ((length = intof(ARG(2))->i_value) < 0)
            ici_argerror(2);
    }

    if (length < 0)
    {
        if ((length += nel) < 0)
            length = 0;
    }
    if (start < 0)
    {
        if ((start += nel) < 0)
        {
            if ((length += start) < 0)
                length = 0;
            start = 0;
        }
    }
    else if (start > nel)
        start = nel;
    if (start + length > nel)
        length = nel - start;

    if (o->o_tcode == TC_STRING)
    {
        return ici_ret_with_decref(objof(ici_str_new(s->s_chars + start, (int)length)));
    }
    else
    {
        if ((a1 = ici_array_new(length)) == NULL)
            return 1;
        ici_array_gather(a1->a_base, a, start, length);
        a1->a_top += length;
        return ici_ret_with_decref(objof(a1));
    }
}

static int
f_explode()
{
    register int        i;
    char                *s;
    ici_array_t         *x;

    if (ici_typecheck("s", &s))
        return 1;
    i = stringof(ARG(0))->s_nchars;
    if ((x = ici_array_new(i)) == NULL)
        return 1;
    while (--i >= 0)
    {
        if ((*x->a_top = objof(ici_int_new(*s++ & 0xFFL))) == NULL)
        {
            ici_decref(x);
            return 1;
        }
        ici_decref(*x->a_top);
        ++x->a_top;
    }
    return ici_ret_with_decref(objof(x));
}

static int
f_implode()
{
    ici_array_t         *a;
    int                 i;
    ici_obj_t           **o;
    ici_str_t           *s;
    char                *p;

    if (ici_typecheck("a", &a))
        return 1;
    i = 0;
    for (o = ici_astart(a); o != ici_alimit(a); o = ici_anext(a, o))
    {
        switch ((*o)->o_tcode)
        {
        case TC_INT:
            ++i;
            break;

        case TC_STRING:
            i += stringof(*o)->s_nchars;
            break;
        }
    }
    if ((s = ici_str_alloc(i)) == NULL)
        return 1;
    p = s->s_chars;
    for (o = ici_astart(a); o != ici_alimit(a); o = ici_anext(a, o))
    {
        switch ((*o)->o_tcode)
        {
        case TC_INT:
            *p++ = (char)intof(*o)->i_value;
            break;

        case TC_STRING:
            memcpy(p, stringof(*o)->s_chars, stringof(*o)->s_nchars);
            p += stringof(*o)->s_nchars;
            break;
        }
    }
    if ((s = stringof(ici_atom(objof(s), 1))) == NULL)
        return 1;
    return ici_ret_with_decref(objof(s));
}

static int
f_sopen()
{
    ici_file_t  *f;
    char        *str;
    char        *mode;

    mode = "r";
    if (ici_typecheck(NARGS() > 1 ? "ss" : "s", &str, &mode))
        return 1;
    if (strcmp(mode, "r") != 0 && strcmp(mode, "rb") != 0)
    {
        ici_chkbuf(strlen(mode) + 50);
        sprintf(buf, "attempt to use mode \"%s\" in sopen()", mode);
        ici_error = buf;
        return 1;
    }
    if ((f = ici_sopen(str, stringof(ARG(0))->s_nchars, ARG(0))) == NULL)
        return 1;
    f->f_name = SS(empty_string);
    return ici_ret_with_decref(objof(f));
}

static int
f_mopen()
{
    ici_mem_t   *mem;
    ici_file_t  *f;
    char        *mode;

    if (ici_typecheck("os", &mem, &mode))
    {
        if (ici_typecheck("o", &mem))
            return 1;
        mode = "r";
    }
    if (!ismem(objof(mem)))
        return ici_argerror(0);
    if (strcmp(mode, "r") && strcmp(mode, "rb"))
    {
        ici_error = "bad open mode for mopen";
        return 1;
    }
    if (mem->m_accessz != 1)
    {
        ici_error = "memory object must have access size of 1 to be opened";
        return 1;
    }
    if ((f = ici_sopen(mem->m_base, (int)mem->m_length, objof(mem))) == NULL)
        return 1;
    f->f_name = SS(empty_string);
    return ici_ret_with_decref(objof(f));
}

int
f_sprintf()
{
    char                *fmt;
    register char       *p;
    register int        i;              /* Where we are up to in buf. */
    register int        j;
    int                 which;
    int                 nargs;
    char                subfmt[40];     /* %...? portion of string. */
    int                 stars[2];       /* Precision and field widths. */
    int                 nstars;
    int                 gotl;           /* Have a long int flag. */
    int                 gotdot;         /* Have a . in % format. */
    long                ivalue;
    double              fvalue;
    char                *svalue;
    ici_obj_t           **o;            /* Argument pointer. */
    ici_file_t          *file;
    char                oname[ICI_OBJNAMEZ];
#ifdef  BAD_PRINTF_RETVAL
#define IPLUSEQ
#else
#define IPLUSEQ         i +=
#endif

    which = (int)CF_ARG1(); /* sprintf, printf, fprintf */
    if (which != 0 && NARGS() > 0 && isfile(ARG(0)))
    {
        which = 2;
        if (ici_typecheck("us*", &file, &fmt))
            return 1;
        o = ARGS() - 2;
        nargs = NARGS() - 2;
    }
    else
    {
        if (ici_typecheck("s*", &fmt))
            return 1;
        o = ARGS() - 1;
        nargs = NARGS() - 1;
    }

    p = fmt;
    i = 0;
    while (*p != '\0')
    {
        if (*p != '%')
        {
            if (ici_chkbuf(i))
                return 1;
            buf[i++] = *p++;
            continue;
        }

        nstars = 0;
        stars[0] = 0;
        stars[1] = 0;
        gotl = 0;
        gotdot = 0;
        subfmt[0] = *p++;
        j = 1;
        while (*p != '\0' && strchr("adiouxXfeEgGcs%", *p) == NULL)
        {
            if (*p == '*')
                ++nstars;
            else if (*p == 'l')
                gotl = 1;
            else if (*p == '.')
                gotdot = 1;
            else if (*p >= '0' && *p <= '9')
            {
                do
                {
                    stars[gotdot] = stars[gotdot] * 10 + *p - '0';
                    subfmt[j++] = *p++;

                } while (*p >= '0' && *p <= '9');
                continue;
            }
            subfmt[j++] = *p++;
        }
        if (gotl == 0 && strchr("diouxX", *p) != NULL)
            subfmt[j++] = 'l';
        subfmt[j++] = *p;
        subfmt[j++] = '\0';
        if (nstars > 2)
            nstars = 2;
        for (j = 0; j < nstars; ++j)
        {
            if (nargs <= 0)
                goto lacking;
            if (!isint(*o))
                goto type;
            stars[j] = (int)intof(*o)->i_value;
            --o;
            --nargs;
        }
        switch (*p++)
        {
        case 'd':
        case 'i':
        case 'o':
        case 'u':
        case 'x':
        case 'X':
            if (nargs <= 0)
                goto lacking;
            if (isint(*o))
                ivalue = intof(*o)->i_value;
            else if (isfloat(*o))
                ivalue = (long)floatof(*o)->f_value;
            else
                goto type;
            if (ici_chkbuf(i + 30 + stars[0] + stars[1])) /* Pessimistic. */
                return 1;
            switch (nstars)
            {
            case 0:
                IPLUSEQ sprintf(&buf[i], subfmt, ivalue);
                break;

            case 1:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], ivalue);
                break;

            case 2:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], stars[1], ivalue);
                break;
            }
            --o;
            --nargs;
            break;

        case 'c':
            if (nargs <= 0)
                goto lacking;
            if (isint(*o))
                ivalue = intof(*o)->i_value;
            else if (isfloat(*o))
                ivalue = (long)floatof(*o)->f_value;
            else
                goto type;
            if (ici_chkbuf(i + 30 + stars[0] + stars[1])) /* Pessimistic. */
                return 1;
            switch (nstars)
            {
            case 0:
                IPLUSEQ sprintf(&buf[i], subfmt, (int)ivalue);
                break;

            case 1:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], (int)ivalue);
                break;

            case 2:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], stars[1], (int)ivalue);
                break;
            }
            --o;
            --nargs;
            break;

        case 's':
            if (nargs <= 0)
                goto lacking;
            if (!isstring(*o))
                goto type;
            svalue = stringof(*o)->s_chars;
            if (ici_chkbuf(i + stringof(*o)->s_nchars + stars[0] + stars[1]))
                return 1;
            switch (nstars)
            {
            case 0:
                IPLUSEQ sprintf(&buf[i], subfmt, svalue);
                break;

            case 1:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], svalue);
                break;

            case 2:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], stars[1], svalue);
                break;
            }
            --o;
            --nargs;
            break;

        case 'a':
            if (nargs <= 0)
                goto lacking;
            ici_objname(oname, *o);
            svalue = oname;
            if (ici_chkbuf(i + ICI_OBJNAMEZ + stars[0] + stars[1]))
                return 1;
            subfmt[strlen(subfmt) - 1] = 's';
            switch (nstars)
            {
            case 0:
                IPLUSEQ sprintf(&buf[i], subfmt, svalue);
                break;

            case 1:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], svalue);
                break;

            case 2:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], stars[1], svalue);
                break;
            }
            --o;
            --nargs;
            break;

        case 'f':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
            if (nargs <= 0)
                goto lacking;
            if (isint(*o))
                fvalue = intof(*o)->i_value;
            else if (isfloat(*o))
                fvalue = floatof(*o)->f_value;
            else
                goto type;
            if (ici_chkbuf(i + 40 + stars[0] + stars[1])) /* Pessimistic. */
                return 1;
            switch (nstars)
            {
            case 0:
                IPLUSEQ sprintf(&buf[i], subfmt, fvalue);
                break;

            case 1:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], fvalue);
                break;

            case 2:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], stars[1], fvalue);
                break;
            }
            --o;
            --nargs;
            break;

        case '%':
            if (ici_chkbuf(i))
                return 1;
            buf[i++] = '%';
            continue;
        }
#ifdef  BAD_PRINTF_RETVAL
        i = strlen(buf); /* old BSD sprintf doesn't return usual value. */
#endif
    }
    buf[i] = '\0';
    switch (which)
    {
    case 1: /* printf */
        if ((file = ici_need_stdout()) == NULL)
            return 1;
    case 2: /* fprintf */
        if (objof(file)->o_flags & F_CLOSED)
        {
            ici_error = "write to closed file";
            return 1;
        }
        {
            char        small_buf[128];
            char        *out_buf;
            ici_exec_t  *x;

            if (i <= sizeof small_buf)
            {
                out_buf = small_buf;
            }
            else
            {
                if ((out_buf = ici_nalloc(i)) == NULL)
                    return 1;
            }
            memcpy(out_buf, buf, i);
            x = ici_leave();
            (*file->f_type->ft_write)(out_buf, i, file->f_file);
            ici_enter(x);
            if (out_buf != small_buf)
                ici_nfree(out_buf, i);
        }
        return ici_int_ret((long)i);

    default: /* sprintf */
        return ici_ret_with_decref(objof(ici_str_new(buf, i)));
    }

type:
    sprintf(buf, "attempt to use a %s with a \"%s\" format in sprintf",
        ici_typeof(*o)->t_name, subfmt);
    ici_error = buf;
    return 1;

lacking:
    ici_error = "not enoughs args to sprintf";
    return 1;
}

static int
f_currentfile()
{
    ici_obj_t   **o;
    int         raw;
    ici_file_t  *f;

    raw = NARGS() > 0 && ARG(0) == SSO(raw);
    for (o = ici_xs.a_top - 1; o >= ici_xs.a_base; --o)
    {
        if (isparse(*o))
        {
            if (raw)
                return ici_ret_no_decref(objof(parseof(*o)->p_file));
            f = ici_file_new(*o, &ici_parse_ftype, parseof(*o)->p_file->f_name, *o);
            if (f == NULL)
                return 1;
            return ici_ret_with_decref(objof(f));
        }
    }
    return ici_null_ret();
}

static int
f_del()
{
    ici_obj_t   *s;
    ici_obj_t   *o;

    if (ici_typecheck("oo", &s, &o))
        return 1;
    if (isstruct(s))
    {
        ici_struct_unassign(structof(s), o);
    }
    else if (isset(s))
    {
        ici_set_unassign(setof(s), o);
    }
    else if (isarray(s))
    {
        ici_array_t     *a;
        ici_obj_t       **e;
        long            i;
        ptrdiff_t       n;

        
        if (!isint(o))
            return ici_null_ret();
        a = arrayof(s);
        i = intof(o)->i_value;
        n = ici_array_nels(a);
        if (i < 0 || i >= n)
            return ici_null_ret();
        if (s->o_flags & O_ATOM)
        {
            ici_error = "attempt to modify to an atomic array";
            return 1;
        }
        if (i >= n / 2)
        {
            ici_obj_t       **prev_e;

            e = ici_array_find_slot(a, i);
            prev_e = e;
            for (e = ici_anext(a, e); e != ici_alimit(a); e = ici_anext(a, e))
            {
                *prev_e = *e;
                prev_e = e;
            }
            ici_array_pop(a);
        }
        else
        {
            ici_obj_t       *prev_o;

            prev_o = *(e = ici_astart(a));
            for (e = ici_anext(a, e); --i >= 0; e = ici_anext(a, e))
            {
                o = *e;
                *e = prev_o;
                prev_o = o;
            }
            ici_array_rpop(a);
        }
    }
    else
    {
        return ici_argerror(0);
    }
    return ici_null_ret();
}

/*
 * super_loop()
 *
 * Return 1 and set error if the super chain of the given struct has
 * a loop. Else return 0.
 */
static int
super_loop(ici_objwsup_t *base)
{
    ici_objwsup_t       *s;

    /*
     * Scan up the super chain setting the O_MARK flag as we go. If we hit
     * a marked struct, we must have looped. Note that the O_MARK flag
     * is a strictly transitory flag that can only be used in local
     * non-allocating areas such as this. It must be left cleared at all
     * times. The garbage collector assumes it is cleared on all objects
     * when it runs.
     */
    for (s = base; s != NULL; s = s->o_super)
    {
        if (objof(s)->o_flags & O_MARK)
        {
            /*
             * A loop. Clear all the O_MARK flags we set and set error.
             */
            for (s = base; objof(s)->o_flags & O_MARK; s = s->o_super)
                objof(s)->o_flags &= ~O_MARK;
            ici_error = "cycle in struct super chain";
            return 1;
        }
        objof(s)->o_flags |= O_MARK;
    }
    /*
     * No loop. Clear all the O_MARK flags we set.
     */
    for (s = base; s != NULL; s = s->o_super)
        objof(s)->o_flags &= ~O_MARK;
    return 0;
}

static int
f_super()
{
    ici_objwsup_t       *o;
    ici_objwsup_t       *newsuper;
    ici_objwsup_t       *oldsuper;

    if (ici_typecheck("o*", &o))
        return 1;
    if (!hassuper(o))
        return ici_argerror(0);
    newsuper = oldsuper = o->o_super;
    if (NARGS() >= 2)
    {
        if (objof(o)->o_flags & O_ATOM)
        {
            ici_error = "attempt to set super of an atomic struct";
            return 1;
        }
        if (isnull(ARG(1)))
            newsuper = NULL;
        else if (hassuper(ARG(1)))
            newsuper = objwsupof(ARG(1));
        else
            return ici_argerror(1);
        ++ici_vsver;
    }
    o->o_super = newsuper;
    if (super_loop(o))
    {
        o->o_super = oldsuper;
        return 1;
    }
    if (oldsuper == NULL)
        return ici_null_ret();
    return ici_ret_no_decref(objof(oldsuper));
}

static int
f_scope()
{
    ici_struct_t    *s;

    s = structof(ici_vs.a_top[-1]);
    if (NARGS() > 0)
    {
        if (ici_typecheck("d", &ici_vs.a_top[-1]))
            return 1;
    }
    return ici_ret_no_decref(objof(s));
}

static int
f_isatom()
{
    ici_obj_t   *o;

    if (ici_typecheck("o", &o))
        return 1;
    if (o->o_flags & O_ATOM)
        return ici_ret_no_decref(objof(ici_one));
    else
        return ici_ret_no_decref(objof(ici_zero));
}

static int
f_alloc()
{
    long        length;
    int         accessz;
    char        *p;

    if (ici_typecheck("i*", &length))
        return 1;
    if (length < 0)
    {
        ici_error = "attempt to allocate negative amount";
        return 1;
    }
    if (NARGS() >= 2)
    {
        if
        (
            !isint(ARG(1))
            ||
            (
                (accessz = (int)intof(ARG(1))->i_value) != 1
                &&
                accessz != 2
                &&
                accessz != 4
            )
        )
            return ici_argerror(1);
    }
    else
        accessz = 1;
    if ((p = ici_alloc((size_t)length * accessz)) == NULL)
        return 1;
    memset(p, 0, (size_t)length * accessz);
    return ici_ret_with_decref(objof(ici_mem_new(p, (unsigned long)length, accessz, ici_free)));
}

#ifndef NOMEM
static int
f_mem()
{
    long        base;
    long        length;
    int         accessz;

    if (ici_typecheck("ii*", &base, &length))
        return 1;
    if (NARGS() >= 3)
    {
        if
        (
            !isint(ARG(2))
            ||
            (
                (accessz = (int)intof(ARG(2))->i_value) != 1
                &&
                accessz != 2
                &&
                accessz != 4
            )
        )
            return ici_argerror(2);
    }
    else
        accessz = 1;
    return ici_ret_with_decref(objof(ici_mem_new((char *)base, (unsigned long)length, accessz, NULL)));
}
#endif

static int
f_assign()
{
    ici_obj_t   *s;
    ici_obj_t   *k;
    ici_obj_t   *v;

    switch (NARGS())
    {
    case 2:
        if (ici_typecheck("oo", &s, &k))
            return 1;
        v = objof(&o_null);
        break;

    case 3:
        if (ici_typecheck("ooo", &s, &k, &v))
            return 1;
        break;

    default:
        return ici_argcount(2);
    }
    if (hassuper(s))
    {
        if (assign_base(s, k, v))
            return 1;
    }
    else
    {
        if (ici_assign(s, k, v))
            return 1;
    }
    return ici_ret_no_decref(v);
}

static int
f_fetch()
{
    ici_struct_t    *s;
    ici_obj_t   *k;

    if (ici_typecheck("oo", &s, &k))
        return 1;
    if (hassuper(s))
        return ici_ret_no_decref(fetch_base(s, k));
    return ici_ret_no_decref(ici_fetch(s, k));
}

#ifndef NOWAITFOR
static int
f_waitfor()
{
    register ici_obj_t  **e;
    int                 nargs;
    fd_set              readfds;
    struct timeval      timeval;
    struct timeval      *tv;
    double              to;
    int                 nfds;
    int                 i;
#ifndef fileno
    extern int          fileno();
#endif

    if (NARGS() == 0)
        return ici_ret_no_decref(objof(ici_zero));
    tv = NULL;
    nfds = 0;
    FD_ZERO(&readfds);
    to = 0.0; /* Stops warnings, not required. */
    for (nargs = NARGS(), e = ARGS(); nargs > 0; --nargs, --e)
    {
        if (isfile(*e))
        {
            /*
             * If the ft_getch routine of the file is the real stdio fgetc,
             * we can assume the file is a real stdio stream file, then
             * we also assume we can use fileno on it.
             */
            if (fileof(*e)->f_type->ft_getch == fgetc)
            {
                setvbuf((FILE *)fileof(*e)->f_file, NULL, _IONBF, 0);
                i = fileno((FILE *)fileof(*e)->f_file);
                FD_SET(i, &readfds);
                if (i >= nfds)
                    nfds = i + 1;
            }
            else
                return ici_ret_no_decref(*e);
        }
        else if (isint(*e))
        {
            if (tv == NULL || to > intof(*e)->i_value / 1000.0)
            {
                to = intof(*e)->i_value / 1000.0;
                tv = &timeval;
            }
        }
        else if (isfloat(*e))
        {
            if (tv == NULL || to > floatof(*e)->f_value)
            {
                to = floatof(*e)->f_value;
                tv = &timeval;
            }
        }
        else
            return ici_argerror(ARGS() - e);
    }
    if (tv != NULL)
    {
        tv->tv_sec = to;
        tv->tv_usec = (to - tv->tv_sec) * 1000000.0;
    }
    ici_signals_blocking_syscall(1);
    switch (select(nfds, &readfds, NULL, NULL, tv))
    {
    case -1:
    ici_signals_blocking_syscall(0);
        ici_error = "could not select";
        return 1;

    case 0:
    ici_signals_blocking_syscall(0);
        return ici_ret_no_decref(objof(ici_zero));
    }
    ici_signals_blocking_syscall(0);
    for (nargs = NARGS(), e = ARGS(); nargs > 0; --nargs, --e)
    {
        if (!isfile(*e))
            continue;
        if (fileof(*e)->f_type->ft_getch == fgetc)
        {
            i = fileno((FILE *)fileof(*e)->f_file);
            if (FD_ISSET(i, &readfds))
                return ici_ret_no_decref(*e);
        }
    }
    ici_error = "no file selected";
    return 1;
}
#endif /* ifndef NOWAITFOR */

static int
f_gettoken()
{
    ici_file_t          *f;
    ici_str_t           *s;
    unsigned char       *seps;
    int                 nseps;
    char                *file;
    int                 (*get)();
    int                 c;
    int                 i;
    int                 j;

    seps = (unsigned char *)" \t\n";
    nseps = 3;
    switch (NARGS())
    {
    case 0:
        if ((f = ici_need_stdin()) == NULL)
            return 1;
        break;

    case 1:
        if (ici_typecheck("o", &f))
            return 1;
        if (isstring(objof(f)))
        {
            if ((f = ici_sopen(stringof(f)->s_chars, stringof(f)->s_nchars, objof(f))) == NULL)
                return 1;
            ici_decref(f);
        }
        else if (!isfile(objof(f)))
            return ici_argerror(0);
        break;

    default:
        if (ici_typecheck("oo", &f, &s))
            return 1;
        if (isstring(objof(f)))
        {
            if ((f = ici_sopen(stringof(f)->s_chars, stringof(f)->s_nchars, objof(f))) == NULL)
                return 1;
            ici_decref(f);
        }
        else if (!isfile(objof(f)))
            return ici_argerror(0);
        if (!isstring(objof(s)))
            return ici_argerror(1);
        seps = (unsigned char *)s->s_chars;
        nseps = s->s_nchars;
        break;
    }
    get = f->f_type->ft_getch;
    file = f->f_file;
    do
    {
        c = (*get)(file);
        if (c == EOF)
            return ici_null_ret();
        for (i = 0; i < nseps; ++i)
        {
            if (c == seps[i])
                break;
        }

    } while (i != nseps);

    j = 0;
    do
    {
        ici_chkbuf(j);
        buf[j++] = c;
        c = (*get)(file);
        if (c == EOF)
            break;
        for (i = 0; i < nseps; ++i)
        {
            if (c == seps[i])
            {
                (*f->f_type->ft_ungetch)(c, file);
                break;
            }
        }

    } while (i == nseps);

    if ((s = ici_str_new(buf, j)) == NULL)
        return 1;
    return ici_ret_with_decref(objof(s));
}

/*
 * Fast (relatively) version for gettokens() if argument is not file.
 */
static int
fast_gettokens(char *str, char *delims)
{
    ici_array_t *a;
    int         k       = 0;
    char        *cp     = str;

    if ((a = ici_array_new(0)) == NULL)
        return 1;
    while (*cp)
    {
        while (*cp && strchr(delims, *cp))
            cp++;
        if ((k = strcspn(cp, delims)))
        {
            if
            (
                ici_stk_push_chk(a, 1)
                ||
                (*a->a_top = objof(ici_str_new(cp, k))) == NULL
            )
            {
                ici_decref(a);
                return 1;
            }
            ici_decref(*a->a_top);
            ++a->a_top;
            if (*(cp += k))
                cp++;
            continue;
        }
    }
    if (a->a_top == a->a_base)
    {
        ici_decref(a);
        return ici_null_ret();
    }
    return ici_ret_with_decref(objof(a));
}

static int
f_gettokens()
{
    ici_file_t          *f;
    ici_str_t           *s;
    unsigned char       *terms;
    int                 nterms;
    unsigned char       *seps;
    int                 nseps;
    unsigned char       *delims = NULL; /* init to shut up compiler */
    int                 ndelims;
    int                 hardsep;
    unsigned char       sep;
    char                *file;
    ici_array_t         *a;
    int                 (*get)();
    int                 c;
    int                 i;
    int                 j = 0; /* init to shut up compiler */
    int                 state;
    int                 what;
    int                 loose_it = 0;

    seps = (unsigned char *)" \t";
    nseps = 2;
    hardsep = 0;
    terms = (unsigned char *)"\n";
    nterms = 1;
    ndelims = 0;
    switch (NARGS())
    {
    case 0:
        if ((f = ici_need_stdin()) == NULL)
            return 1;
        break;

    case 1:
        if (ici_typecheck("o", &f))
            return 1;
        if (isstring(objof(f)))
        {
            return fast_gettokens(stringof(f)->s_chars, " \t");
        }
        else if (!isfile(objof(f)))
            return ici_argerror(0);
        break;

    case 2:
    case 3:
    case 4:
        if (ici_typecheck("oo*", &f, &s))
            return 1;
        if (NARGS() == 2 && isstring(objof(f)) && isstring(objof(s)))
        {
            return fast_gettokens(stringof(f)->s_chars, stringof(s)->s_chars);
        }
        if (isstring(objof(f)))
        {
            if ((f = ici_sopen(stringof(f)->s_chars, stringof(f)->s_nchars, objof(f))) == NULL)
                return 1;
            loose_it = 1;
        }
        else if (!isfile(objof(f)))
            return ici_argerror(0);
        if (isint(objof(s)))
        {
            sep = (unsigned char)intof(objof(s))->i_value;
            hardsep = 1;
            seps = (unsigned char *)&sep;
            nseps = 1;
        }
        else if (isstring(objof(s)))
        {
            seps = (unsigned char *)s->s_chars;
            nseps = s->s_nchars;
        }
        else
        {
            if (loose_it)
                ici_decref(f);
            return ici_argerror(1);
        }
        if (NARGS() > 2)
        {
            if (!isstring(ARG(2)))
            {
                if (loose_it)
                    ici_decref(f);
                return ici_argerror(2);
            }
            terms = (unsigned char *)stringof(ARG(2))->s_chars;
            nterms = stringof(ARG(2))->s_nchars;
            if (NARGS() > 3)
            {
                if (!isstring(ARG(3)))
                {
                    if (loose_it)
                        ici_decref(f);
                    return ici_argerror(3);
                }
                delims = (unsigned char *)stringof(ARG(3))->s_chars;
                ndelims = stringof(ARG(3))->s_nchars;
            }
        }
        break;

    default:
        return ici_argcount(4);
    }
    get = f->f_type->ft_getch;
    file = f->f_file;

#define S_IDLE  0
#define S_INTOK 1

#define W_EOF   0
#define W_SEP   1
#define W_TERM  2
#define W_TOK   3
#define W_DELIM 4

    state = S_IDLE;
    if ((a = ici_array_new(0)) == NULL)
        goto fail;
    for (;;)
    {
        /*
         * Get the next character and classify it.
         */
        if ((c = (*get)(file)) == EOF)
        {
            what = W_EOF;
            goto got_what;
        }
        for (i = 0; i < nseps; ++i)
        {
            if (c == seps[i])
            {
                what = W_SEP;
                goto got_what;
            }
        }
        for (i = 0; i < nterms; ++i)
        {
            if (c == terms[i])
            {
                what = W_TERM;
                goto got_what;
            }
        }
        for (i = 0; i < ndelims; ++i)
        {
            if (c == delims[i])
            {
                what = W_DELIM;
                goto got_what;
            }
        }
        what = W_TOK;
    got_what:

        /*
         * Act on state and current character classification.
         */
        switch ((state << 8) + what)
        {
        case (S_IDLE << 8) + W_EOF:
            if (loose_it)
                ici_decref(f);
            if (a->a_top == a->a_base)
            {
                ici_decref(a);
                return ici_null_ret();
            }
            return ici_ret_with_decref(objof(a));

        case (S_IDLE << 8) + W_TERM:
            if (!hardsep)
            {
                if (loose_it)
                    ici_decref(f);
                return ici_ret_with_decref(objof(a));
            }
            j = 0;
        case (S_INTOK << 8) + W_EOF:
        case (S_INTOK << 8) + W_TERM:
            if (ici_stk_push_chk(a, 1))
                goto fail;
            if ((s = ici_str_new(buf, j)) == NULL)
                goto fail;
            *a->a_top++ = objof(s);
            if (loose_it)
                ici_decref(f);
            ici_decref(s);
            return ici_ret_with_decref(objof(a));

        case (S_IDLE << 8) + W_SEP:
            if (!hardsep)
                break;
            j = 0;
        case (S_INTOK << 8) + W_SEP:
            if (ici_stk_push_chk(a, 1))
                goto fail;
            if ((s = ici_str_new(buf, j)) == NULL)
                goto fail;
            *a->a_top++ = objof(s);
            ici_decref(s);
            if (hardsep)
            {
                j = 0;
                state = S_INTOK;
            }
            else
                state = S_IDLE;
            break;

        case (S_INTOK << 8) + W_DELIM:
            if (ici_stk_push_chk(a, 1))
                goto fail;
            if ((s = ici_str_new(buf, j)) == NULL)
                goto fail;
            *a->a_top++ = objof(s);
            ici_decref(s);
        case (S_IDLE << 8) + W_DELIM:
            if (ici_stk_push_chk(a, 1))
                goto fail;
            buf[0] = c;
            if ((s = ici_str_new(buf, 1)) == NULL)
                goto fail;
            *a->a_top++ = objof(s);
            ici_decref(s);
            j = 0;
            state = S_IDLE;
            break;

        case (S_IDLE << 8) + W_TOK:
            j = 0;
            state = S_INTOK;
        case (S_INTOK << 8) + W_TOK:
            if (ici_chkbuf(j))
                goto fail;
            buf[j++] = c;
        }
    }

fail:
    if (loose_it)
        ici_decref(f);
    if (a != NULL)
        ici_decref(a);
    return 1;
}


/*
 * sort(array, cmp)
 */
static int
f_sort()
{
    ici_array_t *a;
    ici_obj_t   **base;
    long        n;
    ici_obj_t   *f;
    long        cmp;
    long        k;                              /* element added or removed */
    long        p;                              /* place in heap */
    long        q;                              /* place in heap */
    long        l;                              /* left child */
    long        r;                              /* right child */
    ici_obj_t   *o;                             /* object used for swapping */
    ici_obj_t   *uarg;                          /* user argument to cmp func */

/*
 * Relations within heap.
 */
#define PARENT(i)       (((i) - 1) >> 1)
#define LEFT(i)         ((i) + (i) + 1)
#define RIGHT(i)        ((i) + (i) + 2)
/*
 * Macro for swapping elements.
 */
#define SWAP(a, b)      {o = base[a]; base[a] = base[b]; base[b] = o;}
#define CMP(rp, a, b)   ici_func(f, "i=ooo", rp, base[a], base[b], uarg)

    uarg = objof(&o_null);
    switch (NARGS())
    {
    case 3:
        if (ici_typecheck("aoo", &a, &f, &uarg))
            return 1;
        if (ici_typeof(f)->t_call == NULL)
            return ici_argerror(1);
        break;

    case 2:
        if (ici_typecheck("ao", &a, &f))
            return 1;
        if (ici_typeof(f)->t_call == NULL)
            return ici_argerror(1);
        break;

    case 1:
        if (ici_typecheck("a", &a))
            return 1;
        f = ici_fetch(ici_vs.a_top[-1], SS(cmp));
        if (ici_typeof(f)->t_call == NULL)
        {
            ici_error = "no suitable cmp function in scope";
            return 1;
        }
        break;

    default:
        return ici_argcount(2);
    }
    if (objof(a)->o_flags & O_ATOM)
    {
        ici_error = "attempt to sort an atomic array";
        return 1;
    }

    n = ici_array_nels(a);
    if (a->a_bot > a->a_top)
    {
        ptrdiff_t       m;
        ici_obj_t       **e;

        /*
         * Can't sort in-place because the array has wrapped. Force the
         * array to be contiguous. ### Maybe this should be a function
         * in array.c.
         */
        m = a->a_limit - a->a_base;
        if ((e = ici_nalloc(m * sizeof(ici_obj_t *))) == NULL)
            goto fail;
        ici_array_gather(e, a, 0, n);
        ici_nfree(a->a_base, m * sizeof(ici_obj_t *));
        a->a_base = e;
        a->a_bot = e;
        a->a_top = e + n;
        a->a_limit = e + m;
    }
    base = a->a_bot;

    /*
     * Shuffle heap.
     */
    for (k = 1; k < n; ++k)
    {
        p = k;
        while (p != 0)
        {
            q = PARENT(p);
            if (CMP(&cmp, p, q))
                goto fail;
            if (cmp <= 0)
                break;
            SWAP(p, q);
            p = q;
        }
    }

    /*
     * Keep taking elements off heap and re-shuffling.
     */
    for (k = n - 1; k > 0; --k)
    {
        SWAP(0, k);
        p = 0;
        while (1)
        {
            l = LEFT(p);
            if (l >= k)
                break;
            r = RIGHT(p);
            if (r >= k)
            {
                if (CMP(&cmp, l, p))
                    goto fail;
                if (cmp <= 0)
                    break;
                SWAP(l, p);
                p = l;
            }
            else
            {
                if (CMP(&cmp, l, p))
                    goto fail;
                if (cmp <= 0)
                {
                    if (CMP(&cmp, r, p))
                        goto fail;
                    if (cmp <= 0)
                        break;
                    SWAP(r, p);
                    p = r;
                }
                else
                {
                    if (CMP(&cmp, r, l))
                        goto fail;
                    if (cmp <= 0)
                    {
                        SWAP(l, p);
                        p = l;
                    }
                    else
                    {
                        SWAP(r, p);
                        p = r;
                    }
                }
            }
        }
    }
    return ici_ret_no_decref(objof(a));

fail:
    return 1;

#undef  PARENT
#undef  LEFT
#undef  RIGHT
#undef  SWAP
}

#ifdef WHOALLOC
static int
f_reclaim()
{
    ici_reclaim();
    return ici_null_ret();
}
#endif

static int
f_abs()
{
    if (isint(ARG(0)))
    {
        if (intof(ARG(0))->i_value >= 0)
            return ici_ret_no_decref(ARG(0));
        return ici_int_ret(-intof(ARG(0))->i_value);
    }
    else if (isfloat(ARG(0)))
    {
        if (floatof(ARG(0))->f_value >= 0)
            return ici_ret_no_decref(ARG(0));
        return ici_float_ret(-floatof(ARG(0))->f_value);
    }
    return ici_argerror(0);
}

static int              got_epoch_time;
static time_t           epoch_time;

static void
get_epoch_time()
{
    struct tm   tm;

    memset(&tm, 0, sizeof tm);
    tm.tm_year = 100; /* 2000 is 100 years since 1900. */
    tm.tm_mday = 1;    /* First day of month is 1 */
    epoch_time = mktime(&tm);
    got_epoch_time = 1;
}

static int
f_now()
{
    if (!got_epoch_time)
        get_epoch_time();
    return ici_float_ret(difftime(time(NULL), epoch_time));
}

static int
f_calendar()
{
    ici_objwsup_t       *s;
    double              d;
    long                l;

    s = NULL;
    if (!got_epoch_time)
        get_epoch_time();
    if (NARGS() != 1)
        return ici_argcount(1);
    if (isfloat(ARG(0)))
    {
        time_t          t;
        struct tm       *tm;

        /*
         * This is really bizarre. ANSI C doesn't define what the units
         * of a time_t are, just that it is an arithmetic type. difftime()
         * can be used to get seconds from time_t values, but their is no
         * inverse operation. So without discovering a conversion factor
         * from calls to mktime(), one can't usefully combine a time in,
         * say seconds, with a time_t. But we all know that time_t is
         * really in seconds. So I'll just assume that.
         */
        t = epoch_time + (time_t)floatof(ARG(0))->f_value;
        tm = localtime(&t);
        if ((s = objwsupof(ici_struct_new())) == NULL)
            return 1;
        if
        (
               ici_set_val(s, SS(second), 'f', (d = tm->tm_sec, &d))
            || ici_set_val(s, SS(minute), 'i', (l = tm->tm_min, &l))
            || ici_set_val(s, SS(hour), 'i', (l = tm->tm_hour, &l))
            || ici_set_val(s, SS(day), 'i', (l = tm->tm_mday, &l))
            || ici_set_val(s, SS(month), 'i', (l = tm->tm_mon, &l))
            || ici_set_val(s, SS(year), 'i', (l = tm->tm_year + 1900, &l))
            || ici_set_val(s, SS(wday), 'i', (l = tm->tm_wday, &l))
            || ici_set_val(s, SS(yday), 'i', (l = tm->tm_yday, &l))
            || ici_set_val(s, SS(isdst), 'i', (l = tm->tm_isdst, &l))
        )
        {
            ici_decref(s);
            return 1;
        }
        return ici_ret_with_decref(objof(s));
    }
    else if (isstruct(ARG(0)))
    {
        time_t          t;
        struct tm       tm;

        memset(&tm, 0, sizeof tm);
        s = objwsupof(ARG(0));
        if (ici_fetch_num(objof(s), SSO(second), &d))
            return 1;
        tm.tm_sec = (int)d;
        if (ici_fetch_int(objof(s), SSO(minute), &l))
            return 1;
        tm.tm_min = l;
        if (ici_fetch_int(objof(s), SSO(hour), &l))
            return 1;
        tm.tm_hour = l;
        if (ici_fetch_int(objof(s), SSO(day), &l))
            return 1;
        tm.tm_mday = l;
        if (ici_fetch_int(objof(s), SSO(month), &l))
            return 1;
        tm.tm_mon = l;
        if (ici_fetch_int(objof(s), SSO(year), &l))
            return 1;
        tm.tm_year = l - 1900;
        if (ici_fetch_int(objof(s), SSO(isdst), &l))
            tm.tm_isdst = -1;
        else
            tm.tm_isdst = l;
        t = mktime(&tm);
        if (t == (time_t)-1)
        {
            ici_error = "unsuitable calendar time";
            return 1;
        }
        return ici_float_ret(difftime(t, epoch_time));
    }
    return ici_argerror(0);
}

/*
 * ICI: sleep(num)
 */
static int
f_sleep()
{
    double              how_long;
    ici_exec_t          *x;

    if (ici_typecheck("n", &how_long))
        return 1;

#ifdef _WIN32
    {
        long            t;

        how_long *= 1000; /* Convert to milliseconds. */
        if (how_long > LONG_MAX)
            t = LONG_MAX;
        else if ((t = (long)how_long) < 1)
            t = 1;
        x = ici_leave();
        Sleep(t);
        ici_enter(x);
    }
#else
    {
        long            t;

        if (how_long > LONG_MAX)
            t = LONG_MAX;
        else if ((t = how_long) < 1)
            t = 1;
        x = ici_leave();
        sleep(t);
        ici_enter(x);
    }
#endif
    return ici_null_ret();
}

/*
 * Return the accumulated cpu time in seconds as a float. The precision
 * is system dependent. If a float argument is provided, this forms a new
 * base cpu time from which future cputime values are relative to. Thus
 * if 0.0 is passed, future (but not this one) returns measure new CPU
 * time accumulated since this call.
 */
static int
f_cputime()
{
    static double       base;
    double              t;

#ifdef  _WIN32
    FILETIME            c;
    FILETIME            e;
    FILETIME            k;
    FILETIME            user;

    if (!GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &user))
        return ici_get_last_win32_error();
    t = (user.dwLowDateTime + user.dwHighDateTime * 4294967296.0) / 1e7;

#else
# ifdef BSD
    struct rusage rusage;

    getrusage(RUSAGE_SELF, &rusage);
    t = rusage.ru_utime.tv_sec + (rusage.ru_utime.tv_usec / 1.0e6);

#else /* _WIN32 */
    ici_error = "cputime function not available on this platform";
    return 1;
# endif
#endif
    t -= base;
    if (NARGS() > 0 && isfloat(ARG(0)))
        base = floatof(ARG(0))->f_value + t;
    return ici_float_ret(t);
}

ici_str_t               *ici_ver_cache;

static int
f_version()
{

    if
    (
        ici_ver_cache == NULL
        &&
        (ici_ver_cache = ici_str_new_nul_term(ici_version_string)) == NULL
    )
        return 1;
    return ici_ret_no_decref(objof(ici_ver_cache));
}

static int
f_strbuf()
{
    ici_str_t           *s;
    ici_str_t           *is;
    int                 n;

    is = NULL;
    n = 10;
    if (NARGS() > 0)
    {
        if (!isstring(ARG(0)))
            return ici_argerror(0);
        is = stringof(ARG(0));
        n = is->s_nchars;
    }
    if ((s = ici_str_buf_new(n)) == NULL)
        return 1;
    if (is != NULL)
    {
        memcpy(s->s_chars, is->s_chars, n);
        s->s_nchars = n;
    }
    return ici_ret_with_decref(objof(s));
}

static int
f_strcat()
{
    ici_str_t           *s1;
    ici_str_t           *s2;
    int                 n;
    int                 i;
    int                 si;
    int                 z;
    int                 sz;


    if (NARGS() < 2)
        return ici_argcount(2);
    if (!isstring(ARG(0)))
        return ici_argerror(0);
    s1 = stringof(ARG(0));
    if (isint(ARG(1)))
    {
        si = 2;
        sz = intof(ARG(1))->i_value;
        if (sz < 0 || sz > s1->s_nchars)
            return ici_argerror(1);
    }
    else
    {
        si = 1;
        sz = s1->s_nchars;
    }
    n = NARGS();
    for (i = si, z = sz; i < n; ++i)
    {
        s2 = stringof(ARG(i));
        if (!isstring(objof(s2)))
            return ici_argerror(i);
        z += s2->s_nchars;
    }
    if (ici_str_need_size(s1, z))
        return 1;
    for (i = si, z = sz; i < n; ++i)
    {
        s2 = stringof(ARG(i));
        memcpy(&s1->s_chars[z], s2->s_chars, s2->s_nchars);
        z += s2->s_nchars;
    }
    if (s1->s_nchars < z)
        s1->s_nchars = z;
    s1->s_chars[s1->s_nchars] = '\0';
    return ici_ret_no_decref(objof(s1));
}

static int
f_which()
{
    ici_objwsup_t       *s;
    ici_obj_t           *k;

    s = NULL;
    if (ici_typecheck(NARGS() < 2 ? "o" : "oo", &k, &s))
        return 1;
    if (s == NULL)
        s = objwsupof(ici_vs.a_top[-1]);
    else if (!hassuper(s))
        return ici_argerror(0);
    while (s != NULL)
    {
        if (isstruct(s))
        {
            if (find_raw_slot(structof(s), k)->sl_key == k)
                return ici_ret_no_decref(objof(s));
        }
        else
        {
            ici_objwsup_t   *t;
            ici_obj_t       *v;
            int             r;

            t = s->o_super;
            s->o_super = NULL;
            r = ici_fetch_super(s, k, &v, NULL);
            s->o_super = t;
            switch (r)
            {
            case -1: return 1;
            case  1: return ici_ret_no_decref(objof(s));
            }
        }
        s = s->o_super;
    }
    return ici_null_ret();
}

/*
 * Cleans up data structures allocated/referenced in this module.
 * Required for a clean shutdown.
 */
void
uninit_cfunc(void)
{
}

ici_cfunc_t std_cfuncs[] =
{
    {CF_OBJ,    (char *)SS(array),        f_array},
    {CF_OBJ,    (char *)SS(copy),         f_copy},
    {CF_OBJ,    (char *)SS(exit),         f_exit},
    {CF_OBJ,    (char *)SS(fail),         f_fail},
    {CF_OBJ,    (char *)SS(float),        f_float},
    {CF_OBJ,    (char *)SS(int),          f_int},
    {CF_OBJ,    (char *)SS(eq),           f_eq},
    {CF_OBJ,    (char *)SS(parse),        f_parse},
    {CF_OBJ,    (char *)SS(string),       f_string},
    {CF_OBJ,    (char *)SS(struct),       f_struct},
    {CF_OBJ,    (char *)SS(set),          f_set},
    {CF_OBJ,    (char *)SS(typeof),       f_typeof},
    {CF_OBJ,    (char *)SS(push),         f_push},
    {CF_OBJ,    (char *)SS(pop),          f_pop},
    {CF_OBJ,    (char *)SS(rpush),        f_rpush},
    {CF_OBJ,    (char *)SS(rpop),         f_rpop},
    {CF_OBJ,    (char *)SS(call),         f_call},
    {CF_OBJ,    (char *)SS(keys),         f_keys},
    {CF_OBJ,    (char *)SS(vstack),       f_vstack},
    {CF_OBJ,    (char *)SS(tochar),       f_tochar},
    {CF_OBJ,    (char *)SS(toint),        f_toint},
    {CF_OBJ,    (char *)SS(rand),         f_rand},
    {CF_OBJ,    (char *)SS(interval),     f_interval},
    {CF_OBJ,    (char *)SS(explode),      f_explode},
    {CF_OBJ,    (char *)SS(implode),      f_implode},
    {CF_OBJ,    (char *)SS(sopen),        f_sopen},
    {CF_OBJ,    (char *)SS(mopen),        f_mopen},
    {CF_OBJ,    (char *)SS(sprintf),      f_sprintf},
    {CF_OBJ,    (char *)SS(currentfile),  f_currentfile},
    {CF_OBJ,    (char *)SS(del),          f_del},
    {CF_OBJ,    (char *)SS(alloc),        f_alloc},
    {CF_OBJ,    (char *)SS(mem),          f_mem},
    {CF_OBJ,    (char *)SS(nels),         f_nels},
    {CF_OBJ,    (char *)SS(super),        f_super},
    {CF_OBJ,    (char *)SS(scope),        f_scope},
    {CF_OBJ,    (char *)SS(isatom),       f_isatom},
    {CF_OBJ,    (char *)SS(gettoken),     f_gettoken},
    {CF_OBJ,    (char *)SS(gettokens),    f_gettokens},
    {CF_OBJ,    (char *)SS(num),          f_num},
    {CF_OBJ,    (char *)SS(assign),       f_assign},
    {CF_OBJ,    (char *)SS(fetch),        f_fetch},
    {CF_OBJ,    (char *)SS(abs),          f_abs},
#ifndef NOMATH
    {CF_OBJ,    (char *)SS(sin),          f_math, sin,    "f=n"},
    {CF_OBJ,    (char *)SS(cos),          f_math, cos,    "f=n"},
    {CF_OBJ,    (char *)SS(tan),          f_math, tan,    "f=n"},
    {CF_OBJ,    (char *)SS(asin),         f_math, asin,   "f=n"},
    {CF_OBJ,    (char *)SS(acos),         f_math, acos,   "f=n"},
    {CF_OBJ,    (char *)SS(atan),         f_math, atan,   "f=n"},
    {CF_OBJ,    (char *)SS(atan2),        f_math, atan2,  "f=nn"},
    {CF_OBJ,    (char *)SS(exp),          f_math, exp,    "f=n"},
    {CF_OBJ,    (char *)SS(log),          f_math, log,    "f=n"},
    {CF_OBJ,    (char *)SS(log10),        f_math, log10,  "f=n"},
    {CF_OBJ,    (char *)SS(pow),          f_math, pow,    "f=nn"},
    {CF_OBJ,    (char *)SS(sqrt),         f_math, sqrt,   "f=n"},
    {CF_OBJ,    (char *)SS(floor),        f_math, floor,  "f=n"},
    {CF_OBJ,    (char *)SS(ceil),         f_math, ceil,   "f=n"},
    {CF_OBJ,    (char *)SS(fmod),         f_math, fmod,   "f=nn"},
#endif
#ifndef NOWAITFOR
    {CF_OBJ,    (char *)SS(waitfor),      f_waitfor},
#endif
    {CF_OBJ,    (char *)SS(top),          f_top},
    {CF_OBJ,    (char *)SS(include),      f_include},
    {CF_OBJ,    (char *)SS(sort),         f_sort},
#ifdef  WHOALLOC
    {CF_OBJ,    (char *)SS(reclaim),      f_reclaim},
#endif
    {CF_OBJ,    (char *)SS(now),          f_now},
    {CF_OBJ,    (char *)SS(calendar),     f_calendar},
    {CF_OBJ,    (char *)SS(version),      f_version},
    {CF_OBJ,    (char *)SS(cputime),      f_cputime},
    {CF_OBJ,    (char *)SS(sleep),        f_sleep},
    {CF_OBJ,    (char *)SS(strbuf),       f_strbuf},
    {CF_OBJ,    (char *)SS(strcat),       f_strcat},
    {CF_OBJ,    (char *)SS(which),        f_which},
    {CF_OBJ,    (char *)SS(cmp),          f_coreici, SS(cmp),       SS(core1)},
    {CF_OBJ,    (char *)SS(pathjoin),     f_coreici, SS(pathjoin),  SS(core2)},
    {CF_OBJ,    (char *)SS(basename),     f_coreici, SS(basename),  SS(core2)},
    {CF_OBJ,    (char *)SS(dirname),      f_coreici, SS(dirname),   SS(core2)},
    {CF_OBJ,    (char *)SS(pfopen),       f_coreici, SS(pfopen),    SS(core2)},
    {CF_OBJ,    (char *)SS(use),          f_coreici, SS(use),       SS(core2)},
    {CF_OBJ,    (char *)SS(min),          f_coreici, SS(min),       SS(core3)},
    {CF_OBJ,    (char *)SS(max),          f_coreici, SS(max),       SS(core3)},
    {CF_OBJ,    (char *)SS(argerror),     f_coreici, SS(argerror),  SS(core3)},
    {CF_OBJ,    (char *)SS(argcount),     f_coreici, SS(argcount),  SS(core3)},
    {CF_OBJ,    (char *)SS(typecheck),    f_coreici, SS(typecheck), SS(core3)},
    {CF_OBJ}
};
