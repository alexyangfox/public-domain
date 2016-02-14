/*
 *  process.c -- Routines to handle the processing of generated clauses.
 *
 */

#include "header.h"

/*************
 *
 *    post_process(c, input, lst) -- finish processing a clause
 *
 *    The clause has already been integrated, indexed, appended to
 *    Sos.  This routine does back subsumption,
 *    and possibly generates more clauses (factoring, back demod, hot
 *    lists, etc.).  Any newly generated and kept clauses will be
 *    appended to lst and will wait their turn to be post_processed.
 *
 *************/

static void post_process(struct clause *c,
			 int input,
			 struct list *lst)
{
  struct clause *d, *e;
  struct clause_ptr *cp1, *cp2;
  struct literal *lit;

#if 0
  printf("Starting post: "); p_clause(c);
#endif

  if (Flags[EQ_UNITS_BOTH_WAYS].val && unit_clause(c)) {

    /* 
     *  Generate a flipped copy if 
     *     1. it's a (pos or neg) eq unit, and
     *     2. either
     *           a. order_eq is clear, or
     *           b. order_eq is set, and it couldn't be oriented.
     */
	
    lit = ith_literal(c, 1);

    if (eq_lit(lit) &&
	(!Flags[ORDER_EQ].val ||
	 !TP_BIT(lit->atom->bits, ORIENTED_EQ_BIT))) {

      struct term *t;
      struct clause *c2;
      struct ilist *ip1, *ip2, *ip3, *ip4, *ip5;
	    
      c2 = cl_copy(c);

      ip1 = get_ilist(); ip1->i = COPY_RULE; c2->parents = ip1;
      ip2 = get_ilist(); ip2->i = c->id; ip1->next = ip2;
      ip3 = get_ilist(); ip3->i = FLIP_EQ_RULE; ip2->next = ip3;
      ip4 = get_ilist(); ip4->i = LIST_RULE-1; ip3->next = ip4;
      ip5 = get_ilist(); ip5->i = 1; ip4->next = ip5;

      lit = ith_literal(c2, 1);
      t = lit->atom->farg->argval;
      lit->atom->farg->argval = lit->atom->farg->narg->argval;
      lit->atom->farg->narg->argval = t;
      CLOCK_STOP(POST_PROC_TIME);
      pre_process(c2, input, lst);
      CLOCK_START(POST_PROC_TIME);
    }
  }

  if (Flags[BACK_DEMOD].val && unit_clause(c)) {
    struct term *atom;
    atom = ith_literal(c,1)->atom;
    if (c->first_lit && TP_BIT(atom->bits, SCRATCH_BIT)) {
      /* c was made into a new demodulator */
      CLEAR_BIT(atom->bits, SCRATCH_BIT);
      d = cl_find(c->id + 1);  /* demod id is 1 more than clause id */
      if (Flags[PRINT_BACK_DEMOD].val || input)
	printf(">>>> Starting back demodulation with %d.\n", d->id);
      CLOCK_START(BACK_DEMOD_TIME);
      back_demod(d, c, input, lst);
      back_demod_hints(d);
      CLOCK_STOP(BACK_DEMOD_TIME);
    }
  }

  if (Flags[BACK_SUB].val) {
    CLOCK_START(BACK_SUB_TIME);
    cp1 = back_subsume(c);
    CLOCK_STOP(BACK_SUB_TIME);
    while (cp1 != NULL) {
      e = cp1->c;
      if (e->container != Passive) {
	Stats[CL_BACK_SUB]++;
	if (Flags[PRINT_BACK_SUB].val || input)
	  printf("%d back subsumes %d.\n", c->id, e->id);
	CLOCK_START(UN_INDEX_TIME);
	un_index_lits_all(e);
	if (e->container == Usable)
	  un_index_lits_clash(e);
	CLOCK_STOP(UN_INDEX_TIME);
	rem_from_list(e);
	hide_clause(e);
      }
      cp2 = cp1;
      cp1 = cp1->next;
      free_clause_ptr(cp2);
    }
  }

