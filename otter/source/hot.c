/*
 *  hot.c -- This file has routines hot-list inference.
 *
 */

#include "header.h"

/* There are separate indexes for hot-list inference.  They are analogous to
 * the ordinary FPA indexes defined in header.h.  They are used in
 * a kludgey way.
 *
 * The indexing and inference operations refer to global pointers
 * to the FPA indexes.  (The indexes should be parameters, but they aren't.)
 *
 * In init_hot_list, pointers to the real FPA indexes are saved.
 * During hot-list operations (index and inference) the ordinary
 * indexes are temporarily replaced with the hot-list indexes;
 * after the operation, the real fpa indexes are restored.
 */

/* These are the hot-list indexes. */
 
static struct fpa_index *Hot_clash_pos_lits;
static struct fpa_index *Hot_clash_neg_lits;
static struct fpa_index *Hot_alphas;
static struct fpa_index *Hot_clash_terms;

/* These are the ordinary indexes. */

static struct fpa_index *Ordinary_clash_pos_lits;
static struct fpa_index *Ordinary_clash_neg_lits;
static struct fpa_index *Ordinary_alphas;
static struct fpa_index *Ordinary_clash_terms;

/*************
 *
 *   init_hot() -- initialize for hot-list inference.
 *
 *************/

void init_hot(void)
{
  Ordinary_clash_pos_lits = Fpa_clash_pos_lits;
  Ordinary_clash_neg_lits = Fpa_clash_neg_lits;
  Ordinary_clash_terms    = Fpa_clash_terms;   
  Ordinary_alphas         = Fpa_alphas;   

  Hot_clash_pos_lits = alloc_fpa_index();
  Hot_clash_neg_lits = alloc_fpa_index();
  Hot_clash_terms    = alloc_fpa_index();
  Hot_alphas         = alloc_fpa_index();

}  /* init_hot */

/*************
 *
 *   heat_is_on()
 *
 *   Inference rules need to know if they are doing hot inference
 *   so that they can set the heat level of generated clauses.
 *   This kludgy routine is used instead of passing a parameter
 *   to all the inference routines.
 *
 *************/

int heat_is_on(void)
{
  /* just check any one of the indexes */
  return(Fpa_alphas == Hot_alphas);
}  /* heat_is_on */

/*************
 *
 *   switch_to_hot_index()
 *
 *************/

void switch_to_hot_index(void)
{
  Fpa_clash_pos_lits = Hot_clash_pos_lits;
  Fpa_clash_neg_lits = Hot_clash_neg_lits;
  Fpa_clash_terms    = Hot_clash_terms;
  Fpa_alphas         = Hot_alphas;
}  /* switch_to_hot_index */

/*************
 *
 *   switch_to_ordinary_index()
 *
 *************/

void switch_to_ordinary_index(void)
{
  Fpa_clash_pos_lits = Ordinary_clash_pos_lits;
  Fpa_clash_neg_lits = Ordinary_clash_neg_lits;
  Fpa_clash_terms    = Ordinary_clash_terms;    
  Fpa_alphas         = Ordinary_alphas;    
}  /* switch_to_ordinary_index */

/*************
 *
 *   hot_index_clause(c)
 *
 *   Index a clause for hot inference.
 *
 *************/

void hot_index_clause(struct clause *c)
{
  int already_hot = heat_is_on();

  if (!already_hot)
    switch_to_hot_index();
  index_lits_clash(c);
  if (!already_hot)
    switch_to_ordinary_index();
}  /* hot_index_clause */

/*************
 *
 *   hot_dynamic(c)
 *
 *   Insert a copy of c into the hot list.
 *
 *************/

void hot_dynamic(struct clause *c)
{
  struct clause *hc;

  hc = cl_copy(c);
  hc->parents = copy_ilist(c->parents);
  hc->heat_level = c->heat_level;
  hot_cl_integrate(hc);

  if (!Hot->first_cl)
    init_hot();

  hot_index_clause(hc);
  append_cl(Hot, hc);
  Stats[HOT_SIZE]++;
  printf("\nHOT NEW CLAUSE!: "); print_clause(stdout, hc); printf("\n");

}  /* hot_dynamic */

