#define ICI_CORE
#include "null.h"

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_null(ici_obj_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof(ici_null_t);
}

ici_type_t  null_type =
{
    mark_null,
    NULL,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "NULL"
};

ici_null_t  o_null  = {{TC_NULL, O_ATOM, 1, 0}};
