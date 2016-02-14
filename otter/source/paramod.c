/*
 *  paramod.c -- Paramodulation inference rules.
 *
 */

#include "header.h"

/*************
 *
 *    struct term *apply_substitute(t, into_term, into_subst, beta, from_subst)
 *
 *    This routine is similar to apply, except that when it reaches the into
 *    term, the appropriate instance of beta is returned.
 *
 *************/

static struct term *apply_substitute(struct term *t,
				     struct term *into_term,
				     struct context *into_subst,
				     struct term *beta,
				     struct context *from_subst,
				     int *pos_vec,
				     int *pi)
{
  struct term *t2;
  struct rel *r1, *r2, *r3;

  if (t == into_term)
    return(apply(beta, from_subst));
  else if (t->type != COMPLEX) {
    if (Flags[PARA_ALL].val == 0) {
      print_term_nl(stdout, t);
      abend("apply_substitute, term not COMPLEX.");
    }
    return(apply(t, into_subst));
  }
  else {
    int i;

    t2 = get_term();
    t2->type = COMPLEX;
    t2->sym_num = t->sym_num;
    r3 = NULL;
    for(r1 = t->farg, i = 1; r1; r1 = r1->narg, i++) {
      r2 = get_rel();
      if (r3 == NULL)
	t2->farg = r2;
      else
	r3->narg = r2;
      /* if we are on the path to the into term || PARA_ALL */
      if (r1->path || Flags[PARA_ALL].val) {
	if (*pi == MAX_FS_TERM_DEPTH)
	  abend("apply_substitute: term too deep.");
	pos_vec[*pi] = i;
	(*pi)++;
	r2->argval = apply_substitute(r1->argval, into_term,
				      into_subst, beta, from_subst,
				      pos_vec, pi);
      }
      else
	r2->argval = apply(r1->argval, into_subst);
      r3 = r2;
    }
    return(t2);
  }
}  /* apply_substitute */

/*************
 *
 *    struct clause *build_bin_para(alpha, from_subst, into_term, into_lit, into_subst)
 *
 *    Construct a binary paramodulant.
 *
 *************/

static struct clause *build_bin_para(struct term *alpha,
				     struct context *from_subst,
				     struct term *into_term,
				     struct literal *into_lit,
				     struct context *into_subst,
				     int *pos_vec,
				     int *pi)
{
  struct clause *paramodulant;
  struct literal *lit, *new, *prev;
  struct term *from_atom, *beta;
  struct ilist *ip0, *ip1, *ip2;
  int i;

  paramodulant = get_clause();
  prev = NULL;

  from_atom = alpha->occ.rel->argof;  /* find beta */
  if (from_atom->farg->argval == alpha)
    beta = from_atom->farg->narg->argval;  /* beta is second arg */
  else
    beta = from_atom->farg->argval;  /* beta is first arg */

  *pi = 1;

  /* go through literals of into clause */
  for (lit = into_lit->container->first_lit, i = 1; lit; lit = lit->next_lit, i++) {
    new = get_literal();
    new->container = paramodulant;
    if (prev == NULL)
      paramodulant->first_lit = new;
    else
      prev->next_lit = new;
    prev = new;
    new->sign = lit->sign;
    if (lit == into_lit || Flags[PARA_ALL].val) {
      pos_vec[0] = i;
      new->atom = apply_substitute(lit->atom, into_term, into_subst,
				   beta, from_subst, pos_vec, pi);
    }
    else
      new->atom = apply(lit->atom, into_subst);
    new->atom->occ.lit = new;
    new->atom->varnum = lit->atom->varnum;  /* copy type of atom */
  }

  /* go through literals of from clause */
  for (lit = from_atom->occ.lit->container->first_lit; lit; lit = lit->next_lit) {
    if (lit->atom != from_atom) {  /* omit instance of from literal */
      new = get_literal();
      new->container = paramodulant;
      if (paramodulant->first_lit == NULL)
	paramodulant->first_lit = new;
      else
	prev->next_lit = new;
      prev = new;
      new->sign = lit->sign;
      new->atom = apply(lit->atom, from_subst);
      new->atom->occ.lit = new;
      new->atom->varnum = lit->atom->varnum;  /* copy type of atom */
    }
  }

  ip0 = get_ilist(); /* rule and parents: to be filled in by caller */
  ip1 = get_ilist();
  ip2 = get_ilist();
  ip0->next = ip1;
  ip1->next = ip2;
  paramodulant->parents = ip0;
  return(paramodulant);
}  /* build_bin_para */

/*************
 *
 *   insert_detailed_para_history()
 *
 *************/

