/*
 *  hints2.c -- The second hints mechanism  Nov 5, 2002.
 *
 */

/* This is a (restricted) implementation of hints that uses indexing.
 * The restriction: it only does back subsumption of hints.  Otherwise,
 * it should work like the old hints code.
 *
 * This new version has many routines similar to the old, but with "hints2" 
 * or "hint2" in the name of the routine.
 */


#include "header.h"

struct hint2_data {
  int bsub_wt;
  int bsub_add_wt;
};

static struct fpa_index *Fpa_hints2_lits = NULL;

/* compiled-in options */

#define KEEP_BACK_DEMODULATED_HINTS 1

/*************
 *
 *  first_nonanswer_lit(clause)
 *
 *************/

static struct literal *first_nonanswer_lit(struct clause *c)
{
  if (c == NULL)
    return NULL;
  else {
    struct literal *lit = c->first_lit;
    while (lit && answer_lit(lit))
      lit = lit->next_lit;
    return lit;
  }
}  /* first_nonanswer_lit */

/*************
 *
 *  next_nonanswer_lit(literal)
 *
 *************/

static struct literal *next_nonanswer_lit(struct literal *lit)
{
  if (lit == NULL)
    return NULL;
  else {
    lit = lit->next_lit;
    while (lit && answer_lit(lit))
      lit = lit->next_lit;
    return lit;
  }
}  /* next_nonanswer_lit */

/*************
 *
 *  zap_ilist
 *
 *************/

static void zap_ilist(struct ilist *p)
{
  if (p != NULL) {
    zap_ilist(p->next);
    free_ilist(p);
  }
}  /* zap_ilist */

/*************
 *
 *  copy_hint2_data
 *
 *************/

static struct hint2_data *copy_hint2_data(struct hint2_data *p)
{
  struct hint2_data *p2 = malloc(sizeof(struct hint2_data));
  p2->bsub_wt = p->bsub_wt;
  p2->bsub_add_wt = p->bsub_add_wt;
  return p2;
}  /* copy_hint2_data */

/*************
 *
 *   print_hint2_clause()
 *
 *   (Not in the same form as they were input.)
 *
 *************/

void print_hint2_clause(FILE *fp,
		       struct clause *c)
{
  struct hint2_data *hd;
  struct term *t;

  hd = (struct hint2_data *) c->parents;

  fprintf(fp, "%d [", c->id);
  if (hd)
    fprintf(fp, "bsub_wt=%d, bsub_add_wt=%d", hd->bsub_wt, hd->bsub_add_wt);
  fprintf(fp, "] ");

  t = clause_to_term(c);
  t = term_fixup_2(t);  /* Change -(=(a,b)) to !=(a,b). */
  print_term(fp, t);
  zap_term(t);

  if (c->attributes)
    print_attributes(fp, c->attributes);

  fprintf(fp, ".\n");
    
}  /* print_hint2_clause */

/*************
 *
 *   print_hints2_cl_list()
 *
 *************/

void print_hints2_cl_list(FILE *fp,
			 struct list *lst)
{
  struct clause *cl;

  if (!lst)
    fprintf(fp, "(hints2 list nil)\n");
  else {
    fprintf(fp, "list(hints2).\n");
    cl = lst->first_cl;
    while (cl) {
      print_hint2_clause(fp, cl);
      cl = cl->next_cl;
    }
    fprintf(fp, "end_of_list.\n");
  }
}  /* print_hints2_cl_list */

/*************
 *
 *   hint2_integrate(c)
 *
 *   Integrate a hint clause.  This orders equalities before calling
 *   the ordinary clause integration routine.
 *
 *************/

void hint2_integrate(struct clause *h)
{
  if (Flags[ORDER_EQ].val) {
    if (Flags[LRPO].val)
      order_equalities_lrpo(h);
    else
      order_equalities(h);
  }
  cl_integrate(h);
}  /* hint2_integrate */

