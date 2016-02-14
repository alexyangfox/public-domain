#ifndef TP_HEADER_H  /* to make sure we don't include this more than once */
#define TP_HEADER_H

/*
 *  header.h -- This is the main "include" file for Otter.
 *  All of the .c files include this file.
 *
 */

/************ BASIC INCLUDES ************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*********** INCLUDES FOR TIMES AND DATES ************/

#include <time.h>

#ifdef TP_RUSAGE  /* getrusage() */
#  include <sys/time.h>  /* needed for SunOS */
#  include <sys/resource.h>
#endif

/*********** SIZES OF INTEGERS ***************/

#include <limits.h>  /* for sizes of integers, etc. */

#define MAX_LONG_INT LONG_MAX   /* a big integer that fits into "long" */
#define MAX_INT      INT_MAX    /* a big integer that fits into "int" */
#define MAX_UNSIGNED_SHORT USHRT_MAX

/******** MISCELLANEOUS LIMITS *********/

#define MAX_NAME 256    /* max chars in any symbol (including '\0') */

#define MAX_BUF 100000 /* max chars in input string (including '\0') */

#define MAX_VARS 64   /* maximum # of distinct variables in clause */
#define VAR_TYPE unsigned short  
                      /* must be able to hold MAX_VARS * (max-multiplier+1) */

#define FPA_SIZE 3793   /* size of FPA hash tables */

#define MAX_FS_TERM_DEPTH 300  /* max depth of terms in IS-tree */
#define MAX_AL_TERM_DEPTH 500  /* max depth of alphas in IMD-tree */

/******** TYPES *********/

#define NAME 1        /* basic types of term */
#define VARIABLE 2
#define COMPLEX 3

                      /* types of non-VARIABLE term -- varnum field is used */

#define TERM 0              /* not an atom */
#define NORM_ATOM 1         /* normal atom */
#define POS_EQ 2            /* positive equality atom */
#define NEG_EQ 3            /* negative equality atom */
#define ANSWER 4            /* answer literal atom */
#define LEX_DEP_DEMOD 5     /* lex-dependent demodulator atom */
#define EVALUABLE 6         /* $ID, $LT, etc */
#define CONDITIONAL_DEMOD 7 /* conditional demodulator */

                        /* types of unification property tree */
#define UNIFY 1
#define INSTANCE 2
#define MORE_GEN 3

                 /* integer codes for membes of parent lists */
                 /* positive integers are clause IDs */

#define BINARY_RES_RULE     -1
#define HYPER_RES_RULE      -2
#define NEG_HYPER_RES_RULE  -3
#define UR_RES_RULE         -4
#define PARA_INTO_RULE      -5
#define PARA_FROM_RULE      -6
#define LINKED_UR_RES_RULE  -7
#define GEO_RULE            -8

#define FACTOR_RULE         -9
#define NEW_DEMOD_RULE     -10
#define BACK_DEMOD_RULE    -11

#define DEMOD_RULE         -12
#define UNIT_DEL_RULE      -13
#define EVAL_RULE          -14
#define GEO_ID_RULE        -15
#define FACTOR_SIMP_RULE   -16
#define COPY_RULE          -17
#define FLIP_EQ_RULE       -18
#define CLAUSIFY_RULE      -19
#define BACK_UNIT_DEL_RULE -20
#define SPLIT_RULE         -21
#define SPLIT_NEG_RULE     -22
#define PROPOSITIONAL_RULE -23

#define LIST_RULE        -1000

                /* integer codes for evaluable functions and predicates */
                /* When adding more, update built_in_symbols in io.c. */
                /* User-defined (foreign) evaluable functions are 0--1000 */

#define MAX_USER_EVALUABLE 1000

#define SUM_SYM          1001
#define PROD_SYM         1002
#define DIFF_SYM         1003
#define DIV_SYM          1004
#define MOD_SYM          1005

#define EQ_SYM           1006
#define NE_SYM           1007
#define LT_SYM           1008
#define LE_SYM           1009
#define GT_SYM           1010
#define GE_SYM           1011

#define FSUM_SYM         1012
#define FPROD_SYM        1013
#define FDIFF_SYM        1014
#define FDIV_SYM         1015

#define FEQ_SYM          1016
#define FNE_SYM          1017
#define FLT_SYM          1018
#define FLE_SYM          1019
#define FGT_SYM          1020
#define FGE_SYM          1021

#define BIT_AND_SYM      1022
#define BIT_OR_SYM       1023
#define BIT_XOR_SYM      1024
#define BIT_NOT_SYM      1025
#define SHIFT_LEFT_SYM   1026
#define SHIFT_RIGHT_SYM  1027

#define INT_TO_BITS_SYM  1028
#define BITS_TO_INT_SYM  1029

#define T_SYM            1030
#define F_SYM            1031
#define AND_SYM          1032
#define OR_SYM           1033
#define TRUE_SYM         1034
#define NOT_SYM          1035
#define IF_SYM           1036

#define ID_SYM           1037
#define LNE_SYM          1038
#define LLT_SYM          1039
#define LLE_SYM          1040
#define LGT_SYM          1041
#define LGE_SYM          1042

#define OCCURS_SYM       1043
#define VOCCURS_SYM      1044
#define VFREE_SYM        1045
#define RENAME_SYM       1046

#define NEXT_CL_NUM_SYM  1047
#define UNIQUE_NUM_SYM   1048

#define ATOMIC_SYM       1049
#define INT_SYM          1050
#define BITS_SYM         1051
#define VAR_SYM          1052
#define GROUND_SYM       1053

#define OUT_SYM          1054
#define COMMON_EXPRESSION_SYM 1055

		   /* comparing symbols and terms */

