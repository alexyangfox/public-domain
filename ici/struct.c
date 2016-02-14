#define ICI_CORE
#include "struct.h"
#include "ptr.h"
#include "exec.h"
#include "func.h"
#include "op.h"
#include "int.h"
#include "buf.h"
#include "str.h"
#include "pc.h"
#include "primes.h"


/*
 * Generation number of look-up look-asides.  All strings that hold a look-up
 * look-aside to shortcut struct lookups also record a generation number.
 * Whenever something happens that would invalidate such a look-aside (that we
 * can't recover from by some local operation) we bump the generation number.
 * This has the effect of globally invalidating all the current look-asides.
 *
 * This is the global generation (version) number.
 */
long    ici_vsver   = 1;

/*
 * Hash a pointer to get the initial position in a struct has table.
 */
#define HASHINDEX(k, s)  (ICI_PTR_HASH(k) & ((s)->s_nslots - 1))

/*
 * Find the struct slot which does, or should, contain the key k.  Does
 * not look down the super chain.
 */
ici_sslot_t *
find_raw_slot(ici_struct_t *s, ici_obj_t *k)
{
    register ici_sslot_t *sl;

    sl = &s->s_slots[HASHINDEX(k, s)];
    while (sl->sl_key != NULL)
    {
        if (sl->sl_key == k)
            return sl;
        if (--sl < s->s_slots)
            sl = s->s_slots + s->s_nslots - 1;
    }
    return sl;
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_struct(ici_obj_t *o)
{
    register ici_sslot_t *sl;
    long                mem;

    do /* Merge tail recursion on o_head.o_super. */
    {
        o->o_flags |= O_MARK;
        mem = sizeof(ici_struct_t) + structof(o)->s_nslots * sizeof(ici_sslot_t);
        if (structof(o)->s_nels != 0)
        {
            for
            (
                sl = &structof(o)->s_slots[structof(o)->s_nslots - 1];
                sl >= structof(o)->s_slots;
                --sl
            )
            {
                if (sl->sl_key != NULL)
                    mem += ici_mark(sl->sl_key);
                if (sl->sl_value != NULL)
                    mem += ici_mark(sl->sl_value);
            }
        }

    } while
    (
        (o = objof(structof(o)->o_head.o_super)) != NULL
        &&
        (o->o_flags & O_MARK) == 0
    );

    return mem;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
static void
free_struct(ici_obj_t *o)
{
    if (structof(o)->s_slots != NULL)
        ici_nfree(structof(o)->s_slots, structof(o)->s_nslots * sizeof(ici_sslot_t));
    ici_tfree(o, ici_struct_t);
    ++ici_vsver;
}

/*
 * Return a new ICI struct object. The returned struct has been increfed.
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_struct_t *
ici_struct_new(void)
{
    register ici_struct_t   *s;

    /*
     * NB: there is a copy of this sequence in copy_struct.
     */
    if ((s = ici_talloc(ici_struct_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(s, TC_STRUCT, O_SUPER, 1, 0);
    s->o_head.o_super = NULL;
    s->s_slots = NULL;
    s->s_nels = 0;
    s->s_nslots = 4; /* Must be power of 2. */
    if ((s->s_slots = (ici_sslot_t*)ici_nalloc(4 * sizeof(ici_sslot_t))) == NULL)
    {
        ici_tfree(s, ici_struct_t);
        return NULL;
    }
    memset(s->s_slots, 0, 4 * sizeof(ici_sslot_t));
    ici_rego(s);
    return s;
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
static int
cmp_struct(ici_obj_t *o1, ici_obj_t *o2)
{
    register int        i;
    register ici_sslot_t *sl1;
    register ici_sslot_t *sl2;

    if (structof(o1) == structof(o2))
        return 0;
    if (structof(o1)->s_nels != structof(o2)->s_nels)
        return 1;
    if (structof(o1)->o_head.o_super != structof(o2)->o_head.o_super)
        return 1;
    sl1 = structof(o1)->s_slots;
    i = structof(o1)->s_nslots;
    while (--i >= 0)
    {
        if (sl1->sl_key != NULL)
        {
            sl2 = find_raw_slot(structof(o2), sl1->sl_key);
            if (sl1->sl_key != sl2->sl_key || sl1->sl_value != sl2->sl_value)
                return 1;
        }
        ++sl1;
    }
    return 0;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
static unsigned long
hash_struct(ici_obj_t *o)
{
    int                         i;
    unsigned long               hk;
    unsigned long               hv;
    ici_sslot_t                 *sl;

    hk = 0;
    hv = 0;
    sl = structof(o)->s_slots;
    i = structof(o)->s_nels;
    /*
     * This assumes NULL will become zero when cast to unsigned long.
     */
    while (--i >= 0)
    {
        hk += (unsigned long)sl->sl_key >> 4;
        hv += (unsigned long)sl->sl_value >> 4;
        ++sl;
    }
    return hv * STRUCT_PRIME_0 + hk * STRUCT_PRIME_1 + STRUCT_PRIME_2;
}

/*
 * Invalidate the lookup lookaside of any string keyed entries in
 * this struct. This can be done for small structs as an alternative
 * to ++ici_vsver which completely invalidates all lookasides. Only
 * call this if you know exactly what you are doing.
 */
void
ici_invalidate_struct_lookaside(ici_struct_t *s)
{
    ici_sslot_t     *sl;
    ici_sslot_t     *sle;
    ici_str_t       *str;

    sl = s->s_slots;
    sle = sl + s->s_nslots;
    while (sl < sle)
    {
        if (sl->sl_key != NULL && isstring(sl->sl_key))
        {
            str = stringof(sl->sl_key);
            /*stringof(sl->sl_key)->s_vsver = 0;*/
            str->s_vsver = ici_vsver;
            str->s_struct = s;
            str->s_slot = sl;
        }
        ++sl;
    }
}

/*
 * Return a copy of the given object, or NULL on error.
 * See the comment on t_copy() in object.h.
 */
static ici_obj_t *
copy_struct(ici_obj_t *o)
{
    ici_struct_t    *s;
    ici_struct_t    *ns;

    s = structof(o);
    if ((ns = (ici_struct_t *)ici_talloc(ici_struct_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(ns, TC_STRUCT, O_SUPER, 1, 0);
    ns->o_head.o_super = s->o_head.o_super;
    ns->s_nels = 0;
    ns->s_nslots = 0;
    ns->s_slots = NULL;
    ici_rego(ns);
    if ((ns->s_slots = (ici_sslot_t*)ici_nalloc(s->s_nslots * sizeof(ici_sslot_t))) == NULL)
        goto fail;
    memcpy((char *)ns->s_slots, (char *)s->s_slots, s->s_nslots*sizeof(ici_sslot_t));
    ns->s_nels = s->s_nels;
    ns->s_nslots = s->s_nslots;
    if (ns->s_nslots <= 16)
        ici_invalidate_struct_lookaside(ns);
    else
        ++ici_vsver;
    return objof(ns);

fail:
    ici_decref(ns);
    return NULL;
}

#ifdef  NOTYET
ici_struct_t *
copy_autos(ici_func_t *f)
{
    ici_struct_t    *ns;
    ici_struct_t    *s;
    int         nslots;

    if (f->f_nautos == 0)
        return structof(copy_struct(f->f_autos));
    s = f->f_autos;
    nslots = f->f_nautos + (f->f_nautos >> 1);
    ns = (ici_struct_t *)ici_nalloc(sizeof(ici_struct_t) + nslots * sizeof(ici_sslot_t));
    if (ns == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(ns, TC_STRUCT, O_SUPER, 1, 0);
    ns->o_head.o_super = s->o_head.o_super;
    ns->s_nels = 0;
    ns->s_nslots = 0;
    ns->s_slots = (ici_sslot_t*)(ns + 1);
    memcpy((char *)ns->s_slots, (char *)s->s_slots, s->s_nslots*sizeof(ici_sslot_t));
    ns->s_nels = s->s_nels;
    ns->s_nslots = nslots;
    ici_rego(ns);
    if (ns->s_nslots <= 16)
        ici_invalidate_struct_lookaside(ns);
    else
        ++ici_vsver;
    return objof(ns);

fail:
    ici_decref(ns);
    return NULL;
}
#endif

/*
 * Grow the struct s so that it has twice as many slots.
 */
static int
grow_struct(ici_struct_t *s)
{
    register ici_sslot_t *sl;
    register ici_sslot_t *oldslots;
    register int        i;

    i = (s->s_nslots * 2) * sizeof(ici_sslot_t);
    if ((sl = (ici_sslot_t*)ici_nalloc(i)) == NULL)
        return 1;
    memset((char *)sl, 0, i);
    oldslots = s->s_slots;
    s->s_slots = sl;
    i = s->s_nslots;
    s->s_nslots *= 2;
    while (--i >= 0)
    {
        if (oldslots[i].sl_key != NULL)
            *find_raw_slot(s, oldslots[i].sl_key) = oldslots[i];
    }
    ici_nfree((char *)oldslots, (s->s_nslots / 2) * sizeof(ici_sslot_t));
    ++ici_vsver;
    return 0;
}

/*
 * Remove the key 'k' from the ICI struct object 's', ignoring super-structs.
 *
 * This --func-- forms part of the --ici-api--.
 */
void
ici_struct_unassign(ici_struct_t *s, ici_obj_t *k)
{
    register ici_sslot_t *sl;
    register ici_sslot_t *ss;
    register ici_sslot_t *ws;    /* Wanted position. */

    if ((ss = find_raw_slot(s, k))->sl_key == NULL)
        return;
    --s->s_nels;
    sl = ss;
    /*
     * Scan "forward" bubbling up entries which would rather be at our
     * current empty slot.
     */
    for (;;)
    {
        if (--sl < s->s_slots)
            sl = s->s_slots + s->s_nslots - 1;
        if (sl->sl_key == NULL)
            break;
        ws = &s->s_slots[HASHINDEX(sl->sl_key, s)];
        if
        (
            (sl < ss && (ws >= ss || ws < sl))
            ||
            (sl > ss && (ws >= ss && ws < sl))
        )
        {
            /*
             * The value at sl, which really wants to be at ws, should go
             * into the current empty slot (ss).  Copy it to there and update
             * ss to be here (which now becomes empty).
             */
            *ss = *sl;
            ss = sl;
            /*
             * If we've moved a slot keyed by a string, that string's
             * look-aside value may be wrong. Trash it.
             */
            if (isstring(ss->sl_key))
                stringof(ss->sl_key)->s_vsver = 0;
        }
    }
    if (isstring(k))
        stringof(k)->s_vsver = 0;
    ss->sl_key = NULL;
    ss->sl_value = NULL;
}

/*
 * Do a fetch where we are the super of some other object that is
 * trying to satisfy a fetch. Don't regard the item k as being present
 * unless it really is. Return -1 on error, 0 if it was not found,
 * and 1 if was found. If found, the value is stored in *v.
 *
 * If not NULL, b is a struct that was the base element of this
 * fetch. This is used to mantain the lookup lookaside mechanism.
 */
static int
fetch_super_struct(ici_obj_t *o, ici_obj_t *k, ici_obj_t **v, ici_struct_t *b)
{
    ici_sslot_t         *sl;

    do
    {
        sl = &structof(o)->s_slots[HASHINDEX(k, structof(o))];
        while (sl->sl_key != NULL)
        {
            if (sl->sl_key == k)
            {
                if (b != NULL && isstring(k))
                {
                    stringof(k)->s_vsver = ici_vsver;
                    stringof(k)->s_struct = b;
                    stringof(k)->s_slot = sl;
                    if (o->o_flags & O_ATOM)
                        k->o_flags |= S_LOOKASIDE_IS_ATOM;
                    else
                        k->o_flags &= ~S_LOOKASIDE_IS_ATOM;
                }
                *v = sl->sl_value;
                return 1;
            }
            if (--sl < structof(o)->s_slots)
                sl = structof(o)->s_slots + structof(o)->s_nslots - 1;
        }
        if ((o = objof(structof(o)->o_head.o_super)) == NULL)
            return 0;

    } while (isstruct(o)); /* Merge tail recursion on structs. */

    return ici_fetch_super(o, k, v, b);
}

/*
 * Return the object at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 */
static ici_obj_t *
fetch_struct(ici_obj_t *o, ici_obj_t *k)
{
    ici_obj_t           *v;

    if
    (
        isstring(k)
        &&
        stringof(k)->s_struct == structof(o)
        &&
        stringof(k)->s_vsver == ici_vsver
    )
    {
        assert(fetch_super_struct(o, k, &v, NULL) == 1);
        assert(stringof(k)->s_slot->sl_value == v);
        return stringof(k)->s_slot->sl_value;
    }
    switch (fetch_super_struct(o, k, &v, structof(o)))
    {
    case -1: return NULL;               /* Error. */
    case  1: return v;                  /* Found. */
    }
    return objof(&o_null);              /* Not found. */
}

static ici_obj_t *
fetch_base_struct(ici_obj_t *o, ici_obj_t *k)
{
    ici_sslot_t         *sl;

    sl = find_raw_slot(structof(o), k);
    if (sl->sl_key == NULL)
        return objof(&o_null);
    if (isstring(k))
    {
        stringof(k)->s_vsver = ici_vsver;
        stringof(k)->s_struct = structof(o);
        stringof(k)->s_slot = sl;
        if (o->o_flags & O_ATOM)
            k->o_flags |= S_LOOKASIDE_IS_ATOM;
        else
            k->o_flags &= ~S_LOOKASIDE_IS_ATOM;
    }
    return sl->sl_value;
}

/*
 * Do an assignment where we are the super of some other object that
 * is trying to satisfy an assign. Don't regard the item k as being
 * present unless it really is. Return -1 on error, 0 if not found
 * and 1 if the assignment was completed.
 *
 * If 0 is returned, no struct may have been modified during the
 * operation of this function.
 *
 * If not NULL, b is a struct that was the base element of this
 * assignment. This is used to mantain the lookup lookaside mechanism.
 */
static int
assign_super_struct(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v, ici_struct_t *b)
{
    ici_sslot_t         *sl;

    do
    {
        if ((o->o_flags & O_ATOM) == 0)
        {
            sl = &structof(o)->s_slots[HASHINDEX(k, structof(o))];
            while (sl->sl_key != NULL)
            {
                if (sl->sl_key == k)
                {
                    sl->sl_value = v;
                    if (b != NULL && isstring(k))
                    {
                        stringof(k)->s_vsver = ici_vsver;
                        stringof(k)->s_struct = b;
                        stringof(k)->s_slot = sl;
                        k->o_flags &= ~S_LOOKASIDE_IS_ATOM;
                    }
                    return 1;
                }
                if (--sl < structof(o)->s_slots)
                    sl = structof(o)->s_slots + structof(o)->s_nslots - 1;
            }
        }
        if ((o = objof(structof(o)->o_head.o_super)) == NULL)
            return 0;

    } while (isstruct(o)); /* Merge tail recursion. */

    return ici_assign_super(o, k, v, b);
}

/*
 * Set the value of key k in the struct s to the value v.  Will add the
 * entry if necessary and grow the struct if necessary.  Returns 1 on
 * failure, else 0.
 * See the comment on t_assign() in object.h.
 */
static int
assign_struct(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    ici_sslot_t         *sl;

    if
    (
        isstring(k)
        &&
        stringof(k)->s_struct == structof(o)
        &&
        stringof(k)->s_vsver == ici_vsver
        &&
        (k->o_flags & S_LOOKASIDE_IS_ATOM) == 0
    )
    {
#ifndef NDEBUG
        ici_obj_t       *av;
#endif

        assert(fetch_super_struct(o, k, &av, NULL) == 1);
        assert(stringof(k)->s_slot->sl_value == av);
        stringof(k)->s_slot->sl_value = v;
        return 0;
    }
    /*
     * Look for it in the base struct.
     */
    sl = &structof(o)->s_slots[HASHINDEX(k, structof(o))];
    while (sl->sl_key != NULL)
    {
        if (sl->sl_key == k)
        {
            if (o->o_flags & O_ATOM)
            {
                ici_error = "attempt to modify an atomic struct";
                return 1;
            }
            goto do_assign;
        }
        if (--sl < structof(o)->s_slots)
            sl = structof(o)->s_slots + structof(o)->s_nslots - 1;
    }
    if (structof(o)->o_head.o_super != NULL)
    {
        switch (ici_assign_super(structof(o)->o_head.o_super, k, v, structof(o)))
        {
        case -1: return 1; /* Error. */
        case 1:  return 0; /* Done. */
        }
    }
    /*
     * Not found. Assign into base struct. We still have sl from above.
     */
    if (o->o_flags & O_ATOM)
    {
        ici_error = "attempt to modify an atomic struct";
        return 1;
    }
    if (structof(o)->s_nels >= structof(o)->s_nslots - structof(o)->s_nslots / 4)
    {
        /*
         * This struct is 75% full.  Grow it.
         */
        if (grow_struct(structof(o)))
            return 1;
        /*
         * Re-find our empty slot.
         */
        sl = &structof(o)->s_slots[HASHINDEX(k, structof(o))];
        while (sl->sl_key != NULL)
        {
            if (--sl < structof(o)->s_slots)
                sl = structof(o)->s_slots + structof(o)->s_nslots - 1;
        }
    }
    ++structof(o)->s_nels;
    sl->sl_key = k;
do_assign:
    sl->sl_value = v;
    if (isstring(k))
    {
        stringof(k)->s_vsver = ici_vsver;
        stringof(k)->s_struct = structof(o);
        stringof(k)->s_slot = sl;
        k->o_flags &= ~S_LOOKASIDE_IS_ATOM;
    }
    return 0;
}

/*
 * Assign a value into a key of a struct, but ignore the super chain.
 * That is, always assign into the lowest level. Usual error coventions.
 */
static int
assign_base_struct(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    ici_sslot_t         *sl;

    if (o->o_flags & O_ATOM)
    {
        ici_error = "attempt to modify an atomic struct";
        return 1;
    }
    sl = find_raw_slot(structof(o), k);
    if (sl->sl_key != NULL)
        goto do_assign;
    /*
     * Not found. Assign into base struct. We still have sl from above.
     */
    if (structof(o)->s_nels >= structof(o)->s_nslots - structof(o)->s_nslots / 4)
    {
        /*
         * This struct is 75% full.  Grow it.
         */
        if (grow_struct(structof(o)))
            return 1;
        /*
         * Re-find out empty slot.
         */
        sl = &structof(o)->s_slots[HASHINDEX(k, structof(o))];
        while (sl->sl_key != NULL)
        {
            if (--sl < structof(o)->s_slots)
                sl = structof(o)->s_slots + structof(o)->s_nslots - 1;
        }
    }
    ++structof(o)->s_nels;
    sl->sl_key = k;
do_assign:
    sl->sl_value = v;
    if (isstring(k))
    {
        stringof(k)->s_vsver = ici_vsver;
        stringof(k)->s_struct = structof(o);
        stringof(k)->s_slot = sl;
        k->o_flags &= ~S_LOOKASIDE_IS_ATOM;
    }
    return 0;
}

ici_type_t  struct_type =
{
    mark_struct,
    free_struct,
    hash_struct,
    cmp_struct,
    copy_struct,
    assign_struct,
    fetch_struct,
    "struct",
    NULL,
    NULL,
    NULL,
    assign_super_struct,
    fetch_super_struct,
    assign_base_struct,
    fetch_base_struct
};

ici_op_t    o_namelvalue    = {OBJ(TC_OP), NULL, OP_NAMELVALUE};
ici_op_t    o_colon         = {OBJ(TC_OP), NULL, OP_COLON};
ici_op_t    o_coloncaret    = {OBJ(TC_OP), NULL, OP_COLONCARET};
ici_op_t    o_dot           = {OBJ(TC_OP), NULL, OP_DOT};
ici_op_t    o_dotkeep       = {OBJ(TC_OP), NULL, OP_DOTKEEP};
ici_op_t    o_dotrkeep      = {OBJ(TC_OP), NULL, OP_DOTRKEEP};
