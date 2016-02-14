/*
 *  types.h -- type declarations
 *
 */

struct term {
    struct rel *farg;         /* subterm list; used for complex only */
    union {              /* term is atom iff (NAME or COMPLEX) && varnum > 0 */
        struct rel *rel;      /* superterm list; used for all except atoms */
	struct literal *lit;  /* containing literal; used for atoms */
	} occ;
    int fpa_id;               /* used to order fpa lists */
    unsigned short sym_num;   /* used for names, complex, and sometimes vars */
    VAR_TYPE varnum;          /* used for variables */
    unsigned char type;       /* NAME, VARIABLE, or COMPLEX */
    unsigned char bits;       /* bit flags (see macros.h) */
    };

struct rel {  /* relations between terms */
    struct term *argval;     /* subterm */
    struct term *argof;      /* superterm */
    struct rel *narg;        /* rest of subterm list */
    struct rel *nocc;        /* rest of superterm list */
    unsigned char path;      /* used in paramod to mark path to into term */
    unsigned char clashable; /* paramodclashability flag */
    };

struct sym_ent {  /* symbol table entry */
    struct sym_ent *next;
    int sym_num;           /* unique identifier */
    int arity;             /* arity 0 for constants, variables */
    int lex_val;           /* can be used to assign a lexical value */
    int eval_code;         /* identifies evaluable functors ($ symbols) */
    int skolem;            /* identifies Skolem constants and functions */
    int special_unary;     /* identifies special unary symbol for lex check */
    int lex_rpo_status;    /* status for LRPO */
    char name[MAX_NAME];   /* the print symbol */
    int special_op;  /* for infix/prefix/postfix functors */
    int op_type;     /* for infix/prefix/postfix functors */
    int op_prec;     /* for infix/prefix/postfix functors */
    };

struct term_ptr {     /* for constructing a list of pointers to terms */
    struct term *term;
    struct term_ptr *next;
    };

struct formula_ptr_2 {     /* for many-linked list of pointers to formulas */
    struct formula *f;
    struct formula_ptr_2 *prev, *next, *left, *right, *up, *down;
    };

struct fpa_tree {     /* for constructing fpa path lookup tree */
    struct fposition position;   /* for leaves only */
    struct fpa_tree *left;    /* for AND and OR nodes */
    struct fpa_tree *right;   /* for AND and OR nodes */
    struct term *left_term;   /* for OR nodes only */
    struct term *right_term;  /* for OR nodes only */
    int type;                 /* 1 AND,  2 OR,  3 LEAF */
    int *path;       /* for debugging only */
    };

struct fpa_head {            /* head of an FPA list */
    struct fnode *terms;       /* terms with path */
    struct fpa_head *next;        /* next FPA list */
    int *path;
    };

struct fpa_index {
    struct fpa_head *table[FPA_SIZE];
    };

struct context {          /* substitution table */
    struct term *terms[MAX_VARS];
    struct context *contexts[MAX_VARS];
    int status[MAX_VARS];  /* for batch occur check */
    int multiplier;  /* needed for apply, not for unify or match */
    int built_in_multiplier;  /* the use of this is optional */
    };

struct trail {     /* to record an entry in a substitution table */
    struct context *context;
    struct trail *next;
    int varnum;
    };

struct imd_tree {         /* index/match/demodulate tree */
    struct imd_tree *next, *kids;
    struct term_ptr *atoms;
    unsigned short lab;   /* variable number or symbol number */
    unsigned char type;   /* VARIABLE, NAME, or COMPLEX */
    
	                  /* the following are used for leaves only */
    VAR_TYPE max_vnum;    /* max. variable number, for clearing substitution */
    };

struct imd_pos {  /* save a stack of states for backtrack in imd indexing */
    struct imd_pos *next;
    struct imd_tree *imd;
    struct rel *rel_stack[MAX_AL_TERM_DEPTH]; /* save position in given term */
    int reset;    /* flag for clearing instantiation on backtracking */
    int stack_pos;                          /* for backtracking */
    };

struct is_tree {  /* index-subsume tree */
    struct is_tree *next;  /* sibling */
    union {
	struct is_tree *kids;    /* for internal nodes */
	struct term_ptr *terms;  /* for leaves */
	} u;
    unsigned short lab;    /* variable number or symbol number */
    unsigned char type;    /* VARIABLE, NAME, or COMPLEX */
    };

