#ifndef ICI_OP_H
#define ICI_OP_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_op
{
    ici_obj_t   o_head;
    int         (*op_func)();
    int         op_ecode;       /* See OP_* below. */
    int         op_code;
};
#define opof(o) ((ici_op_t *)o)
#define isop(o) ((o)->o_tcode == TC_OP)

/*
 * End of ici.h export. --ici.h-end--
 */

/*
 * Operator codes. These are stored in the op_ecode field and
 * allow direct switching to the appropriate code in the main
 * execution loop. If op_ecode is OP_OTHER, then the op_func field
 * is significant instead.
 */
enum
{
    OP_OTHER,
    OP_CALL,
    OP_NAMELVALUE,
    OP_DOT,
    OP_DOTKEEP,
    OP_DOTRKEEP,
    OP_ASSIGN,
    OP_ASSIGN_TO_NAME,
    OP_ASSIGNLOCAL,
    OP_EXEC,
    OP_LOOP,
    OP_REWIND,
    OP_ENDCODE,
    OP_IF,
    OP_IFELSE,
    OP_IFNOTBREAK,
    OP_IFBREAK,
    OP_BREAK,
    OP_QUOTE,
    OP_BINOP,
    OP_AT,
    OP_SWAP,
    OP_BINOP_FOR_TEMP,
    OP_AGGR_KEY_CALL,
    OP_COLON,
    OP_COLONCARET,
    OP_METHOD_CALL,
    OP_SUPER_CALL,
    OP_ASSIGNLOCALVAR,
    OP_CRITSECT,
    OP_WAITFOR,
    OP_POP,
    OP_CONTINUE,
    OP_LOOPER,
    OP_ANDAND,
    OP_SWITCH,
    OP_SWITCHER,
};

/*
 * Extern definitions for various statically defined op objects. They
 * Are defined in various source files. Generally where they are
 * implemented.
 */
extern ici_op_t         o_quote;
extern ici_op_t         o_looper;
extern ici_op_t         o_loop;
extern ici_op_t         o_rewind;
extern ici_op_t         o_end;
extern ici_op_t         o_break;
extern ici_op_t         o_continue;
extern ici_op_t         o_offsq;
extern ici_op_t         o_exec;
extern ici_op_t         o_mkfunc;
extern ici_op_t         o_return;
extern ici_op_t         o_call;
extern ici_op_t         o_method_call;
extern ici_op_t         o_super_call;
extern ici_op_t         o_if;
extern ici_op_t         o_ifnot;
extern ici_op_t         o_ifnotbreak;
extern ici_op_t         o_ifbreak;
extern ici_op_t         o_ifelse;
extern ici_op_t         o_pop;
extern ici_op_t         o_colon;
extern ici_op_t         o_coloncaret;
extern ici_op_t         o_dot;
extern ici_op_t         o_dotkeep;
extern ici_op_t         o_dotrkeep;
extern ici_op_t         o_mkptr;
extern ici_op_t         o_openptr;
extern ici_op_t         o_fetch;
extern ici_op_t         o_for;
extern ici_op_t         o_mklvalue;
extern ici_op_t         o_rematch;
extern ici_op_t         o_renotmatch;
extern ici_op_t         o_reextract;
extern ici_op_t         o_onerror;
extern ici_op_t         o_andand;
extern ici_op_t         o_barbar;
extern ici_op_t         o_namelvalue;
extern ici_op_t         o_switch;
extern ici_op_t         o_switcher;
extern ici_op_t         o_critsect;
extern ici_op_t         o_waitfor;

#endif /* ICI_OP_H */
