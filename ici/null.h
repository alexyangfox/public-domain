#ifndef ICI_NULL_H
#define ICI_NULL_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_null_t
{
    ici_obj_t   o_head;
};
#define nullof(o)       ((ici_null_t *)o)
#define isnull(o)       ((o) == objof(&o_null))

/*
 * End of ici.h export. --ici.h-end--
 */

#endif /* ICI_NULL_H */
