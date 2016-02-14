#define ICI_CORE
#include "func.h"
#include "exec.h"
#include "ptr.h"
#include "struct.h"
#include "op.h"
#include "pc.h"
#include "str.h"
#include "catch.h"
#include "buf.h"
#include "mark.h"
#include "null.h"
#include "primes.h"
#ifndef NOPROFILE
#include "profile.h"
#endif

static void
objname_func(ici_obj_t *o, char p[ICI_OBJNAMEZ])
{
    ici_str_t   *s;

    s = funcof(o)->f_name;
    if (s->s_nchars > ICI_OBJNAMEZ - 2 - 1)
        sprintf(p, "%.*s...()", ICI_OBJNAMEZ - 6, s->s_chars);
    else
        sprintf(p, "%s()", s->s_chars);
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_func(ici_obj_t *o)
{
    long        mem;

    o->o_flags |= O_MARK;
    mem = sizeof(ici_func_t);
    if (funcof(o)->f_code != NULL)
        mem += ici_mark(objof(funcof(o)->f_code));
    if (funcof(o)->f_args != NULL)
        mem += ici_mark(objof(funcof(o)->f_args));
    if (funcof(o)->f_autos != NULL)
        mem += ici_mark(objof(funcof(o)->f_autos));
    if (funcof(o)->f_name != NULL)
        mem += ici_mark(objof(funcof(o)->f_name));
    return mem;
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
static int
cmp_func(ici_obj_t *o1, ici_obj_t *o2)
{
    return funcof(o1)->f_code != funcof(o2)->f_code
        || funcof(o1)->f_autos != funcof(o2)->f_autos
        || funcof(o1)->f_args != funcof(o2)->f_args
        || funcof(o1)->f_name != funcof(o2)->f_name;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
static unsigned long
hash_func(ici_obj_t *o)
{
    return (unsigned long)funcof(o)->f_code * FUNC_PRIME;
}

ici_func_t *
new_func()
{
    register ici_func_t *f;

    if ((f = ici_talloc(ici_func_t)) == NULL)
        return NULL;
    memset((char *)f, 0, sizeof(ici_func_t));
    ICI_OBJ_SET_TFNZ(f, TC_FUNC, 0, 1, 0);
    ici_rego(f);
    return f;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
static void
free_func(ici_obj_t *o)
{
    ici_tfree(o, ici_func_t);
}

/*
 * Return the object at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 */
static ici_obj_t *
fetch_func(ici_obj_t *o, ici_obj_t *k)
{
    ici_obj_t           *r;

    ici_error = NULL;
    r = NULL;
    if (k == SSO(autos))
        r = objof(funcof(o)->f_autos);
    else if (k == SSO(args))
        r = objof(funcof(o)->f_args);
    else if (k == SSO(name))
        r = objof(funcof(o)->f_name);
    if (r == NULL && ici_error == NULL)
        r = objof(&o_null);
    return r;
}

int
ici_op_return()
{
    ici_obj_t           **x;
    static int          occasionally;
    ici_obj_t           *f;

    if (ici_debug_active)
        ici_debug->idbg_fnresult(ici_os.a_top[-1]);

    x = ici_xs.a_top - 1;
    while
    (
        !ismark(*x)
        &&
        --x >= ici_xs.a_base
        &&
        !(iscatch(*x) && isnull(catchof(*x)->c_catcher))
    )
        ;
    if (x < ici_xs.a_base || !ismark(*x))
    {
        ici_error = "return not in function";
        return 1;
    }
    ici_xs.a_top = x;

    /*
     * If convenient, record the total nels of the autos that this function
     * ended up with, as a hint for the auto struct allocation on next call.
     * If it isn't convenient, do it occasionally anyway.
     */
    if
    (
        objof(SS(_func_)->s_struct) == ici_vs.a_top[-1]
        &&
        SS(_func_)->s_vsver == ici_vsver
        &&
        isfunc(f = SS(_func_)->s_slot->sl_value)
    )
    {
        funcof(f)->f_nautos = structof(ici_vs.a_top[-1])->s_nels;
    }
    else if (--occasionally <= 0)
    {
        occasionally = 10;
        f = ici_fetch(ici_vs.a_top[-1], SSO(_func_));
        if (isstruct(ici_vs.a_top[-1]) && isfunc(f))
            funcof(f)->f_nautos = structof(ici_vs.a_top[-1])->s_nels;
    }

    --ici_vs.a_top;
#ifndef NOPROFILE
    if (ici_profile_active)
        ici_profile_return();
#endif
    return 0;
}

/*
 * arg(N-1) .. arg1 arg0 nargs func     => (os) OR
 * arg(N-1) .. arg1 arg0 nargs ptr      => (os) OR
 * arg(N-1) .. arg1 arg0 nargs aggr key => (os) iff OP_AGGR_KEY_CALL
 *                                => auto-struct  (vs)
 *                      call      => mark pc      (xs)
 *
 * Calling a function pushes a structure for auto variables on the
 * variable stack. It then pushes a mark and a pc starting at the first
 * element of the code array on the execution stack. Any arguments are
 * assigned to the corresponding formal argument names in the auto var
 * structure.
 */
static int
call_func(ici_obj_t *o, ici_obj_t *subject)
{
    register ici_func_t *f;
    register ici_struct_t   *d;     /* The local variable structure. */
    register ici_obj_t  **ap;   /* Actual parameter. */
    register ici_obj_t  **fp;   /* Formal parameter. */
    ici_sslot_t         *sl;
    ici_array_t         *va;
    int                 n;

    f = funcof(o);
#ifndef NOPROFILE
    if (ici_profile_active)
        ici_profile_call(f);
#endif

    if ((d = structof(copy(objof(f->f_autos)))) == NULL)
        goto fail;
    if (subject != NULL)
    {
        /*
         * This is a method call, that is, it has a subject object that
         * becomes the scope.
         */
        if (!hassuper(subject))
        {
            char        n1[30];

            sprintf(buf, "attempt to call method on %s", ici_objname(n1, subject));
            ici_error = buf;
            goto fail;
        }
        objwsupof(d)->o_super = objwsupof(subject);
        /*
         * Set the special instantiation variables.
         */
        if (assign_base(d, SSO(this), subject))
            goto fail;
        if
        (
            objwsupof(f->f_autos)->o_super != NULL
            &&
            assign_base(d, SSO(class), objwsupof(f->f_autos)->o_super)
        )
            goto fail;
    }
    n = NARGS(); /* Number of actual args. */
    ap = ARGS();
    if (f->f_args != NULL)
    {
        /*
         * There are explicit formal parameters.
         */
        fp = f->f_args->a_base;
        /*
         * Assign the actuals to the formals.
         */
        while (fp < f->f_args->a_top && n > 0)
        {
            assert(isstring(*fp));
            if (stringof(*fp)->s_struct == d && stringof(*fp)->s_vsver == ici_vsver)
            {
                stringof(*fp)->s_slot->sl_value = *ap;
            }
            else
            {
                if (ici_assign(d, *fp, *ap))
                    goto fail;
            }
            ++fp;
            --ap;
            --n;
        }
    }
    va = NULL;
    if
    (
        n > 0
        &&
        (sl = find_raw_slot(d, SSO(vargs))) != NULL
        &&
        (va = ici_array_new(n)) != NULL
    )
    {
        /*
         * There are left-over actual parameters and a "vargs"
         * auto to put them in, and everything else looks good.
         */
        while (--n >= 0)
            *va->a_top++ = *ap--;
        sl->sl_value = objof(va);
    }
    if (va != NULL)
        ici_decref(va);

    /*
     * we push the current source marker onto the execution stack.
     * That way, after the function returns, it will cause the current
     * source marker to be reset to the correct value.
     */
    ici_xs.a_top[-1] = objof(ici_exec->x_src);

    *ici_xs.a_top++ = objof(&o_mark);
    get_pc(f->f_code, ici_xs.a_top);
    ++ici_xs.a_top;
    *ici_vs.a_top++ = objof(d);
    ici_decref(d);
    ici_os.a_top -= NARGS() + 2;
    return 0;

fail:
    if (d != NULL)
        ici_decref(d);
    return 1;
}

ici_type_t  ici_func_type =
{
    mark_func,
    free_func,
    hash_func,
    cmp_func,
    ici_copy_simple,
    ici_assign_fail,
    fetch_func,
    "func",
    objname_func,
    call_func
};

ici_op_t    o_return        = {OBJ(TC_OP), ici_op_return};
ici_op_t    o_call          = {OBJ(TC_OP), NULL, OP_CALL};
ici_op_t    o_method_call   = {OBJ(TC_OP), NULL, OP_METHOD_CALL};
ici_op_t    o_super_call    = {OBJ(TC_OP), NULL, OP_SUPER_CALL};
