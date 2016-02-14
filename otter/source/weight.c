/*
 *  weight.c -- Routines to weigh clauses, literals and terms.
 *  (also some routines that handle lexical ordering (not LRPO)).
 *
 */

#include "header.h"

/*************
 *
 *    struct term_ptr *read_wt_list(fp, ep)
 *
 *    read_list then set_vars for each term.
 *
 *************/

struct term_ptr *read_wt_list(FILE *fp,
			      int *ep)
{
  struct term_ptr *p1, *p2;

  Internal_flags[REALLY_CHECK_ARITY] = 1;

  p1 = read_list(fp, ep, 0);  /* don't integrate */
  for (p2 = p1; p2; p2 = p2->next) {
    if (!set_vars(p2->term)) {
      fprintf(stdout, "ERROR, too many variables, max is %d: ", MAX_VARS);
      print_term_nl(stdout, p2->term);
      (*ep)++;
    }
  }

  Internal_flags[REALLY_CHECK_ARITY] = 0;
  return(p1);
}  /* read_wt_list */

/*************
 *
 *    static struct is_tree *weight_retrieve(t, wt_index)
 *
 *************/

static struct is_tree *weight_retrieve(struct term *t,
				       struct is_tree *wt_index)
{
  struct is_tree *is;

  if (!wt_index)
    return(NULL);
  else {
    is = wt_index->u.kids;
    while (is && ((t->type != is->type) ||
		  (t->type != VARIABLE && (t->sym_num != is->lab))))
      is = is->next;
    return(is);
  }
}  /* weight_retrieve */

/*************
 *
 *    int noncomplexifying(c) -- True iff c is a noncomplexifying substitution
 *
 *************/

int noncomplexifying(struct context *c)
{
  int i;
  for (i = 0; i < MAX_VARS; i++) {
    if (c->terms[i] && c->terms[i]->type == COMPLEX)
      return(0);
  }
  return(1);
}  /* noncomplexifying */

/*************
 *
 *    int overbeek_match(t) -- True iff t is instance of one of the overbeek_terms.
 *
 *************/

int overbeek_match(struct term *t)
{
  struct term *l, *member;
  struct context *c;
  struct trail *tr;

  c = get_context();
  /* Assume Overbeek_terms is a proper list. */
  for (l = Overbeek_terms; l->sym_num != Nil_sym_num; l = l->farg->narg->argval) {
    member = l->farg->argval;
    tr = NULL;
    if (match(member, c, t, &tr)) {
      if (noncomplexifying(c)) {
	clear_subst_1(tr);
	free_context(c);
	return(1);
      }
      else
	clear_subst_1(tr);
    }
  }
  free_context(c);
  return(0);
}  /* overbeek_match */

/*************
 *
 *    int weight(term, wt_index) -- Return the weight a term.
 *
 *************/

int weight(struct term *t,
	   struct is_tree *wt_index)
{
  struct is_tree *is;
  struct term_ptr *p;
  struct rel *r;
  int wt, w1, max;

  if (overbeek_weight(t, &wt))
    return wt;

  is = weight_retrieve(t, wt_index);
  if (is)
    p = is->u.terms;
  else
    p = NULL;

  wt = 0;
  while (p != NULL &&
	 wt_match(t, p->term->farg->argval, &wt, wt_index) == 0) {
    p = p->next;
    wt = 0;
  }

  if (p != NULL)  /* we have a match */
    return(wt + p->term->farg->narg->argval->fpa_id);
  else if (is_atom(t) && t->varnum == ANSWER)
    return(0);  /* default weight of answer atom */
  else if (t->type == VARIABLE || t->type == NAME)
    return(1);  /* default weight of symbol */
  else {   /* compute default weight of term or atom */
    /*
      if (flag is set)
      weight of t is (max or weights of args) + 1
      else
      weight of t is (sum weights of subterms) + 1
    */
    if (is_atom(t))
      max = Flags[ATOM_WT_MAX_ARGS].val;
    else
      max = Flags[TERM_WT_MAX_ARGS].val;
    wt = 0;
    r = t->farg;
    while (r != NULL) {
      if (is_atom(t) && Overbeek_terms && overbeek_match(r->argval))
	w1 = 0;
      else
	w1 = weight(r->argval, wt_index);
      if (max)
	wt = (w1 > wt ? w1 : wt);
      else
	wt += w1;
      r = r->narg;
    }
    return(wt + 1);
  }
}  /* weight */

