#include "Mace2.h"

#define MAX_LITS 100

/*************
 *
 *   num_pos_lits()
 *
 *************/

static int num_pos_lits(struct clause *c)
{
  struct literal *lit;
  int n = 0;
  for (lit = c->first_lit; lit; lit = lit->next_lit)
    if (lit->sign)
      n++;
  return n;
}  /* num_pos_lits */

/*************
 *
 *   copy_literal()
 *
 *************/

static struct literal *copy_literal(struct literal *l1)
{
  struct literal *l2 = get_literal();
  l2->sign = l1->sign;
  l2->atom = copy_term(l1->atom);
  return l2;
}  /* copy_literal */

/*************
 *
 *   append_literal()
 *
 *************/

static void append_literal(struct clause *c, struct literal *lit)
{
  if (c->first_lit == NULL)
    c->first_lit = lit;
  else {
    struct literal *l2 = c->first_lit;
    while (l2->next_lit != NULL)
      l2 = l2->next_lit;
    l2->next_lit = lit;
  }
}  /* append_literal */

/*************
 *
 *   append_subterm()
 *
 *************/

static void append_subterm(struct term *t, struct term *s)
{
  struct rel *r = get_rel();
  r->argval = s;

  if (t->farg == NULL)
    t->farg = r;
  else {
    struct rel *r2 = t->farg;
    while (r2->narg != NULL)
      r2 = r2->narg;
    r2->narg = r;
  }
}  /* append_subterm */

/*************
 *
 *   p_vars()
 *
 *************/

static void p_vars(int *vars, int n)
{
  int i;
  for (i = 0; i < n; i++)
    if (vars[i])
      printf(" v%d", i);
  printf("\n");
}  /* p_vars */

/*************
 *
 *   intersect_vars()
 *
 *************/

static void intersect_vars(int *v1,
			   int *v2,
			   int n,
			   int *c)
{
  int i;
  for (i = 0; i < n; i++)
    c[i] = v1[i] && v2[i];
}  /* intersect_vars */

/*************
 *
 *   union_vars()
 *
 *************/

static void union_vars(int *v1,
			   int *v2,
			   int n,
			   int *c)
{
  int i;
  for (i = 0; i < n; i++)
    c[i] = v1[i] || v2[i];
}  /* union_vars */

/*************
 *
 *   cardinality()
 *
 *************/

static int cardinality(int *a, int n)
{
  int card = 0;
  int i;
  for (i = 0; i < n; i++)
    if (a[i])
      card++;
  return card;
}  /* cardinality */

/*************
 *
 *   diff_cardinality()
 *
 *************/

static int diff_cardinality(int *a, int *b, int n)
{
  int diff = 0;
  int i;
  for (i = 0; i < n; i++)
    if (a[i] && !b[i])
      diff++;
  return diff;
}  /* diff_cardinality */

/*************
 *
 *   which_vars_term()
 *
 *************/

static void which_vars_term(int *vars, struct term *t)
{
  if (t->type == VARIABLE)
    vars[t->varnum] = 1;
  else {
    struct rel *r;
    for (r = t->farg; r; r = r->narg) {
      which_vars_term(vars, r->argval);
    }
  }
}  /* which_vars_term */

/*************
 *
 *   which_vars()
 *
 *************/

static void which_vars(int *vars, struct clause *c)
{
  struct literal *lit;
  for (lit = c->first_lit; lit; lit = lit->next_lit)
    which_vars_term(vars, lit->atom);
}  /* which_vars */

/*************
 *
 *   which_vars_part()
 *
 *  which_vars_part(vars_1, c, p, n, 0);
 *
 *************/

static void which_vars_part(int *vars,
			    struct clause *c,
			    int *part,
			    int to_check)
{
  struct literal *lit;
  int i;
  for (lit = c->first_lit, i = 0; lit; lit = lit->next_lit, i++) {
    if (part[i] == to_check)
      which_vars_term(vars, lit->atom);
  }
}  /* which_vars_part */

/*************
 *
 *   gen_connector_symnum()
 *
 *************/

static int gen_connector_symnum(int arity)
{
  static int Count;
  char s1[MAX_NAME], s2[MAX_NAME];
  int symnum;

  int_str(++Count, s1);
  cat_str("$Connect", s1, s2); 

  /* str_to_sn knows it's ok to use a symbol starting with $Connect */

  symnum = str_to_sn(s2, arity);
  return symnum;
}  /* gen_connector_symnum */

/*************
 *
 *   connector()
 *
 *************/

static struct literal *connector(int *vars, int n)
{
  struct literal *lit;
  struct term *t = get_term();
  int i;
  int arity = 0;

  for (i = 0; i < n; i++) {
    if (vars[i]) {
      struct term *var = get_term();
      var->type = VARIABLE;
      var->varnum = i;
      append_subterm(t, var);
      arity++;
    }
  }
  t->sym_num = gen_connector_symnum(arity);
  t->type = (arity == 0 ? NAME : COMPLEX);

  lit = get_literal();
  lit->sign = 1;
  lit->atom = t;
  return lit;
}  /* connector */

/*************
 *
 *   score_the_part()
 *
 *************/

