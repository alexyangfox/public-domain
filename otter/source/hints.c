/*
 *  hints.c -- routines related to the hints strategy.
 *
 */

/*

The main purpose of the hints mechanism is to set or adjust the
pick-given weight of clauses.  A hint H can apply to a clause C
in 3 ways:
    H subsumes C (forward subsume, fsub),
    C subsumes H (back subsume, bsub), and
    H is equivalent to C (equiv, which implies fsub and bsub).

2 more ways, which apply to unit clauses only, to be added later:
    H and C unify
    H anc C have the same shape (identical-except-variables)

Another purpose of hints is to retain clauses that would 
otherwise be discarded because the purge-gen weight is
too high.  The Flag KEEP_HINT_SUBSUMERS (default clear)
says to skip the purge-gen test on generated clauses that
subsume hints (i.e., bsub).

The Parms are

FSUB_HINT_WT    FSUB_HINT_ADD_WT  
BSUB_HINT_WT	BSUB_HINT_ADD_WT  
EQUIV_HINT_WT	EQUIV_HINT_ADD_WT 

These can be overridden for individual hints with  corresponding
attributes on the hints, e.g.,

p0(a,x)    # bsub_hint_wt(200)  # fsub_hint_wt(100).

If the Parms (attributes) are not set, they are not used; if you
have a list of hints with no attributes, and you don't set any
hint parms, the hints won't be used for anything.

If more than Parm (attribute) might apply, equiv is tested
first, then fsub, then bsub.  If you use both WT and ADD_WT,
then BOTH can apply, e.g., when the hint

p  # bsub_hint_wt(200)  # bsub_hint_add_wt(20).

applies to a clause, the clause gets pick-given weight 220.

The hint attributes and parameters are compiled into a special
structure that is attached to the hint clause with the parents
pointer.  This causes several problems.  (1) Compiled hints
must be printed with print_hint_clause() instead of print_clause(),
and (2) the Parms in effect at the start of the search are
compiled in; if the user changes hint parms during the search,
this will have no effect.

*/

#include "header.h"

/* Any hint-wt attributes on hints are compiled into a hint_data
 * structure (attached to the hint with parent pointer) 
 * to avoid costly attribute processing when using the hints.
 */

struct hint_data {
  char fsub, bsub, equiv;
  int fsub_wt, bsub_wt, equiv_wt;
  int fsub_add_wt, bsub_add_wt, equiv_add_wt;
};

/*************
 *
 *   process_hint_attributes()
 *
 *   Look for hint-wt attributes, put the data into a hint_data node.
 *
 *************/

static void process_hint_attributes(struct clause *c)
{
  struct cl_attribute *a;
  struct hint_data *hd;

  hd = (struct hint_data *) tp_alloc((int) sizeof(struct hint_data));
  c->parents = (struct ilist *) hd;

  hd->fsub = 0;
  hd->fsub_wt = Parms[FSUB_HINT_WT].val;
  hd->fsub_add_wt = Parms[FSUB_HINT_ADD_WT].val;
  hd->bsub = 0;
  hd->bsub_wt = Parms[BSUB_HINT_WT].val;
  hd->bsub_add_wt = Parms[BSUB_HINT_ADD_WT].val;
  hd->equiv = 0;
  hd->equiv_wt = Parms[EQUIV_HINT_WT].val;
  hd->equiv_add_wt = Parms[EQUIV_HINT_ADD_WT].val;

  for (a = c->attributes; a; a = a->next) {
    switch (a->name) {
    case FSUB_HINT_WT_ATTR:
      hd->fsub_wt = a->u.i; break;
    case BSUB_HINT_WT_ATTR:
      hd->bsub_wt = a->u.i; break;
    case EQUIV_HINT_WT_ATTR:
      hd->equiv_wt = a->u.i; break;
    case FSUB_HINT_ADD_WT_ATTR:
      hd->fsub_add_wt = a->u.i; break;
    case BSUB_HINT_ADD_WT_ATTR:
      hd->bsub_add_wt = a->u.i; break;
    case EQUIV_HINT_ADD_WT_ATTR:
      hd->equiv_add_wt = a->u.i; break;
    }
  }

  hd->fsub  = !(hd->fsub_wt  == MAX_INT && hd->fsub_add_wt  == 0);
  hd->bsub  = !(hd->bsub_wt  == MAX_INT && hd->bsub_add_wt  == 0);
  hd->equiv = !(hd->equiv_wt == MAX_INT && hd->equiv_add_wt == 0);

  if (hd->fsub == 0 && hd->bsub == 0 && hd->equiv == 0 &&
      !Flags[KEEP_HINT_SUBSUMERS].val && !Flags[KEEP_HINT_EQUIVALENTS].val) {

    printf("\n%cWARNING, hint will not be used, because no weights have been\nset for it: ", Bell);
    print_hint_clause(stdout, c);
  }
}  /* process_hint_attributes */

/*************
 *
 *  compile_hints
 *
 *************/

void compile_hints(void)
{
  struct clause *h;
  for (h = Hints->first_cl; h != NULL; h = h->next_cl)
    process_hint_attributes(h);
}  /* compile_hints2 */

