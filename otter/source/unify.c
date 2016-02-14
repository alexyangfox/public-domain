/*
 *  unify.c -- unification and matching routines
 *
 */

#include "header.h"

/*  The following macros are used throughout this file.  */

#define BIND(i, c1, t2, c2, trp) { struct trail *tr; \
    c1->terms[i] = t2; c1->contexts[i] = c2; \
    tr = get_trail(); tr->varnum = i; tr->context = c1; \
    tr->next = *trp; *trp = tr; }

#define DEREFERENCE(t, c) { int i; \
    while (t->type == VARIABLE && c->terms[i = t->varnum]) \
    { t = c->terms[i]; c = c->contexts[i]; } }

/*************
 *
 *    int occur_check(varnum, var_context, term, term_context)
 *
 *    Return 0 iff variable occurs in term under substitution
 *       (including var==term).
 *
 *************/

int occur_check(int vn,
		struct context *vc,
		struct term *t,
		struct context *c)
{
  if (t->type == NAME)
    return 1;
  else if (t->type == COMPLEX) {
    struct rel *r = t->farg;
    while (r != NULL && occur_check(vn, vc, r->argval, c))
      r = r->narg;
    return (r == NULL);
  }
  else {  /* variable */
    int tvn = t->varnum;
    if (tvn == vn && c == vc)
      return 0;  /* fail occur_check here */
    else if (c->terms[tvn] == NULL)
      return 1;  /* uninstantiated variable */
    else
      return occur_check(vn, vc, c->terms[tvn], c->contexts[tvn]);
  }
}  /* occur_check */

/*************
 *
 *    int unify(t1, c1, t2, c2, trail_address)
 *
 *        Attempt to unify t1 in context c1 with t2 in context c2.
 *    If successful, return 1 and and a pointer to the trail (a record
 *    of the substitutions).  The trail is extended by adding new
 *    entries to the front, and the front is returned.  On entry,
 *    *trail_address must be either NULL or the result of a previous
 *    call to unify.  If unification fails, the trail is unchanged.
 *    A context is a substitution table along with a multiplier for
 *    the variables.  The multiplier need not be present for
 *    unification, but it is needed for `apply'.
 *
 *        An example of its use:
 *
 *             c1 = get_context(); c1->multiplier = 0;
 *             c2 = get_context(); c2->multiplier = 1;
 *             tr = NULL;
 *             if (unify(t1, c1, t2, c2, &tr)) {
 *                 print_subst(stdout, c1);
 *                 print_subst(stdout, c2);
 *                 print_trail(stdout, tr);
 *                 t3 = apply(t1, c1);
 *                 t4 = apply(t2, c2);
 *                 printf("apply substitution: ");
 *                 print_term(stdout, t3); printf(" ");
 *                 print_term(stdout, t4); printf("\n");
 *                 clear_subst_1(tr);
 *                 zap_term(t3);
 *                 zap_term(t4);
 *                 }
 *             else
 *                 printf("unify fails\n");
 *             free_context(c1);
 *             free_context(c2);
 *
 *************/


int unify(struct term *t1,
	  struct context *c1,
	  struct term *t2,
	  struct context *c2,
	  struct trail **trp)
{
  DEREFERENCE(t1, c1);
  DEREFERENCE(t2, c2);

  /* Now, neither t1 nor t2 is a bound variable. */

  if (t1->type == VARIABLE) {
    int vn1 = t1->varnum;
    if (t2->type == VARIABLE) {
      /* both t1 and t2 are variables */
      if (vn1 == t2->varnum && c1 == c2)
	return 1;  /* identical */
      else {
	BIND(vn1, c1, t2, c2, trp);
	return 1;
      }
    }
    else {
      /* t1 variable, t2 not variable */
      if (occur_check(vn1, c1, t2, c2)) {
	BIND(vn1, c1, t2, c2, trp);
	return 1;
      }
      else
	return 0;  /* failed occur_check */
    }
  }

  else if (t2->type == VARIABLE) {
    /* t2 variable, t1 not variable */
    int vn2 = t2->varnum;
    if (occur_check(vn2, c2, t1, c1)) {
      BIND(vn2, c2, t1, c1, trp);
      return 1;
    }
    else
      return 0;  /* failed occur_check */
  }

  else if (t1->sym_num != t2->sym_num)
    return 0;  /* fail because of symbol clash */

  else if (t1->type == NAME)
    return 1;