/*************
 *
 *   wt_match_dots()
 *
 *************/

static int wt_match_dots(struct term *t,
			 struct term *template,
			 int *wtp,
			 struct is_tree *wt_index)
{
  if (wt_match(t, template, wtp, wt_index))
    return(1);
  else {
    struct rel *r;
    for (r = t->farg; r; r = r->narg) {
      if (wt_match_dots(r->argval, template, wtp, wt_index))
	return(1);
    }
    return(0);
  }
}  /* wt_match_dots */

/*************
 *
 *    int weight_match(term, template, wtp, wt_index)
 *
 *        Attempt to match a term with a weight template.  If
 *    successful, add the weight of the term to *wtp, and
 *    return(1); else return(0).
 *
 *************/

int wt_match(struct term *t,
	     struct term *template,
	     int *wtp,
	     struct is_tree *wt_index)
{
  struct rel *r1,*r2;
  int go;

  if (template->type == COMPLEX && template->sym_num == Dots_sym_num &&
      wt_match_dots(t, template->farg->argval, wtp, wt_index))
    return(1);
  else if (t->type != template->type)
    return(0);
  else if (t->type == VARIABLE)
    return(1);
  else if (t->type == NAME)
    return(t->sym_num == template->sym_num);
  else {  /* complex */
    if (t->sym_num != template->sym_num)
      return(0);
    else {
      go = 1;
      r1 = t->farg;
      r2 = template->farg;
      while (go && r1 != NULL && r2 != NULL) {
	if (TP_BIT(r2->argval->bits, SCRATCH_BIT))
	  /* term is a multiplier */
	  *wtp += r2->argval->fpa_id * weight(r1->argval,wt_index);
	else
	  go = wt_match(r1->argval, r2->argval, wtp, wt_index);
	r1 = r1->narg;
	r2 = r2->narg;
      }
      return(go && r1 == NULL && r2 == NULL);
    }
  }
}  /* wt_match */

/*************
 *
 *    static void set_wt_term(term)
 *
 *    Mark multipliers with SCRATCH_BIT,
 *    and store the multipliers in fpa_id field.
 *
 *    This week, multipliers look like this: $(100),  $(-3)
 *
 *************/

static void set_wt_term(struct term *t)
{
  struct rel *r;
  int n;

  if (t->type == COMPLEX) {
	
    if (is_symbol(t, "$", 1) && t->farg->argval->type == NAME &&
	str_int(sn_to_str(t->farg->argval->sym_num), &n)) {
	
      /* this is a trick to mark a multiplier */
      SET_BIT(t->bits, SCRATCH_BIT);
      t->fpa_id = n;
    }
    else {
      for (r = t->farg; r; r = r->narg)
	set_wt_term(r->argval);
    }
  }

}  /* set_wt_term */

/*************
 *
 *    static int set_wt_template(template)
 *
 *        Make sure that the template is OK, and mark the multipliers
 *    and the adder.  Return 1 for success and 0 for failure.
 *    Example weight templates:  weight(f($1,f($3,a)),5),
 *    weight(x,-100), (all variables have weight -100),
 *    weight(f(x,g(a,x)),30) (x matches any variable, and the
 *    two occurrences of x don't have to match the same variable.
 *
 *************/