static void insert_detailed_para_history(struct ilist *ip_from,
					 struct ilist *ip_into,
					 struct term *alpha,
					 int *pos_vec,
					 int pos_vec_size)
{

  struct ilist *ip2, *ip3, *ip4;
  int i;
		
  /* Insert position of into term */
  ip2 = get_ilist();
  ip2->i = LIST_RULE - pos_vec_size;
  ip4 = ip_into->next;
  ip_into->next = ip2;
  for (i = 0; i < pos_vec_size; i++) {
    ip3 = get_ilist();
    ip3->i = pos_vec[i];
    ip2->next = ip3;
    ip2 = ip3;
  }
  ip2->next = ip4;

  /* Insert position of alpha. */
  ip2 = get_ilist();
  ip3 = get_ilist();
  ip4 = get_ilist();
  ip2->next = ip3;
  ip3->next = ip4;
    
  ip2->i = LIST_RULE - 2;
  ip3->i = literal_number(alpha->occ.rel->argof->occ.lit);
  ip4->i = (alpha->occ.rel->argof->farg->argval == alpha ? 1 : 2);

  ip4->next = ip_from->next;
  ip_from->next = ip2;

}  /* insert_detailed_para_history */

/*************
 *
 *    para_from_up(t, into_term, into_subst, alpha, from_subst)
 *
 *    We are paramodulating from the given clause, and a clashable into term
 *    has been found.  This routine recursively goes through the clashable
 *    superterms of the into term.
 *
 *************/

static void para_from_up(struct term *t,
			 struct term *into_term,
			 struct context *into_subst,
			 struct term *alpha,
			 struct context *from_subst)
{
  struct clause *paramodulant, *from_parent;
  struct rel *r;
  struct ilist *ip;
  int pos_vec[MAX_FS_TERM_DEPTH];
  int pos_vec_size;

  from_parent = alpha->occ.rel->argof->occ.lit->container;

  if (t->type == COMPLEX && t->varnum != 0) {  /* it's an atom */
    if (Flags[PARA_INTO_UNITS_ONLY].val == 0 ||
	unit_clause(t->occ.lit->container)) {
      paramodulant = build_bin_para(alpha, from_subst, into_term,
				    t->occ.lit, into_subst,
				    pos_vec, &pos_vec_size);
      /* fill in derivation info */
      ip = paramodulant->parents;
      ip->i = PARA_FROM_RULE;
      ip->next->i = from_parent->id;
      ip->next->next->i = t->occ.lit->container->id;
      if (Flags[DETAILED_HISTORY].val) {
	insert_detailed_para_history(ip->next, ip->next->next, alpha,
				     pos_vec, pos_vec_size);

      }
      Stats[CL_GENERATED]++;
      Stats[PARA_FROM_GEN]++;
      if (heat_is_on())
	paramodulant->heat_level = from_parent->heat_level + 1;
      CLOCK_STOP(PARA_FROM_TIME);
      pre_process(paramodulant, 0, Sos);
      CLOCK_START(PARA_FROM_TIME);
    }
  }
  else {
    r = t->occ.rel;
    while (r != NULL) {
      if (r->clashable) {
	r->path = 1;  /* mark path from into_term up to atom */
	para_from_up(r->argof, into_term, into_subst, alpha, from_subst);
	r->path = 0;  /* remove mark */
      }
      r = r->nocc;
    }
  }
}  /* para_from_up */

/*************
 *
 *    para_from_alpha(alpha)
 *
 *    We are paramodulating from the given clause.  This routine
 *    paramodulates from an alpha.
 *
 *************/

static void para_from_alpha(struct term *alpha)
{
  struct context *into_subst, *from_subst;
  struct term *into_term;
  struct fpa_tree *ut;
  struct trail *tr;

  into_subst = get_context();
  into_subst->multiplier = 0;
  from_subst = get_context();
  from_subst->multiplier = 1;

  if (alpha->type == VARIABLE && Flags[PARA_FROM_VARS].val == 0)
    ;  /* do nothing */
  else {
    if (alpha->type == VARIABLE)
      ut = build_for_all(Fpa_clash_terms);  /* get all terms in index */
    else
      ut = build_tree(alpha, UNIFY, Parms[FPA_TERMS].val,
		      Fpa_clash_terms);
    into_term = next_term(ut, 0);
    while (into_term != NULL) {
      tr = NULL;
      if (unify(into_term, into_subst, alpha, from_subst, &tr)) {
	para_from_up(into_term, into_term, into_subst, alpha, from_subst);
	clear_subst_1(tr);
      }
      into_term = next_term(ut, 0);
    }
  }

  free_context(into_subst);
  free_context(from_subst);
}  /* para_from_alpha */

/*************
 *
 *    para_from(giv_cl) -- binary paramodulation from the given clause
 *
 *    Paramodulants are given to the routine pre_process.
 *
 *************/

