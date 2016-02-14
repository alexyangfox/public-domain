/*
 *  index.c -- Routines for indexing and unindexing clauses.
 *
 */

#include "header.h"

/*************
 *
 *    index_mark_clash(r) -- recursive routine to mark and index
 *    clashable terms (terms that can be used by paramodulation).
 *
 *************/

static void index_mark_clash(struct rel *r)
{
  struct term *t;
  struct rel *r1;

  t = r->argval;

  if (t->type == VARIABLE && Flags[PARA_INTO_VARS].val == 0)
    return;
  else {
    r1 = t->occ.rel;
    while (r1 != NULL && r1->clashable == 0)
      r1 = r1->nocc;
    r->clashable = 1;
    if (r1 != NULL)
      return;  /* becuase t is already clashable */
    else {
      if (Flags[PARA_FROM].val)
	fpa_insert(t, Parms[FPA_TERMS].val, Fpa_clash_terms);
      if (t->type == COMPLEX) {
	if (Flags[PARA_SKIP_SKOLEM].val == 0 || is_skolem(t->sym_num) == 0) {
	  r = t->farg;
	  while (r != NULL) {
	    index_mark_clash(r);
	    if (Flags[PARA_ONES_RULE].val)
	      r = NULL;
	    else
	      r = r->narg;
	  }
	}
      }
    }
  }
}  /* index_mark_clash */

/*************
 *
 *    un_index_mark_clash(r)
 *
 *    See index_mark_clash.
 *
 *************/

static void un_index_mark_clash(struct rel *r)
{
  struct term *t;
  struct rel *r1;

  t = r->argval;

  if (t->type == VARIABLE && Flags[PARA_INTO_VARS].val == 0)
    return;
  else {
    r->clashable = 0;
    r1 = t->occ.rel;
    while (r1 != NULL && r1->clashable == 0)
      r1 = r1->nocc;
    if (r1 != NULL)
      return;  /* becuase t is clashable from another containing term */
    else {
      if (Flags[PARA_FROM].val)
	fpa_delete(t, Parms[FPA_TERMS].val, Fpa_clash_terms);
      if (t->type == COMPLEX) {
	if (Flags[PARA_SKIP_SKOLEM].val == 0 || is_skolem(t->sym_num) == 0) {
	  r = t->farg;
	  while (r != NULL) {
	    un_index_mark_clash(r);
	    if (Flags[PARA_ONES_RULE].val)
	      r = NULL;
	    else
	      r = r->narg;
	  }
	}
      }
    }
  }

}  /* un_index_mark_clash */

/*************
 *
 *    index_paramod(atom) -- index for paramodulation inference rules
 *
 *    Index clashable terms for `from' paramodulation, and
 *    index clashable args of equality for `into' paramodulation.
 *
 *    Also mark clashable terms for the paramodulation routines.
 *
 *************/

static void index_paramod(struct term *atom)
{
  struct rel *r;
  struct literal *lit;

  lit = atom->occ.lit;

  if (eq_lit(lit)&&term_ident(atom->farg->argval, atom->farg->narg->argval))
    return;

  /* First index clashable `into' terms for `from' paramodulation. */

  if (!Flags[PARA_INTO_UNITS_ONLY].val || unit_clause(lit->container)) {
	         
    if (pos_eq_lit(lit) || neg_eq_lit(lit)) {
      if (Flags[PARA_INTO_LEFT].val)
	index_mark_clash(atom->farg);
      if (Flags[PARA_INTO_RIGHT].val)
	index_mark_clash(atom->farg->narg);
    }
    else {
      for (r = atom->farg; r; r = r->narg)
	index_mark_clash(r);
    }
  }

  /* Now index clashable `from' terms for `into' paramodulation. */

  if (!Flags[PARA_FROM_UNITS_ONLY].val || unit_clause(lit->container)) {
		 
    if (pos_eq_lit(lit) && Flags[PARA_INTO].val) {
      if (Flags[PARA_FROM_LEFT].val) {
	if (Flags[PARA_FROM_VARS].val || atom->farg->argval->type != VARIABLE)
	  fpa_insert(atom->farg->argval, Parms[FPA_TERMS].val, Fpa_alphas);
      }

      if (Flags[PARA_FROM_RIGHT].val) {
	if (Flags[PARA_FROM_VARS].val || atom->farg->narg->argval->type != VARIABLE)
	  fpa_insert(atom->farg->narg->argval, Parms[FPA_TERMS].val, Fpa_alphas);
      }
    }
  }

}  /* index_paramod */

/*************
 *
 *    un_index_paramod(atom)
 *
 *    See index_paramod.
 *
 *************/

static void un_index_paramod(struct term *atom)
{
  struct rel *r;
  struct literal *lit;

  lit = atom->occ.lit;

  if (eq_lit(lit)&&term_ident(atom->farg->argval, atom->farg->narg->argval))
    return;

  if (!Flags[PARA_INTO_UNITS_ONLY].val || unit_clause(lit->container)) {

    if (pos_eq_lit(lit) || neg_eq_lit(lit)) {
      if (Flags[PARA_INTO_LEFT].val)
	un_index_mark_clash(atom->farg);
      if (Flags[PARA_INTO_RIGHT].val)
	un_index_mark_clash(atom->farg->narg);
    }
    else {
      for (r = atom->farg; r; r = r->narg)
	un_index_mark_clash(r);
    }
  }

  if (!Flags[PARA_FROM_UNITS_ONLY].val || unit_clause(lit->container)) {

    if (pos_eq_lit(lit) && Flags[PARA_INTO].val) {
      if (Flags[PARA_FROM_LEFT].val)
	if (Flags[PARA_FROM_VARS].val || atom->farg->argval->type != VARIABLE)
	  fpa_delete(atom->farg->argval, Parms[FPA_TERMS].val, Fpa_alphas);

      if (Flags[PARA_FROM_RIGHT].val)
	if (Flags[PARA_FROM_VARS].val || atom->farg->narg->argval->type != VARIABLE)
	  fpa_delete(atom->farg->narg->argval, Parms[FPA_TERMS].val, Fpa_alphas);
    }
  }

}  /* un_index_paramod */

