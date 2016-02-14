#define ICI_CORE
#include "exec.h"
#include "src.h"
#include "str.h"

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_src(ici_obj_t *o)
{
    long        mem;

    o->o_flags |= O_MARK;
    mem = sizeof(ici_src_t);
    if (srcof(o)->s_filename != NULL)
        mem += ici_mark(srcof(o)->s_filename);
    return mem;
}

ici_src_t *
new_src(int lineno, ici_str_t *filename)
{
    register ici_src_t  *s;

    if ((s = ici_talloc(ici_src_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(s, TC_SRC, 0, 1, 0);
    s->s_lineno = lineno;
    s->s_filename = filename;
    ici_rego(s);
    return s;
}

#if 0
static ici_obj_t *
fetch_src(ici_obj_t *o, ici_obj_t *k)
{
    if (k == SSO(file))
        return objof(srcof(o)->s_filename);
    if (k == SSO(line))
    {
        ici_int_t   *io;

        if ((io = ici_int_new(srcof(o)->s_lineno)) == NULL)
            return NULL;
        ici_decref(io);
        return objof(io);
    }
    return ici_fetch_fail(o, k);
}
#endif

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
static void
free_src(ici_obj_t *o)
{
    ici_tfree(o, ici_src_t);
}

ici_type_t  src_type =
{
    mark_src,
    free_src,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "src"
};