/*************
 *
 *   print_hint_clause()
 *
 *   (Not in the same form as they were input.)
 *
 *************/

void print_hint_clause(FILE *fp,
		       struct clause *c)
{
  struct hint_data *hd;
  struct term *t;

  hd = (struct hint_data *) c->parents;

  fprintf(fp, "%d [", c->id);
  if (hd && hd->fsub)
    fprintf(fp, " fsub_wt=%d", hd->fsub_wt);
  if (hd && hd->bsub)
    fprintf(fp, " bsub_wt=%d", hd->bsub_wt);
  if (hd && hd->equiv)
    fprintf(fp, " equiv_wt=%d", hd->equiv_wt);
  fprintf(fp, "] ");

  t = clause_to_term(c);
  t = term_fixup_2(t);  /* Change -(=(a,b)) to !=(a,b). */
  print_term(fp, t);
  zap_term(t);

  if (c->attributes)
    print_attributes(fp, c->attributes);

  fprintf(fp, ".\n");
    
}  /* print_hint_clause */

/*************
 *
 *   p_hint_clause()
 *
 *************/

void p_hint_clause(struct clause *c)
{
  print_hint_clause(stdout, c);
}  /* p_hint_clause */

/*************
 *
 *   print_hints_cl_list()
 *
 *************/

void print_hints_cl_list(FILE *fp,
			 struct list *lst)
{
  struct clause *cl;

  if (!lst)
    fprintf(fp, "(hints list nil)\n");
  else {
    cl = lst->first_cl;
    while (cl) {
      print_hint_clause(fp, cl);
      cl = cl->next_cl;
    }
    fprintf(fp, "end_of_list.\n");
  }
}  /* print_hints_cl_list */

/*************
 *
 *   p_hints_cl_list()
 *
 *************/

void p_hints_cl_list(struct list *lst)
{
  print_hints_cl_list(stdout, lst);
}  /* p_hints_cl_list */

/*************
 *
 *   adjust_weight_with_hints()
 *
 *   This routine uses the list Hints to adjust or reset 
 *   the pick_weight of a clause.
 *
 *   Traverse the hints, looking for one that "matches" the
 *   clause.  If a match is found, change the pick_weight
 *   of the clause.
 *
 *************/

void adjust_weight_with_hints(struct clause *c)
{
  int f_test, b_test, e_test;
  int fsub, bsub, done;
  struct hint_data *hd;
  struct clause *h;

  CLOCK_START(HINTS_TIME);

  h = Hints->first_cl;
  done = 0;
  while (h && !done) {

    hd = (struct hint_data *) h->parents;

    f_test = hd->fsub;
    b_test = hd->bsub;
    e_test = hd->equiv;

    if (f_test || e_test)
      fsub = subsume(h, c);
    else
      fsub = 0;

    if (b_test || e_test)
      bsub = subsume(c, h);
    else
      bsub = 0;

    if (e_test && fsub && bsub) {
      if (hd->equiv_wt != MAX_INT)
	c->pick_weight = hd->equiv_wt;
      c->pick_weight += hd->equiv_add_wt;
      done = 1;
    }
    else if (f_test && fsub) {
      if (hd->fsub_wt != MAX_INT)
	c->pick_weight = hd->fsub_wt;
      c->pick_weight += hd->fsub_add_wt;
      done = 1;
    }
    else if (b_test && bsub) {
      if (hd->bsub_wt != MAX_INT)
	c->pick_weight = hd->bsub_wt;
      c->pick_weight += hd->bsub_add_wt;
      done = 1;
    }

    if (!done)
      h = h->next_cl;
  }

  if (done) {
    /* The clause gets the label of the hint. */
    struct cl_attribute *a1;
    if ((a1 = get_attribute(h, LABEL_ATTR))) {
      set_attribute(c, LABEL_ATTR, (void *) a1->u.s);
    }
  }
  CLOCK_STOP(HINTS_TIME);
}  /* adjust_weight_with_hints */

/*************
 *
 *   hint_keep_test()
 *
 *   We might want to speed this up with indexing, because it will be
 *   called with all generated clauses (if KEEP_HINT_SUBSUMERS is set).
 *
 *************/

int hint_keep_test(struct clause *c)
{
  struct clause *h;
  int ok = 0;

  /* Note that KEEP_HINT_SUBSUMERS is ignored when
     KEEP_HINT_EQUIVALENTS is set.
  */

  CLOCK_START(HINTS_KEEP_TIME)
  if (Flags[KEEP_HINT_EQUIVALENTS].val) {
    for (h = Hints->first_cl; h && !ok; h = h->next_cl)
      ok = subsume(c, h) && subsume(h, c);
  }
  else if (Flags[KEEP_HINT_SUBSUMERS].val) {
    for (h = Hints->first_cl; h && !ok; h = h->next_cl)
      ok = subsume(c, h);
  }
  CLOCK_STOP(HINTS_KEEP_TIME)

  return(ok);
}  /* hint_keep_test */
