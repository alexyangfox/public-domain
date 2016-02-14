/*
 *  resolve.c -- Resolution inference rules.
 *
 */

#include "header.h"

/*************
 *
 *    struct clause *build_hyper(clash,nuc_subst,nuc_lits,nuc,
 *                                     giv_subst,giv_lits,giv_sat,nuc_pos,sat_indexes)
 *
 *    This routine constructs a hyperresolvent or UR-resolvent.
 *
 *************/

static struct clause *build_hyper(struct clash_nd *cla,
				  struct context *nuc_subst,
				  struct literal *nuc_lits,
				  struct clause *nuc,
				  struct context *giv_subst,
				  struct literal *giv_lits,
				  struct clause *giv_sat,
				  int nuc_pos,
				  int sat_indexes)
{
  struct clause *res, *sat;
  struct literal *lit, *new, *prev;
  struct clash_nd *c;
  struct ilist *ip1, *ip2, *ip3, *ip4;
  int i, j;
  int n = 0;

  res = get_clause();

  ip1 = get_ilist(); /* to be filled in by caller with name of inference rule */
  res->parents = ip1;
  /* If given clause is satellite, add number to parent list. */
  ip3 = NULL;
  if (giv_sat) {
    ip2 = get_ilist();
    ip2->i = giv_sat->id;
    if (Flags[ORDER_HISTORY].val && nuc_pos != 0)
      /* insert later in correct position */
      ip3 = ip2;
    else {
      ip1->next = ip2;
      ip1 = ip2;
    }
  }

  ip2 = get_ilist();
  ip2->i = nuc->id;
  ip1->next = ip2;

  lit = giv_lits;
  prev = NULL;
  while (lit != NULL) {
    new = get_literal();
    new->container = res;
    if (prev == NULL)
      res->first_lit = new;
    else
      prev->next_lit = new;
    prev = new;
    new->sign = lit->sign;
    new->atom = apply(lit->atom, giv_subst);
    new->atom->occ.lit = new;
    new->atom->varnum = lit->atom->varnum;  /* copy type of atom */
    lit = lit->next_lit;
  }

  lit = nuc_lits;
  while (lit != NULL) {
    new = get_literal();
    new->container = res;
    if (res->first_lit == NULL)
      res->first_lit = new;
    else
      prev->next_lit = new;
    prev = new;
    new->sign = lit->sign;
    new->atom = apply(lit->atom, nuc_subst);
    new->atom->occ.lit = new;
    new->atom->varnum = lit->atom->varnum;  /* copy type of atom */
    lit = lit->next_lit;
  }

  c = cla;
  i = 1;
  while (c != NULL) {
    if (ip3 && i == nuc_pos) {
      /* insert given clause (which is satellite) number here */
      ip2->next = ip3;
      ip2 = ip3;
    }
    ip1 = get_ilist();
    ip2->next = ip1;
    ip2 = ip1;
    if (c->evaluable)
      ip1->i = EVAL_RULE;
    else {
      sat = c->found_atom->occ.lit->container;
      lit = sat->first_lit;
      j = 0;
      while (lit != NULL) {
	j++;
	if (lit->atom != c->found_atom) {
	  new = get_literal();
	  new->container = res;
	  if (res->first_lit == NULL)
	    res->first_lit = new;
	  else
	    prev->next_lit = new;
	  prev = new;
	  new->sign = lit->sign;
	  new->atom = apply(lit->atom, c->subst);
	  new->atom->occ.lit = new;
	  new->atom->varnum = lit->atom->varnum;  /* type of atom */
	}
	else
	  n = j;
	lit = lit->next_lit;
      }
      ip1->i = sat->id;
      if (sat_indexes) {
	ip1 = get_ilist();
	ip4 = get_ilist();
	ip1->next = ip4;
	ip2->next = ip1;
	ip1->i = LIST_RULE-1;
	ip4->i = n;
	ip2 = ip4;
      }
    }
    i++;
    c = c->next;
  }

  if (ip3 && i == nuc_pos) {
    /* insert given clause (which is satellite) number here */
    ip2->next = ip3;
    ip2 = ip3;
  }

  if (ip3 && sat_indexes) {
    struct literal *l1, *l2;
    ip1 = get_ilist();
    ip4 = get_ilist();
    ip1->next = ip4;
    ip4->next = ip3->next;
    ip3->next = ip1;
    ip1->i = LIST_RULE-1;
    /* Need index of clashed literal in given clause (which is satellite). */
    l1 = giv_sat->first_lit;
    l2 = giv_lits;
    i = 1;
    while (l2 && l1->sign == l2->sign && l1->atom == l2->atom) {
      l1 = l1->next_lit; l2 = l2->next_lit; i++;
    }
    ip4->i = i;
  }

  return(res);

}  /* build_hyper */

/*************
 *
 *   maximal_lit()
 *
 *   true iff no predicate symbol in clause has a higher lex val.
 *
 *************/

int maximal_lit(struct literal *l1)
{
  struct literal *l2;
  int i1;

  i1 = sn_to_node(l1->atom->sym_num)->lex_val;

  for (l2 = l1->container->first_lit; l2; l2 = l2->next_lit) {
    if (l2 != l1 && l2->atom->varnum != ANSWER) {
      if (l2->sign > l1->sign)
	return(0);
      else if (l2->sign == l1->sign &&
	       sn_to_node(l2->atom->sym_num)->lex_val > i1)
	return(0);
    }
  }
  return(1);
}  /* maximal_lit */

