/*
 *   geometry.c -- Padmanabhan's inference rule
 *
 *   This is an inference rule of R. Padmanabhan.  Assume f is a binary
 *   functor, left and right cancellation hold for f, and the following
 *   axiom holds: f(f(x,y),f(z,u)) = f(f(x,z),f(y,u)).  Then the following
 *   inference rule is sound:
 *
 *   Consider A=B, in which A and B are terms built from f, variables, and
 *   constants.  (If other terms appear, we can treat them as constants.)
 *   If A and B have identical terms, say C, in corresponding positions,
 *   then infer A'=B', which is similar to A=B, except that the two occurrences
 *   of C have been replaced with a fresh variable.
 *
 *   Of course, if two terms in corresponding positions can be made
 *   identical by unification, then we can instantiate that equality
 *   and replace the terms with a fresh variable.
 *
 *   This file contains several versions of the inference rule.
 *
 *   1. As a rewrite rule (separate from demodulation) (without unification).
 *      routine geo_rewrite below.
 *
 *   2. As an inference rule (applied to given clause), with unification.
 *      routine geometry_rule_unif below.
 *
 */

#include "header.h"

#define MAX_DEPTH 50

/*************
 *
 *   is_geometry_symbol() -- THIS IS LIKELY TO CHANGE!!!
 *
 *************/

static int is_geometry_symbol(int sn)
{
#if 1  /* August 9.  make gL apply everywhere. */
  return(1);
#else
  return(str_ident(sn_to_str(sn), "f") ||
	 str_ident(sn_to_str(sn), "g") ||
	 str_ident(sn_to_str(sn), "*"));
#endif
}  /* is_geometry_symbol */

/*************
 *
 *   geo_rewrite_recurse()
 *
 *************/

static int geo_rewrite_recurse(struct rel *a,
			       struct rel *b,
			       struct clause *cl,
			       int *next_var)
{
#if 0
  if (a->argval->type == VARIABLE || b->argval->type == VARIABLE)
    return(0);
  else
#endif
	
    if (term_ident(a->argval, b->argval)) {

      if (a->argval->type == VARIABLE && occurrences(a->argval, cl->first_lit->atom) == 2)
	/* a & b are the only occurrences, so there is no point in applying the rule. */
	return(0);
      else {
	struct term *t;
	struct ilist *p1, *p2;

	if (*next_var == MAX_VARS) {
	  abend("geo_rewrite_recurse, too many variables.");
	}
	
	for (p1 = cl->parents; p1 && p1->next; p1 = p1->next);
	p2 = get_ilist(); p2->i = GEO_ID_RULE; p2->next = NULL;
	if (p1)
	  p1->next = p2;
	else
	  cl->parents = p2;
	
	zap_term(a->argval);
	zap_term(b->argval);
	t = get_term(); t->type = VARIABLE; t->varnum = *next_var; a->argval = t;
	t = get_term(); t->type = VARIABLE; t->varnum = *next_var; b->argval = t;
	(*next_var)++;
	
	return(1);
      }
    }
    else if (is_geometry_symbol(a->argval->sym_num) && a->argval->sym_num == b->argval->sym_num) {
      int n;
      struct rel *a1, *b1;

      n = 0;
      for (a1 = a->argval->farg, b1 = b->argval->farg; a1; a1 = a1->narg, b1 = b1->narg)
	n += geo_rewrite_recurse(a1, b1, cl, next_var);
      return(n);
    }
    else
      return(0);
}  /* geo_rewrite_recurse */

/*************
 *
 *   geo_rewrite()
 *
 *   This is a rewrite version of Padmanabhan's geometry law.
 *
 *   When applied, this rule introduces a new variable, and the number
 *   of the new variable is 1 more than the greatest variable already
 *   in the equality.
 *
 *   This routine applies the rule as much as possible.
 *
 *************/

int geo_rewrite(struct clause *c)
{
  if (c->first_lit && !c->first_lit->next_lit && pos_eq_lit(c->first_lit)) {

    struct rel *a, *b;
    int next_var, replacements;

    a = c->first_lit->atom->farg;
    b = a->narg;
    next_var = biggest_var(c->first_lit->atom) + 1;
    replacements = geo_rewrite_recurse(a, b, c, &next_var);
    return(replacements > 0);
  }
  else
    return(0);
}  /* geo_rewrite */

/*************
 *
 *   geo_replace_unif()
 *
 *************/