  if (Flags[FACTOR].val) {
    CLOCK_START(FACTOR_TIME);
    all_factors(c, lst);
    CLOCK_STOP(FACTOR_TIME);
  }

  if (Flags[GL_DEMOD].val) {
    gl_demod(c, lst);
  }

  if (Hot->first_cl && !input) {
    /* Don't hot-list input clauses. */
    CLOCK_STOP(POST_PROC_TIME);
    hot_inference(c);
    CLOCK_START(POST_PROC_TIME);
  }

  if (Flags[BACK_UNIT_DELETION].val && unit_clause(c)) {
    if (Flags[PRINT_BACK_DEMOD].val || input)
      printf(">>>> Starting back unit deletion with %d.\n", c->id);
    CLOCK_START(BACK_UNIT_DEL_TIME);
    back_unit_deletion(c, input);
    CLOCK_STOP(BACK_UNIT_DEL_TIME);
  }

}  /* post_process */

/*************
 *
 *    post_proc_all(lst_pos, input, lst)
 *
 *************/

void post_proc_all(struct clause *lst_pos,
		   int input,
		   struct list *lst)
{
  struct clause *c;

  CLOCK_START(POST_PROC_TIME);
  if (lst_pos == NULL)
    c = lst->first_cl;
  else
    c = lst_pos->next_cl;

  while (c != NULL) {
    struct clause *d;
    post_process(c, input, lst); /* this may alter c->next_cl */
    d = c;
    c = c->next_cl;
#if 1
    if (TP_BIT(d->bits, SCRATCH_BIT)) {
      CLEAR_BIT(d->bits, SCRATCH_BIT);
      rem_from_list(d);
      index_lits_clash(d);
      append_cl(Usable, d);
      /* printf("Clause %d moved to Usable.\n", d->id); */
    }
#endif
    /* following moved from end of infer_and_process 19 Jan 90 */
    if (Flags[REALLY_DELETE_CLAUSES].val)
      /* clauses hidden by back demod, back subsumption */
      /* also empty clauses are hidden */
      del_hidden_clauses();
  }

  CLOCK_STOP(POST_PROC_TIME);
}  /* post_proc_all */

/*************
 *
 *   given_clause_ok(id)
 *
 *************/

static int given_clause_ok(int id)
{
  struct clause *c;

  c = cl_find(id);
  if (c)
    return(c->container != NULL);
  else
    return(0);
}  /* given_clause_ok */

/*************
 *
 *    infer_and_process(giv_cl)
 *
 *    The inference rules append kept clauses to Sos.  After each
 *    inference rule is finished, the newly kept clauses are
 *    `post_process'ed (back subsump, back demod, etc.).
 *
 *************/