static void score_the_part(int *p,
			   int n,
			   struct clause *c,
			   int *bp,
			   int *bs)
{
  int vars_1[MAX_VARS], vars_2[MAX_VARS];
  int vars_union[MAX_VARS], vars_intersection[MAX_VARS];
  int nvars = biggest_var_clause(c) + 1;
  int i, card_intersection;
  
  for (i = 0; i < MAX_VARS; i++)
    vars_1[i] = vars_2[i] = 0;

  which_vars_part(vars_1, c, p, 0);
  which_vars_part(vars_2, c, p, 1);

  union_vars(vars_1, vars_2, nvars, vars_union);
  intersect_vars(vars_1, vars_2, nvars, vars_intersection);
  card_intersection = cardinality(vars_intersection, nvars);

  if (card_intersection <= MAX_ARITY) {
    int c1 = cardinality(vars_1, nvars);
    int c2 = cardinality(vars_2, nvars);

    int diff1 = diff_cardinality(vars_union, vars_1, nvars);
    int diff2 = diff_cardinality(vars_union, vars_2, nvars);

    if (diff1 > 0 && diff2 > 0) {
      /* Each has a variable not in the other, so we calculate a score.

	 Primary sort: fewest variables (in the clause with more variables).
	 Secondary sort: smallest intersection.
      */
      int score = (MAX(c1,c2) * 100) + card_intersection;
      if (score < *bs) {
	*bs = score;
	for (i = 0; i < n; i++)
	  bp[i] = p[i];
      }
    }
  }
}  /* score_the_part */

/*************
 *
 *   parts()
 *
 *   This goes through the 2^n ways to (binary) partition the clause.
 *   It scores each partition, remembering the best score and partition.
 *
 *************/

static void parts(int i,
		  int *p,
		  int n,
		  struct clause *c,
		  int *bp,
		  int *bs)
{
  if (i == n)
    score_the_part(p, n, c, bp, bs);
  else {
    p[i] = 0;
    parts(i+1, p, n, c, bp, bs);
    p[i] = 1;
    parts(i+1, p, n, c, bp, bs);
  }
}  /* parts */

/*************
 *
 *   try_to_part()
 *
 *************/

static void try_to_part(struct clause *c,
			struct clause **p1,
			struct clause **p2,
			int part_vars)
{
  int nvars = biggest_var_clause(c) + 1;  /* assume variables normalized */
  *p1 = *p2 = NULL;

  if (nvars >= part_vars) {
    int p[MAX_LITS];
    int bp[MAX_LITS];     /* best partition */
    int bs = INT_MAX;     /* best score */
    int n = num_literals(c);
    int i;

    for (i = 0; i < n; i++)
      bp[i] = p[i] = 0;

    parts(0, p, n, c, bp, &bs);

    if (bs != INT_MAX) {
      /* We shall part the clause! */
      struct clause *n1 = get_clause();
      struct clause *n2 = get_clause();
      n1->id = c->id;  /* This is questionalble. */
      n2->id = c->id;  /* This is questionalble. */
      for (i = 0; i < n; i++) {
	struct literal *lit = copy_literal(ith_literal(c, i+1));
	if (bp[i] == 0)
	  append_literal(n1, lit);
	else
	  append_literal(n2, lit);
      }
      {
	struct literal *lit1;
	struct literal *lit2;

	int vars_1[MAX_VARS], vars_2[MAX_VARS], c_vars[MAX_VARS];

	for (i = 0; i < MAX_VARS; i++)
	  vars_1[i] = vars_2[i] = c_vars[i] = 0;

	which_vars(vars_1, n1);
	which_vars(vars_2, n2);
	intersect_vars(vars_1, vars_2, nvars, c_vars);

	lit1 = connector(c_vars, nvars);
	lit2 = copy_literal(lit1);

	/* It's very important to keep the number of non-Horn clauses
	   small, because we have to scan them when selecting variables.
	   In general it seems important to minimize the number
	   positive literals in each clause. */

	if (num_pos_lits(n1) < num_pos_lits(n2))
	  { lit1->sign = 1; lit2->sign = 0; }
	else
	  { lit1->sign = 0; lit2->sign = 1; }
	
	append_literal(n1, lit1);
	append_literal(n2, lit2);

#if 0
	printf("\nParting clause: "); p_clause(c);
	printf("Part1:          "); p_clause(n1);
	printf("Part2:          "); p_clause(n2);
#endif

	renumber_vars(n1);
	renumber_vars(n2);
      }
      *p1 = n1;
      *p2 = n2;
    }
  }
}  /* try_to_part */

/*************
 *
 *   var_opt()
 *
 *************/

static struct list *var_opt(struct clause *c, int part_vars)
{
  struct clause *p1, *p2;
  try_to_part(c, &p1, &p2, part_vars);

  if (p1 == NULL) {
    struct list *l = get_list();
    append_cl(l, c);
    return l;
  }
  else {
    struct list *l1 = var_opt(p1, part_vars);
    struct list *l2 = var_opt(p2, part_vars);
    append_lists(l1, l2);
    return l1;
  }
}  /* var_opt */

/*************
 *
 *   variable_optimize()
 *
 *************/

struct list *variable_optimize(struct list *l, int part_vars)
{
  struct list *nl = get_list();
  struct clause *c;

  for (c = l->first_cl; c; c = c->next_cl) {
    struct list *ol;
    struct clause *d = cl_copy(c);
    d->id = c->id;
    ol = var_opt(d, part_vars);
    append_lists(nl, ol);
  }

  return nl;
}  /* variable_optimize */