static int set_wt_template(struct term *t)
{
  int n;

  /* first make sure that template is ok; if ok, str_int gets adder */
  if (t->type != COMPLEX || str_ident(sn_to_str(t->sym_num), "weight") == 0
      || t->farg == NULL || t->farg->narg == NULL
      || t->farg->narg->narg != NULL || t->farg->narg->argval->type != NAME
      || str_int(sn_to_str(t->farg->narg->argval->sym_num), &n) == 0) {
    return(0);
  }
  else {
    /* stash adder in fpa_id field */
    t->farg->narg->argval->fpa_id = n;
    set_wt_term(t->farg->argval);
    return(1);
  }
}  /* set_wt_template */

/*************
 *
 *    static void weight_insert(t, wt_index)
 *
 *************/

static void weight_insert(struct term *t,
			  struct is_tree *wt_index)
{
  struct is_tree *is;
  struct term_ptr *tp, *new_tp;
  struct term *t1;

  new_tp = get_term_ptr();
  new_tp->term = t;

  is = weight_retrieve(t->farg->argval, wt_index);

  if (is) {
    /* Put new template at end of list. */
    tp = is->u.terms;
    while (tp->next)
      tp = tp->next;
    tp->next = new_tp;
  }
  else {
    t1 = t->farg->argval;
    is = get_is_tree();
    is->type = t1->type;
    if (t1->type == VARIABLE)
      is->lab = t1->varnum;
    else
      is->lab = t1->sym_num;
    is->u.terms = new_tp;
    is->next = wt_index->u.kids;
    wt_index->u.kids = is;
  }

}  /* weight_insert */

/*************
 *
 *    set_wt_list(wt_list, wt_index, error_ptr) -- Set a list of weight termplates.
 *
 *************/

void set_wt_list(struct term_ptr *wt_list,
		 struct is_tree *wt_index,
		 int *ep)
{
  struct term_ptr *p;

  *ep = 0;
  p = wt_list;
  while (p != NULL) {
    if (set_wt_template(p->term) == 0) {
      fprintf(stdout, "ERROR, weight template: ");
      print_term_nl(stdout, p->term);
      (*ep)++;
    }
    else
      weight_insert(p->term, wt_index);
    p = p->next;
  }

}  /* set_wt_list */

/*************
 *
 *    void weight_index_delete(wt_index)
 *
 *************/

void weight_index_delete(struct is_tree *wt_index)
{
  struct is_tree *is1, *is2;
  struct term_ptr *tp1, *tp2;

  if (wt_index) {
    is1 = wt_index->u.kids;
    while (is1) {
      tp1 = is1->u.terms;
      while (tp1) {
	/* Do not free template; it belongs to Weight_list. */
	tp2 = tp1;
	tp1 = tp1->next;
	free_term_ptr(tp2);
      }
      is2 = is1;
      is1 = is1->next;
      free_is_tree(is2);
    }
    free_is_tree(wt_index);
  }
	
}  /* weight_index_delete */

/*************
 *
 *    lex_compare_sym_nums(n1, n2)
 *
 *    We must always have a total order on the symbols.
 *
 *************/

static int lex_compare_sym_nums(int n1,
				int n2)
{
  int v1, v2;
  struct sym_ent *p1, *p2;

  if (n1 == n2)
    return(SAME_AS);
  else {
    p1 = sn_to_node(n1);
    p2 = sn_to_node(n2);
    v1 = p1->lex_val;
    v2 = p2->lex_val;
    if (v1 < v2)
      return(LESS_THAN);
    else if (v1 > v2)
      return(GREATER_THAN);
    else
      /* This occurs if a lex command omits symbols or if
       * new symbols are introduced on the fly.
       */
      return(compare_for_auto_lex_order(p1, p2));
  }
}  /* lex_compare_sym_nums */

/*************

 *
 *    int lex_order(t1, t2)
 *
 *    Return SAME_AS, GREATER_THAN, LESS_THAN, or NOT_COMPARABLE.
 *
 *    A variable is comparable only to an identical variable
 *    (nonground terms can still be compared: if a < b, f(a,x) < f(b,y).)
 *    For pairs of nonvariables, use the lex_val field of the symbol_table node;
 *    if identical, use the sym_num's of the terms.
 *
 *************/