/*************
 *
 *    clash(c_start, nuc_subst, nuc_lits, nuc, giv_subst, giv_lits, giv_sat,
 *                  sat_proc, inf_clock, nuc_pos)
 *
 *    This routine is called by both hyper and UR to clash away the
 *    marked literals of the given nucleus, and append kept resolvents
 *    to Sos.
 *
 *    c_start:    Start of the clash_structure list.  There is one node
 *                for each literal that is to be clashed away.
 *    nuc_subst:  Substitution for the nucleus.
 *    nuc_lits:   Non-clashed literals of the nucleus.
 *    nuc:        The nucleus.
 *    giv_subst:  If the given clause is a satellite, then this is its
 *                substitution; else NULL.
 *    giv_lits:   If the given clause is a satellite, then these are its
 *                non-clashed literals; else NULL.
 *    giv_sat:    If the given clause is a satellite, then this is it;
 *                else NULL.
 *    sat_proc:   procedure to identify (other) satellites:  `pos_clause'
 *                for hyper, `unit_clause' for UR.
 *    inf_clock:  Clock (HYPER_TIME or UR_TIME) to be turned off during
 *                call to `pre_process'.
 *    nuc_pos:    If not 0, giv cl is sat, and nuc_pos gives position
 *                of "missing" clash node.  To construct history.
 *    ur_box:     if not 0, UR is rule, and this is the index of the boxed literal.
 *
 *************/

static void clash(struct clash_nd *c_start,
		  struct context *nuc_subst,
		  struct literal *nuc_lits,
		  struct clause *nuc,
		  struct context *giv_subst,
		  struct literal *giv_lits,
		  struct clause *giv_sat,
		  int (*sat_proc)(struct clause *c),
		  int inf_clock,
		  int nuc_pos,
		  int ur_box)
{
  struct clash_nd *c;
  struct clash_nd *c_end = NULL;
  int found, backup, fpa_depth, sign;
  struct term *f_atom, *nuc_atom_instance;
  struct trail *tr;
  struct clause *res;
  char *s;

  fpa_depth = Parms[FPA_LITERALS].val;
  c = NULL;
  backup = 0;

  while (1) {  /* return from within loop */
    if (backup == 0) {
      if (c_start == NULL || (c != NULL && c->next == NULL)) {
	/* clash is complete */
	res = build_hyper(c_start, nuc_subst, nuc_lits, nuc,
			  giv_subst, giv_lits, giv_sat, nuc_pos,
			  (inf_clock == HYPER_TIME ||
			   inf_clock == NEG_HYPER_TIME) &&
			  (Flags[BUILD_PROOF_OBJECT_1].val ||
			   Flags[BUILD_PROOF_OBJECT_2].val));
	if (inf_clock == HYPER_TIME) {
	  Stats[HYPER_RES_GEN]++;
	  res->parents->i = HYPER_RES_RULE;
	}
	else if (inf_clock == NEG_HYPER_TIME) {
	  Stats[NEG_HYPER_RES_GEN]++;
	  res->parents->i = NEG_HYPER_RES_RULE;
	}
	else {
	  Stats[UR_RES_GEN]++;
	  res->parents->i = UR_RES_RULE;
	  if (Flags[BUILD_PROOF_OBJECT_1].val ||
	      Flags[BUILD_PROOF_OBJECT_2].val) {
	    /* Insert position of NONclashed nuc literal into history. */
	    struct ilist *ip1, *ip2;
	    ip1 = get_ilist();
	    ip2 = get_ilist();
	    ip1->next = ip2;
	    ip2->next = res->parents->next->next;
	    res->parents->next->next = ip1;
	    ip1->i = LIST_RULE-1;
	    ip2->i = ur_box;
	  }
	}
	Stats[CL_GENERATED]++;
	if (heat_is_on()) {
	  struct clause *giv;
	  giv = (giv_sat ? giv_sat : nuc);
	  res->heat_level = giv->heat_level + 1;
	}
	CLOCK_STOP(inf_clock);
	pre_process(res, 0, Sos);
	CLOCK_START(inf_clock);
	backup = 1;
	c_end = c;
	c = NULL;
      }
      else {
	if (c == NULL)   /* just starting */
	  c = c_start;
	else
	  c = c->next;
		
	nuc_atom_instance = apply(c->nuc_atom, nuc_subst);
		
	if (c->evaluable) {
	  /* evaluate, but don't take any action yet */
	  nuc_atom_instance = convenient_demod(nuc_atom_instance);
	  s = sn_to_str(nuc_atom_instance->sym_num);
	  sign = c->nuc_atom->occ.lit->sign;
	  if (sign)
	    c->evaluation = str_ident(s, "$F");
	  else
	    c->evaluation = str_ident(s,"$T");
	  c->already_evaluated = 0;
	}
	else {  /* not evaluable */
	  c->u_tree = build_tree(nuc_atom_instance, UNIFY,
				 fpa_depth, c->db);
	}
		
	zap_term(nuc_atom_instance);
      }
    }
    else {  /* backup */
      if (c_start == NULL ||
	  (c != NULL && c->prev == NULL))   /* done with this nucleus */
	return;
      else {
	if (c == NULL)
	  c = c_end;
	else
	  c = c->prev;
	if (!c->evaluable)
	  clear_subst_1(c->tr);
	backup = 0;
      }
    }

    if (backup == 0) {
      found = 0;
      if (c->evaluable) {
	if (c->already_evaluated || !c->evaluation)
	  backup = 1;
	else
	  /* Set flag and proceed. */
	  c->already_evaluated = 1;
      }
      else {
	f_atom = next_term(c->u_tree, 0);
	tr = NULL;
	while (f_atom && !found) {
	  /* Sorry this test is so complicated. */
	  if (
	      /* Basic satellite test. */
	      (*sat_proc)(f_atom->occ.lit->container) &&

	      /* order_hyper & !UR -> maximal_lit */
	      (!Flags[ORDER_HYPER].val ||
	       inf_clock == UR_TIME ||
	       maximal_lit(f_atom->occ.lit)) &&

	      unify(c->nuc_atom,nuc_subst,f_atom,c->subst,&tr)
			
	      ) {
			
	    found = 1;
	  }
	  if (!found)
	    f_atom = next_term(c->u_tree, 0);
	}
		
	if (found) {
	  c->found_atom = f_atom;
	  c->tr = tr;
	}
	else {
	  backup = 1;
	}
      }
    }
  }  /* while */
}  /* clash */

