#ifndef ICI_FLOAT_H
#define ICI_FLOAT_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * The C struct that is the ICI float object.
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct ici_float
{
    ici_obj_t   o_head;
    double      f_value;
};
#define floatof(o)      ((ici_float_t *)o)
#define isfloat(o)      ((o)->o_tcode == TC_FLOAT)
/*
 * End of ici.h export. --ici.h-end--
 */

/*
 * Are two doubles the same bit pattern? Assumes that doubles are 2 x long
 * size. There are some asserts for this around the place. a and b are
 * pointers to the doubles.
 *
 * We use this when we look up floats in the atom table. We can't just use
 * floating point comparison because the hash is based on the exact bit
 * pattern, and you can get floats that have equal value, but have different
 * bit patterns (0.0 and -0.0 for example). We can end up with different
 * float objects that have apparently equal value. But that's better than
 * having our hash table go bad.
 */
#define DBL_BIT_CMP(a, b)     (((long *)(a))[0] == ((long *)(b))[0] \
                            && ((long *)(a))[1] == ((long *)(b))[1])

#endif /* ICI_FLOAT_H */
