#define ICI_CORE
#include "exec.h"
#include "pc.h"
#include "struct.h"
#include "set.h"
#include "forall.h"
#include "str.h"
#include "buf.h"

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_forall(ici_obj_t *o)
{
    register int        i;
    unsigned long       mem;

    o->o_flags |= O_MARK;
    mem = sizeof(ici_forall_t);
    for (i = 0; i < nels(forallof(o)->fa_objs); ++i)
    {
        if (forallof(o)->fa_objs[i] != NULL)
            mem += ici_mark(forallof(o)->fa_objs[i]);
    }
    return mem;
}

/*
 * va vk ka kk aggr code        => (os)
 *                              => forall (xs)
 */
int
ici_op_forall()
{
    register ici_forall_t   *fa;

    if (ici_os.a_top[-2] == objof(&o_null))
    {
        ici_os.a_top -= 6;
        --ici_xs.a_top;
        return 0;
    }
    if ((fa = ici_talloc(ici_forall_t)) == NULL)
        return 1;
    ICI_OBJ_SET_TFNZ(fa, TC_FORALL, 0, 0, 0);
    ici_rego(fa);
    fa->fa_index = -1;
    fa->fa_code = *--ici_os.a_top;
    fa->fa_aggr = *--ici_os.a_top;
    fa->fa_kkey = *--ici_os.a_top;
    fa->fa_kaggr = *--ici_os.a_top;
    fa->fa_vkey = *--ici_os.a_top;
    fa->fa_vaggr = *--ici_os.a_top;
    ici_xs.a_top[-1] = objof(fa);
    return 0;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
static void
free_forall(ici_obj_t *o)
{
    ici_tfree(o, ici_forall_t);
}

/*
 * forall => forall pc (xs)
 *  OR
 * forall => (xs)
 */
int
exec_forall()
{
    register ici_forall_t   *fa;
    char                n[30];

    fa = forallof(ici_xs.a_top[-1]);
    switch (fa->fa_aggr->o_tcode)
    {
    case TC_STRUCT:
        {
            register ici_struct_t   *s;
            register ici_sslot_t *sl;

            s = structof(fa->fa_aggr);
            while (++fa->fa_index < s->s_nslots)
            {
                if ((sl = &s->s_slots[fa->fa_index])->sl_key == NULL)
                    continue;
                if (fa->fa_vaggr != objof(&o_null))
                {
                    if (ici_assign(fa->fa_vaggr, fa->fa_vkey, sl->sl_value))
                        return 1;
                }
                if (fa->fa_kaggr != objof(&o_null))
                {
                    if (ici_assign(fa->fa_kaggr, fa->fa_kkey, sl->sl_key))
                        return 1;
                }
                goto next;
            }
        }
        goto fin;

    case TC_SET:
        {
            register ici_set_t  *s;
            register ici_obj_t  **sl;

            s = setof(fa->fa_aggr);
            while (++fa->fa_index < s->s_nslots)
            {
                if (*(sl = &s->s_slots[fa->fa_index]) == NULL)
                    continue;
                if (fa->fa_kaggr == objof(&o_null))
                {
                    if (fa->fa_vaggr != objof(&o_null))
                    {
                        if (ici_assign(fa->fa_vaggr, fa->fa_vkey, *sl))
                            return 1;
                    }
                }
                else
                {
                    if (fa->fa_vaggr != objof(&o_null))
                    {
                        if (ici_assign(fa->fa_vaggr, fa->fa_vkey, ici_one))
                            return 1;
                    }
                    if (ici_assign(fa->fa_kaggr, fa->fa_kkey, *sl))
                        return 1;
                }
                goto next;
            }
        }
        goto fin;

    case TC_ARRAY:
        {
            register ici_array_t    *a;
            register ici_int_t  *i;

            a = arrayof(fa->fa_aggr);
            if (++fa->fa_index >= ici_array_nels(a))
                goto fin;
            if (fa->fa_vaggr != objof(&o_null))
            {
                if (ici_assign(fa->fa_vaggr, fa->fa_vkey, ici_array_get(a, fa->fa_index)))
                    return 1;
            }
            if (fa->fa_kaggr != objof(&o_null))
            {
                if ((i = ici_int_new((long)fa->fa_index)) == NULL)
                    return 1;
                if (ici_assign(fa->fa_kaggr, fa->fa_kkey, i))
                    return 1;
                ici_decref(i);
            }
        }
        goto next;

    case TC_STRING:
        {
            register ici_str_t  *s;
            register ici_int_t  *i;

            s = stringof(fa->fa_aggr);
            if (++fa->fa_index >= s->s_nchars)
                goto fin;
            if (fa->fa_vaggr != objof(&o_null))
            {
                if ((s = ici_str_new(&s->s_chars[fa->fa_index], 1)) == NULL)
                    return 1;
                if (ici_assign(fa->fa_vaggr, fa->fa_vkey, s))
                    return 1;
                ici_decref(s);
            }
            if (fa->fa_kaggr != objof(&o_null))
            {
                if ((i = ici_int_new((long)fa->fa_index)) == NULL)
                    return 1;
                if (ici_assign(fa->fa_kaggr, fa->fa_kkey, i))
                    return 1;
                ici_decref(i);
            }
        }
        goto next;
    }
    sprintf(buf, "attempt to forall over %s", ici_objname(n, fa->fa_aggr));
    ici_error = buf;
    return 1;

next:
    get_pc(arrayof(fa->fa_code), ici_xs.a_top);
    ++ici_xs.a_top;
    return 0;

fin:
    --ici_xs.a_top;
    return 0;
}

ici_type_t  forall_type =
{
    mark_forall,
    free_forall,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "forall"
};
