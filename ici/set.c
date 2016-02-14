#define ICI_CORE
#include "object.h"
#include "set.h"
#include "op.h"
#include "int.h"
#include "buf.h"
#include "null.h"
#include "primes.h"

#define SET_HASHINDEX(k, s) (ICI_PTR_HASH(k) & ((s)->s_nslots - 1))

/*
 * Find the set slot which does, or should, contain the key k.
 */
ici_obj_t **
find_set_slot(ici_set_t *s, ici_obj_t *k)
{
    register ici_obj_t  **e;

    e = &s->s_slots[SET_HASHINDEX(k, s)];
    while (*e != NULL)
    {
        if (*e == k)
            return e;
        if (--e < s->s_slots)
            e = s->s_slots + s->s_nslots - 1;
    }
    return e;
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_set(ici_obj_t *o)
{
    register ici_obj_t  **e;
    long                mem;

    o->o_flags |= O_MARK;
    mem = sizeof(ici_set_t) + setof(o)->s_nslots * sizeof(ici_obj_t *);
    if (setof(o)->s_nels == 0)
        return mem;
    for
    (
        e = &setof(o)->s_slots[setof(o)->s_nslots - 1];
        e >= setof(o)->s_slots;
        --e
    )
    {
        if (*e != NULL)
            mem += ici_mark(*e);
    }
    return mem;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
static void
free_set(ici_obj_t *o)
{
    if (setof(o)->s_slots != NULL)
        ici_nfree(setof(o)->s_slots, setof(o)->s_nslots * sizeof(ici_obj_t *));
    ici_tfree(o, ici_set_t);
}

/*
 * Return a new ICI set object. The returned set has been increfed.
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_set_t *
ici_set_new()
{
    register ici_set_t  *s;

    /*
     * NB: there is a copy of this sequence in copy_set.
     */
    if ((s = ici_talloc(ici_set_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(s, TC_SET, 0, 1, 0);
    s->s_nels = 0;
    s->s_nslots = 4; /* Must be power of 2. */
    if ((s->s_slots = (ici_obj_t **)ici_nalloc(4 * sizeof(ici_obj_t *))) == NULL)
    {
        ici_tfree(s, ici_set_t);
        return NULL;
    }
    memset(s->s_slots, 0, 4 * sizeof(ici_obj_t *));
    ici_rego(s);
    return s;
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
static int
cmp_set(ici_obj_t *o1, ici_obj_t *o2)
{
    register int        i;
    register ici_obj_t  **e;

    if (o1 == o2)
        return 0;
    if (setof(o1)->s_nels != setof(o2)->s_nels)
        return 1;
    e = setof(o1)->s_slots;
    i = setof(o1)->s_nslots;
    while (--i >= 0)
    {
        if (*e != NULL && *find_set_slot(setof(o2), *e) == NULL)
            return 1;
        ++e;
    }
    return 0;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
static unsigned long
hash_set(ici_obj_t *o)
{
    int                         i;
    unsigned long               h;
    ici_obj_t                   **po;

    h = 0;
    po = setof(o)->s_slots;
    i = setof(o)->s_nels;
    /*
     * This assumes NULL will become zero when cast to unsigned long.
     */
    while (--i >= 0)
        h += (unsigned long)*po++ >> 4;
    return h * SET_PRIME_0 + SET_PRIME_1;
}

/*
 * Return a copy of the given object, or NULL on error.
 * See the comment on t_copy() in object.h.
 */
static ici_obj_t *
copy_set(ici_obj_t *o)
{
    ici_set_t   *s;
    ici_set_t   *ns;

    s = setof(o);
    if ((ns = ici_talloc(ici_set_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(ns, TC_SET, 0, 1, 0);
    ns->s_nels = 0;
    ns->s_nslots = 0;
    ici_rego(ns);
    if ((ns->s_slots = (ici_obj_t **)ici_nalloc(s->s_nslots * sizeof(ici_obj_t *))) == NULL)
        goto fail;
    memcpy(ns->s_slots, s->s_slots, s->s_nslots*sizeof(ici_obj_t *));
    ns->s_nels = s->s_nels;
    ns->s_nslots = s->s_nslots;
    return objof(ns);

fail:
    ici_decref(ns);
    return NULL;
}

/*
 * Grow the set s so that it has twice as many slots.
 */
static int
grow_set(ici_set_t *s)
{
    ici_obj_t           **e;
    ici_obj_t           **oldslots;
    int                 i;
    ptrdiff_t           oldn;

    oldn = s->s_nslots;
    i = (oldn * 2) * sizeof(ici_obj_t *);
    if ((e = (ici_obj_t **)ici_nalloc(i)) == NULL)
        return 1;
    memset((char *)e, 0, i);
    oldslots = s->s_slots;
    s->s_slots = e;
    s->s_nslots = oldn * 2;
    i = oldn;
    while (--i >= 0)
    {
        if (oldslots[i] != NULL)
            *find_set_slot(s, oldslots[i]) = oldslots[i];
    }
    ici_nfree(oldslots, oldn * sizeof(ici_obj_t *));
    return 0;
}

/*
 * Remove the key from the set.
 */
int
ici_set_unassign(ici_set_t *s, ici_obj_t *k)
{
    register ici_obj_t  **sl;
    register ici_obj_t  **ss;
    register ici_obj_t  **ws;   /* Wanted position. */

    if (*(ss = find_set_slot(s, k)) == NULL)
        return 0;
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
        if (*sl == NULL)
            break;
        ws = &s->s_slots[SET_HASHINDEX(*sl, s)];
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
        }
    }
    *ss = NULL;
    return 0;
}

/*
 * Assign to key k of the object o the value v. Return 1 on error, else 0.
 * See the comment on t_assign() in object.h.
 *
 * Add or delete the key k from the set based on the value of v.
 */
static int
assign_set(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    register ici_obj_t  **e;

    if (o->o_flags & O_ATOM)
    {
        ici_error = "attempt to modify an atomic set";
        return 1;
    }
    if (isfalse(v))
    {
        return ici_set_unassign(setof(o), k);
    }
    else
    {
        if (*(e = find_set_slot(setof(o), k)) != NULL)
            return 0;
        if (setof(o)->s_nels >= setof(o)->s_nslots - setof(o)->s_nslots / 4)
        {
            /*
             * This set is 75% full.  Grow it.
             */
            if (grow_set(setof(o)))
                return 1;
            e = find_set_slot(setof(o), k);
        }
        ++setof(o)->s_nels;
        *e = k;
    }

    return 0;
}

/*
 * Return the object at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 */
static ici_obj_t *
fetch_set(ici_obj_t *o, ici_obj_t *k)
{
    return *find_set_slot(setof(o), k) == NULL ? objof(&o_null) : objof(ici_one);
}

ici_type_t  set_type =
{
    mark_set,
    free_set,
    hash_set,
    cmp_set,
    copy_set,
    assign_set,
    fetch_set,
    "set"
};

/*
 * Return 1 if a is a subset of b, else 0.
 */
int
set_issubset(ici_set_t *a, ici_set_t *b) /* a is a subset of b */
{
    register ici_obj_t  **sl;
    register int        i;

    for (sl = a->s_slots, i = 0; i < a->s_nslots; ++i, ++sl)
    {
        if (*sl == NULL)
            continue;
        if (*find_set_slot(b, *sl) == NULL)
            return 0;
    }
    return 1;
}

/*
 * Return 1 if a is a proper subset of b, else 0. That is, is a subset
 * and not equal.
 */
int
set_ispropersubset(ici_set_t *a, ici_set_t *b) /* a is a proper subset of b */
{
    return a->s_nels < b->s_nels && set_issubset(a, b);
}