/*************
 *
 *  index_hint2(clause)
 *
 *  This inserts (or deletes) the first literal of the clause into the
 *  FPA hints2 index. If back demodulation is enabled for hints, it is
 *  handled by the ordinary indexing for back demodulation.
 *
 *************/

static void index_hint2(struct clause *c, int insert)
{
  struct literal *lit = first_nonanswer_lit(c);
  while (lit != NULL) {
    if (insert)
      fpa_insert(lit->atom, Parms[FPA_LITERALS].val, Fpa_hints2_lits);
    else
      fpa_delete(lit->atom, Parms[FPA_LITERALS].val, Fpa_hints2_lits);
    lit = next_nonanswer_lit(lit);
  }
}  /* index_hint2 */

/*************
 *
 *   process_hint2_attributes()
 *
 *   This "compiles" a hint:  (1) Any hint-related attributes on the
 *   clause are put into a hint2_data structure, which is attached
 *   to the hint by borrowing the hint's parent pointer.  (2) The hint
 *   is indexed so that it can be found quickly.
 *
 *************/

static void process_hint2_attributes(struct clause *c)
{
  struct cl_attribute *a;
  struct hint2_data *hd;
  int can_be_used = 0;

  hd = malloc(sizeof(struct hint2_data));
  c->parents = (struct ilist *) hd;

  hd->bsub_wt = Parms[BSUB_HINT_WT].val;
  hd->bsub_add_wt = Parms[BSUB_HINT_ADD_WT].val;

  for (a = c->attributes; a; a = a->next) {
    switch (a->name) {
    case BSUB_HINT_WT_ATTR:
      hd->bsub_wt = a->u.i; break;
    case BSUB_HINT_ADD_WT_ATTR:
      hd->bsub_add_wt = a->u.i; break;
    }
  }

  can_be_used = !(hd->bsub_wt  == MAX_INT && hd->bsub_add_wt  == 0);

  if (!can_be_used && !Flags[KEEP_HINT_SUBSUMERS].val) {
    printf("\n%cWARNING, hint cannot be used, because no weights have been\nset for it: ", Bell);
    print_hint2_clause(stdout, c);
  }
  index_hint2(c, 1);  /* second arg says "insert" */
}  /* process_hint2_attributes */

/*************
 *
 *  compile_hints2 - compile a list of hints.
 *
 *************/

void compile_hints2(void)
{
  struct clause *h;

  Fpa_hints2_lits = alloc_fpa_index();

  for (h = Hints2->first_cl; h != NULL; h = h->next_cl)
    process_hint2_attributes(h);
}  /* compile_hints2 */

/*************
 *
 *  find_hint2(clause)
 *
 *  This looks for a hint that "matches" a clause.
 *  For now, "matches" means that the clause subsumes the hint.
 *
 *************/

struct clause *find_hint2(struct clause *c)
{
  struct literal *lit = first_nonanswer_lit(c);
  if (lit != NULL) {
    struct fpa_tree *ut = build_tree(lit->atom,
				     INSTANCE,
				     Parms[FPA_LITERALS].val,
				     Fpa_hints2_lits);
    struct term *fatom = next_term(ut, 0);
    while (fatom != NULL) {
      struct literal *flit = fatom->occ.lit;
      if (lit->sign == flit->sign) {
	struct clause *hint = flit->container;
	if (subsume(c, hint)) {
	  zap_prop_tree(ut);
	  return hint;
	}
      }
      fatom = next_term(ut, 0);
    }
  }
  return NULL;
}  /* find_hint2 */

/*************
 *
 *   adjust_weight_with_hints2()
 *
 *   This routine uses the list Hints2 to adjust or reset 
 *   the pick_weight of a clause.
 *
 *   Look for a hint that "matches" the clause.  If a match
 *   is found, change the pick_weight of the clause.
 *
 *   Also, if the matching hint has a label, the label is copied
 *   to the clause.
 *
 *************/

