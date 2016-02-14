#include "Mace2.h"

/* This file has all of the code for the first-order interface to
 * propositional model searching.
 */

/* TO DO:  (This list is very OLD!!)
 *  1. When add units from assignments, also add neg units??
 *  2. Move unit preprocessing to DP code??
 *  3. Enumerated sorts.
 *  4. Allow assigned or enumerated constants in clauses.
 *  5. Otter input for sorted logic.
 */

#if 0
#define PRINT_CLAUSES  /* print ground clauses that are sent to DP */
#endif

/* Limits
 * WARNING: you can't simply change MAX_VARS or MAX_ARITY.
 */

#define MAX_MLITERALS   MAX_DOMAIN  /* per clause */
#define MAX_CLAUSES           1000  /* first-order clauses */

/******** Type declarations *********/

struct rel_lit {      /* first_order literal */
  int sign;         /* 1 or -1 */
  int functor;      /* index into Symbols */
  int a[MAX_ARITY]; /* args: vars are +, domain elements are -(e+100). */
  char ai[MAX_ARITY][MAX_MNAME];  /* array of args in string form */
};

struct rel_clause {     /* first_order clause */
  int id;
  int num_lits;
  struct rel_lit lits[MAX_MLITERALS];
};

/***************************************************************/

/******* Global variables ***************/

extern int Greatest_atom;  /* owned by dp.c */

struct sort Sorts[MAX_SORTS];
int Num_sorts;

struct symbol Symbols[MAX_SYMBOLS];
int Next_base = 1;
int Num_symbols;

static int Clauses_generated;
static int Clauses_inserted;

static struct rel_clause Clauses[MAX_CLAUSES];
static int Num_rel_clauses;

static int *Units;
static int Unsatisfiable = 0;  /* This flags an unsatisfiable set */

/*************
 *
 *   sort_index()
 *
 *************/

static int sort_index(char *s)
{
  int i;
  for (i = 0; i < Num_sorts; i++)
    if (str_ident(s, Sorts[i].name))
      return(i);
  return(-1);
}  /* sort_index */

/*************
 *
 *   declare_symbol_sorted()
 *
 *************/

static int declare_symbol_sorted(char *name, int arity, char at[][MAX_MNAME])
{
  int i, j, m;
  struct symbol *s;

  if (Num_symbols == MAX_SYMBOLS)
    MACE_abend("declare_symbol_sorted, too many symbols");

  s = Symbols + Num_symbols;
  Num_symbols++;
  strcpy(s->name, name);
  s->arity = arity;
  s->base = Next_base;
  s->assigned_value = -1;

  for (i = 0; i < arity; i++) {
    j = sort_index(at[i]);
    if (j == -1) {
      printf("%s", at[i]);
      MACE_abend("declare_symbol_sorted: sort not found");
    }
    s->args[i].sort = j;
    s->args[i].n = Sorts[j].n;
  }

  for (i = arity-1, m = 1; i >= 0; i--) {
    s->args[i].mult = m;
    m = m * s->args[i].n;
  }
  s->end = (s->base + m) - 1;

  Next_base += m;
  return(Num_symbols-1);
}  /* declare_symbol_sorted */

/*************
 *
 *   str_to_id()
 *
 *   Given the string form of a functor, return the ID (or -1 if not found).
 *
 *************/

static int str_to_id(char *name)
{
  int i;
  for (i = 0; i < Num_symbols; i++)
    if (str_ident(name, Symbols[i].name))
      return(i);
  return(-1);
}  /* str_to_id */

/*************
 *
 *   decode_int()
 *
 *   Given a propositional variable (an integer), find the atom that
 *   it represents.  This is the inverse of the ATOM macros.
 *   This is used mostly for printing ground clauses in a readable way.
 *
 *************/

int decode_int(int atom, char **func, int *arity, int *args)
{
  int i, c;
  struct symbol *s;

  if (atom >= Next_base || atom < 1)
    return(-1);
  for (i = 0; i < Num_symbols; i++)
    if (atom < Symbols[i].base)
      break;
  i--;
  s = Symbols+i;
  c = atom - s->base;
  *func = s->name;
  *arity = s->arity;

  switch (*arity) {
  case 1:
    args[0] = c;
    break;
  case 2:
    args[1] = c % s->args[1].n; c = c / s->args[1].n;
    args[0] = c;
    break;
  case 3:
    args[2] = c % s->args[2].n; c = c / s->args[2].n;
    args[1] = c % s->args[1].n; c = c / s->args[1].n;
    args[0] = c;
    break;
  case 4:
    args[3] = c % s->args[3].n; c = c / s->args[3].n;
    args[2] = c % s->args[2].n; c = c / s->args[2].n;
    args[1] = c % s->args[1].n; c = c / s->args[1].n;
    args[0] = c;
    break;
  }
  return(1);
}  /* decode_int */

/*************
 *
 *  p_unit(u)
 *
 *************/

void p_unit(int u)
{
  char *name, *sign;
  int arity, args[MAX_ARITY], j;

  sign = (u < 0 ? "-" : "");
  decode_int(abs(u), &name, &arity, args);
  printf(" %s%s", sign, name);
  for (j=0; j<arity; j++)
    printf("%d", args[j]);
} /* p_unit */

/*************
 *
 *   decode_and_p_clause()
 *
 *   Decode and print a propositional clause.
 *
 *************/

void decode_and_p_clause(int c[], int n)
{
  char *name, *sign;
  int arity, args[MAX_ARITY], i, j;

  for (i = 0; i < n; i++) {
    sign = (c[i] < 0 ? "-" : "");
    decode_int(abs(c[i]), &name, &arity, args);
    printf("  %s%s", sign, name);
    for (j=0; j<arity; j++)
      printf("%d", args[j]);
  }
  printf("\n");
}  /* decode_and_p_clause */

/*************
 *
 *   p_relational_clause()
 *
 *************/