int lex_order(struct term *t1,
	      struct term *t2)
{
  struct rel *r1, *r2;
  int i, t1_special, t2_special;

  /* The following handles special unary functions that are to be */
  /* ignored during lex_check.  For example, when using lex-dependent */
  /* demodulation to sort AC expressions, you can make the canonical */
  /* form be a + -a + b + -b + c + -c.                              */

  if (Internal_flags[SPECIAL_UNARY_PRESENT]) {
    t1_special = (t1->type == COMPLEX && sn_to_node(t1->sym_num)->special_unary);
    t2_special = (t2->type == COMPLEX && sn_to_node(t2->sym_num)->special_unary);
	
    if (t1_special && !t2_special) {
      if (term_ident(t1->farg->argval, t2))
	return(GREATER_THAN);
      else
	return(lex_order(t1->farg->argval, t2));
    }
    else if (!t1_special && t2_special) {
      if (term_ident(t2->farg->argval, t1))
	return(LESS_THAN);
      else
	return(lex_order(t1, t2->farg->argval));
    }
    else if (t1_special && t2_special) {
      int argcomp = lex_order(t1->farg->argval, t2->farg->argval);
      if (argcomp != SAME_AS)
	return(argcomp);
      /* else fall through and treat as normal terms */
    }
  }

  /* end of special_unary code */

  if (t1->type == VARIABLE)
    if (t2->type == VARIABLE)
      return(t1->varnum == t2->varnum ? SAME_AS : NOT_COMPARABLE);
    else
      return(occurs_in(t1, t2) ? LESS_THAN : NOT_COMPARABLE);
  else if (t2->type == VARIABLE)
    return(occurs_in(t2, t1) ? GREATER_THAN : NOT_COMPARABLE);
  else if (t1->sym_num == t2->sym_num) {
    r1 = t1->farg;
    r2 = t2->farg;
    i = SAME_AS;
    while (r1 && (i = lex_order(r1->argval,r2->argval)) == SAME_AS) {
      r1 = r1->narg;
      r2 = r2->narg;
    }
    return(i);
  }
  else
    return(lex_compare_sym_nums(t1->sym_num, t2->sym_num));
}  /* lex_order */

/*************
 *
 *    int lex_order_vars(t1, t2)
 *
 *    Similar to lex_order, except that variables are lowest, and are ordered
 *    by number.
 *
 *************/

int lex_order_vars(struct term *t1,
		   struct term *t2)
{
  struct rel *r1, *r2;
  int i, t1_special, t2_special;

  /* The following handles special unary functions that are to be */
  /* ignored during lex_check.  For example, when using lex-dependent */
  /* demodulation to sort AC expressions, you can make the canonical */
  /* form be a + -a + b + -b + c + -c.                              */

  if (Internal_flags[SPECIAL_UNARY_PRESENT]) {
    t1_special = (t1->type == COMPLEX && sn_to_node(t1->sym_num)->special_unary);
    t2_special = (t2->type == COMPLEX && sn_to_node(t2->sym_num)->special_unary);
    if (t1_special && !t2_special) {
      if (term_ident(t1->farg->argval, t2))
	return(GREATER_THAN);
      else
	return(lex_order_vars(t1->farg->argval, t2));
    }
    else if (!t1_special && t2_special) {
      if (term_ident(t2->farg->argval, t1))
	return(LESS_THAN);
      else
	return(lex_order_vars(t1, t2->farg->argval));
    }
    else if (t1_special && t2_special) {
      int argcomp = lex_order_vars(t1->farg->argval, t2->farg->argval);
      if (argcomp != SAME_AS)
	return(argcomp);
      /* else fall through and treat as normal terms */
    }
  }

  /* end of special_unary code */

