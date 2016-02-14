#define ICI_CORE
#include "float.h"
#include "primes.h"
#include <assert.h>

/*
 * Return an ICI float object corresponding to the given value 'v'.  Note that
 * floats are intrinsically atomic.  The returned object will have had its
 * reference count inceremented. Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_float_t *
ici_float_new(double v)
{
    ici_float_t          *f;
    ici_obj_t           **po;
    static ici_float_t   proto = {OBJ(TC_FLOAT)};

    proto.f_value = v;
    if ((f = floatof(atom_probe(objof(&proto), &po))) != NULL)
    {
        ici_incref(f);
        return f;
    }
    ++ici_supress_collect;
    if ((f = ici_talloc(ici_float_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(f, TC_FLOAT, O_ATOM, 1, sizeof(ici_float_t));
    f->f_value = v;
    ici_rego(f);
    --ici_supress_collect;
    ICI_STORE_ATOM_AND_COUNT(po, f);
    return f;
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_float(ici_obj_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof(ici_float_t);
}

/*
 * Returns 0 if these objects are eq, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
static int
cmp_float(ici_obj_t *o1, ici_obj_t *o2)
{
    assert(sizeof(double) == 2 * sizeof(long));
    return !DBL_BIT_CMP(&floatof(o1)->f_value, &floatof(o2)->f_value);
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
static void
free_float(ici_obj_t *o)
{
    ici_tfree(o, ici_float_t);
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
static unsigned long
hash_float(ici_obj_t *o)
{
    unsigned long       h;
    int                 i;

    h = FLOAT_PRIME;
    /*
     * We assume that the compiler will decide this constant expression
     * at compile time and not actually make a run-time decision about
     * which bit of code to run.
     *
     * WARNING: there is an in-line expansion of this in binop.h.
     */
    if (sizeof floatof(o)->f_value == 2 * sizeof(unsigned long))
    {
        unsigned long   *p;

        p = (unsigned long *)&floatof(o)->f_value;
        h += p[0] + p[1] * 31;
        h ^= (h >> 12) ^ (h >> 24);
    }
    else
    {
        unsigned char   *p;

        p = (unsigned char *)&floatof(o)->f_value;
        i = sizeof(floatof(o)->f_value);
        while (--i >= 0)
            h = *p++ + h * 31;
    }
    return h;
}

ici_type_t  float_type =
{
    mark_float,
    free_float,
    hash_float,
    cmp_float,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "float"
};
