#ifndef ICI_PARSE_H
#define ICI_PARSE_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

typedef struct
{
    int         t_what;         /* See TM_* and T_* below. */
    union
    {
        long    tu_int;
        double  tu_float;
        ici_obj_t *tu_obj;
    }
        tu;
}
    token_t;

#define t_int   tu.tu_int
#define t_float tu.tu_float
#define t_obj   tu.tu_obj

struct ici_parse
{
    ici_obj_t   o_head;
    ici_file_t  *p_file;
    int         p_lineno;       /* Diagnostic information. */
    short       p_sol;          /* At first char in line. */
    short       p_cr;           /* New-line caused by \r, not \n. */
    token_t     p_got;
    token_t     p_ungot;
    ici_func_t  *p_func;        /* NULL when not within scope. */
    int         p_module_depth; /* Depth within module, 0 is file level. */
};
/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
#define isparse(o)      (objof(o)->o_tcode == TC_PARSE)
#define parseof(o)      ((ici_parse_t *)(o))
/*
 * End of ici.h export. --ici.h-end--
 */

/*
 * Token numbers.  Note that the precedence and order of binary operators
 * is built into the token number; this drives the expression parser.
 */
#define TM_SUBTYPE      0x003F          /* 6 bits. */
#define TM_TYPE         0x07C0          /* 5 bits. */
#define TM_PREC         0x7800          /* 4 bits, 0 is high (tight bind).*/
#define TM_HASOBJ       0x8000          /* Implies incref on t_obj. */
#define t_subtype(t)    ((t) & TM_SUBTYPE)
#define t_type(t)       ((t) & TM_TYPE)
#define t_prec(t)       (((t) & TM_PREC) >> 11)
#define TYPE(n)         ((n) << 6)
#define PREC(n)         ((n) << 11)

#define T_NONE          TYPE(0)
#define T_NAME          (TM_HASOBJ|TYPE(1))
#define T_REGEXP        (TM_HASOBJ|TYPE(2))
#define T_STRING        (TM_HASOBJ|TYPE(3))
#define T_SEMICOLON     TYPE(4)
#define T_EOF           TYPE(5)
#define T_INT           TYPE(6)
#define T_FLOAT         TYPE(7)
#define T_BINOP         TYPE(8)     /* Has sub-types, see below. */
#define T_ERROR         TYPE(9)
#define T_NULL          TYPE(10)
#define T_ONROUND       TYPE(11)
#define T_OFFROUND      TYPE(12)
#define T_ONCURLY       TYPE(13)
#define T_OFFCURLY      TYPE(14)
#define T_ONSQUARE      TYPE(15)
#define T_OFFSQUARE     TYPE(16)
#define T_DOT           TYPE(17)
#define T_PTR           TYPE(18)
#define T_EXCLAM        TYPE(19)
#define T_PLUSPLUS      TYPE(20)
#define T_MINUSMINUS    TYPE(21)
#define T_CONST         TYPE(22)
#define T_PRIMARYCOLON  TYPE(23)
/*#define T_2COLON        TYPE(24)*/
#define T_DOLLAR        TYPE(25)
#define T_COLONCARET    TYPE(26)
/*#define       T_OFFCURLYOFFSQ TYPE(27)*/
#define T_AT            TYPE(28)
#define T_BINAT         TYPE(29)
/* Maximum value        TYPE(31) */

/*
 * T_BINOP sub types.
 */
#define T_ASTERIX       (PREC(0)|T_BINOP|1)
#define T_SLASH         (PREC(0)|T_BINOP|2)
#define T_PERCENT       (PREC(0)|T_BINOP|3)
#define T_PLUS          (PREC(1)|T_BINOP|4)
#define T_MINUS         (PREC(1)|T_BINOP|5)
#define T_GRTGRT        (PREC(2)|T_BINOP|6)
#define T_LESSLESS      (PREC(2)|T_BINOP|7)
#define T_LESS          (PREC(3)|T_BINOP|8)
#define T_GRT           (PREC(3)|T_BINOP|9)
#define T_LESSEQ        (PREC(3)|T_BINOP|10)
#define T_GRTEQ         (PREC(3)|T_BINOP|11)
#define T_EQEQ          (PREC(4)|T_BINOP|12)
#define T_EXCLAMEQ      (PREC(4)|T_BINOP|13)
#define T_TILDE         (PREC(4)|T_BINOP|14)
#define T_EXCLAMTILDE   (PREC(4)|T_BINOP|15)
#define T_2TILDE        (PREC(4)|T_BINOP|16)
#define T_3TILDE        (PREC(4)|T_BINOP|17)
#define T_AND           (PREC(5)|T_BINOP|18)
#define T_CARET         (PREC(6)|T_BINOP|19)
#define T_BAR           (PREC(7)|T_BINOP|20)
#define T_ANDAND        (PREC(8)|T_BINOP|21)
#define T_BARBAR        (PREC(9)|T_BINOP|22)
#define T_COLON         (PREC(10)|T_BINOP|23)
#define T_QUESTION      (PREC(11)|T_BINOP|24)
#define T_EQ            (PREC(12)|T_BINOP|25)
#define T_COLONEQ       (PREC(12)|T_BINOP|26)
#define T_PLUSEQ        (PREC(12)|T_BINOP|27)
#define T_MINUSEQ       (PREC(12)|T_BINOP|28)
#define T_ASTERIXEQ     (PREC(12)|T_BINOP|29)
#define T_SLASHEQ       (PREC(12)|T_BINOP|30)
#define T_PERCENTEQ     (PREC(12)|T_BINOP|31)
#define T_GRTGRTEQ      (PREC(12)|T_BINOP|32)
#define T_LESSLESSEQ    (PREC(12)|T_BINOP|33)
#define T_ANDEQ         (PREC(12)|T_BINOP|34)
#define T_CARETEQ       (PREC(12)|T_BINOP|35)
#define T_BAREQ         (PREC(12)|T_BINOP|36)
#define T_2TILDEEQ      (PREC(12)|T_BINOP|37)
#define T_LESSEQGRT     (PREC(12)|T_BINOP|38)
#define T_COMMA         (PREC(13)|T_BINOP|39)
#define BINOP_MAX       39
/* Maximum values       (PREC(15)|T_BINOP|63) */

/*
 * Reasons for doing things (compiling expressions generally).
 */
#define FOR_VALUE       0
#define FOR_LVALUE      1
#define FOR_EFFECT      2
#define FOR_TEMP        3

/*
 * Flags modifying the behaviour of the colon operator. These are stored
 * in the op_code field of a OP_COLON operators by the compiler and noticed
 * by the execution loop.
 */
#define OPC_COLON_CARET     0x0001  /* It's a :^ not a : */
#define OPC_COLON_CALL      0x0002  /* Don't form a method, just call it. */

/*
 * Expression tree.  This is what the parseing functions build and
 * pass to compile_expr().
 */
struct expr
{
    int         e_what;         /* A token identifier, ie. T_*. */
    expr_t      *e_arg[2];
    ici_obj_t   *e_obj;
};

#endif  /* ICI_PARSE_H */
