#define ICI_CORE
#include "exec.h"
#include "pc.h"

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_pc(ici_obj_t *o)
{
    o->o_flags |= O_MARK;
    if (pcof(o)->pc_code != NULL)
        return sizeof(ici_pc_t) + ici_mark(pcof(o)->pc_code);
    return sizeof(ici_pc_t);
}

/*
 * Return a new pc object.
 *
 * NOTE: pc's come back ready-decref'ed.
 */
ici_pc_t *
new_pc(void)
{
    ici_pc_t            *pc;

    if ((pc = pcof(ici_talloc(ici_pc_t))) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(pc, TC_PC, 0, 0, 0);
    pc->pc_code = NULL;
    ici_rego(pc);
    return pc;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
static void
free_pc(ici_obj_t *o)
{
    ici_tfree(o, ici_pc_t);
}

ici_type_t  pc_type =
{
    mark_pc,
    free_pc,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "pc"
};