#define LESS_THAN        1
#define GREATER_THAN     2
#define SAME_AS          3
#define NOT_COMPARABLE   4
#define NOT_GREATER_THAN 5
#define NOT_LESS_THAN    6

#define LRPO_MULTISET_STATUS  0  /* lex RPO multiset status   */
#define LRPO_LR_STATUS        1  /* lex RPO left-right status */
/* (RL status removed for Otter 3.0 release.) */

/* Operator types */

#define XFX 1
#define XFY 2
#define YFX 3
#define FX  4
#define FY  5
#define XF  6
#define YF  7

		   /* linked-UR resolution inference rule. */

#define BOOLEAN char
#define FALSE 0
#define TRUE 1
#define UNDEFINED -1
#define NOT_SPECIFIED 0
#define NUCLEUS      1
#define LINK         2
#define BOTH         3
#define SATELLITE    4

                   /* first-order formulas */

#define ATOM_FORM 1
#define NOT_FORM 2
#define AND_FORM 3
#define OR_FORM 4
#define IMP_FORM 5
#define IFF_FORM 6
#define QUANT_FORM 7

#define ALL_QUANT 1
#define EXISTS_QUANT 2

                  /* exit codes */

#define  KEEP_SEARCHING      100
#define  INPUT_ERROR_EXIT    101
#define  ABEND_EXIT          102
#define  PROOF_EXIT          103
#define  SOS_EMPTY_EXIT      104
#define  MAX_GIVEN_EXIT      105
#define  MAX_SECONDS_EXIT    106
#define  MAX_GEN_EXIT        107
#define  MAX_KEPT_EXIT       108
#define  MAX_MEM_EXIT        109
#define  MALLOC_NULL_EXIT    110
#define  INTERACTIVE_EXIT    111
#define  SEGV_EXIT           112
#define  USR1_EXIT           113
#define  POSSIBLE_MODEL_EXIT 114
#define  MAX_LEVELS_EXIT     115

/************* END OF ALL GLOBAL CONSTANT DEFINITIONS ****************/

#include "cos.h"           /* flag, parameter, statistic, and clock names */
#include "foreign.h"       /* user-defined evaluable functions */
#include "macros.h"        /* preprocessor (#define) macros */

#include "fpa2.h"          /* new fpa code */
#include "types.h"         /* all of the type declarations */
#include "proto.h"         /* function prototypes */

/*********** GLOBAL VARIABLES ***********/

#ifdef IN_MAIN
#  define CLASS         /* empty string if included by main program */
#else
#  define CLASS extern  /* extern if included by anything else */
#endif

/* lists of clauses */

CLASS struct list *Usable;
CLASS struct list *Sos;
CLASS struct list *Demodulators;
CLASS struct list *Passive;
CLASS struct list *Hot;
CLASS struct list *Hints;
CLASS struct list *Hints2;
CLASS struct list *Mace_constraints;

/* FPA (indexing) lists for resolution inference rules */

CLASS struct fpa_index *Fpa_clash_pos_lits;
CLASS struct fpa_index *Fpa_clash_neg_lits;

/* FPA lists for unit conflict and back subsumption */

CLASS struct fpa_index *Fpa_pos_lits;
CLASS struct fpa_index *Fpa_neg_lits;

/* FPA lists for paramodulation inference rules */

CLASS struct fpa_index *Fpa_clash_terms; /* clashable terms */
CLASS struct fpa_index *Fpa_alphas;      /* alphas (left and right) */

/* FPA list for back demodulation */

CLASS struct fpa_index *Fpa_back_demod;  /* back demod candidates */

/* discrimination tree forward subsumption index */

CLASS struct is_tree *Is_pos_lits;  /* positive literals */
CLASS struct is_tree *Is_neg_lits;  /* negative literals */

/* discrimination tree index for demodulators */

CLASS struct imd_tree *Demod_imd;

/* Lists of weight templates */

CLASS struct term_ptr *Weight_purge_gen;    /* screen generated clauses */
CLASS struct term_ptr *Weight_pick_given;   /* pick given clause */
CLASS struct term_ptr *Weight_terms;        /* order terms */

/* Simple indexes (one level only) for weight templates */

CLASS struct is_tree *Weight_purge_gen_index;
CLASS struct is_tree *Weight_pick_given_index;
CLASS struct is_tree *Weight_terms_index;

/* options (Flags and Parms) */

CLASS struct {  /* Flags are boolean valued options */
    char *name;
    int val;
    } Flags[MAX_FLAGS];

CLASS struct {  /* Parms are integer valued options */
    char *name;
    int val;
    int min, max;  /* minimum and maximum permissible values */
    } Parms[MAX_PARMS];

CLASS int Internal_flags[MAX_INTERNAL_FLAGS];  /* invisible to user */

/* statistics */

CLASS long Stats[MAX_STATS];
CLASS int Subsume_count[100];

/* clocks */

CLASS struct clock Clocks[MAX_CLOCKS];

/* Other built-in symbols */

CLASS int Cons_sym_num, Nil_sym_num, Ignore_sym_num, Chr_sym_num, Dots_sym_num;

/* table of user functions */

CLASS struct user_function User_functions[MAX_USER_FUNCTIONS];

CLASS FILE *Null_output;

/* Miscellaneous global variables */

CLASS char Float_format[MAX_NAME];
CLASS struct term *Overbeek_terms;  /* Special weighting */
CLASS struct term *Split_atoms;     /* Atoms to split */

CLASS char Bell;

CLASS int Max_input_id;  /* Maxumim ID of an input clause */

/* More special weighting */

CLASS struct term_ptr **Overbeek_world;

#endif  /* ! TP_HEADER_H */