/*************
 *
 *    hyper_res(c) -- hyperresolution
 *
 *    Append kept resolvents to Sos.  Each kept
 *    clause has already passed the pre_process filter (forward
 *    subsumption, etc.), been integrated, and inserted into
 *    appropriate indexes.
 *
 *************/

void hyper_res(struct clause *giv_cl)
{
  struct literal *lit, *nuc_lits, *giv_lits, *l1, *l2, *l3;
  struct context *nuc_subst, *giv_subst;
  struct clash_nd *clash_list, *c1, *c2;
  struct clause *nuc;
  int m, i, nuc_pos;
  struct term *f_atom;
  struct fpa_tree *ut;
  struct trail *tr;
  
  CLOCK_START(HYPER_TIME);
  if (num_literals(giv_cl) == 0) {
    CLOCK_STOP(HYPER_TIME);
    return;
  }
  else if (!pos_clause(giv_cl)) {  /* given clause is nucleus */
    clash_list = NULL;
    nuc_lits = NULL;
    nuc_subst = get_context();
    nuc_subst->multiplier = 0;
    m = 1;
    lit = giv_cl->first_lit;
    l2 = NULL; c2 = NULL;  /* to quiet lint */
    while (lit != NULL) {
      /* positive literal || answer literal */
      if (lit->sign || lit->atom->varnum == ANSWER) {
	l1 = get_literal();
	if (nuc_lits == NULL)
	  nuc_lits = l1;
	else
	  l2->next_lit = l1;
	l2 = l1;
	l1->sign = lit->sign;
	l1->atom = lit->atom;
      }
      else {            /* put negative literal into clash structure */
	c1 = get_clash_nd();
	if (clash_list == NULL)
	  clash_list = c1;
	else {
	  c2->next = c1;
	  c1->prev = c2;
	}
	c2 = c1;
	c2->db = Fpa_clash_pos_lits;
	c2->subst = get_context();
	c2->subst->multiplier = m++;
	c2->nuc_atom = lit->atom;
	c2->evaluable = (lit->atom->varnum == EVALUABLE);
      }
      lit = lit->next_lit;
    }
    clash(clash_list, nuc_subst, nuc_lits, giv_cl,
	  (struct context *) NULL, (struct literal *) NULL,
	  (struct clause *) NULL,
	  pos_clause, HYPER_TIME, 0, 0);
    c1 = clash_list;
    while (c1 != NULL) {
      free_context(c1->subst);
      c2 = c1;
      c1 = c1->next;
      free_clash_nd(c2);
    }
    l1 = nuc_lits;
    while (l1 != NULL) {
      l2 = l1;
      l1 = l1->next_lit;
      free_literal(l2);
    }
    free_context(nuc_subst);
    CLOCK_STOP(HYPER_TIME);
    return;
  }
  else {  /* given clause is satellite (positive) */
    giv_subst = get_context();  /* substitution for given satellite */
    giv_subst->multiplier = 0;
    nuc_subst = get_context();  /* substitution for nucleus */
    nuc_subst->multiplier = 1;
    l3 = giv_cl->first_lit;
    l2 = NULL;  c2 = NULL;
    while (l3 != NULL) {  /* for each literal in given satellite */
      if (!Flags[ORDER_HYPER].val || maximal_lit(l3)) {
	/* collect non-clashed lits (including answers) of given sat*/
	giv_lits = NULL;
	lit = giv_cl->first_lit;
	while (lit != NULL) {
	  if (lit != l3) {
	    l1 = get_literal();
	    if (giv_lits == NULL)
	      giv_lits = l1;
	    else
	      l2->next_lit = l1;
	    l2 = l1;
	    l1->sign = lit->sign;
	    l1->atom = lit->atom;
	  }
	  lit = lit->next_lit;
	}
	ut = build_tree(l3->atom, UNIFY,
			Parms[FPA_LITERALS].val, Fpa_clash_neg_lits);
	f_atom = next_term(ut, 0);
	while (f_atom != NULL) {  /* for each potential nucleus */
	  tr = NULL;
	  nuc = f_atom->occ.lit->container;
	  if (!pos_clause(nuc) &&
	      unify(l3->atom, giv_subst, f_atom, nuc_subst, &tr)) {
	    /* we have a nucleus */

	    /* there are three kinds of literal in the nucleus:    */
	    /*    1. the clashed literal -> do nothing             */
	    /*    2. positive or answer literals -> collect them   */
	    /*    3. negative literals -> put into clash structure */

	    nuc_lits = NULL;
	    clash_list = NULL;
	    m = 2;  /* multipliers for found sats start with 2 */
	    lit = nuc->first_lit;
	    i = 1;  /* find index of clausable lit that sat clahes with. */
	    nuc_pos = 0;
	    while (lit != NULL) {
	      if (lit->atom == f_atom)  /* save position */
		nuc_pos = i;
	      /* positive || answer */
	      else if (lit->sign || lit->atom->varnum == ANSWER) {
		l1 = get_literal();
		if (nuc_lits == NULL)
		  nuc_lits = l1;
		else
		  l2->next_lit = l1;
		l2 = l1;
		l1->sign = lit->sign;
		l1->atom = lit->atom;
	      }
	      else {  /* put literal into clash structure */
		i++;
		c1 = get_clash_nd();
		if (clash_list == NULL)
		  clash_list = c1;
		else {
		  c2->next = c1;
		  c1->prev = c2;
		}
		c2 = c1;
		c2->db = Fpa_clash_pos_lits;
		c2->subst = get_context();
		c2->subst->multiplier = m++;
		c2->nuc_atom = lit->atom;
		c2->evaluable = (lit->atom->varnum == EVALUABLE);
	      }
	      lit = lit->next_lit;
	    }

	    clash(clash_list, nuc_subst, nuc_lits, nuc,
		  giv_subst, giv_lits, giv_cl,
		  pos_clause, HYPER_TIME, nuc_pos, 0);

	    /* now deallocate the clash structure and literal nodes */
	    c1 = clash_list;
	    while (c1 != NULL) {
	      free_context(c1->subst);
	      c2 = c1;
	      c1 = c1->next;
	      free_clash_nd(c2);
	    }
	    l1 = nuc_lits;
	    while (l1 != NULL) {
	      l2 = l1;
	      l1 = l1->next_lit;
	      free_literal(l2);
	    }
	    clear_subst_1(tr);
	  }
	  f_atom = next_term(ut, 0);
	}
	l1 = giv_lits;
	while (l1 != NULL) {
	  l2 = l1;
	  l1 = l1->next_lit;
	  free_literal(l2);
	}
      }
      l3 = l3->next_lit;
    }    
    free_context(giv_subst);
    free_context(nuc_subst);
    CLOCK_STOP(HYPER_TIME);
    return;
  }
}  /* hyper_res */