static void p_relational_clause(struct rel_clause *c)
{
  int i, j, arity;
    
  for (i = 0; i < c->num_lits; i++) {
    int v, sign;

    sign = c->lits[i].sign;
    arity = Symbols[c->lits[i].functor].arity;
    printf("%s%s%s",
	   sign==1?"":"-",
	   Symbols[c->lits[i].functor].name,
	   arity>0?"(":"");
    for (j = 0; j < arity; j++) {
      v = c->lits[i].a[j];
      if (v >= 0)
	printf("v%d", v);  /* variable */
      else
	printf("%d", -(v+100));  /* element */
      printf("%s", j == arity-1 ? ") " : ",");
    }
  }
  printf("\n");
  fflush(stdout);
}  /* p_relational_clause */

/*************
 *
 *   sort_clause() -- propositional clause, by atom.
 *
 *************/

static void sort_clause(int c[], int n)
{
  int i, j, temp;

  for (i = 0; i < n-1; i++)
    for (j = i+1; j < n; j++)
      if (abs(c[j]) < abs(c[i])) {
	temp = c[i]; c[i] = c[j]; c[j] = temp;
      }

}  /* sort_clause */

/*************
 *
 *   unit_subsumed()
 *
 *   Is a given propositional clause subsumed by anything in the Units array?
 *
 *************/

static int unit_subsumed(int c[], int n)
{
  int i, a, sign;
  for (i = 0; i < n; i++) {
    a = abs(c[i]);
    sign = (c[i] < 0) ? -1 : 1;
    if (Units[a] == sign)
      return(1);
  }
  return(0);
}  /* unit_subsumed */

/*************
 *
 *   MACE_tautology() -- propositional; assume sorted by atom.
 *
 *************/

static int MACE_tautology(int c[], int n)
{
  int i;
  for (i = 0; i < n-1; i++)
    if (c[i] == -c[i+1])
      return(1);
  return(0);
}  /* MACE_tautology */

/*************
 *
 *   merge_lits() -- propositional; assume sorted by atom.
 *
 *************/

static int merge_lits(int c[], int n)
{
  int i, j, m;
  m = n;
  for (i = 0; i < m-1; i++)
    if (c[i] == c[i+1]) {
      for (j = i+1; j < m-1; j++)
	c[j] = c[j+1];
      m--;
      i--;
    }
  return(m);
}  /* merge_lits */

/*************
 *
 *   unit_delete()
 *
 *   Given a propositional clause, do unit resolution with Units array.
 *   This kind of clause is an array of ground literals.  When a literal
 *   is removed, literals to the right are shifted left.
 *   Return the number of remaining literals.
 *
 *************/

static int unit_delete(int c[], int n)
{
  int i, j, l, a, v, m;
  m = n;
  for (i = 0; i < m; i++) {
    l = c[i]; a = abs(l); v = Units[a];
    if ((v == -1 && l > 0) || (v == 1 && l < 0)) {
      for (j = i; j < m-1; j++)
	c[j] = c[j+1];
      m--;
      i--;
    }
  }
  return(m);
}  /* unit_delete */

/*************
 *
 *   add_clause()
 *
 *   Given a propositional clause, do some preprocessing, and if it
 *   passes the tests, send it to insert_dp_clause(), which is part
 *   of the interface to the DP procedure.
 *
 *************/

static void add_clause(int c[], int n)
{
  Clauses_generated++;
#ifdef PRINT_CLAUSES
  decode_and_p_clause(c,n);
#endif

  n = unit_delete(c, n);
  if (n == 0) {
    printf("*** Unsatisfiability detected ***\n");
    Unsatisfiable = 1;
  }
  else {
    sort_clause(c, n);
    if (!unit_subsumed(c, n) && !MACE_tautology(c, n)) {
      n = merge_lits(c, n);
      if (n == 1)
	Units[abs(c[0])] = c[0] < 0 ? -1 : 1;
      insert_dp_clause(c, n);
      Clauses_inserted++;
    }
  }
}  /* add_clause */

/*************
 *
 *   add_2clause()
 *
 *   Given 2 literals, make a 2-clause and send it to add_clause.
 *
 *************/

static void add_2clause(int a, int b)
{
  int c[2];
  c[0] = a; c[1] = b;
  add_clause(c, 2);
}  /* add_2clause */

/*************
 *
 *   add_unit()
 *
 *   Build a literal and send it to add_clause.
 *
 *************/

static void add_unit(int sign, struct symbol *s, int a0, int a1, int a2, int a3)
{
  int c[1];

  if (a0 >= s->args[0].n || a1 >= s->args[1].n || a2 >= s->args[2].n || a3 >= s->args[3].n)
    MACE_abend("add_unit: element out of domain");
  if (abs(sign) != 1)
    MACE_abend("add_unit: sign must be 1 or -1");

#if 0
  printf("add_unit: %s%s %d %d %d %d.\n", 
	 sign==1?"":"-", Symbols[functor].name, a0, a1, a2, a3);
#endif
    
  switch(s->arity) {
  case 0: c[0] = sign * ATOM0(s); break;
  case 1: c[0] = sign * ATOM1(s, a0); break;
  case 2: c[0] = sign * ATOM2(s, a0, a1); break;
  case 3: c[0] = sign * ATOM3(s, a0, a1, a2); break;
  case 4: c[0] = sign * ATOM4(s, a0, a1, a2, a3); break;
  }
  add_clause(c, 1);
}  /* add_unit */

/*
 * The following set of routines assert various properties of functions.
 * Recall that an n-ary function is handled as an (n+1)-ary relation,
 * so we have to say that the function is well-defined and closed.
 * Similar routines can be used to assert that functions are 1-1, onto,
 * cancellative, etc.
 */

/*************
 *
 *   well_defined_0() - constant cannot have 2 values.
 *
 *************/

static void well_defined_0(struct symbol *s)
{
  int y, z;
  int n0 = s->args[0].n;
    
  /* -By | -Bz, for y < z */
  for (y = 0; y < n0; y++)
    for (z = y+1; z < n0; z++)
      add_2clause( - ATOM1(s,y), - ATOM1(s,z));
}  /* well_defined_0 */

