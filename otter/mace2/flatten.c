#include "Mace2.h"

/*************
 *
 *   kludgey_e_subsume(c, d)
 *
 *   Does c subsume d (mod equality flipping)?  This works by
 *   inserting flips into (a copy of) d then doing an ordinary
 *   subsumption check.
 *
 *************/

int kludgey_e_subsume(struct clause *c,
		      struct clause *d)
{
  struct clause *d1;
  struct literal *l1, *l2;
  struct term *t;
  int rc;
    
  d1 = cl_copy(d);
  for (l1 = d1->first_lit; l1; l1 = l1->next_lit) {
    if (is_eq(l1->atom->sym_num)) {
      l2 = get_literal();
      l2->next_lit = d1->first_lit;
      d1->first_lit = l2;
      l2->sign = l1->sign;
      l2->atom = copy_term(l1->atom);
      t = l2->atom->farg->argval;
      l2->atom->farg->argval = l2->atom->farg->narg->argval;
      l2->atom->farg->narg->argval = t;
    }
  }
  rc = subsume(c, d1);
  cl_del_non(d1);
  return(rc);
}  /* kludgey_e_subsume */

/*************
 *
 *   check_for_bad_things()
 *
 *************/

void check_for_bad_things(struct clause *c)
{
  struct literal *lit;
  for (lit = c->first_lit; lit; lit = lit->next_lit) {
    if (answer_lit(lit))
      abend("answer literals not allowed in MACE input");
    if (lit->atom->varnum == EVALUABLE)
      abend("evaluable literals not allowed in MACE input");
  }
}  /* check_for_bad_things */

/*************
 *
 *   int_term(t)
 *
 *   Is t a constant with a symbol representing a positive integer?
 *
 *************/

int int_term(struct term *t)
{
  int i;
  return(t->type == NAME && str_int(sn_to_str(t->sym_num),&i) && i>=0);
}  /* int_term */

/*************
 *
 *   domain_element(t)
 *
 *   Is t a domain element?
 *
 *************/

int domain_element(struct term *t)
{
  return(int_term(t));
}  /* int_term */

/*************
 *
 *    build_binary_term
 *
 *************/

static struct term *build_binary_term(int sn,
				      struct term *t1,
				      struct term *t2)
{
  struct term *t;
  struct rel *r1, *r2;

  t = get_term(); t->type = COMPLEX; t->sym_num = sn;
  r1 = get_rel(); r1->argval = t1;
  r2 = get_rel(); r2->argval = t2;
  t->farg = r1; r1->narg = r2;
  return(t);
}  /* build_binary_term */

/*************
 *
 *   replace_term()
 *
 *************/

static struct term *replace_term(struct term *t,
				 struct term *ct,
				 struct term *vt)
{
  if (term_ident(t, ct))
    return(copy_term(vt));
  else {
    struct rel *r;
    for (r = t->farg; r; r= r->narg)
      r->argval = replace_term(r->argval, ct, vt);
    return(t);
  }
}  /* replace_term */

/*************
 *
 *   process_negative_equalities()
 *
 *   Simplify literals x!=t, where x does not occur in t, by,
 *   in effect, rewolving with x=x.
 *
 *************/

void process_negative_equalities(struct clause *c)
{
  struct literal *curr, *prev, *l;
  struct term *t1, *t2, *v, *t;

  t = NULL;  /* to quiet -Wall */

  curr = c->first_lit;
  prev = NULL;
  while (curr) {
    if (is_eq(curr->atom->sym_num) && !curr->sign) {
      t1 = curr->atom->farg->argval;
      t2 = curr->atom->farg->narg->argval;
      v = NULL;
      if (t1->type == VARIABLE && !occurs_in(t1, t2)) {
	v = t1; t = t2;
      }
      else if (t2->type == VARIABLE && !occurs_in(t2, t1)) {
	v = t2; t = t1;
      }

      if (v) {
	if (prev)
	  prev->next_lit = curr->next_lit;
	else
	  c->first_lit = curr->next_lit;
	for (l = c->first_lit; l; l = l->next_lit)
	  l->atom = replace_term(l->atom, v, t);
      }
      else
	prev = curr;
    }
    else
      prev = curr; 

    curr = curr->next_lit;
  }
}  /* process_negative_equalities */