/*************
 *
 *    neg_hyper_res(c) -- negative hyperresolution
 *
 *    Append kept resolvents to Sos.  Each kept
 *    clause has already passed the pre_process filter (forward
 *    subsumption, etc.), been integrated, and inserted into
 *    appropriate indexes.
 *
 *************/

void neg_hyper_res(struct clause *giv_cl)
{
  struct literal *lit, *nuc_lits, *giv_lits, *l1, *l2, *l3;
  struct context *nuc_subst, *giv_subst;
  struct clash_nd *clash_list, *c1, *c2;
  struct clause *nuc;
  int m, i, nuc_pos;
  struct term *f_atom;
  struct fpa_tree *ut;
  struct trail *tr;

  /* This code should have been combined with the pos hyper code. */

  CLOCK_START(NEG_HYPER_TIME);
  if (num_literals(giv_cl) == 0) {
    CLOCK_STOP(NEG_HYPER_TIME);
    return;
  }
  else if (!neg_clause(giv_cl)) {  /* given clause is nucleus */
    clash_list = NULL;
    nuc_lits = NULL;
    nuc_subst = get_context();
    nuc_subst->multiplier = 0;
    m = 1;
    lit = giv_cl->first_lit;
    l2 = NULL; c2 = NULL;  /* to quiet lint */
    while (lit != NULL) {
      /* negative literal || answer literal */
      if (!lit->sign || lit->atom->varnum == ANSWER) {
	l1 = get_literal();
	if (nuc_lits == NULL)
	  nuc_lits = l1;
	else
	  l2->next_lit = l1;
	l2 = l1;
	l1->sign = lit->sign;
	l1->atom = lit->atom;
      }
      else {            /* put positive literal into clash structure */
	c1 = get_clash_nd();
	if (clash_list == NULL)
	  clash_list = c1;
	else {
	  c2->next = c1;
	  c1->prev = c2;
	}
	c2 = c1;
	c2->db = Fpa_clash_neg_lits;
	c2->subst = get_context();
	c2->subst->multiplier = m++;
	c2->nuc_atom = lit->atom;
	c2->evaluable = (lit->atom->varnum == EVALUABLE);
      }
      lit = lit->next_lit;
    }
    clash(clash_list, nuc_subst, nuc_lits, giv_cl,
	  (struct context *) NULL, (struct literal *) NULL,
	  (struct clause *) NULL,
	  neg_clause, NEG_HYPER_TIME, 0, 0);
    c1 = clash_list;
    while (c1 != NULL) {
      free_context(c1->subst);
      c2 = c1;
      c1 = c1->next;
      free_clash_nd(c2);
    }
    l1 = nuc_lits;
    while (l1 != NULL) {
      l2 = l1;
      l1 = l1->next_lit;
      free_literal(l2);
    }
    free_context(nuc_subst);
    CLOCK_STOP(NEG_HYPER_TIME);
    return;
  }
  else {  /* given clause is satellite (negative) */
    giv_subst = get_context();  /* substitution for given satellite */
    giv_subst->multiplier = 0;
    nuc_subst = get_context();  /* substitution for nucleus */
    nuc_subst->multiplier = 1;
    l3 = giv_cl->first_lit;
    l2 = NULL; c2 = NULL;
    while (l3 != NULL) {  /* for each literal in given satellite */
      if (!Flags[ORDER_HYPER].val || maximal_lit(l3)) {
	/* collect non-clashed lits (including answers) of given sat*/
	giv_lits = NULL;
	lit = giv_cl->first_lit;
	while (lit != NULL) {
	  if (lit != l3) {
	    l1 = get_literal();
	    if (giv_lits == NULL)
	      giv_lits = l1;
	    else
	      l2->next_lit = l1;
	    l2 = l1;
	    l1->sign = lit->sign;
	    l1->atom = lit->atom;
	  }
	  lit = lit->next_lit;
	}
	ut = build_tree(l3->atom, UNIFY,
			Parms[FPA_LITERALS].val, Fpa_clash_pos_lits);
	f_atom = next_term(ut, 0);
	while (f_atom != NULL) {  /* for each potential nucleus */
	  tr = NULL;
	  nuc = f_atom->occ.lit->container;
	  if (!neg_clause(nuc) &&
	      unify(l3->atom, giv_subst, f_atom, nuc_subst, &tr)) {
	    /* we have a nucleus */

	    /* there are three kinds of literal in the nucleus:    */
	    /*    1. the clashed literal -> do nothing             */
	    /*    2. negative or answer literals -> collect them   */
	    /*    3. positive literals -> put into clash structure */

	    nuc_lits = NULL;
	    clash_list = NULL;
	    m = 2;  /* multipliers for found sats start with 2 */
	    lit = nuc->first_lit;
	    i = 1;  /* find index of clahsable lit that sat clashes with. */
	    nuc_pos = 0;
	    while (lit != NULL) {
	      if (lit->atom == f_atom)  /* save position */
		nuc_pos = i;
	      /* negative || answer */
	      else if (!lit->sign || lit->atom->varnum == ANSWER) {
		l1 = get_literal();
		if (nuc_lits == NULL)
		  nuc_lits = l1;
		else
		  l2->next_lit = l1;
		l2 = l1;
		l1->sign = lit->sign;
		l1->atom = lit->atom;
	      }
	      else {  /* put literal into clash structure */
		i++;
		c1 = get_clash_nd();
		if (clash_list == NULL)
		  clash_list = c1;
		else {
		  c2->next = c1;
		  c1->prev = c2;
		}
		c2 = c1;
		c2->db = Fpa_clash_neg_lits;
		c2->subst = get_context();
		c2->subst->multiplier = m++;
		c2->nuc_atom = lit->atom;
		c2->evaluable = (lit->atom->varnum == EVALUABLE);
	      }
	      lit = lit->next_lit;
	    }

	    clash(clash_list, nuc_subst, nuc_lits, nuc,
		  giv_subst, giv_lits, giv_cl,
		  neg_clause, NEG_HYPER_TIME, nuc_pos, 0);

	    /* now deallocate the clash structure and literal nodes */
	    c1 = clash_list;
	    while (c1 != NULL) {
	      free_context(c1->subst);
	      c2 = c1;
	      c1 = c1->next;
	      free_clash_nd(c2);
	    }
	    l1 = nuc_lits;
	    while (l1 != NULL) {
	      l2 = l1;
	      l1 = l1->next_lit;
	      free_literal(l2);
	    }
	    clear_subst_1(tr);
	  }
	  f_atom = next_term(ut, 0);
	}
	l1 = giv_lits;
	while (l1 != NULL) {
	  l2 = l1;
	  l1 = l1->next_lit;
	  free_literal(l2);
	}
      }
      l3 = l3->next_lit;
    }
    free_context(giv_subst);
    free_context(nuc_subst);
    CLOCK_STOP(NEG_HYPER_TIME);
    return;
  }
}  /* neg_hyper_res */