void adjust_weight_with_hints2(struct clause *c)
{
  struct clause *hint;

  CLOCK_START(HINTS_TIME);
  hint = find_hint2(c);
  if (hint != NULL) {
    struct hint2_data *hd = (struct hint2_data *) hint->parents;
    if (hd->bsub_wt != MAX_INT)
      c->pick_weight = hd->bsub_wt;
    c->pick_weight += hd->bsub_add_wt;
    /* If the hint has a label, copy it to the clause. */
    {
      struct cl_attribute *a1;
      if ((a1 = get_attribute(hint, LABEL_ATTR))) {
	set_attribute(c, LABEL_ATTR, (void *) a1->u.s);
      }
    }
    if (Flags[DEGRADE_HINTS2].val) {
      /* This is Bob Veroff's hint degradation strategy.  It addresses
	 the problem of a hint matching MANY different clauses, which
	 happens more than you might think.  This solution: when a hint
	 gets used, make the hint weaker by cutting in half its
	 bsub_add_wt.  This assumes that the bsub_add_wt starts
	 as a large negative number.  (Bob has ideas about how
	 to generalize this.)
      */
	 
      hd -> bsub_add_wt /= 2;
      printf("+++ bsub adjust, cl %d, new wt %d\n",
	     hint->id, hd -> bsub_add_wt);
    }
  }
  CLOCK_STOP(HINTS_TIME);
}  /* adjust_weight_with_hints2 */

/*************
 *
 *   hint2_keep_test(clause) 
 *
 *   Return TRUE iff KEEP_HINT_SUBSUMERS is set and there is a hint
 *   that "matches" the clause.
 *
 *************/

int hint2_keep_test(struct clause *c)
{
  if (!Flags[KEEP_HINT_SUBSUMERS].val)
    return 0;
  else {
    struct clause *hint;
    CLOCK_START(HINTS_KEEP_TIME);
    hint = find_hint2(c);
    CLOCK_STOP(HINTS_KEEP_TIME);
    return(hint != NULL);
  }
}  /* hint2_keep_test */

/*************
 *
 *  all_containing_hints2(t, cpp) - insert containing clauses of t into *cpp
 *
 *  Given a term t, find all containing clauses that are in the Hints2 list.
 *  For each, insert the clause into the cpp list.
 *
 *************/

static void all_containing_hints2(struct term *t,
				  struct clause_ptr **cpp)
{
  struct rel *r;
  struct clause *c;
  struct list *l;

  if (t->type != VARIABLE && t->varnum != 0) {  /* atom */
    c = t->occ.lit->container;
    l = c->container;
    if (l == Hints2)
      insert_clause(c, cpp);
  }
  else {  /* term */
    r = t->occ.rel;
    while (r) {
      all_containing_hints2(r->argof, cpp);
      r = r->nocc;
    }
  }
}  /* all_containing_hints2 */

/*************
 *
 *  xx_tautology - does a clause have a literal that is an instance of x=x?
 *
 *  This is a pre-subsumption check.
 *
 *************/

static int xx_tautology(struct clause *c)
{
  struct literal *lit;

  lit = c->first_lit;
  while (lit) {
    struct term *atom = lit->atom;
    if (lit->sign &&
	is_eq(atom->sym_num) &&
	term_ident(atom->farg->argval, atom->farg->narg->argval)) {
      return(1);
    }
    lit = lit->next_lit;
  }
  return(0);
}  /* xx_tautology */

/*************
 *
 *  hint2_subsumed(c)
 *
 *  Use the FPA hint index to look for a hint that subsumes clause c.
 *
 *  This is not complete, because we only consider the first literal of c.
 *  For example, given clause p|q|r, we will not find hint q|r.
 *
 *************/

