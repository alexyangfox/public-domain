#define ICI_CORE
#include "mem.h"
#include "int.h"
#include "buf.h"
#include "primes.h"

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_mem(ici_obj_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof(ici_mem_t);
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
static int
cmp_mem(ici_obj_t *o1, ici_obj_t *o2)
{
    return memof(o1)->m_base != memof(o2)->m_base
        || memof(o1)->m_length != memof(o2)->m_length
        || memof(o1)->m_accessz != memof(o2)->m_accessz
        || memof(o1)->m_free != memof(o2)->m_free;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
static unsigned long
hash_mem(ici_obj_t *o)
{
    return (unsigned long)memof(o)->m_base * MEM_PRIME_0
        + (unsigned long)memof(o)->m_length * MEM_PRIME_1
        + (unsigned long)memof(o)->m_accessz * MEM_PRIME_2;
}

/*
 * Assign to key k of the object o the value v. Return 1 on error, else 0.
 * See the comment on t_assign() in object.h.
 */
static int
assign_mem(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    register long       i;

    if (!isint(k) || !isint(v))
        return ici_assign_fail(o, k, v);
    i = intof(k)->i_value;
    if (i < 0 || i >= (long)memof(o)->m_length)
    {
        sprintf(buf, "attempt to write at mem index %ld\n", i);
        ici_error = buf;
        return 1;
    }
    switch (memof(o)->m_accessz)
    {
    case 1:
        ((unsigned char *)memof(o)->m_base)[i] = (unsigned char)intof(v)->i_value;
        break;

    case 2:
        ((unsigned short *)memof(o)->m_base)[i] = (unsigned short)intof(v)->i_value;
        break;

    case 4:
        ((long *)memof(o)->m_base)[i] = intof(v)->i_value;
        break;
    }
    return 0;
}

/*
 * Return the object at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 */
static ici_obj_t *
fetch_mem(ici_obj_t *o, ici_obj_t *k)
{
    long        i;

    if (!isint(k))
        return ici_fetch_fail(o, k);
    i = intof(k)->i_value;
    if (i < 0 || i >= (long)memof(o)->m_length)
        return objof(&o_null);
    switch (memof(o)->m_accessz)
    {
    case 1:
        i = ((unsigned char *)memof(o)->m_base)[i];
        break;

    case 2:
        i = ((unsigned short *)memof(o)->m_base)[i];
        break;

    case 4:
        i = ((long *)memof(o)->m_base)[i];
        break;
    }
    o = objof(ici_int_new(i));
    ici_decref(o);
    return o;
}

/*
 * Return a new ICI mem object refering to the memory at address 'base'
 * with length 'length', which is measured in units of 'accessz' bytes.
 * 'accessz' must be either 1, 2 or 4. If 'free_func' is provided it
 * will be called when the mem object is about to be freed with 'base'
 * as an argument.
 *
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_mem_t *
ici_mem_new(void *base, size_t length, int accessz, void (*free_func)())
{
    register ici_mem_t  *m;

    if ((m = ici_talloc(ici_mem_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(m, TC_MEM, 0, 1, sizeof(ici_mem_t));
    ici_rego(m);
    m->m_base = base;
    m->m_length = length;
    m->m_accessz = accessz;
    m->m_free = free_func;
    return memof(ici_atom(objof(m), 1));
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
static void
free_mem(ici_obj_t *o)
{
    if (memof(o)->m_free != NULL)
        (*memof(o)->m_free)(memof(o)->m_base);
    ici_tfree(o, ici_mem_t);
}

ici_type_t  mem_type =
{
    mark_mem,
    free_mem,
    hash_mem,
    cmp_mem,
    ici_copy_simple,
    assign_mem,
    fetch_mem,
    "mem"
};
