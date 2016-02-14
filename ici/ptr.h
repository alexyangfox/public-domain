#ifndef ICI_PTR_H
#define ICI_PTR_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_ptr
{
    ici_obj_t   o_head;
    ici_obj_t   *p_aggr;        /* The aggregate which contains the object. */
    ici_obj_t   *p_key;         /* The key which references it. */
};
#define ptrof(o)        ((ici_ptr_t *)o)
#define isptr(o)        ((o)->o_tcode == TC_PTR)
/*
 * End of ici.h export. --ici.h-end--
 */

#endif  /* ICI_PTR_H */