void infer_and_process(struct clause *giv_cl)
{
  struct clause *c, *sos_pos;
  struct ilist *ip;
  int given_id;

  if (Flags[CONTROL_MEMORY].val) {
    control_memory();
  }

  if (Parms[WARN_MEM].val != -1 && total_mem() > Parms[WARN_MEM].val) {
    int i = Parms[WARN_MEM_MAX_WEIGHT].val;
    if (Parms[MAX_WEIGHT].val != i) {
      Parms[MAX_WEIGHT].val = i;
      fprintf(stderr,"\nMemory warning: resetting max_weight to %d.\n\n",i);
      fprintf(stdout,"\nMemory warning: resetting max_weight to %d.\n\n",i);
      fflush(stdout);
    }
  }

  given_id = giv_cl->id;

  if (Flags[BINARY_RES].val && given_clause_ok(given_id)) {
    sos_pos = Sos->last_cl;  /* Save position of last clauses in Sos. */
    bin_res(giv_cl);  /* Inf rule appends newly kept clauses to Sos. */

    /* Now post_process new clauses in Sos. */
    /* (Post_process may append even more clauses to Sos. Do them all.) */
    /* (ROO does not do this.) */
    post_proc_all(sos_pos, 0, Sos);
  }

  /* For subsequent inference rules, check that the given clause  */
  /* has not back demodulated or back subsumed. */

  if (Flags[HYPER_RES].val && given_clause_ok(given_id)) {
    sos_pos = Sos->last_cl;
    hyper_res(giv_cl);
    post_proc_all(sos_pos, 0, Sos);

  }

  if (Flags[NEG_HYPER_RES].val && given_clause_ok(given_id)) {
    sos_pos = Sos->last_cl;
    neg_hyper_res(giv_cl);
    post_proc_all(sos_pos, 0, Sos);
  }

  if (Flags[UR_RES].val && given_clause_ok(given_id)) {
    sos_pos = Sos->last_cl;
    ur_res(giv_cl);
    post_proc_all(sos_pos, 0, Sos);
  }

  if (Flags[PARA_INTO].val && given_clause_ok(given_id)) {
    sos_pos = Sos->last_cl;
    para_into(giv_cl);
    post_proc_all(sos_pos, 0, Sos);
  }

  if (Flags[PARA_FROM].val && given_clause_ok(given_id)) {
    sos_pos = Sos->last_cl;
    para_from(giv_cl);
    post_proc_all(sos_pos, 0, Sos);
  }

  if (Flags[LINKED_UR_RES].val && given_clause_ok(given_id)) {
    sos_pos = Sos->last_cl;
    linked_ur_res(giv_cl);
    post_proc_all(sos_pos, 0, Sos);
  }

  if (Flags[LINKED_HYPER_RES].val && given_clause_ok(given_id)) {
    sos_pos = Sos->last_cl;
    linked_hyper_res(giv_cl);
    post_proc_all(sos_pos, 0, Sos);
  }

  if (Flags[DEMOD_INF].val && given_clause_ok(given_id)) {
    sos_pos = Sos->last_cl;
    c = cl_copy(giv_cl);
    ip = get_ilist();
    ip->i = giv_cl->id;
    c->parents = ip;
    Stats[CL_GENERATED]++;
    Stats[DEMOD_INF_GEN]++;
    pre_process(c, 0, Sos);
    post_proc_all(sos_pos, 0, Sos);
  }

  if (Flags[GEOMETRIC_RULE].val && given_clause_ok(given_id)) {
    sos_pos = Sos->last_cl;
    geometry_rule_unif(giv_cl);
    post_proc_all(sos_pos, 0, Sos);
  }

}  /* infer_and_process */

/*************
 *
 *    int proc_gen(c, input)
 *
 *    This is the main processing applied to generated clauses.
 *
 *    If (input), c is an input clause, and some tests should not be performed.
 *
 *    This routine takes a generated clause and (* means optional):
 *
 *       renumber variables
 *     * print the clause
 *     * gL rewriting
 *       demodulate, including $evaluation
 *     * gL rewriting
 *       handle evaluable literals
 *     * order equalities
 *     * unit_deletion
 *     * factor-simplify
 *       merge identical literals
 *     * max literals test (if not input)
 *     * max_distinct_vars check  (if not input)
 *       tautology check
 *     * max weight test  (if not input)
 *     * delete_identical_nested_skolems (if not input)
 *     * sort literals
 *     * forward subsumption
 *       renumber variables (again)
 *
 *    Return 0 if clause should be deleted.
 *
 *************/