/*************
 *
 *   well_defined_1()
 *
 *************/

static void well_defined_1(struct symbol *s)
{
  int x, y, z;
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;
    
  /* -Bxy | -Bxz, for y < z */
  for (x = 0; x < n0; x++)
    for (y = 0; y < n1; y++)
      for (z = y+1; z < n1; z++)
	add_2clause( - ATOM2(s,x,y), - ATOM2(s,x,z));
}  /* well_defined_1 */

/*************
 *
 *   well_defined_2()
 *
 *************/

static void well_defined_2(struct symbol *s)
{
  int u, x, y, z;
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;
  int n2 = s->args[2].n;

  /* -Buxy | -Buxz, for y < z */
  for (u = 0; u < n0; u++)
    for (x = 0; x < n1; x++)
      for (y = 0; y < n2; y++)
	for (z = y+1; z < n2; z++)
	  add_2clause( - ATOM3(s,u,x,y), - ATOM3(s,u,x,z));
}  /* well_defined_2 */

/*************
 *
 *   well_defined_3()
 *
 *************/

static void well_defined_3(struct symbol *s)
{
  int v, u, x, y, z;
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;
  int n2 = s->args[2].n;
  int n3 = s->args[3].n;

  /* -Bvuxy | -Bvuxz, for y < z */
  for (v = 0; v < n0; v++)
    for (u = 0; u < n1; u++)
      for (x = 0; x < n2; x++)
	for (y = 0; y < n3; y++)
	  for (z = y+1; z < n3; z++)
	    add_2clause( - ATOM4(s,v,u,x,y), - ATOM4(s,v,u,x,z));
}  /* well_defined_3 */

/*************
 *
 *   closed_0() - value of constant is one of the domain elements.
 *
 *************/

static void closed_0(struct symbol *s)
{
  int x, c[MAX_MLITERALS];
  int n0 = s->args[0].n;

  /* B0 | B1 | .. | Bn-1 */
  for (x = 0; x < n0; x++)
    c[x] = ATOM1(s,x);
  add_clause(c, n0);
}  /* closed_0 */

/*************
 *
 *   closed_1()
 *
 *************/

static void closed_1(struct symbol *s)
{
  int x, y, c[MAX_MLITERALS];
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;

  /* Bx0 | Bx1 | .. | Bxn-1, 0<=x<=n-1 */
  for (x = 0; x < n0; x++) {
    for (y = 0; y < n1; y++)
      c[y] = ATOM2(s,x,y);
    add_clause(c, n1);
  }
}  /* closed_1 */

/*************
 *
 *   closed_2()
 *
 *************/

static void closed_2(struct symbol *s)
{
  int x, y, z, c[MAX_MLITERALS];
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;
  int n2 = s->args[2].n;

  /* Bxy0 | Bxy1 | .. | Bxyn-1, 0<=x<=n-1, 0<=y<=n-1 */
  for (x = 0; x < n0; x++)
    for (y = 0; y < n1; y++) {
      for (z = 0; z < n2; z++)
	c[z] = ATOM3(s,x,y,z);
      add_clause(c, n2);
    }
}  /* closed_2 */

/*************
 *
 *   closed_3()
 *
 *************/

static void closed_3(struct symbol *s)
{
  int x, y, z, u, c[MAX_MLITERALS];
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;
  int n2 = s->args[2].n;
  int n3 = s->args[3].n;

  /* Bxyz0 | Bxyz1 | .. | Bxyzn-1, 0<=x<=n-1, 0<=y<=n-1, 0<=z<=n-1 */
  for (x = 0; x < n0; x++)
    for (y = 0; y < n1; y++)
      for (z = 0; z < n2; z++) {
	for (u = 0; u < n3; u++)
	  c[u] = ATOM4(s,x,y,z,u);
	add_clause(c, n3);
      }
}  /* closed_3 */

/*************
 *
 *   well_defined()
 *
 *************/

static void well_defined(struct symbol *s)
{
  switch(s->arity) {
  case 1: well_defined_0(s); break;
  case 2: well_defined_1(s); break;
  case 3: well_defined_2(s); break;
  case 4: well_defined_3(s); break;
  default: MACE_abend("well_defined, bad arity");
  }
}  /* well_defined */

/*************
 *
 *   closed()
 *
 *************/

static void closed(struct symbol *s)
{
  switch(s->arity) {
  case 1: closed_0(s); break;
  case 2: closed_1(s); break;
  case 3: closed_2(s); break;
  case 4: closed_3(s); break;
  default: MACE_abend("closed, bad arity");
  }
}  /* closed */

/*************
 *
 *   one_one()
 *
 *************/

static void one_one(struct symbol *s)
{
  int x, y, z;
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;

  /* -Bxy | -Bzy, x < z */
  for (y = 0; y < n1; y++)
    for (x = 0; x < n0; x++)
      for (z = x+1; z < n0; z++)
	add_2clause( - ATOM2(s,x,y), - ATOM2(s,z,y));

}  /* one_one */

/*************
 *
 *   onto()
 *
 *************/

static void onto(struct symbol *s)
{
  int x, y, c[MAX_MLITERALS];
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;

  /* B0x | B1x | .. | Bn-1x, 0<=x<=n-1 */

  for (x = 0; x < n1; x++) {
    for (y = 0; y < n0; y++)
      c[y] = ATOM2(s,y,x);
    add_clause(c, n1);
  }
}  /* onto */

/*************
 *
 *   left_cancel()
 *
 *************/

static void left_cancel(struct symbol *s)
{
  int u, x, y, z;
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;
  int n2 = s->args[2].n;

  /* -Buyx | -Buzx, for y < z */

  for (u = 0; u < n0; u++)
    for (x = 0; x < n2; x++)
      for (y = 0; y < n1; y++)
	for (z = y+1; z < n1; z++)
	  add_2clause( - ATOM3(s,u,y,x), - ATOM3(s,u,z,x));

}  /* left_cancel */

