#ifndef TP_DP_H
#define TP_DP_H

#define UNSATISFIABLE  0
#define SATISFIABLE    1
#define NOT_DETERMINED 2

#define ATOM_FALSE         0
#define ATOM_TRUE          1
#define ATOM_NOT_ASSIGNED  2

/* Memory conservation is important, so the following integer types are
 * defined to be just as big as required.  All can safely be unsigned.
 *
 * ATOM_INT: To store integers representing atoms.  (The greatest number
 *      of atoms I have seen is 37026 in qg7.33).
 * LIT_INT:  To store the number of pos or neg literals in a clause.
 *      For large problems, this is usually limited by the domain size.
 */

/* August 2003.  When I installed the "parting" code, this allowed the
   number of atoms to go way up, so I changed the data type to int.
*/

#if 0  /* This is faster and uses less memory. */
#  define ATOM_INT          unsigned short
#  define ATOM_INT_MAX      USHRT_MAX
#else
#  define ATOM_INT          int
#  define ATOM_INT_MAX      INT_MAX
#endif

#define LIT_INT           unsigned char
#define LIT_INT_MAX       UCHAR_MAX

struct MACE_clause {
    LIT_INT num_pos;      /* number of positive literals */
    LIT_INT num_neg;      /* number of negative literals */
    LIT_INT active_pos;   /* number of active (unassigned) positive literals */
    LIT_INT active_neg;   /* number of active (unassigned) negative literals */
    /* The following two are dynamically allocated arrays of ATOM_INT. */
    ATOM_INT *pos;        /* array of positive literals */
    ATOM_INT *neg;        /* array of negative literals (stored positively) */
    struct MACE_clause *next;
    ATOM_INT subsumer;    /* 0 if not subsumed; else index of subsuming atom */
    };

typedef struct MACE_clause *Clause_ptr;

struct atom {
    int value;            /* ATOM_TRUE, ATOM_FALSE, ATOM_NOT_ASSIGNED */
    int enqueued_value;   /* ATOM_TRUE, ATOM_FALSE, ATOM_NOT_ASSIGNED */
    int num_pos_occ;      /* number of positive occurrences */
    int num_neg_occ;      /* number of negative occurrences */
    /* The following two are dynamically allocated arrays of ptrs to clauses. */
    Clause_ptr *pos_occ;  /* array of pos occurrences (pointers to clauses) */
    Clause_ptr *neg_occ;  /* array of neg occurrences (pointers to clauses) */
    };

typedef struct atom *Atom_ptr;

struct gen_ptr {  /* for constructing lists of pointers or of integers */
    struct gen_ptr *next;
    union {
        void *v;
	int i;
	} u;
    };

typedef struct gen_ptr *Gen_ptr_ptr;

/* function prototypes from dp.c */

void init_dp();
void exit_if_over_time_limit();
int subsumed(Clause_ptr c);
void MACE_p_clause(Clause_ptr c);
void MACE_pp_clause(Clause_ptr c);
void p_atom(int i);
void insert_dp_clause(int c[], int n);
int read_all_clauses(FILE *fp);
int more_setup();
int atom_value(int atom);
void save_if_appropriate(void);
void restore_assignments(void);
int dp_prover();
void reinit_dp(void);

#endif  /* ! TP_DP_H */