int proc_gen(struct clause *c,
	     int input)
{
  struct clause *e;
  int wt, i;

  /* Renumbering variables:  Some of the processing requires variables
     to be in range (< MAX_VARS), so we renumber first.  Any of the
     processing that might introduce variables out of range should
     renumber again.  Some provessing can remove or reorder variables,
     so we renumber again at the end.  Renumbering can be slow, so
     this could be improved.
  */

  CLOCK_START(RENUMBER_TIME);
  if (renumber_vars(c) == 0) {
    Stats[CL_VAR_DELETES]++;
    CLOCK_STOP(RENUMBER_TIME);
    return(0);
  }
  CLOCK_STOP(RENUMBER_TIME);

  if (Flags[VERY_VERBOSE].val) {
    printf("\n  ");
    CLOCK_START(PRINT_CL_TIME);
    print_clause(stdout, c);
    CLOCK_STOP(PRINT_CL_TIME);
  }

  if (Flags[GEOMETRIC_REWRITE_BEFORE].val)
    i = geo_rewrite(c);

  if ((Demodulators->first_cl || Internal_flags[DOLLAR_PRESENT]) &&
      !Flags[GL_DEMOD].val) {
    CLOCK_START(DEMOD_TIME);
    demod_cl(c);
    CLOCK_STOP(DEMOD_TIME);
    if (Flags[VERY_VERBOSE].val) {
      printf("  after demodulation: ");
      CLOCK_START(PRINT_CL_TIME);
      print_clause(stdout, c);
      CLOCK_STOP(PRINT_CL_TIME);
    }
    /* If demodulation introduced new variables, the clause was renumbered,
       so so all vars are still < MAX_VARS.  However, they might no longer
       be in order.
    */
  }

  if (Flags[GEOMETRIC_REWRITE_AFTER].val)
    i = geo_rewrite(c);
	
  /* False lits of c may be deleted even if test fails. */
  if (Internal_flags[DOLLAR_PRESENT] && lit_t_f_reduce(c)) {
    Stats[CL_TAUTOLOGY]++;
    return(0);
  }

  if (Flags[ORDER_EQ].val) {
    CLOCK_START(ORDER_EQ_TIME);
    if (Flags[LRPO].val)
      order_equalities_lrpo(c);
    else
      order_equalities(c);
    CLOCK_STOP(ORDER_EQ_TIME);
    if (!input &&
	Flags[DISCARD_NON_ORIENTABLE_EQ].val &&
	unit_clause(c) &&
	num_literals_including_answers(c) == 1 &&
	pos_eq_lit(ith_literal(c, 1)) &&
	!TP_BIT(ith_literal(c, 1)->atom->bits, ORIENTED_EQ_BIT))
      return(0);
  }

  if (Flags[UNIT_DELETION].val && num_literals(c) > 1) {
    CLOCK_START(UNIT_DEL_TIME);
    i = unit_del(c);
    CLOCK_STOP(UNIT_DEL_TIME);
  }

  if (Flags[FACTOR].val) {
    CLOCK_START(FACTOR_SIMP_TIME);
    i = factor_simplify(c);
    CLOCK_STOP(FACTOR_SIMP_TIME);
    Stats[FACTOR_SIMPLIFICATIONS] += i;
  }

  /* I had to move cl_merge() after factor_simplify, because
   * build_proof_object() wants cl_merge() to be the last operation.
   */

  cl_merge(c);

  if (!input && Parms[MAX_LITERALS].val != -1) {
    if (num_literals(c) > Parms[MAX_LITERALS].val) {
      Stats[CL_WT_DELETE]++;
      return(0);
    }
  }

  if (!input && Parms[MAX_ANSWERS].val != -1) {
    if (num_answers(c) > Parms[MAX_ANSWERS].val) {
      Stats[CL_WT_DELETE]++;
      return(0);
    }
  }

  if (!input && Parms[MAX_DISTINCT_VARS].val != -1) {
    if (distinct_vars(c) > Parms[MAX_DISTINCT_VARS].val) {
      Stats[CL_WT_DELETE]++;
      return(0);
    }
  }

  if (!input && Flags[DISCARD_XX_RESOLVABLE].val && xx_resolvable(c)) {
    Stats[CL_WT_DELETE]++;
    return(0);
  }

  if (tautology(c)) {
    Stats[CL_TAUTOLOGY]++;
    return(0);
  }

  if (!input && Parms[MAX_WEIGHT].val != MAX_INT) {
    CLOCK_START(WEIGH_CL_TIME);
    wt = weight_cl(c, Weight_purge_gen_index);
    CLOCK_STOP(WEIGH_CL_TIME);

    if (wt > Parms[MAX_WEIGHT].val) {
      /* Do not delete if it hint_keep_test() says to keep it. */
      int hint_keep = 0;
      if (Internal_flags[HINTS_PRESENT])
	hint_keep = hint_keep_test(c);
      else if (Internal_flags[HINTS2_PRESENT])
	hint_keep = hint2_keep_test(c);

      if (hint_keep) {
	if (Flags[VERY_VERBOSE].val)
	  printf("  keeping clause because it matches a hint.\n");
      }
      else {
	if (Flags[VERY_VERBOSE].val)
	  printf("  deleted because weight=%d.\n", wt);
	Stats[CL_WT_DELETE]++;
	return(0);
      }
    }
  }

  if (!input && Flags[DELETE_IDENTICAL_NESTED_SKOLEM].val) {
    if (ident_nested_skolems(c)) {
      Stats[CL_WT_DELETE]++;
      return(0);
    }
  }

  if (Flags[SORT_LITERALS].val) {
    CLOCK_START(SORT_LITS_TIME);
    if (sort_lits(c) && input) {
      /* If an input clause is changed by sorting, make a note in
	 the justification.
      */
      struct ilist *p;
      struct ilist *ip = get_ilist();
      ip->i = PROPOSITIONAL_RULE;
      p = c->parents;
      if (p == NULL)
	c->parents = ip;
      else {
	while (p->next)
	  p = p->next;
	p->next = ip;
      }
    }
    CLOCK_STOP(SORT_LITS_TIME);
  }

  if (Flags[ORDER_EQ].val) {
    /* For each eq literal that has been flipped, add an entry to
     * the history.  To make sense, this has to be done after sort_lits.
     */
    struct ilist *ip1, *ip2, *ip3;
    struct literal *lit;
    int i;
    for(lit = c->first_lit, i = 1; lit; lit = lit->next_lit, i++) {
      if (TP_BIT(lit->atom->bits, SCRATCH_BIT)) {
	CLEAR_BIT(lit->atom->bits, SCRATCH_BIT);
	ip1 = get_ilist(); ip1->i = FLIP_EQ_RULE;
	ip2 = get_ilist(); ip2->i = LIST_RULE-1; ip1->next = ip2;
	ip3 = get_ilist(); ip3->i = i; ip2->next = ip3;
	if (c->parents) {
	  for (ip3 = c->parents; ip3->next; ip3 = ip3->next);
	  ip3->next = ip1;
	}
	else
	  c->parents = ip1;
      }
    }
  }

  if (Flags[FOR_SUB].val) {
    CLOCK_START(FOR_SUB_TIME);
    e = forward_subsume(c);
    CLOCK_STOP(FOR_SUB_TIME);
    if (e) {
      if (Flags[VERY_VERBOSE].val)
	printf("  Subsumed by %d.\n", e->id);
      else if (input) {
	printf("  Following clause subsumed by %d during input processing: ", e->id);
	print_clause(stdout, c);
      }
      Stats[CL_FOR_SUB]++;
      if (e->container == Sos)
	Stats[FOR_SUB_SOS]++;
      if (e->id < 100)
	Subsume_count[e->id]++;
      else
	Subsume_count[0]++;
      return(0);
    }
  }

  if (Flags[DISCARD_COMMUTATIVITY_CONSEQUENCES].val &&
      commutativity_consequence(c)) {
    Stats[CL_WT_DELETE]++;
    return(0);
  }

  CLOCK_START(RENUMBER_TIME);
  if (renumber_vars(c) == 0) {
    Stats[CL_VAR_DELETES]++;
    CLOCK_STOP(RENUMBER_TIME);
    return(0);
  }
  CLOCK_STOP(RENUMBER_TIME);

  return(1);

}  /* proc_gen */