  if (t1->type == VARIABLE)
    if (t2->type == VARIABLE)
      if (t1->varnum == t2->varnum)
	return(SAME_AS);
      else
	return(t1->varnum > t2->varnum ? GREATER_THAN : LESS_THAN);
    else
      return(LESS_THAN);

  else if (t2->type == VARIABLE)
    return(GREATER_THAN);

  else if (t1->sym_num == t2->sym_num) {
    r1 = t1->farg;
    r2 = t2->farg;
    i = SAME_AS;
    while (r1 && (i = lex_order_vars(r1->argval,r2->argval)) == SAME_AS) {
      r1 = r1->narg;
      r2 = r2->narg;
    }
    return(i);
  }
  else
    return(lex_compare_sym_nums(t1->sym_num, t2->sym_num));
}  /* lex_order_vars */

/*************
 *
 *    int wt_lex_order(t1, t2)
 *
 *    Return SAME_AS, GREATER_THAN, LESS_THAN, or NOT_COMPARABLE.
 *
 *************/

static int wt_lex_order(struct term *t1,
			struct term *t2)
{
  int i1, i2;

  i1 = weight(t1, Weight_terms_index);
  i2 = weight(t2, Weight_terms_index);

  if (i1 > i2)
    return(GREATER_THAN);
  else if (i1 < i2)
    return(LESS_THAN);
  else
    return(lex_order(t1, t2));
}  /* wt_lex_order */

/*************
 *
 *    int lex_check(t1, t2)
 *
 *    Return SAME_AS, GREATER_THAN, LESS_THAN, or NOT_COMPARABLE.
 *
 *    Consult a flag to see if variables should be considered.
 *
 *************/

int lex_check(struct term *t1,
	      struct term *t2)
{
  if (Flags[LEX_ORDER_VARS].val)
    return(lex_order_vars(t1, t2));
  else
    return(lex_order(t1, t2));
}  /* lex_check */

/*************
 *
 *    get_var_multiset(t, a)
 *
 *    Get (or continue getting) multiset of variables in t by
 *    Filling in array a.
 *
 *************/

static void get_var_multiset(struct term *t,
			     int *a)
{
  struct rel *r;

  if (t->type == VARIABLE)
    a[t->varnum]++;
  else if (t->type == COMPLEX) {
    r = t->farg;
    while (r != NULL) {
      get_var_multiset(r->argval, a);
      r = r->narg;
    }
  }
}  /* get_var_multiset */

/*************
 *
 *    int var_subset(t1, t2)
 *
 *    True if vars(t1) is a subset of vars(t2)
 *
 *************/

int var_subset(struct term *t1,
	       struct term *t2)
{
  int t1_vars[MAX_VARS], t2_vars[MAX_VARS], i;

  for (i = 0; i < MAX_VARS; i++)
    t1_vars[i] = t2_vars[i] = 0;

  get_var_multiset(t1, t1_vars);
  get_var_multiset(t2, t2_vars);

  /* now make sure every variable in t1 is in t2 */

  for (i = 0; i < MAX_VARS; i++)
    if (t2_vars[i] == 0 && t1_vars[i] != 0)
      return(0);

  return(1);

}  /* var_subset */

/*************
 *
 *    int sym_occur(sym_num, t)
 *
 *    True if sym_num is the symbol number of one of the constants
 *    or functors in t.
 *
 *************/

static int sym_occur(int sym_num,
		     struct term *t)
{
  struct rel *r;
  int found;

  if (t->type == VARIABLE)
    return(0);
  else if (t->sym_num == sym_num)
    return(1);  /* NAME or COMPLEX */
  else if (t->type == NAME)
    return(0);
  else {  /* complex with different sym_num */
    r = t->farg;
    found = 0;
    while (r != NULL && found == 0) {
      found = sym_occur(sym_num, r->argval);
      r = r->narg;
    }
    return(found);
  }
}  /* sym_occur */

