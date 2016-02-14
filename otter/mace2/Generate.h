#ifndef TP_GENERATE_H
#define TP_GENERATE_H

#define FUNCTION 0
#define RELATION 1

#define MACE_EQ_BIT           1
#define MACE_ORDER_BIT        2
#define MACE_QUASIGROUP_BIT   4
#define MACE_BIJECTION_BIT    8
#define MACE_COMMUTATIVE_BIT 16

#define MAX_MNAME       32  /* string length of functors */
#define MAX_ARITY        4  /* See generate_prop_clauses(), ATOM, etc. */
#define MAX_MVARS       12  /* If changed, change generate_prop_clauses(). */
#define MAX_SYMBOLS    500  /* number of functors */
#define MAX_SORTS       10  /* number of sorts */

struct sort {
  char name[MAX_MNAME];    /* name of sort */
  int n;                   /* size of sort */
};

struct symbol {
  int base;                    /* prop. atom numbers start here */
  int end;                     /* prop. atom numbers end here */
  int arity;                   /* of symbol */
  struct {
    int sort;                /* index of sort */
    int n;                   /* size of sort */
    int mult;                /* for constructing prop. variables */
  } args[MAX_ARITY];
  char name[MAX_MNAME];         /* for printing */
  int type;                     /* FUNCTION or RELATION */
  int properties;               /* special properties of symbol */
  int assigned_value;           /* for distinct constants option */
};

/* The following macros take sequences of integers and give unique integers
 * that are the propositional variables.  The input, for example b,i,j,
 * represents application of symbol with "base" b to elements of the
 * domain.
 */

#define ATOM0(s) (s->base)
#define ATOM1(s,i) (i + s->base)
#define ATOM2(s,i,j) ((i)*s->args[0].mult + j + s->base)
#define ATOM3(s,i,j,k) ((i)*s->args[0].mult + (j)*s->args[1].mult + k + s->base)
#define ATOM4(s,i,j,k,l) ((i)*s->args[0].mult + (j)*s->args[1].mult + (k)*s->args[2].mult + l + s->base)

/* function prototypes from generate.c */

void read_all_mace_input(int argc,
			 char **argv);
int dp_trans(int domain_size, int distinct_constants, int qg_constraint);
void decode_and_p_clause(int c[], int n);
void reinit_generate(void);
int decode_int(int atom, char **func, int *arity, int *args);
void p_unit(int u);

#endif  /* ! TP_GENERATE_H */