/*************
 *
 *   right_cancel()
 *
 *************/

static void right_cancel(struct symbol *s)
{
  int u, x, y, z;
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;
  int n2 = s->args[2].n;

  /* -Byux | -Bzux, for y < z */

  for (u = 0; u < n1; u++)
    for (x = 0; x < n2; x++)
      for (y = 0; y < n0; y++)
	for (z = y+1; z < n0; z++)
	  add_2clause( - ATOM3(s,y,u,x), - ATOM3(s,z,u,x));

}  /* right_cancel */

/*************
 *
 *   left_onto()
 *
 *************/

static void left_onto(struct symbol *s)
{
  int x, y, z, c[MAX_MLITERALS];
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;
  int n2 = s->args[2].n;

  /* Bx0y | Bx1y | .. | Bxn-1y, 0<=x<=n-1, 0<=y<=n-1 */

  for (x = 0; x < n0; x++)
    for (y = 0; y < n2; y++) {
      for (z = 0; z < n1; z++)
	c[z] = ATOM3(s,x,z,y);
      add_clause(c, n1);
    }
}  /* left_onto */

/*************
 *
 *   right_onto()
 *
 *************/

static void right_onto(struct symbol *s)
{
  int x, y, z, c[MAX_MLITERALS];
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;
  int n2 = s->args[2].n;

  /* B0xy | B1xy | .. | Bn-1xy, 0<=x<=n-1, 0<=y<=n-1 */

  for (x = 0; x < n1; x++)
    for (y = 0; y < n2; y++) {
      for (z = 0; z < n0; z++)
	c[z] = ATOM3(s,z,x,y);
      add_clause(c, n0);
    }
}  /* right_onto */

/*************
 *
 *   process_variables()
 *
 *   Check that all occurrences of each variable are in a position of the
 *   same sort, collect the domain sizes of the variables into an array,
 *   and return the biggest variable.
 *
 *************/

static int process_variables(struct rel_lit *lits, int num_lits, int domains[])
{
  int i, j, biggest_var, v;
  struct symbol *s;

  for (i = 0; i < MAX_MVARS; i++)
    domains[i] = -1;
  biggest_var = -1;

  for (i = 0; i < num_lits; i++) {
    s = Symbols + lits[i].functor;
    for (j = 0; j < s->arity; j++) {
      v = lits[i].a[j];
      if (v >= 0) {
	if (v > biggest_var) {
	  if (v >= MAX_MVARS)
	    MACE_abend("process_variables, variable too big.");
	  biggest_var = v;
	}
	if (domains[v] == -1)
	  domains[v] = s->args[j].sort;
	else if (domains[v] != s->args[j].sort)
	  MACE_abend("process_variables, badly sorted clause");
      }
    }
  }

  /* Now replace sort indexes with sort sizes. */

  for (i = 0; i <= biggest_var; i++)
    domains[i] = Sorts[domains[i]].n;

  return(biggest_var);
}  /* process_variables */

/*************
 *
 *   generate_prop_clauses()
 *
 *   Given a flattened general clause, generate the corresponding
 *   ground clauses over the domain and send them to add_clause.
 *
 *   Note that limits on the arity and on the number of variables
 *   are wired into the code.
 *
 *************/

static void generate_prop_clauses(struct rel_clause *c)
{
  int v0, v0_lim, v1, v1_lim, v2, v2_lim, v3, v3_lim, v4, v4_lim, v5, v5_lim;
  int v6, v6_lim, v7, v7_lim, v8, v8_lim, v9, v9_lim, v10, v10_lim, v11, v11_lim;
  int biggest_var, val[MAX_MVARS], i, sign, domains[MAX_MVARS];
  int a0, a1, a2, a3;
  int prop_clause[MAX_MLITERALS];
  struct symbol *s;
  struct rel_lit *lits = c->lits;
  int num_lits = c->num_lits;

  a0 = a1 = a2 = a3 = 0;

  biggest_var = process_variables(lits, num_lits, domains);

#ifdef PRINT_CLAUSES
  printf("  biggest_var=%d: ", biggest_var);
  p_relational_clause(c);
#endif

  /* Now domains[] has the domain sizes for all the variables.
   * Fill in the remaining slots with 1 so that the nested loops
   * below work correctly.
   */

  for (i = biggest_var+1; i < MAX_MVARS; i++)
    domains[i] = 1;

  v0_lim = domains[0]; v1_lim = domains[1]; v2_lim = domains[2];
  v3_lim = domains[3]; v4_lim = domains[4]; v5_lim = domains[5];
  v6_lim = domains[6]; v7_lim = domains[7]; v8_lim = domains[8];
  v9_lim = domains[9]; v10_lim = domains[10]; v11_lim = domains[11];

  /* Check the time limit somewhere in here.  It can be expensive,
     especially on solaris, so do do it too often.
  */

  for (v0 = 0; v0 < v0_lim; v0++) { val[0] = v0;
  for (v1 = 0; v1 < v1_lim; v1++) { val[1] = v1;
  for (v2 = 0; v2 < v2_lim; v2++) { val[2] = v2; exit_if_over_time_limit();
  for (v3 = 0; v3 < v3_lim; v3++) { val[3] = v3;
  for (v4 = 0; v4 < v4_lim; v4++) { val[4] = v4;
  for (v5 = 0; v5 < v5_lim; v5++) { val[5] = v5;
  for (v6 = 0; v6 < v6_lim; v6++) { val[6] = v6;
  for (v7 = 0; v7 < v7_lim; v7++) { val[7] = v7;
  for (v8 = 0; v8 < v8_lim; v8++) { val[8] = v8;
  for (v9 = 0; v9 < v9_lim; v9++) { val[9] = v9;
  for (v10 = 0; v10 < v10_lim; v10++) { val[10] = v10;
  for (v11 = 0; v11 < v11_lim; v11++) { val[11] = v11;

  for (i = 0; i < num_lits; i++) {
    s = Symbols + lits[i].functor;

    switch (s->arity) {  /* note no breaks */
    case 4:
      a3 = lits[i].a[3];
      a3 = (a3 < 0 ? -(a3+100) : val[a3]);
    case 3:
      a2 = lits[i].a[2];
      a2 = (a2 < 0 ? -(a2+100) : val[a2]);
    case 2:
      a1 = lits[i].a[1];
      a1 = (a1 < 0 ? -(a1+100) : val[a1]);
    case 1:
      a0 = lits[i].a[0];
      a0 = (a0 < 0 ? -(a0+100) : val[a0]);
    }

#if 0
    if (num_lits > 2 &&
	a0 < a1 &&
	TP_BIT(s->properties, MACE_COMMUTATIVE_BIT)) {
      int tmp = a0; a0 = a1; a1 = tmp;
    }
#endif

    sign = lits[i].sign;
    switch (s->arity) {
    case 0: prop_clause[i] = sign * ATOM0(s); break;
    case 1: prop_clause[i] = sign * ATOM1(s,a0); break;
    case 2: prop_clause[i] = sign * ATOM2(s,a0,a1); break;
    case 3: prop_clause[i] = sign * ATOM3(s,a0,a1,a2); break;
    case 4: prop_clause[i] = sign * ATOM4(s,a0,a1,a2,a3); break;
    }
  }

  /* decode_and_p_clause(prop_clause, num_lits); */
  add_clause(prop_clause, num_lits);

  }}}}}}}}}}}}

}  /* generate_prop_clauses */