/*************
 *
 *    ur_res(c) -- unit resulting (UR) resolution
 *
 *    Append kept resolvents to Sos.  Each kept
 *    clause has already passed the pre_process filter (forward
 *    subsumption, etc.), been integrated, and inserted into
 *    appropriate indexes.
 *
 *************/

void ur_res(struct clause *giv_cl)
{
  struct literal *lit, *nuc_lits, *giv_lits;
  struct literal *l1, *l2, *l3, *box, *f_lit;
  struct context *nuc_subst, *giv_subst;
  struct clash_nd *clash_list, *c1, *c2;
  struct clause *nuc;
  int m, i, nlits, j, nuc_pos;
  struct term *f_atom;
  struct fpa_tree *ut;
  struct trail *tr;

  CLOCK_START(UR_TIME);
  nlits = num_literals(giv_cl);
  if (nlits == 0) {
    CLOCK_STOP(UR_TIME);
    return;
  }
  if (nlits > 1) {  /* given clause is nucleus (non-unit) */
    clash_list = NULL;
    nuc_subst = get_context();
    nuc_subst->multiplier = 0;
    m = 1;
    nuc_lits = get_literal();  /* for boxed literal */
    l2 = nuc_lits;
    lit = giv_cl->first_lit;
    while (lit != NULL) {
      if (lit->atom->varnum == ANSWER) {  /* if answer literal */
	l1 = get_literal();
	l2->next_lit = l1;
	l2 = l1;
	l1->sign = lit->sign;
	l1->atom = lit->atom;
      }
      lit = lit->next_lit;
    }
    c2 = NULL;  /* to quiet lint */
    for (i = 1; i < nlits; i++) {  /* set up nlits-1 empty clash nodes */
      c1 = get_clash_nd();
      if (clash_list == NULL)
	clash_list = c1;
      else {
	c2->next = c1;
	c1->prev = c2;
      }
      c2 = c1;
      c2->subst = get_context();
      c2->subst->multiplier = m++;
    }
    box = giv_cl->first_lit;
    if (Flags[UR_LAST].val) {
      /* boxed literal must be the last literal */
      while (box->next_lit)
	box = box->next_lit;
    }
    i = 1;
    while (box != NULL) {
      if (box->atom->varnum != ANSWER) {  /* if not answer literal */
	c1 = clash_list;
	nuc_lits->sign = box->sign;
	nuc_lits->atom = box->atom;
	lit = giv_cl->first_lit;
	while (lit != NULL) {
	  /* if not boxed or answer literal */
	  if (lit != box && lit->atom->varnum != ANSWER) {
	    c1->nuc_atom = lit->atom;
	    c1->db = (lit->sign ? Fpa_clash_neg_lits :
		      Fpa_clash_pos_lits);
	    c1 = c1->next;
	  }
	  lit = lit->next_lit;
	}
	if (c1 != NULL) {
	  abend("ur_res: too many clash nodes (nuc).");
	}
	clash(clash_list, nuc_subst, nuc_lits, giv_cl,
	      (struct context *) NULL, (struct literal *) NULL,
	      (struct clause *) NULL,
	      unit_clause, UR_TIME, 0, i);
      }
      box = box->next_lit;
      i++;
    }
    c1 = clash_list;
    while (c1 != NULL) {
      free_context(c1->subst);
      c2 = c1;
      c1 = c1->next;
      free_clash_nd(c2);
    }
    l1 = nuc_lits;
    while (l1 != NULL) {
      l2 = l1;
      l1 = l1->next_lit;
      free_literal(l2);
    }
    free_context(nuc_subst);
    CLOCK_STOP(UR_TIME);
    return;
  }
  else {  /* given clause is satellite (unit) */
    giv_subst = get_context();  /* substitution for given satellite */
    giv_subst->multiplier = 0;
    nuc_subst = get_context();  /* substitution for nucleus */
    nuc_subst->multiplier = 1;
    /* collect any answer literals from given satellite */
    /* and get clashable literal (l3) */
    giv_lits = NULL;
    lit = giv_cl->first_lit;
    l2 = NULL; l3 = NULL; c2 = NULL; 
    while (lit != NULL) {
      if (lit->atom->varnum != ANSWER)  /* if not answer lit */
	l3 = lit;  /* the only non-answer literal */
      else {
	l1 = get_literal();
	if (giv_lits == NULL)
	  giv_lits = l1;
	else
	  l2->next_lit = l1;
	l2 = l1;
	l1->sign = lit->sign;
	l1->atom = lit->atom;
      }
      lit = lit->next_lit;
    }

    ut = build_tree(l3->atom, UNIFY, Parms[FPA_LITERALS].val,
		    l3->sign ? Fpa_clash_neg_lits : Fpa_clash_pos_lits);
    f_atom = next_term(ut, 0);
    while (f_atom != NULL) {  /* for each potential nucleus */
      tr = NULL;
      f_lit = f_atom->occ.lit;
      nuc = f_lit->container;
      nlits = num_literals(nuc);
      if (nlits > 1 &&
	  (!Flags[UR_LAST].val || f_lit->next_lit) &&
	  unify(l3->atom, giv_subst, f_atom, nuc_subst, &tr)) {
	/* we have a nucleus */
	m = 2;
	nuc_lits = get_literal();  /* for boxed literal */
	/* now append any answer literals to nuc_lits */
	l2 = nuc_lits;
	lit = nuc->first_lit;
	while (lit != NULL) {
	  if (lit->atom->varnum == ANSWER) {  /* if answer literal */
	    l1 = get_literal();
	    l2->next_lit = l1;
	    l2 = l1;
	    l1->sign = lit->sign;
	    l1->atom = lit->atom;
	  }
	  lit = lit->next_lit;
	}
	/* build clash structure for this nucleus */
	clash_list = NULL;
	for (i = 2; i < nlits; i++) {  /* nlits-2 empty clash nodes */
	  c1 = get_clash_nd();
	  if (clash_list == NULL)
	    clash_list = c1;
	  else {
	    c2->next = c1;
	    c1->prev = c2;
	  }
	  c2 = c1;
	  c2->subst = get_context();
	  c2->subst->multiplier = m++;
	}
	box = nuc->first_lit;
	if (Flags[UR_LAST].val) {
	  /* boxed literal must be the last literal */
	  while (box->next_lit)
	    box = box->next_lit;
	}
	i = 1;
	while (box != NULL) {
	  /* if not clashed or answer literal */
	  if (box != f_lit && box->atom->varnum != ANSWER) {
	    c1 = clash_list;
	    nuc_lits->sign = box->sign;
	    nuc_lits->atom = box->atom;
	    lit = nuc->first_lit;
	    j = 1;
	    nuc_pos = 0;
	    while (lit != NULL) {
	      /* if not boxed or clashed or answer literal */
	      if (lit != box && lit != f_lit &&
		  lit->atom->varnum != ANSWER) {
		c1->nuc_atom = lit->atom;
		c1->db = (lit->sign ? Fpa_clash_neg_lits :
			  Fpa_clash_pos_lits);
		c1 = c1->next;
		j++;
	      }
	      if (lit == f_lit)
		nuc_pos = j;  /* For ordered history option */
	      lit = lit->next_lit;
	    }
	    if ( c1 != NULL)  {
	      abend("ur_res: too many clash nodes (sat).");
	    }
	    clash(clash_list, nuc_subst, nuc_lits, nuc,
		  giv_subst, giv_lits, giv_cl,
		  unit_clause, UR_TIME, nuc_pos, i);
	  }
	  box = box->next_lit;
	  i++;
	}
	c1 = clash_list;
	while (c1 != NULL) {
	  free_context(c1->subst);
	  c2 = c1;
	  c1 = c1->next;
	  free_clash_nd(c2);
	}
	l1 = nuc_lits;
	while (l1 != NULL) {
	  l2 = l1;
	  l1 = l1->next_lit;
	  free_literal(l2);
	}

	clear_subst_1(tr);
      }
      f_atom = next_term(ut, 0);
    }

    /* free answer literals from given satellite */
    l1 = giv_lits;
    while (l1 != NULL) {
      l2 = l1;
      l1 = l1->next_lit;
      free_literal(l2);
    }

    free_context(giv_subst);
    free_context(nuc_subst);
    CLOCK_STOP(UR_TIME);
    return;
  }
}  /* ur_res */

