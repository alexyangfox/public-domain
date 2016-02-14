#ifndef ICI_STRUCT_H
#define ICI_STRUCT_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif
/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

struct ici_sslot
{
    ici_obj_t   *sl_key;
    ici_obj_t   *sl_value;
};

struct ici_struct
{
    ici_objwsup_t   o_head;
    int         s_nels;         /* How many slots used. */
    int         s_nslots;       /* How many slots allocated. */
    ici_sslot_t *s_slots;
};
#define structof(o)     ((ici_struct_t *)(o))
#define isstruct(o)     (objof(o)->o_tcode == TC_STRUCT)

/*
 * End of ici.h export. --ici.h-end--
 */

#endif /* ICI_STRUCT_H */
