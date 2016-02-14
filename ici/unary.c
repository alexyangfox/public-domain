#define ICI_CORE
#include "exec.h"
#include "float.h"
#include "int.h"
#include "op.h"
#include "parse.h"
#include "buf.h"
#include "null.h"

int
ici_op_unary(void)
{
    ici_int_t   *i;

    switch (opof(ici_xs.a_top[-1])->op_code)
    {
    case t_subtype(T_EXCLAM):
        if (isfalse(ici_os.a_top[-1]))
            ici_os.a_top[-1] = objof(ici_one);
        else
            ici_os.a_top[-1] = objof(ici_zero);
        --ici_xs.a_top;
        return 0;

    case t_subtype(T_TILDE):
        if (!isint(ici_os.a_top[-1]))
            goto fail;
        if ((i = ici_int_new(~intof(ici_os.a_top[-1])->i_value)) == NULL)
            return 1;
        ici_os.a_top[-1] = objof(i);
        ici_decref(i);
        --ici_xs.a_top;
        return 0;

    case t_subtype(T_MINUS):
        /*
         * Unary minus is implemented as a binary op because they are
         * more heavily optimised.
         */
        assert(0);
    fail:
    default:
        switch (opof(ici_xs.a_top[-1])->op_code)
        {
        case t_subtype(T_EXCLAM): ici_error = "!"; break;
        case t_subtype(T_TILDE): ici_error = "~"; break;
        case t_subtype(T_MINUS): ici_error = "-"; break;
        default: ici_error = "<unknown unary operator>"; break;
        }
        sprintf(buf, "attempt to perform \"%s %s\"",
            ici_error, ici_typeof(ici_os.a_top[-1])->t_name);
        ici_error = buf;
        return 1;
    }
}