/*************
 *
 *    sym_elim(alpha, beta)
 *
 *    True if alpha is complex, all args of alpha are unique vars, functor
 *    of alpha doesn't occur in beta, and subset(vars(beta),vars(alpha)) .
 *    (If true, alpha = beta can be made into a symbol-eliminating
 *    demodulator.)
 *
 *************/

static int sym_elim(struct term *alpha,
		    struct term *beta)
{
  struct rel *r;
  struct term *t1;
  int i, a[MAX_VARS], ok;

  if (alpha->type == VARIABLE)
    return(0);
  else {
    if (alpha->type == NAME)
      ok = 0;
    else {
      /* check for list of unique vars */
      for (i = 0; i < MAX_VARS; i++)
	a[i] = 0;
      ok = 1;
      r = alpha->farg;
      while (r != NULL && ok) {
	t1 = r->argval;
	ok = (t1->type == VARIABLE && a[t1->varnum] == 0);
	a[t1->varnum] = 1;
	r = r->narg;
      }
    }
    if (ok == 0)
      return(0);
    else { /* check that functor of alpha doesn't occur in beta */
      /* and that vars(beta) is a subset of vars(alpha)    */
      return(sym_occur(alpha->sym_num, beta) == 0 && var_subset(beta, alpha));
    }
  }
}  /* sym_elim */

/*************
 *
 *    order_equalities(c)
 *
 *    For each equality literal (pos or neg), flip args if the right
 *    side is heavier.  After possible filp, if the left side is
 *    heavier, set the ORIENTED_EQ_BIT in the atom.
 *    If the atom is flipped, set SCRATCH_BIT.
 *
 *************/

void order_equalities(struct clause *c)
{
  struct literal *l;
  struct rel *r1, *r2;
  struct term *alpha, *beta;
  int alpha_bigger, beta_bigger;

  for (l = c->first_lit; l; l = l->next_lit) {
    alpha_bigger = 0; beta_bigger = 0;
    if (eq_lit(l)) {
      r1 = l->atom->farg;
      r2 = r1->narg;
      alpha = r1->argval;
      beta  = r2->argval;
      if (!term_ident(alpha, beta)) {
	if (Flags[SYMBOL_ELIM].val && sym_elim(alpha, beta))
	  alpha_bigger = 1;
	else if (Flags[SYMBOL_ELIM].val && sym_elim(beta, alpha))
	  beta_bigger = 1;
	else if (occurs_in(beta, alpha))
	  alpha_bigger = 1;
	else if (occurs_in(alpha, beta))
	  beta_bigger = 1;
	else {
	  int rc;

	  rc = wt_lex_order(alpha, beta);
	  if (rc == GREATER_THAN)
	    alpha_bigger = 1;
	  else if (rc == LESS_THAN)
	    beta_bigger = 1;
	}

	if (alpha_bigger || beta_bigger) {
	  if (beta_bigger) {
	    r1->argval = beta;
	    r2->argval = alpha;
	    SET_BIT(l->atom->bits, SCRATCH_BIT);
	  }
	  SET_BIT(l->atom->bits, ORIENTED_EQ_BIT);
	}
      }
    }
  }
}  /* order_equalities */

/*************
 *
 *    int term_ident_x_vars(term1, term2) -- Compare two terms, ignoring variables
 *
 *        If identical except for vars, return(1); else return(0).  The bits
 *    field is not checked.
 *
 *************/

int term_ident_x_vars(struct term *t1,
		      struct term *t2)
{
  struct rel *r1, *r2;

  if (t1->type != t2->type)
    return(0);
  else if (t1->type == COMPLEX) {
    if (t1->sym_num != t2->sym_num)
      return(0);
    else {
      r1 = t1->farg;
      r2 = t2->farg;
      while (r1 && term_ident_x_vars(r1->argval,r2->argval)) {
	r1 = r1->narg;
	r2 = r2->narg;
      }
      return(r1 == NULL);
    }
  }
  else if (t1->type == VARIABLE)
    return(1);
  else  /* NAME */
    return(t1->sym_num == t2->sym_num);
}  /* term_ident_x_vars */