/*************
 *
 *   add_clauses_for_qg_constraint()
 *
 *************/

static void add_clauses_for_qg_constraint(void)
{
  int i, k, id, n;
  struct symbol *s;

  n = Sorts[0].n;

  /* quasigroup constraint:  x*n != z, when z < x-1. */

  id = str_to_id("f");
  if (id == -1)
    abend("add_clauses_for_qg_constraint, symbol f not found");

  s = Symbols+id;

  printf("Adding basic QG constraints on last column of f.\n");

  for (i = 0; i < n; i++) {
    for (k = 0; k < i-1; k++) {
      add_unit(-1, s, i, n-1, k, -1);
    }
  }
}  /* add_clauses_for_qg_constraint */

/*************
 *
 *   number_variables()
 *
 *   Given a first-order (flat) clause with string arguments
 *   (vars and domain elements), assign vars positive ints,
 *   and domain elements negative ints.
 *
 *   Return the maximum domain element seen (-1 if none).
 *
 *************/

static int number_variables(struct rel_clause *c)
{
  char *vnames[MAX_MVARS], *s;

  int i, j, k, arity, el;
  int max_element = -1;

  for (i = 0; i < MAX_MVARS; i++)
    vnames[i] = NULL;

  for (i = 0; i < c->num_lits; i++) {
    arity = Symbols[c->lits[i].functor].arity;
    for (j = 0; j < arity; j++) {
      s = c->lits[i].ai[j];
      if (str_int(s, &el)) {  /* Domain element */
	if (el < 0 || el >= Symbols[c->lits[i].functor].args[j].n) {
	  printf("\nThe bad element is %d.\n", el);
	  MACE_abend("number_variables, element out of range");
	}
	c->lits[i].a[j] = -(el+100);
	if (el > max_element)
	  max_element = el;
      }
      else {  /* Variable */
	for (k = 0; k < MAX_MVARS && vnames[k] && !str_ident(s,vnames[k]); k++);
	if (k == MAX_MVARS) {
	  printf("\nMaximum number of variables is %d.\n", MAX_MVARS);
	  MACE_abend("number_variables, too many variables");
	}
	else {
	  c->lits[i].a[j] = k;
	  if (vnames[k] == NULL)
	    vnames[k] = s;
	}
      }
    }
  }
  return(max_element);
}  /* number_variables */

/*************
 *
 *   trans_relational_clause()
 *
 *************/

static int trans_relational_clause(struct clause *cin)
{
  struct literal *lit;
  struct rel_clause *c;
  struct rel_lit *l;
  int i, j, arity, fid;
  char *functor;
  struct symbol *s = NULL;
  int max_element;

  /* printf("translating: "); p_clause(cin); */

  if (Num_rel_clauses == MAX_CLAUSES)
    MACE_abend("trans_relational_clause: too many first_order clauses");
  c = Clauses + Num_rel_clauses;
  Num_rel_clauses++;

  j = 0;  /* literal number */
  for (lit = cin->first_lit; lit; lit = lit->next_lit) {
    struct term *a = lit->atom;
    struct rel *r;
    l = c->lits + j;
    l->sign = (lit->sign ? 1 : -1);

    if (is_eq(a->sym_num) &&
	a->farg->argval->type != VARIABLE &&
	!domain_element(a->farg->argval)) {
      /* f(t1,...,tn) = t4 */
      functor = sn_to_str(a->farg->argval->sym_num);
      i = 0;
      /* Do the args of the function symbol. */
      for (r = a->farg->argval->farg; r; r = r->narg) {
	if (r->argval->type == VARIABLE)
	  sprintf(l->ai[i], "v%d", r->argval->varnum);
	else if (r->argval->type == NAME)
	  sprintf(l->ai[i], "%s", sn_to_str(r->argval->sym_num));
	else
	  MACE_abend("trans_relational_clause: bad DP arg");
	i++;
      }
      /* Do the right-hand arg. */
      if (a->farg->narg->argval->type == VARIABLE)
	sprintf(l->ai[i], "v%d", a->farg->narg->argval->varnum);
      else if (a->farg->narg->argval->type == NAME)
	sprintf(l->ai[i], "%s", sn_to_str(a->farg->narg->argval->sym_num));
      else
	MACE_abend("trans_relational_clause: bad DP arg (right)");
    }
    else {  /* t1 = t2  or p(t1,...,tn) */
      functor = sn_to_str(a->sym_num);
      i = 0;
      for (r = a->farg; r; r = r->narg) {
	if (r->argval->type == VARIABLE)
	  sprintf(l->ai[i], "v%d", r->argval->varnum);
	else if (domain_element(r->argval))
	  sprintf(l->ai[i], "%s", sn_to_str(r->argval->sym_num));
	else
	  MACE_abend("trans_relational_clause: bad eq arg");
	i++;
      }
    }

    fid = str_to_id(functor);
    if (fid == -1) {
      printf("Bad name=%s\n", functor);
      MACE_abend("trans_relational_clause, bad functor");
    }
    s = Symbols + fid;
    arity = s->arity;
    l->functor = fid;

    j++;  /* move to next literal */
  }

  c->num_lits = j;
  max_element = number_variables(c);
    
#ifdef PRINT_CLAUSES
  p_relational_clause(c);
#endif
  return(max_element);
}  /* trans_relational_clause */