/****************************************************************************/
/****************************************************************************/

/*************
 *
 *   yankable_term()
 *
 *************/

static struct term *yankable_term(struct term *t, int check)
{
  if (check && t->type != VARIABLE && !domain_element(t))
    return t;
  else {
    struct rel *r;
    struct term *t1;
    for (r = t->farg; r; r = r->narg) {
      t1 = yankable_term(r->argval, 1);
      if (t1 != NULL)
	return t1;
    }
    return NULL;
  }
}  /* yankable_term */

/*************
 *
 *   yankable()
 *
 *   Given a clause, return the first yankable term.
 *
 *   A term is yankable if it is not a variable, domain element,
 *   or the left side of an equality.
 *
 *************/

static struct term *yankable(struct clause *c)
{
  struct literal *lit;
  struct term *t = NULL;

  for (lit = c->first_lit; lit; lit = lit->next_lit) {
    struct term *a = lit->atom;
    if (!is_eq(a->sym_num))
      t = yankable_term(a, 0);
    else {
      t = yankable_term(a->farg->argval, 0);
      if (t == NULL)
	t = yankable_term(a->farg->narg->argval, 1);
    }
    if (t != NULL)
      return t;
  }
  return NULL;
}  /* yankable */

/*************
 *
 *   eq_sn() -- return a sym_num for equality
 *
 *************/

static int eq_sn()
{
  if (Flags[TPTP_EQ].val)
    return str_to_sn("equal", 2);
  else
    return str_to_sn("=", 2);
}  /* eq_sn */

/*************
 *
 *   flatten_clause()
 *
 *   Apply this kind of transformation
 *
 *     p(a)    ========>      a != x | p(x)
 *
 *   to all terms except domain_elements and left sides of equalities (+ or -)
 *
 *************/

void flatten_clause(struct clause *d)
{
  struct term *t, *t1, *t2;
  struct literal *lit;
  int vnum = MAX_VARS;

  t = yankable(d);
  while (t) {
    lit = get_literal();
    lit->next_lit = d->first_lit;
    d->first_lit = lit;
    lit->sign = 0;
    t1 = copy_term(t);
    t2 = get_term();
    t2->type = VARIABLE;
    t2->varnum = ++vnum;
    lit->atom = build_binary_term(eq_sn(), t1, t2);
    for (lit = lit->next_lit; lit; lit = lit->next_lit)
      lit->atom = replace_term(lit->atom, t, t2);
    t = yankable(d);
  }
}  /* flatten_clause */

/*************
 *
 *   flatten_clauses()
 *
 *************/

struct list *flatten_clauses(struct list *l)
{
  struct clause *c;
  struct list *nl = get_list();
  for (c = l->first_cl; c; c = c->next_cl) {
    struct clause *d = cl_copy(c);
    d->id = c->id;  /* This is questionable. */
    check_for_bad_things(d);
    flatten_clause(d);
    if (renumber_vars(d) == 0)
      MACE_abend("dp_trans, too many variables");
    append_cl(nl, d);
  }
  return nl;
}  /* flatten_clauses */

/*************
 *
 *   check_transformed_clause()
 *
 *   This checks that the transformed clause is equality-equivalent
 *   to the original.
 *
 *************/

void check_transformed_clause(struct clause *c,
			      struct clause *orig)
{
  struct clause *d = cl_copy(c);

  process_negative_equalities(d);
  if (!kludgey_e_subsume(d, orig) || !kludgey_e_subsume(orig, d)) {
    printf("%% WARNING: possible error in check_transformed_clause:\n");
    printf("%%    original:    "); p_clause(orig);
    printf("%%    transformed: "); p_clause(d);
  }
}  /* check_transformed_clause */

