#ifndef ICI_RE_H
#define ICI_RE_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

#include "pcre/pcre.h"

struct ici_regexp
{
    ici_obj_t   o_head;
    pcre        *r_re;
    pcre_extra  *r_rex;
    ici_str_t   *r_pat;
};
/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
#define regexpof(o)     ((ici_regexp_t *)(o))
#define isregexp(o)     ((o)->o_tcode == TC_REGEXP)

extern int
ici_pcre(ici_regexp_t *r,
    const char *subject, int length, int start_offset,
    int options, int *offsets, int offsetcount);


/*
 * End of ici.h export. --ici.h-end--
 */

#endif