/*************
 *
 *   remove_ans_literals()
 *
 *************/

static void remove_ans_literals(struct list *lst)
{
  struct clause *c;

  for (c = Usable->first_cl; c; c = c->next_cl) {
    if (num_answers(c) > 0) {
      struct literal *lit, *prev;
      
      printf("Removing answer literals from clause %d.\n", c->id);

      for (lit=c->first_lit, prev=NULL; lit; prev=lit, lit=lit->next_lit) {
	if (answer_lit(lit)) {
	  if (prev == NULL)
	    c->first_lit = lit->next_lit;
	  else
	    prev->next_lit = lit->next_lit;
	  /* don't bother to deallocate lit */
	}
      }
    }
  }
}  /* remove_ans_literals */

/*************
 *
 *    read_all_mace_input()
 *
 *    This is like Otter's read_all_input(), except that it
 *    doesn't do the processining (in particular process_input)
 *    after read_a_file().
 *
 *************/

void read_all_mace_input(int argc,
			 char **argv)
{
  Usable = get_list();
  Sos = get_list();
  Demodulators = get_list();
  Passive = get_list();
  Hot = get_list();
  Hints = get_list();
  Hints2 = get_list();
  Mace_constraints = get_list();

  CLOCK_START(INPUT_TIME);
  read_a_file(stdin, stdout);  /* Otter's routine */

  /* Move Sos, Demodulators, and Passive into Usable */

  append_lists(Usable,Sos);
  append_lists(Usable,Demodulators);
  append_lists(Usable,Passive);

  if (Flags[PROCESS_INPUT].val) {
    /* Integrate clauses so that they get IDs. */
    struct clause *c;
    for (c = Usable->first_cl; c; c = c->next_cl)
      cl_integrate(c);
    printf("\nlist(clauses).\n");
    print_cl_list(stdout, Usable);
  }
    
  remove_ans_literals(Usable);

  CLOCK_STOP(INPUT_TIME);

  fflush(stdout);

}  /* read_all_mace_input */

/*************
 *
 *   MACE_collect_symbols()
 *
 *************/

static void MACE_collect_symbols(struct term *t,
				 int type)
{
  if (t->type == VARIABLE || domain_element(t))
    return;
  else {
    char *name = sn_to_str(t->sym_num);
    int arity = sn_to_arity(t->sym_num) + (type == FUNCTION ? 1 : 0);
    int id = str_to_id(name);
    struct rel *r;

    if (id != -1) {
      /* We have already collected the symbol. */
      struct symbol *s = Symbols + id;
      if (s->arity != arity)
	abend("MACE_collect_symbols, multiple arity");
    }
    else {
      /* New symbol. */
      int j;
      char arg_types[MAX_ARITY][MAX_MNAME];
      struct symbol *s;

      if (arity > MAX_ARITY)
	MACE_abend("arity too great");

      for (j = 0; j < arity; j++)
	strcpy(arg_types[j], "univ");

      id = declare_symbol_sorted(name, arity, arg_types);
      s = Symbols + id;

      s->type = type;  /* FUNCTION or RELATION */
      if (type == RELATION) {
	if (is_eq(t->sym_num))
	  SET_BIT(s->properties, MACE_EQ_BIT);
	else if (t->sym_num == str_to_sn("<", 2))
	  SET_BIT(s->properties, MACE_ORDER_BIT);
      }
    }
    for (r = t->farg; r; r = r->narg)
      MACE_collect_symbols(r->argval, FUNCTION);
  }
}  /* MACE_collect_symbols */

/*************
 *
 *   iso_constants_recurse()
 *
 *************/

static void iso_constants_recurse(struct symbol **c, /* constants */
				  int clause[],      /* for building clauses */
				  int lits,          /* current # of lits */
				  int tempclause[],
				  int max_element,   /* max on current path */
				  int domain_size)
/* For 3 constants, e a b:
 *  e0
 * -e0  a0  a1
 * -e0 -a0  b0  b1
 * -e0 -a1  b0  b1  b2
 */

{
  if (*c == NULL)
    return;
  else if (max_element + 2 >= domain_size)
    return;
  else {
    int i, j, n;
    n = lits;
    for (i = 0; i <= max_element+1; i++)
      clause[n++] = ATOM1((*c), i);
#if 0
    printf(">>>> "); decode_and_p_clause(clause,n);
#endif
    for (j = 0; j < n; j++)
      tempclause[j] = clause[j];
    add_clause(tempclause, n);

    for (i = 0; i <= max_element+1; i++) {
      clause[lits] = - ATOM1((*c), i);
      iso_constants_recurse(c+1,clause,lits+1,tempclause,
			    MAX(max_element,i), domain_size);
    }
  }
}  /* iso_constants_recurse */

/*************
 *
 *   iso_constants()
 *
 *************/