/*************
 *
 *    hot_mark_clash(r)
 *
 *    See hot_mark_clash_cl below.
 *
 *************/

static void hot_mark_clash(struct rel *r)
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
      if (t->type == COMPLEX) {
	if (Flags[PARA_SKIP_SKOLEM].val == 0 || is_skolem(t->sym_num) == 0) {
	  r = t->farg;
	  while (r != NULL) {
	    hot_mark_clash(r);
	    if (Flags[PARA_ONES_RULE].val)
	      r = NULL;
	    else
	      r = r->narg;
	  }
	}
      }
    }
  }
}  /* hot_mark_clash */

/*************
 *
 *    hot_unmark_clash(r)
 *
 *    See hot_mark_clash_cl below.
 *
 *************/

static void hot_unmark_clash(struct rel *r)
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
      if (t->type == COMPLEX) {
	if (Flags[PARA_SKIP_SKOLEM].val == 0 || is_skolem(t->sym_num) == 0) {
	  r = t->farg;
	  while (r != NULL) {
	    hot_unmark_clash(r);
	    if (Flags[PARA_ONES_RULE].val)
	      r = NULL;
	    else
	      r = r->narg;
	  }
	}
      }
    }
  }

}  /* hot_unmark_clash */

/*************
 *
 *    hot_mark_clash_cl(c, mark)
 *
 *    This is used for hot paramoudulation into the new clause.
 *    The clashable subterms have to be marked so that the ordinary
 *    para_into will work.   This is similar to (un)index_mark_clash
 *    in index.c, but it marks/unmarks only---no indexing is done.
 *
 *************/

void hot_mark_clash_cl(struct clause *c,
		       int mark)
{
  struct literal *lit;

  for (lit = c->first_lit; lit; lit = lit->next_lit) {

    if (!Flags[PARA_INTO_UNITS_ONLY].val || unit_clause(c)) {
      if (eq_lit(lit)) {
	if (Flags[PARA_INTO_LEFT].val) {
	  if (mark)
	    hot_mark_clash(lit->atom->farg);
	  else
	    hot_unmark_clash(lit->atom->farg);
	}
	if (Flags[PARA_INTO_RIGHT].val) {
	  if (mark)
	    hot_mark_clash(lit->atom->farg->narg);
	  else
	    hot_unmark_clash(lit->atom->farg->narg);
	}
      }
      else {
	struct rel *r;
	for (r = lit->atom->farg; r; r = r->narg)
	  if (mark)
	    hot_mark_clash(r);
	  else
	    hot_unmark_clash(r);
      }
    }
  }
}  /* hot_mark_clash_cl */

/*************
 *
 *   hot_inference(new_cl)
 *
 *************/

void hot_inference(struct clause *new_cl)
{
  if (new_cl->heat_level < Parms[HEAT].val) {

    CLOCK_START(HOT_TIME);

    switch_to_hot_index();  /* Swap in hot indexes. */

    if (Flags[BINARY_RES].val)
      bin_res(new_cl);

    if (Flags[HYPER_RES].val)
      hyper_res(new_cl);

    if (Flags[NEG_HYPER_RES].val)
      neg_hyper_res(new_cl);

    if (Flags[UR_RES].val)
      ur_res(new_cl);

    if (Flags[PARA_INTO].val) {

      /* only need the clash marks, not the indexing */
	     
      hot_mark_clash_cl(new_cl, 1);
      para_into(new_cl);
      hot_mark_clash_cl(new_cl, 0);
    }

    if (Flags[PARA_FROM].val)
      para_from(new_cl);

    switch_to_ordinary_index(); /* Restore the ordinary indexes. */

    CLOCK_STOP(HOT_TIME);

  }
}  /* hot_inference */