/*************
 *
 *    int one_unary_answer(c)
 *
 *************/

int one_unary_answer(struct clause *c)
{
  struct literal *l;

  for (l = c->first_lit; l != NULL && l->atom->varnum != ANSWER; l = l->next_lit);  /* empty body */
  if (l == NULL)
    return(0);
  else if (sn_to_arity(l->atom->sym_num) != 1)
    return(0);
  else {
    for (l = l->next_lit; l != NULL && l->atom->varnum != ANSWER; l = l->next_lit);  /* empty body */
    return(l == NULL);
  }
}  /* one_unary_answer */

/*************
 *
 *    struct term *build_term(sn, arg1, arg2, arg3)
 *
 *************/

struct term *build_term(int sn,
			struct term *arg1,
			struct term *arg2,
			struct term *arg3)
{
  int arity;
  struct rel *r1, *r2, *r3;
  struct term *t;

  arity = sn_to_arity(sn);
  if (arity != 3) {
    abend("build_term, bad arity.");
  }
  t = get_term();
  t->sym_num = sn;
  t->type = COMPLEX;
  r1 = get_rel();
  r2 = get_rel();
  r3 = get_rel();
  t->farg = r1;
  r1->narg = r2;
  r2->narg = r3;
  r1->argval = arg1;
  r2->argval = arg2;
  r3->argval = arg3;
  return(t);
}  /* build_term */


/*************
 *
 *    void combine_answers(res, a1, s1, a2, s2)
 *
 *************/

void combine_answers(struct clause *res,
		     struct term *a1,
		     struct context *s1,
		     struct term *a2,
		     struct context *s2)
{
  struct clause *par1, *par2;
  int condition_par1;
  struct term *condition, *then_part, *else_part;
  struct literal *lit1, *lit2, *prev_lit;