  else {  /* both COMPLEX with same functor */
    struct trail *tpos = *trp;  /* save trail position in case of failure */
    struct rel *r1 = t1->farg;
    struct rel *r2 = t2->farg;
    while (r1 && unify(r1->argval, c1, r2->argval, c2, trp)) {
      r1 = r1->narg;
      r2 = r2->narg;
    }
    if (r1 == NULL)
      return 1;
    else {  /* restore trail and fail */
      struct trail *tp = *trp;
      while (tp != tpos) {
	struct trail *t3 = tp;
	tp->context->terms[tp->varnum] = NULL;
	tp = tp->next;
	free_trail(t3);
      }
      *trp = tpos;
      return 0;
    }
  }
}  /* unify */

/*************
 *
 *    int unify_no_occur_check(t1, c1, t2, c2, trp)
 *
 *************/

int unify_no_occur_check(struct term *t1,
			 struct context *c1,
			 struct term *t2,
			 struct context *c2,
			 struct trail **trp)
{
  DEREFERENCE(t1, c1);
  DEREFERENCE(t2, c2);

  /* Now, neither t1 nor t2 is a bound variable. */

  if (t1->type == VARIABLE) {
    int vn1 = t1->varnum;
    if (t2->type == VARIABLE && vn1 == t2->varnum && c1 == c2)
      return 1;  /* identical */
    else {
      /* occur check would be here */
      BIND(vn1, c1, t2, c2, trp);
      return 1;
    }
  }
	
  else if (t2->type == VARIABLE) {
    int vn2 = t2->varnum;
    /* occur check would be here */
    BIND(vn2, c2, t1, c1, trp);
    return 1;
  }

  else if (t1->sym_num != t2->sym_num)
    return 0;  /* fail because of symbol clash */

  else if (t1->type == NAME)
    return 1;

  else {  /* both COMPLEX with same functor */
    struct trail *tpos = *trp;  /* save trail position in case of failure */
    struct rel *r1 = t1->farg;
    struct rel *r2 = t2->farg;
    while (r1 && unify_no_occur_check(r1->argval, c1, r2->argval, c2, trp)) {
      r1 = r1->narg;
      r2 = r2->narg;
    }
    if (r1 == NULL)
      return 1;
    else {  /* restore trp and fail */
      struct trail *tp = *trp;
      while (tp != tpos) {
	struct trail *t3 = tp;
	tp->context->terms[tp->varnum] = NULL;
	tp = tp->next;
	free_trail(t3);
      }
      *trp = tpos;
      return 0;
    }
  }
}  /* unify_no_occur_check */

/*************
 *
 *    int match(t1, c1, t2, trail_address) -- one-way unification.
 *
 *        Match returns 1 if t2 is an instance of {t1 in context c1}.
 *    This is not a very general version, but it is useful for
 *    demodulation and subsumption.  It assumes that the variables
 *    of t1 and t2 are separate, that none of the variables in t2
 *    have been instantiated, and that none of those t2's variables
 *    will be instantiatied.  Hence, there is no context for t2,
 *    no need to dereference more than one level, and no need for
 *    an occur_check.
 *
 *        The use of the trail is the same as in `unify'.
 *
 *************/

int match(struct term *t1,
	  struct context *c1,
	  struct term *t2,
	  struct trail **trp)
{
  if (t1->type == VARIABLE) {  /* t1 variable */
    int vn = t1->varnum;
    if (c1->terms[vn] == NULL) {
      BIND(vn, c1, t2, NULL, trp);
      return 1;
    }
    else
      return term_ident(c1->terms[vn], t2);
  }
  else if (t2->type == VARIABLE)  /* t1 not variable, t2 variable, so fail */
    return 0;
  else  /* neither term is a variable */
    if (t1->sym_num != t2->sym_num)
      return 0;  /* fail because of symbol clash */
    else {  /* following handles both names and complex terms */
      struct trail *tpos = *trp;  /* save trail position in case of failure */
      struct rel *r1 = t1->farg;
      struct rel *r2 = t2->farg;
      /* arities are same because sym_nums are the same */
      while (r1 != NULL && match(r1->argval, c1, r2->argval, trp)) {
	r1 = r1->narg;
	r2 = r2->narg;
      }
      if (r1 == NULL)
	return 1;
      else {  /* restore from trail and fail */
	struct trail *tp = *trp;
	while (tp != tpos) {
	  struct trail *t3 = tp;
	  tp->context->terms[tp->varnum] = NULL;
	  tp = tp->next;
	  free_trail(t3);
	}
	*trp = tpos;
	return 0;
      }
    }
}  /* match */

/*************
 *
 *    struct term *apply(term, context) -- Apply a substitution to a term.
 *
 *       Apply always succeeds and returns a pointer to the
 *    instantiated term.
 *
 *************/