struct is_pos {  /* save a stack of states for backtrack in is indexing */
    struct is_pos *next;
    struct is_tree *is;
    struct rel *rel_stack[MAX_FS_TERM_DEPTH]; /* save position in given term */
    int reset;    /* flag for clearing instantiation on backtracking */
    int stack_pos;                          /* for backtracking            */
    };

struct fsub_pos {  /* to save position in set of subsuming literals */
    struct term_ptr *terms;  /* list of identical terms from leaf of is tree */
    struct is_pos *pos;  /* stack of states for backtracking */
    };

struct literal {
    struct clause *container;  /* containing clause */
    struct literal *next_lit;
    struct term *atom;
    char sign;
    BOOLEAN target;
    };

struct clause {
    struct ilist *parents;
    struct g2list *multi_parents;  /* for proof-shortening experiment */
    struct list *container;
    struct clause *prev_cl, *next_cl;  /* prev and next clause in list */
    struct literal *first_lit;
    int id;
    int pick_weight;
    struct cl_attribute *attributes;
    short type;          /* for linked inf rules */
    unsigned char bits;  /* for linked inf rules */
    char heat_level;
    };

struct list {  /* the primary way to build a list of clauses */
    struct clause *first_cl, *last_cl;
    char name[MAX_NAME];  /* name of list */
    };

struct clause_ptr {  /* an alternate way to build a list of clauses */
    struct clause *c;
    struct clause_ptr *next;
    };

struct ilist {  /* for building a list of integers */
    struct ilist *next;
    int i;
    };

struct ci_ptr {  /* for building a list of <clause,ilist> pairs */
    struct ci_ptr *next;
    struct clause *c;
    struct ilist *v;
    };

struct clash_nd {   /* for hyper and UR--one for each clashable lit of nuc */
    struct term *nuc_atom;   /* atom from nucleus */
    struct fpa_index *db;    /* fpa index to use for finding satellites */
    struct fpa_tree *u_tree; /* unification path tree (position in sats) */
    struct context *subst;   /* unifying substitution */
    struct trail *tr;        /* trail to undo substitution */
    struct term *found_atom; /* unifying atom */
    int evaluable;           /* $ evaluation */
    int evaluation;          /* $ evaluation */
    int already_evaluated;   /* $ evaluation */
    struct clash_nd *prev, *next;  /* links */
    };

struct clock {   /* for timing operations, see cos.h, macros.h, clocks.c */
    long accum_sec;   /* accumulated time */
    long accum_usec;
    long curr_sec;    /* time since clock has been turned on */
    long curr_usec;
    };

struct ans_lit_node {
    struct ans_lit_node *next;
    struct link_node *parent;
    struct literal *lit;
    };

struct link_node {
    struct link_node *parent, *next_sibling, *prev_sibling, *first_child;
    struct ans_lit_node *child_first_ans, *child_last_ans;
    BOOLEAN first;
    BOOLEAN unit_deleted;  /* TRUE if goal_to_resolve has been unit deleted */
    struct term *goal, *goal_to_resolve;
    struct clause *current_clause;
    struct context *subst;
    struct trail *tr;
    struct fpa_tree *unif_position;
    int near_poss_nuc, farthest_sat, target_dist, back_up;
    };

struct formula_box {  /* This is for the "Formed" formula display program. */
    int type;         /* FORMULA, OPERATOR */
    int subtype;      /* COMPLEX_FORM, ATOM_FORM */
                      /* OR_OP, AND_OP, NOT_OP, EXISTS_OP, ALL_OP */
    struct formula *f;
    char str[100];

    int length, height;   /* size of box */
    int x_off, y_off;     /* offset from parent */
    int abs_x_loc, abs_y_loc; /* absolute location of box in window */

    struct formula_box *first_child;
    struct formula_box *next;
    struct formula_box *parent;
    };

struct formula {
    struct formula *parent, *first_child, *next;
    struct term *t;  /* for atoms and for quantifier variables */
    char type;
    char quant_type;
    };

struct formula_ptr {
    struct formula *f;
    struct formula_ptr *next;
    };

struct cl_attribute {
    int name;
    union {
	int i;
	double d;
	char *s;
	struct term *t;
	} u;
    struct cl_attribute *next;
    };

struct glist {
  void *v;
  struct glist *next;
};

struct g2list {
  void *v1;
  void *v2;
  struct g2list *next;
};

