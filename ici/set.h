#ifndef ICI_SETS_H
#define ICI_SETS_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif
/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_set
{
    ici_obj_t   o_head;
    int         s_nels;         /* How many slots used. */
    int         s_nslots;       /* How many slots allocated. */
    ici_obj_t   **s_slots;
};
#define setof(o)        ((ici_set_t *)(o))
#define isset(o)        ((o)->o_tcode == TC_SET)
/*
 * End of ici.h export. --ici.h-end--
 */


#endif /* ICI_SETS_H */