void para_from(struct clause *giv_cl)
{
  struct literal *from_lit;
  struct term *atom;

  CLOCK_START(PARA_FROM_TIME);

  if (!Flags[PARA_FROM_UNITS_ONLY].val || unit_clause(giv_cl)) {

    from_lit = giv_cl->first_lit;
    while (from_lit) {
      atom = from_lit->atom;
      if (pos_eq_lit(from_lit) &&
	  !term_ident(atom->farg->argval, atom->farg->narg->argval)) {
	if (Flags[PARA_FROM_LEFT].val)
	  para_from_alpha(atom->farg->argval);
	if (Flags[PARA_FROM_RIGHT].val)
	  para_from_alpha(atom->farg->narg->argval);
      }
      from_lit = from_lit->next_lit;
    }
  }

  CLOCK_STOP(PARA_FROM_TIME);
}  /* para_from */

/*************
 *
 *    para_into_terms(t, into_lit, from_subst, into_subst)
 *
 *    We are paramodulating into the given clause.  This routine recursively
 *    goes through the clashable subterms of the given literal.
 *
 *************/

static void para_into_terms(struct term *into_term,
			    struct literal *into_lit,
			    struct context *from_subst,
			    struct context *into_subst)
{
  struct term *alpha;
  struct trail *tr;
  struct fpa_tree *ut;
  struct clause *paramodulant;
  struct rel *r;
  struct ilist *ip;
  int pos_vec[MAX_FS_TERM_DEPTH];
  int pos_vec_size;

  if (into_term->type == COMPLEX) {
    r = into_term->farg;
    while (r != NULL) {
      if (r->clashable) {
	r->path = 1;  /* mark path to into term */
	para_into_terms(r->argval, into_lit, from_subst, into_subst);
	r->path = 0;  /* remove mark */
      }
      r = r->narg;
    }
  }

  /* no need to check if variable and `no para into vars' */
  /* because the clashability flag handles it */

  if (into_term->type == VARIABLE)
    ut = build_for_all(Fpa_alphas);  /* get all terms in index */
  else
    ut = build_tree(into_term, UNIFY, Parms[FPA_TERMS].val, Fpa_alphas);

  alpha = next_term(ut, 0);
  while (alpha != NULL) {
    tr = NULL;
    if (unify(into_term, into_subst, alpha, from_subst, &tr)) {
	    
      paramodulant = build_bin_para(alpha, from_subst, into_term,
				    into_lit, into_subst,
				    pos_vec, &pos_vec_size);
      /* fill in derivation info */
      ip = paramodulant->parents;
      ip->i = PARA_INTO_RULE;
      ip->next->i = into_lit->container->id;
      ip->next->next->i = alpha->occ.rel->argof->occ.lit->container->id;

      if (Flags[DETAILED_HISTORY].val) {
	insert_detailed_para_history(ip->next->next, ip->next, alpha,
				     pos_vec, pos_vec_size);
      }

      clear_subst_1(tr);
      Stats[CL_GENERATED]++;
      Stats[PARA_INTO_GEN]++;
      if (heat_is_on())
	paramodulant->heat_level = into_lit->container->heat_level + 1;
      CLOCK_STOP(PARA_INTO_TIME);
      pre_process(paramodulant, 0, Sos);
      CLOCK_START(PARA_INTO_TIME);
    }
    alpha = next_term(ut, 0);
  }
}  /* para_into_terms */

/*************
 *
 *    para_into(giv_cl) -- binary paramodulation into the given clause
 *
 *    Paramodulants are given to the routine pre_process.
 *
 *************/

void para_into(struct clause *giv_cl)
{
  struct literal *into_lit;
  struct context *into_subst, *from_subst;
  struct rel *r;

  CLOCK_START(PARA_INTO_TIME);

  if (!Flags[PARA_INTO_UNITS_ONLY].val || unit_clause(giv_cl)) {

    /* Substitutions are allocated here instead of in */
    /* para_into_terms to save procedure calls.       */

    into_subst = get_context();
    into_subst->multiplier = 0;
    from_subst = get_context();
    from_subst->multiplier = 1;
    into_lit = giv_cl->first_lit;

    while (into_lit != NULL) {
      if (into_lit->atom->varnum != ANSWER) {  /* if not answer literal */
	r = into_lit->atom->farg;
	while (r != NULL) {
	  if (r->clashable) {
	    r->path = 1;  /* mark path to into term */
	    para_into_terms(r->argval, into_lit, from_subst, into_subst);
	    r->path = 0;  /* remove mark */
	  }
	  r = r->narg;
	}
      }
      into_lit = into_lit->next_lit;
    }
    free_context(into_subst);
    free_context(from_subst);
  }
  CLOCK_STOP(PARA_INTO_TIME);
}  /* para_into */