/*************
 *
 *    pre_process(c, input, lst)
 *
 *************/

void pre_process(struct clause *c,
		 int input,
		 struct list *lst)
{
  int i;
  struct clause *e, *original_input;

  CLOCK_START(PRE_PROC_TIME);

  if (heat_is_on())  /* if c was generated by hot inference */
    Stats[HOT_GENERATED]++;

  if (!c->parents)
    original_input = cl_copy(c);
  else
    original_input = NULL;

  i = proc_gen(c, input);
  if (!i) {
    CLOCK_START(DEL_CL_TIME);
    cl_del_non(c);
    if (original_input)
      cl_del_non(original_input);
    CLOCK_STOP(DEL_CL_TIME);
    CLOCK_STOP(PRE_PROC_TIME);
    return;
  }

  if (original_input && c->parents) {
    /* When input clauses are changed (demod, unit_del, factor_simp,
     * sort_lits) during pre_process, we keep the original so that proofs
     * make sense (in particular, so that proof_objects make sense).
     */
    struct ilist *ip1, *ip2;
    cl_integrate(original_input);
    hide_clause(original_input);
    ip1 = get_ilist();
    ip2 = get_ilist();
    ip1->i = COPY_RULE;
    ip2->i = original_input->id;
    ip1->next = ip2;
    ip2->next = c->parents;
    c->parents = ip1;
  }

