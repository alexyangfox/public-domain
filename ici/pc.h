#ifndef ICI_PC_H
#define ICI_PC_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_pc
{
    ici_obj_t   o_head;
    ici_array_t *pc_code;
    ici_obj_t   **pc_next;
};
#define pcof(o)         ((ici_pc_t *)(o))
#define ispc(o)         ((o)->o_tcode == TC_PC)
/*
 * End of ici.h export. --ici.h-end--
 */

#endif  /* ICI_PC_H */