struct term *apply(struct term *t,
		   struct context *c)
{
  /* dereference if variable */

  /* A NULL context means that the subst was generated by match. */
  /* If the context is NULL, then apply just copies the term.    */

  while (t->type == VARIABLE && c != NULL && c->terms[t->varnum] != NULL) {
    int vn = t->varnum;
    t = c->terms[vn];
    c = c->contexts[vn];
  }

  if (t->type == VARIABLE) {  /* variable */
    struct term *t2 = get_term();
    t2->type = VARIABLE;
    if (c == NULL)
      t2->varnum = t->varnum;
    else
      t2->varnum = c->multiplier * MAX_VARS + t->varnum;
    return t2;
  }
  else if (t->type == NAME) {  /* name */
    struct term *t2 = get_term();
    t2->type = NAME;
    t2->sym_num = t->sym_num;
    return t2;
  }
  else {  /* complex term */
    struct rel *r1, *r2, *r3;
    struct term *t2 = get_term();
    t2->type = COMPLEX;
    t2->sym_num = t->sym_num;
    r3 = NULL;
    r1 = t->farg;
    while (r1 != NULL ) {
      r2 = get_rel();
      if (r3 == NULL)
	t2->farg = r2;
      else
	r3->narg = r2;
      r2->argval = apply(r1->argval, c);
      r3 = r2;
      r1 = r1->narg;
    }
    return t2;
  }
}  /* apply */

/*************
 *
 *    int term_ident(term1, term2) -- Compare two terms.
 *
 *        If identical return 1; else return 0.  The bits
 *    field is not checked.
 *
 *************/

int term_ident(struct term *t1,
	       struct term *t2)
{
  if (t1->type != t2->type)
    return 0;
  else if (t1->type == COMPLEX) {
    if (t1->sym_num != t2->sym_num)
      return 0;
    else {
      struct rel *r1 = t1->farg;
      struct rel *r2 = t2->farg;
      while (r1 && term_ident(r1->argval,r2->argval)) {
	r1 = r1->narg;
	r2 = r2->narg;
      }
      return (r1 == NULL);
    }
  }
  else if (t1->type == VARIABLE)
    return (t1->varnum == t2->varnum);
  else  /* NAME */
    return (t1->sym_num == t2->sym_num);
}  /* term_ident */

/*************
 *
 *    clear_subst_2(trail_1, trail_2) -- Clear part of a substitution.
 *
 *        It is assumed that trail_2 (including NULL) is a subtrail
 *    of trail_1. This routine clears entries starting at trail_1,
 *    up to (but not including) trail_2.
 *
 *************/

void clear_subst_2(struct trail *t1,
		   struct trail *t2)
{
  if (t1 == NULL || t1 == t2)
    return;
  else {
    clear_subst_2(t1->next, t2);
    t1->context->terms[t1->varnum] = NULL;
    free_trail(t1);
  }
}  /* clear_subst_2 */

/*************
 *
 *    clear_subst_1(trail_1) -- Clear a substitution.
 *
 *    Clear all members of the trail.
 *
 *************/

void clear_subst_1(struct trail *t1)
{
  if (t1 == NULL)
    return;
  else {
    clear_subst_1(t1->next);
    t1->context->terms[t1->varnum] = NULL;
    free_trail(t1);
  }
}  /* clear_subst_1 */

/*************
 *
 *    print_subst(file_ptr, context)
 *
 *************/

void print_subst(FILE *fp,
		 struct context *c)
{
  int i;

  fprintf(fp, "Substitution in context %p, multiplier %d\n", (void *) c, c->multiplier);

  for (i=0; i< MAX_VARS; i++)
    if (c->terms[i] != NULL) {
      fprintf(fp, "v%d -> ", i);
      print_term(fp, c->terms[i]);
      fprintf(fp, " context %p", (void *) c->contexts[i]);
      fprintf(fp, " status %d\n", c->status[i]);
    }
}  /* print_subst */

/*************
 *
 *    p_subst(context)
 *
 *************/

void p_subst(struct context *c)
{
  print_subst(stdout, c);
}  /* p_subst */

/*************
 *
 *    print_trail(file_ptr, context)
 *
 *************/

void print_trail(FILE *fp,
		 struct trail *t)
{
  struct trail *t2;
  fprintf(fp, "Trail:");
  t2 = t;
  while (t2 != NULL) {
    fprintf(fp, " <%d,%p>", t2->varnum, (void *) t2->context);
    t2 = t2->next;
  }
  fprintf(fp, ".\n");
}  /* print_trail */

