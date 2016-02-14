#define ICI_CORE
#include "exec.h"
#include "op.h"
#include "int.h"
#include "buf.h"
#include "pc.h"
#include "struct.h"
#include "null.h"
#include "forall.h"
#include "catch.h"

/*
 * self => array looper &array[2] (xs)
 * array => - (os)
 *
 * Ie, like a loop, but on the first time in it skips the first two elements
 * of the the loop, which are expected to be an array of code for the step
 * and an exec operator to run it.
 */
int
ici_op_for()
{
    get_pc(arrayof(ici_os.a_top[-1]), ici_xs.a_top + 1);
    pcof(ici_xs.a_top[1])->pc_next += opof(ici_xs.a_top[-1])->op_code;
    ici_xs.a_top[-1] = ici_os.a_top[-1];
    *ici_xs.a_top++ = objof(&o_looper);
    ++ici_xs.a_top; /* pc */
    --ici_os.a_top;
    return 0;
}

ici_op_t    o_exec          = {OBJ(TC_OP), NULL, OP_EXEC};
ici_op_t    o_looper        = {OBJ(TC_OP), NULL, OP_LOOPER};
ici_op_t    o_loop          = {OBJ(TC_OP), NULL, OP_LOOP};
ici_op_t    o_break         = {OBJ(TC_OP), NULL, OP_BREAK};
ici_op_t    o_continue      = {OBJ(TC_OP), NULL, OP_CONTINUE};
ici_op_t    o_if            = {OBJ(TC_OP), NULL, OP_IF};
ici_op_t    o_ifnotbreak    = {OBJ(TC_OP), NULL, OP_IFNOTBREAK};
ici_op_t    o_ifbreak       = {OBJ(TC_OP), NULL, OP_IFBREAK};
ici_op_t    o_ifelse        = {OBJ(TC_OP), NULL, OP_IFELSE};
ici_op_t    o_pop           = {OBJ(TC_OP), NULL, OP_POP};
ici_op_t    o_andand        = {OBJ(TC_OP), NULL, OP_ANDAND, 1};
ici_op_t    o_barbar        = {OBJ(TC_OP), NULL, OP_ANDAND, 0};
ici_op_t    o_switch        = {OBJ(TC_OP), NULL, OP_SWITCH};
ici_op_t    o_switcher      = {OBJ(TC_OP), NULL, OP_SWITCHER};
ici_op_t    o_critsect      = {OBJ(TC_OP), NULL, OP_CRITSECT};
ici_op_t    o_waitfor       = {OBJ(TC_OP), NULL, OP_WAITFOR};
ici_op_t    o_rewind        = {OBJ(TC_OP), NULL, OP_REWIND};
ici_op_t    o_end           = {OBJ(TC_OP), NULL, OP_ENDCODE};