static void iso_constants(int domain_size, int max_element)
{
  int id;
  int num_constants = 0;
  struct symbol *s, *constants[MAX_SYMBOLS];
  int clause[MAX_SYMBOLS];
  int tempclause[MAX_SYMBOLS];  /* because add_clause changes the clause! */
  for (id = 0; id < Num_symbols; id++) {
    s = Symbols + id;
    if (s->type == FUNCTION && s->arity == 1 && s->assigned_value == -1) {
      constants[num_constants++] = s;
    }
  }
  
  num_constants = MIN(num_constants, MACE_Parms[ISO_CONSTANTS].val);
  constants[num_constants] = NULL;

  if (num_constants > 0) {
    int i, n;
    printf("Applying isomorphism constraints to constants:");
    fflush(stdout);
    for (i = 0; i < num_constants; i++) {
      printf(" %s", constants[i]->name);
    }
    printf(".\n");
    fflush(stdout);
    n = MACE_Stats[INPUT_CLAUSES];
    iso_constants_recurse(constants, clause, 0, tempclause,
			  max_element, domain_size);
  }
}  /* iso_constants */

/*************
 *
 *   dp_trans()
 *
 *   This is copied from Otter's dp_transform(), then modified.
 *   Instead of writing symbols, transformed clauses, and assignments to
 *   a file, it puts that data directly into the MACE data structures.
 *
 *   If unsatisfiability is detected, return 0; else return 1.
 *
 *************/