/*************
 *
 *    index_lits_all(c)
 *
 *    Index literals for forward subsumption, back subsumption, and
 *    unit conflict.
 *    Positive and negative literals go into different indexes.
 *    The NO_FAPL, NO_FANL and FOR_SUB_FPA flags are checked to determine
 *    what and how to index.
 *
 *    NO_FAPL can be set if you are generating only positive clauses
 *    and back subsumption is off.  It surpresses indexing of positive
 *    literals in the non-clashable index.  (The index for subsumption
 *    and unit conflict.)
 *    Similarly for negative literals with NO_FANL.
 *
 *************/

void index_lits_all(struct clause *c)
{
  struct literal *lit;

  lit = c->first_lit;
  while (lit != NULL) {
    if (lit->atom->varnum == ANSWER)
      ;  /* skip answer literal */
    else if (lit->sign) {
      if (Flags[NO_FAPL].val == 0)
	fpa_insert(lit->atom, Parms[FPA_LITERALS].val, Fpa_pos_lits);
      if ((Flags[FOR_SUB_FPA].val == 0 && Flags[FOR_SUB].val) ||
	  (Flags[UNIT_DELETION].val && num_literals(c) == 1))
	is_insert(lit->atom, Is_pos_lits);
    }
    else {
      if (Flags[NO_FANL].val == 0)
	fpa_insert(lit->atom, Parms[FPA_LITERALS].val, Fpa_neg_lits);
      if ((Flags[FOR_SUB_FPA].val == 0 && Flags[FOR_SUB].val) ||
	  (Flags[UNIT_DELETION].val && num_literals(c) == 1))
	is_insert(lit->atom, Is_neg_lits);
    }
    lit = lit->next_lit;
  }
}  /* index_lits_all */

/*************
 *
 *    un_index_lits_all(c)
 *
 *    See index_lits_all.
 *
 *************/

void un_index_lits_all(struct clause *c)
{
  struct literal *lit;

  lit = c->first_lit;
  while (lit != NULL) {
    if (lit->atom->varnum == ANSWER)
      ;  /* skip answer literal */
    else if (lit->sign) {
      if (Flags[NO_FAPL].val == 0)
	fpa_delete(lit->atom, Parms[FPA_LITERALS].val, Fpa_pos_lits);
      if ((Flags[FOR_SUB_FPA].val == 0 && Flags[FOR_SUB].val) ||
	  (Flags[UNIT_DELETION].val && num_literals(c) == 1))
	is_delete(lit->atom, Is_pos_lits);
    }
    else {
      if (Flags[NO_FANL].val == 0)
	fpa_delete(lit->atom, Parms[FPA_LITERALS].val, Fpa_neg_lits);
      if ((Flags[FOR_SUB_FPA].val == 0 && Flags[FOR_SUB].val) ||
	  (Flags[UNIT_DELETION].val && num_literals(c) == 1))
	is_delete(lit->atom, Is_neg_lits);
    }
    lit = lit->next_lit;
  }
}  /* un_index_lits_all */

/*************
 *
 *    index_lits_clash(c)
 *
 *    Index literals for inference rules, and index terms for paramodulation if
 *    any paramodulation inference rules are set.
 *
 *************/

void index_lits_clash(struct clause *c)
{
  struct literal *lit;

  lit = c->first_lit;
  while (lit != NULL) {
    if (lit->atom->varnum == ANSWER || lit->atom->varnum == EVALUABLE)
      ;  /* skip answer literals and evaluable literals */
    else if (lit->sign)
      fpa_insert(lit->atom, Parms[FPA_LITERALS].val, Fpa_clash_pos_lits);
    else
      fpa_insert(lit->atom, Parms[FPA_LITERALS].val, Fpa_clash_neg_lits);
    if (Flags[PARA_FROM].val || Flags[PARA_INTO].val)
      index_paramod(lit->atom);
    lit = lit->next_lit;
    if (Flags[HYPER_SYMMETRY_KLUDGE].val)
      break;
  }
}  /* index_lits_clash */

/*************
 *
 *    un_index_lits_clash(c)
 *
 *    See index_lits_clash.
 *
 *************/

void un_index_lits_clash(struct clause *c)
{
  struct literal *lit;

  lit = c->first_lit;
  while (lit != NULL) {
    if (lit->atom->varnum == ANSWER || lit->atom->varnum == EVALUABLE)
      ;  /* skip answer literals and evaluable literals */
    else if (lit->sign)
      fpa_delete(lit->atom, Parms[FPA_LITERALS].val, Fpa_clash_pos_lits);
    else
      fpa_delete(lit->atom, Parms[FPA_LITERALS].val, Fpa_clash_neg_lits);
    if (Flags[PARA_FROM].val || Flags[PARA_INTO].val)
      un_index_paramod(lit->atom);
    lit = lit->next_lit;
    if (Flags[HYPER_SYMMETRY_KLUDGE].val)
      break;
  }
}  /* un_index_lits_clash */