static struct term *geo_replace_unif(struct term *t,
				     int *pos_vec,
				     int depth)
{
  if (depth == 0) {
    zap_term(t);
    t = get_term();
    t->type = VARIABLE;
    t->varnum = MAX_VARS+1;
  }
  else {
    struct rel *r;
    int i;

    for (i=1, r=t->farg; i < *pos_vec; i++, r = r->narg);

    r->argval = geo_replace_unif(r->argval, pos_vec+1, depth-1);
  }
  return(t);
}  /* geo_replace_unif */

/*************
 *
 *   geo_generate_unif()
 *
 *************/

static void geo_generate_unif(struct clause *giv_cl,
			      int *pos_vec,
			      int depth,
			      struct context *subst)
{
  struct clause *new_clause;
  struct ilist *ip;
  struct literal *lit;

  new_clause = get_clause();
  new_clause->type = giv_cl->type;
  lit = get_literal();
  lit->sign = giv_cl->first_lit->sign;
  lit->container = new_clause;
  new_clause->first_lit = lit;
  lit->atom = apply(giv_cl->first_lit->atom, subst);
  lit->atom->occ.lit = lit;
  lit->atom->varnum = giv_cl->first_lit->atom->varnum;  /* type of atom */

  lit->atom->farg->argval = geo_replace_unif(lit->atom->farg->argval, pos_vec, depth);
  lit->atom->farg->narg->argval = geo_replace_unif(lit->atom->farg->narg->argval, pos_vec, depth);

  ip = get_ilist();
  ip->i = GEO_RULE;
  new_clause->parents = ip;

  ip = get_ilist();
  ip->i = giv_cl->id;
  new_clause->parents->next = ip;

  Stats[CL_GENERATED]++;
  Stats[GEO_GEN]++;
  pre_process(new_clause, 0, Sos);

}  /* geo_generate_unif */

/*************
 *
 *   geo_recurse_unif()
 *
 *************/

static void geo_recurse_unif(struct term *a,
			     struct term *b,
			     struct clause *giv_cl,
			     int *pos_vec,
			     int depth)
{
  struct context *subst;
  struct trail *tr;

  subst = get_context();
  subst->multiplier = 0;
  tr = NULL;

  if (unify(a, subst, b, subst, &tr)) {
    geo_generate_unif(giv_cl, pos_vec, depth, subst);
    clear_subst_1(tr);
  }

  if (is_geometry_symbol(a->sym_num) && a->sym_num == b->sym_num) {
    struct rel *a1, *b1;
    int i;
	
    if (depth == MAX_DEPTH) {
      abend("geo_recurse, MAX_DEPTH.");
    }
    for (i = 1, a1 = a->farg, b1 = b->farg; a1; i++, a1 = a1->narg, b1 = b1->narg) {
      pos_vec[depth] = i;
      geo_recurse_unif(a1->argval, b1->argval, giv_cl, pos_vec, depth+1);
    }

    pos_vec[depth] = 0;  /* not really necessary */
  }
}  /* geo_recurse_unif */

/*************
 *
 *   geometry_rule_unif()
 *
 *   As in other inference rules, this rule assumes that the given clause
 *   has variables renumbered, in particular, that all variable numbers
 *   are < MAX_VARS.  And as in other inference rules, clauses inferred by
 *   this rule may have variable numbers >= MAX_VARS.
 *
 *************/

void geometry_rule_unif(struct clause *giv_cl)
{
  if (giv_cl->first_lit && !giv_cl->first_lit->next_lit &&
      pos_eq_lit(giv_cl->first_lit)) {

    struct term *a, *b;
    int pos_vec[MAX_DEPTH];

    a = giv_cl->first_lit->atom->farg->argval;
    b = giv_cl->first_lit->atom->farg->narg->argval;

    geo_recurse_unif(a, b, giv_cl, pos_vec, 0);
  }

}  /* geometry_rule_unif */

/*************
 *
 *   child_of_geometry()
 *
 *************/

int child_of_geometry(struct clause *c)
{
  struct ilist *p;

  for (p = c->parents; p; p = p->next) {
    if (p->i == GEO_ID_RULE ||
	p->i == GEO_RULE)
      return(1);
  }
  return(0);

}  /* child_of_geometry */

/*************
 *
 *   gl_demod()
 *
 *   Copy, demodulate, then pre_process.
 *
 *************/

void gl_demod(struct clause *c,
	      struct list *lst)
{
  struct clause *d;

  struct ilist *ip1, *ip2;
	    
  d = cl_copy(c);
    
  ip1 = get_ilist(); ip1->i = COPY_RULE; d->parents = ip1;
  ip2 = get_ilist(); ip2->i = c->id; ip1->next = ip2;

  demod_cl(d);

  pre_process(d, 0, lst);

}  /* gl_demod */
