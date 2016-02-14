#define NOCLASSPROTO

#define ICI_CORE
#include "exec.h"
#include "func.h"
#include "str.h"
#include "int.h"
#include "float.h"
#include "struct.h"
#include "buf.h"
#include "re.h"
#include "null.h"
#include "op.h"
#include "array.h"
#include "method.h"

/*
 * Return 0 if o (the subject object argument supplied to C implemented
 * methods) is present (indicating a method call was made) and is an
 * object with a super and, (if tcode != TC_NONE) has the given type
 * code. Else return 1 and set error appropriately.
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_method_check(ici_obj_t *o, int tcode)
{
    char        n1[30];
    char        n2[30];

    if (o == NULL)
    {
        sprintf(buf, "attempt to call method %s as a function",
            ici_objname(n1, ici_os.a_top[-1]));
        ici_error = buf;
        return 1;
    }
    if (tcode != 0 && o->o_tcode != tcode)
    {
        sprintf(buf, "attempt to apply method %s to %s",
            ici_objname(n1, ici_os.a_top[-1]),
            ici_objname(n2, o));
        ici_error = buf;
        return 1;
    }
    return 0;
}

/*
 * Implemantation of the ICI new method.
 */
static int
m_new(ici_obj_t *o)
{
    ici_struct_t    *s;

    if (ici_method_check(o, 0))
        return 1;
    if ((s = ici_struct_new()) == NULL)
        return 1;
    s->o_head.o_super = objwsupof(o);
    return ici_ret_with_decref(objof(s));
}

static int
m_isa(ici_obj_t *o)
{
    ici_objwsup_t   *s;
    ici_obj_t   *class;

    if (ici_method_check(o, 0))
        return 1;
    if (ici_typecheck("o", &class))
        return 1;
    for (s = objwsupof(o); s != NULL; s = s->o_super)
    {
        if (objof(s) == class)
            return ici_ret_no_decref(objof(ici_one));
    }
    return ici_ret_no_decref(objof(ici_zero));
}

static int
m_respondsto(ici_obj_t *o)
{
    ici_obj_t   *classname;
    ici_obj_t   *v;

    if (ici_method_check(o, 0))
        return 1;
    if (ici_typecheck("o", &classname))
        return 1;
    if ((v = ici_fetch(o, classname)) == NULL)
        return 1;
    if (ismethod(v) || isfunc(v))
        return ici_ret_no_decref(objof(ici_one));
    return ici_ret_no_decref(objof(ici_zero));
}

ici_cfunc_t ici_oo_funcs[] =
{
    {CF_OBJ,    (char *)SS(new),          m_new},
    {CF_OBJ,    (char *)SS(isa),          m_isa},
    {CF_OBJ,    (char *)SS(respondsto),   m_respondsto},
    {CF_OBJ}
};