  par1 = a1->occ.lit->container;
  par2 = a2->occ.lit->container;

  if (one_unary_answer(par1) && one_unary_answer(par2)) {

    condition_par1 = a2->occ.lit->sign;

    if (condition_par1)
      condition = apply(a1, s1);
    else
      condition = apply(a2, s2);

    for (lit1 = res->first_lit, prev_lit = NULL;
	 lit1->atom->varnum != ANSWER;
	 prev_lit = lit1, lit1 = lit1->next_lit);
    /* empty body */
    for (lit2 = lit1->next_lit; lit2->atom->varnum != ANSWER; lit2 = lit2->next_lit);
    /* empty body */

    if (condition_par1) {
      then_part = lit1->atom->farg->argval;
      else_part = lit2->atom->farg->argval;
    }
    else {
      then_part = lit2->atom->farg->argval;
      else_part = lit1->atom->farg->argval;
    }

    if (prev_lit == NULL)
      res->first_lit = lit1->next_lit;
    else
      prev_lit->next_lit = lit1->next_lit;

    free_rel(lit1->atom->farg);
    free_term(lit1->atom);
    free_literal(lit1);

    lit2->atom->farg->argval = build_term(str_to_sn("if",3),condition,then_part,else_part);
  }

}  /* combine_answers */

/*************
 *
 *    struct clause *build_bin_res(a1, s1, a2, s2)
 *
 *    Build a binary resolvent.  a1 and a2 are the clashed literals,
 *    and s1 and s2 are the respective unifying substitutions.
 *
 *************/

struct clause *build_bin_res(struct term *a1,
			     struct context *s1,
			     struct term *a2,
			     struct context *s2)
{
  struct clause *res;
  struct literal *lit, *new, *prev;
  struct ilist *ip0, *ip1, *ip2;

  res = get_clause();
  prev = NULL;
  lit = a1->occ.lit->container->first_lit;
  while (lit != NULL) {
    if (lit->atom != a1) {
      new = get_literal();
      new->container = res;
      if (prev == NULL)
	res->first_lit = new;
      else
	prev->next_lit = new;
      prev = new;
      new->sign = lit->sign;
      new->atom = apply(lit->atom, s1);
      new->atom->occ.lit = new;
      new->atom->varnum = lit->atom->varnum;  /* copy type of atom */
    }
    lit = lit->next_lit;
  }

  lit = a2->occ.lit->container->first_lit;
  while (lit != NULL) {
    if (lit->atom != a2) {
      new = get_literal();
      new->container = res;
      if (res->first_lit == NULL)
	res->first_lit = new;
      else
	prev->next_lit = new;
      prev = new;
      new->sign = lit->sign;
      new->atom = apply(lit->atom, s2);
      new->atom->occ.lit = new;
      new->atom->varnum = lit->atom->varnum;  /* copy type of atom */
    }
    lit = lit->next_lit;
  }

  ip0 = get_ilist();
  ip1 = get_ilist();
  ip2 = get_ilist();
  ip0->i = BINARY_RES_RULE;
  ip1->i = a1->occ.lit->container->id;
  ip2->i = a2->occ.lit->container->id;
  ip0->next = ip1;
  ip1->next = ip2;
  res->parents = ip0;

  if (Flags[DETAILED_HISTORY].val) {
    ip0 = get_ilist();
    ip1 = get_ilist();
    ip0->next = ip1;
    ip1->next = res->parents->next->next->next;
    res->parents->next->next->next = ip0;
    ip0->i = LIST_RULE - 1;
    ip1->i = literal_number(a2->occ.lit);

    ip0 = get_ilist();
    ip1 = get_ilist();
    ip0->next = ip1;
    ip1->next = res->parents->next->next;
    res->parents->next->next = ip0;
    ip0->i = LIST_RULE - 1;
    ip1->i = literal_number(a1->occ.lit);
	
  }

  if (Flags[PROG_SYNTHESIS].val)
    combine_answers(res, a1, s1, a2, s2);
  return(res);
}  /* build_bin_res */

/*************
 *
 *    struct clause *apply_clause(c, s)
 *
 *************/

struct clause *apply_clause(struct clause *c,
			    struct context *s)
{
  struct clause *d;
  struct literal *lit, *new, *prev;

  d = get_clause();
  prev = NULL;
  for (lit = c->first_lit; lit; lit = lit->next_lit) {
    new = get_literal();
    new->container = d;
    if (!prev)
      d->first_lit = new;
    else
      prev->next_lit = new;
    prev = new;
    new->sign = lit->sign;
    new->atom = apply(lit->atom, s);
    new->atom->occ.lit = new;
    new->atom->varnum = lit->atom->varnum;  /* copy type of atom */
  }
  return(d);
}  /* apply_clause */

/*************
 *
 *    bin_res(giv_cl) -- binary resolution
 *
 *************/

void bin_res(struct clause *giv_cl)
{
  struct literal *g_lit;
  struct term *g_atom, *f_atom;
  struct context *gs, *fs;
  struct trail *tr;
  struct fpa_tree *ut;
  struct fpa_index *db;
  struct clause *resolvent;
  int given_unit = unit_clause(giv_cl);

  CLOCK_START(BINARY_TIME);
  gs = get_context();
  gs->multiplier = 0;
  fs = get_context();
  fs->multiplier = 1;
  g_lit = giv_cl->first_lit;
  while (g_lit != NULL) {
    g_atom = g_lit->atom;
    if (g_atom->varnum != ANSWER)  {  /* if not answer literal */
      if (g_lit->sign)
	db = Fpa_clash_neg_lits;
      else
	db = Fpa_clash_pos_lits;
      ut = build_tree(g_lit->atom, UNIFY,
		      Parms[FPA_LITERALS].val, db);
      f_atom = next_term(ut, 0);
      while (f_atom != NULL) {
	tr = NULL;
	if (!Flags[UNIT_RES].val ||
	    given_unit ||
	    unit_clause(f_atom->occ.lit->container)) {
	  if (unify(g_atom, gs, f_atom, fs, &tr)) {
	    resolvent = build_bin_res(g_atom, gs, f_atom, fs);
	    clear_subst_1(tr);
	    Stats[CL_GENERATED]++;
	    Stats[BINARY_RES_GEN]++;
	    if (heat_is_on())
	      resolvent->heat_level = giv_cl->heat_level + 1;
	    CLOCK_STOP(BINARY_TIME);
	    pre_process(resolvent, 0, Sos);
	    CLOCK_START(BINARY_TIME);
	  }  
	}
	f_atom = next_term(ut, 0);
      }
    }
    g_lit = g_lit->next_lit;
  }
  free_context(gs);
  free_context(fs);
  CLOCK_STOP(BINARY_TIME);
}  /* bin_res */

