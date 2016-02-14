#define ICI_CORE
#include "int.h"
#include "primes.h"

ici_int_t                   *ici_small_ints[32];

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_int(ici_obj_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof(ici_int_t);
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
static int
cmp_int(ici_obj_t *o1, ici_obj_t *o2)
{
    return intof(o1)->i_value != intof(o2)->i_value;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
static unsigned long
hash_int(ici_obj_t *o)
{
    /*
     * There are in-line versions of this in object.c and binop.h.
     */
    return (unsigned long)intof(o)->i_value * INT_PRIME;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
static void
free_int(ici_obj_t *o)
{
    ici_tfree(o, ici_int_t);
}


/*
 * Return the int object with the value 'v'.  The returned object has had its
 * ref count incremented.  Returns NULL on error, usual convention.  Note that
 * ints are intrinsically atomic, so if the given integer already exists, it
 * will just incref it and return it.
 *
 * Note, 0 and 1 are available directly as 'ici_zero' and 'ici_one'.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_int_t *
ici_int_new(long i)
{
    ici_obj_t           *o;
    ici_obj_t           **po;

    if ((i & ~ICI_SMALL_INT_MASK) == 0 && (o = objof(ici_small_ints[i])) != NULL)
    {
        ici_incref(o);
        return intof(o);
    }
    for
    (
        po = &atoms[ici_atom_hash_index((unsigned long)i * INT_PRIME)];
        (o = *po) != NULL;
        --po < atoms ? po = atoms + atomsz - 1 : NULL
    )
    {
        if (isint(o) && intof(o)->i_value == i)
        {
            ici_incref(o);
            return intof(o);
        }
    }
    ++ici_supress_collect;
    if ((o = objof(ici_talloc(ici_int_t))) == NULL)
    {
        --ici_supress_collect;
        return NULL;
    }
    ICI_OBJ_SET_TFNZ(o, TC_INT, O_ATOM, 1, sizeof(ici_int_t));
    ici_rego(o);
    intof(o)->i_value = i;
    --ici_supress_collect;
    ICI_STORE_ATOM_AND_COUNT(po, o);
    return intof(o);
}

ici_type_t  int_type =
{
    mark_int,
    free_int,
    hash_int,
    cmp_int,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "int"
};