int dp_trans(int domain_size, int distinct_constants, int qg_constraint)
{
  struct clause *c, *d;
  struct term *a;
  struct rel *r;
  struct literal *lit;
  int i, k;
  int max_theory_element = -1;      /* in ordinary clauses */
  int max_constrained_element = -1;    /* in mace_constraints */
  int id;

  strcpy(Sorts[0].name, "univ");
  Sorts[0].n = domain_size;
  Num_sorts = 1;

  Parms[STATS_LEVEL].val = 0;

  /* Go through all of the clauses, collecting names/arities of
     all the symbols (function, relation, constant).
  */

  for (c = Usable->first_cl; c; c = c->next_cl) {
    for (lit = c->first_lit; lit; lit = lit->next_lit) {
      MACE_collect_symbols(lit->atom, RELATION);
    }
  }

  /* Get additional constraints from the Mace_constraints list. */

  for (c = Mace_constraints->first_cl; c; c = c->next_cl) {
    int id;
    struct symbol *s;
    a = c->first_lit->atom;
    if (sn_to_arity(a->sym_num) != 2)
      MACE_abend("dp_trans, wrong arity in mace_constraints list.");

    id = str_to_id(sn_to_str(a->farg->argval->sym_num));

    if (id == -1)
      MACE_abend("dp_trans, symbol in mace_constraints list not found");

    s = Symbols + id;

    if (is_symbol(a, "property", 2)) {
      char *prop = sn_to_str(a->farg->narg->argval->sym_num);
      if (str_ident(prop, "equality"))
	SET_BIT(s->properties, MACE_EQ_BIT);
      else if (str_ident(prop, "order"))
	SET_BIT(s->properties, MACE_ORDER_BIT);
      else if (str_ident(prop, "quasigroup"))
	SET_BIT(s->properties, MACE_QUASIGROUP_BIT);
      else if (str_ident(prop, "bijection"))
	SET_BIT(s->properties, MACE_BIJECTION_BIT);
      else if (str_ident(prop, "commutative"))
	SET_BIT(s->properties, MACE_COMMUTATIVE_BIT);
      else
	MACE_abend("dp_trans, mace_constrationts property not understood");
    }
    else if (!is_symbol(a, "assign", 2))  /* handle assigns below */
      MACE_abend("dp_trans, mace_constraints command not understood");
  }

#if 0
  /* Print data for each symbol. */
  for (i = 0; i < Num_symbols; i++) {
    struct symbol *s = Symbols + i;
    printf("Symbol %2d, %s, %6s/%d, ",
	   i, (s->type == FUNCTION ? "function" : "relation"),
	   s->name, s->arity);
    printf("atoms %4d --%4d, %10d.\n", s->base, s->end, s->properties);
  }
#endif

  /* Now we know how many atoms there are (determined by symbols/arities
     and domain-size).  So let's set up the Units array, which is used
     for storing assignments to atoms.
  */

  Greatest_atom = Next_base-1;
  i = Greatest_atom + 1;
  Units = malloc(i * sizeof(int));
  MACE_Stats[MEM_MALLOCED] += i;
  for (i = 1; i <= Greatest_atom; i++)
    Units[i] = 0;
  
  /*****************************************************************/

  /* Go through the clauses, making them into relational clauses,
     and storing the flat clauses on a stack (Clauses).
  */

  for (c = Usable->first_cl; c; c = c->next_cl) {
    i = trans_relational_clause(c);
    if (i > max_theory_element)
      max_theory_element = i;
  }

  /* process Mace_constraints */

  for (c = Mace_constraints->first_cl; c; c = c->next_cl) {
    a = c->first_lit->atom;

    if (is_symbol(a, "assign", 2)) {
      int arity;  /* includes extra arg for functions */
      struct symbol *s;
      int sign;  /* -1 for negative, +1 for positive */
      int relation;
      int arg[MAX_ARITY];
      char *str, *functor;
      int func_val;
      int id;

      functor = sn_to_str(a->farg->argval->sym_num);
      id = str_to_id(functor);
      
      if (id == -1) {
	printf("Bad name=%s, \n", functor);
	MACE_abend("dp_trans, bad functor");
      }
	    
      s = Symbols + id;
      arity = s->arity;

      for (i = 0; i < MAX_ARITY; i++)
	arg[i] = -1;

      relation = (s->type == RELATION);

      if (!relation) {
	if (!str_int(sn_to_str(a->farg->narg->argval->sym_num), &func_val))
	  abend("dp_trans, bad integer (RHS) in assignment");
	arg[arity-1] = func_val;
	sign = 1;
      }
      else {
	str = sn_to_str(a->farg->narg->argval->sym_num);
	if (!str_ident(str, "T") && !str_ident(str, "F"))
	  abend("dp_trans, bad truth value in assignment");
	sign = str_ident(str, "F") ? -1 : 1;
      }

      i = 0;
      for (r = a->farg->argval->farg; r; r = r->narg) {
	if (!str_int(sn_to_str(r->argval->sym_num), &k))
	  abend("dp_trans, bad integer (LHS) in assignment");
	arg[i++] = k;
      }

      printf("Adding unit assigning %s%s", sign == -1 ? "-" : "", functor);
      for (i = 0; i < s->arity; i++)
	printf(" %d", arg[i]);
      printf("\n");
      
      add_unit(sign, s, arg[0], arg[1], arg[2], arg[3]);
      if (Unsatisfiable)
	return 0;

      if (!relation && s->arity == 1) {
	s->assigned_value = arg[0];
	if (arg[0] > max_constrained_element)
	  max_constrained_element = arg[0];
      }
    }
  }

  /* printf("\n%% =======END OF DP INPUT=======.\n"); */

  if (!distinct_constants) {
    iso_constants(domain_size, max_theory_element);
  }
  else {
    struct symbol *s;
    int n = max_theory_element;
    for (id = 0; id < Num_symbols; id++) {
      s = Symbols + id;
      if (s->type == FUNCTION &&
	  s->arity == 1 &&
	  s->assigned_value == -1) {
	n++;
	if (n >= domain_size)
	  printf("Cannot make constant %s distinct, domain is too small.\n",
		 s->name);
	else {
	  printf("Adding unit assigning %s %d (distinct_constants).\n",
		 s->name, n);
	  add_unit(1, s, n, -1, -1, -1);
	  if (Unsatisfiable)
	    return 0;
	}
      }
    }
  }  /* distinct_constants */

  /* Unit assignments for order, equality relations, if present. */
	
  for (id = 0; id < Num_symbols; id++) {
    struct symbol *s = NULL;
    s = Symbols + id;
    if (s->type == RELATION && s->arity == 2) {
      if (TP_BIT(s->properties, MACE_ORDER_BIT)) {
	int n, j;
	if (s->args[0].sort != s->args[1].sort)
	  MACE_abend("order relation must have args of same type");
	n = s->args[0].n;
#if 0
	printf("Adding units for order relation %s.\n", s->name);
#endif
	for (i = 0; i < n; i++) {
	  for (j = 0; j < n; j++) {
	    if (i < j)
	      add_unit(1, s, i, j, -1, -1);
	    else
	      add_unit(-1, s, i, j, -1, -1);
	  }
	}
      }
      else if (TP_BIT(s->properties, MACE_EQ_BIT)) {
	int n, j;
	if (s->args[0].sort != s->args[1].sort)
	  MACE_abend("equality relation must have args of same type");
	n = s->args[0].n;
#if 0
	printf("Adding units for equality relation %s.\n", s->name);
#endif
	for (i = 0; i < n; i++) {
	  for (j = 0; j < n; j++) {
	    if (i == j)
	      add_unit(1, s, i, j, -1, -1);
	    else
	      add_unit(-1, s, i, j, -1, -1);
	  }
	}
      }
    }
    if (Unsatisfiable)
      return 0;
  }  /* */

  if (qg_constraint)
    add_clauses_for_qg_constraint();
  if (Unsatisfiable)
    return 0;

  /* When processing generated clauses, forward unit subsumption and
   * forward unit deletion are applied, but the backward operations are not.
   * So to save memory, it's best to carefully order the generation of
   * clauses.  Because of unit-deletion, it's not clear which way is best.
   */

  for (i = 0; i < Num_rel_clauses; i++) {
    struct rel_clause *c = Clauses + i;
    if (c->num_lits <= 2) {
#if 0
      printf("Instantiating clause: ");
      p_relational_clause(c);
#endif
      generate_prop_clauses(c);
      exit_if_over_time_limit();
      if (Unsatisfiable)
	return 0;
    }
  }

  /* Well-defined, closed, quasigroup, bijection. */
	
  for (i = 0; i < Num_symbols; i++) {
  struct symbol *s = Symbols + i;
  if (s->type == FUNCTION) {
#if 0
      printf("Function %s well-defined and closed.\n", s->name);
#endif
      well_defined(s);
      closed(s);
      if (s->arity == 2 && TP_BIT(s->properties, MACE_BIJECTION_BIT)) {
	printf("function %s bijection.\n", s->name);
	one_one(s);
	onto(s);
      }
      else if (s->arity == 3 && TP_BIT(s->properties, MACE_QUASIGROUP_BIT)) {
	printf("function %s quasigroup.\n", s->name);
	left_cancel(s);
	right_cancel(s);
	left_onto(s);
	right_onto(s);
      }
    }
  if (Unsatisfiable)
    return 0;
  }

  for (i = 0; i < Num_rel_clauses; i++) {
    struct rel_clause *c = Clauses + i;
    if (c->num_lits > 2) {
#if 0
      printf("Instantiating clause: ");
      p_relational_clause(c);
#endif
      generate_prop_clauses(c);
      exit_if_over_time_limit();
      if (Unsatisfiable)
	return 0;
    }
  }

  printf("\n%d clauses were generated; %d of those survived the first stage\n"
	 "of unit preprocessing; there are %d atoms.\n", 
	 Clauses_generated, Clauses_inserted, Greatest_atom);
	   
  fflush(stdout);

  return 1;  /* Unsatisfiability not detected */

}  /* dp_trans */

/*************
 *
 *    reinit_generate
 *
 *    Free memory and reinitialize global variables in this file.
 *
 *************/

void reinit_generate(void)
{
  free(Units);

  Num_sorts = 0;
  Next_base = 1;
  Num_symbols = 0;
  Clauses_generated = 0;
  Clauses_inserted = 0;
  Num_rel_clauses = 0;

  Units = NULL;
  Unsatisfiable = 0;

}  /* reinit_generate */