  CLOCK_START(KEEP_CL_TIME);
  cl_integrate(c);
  index_lits_all(c);
  if (lst == Usable)
    index_lits_clash(c);
  
  append_cl(lst, c);

  c->pick_weight = weight_cl(c, Weight_pick_given_index);

  if (Internal_flags[HINTS_PRESENT])
    adjust_weight_with_hints(c);
  else if (Internal_flags[HINTS2_PRESENT])
    adjust_weight_with_hints2(c);

  if (Parms[AGE_FACTOR].val != 0)
    c->pick_weight += (Stats[CL_GIVEN] / Parms[AGE_FACTOR].val);
  if (Parms[DISTINCT_VARS_FACTOR].val != 0)
    c->pick_weight += distinct_vars(c) * Parms[DISTINCT_VARS_FACTOR].val;

  Stats[CL_KEPT]++;
  if (c->heat_level > 0)
    Stats[HOT_KEPT]++;
  CLOCK_STOP(KEEP_CL_TIME);

  if (input || Flags[PRINT_KEPT].val) {
    printf("** KEPT (pick-wt=%d): ", c->pick_weight);
    CLOCK_START(PRINT_CL_TIME);
    print_clause(stdout, c);
    CLOCK_STOP(PRINT_CL_TIME);
  }

  if (Flags[DYNAMIC_DEMOD].val && 
      unit_clause(c) &&
      num_literals_including_answers(c) == 1 &&
      pos_eq_lit(ith_literal(c, 1))) {

    int demod_flag;

    CLOCK_START(NEW_DEMOD_TIME);
    demod_flag = dynamic_demodulator(c);
    if (demod_flag != 0) {
      /* make sure there are no calls to cl_integrate between
       * KEEP and here, because new_demod ID must be one more
       * than KEPT copy.  In particular, check_for_proof. */
      struct clause *d;
      d = new_demod(c, demod_flag);
      if (Flags[PRINT_NEW_DEMOD].val || input) {
	printf("---> New Demodulator: ");
	if (demod_flag == 2)
	  printf("(lex-dependent) ");
	print_clause(stdout, d);
      }
    }
    CLOCK_STOP(NEW_DEMOD_TIME);
  }

  CLOCK_START(CONFLICT_TIME);
  e = check_for_proof(c);
  CLOCK_STOP(CONFLICT_TIME);

  if (Parms[MAX_PROOFS].val != -1 &&
      Stats[EMPTY_CLAUSES] >= Parms[MAX_PROOFS].val) {
      
    if (!splitting() || current_case() == NULL) {
      fprintf(stderr, "\n%cSearch stopped by max_proofs option.\n\n", Bell);
      printf("\nSearch stopped by max_proofs option.\n");
      cleanup();
    }
    else {
      /* This is a case. */
      if (!Flags[REALLY_DELETE_CLAUSES].val) {
	/* Send assumptions used for refutation to the parent. */
	assumps_to_parent(e);
      }
      output_stats(stdout, Parms[STATS_LEVEL].val);
      printf("\nProcess %d finished %s", my_process_id(), get_time());
    }

    if (multi_justifications()) {
      struct clause *ee = proof_last_hidden_empty();
      multi_just_process(ee);
      output_stats(stdout, Parms[STATS_LEVEL].val);
    }

    exit(PROOF_EXIT);
  }

  if (!input && c->pick_weight <= Parms[DYNAMIC_HEAT_WEIGHT].val)
    hot_dynamic(c);  /* add to the hot list */

  CLOCK_STOP(PRE_PROC_TIME);

}  /* pre_process */