static struct clause *hint2_subsumed(struct clause *c)
{
  struct literal *lit = first_nonanswer_lit(c);
  if (lit != NULL) {
    struct fpa_tree *ut = build_tree(lit->atom,
				     MORE_GEN,
				     Parms[FPA_LITERALS].val,
				     Fpa_hints2_lits);
    struct term *fatom = next_term(ut, 0);
    while (fatom != NULL) {
      struct literal *flit = fatom->occ.lit;
      if (lit->sign == flit->sign) {
	struct clause *hint = flit->container;
	if (subsume(hint, c)) {
	  zap_prop_tree(ut);
	  return hint;
	}
      }
      fatom = next_term(ut, 0);
    }
  }
  return NULL;
}  /* hint2_subsumed */

/*************
 *
 *  back_demod_hints(clause)
 *
 *  This uses the ordinary back demodulation indexing.  This works
 *  because hints are integrated into the ordinary shared structures.
 *
 *************/

void back_demod_hints(struct clause *d)
{
  struct term *atom;
  struct term_ptr *tp, *tp2;
  struct clause_ptr *cp, *cp2;

  /* return; */

  atom = ith_literal(d,1)->atom;

  if (Flags[INDEX_FOR_BACK_DEMOD].val)
    tp = all_instances_fpa(atom, Fpa_back_demod);
  else
    tp = all_instances(atom);

  cp = NULL;
  while (tp) {
    all_containing_hints2(tp->term, &cp);
    tp2 = tp;
    tp = tp->next;
    free_term_ptr(tp2);
  }

  while (cp) {
    struct clause *h1, *h2;
    h1 = cp->c;

    if (h1->container != Hints2)
      abend("back_demod_hints: clause not in Hints2.");

    /* Stats[CL_BACK_DEMOD]++; */

    h2 = cl_copy(h1);
    demod_cl(h2);
    
    if (1 || Flags[PRINT_BACK_DEMOD].val) {
      printf("    >> BACK DEMODULATING HINT %d WITH %d.\n", h1->id, d->id);
      /* printf("before: "); print_hint2_clause(stdout, h1); */
      /* printf("after:  "); p_clause(h2); */
    }

    /* copy hint data to new hint */

    zap_ilist(h2->parents);

    h2->parents = (struct ilist *) copy_hint2_data((void *) h1->parents);

    /* If the hint has a label, copy it to the clause. */
    {
      struct cl_attribute *a1;
      if ((a1 = get_attribute(h1, LABEL_ATTR))) {
	/* set_attribute(h2, LABEL_ATTR, (void *) a1->u.s); */
	set_attribute(h2, LABEL_ATTR, "bd");
      }
    }

    if (!KEEP_BACK_DEMODULATED_HINTS) {
      rem_from_list(h1);
      index_hint2(h1, 0);  /* second arg says "delete" */
      free(h1->parents);   /* hint data */
      h1->parents = NULL;
      cl_del_int(h1);
    }

    if (Flags[ORDER_EQ].val) {
      if (Flags[LRPO].val)
	order_equalities_lrpo(h2);
      else
	order_equalities(h2);
    }

    if (xx_tautology(h2) || hint2_subsumed(h2)) {
      free(h2->parents);  /* hint data */
      h2->parents = NULL;
      cl_del_non(h2);
    }
    else {
      hint2_integrate(h2);
      append_cl(Hints2, h2);
      index_hint2(h2, 1);  /* second arg says "insert" */
      if (1 || Flags[PRINT_KEPT].val) {
	printf("NEW HINT: ");
	print_hint2_clause(stdout, h2);
      }
    }
    
    cp2 = cp;
    cp = cp->next;
    free_clause_ptr(cp2);
  }
}  /* back_demod_hints */

/*************
 *
 *  zap_hints2 - Free all memory associated with hints2.
 *
 *  This is used mostly to check for memory leaks.
 *
 *************/

void zap_hints2(void)
{
  struct clause *c;

  c = find_last_cl(Hints2);
  while (c) {
    rem_from_list(c);
    index_hint2(c, 0);    /* second arg says "delete" */
    free(c->parents);     /* hint data */
    c->parents = NULL;
    cl_del_int(c);
    c = find_last_cl(Hints2);
  }
  free_list(Hints2);
  Hints2 = NULL;

}  /* zap_hints2 */

