#ifndef ICI_INT_H
#define ICI_INT_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
/*
 * The C struct that is the ICI int object.
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct ici_int
{
    ici_obj_t   o_head;
    long        i_value;
};
#define intof(o)        ((ici_int_t *)(o))
#define isint(o)        ((o)->o_tcode == TC_INT)
/*
 * End of ici.h export. --ici.h-end--
 */

#define ICI_SMALL_INT_MASK  0x1F
extern ici_int_t            *ici_small_ints[32];

#endif /* ICI_INT_H */