/*************
 *
 *   first_or_next_factor(c, l1p, l2p)
 *
 *   Generate the first (*l1p == NULL) or next (*l1p and *l2p are the
 *   previously factored literals) factor from c.
 *
 *************/

struct clause *first_or_next_factor(struct clause *c,
				    struct literal **l1p,
				    struct literal **l2p)
{
  int factored = 0;
  struct literal *l1 = *l1p;
  struct literal *l2 = *l2p;
  struct context *subst = get_context();
  struct trail *tr;
  struct clause *factor;
  struct literal *l3, *l4, *l5;


  if (!l1)
    l1 = l2 = c->first_lit;

  while (l1 && !factored) {
    l2 = l2->next_lit;
    while (l2 && !factored) {
      tr = NULL;
      if (l1->sign == l2->sign &&
	  unify(l1->atom, subst, l2->atom, subst, &tr)) {
	factored = 1;
      }
      else
	l2 = l2->next_lit;
    }
    if (!factored)
      l1 = l2 = l1->next_lit;
  }

  if (factored) {
    subst->multiplier = 0;
    factor = get_clause();
    /* do not fill in parents */
    l3 = NULL;
    l5 = c->first_lit;
    while (l5 != NULL) {  /* l2 is the literal to exclude */
      if (l5 != l2) {
	l4 = get_literal();
	l4->sign = l5->sign;
	l4->container = factor;
	if (l3 == NULL)
	  factor->first_lit = l4;
	else
	  l3->next_lit = l4;
	l4->atom = apply(l5->atom, subst);
	/* Following is for factor_simp; shouldn't hurt otherwise. */
	if (TP_BIT(l5->atom->bits, ORIENTED_EQ_BIT))
	  SET_BIT(l4->atom->bits, ORIENTED_EQ_BIT);
	l4->atom->occ.lit = l4;
	l4->atom->varnum = l5->atom->varnum;  /* copy type */
	l3 = l4;
      }
      l5 = l5->next_lit;
    }
    clear_subst_1(tr);
    *l2p = l2; *l1p = l1;
  }
  else
    factor = NULL;

  free_context(subst);
  return(factor);
}  /* first_or_next_factor */

/*************
 *
 *    all_factors(c, lst) -- generate and pre_process all binary factors c.
 *
 *    Indirect recursive calls will get factors of factors, etc.
 *
 *************/

void all_factors(struct clause *c,
		 struct list *lst)
{
  struct literal *l1, *l2;
  struct clause *factor;
  struct ilist *ip0, *ip1;

  l1 = NULL;
  factor = first_or_next_factor(c, &l1, &l2);
  while (factor) {
    ip0 = get_ilist(); ip0->i = FACTOR_RULE;
    ip1 = get_ilist(); ip1->i = c->id;
    factor->parents = ip0; ip0->next = ip1;
    if (Flags[DETAILED_HISTORY].val) {
      /* append list of indexes of factored literals */
      struct ilist *ip3, *ip4, *ip5;
      ip3 = get_ilist(); ip3->i = LIST_RULE-2;
      ip4 = get_ilist(); ip4->i = literal_number(l1);
      ip5 = get_ilist(); ip5->i = literal_number(l2);
      ip1->next = ip3; ip3->next = ip4; ip4->next = ip5;
    }

    Stats[CL_GENERATED]++;
    Stats[FACTOR_GEN]++;
    CLOCK_STOP(FACTOR_TIME);
    CLOCK_STOP(POST_PROC_TIME);
    pre_process(factor, 0, lst);
    CLOCK_START(POST_PROC_TIME);
    CLOCK_START(FACTOR_TIME);
    factor = first_or_next_factor(c, &l1, &l2);
  }
}  /* all_factors */

/*************
 *
 *   factor_simpify(c)
 *
 *   Return the number of literals removed.
 *
 *************/

int factor_simplify(struct clause *c)
{
  struct literal *l1, *l2;
  struct clause *f;
  struct ilist *p0, *p1;
  int n = 0;

  l1 = NULL;
  f = first_or_next_factor(c, &l1, &l2);
  while (f) {
    if (subsume(f, c)) {
      n++;
      /* Swap list of literals. */
      l1 = c->first_lit;
      c->first_lit = f->first_lit;
      f->first_lit = l1;
      for (l1 = c->first_lit; l1; l1 = l1->next_lit)
	l1->container = c;
      for (l1 = f->first_lit; l1; l1 = l1->next_lit)
	l1->container = f;
      cl_del_non(f);

      if (Flags[DETAILED_HISTORY].val) {
	for (p1 = c->parents, p0 = NULL; p1; p1 = p1->next)
	  p0 = p1;
	p1 = get_ilist();
	p1->i = FACTOR_SIMP_RULE;
	if (p0)
	  p0->next = p1;
	else
	  c->parents = p1;
      }

      l1 = NULL;
      f = first_or_next_factor(c, &l1, &l2);
    }
    else {
      cl_del_non(f);
      f = first_or_next_factor(c, &l1, &l2);
    }
  }
  return(n);
}  /* factor_simplify */
