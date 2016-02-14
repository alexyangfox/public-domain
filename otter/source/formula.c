/*
 *  formula.c
 *
 *    This file has routines to input and output quantified formulas and
 *    to convert them to lists of clauses (Skolemization and CNF translation).
 *
 */

#include "header.h"

static int Sk_func_num, Sk_const_num;  /* for creating new skolem symbols */

/*************
 *
 *    print_formula(fp, t) -- print a formula to a file.
 *
 *************/

void print_formula(FILE *fp,
		   struct formula *f)
{
#if 1
  struct term *t;

  t = formula_to_term(f);
  t = term_fixup_2(t);
  print_term(fp, t);
  zap_term(t);
#else
  char op[MAX_NAME];
  struct formula *f1;

  if (f == NULL)
    fprintf(fp, "(nil)");
  else if (f->type == ATOM_FORM) {
	
    print_term(fp, f->t);
  }
  else if (f->type == NOT_FORM) {
    fprintf(fp, "-");
    print_formula(fp, f->first_child);
  }
  else if (f->type == AND_FORM && f->first_child == NULL)
    fprintf(fp, "TRUE");
  else if (f->type == OR_FORM && f->first_child == NULL)
    fprintf(fp, "FALSE");
  else if (f->type == QUANT_FORM) {
    fprintf(fp, "(");
    if (f->quant_type == ALL_QUANT)
      fprintf(fp, "all ");
    else
      fprintf(fp, "exists ");
    print_term(fp, f->t);
    fprintf(fp, " ");
    print_formula(fp, f->first_child);
    fprintf(fp, ")");
  }
  else {
    if (f->type == AND_FORM)
      strcpy(op, "& ");
    else if (f->type == OR_FORM)
      strcpy(op, "| ");
    else if (f->type == IMP_FORM)
      strcpy(op, "-> ");
    else if (f->type == IFF_FORM)
      strcpy(op, "<-> ");
    else
      op[0] = '\0';
	
    fprintf(fp, "(");
    for (f1 = f->first_child; f1; f1 = f1->next) {
      print_formula(fp, f1);
      if (f1->next)
	fprintf(fp, " %s", op);
    }
    fprintf(fp, ")");
  }
#endif
}  /* print_formula */

/*************
 *
 *    p_formula(f) -- print formula to standard output
 *
 *************/

void p_formula(struct formula *f)
{
  print_formula(stdout, f);
}  /* p_formula */

/*************
 *
 *    struct term *formula_args_to_term(f, type)
 *
 *    Conver list of formulas to right-associated term.
 *    Works for AND_FORM, OR_FORM, IMP_FORM, IFF_FORM.
 *
 *************/

static struct term *formula_args_to_term(struct formula *f,
					 int type)
{
  struct term *t, *t1, *t2;
  struct rel *r1, *r2;

  if (!f) {  /* empty disjunction or conjunction */
    t = get_term();
    t->type = NAME;
    if (type == AND_FORM)
      t->sym_num = str_to_sn("TRUE", 0);
    else
      t->sym_num = str_to_sn("FALSE", 0);
  }
  else {
    t1 = formula_to_term(f);
    if (f->next) {
      t2 = formula_args_to_term(f->next, type);
      t = get_term(); r1 = get_rel(); r2 = get_rel();
      t->farg = r1; r1->narg = r2;
      r1->argval = t1; r2->argval = t2;
      t->type = COMPLEX;
      switch (type) {
      case AND_FORM: t->sym_num = str_to_sn("&", 2); break;
      case OR_FORM:  t->sym_num = str_to_sn("|", 2); break;
      case IMP_FORM: t->sym_num = str_to_sn("->", 2); break;
      case IFF_FORM: t->sym_num = str_to_sn("<->", 2); break;
      }
    }
    else
      t = t1;
  }
  return(t);
}  /* formula_args_to_term */

/*************
 *
 *    struct term *formula_to_term(f)
 *
 *************/

struct term *formula_to_term(struct formula *f)
{
  struct term *t, *t1;
  struct rel *r, *r1, *prev_r;
  int prev_q, i;
  struct formula *f1;

  switch (f->type) {
  case ATOM_FORM: t = copy_term(f->t); break;
  case IMP_FORM:
  case IFF_FORM:
  case AND_FORM:
  case OR_FORM:   t = formula_args_to_term(f->first_child, f->type); break;
  case NOT_FORM:
    t = get_term();
    t->type = COMPLEX;
    t->sym_num = str_to_sn("-", 1);
    r = get_rel();
    t->farg = r;
    r->argval = formula_to_term(f->first_child);
    break;
  case QUANT_FORM:
    t = get_term();
    t->type = COMPLEX;
    i = 0; prev_q = MAX_INT; prev_r = NULL;
    for (f1 = f; f1->type == QUANT_FORM; f1 = f1->first_child) {
      if (f1->quant_type != prev_q) {
	i++;
	t1 = get_term();
	r1 = get_rel();
	r1->argval = t1;
	if (!t->farg)
	  t->farg = r1;
	else
	  prev_r->narg = r1;
	prev_r = r1;
	t1->type = NAME;
	if (f1->quant_type == ALL_QUANT)
	  t1->sym_num = str_to_sn("all", 0);
	else
	  t1->sym_num = str_to_sn("exists", 0);
	prev_q = f1->quant_type;
      }
      i++;
      r1 = get_rel();
      r1->argval = copy_term(f1->t);  /* variable */
      prev_r->narg = r1;
      prev_r = r1;
    }
    t->sym_num = str_to_sn("$Quantified", i);
    r1 = get_rel();
    prev_r->narg = r1;
    r1->argval = formula_to_term(f1);
    break;
  default: t = NULL;
  }
  return(t);
}  /* formula_to_term */

/*************
 *
 *    struct formula *term_to_formula(t)
 *
 *************/

struct formula *term_to_formula(struct term *t)
{
  struct formula *f1, *f2, *f3;
  struct rel *r;
  int type;

  type = MAX_INT;

  if (is_symbol(t, "&", 2))
    type = AND_FORM;
  else if (is_symbol(t, "|", 2))
    type = OR_FORM;
  else if (is_symbol(t, "->", 2))
    type = IMP_FORM;
  else if (is_symbol(t, "<->", 2))
    type = IFF_FORM;

  if (type != MAX_INT) {
    f1 = get_formula();
    f1->type = type;
    f1->first_child = term_to_formula(t->farg->argval);
    f1->first_child->next =  term_to_formula(t->farg->narg->argval);
    if (type == AND_FORM || type == OR_FORM)
      flatten_top(f1);
  }
  else if (is_symbol(t, "-", 1)) {
    f1 = get_formula();
    f1->type = NOT_FORM;
    f1->first_child = term_to_formula(t->farg->argval);
  }
  else if (t->type == COMPLEX &&
	   str_ident(sn_to_str(t->sym_num), "$Quantified")) {
    f3 = f1 = NULL;
    for (r = t->farg; r->narg; r = r->narg) {
      if (is_symbol(r->argval, "all", 0))
	type = ALL_QUANT;
      else if (is_symbol(r->argval, "exists", 0))
	type = EXISTS_QUANT;
      else {
	f2 = get_formula();
	if (f3)
	  f3->first_child = f2;
	else
	  f1 = f2;
	f2->type = QUANT_FORM;
	f2->quant_type = type;
	f2->t = copy_term(r->argval);
	f3 = f2;
      }
    }
    f3->first_child = term_to_formula(r->argval);
  }
  else {  /* assume atomic formula */
    f1 = get_formula();
    f1->type = ATOM_FORM;
    f1->t = copy_term(t);
  }
  return(f1);
	
}  /* term_to_formula */

/*************
 *
 *    struct formula *read_formula(fp, rcp) -- read a formula from a file
 *
 *    The return code *rcp:
 *        0 - an error was encountered and reported; NULL is returned.
 *        1 - OK; if EOF was found instead of a formula, NULL is returned.
 *
 *************/

struct formula *read_formula(FILE *fp,
			     int *rcp)
{
  int rc;
  struct formula *f;
  struct term *t;

  t = read_term(fp, &rc);
  if (!rc) {
    *rcp = 0;
    return(NULL);
  }
  else if (t == NULL) {
    *rcp = 1;
    return(NULL);
  }
  else {
    if (contains_skolem_symbol(t)) {
      fprintf(stdout, "\nERROR, input formula contains Skolem symbol:\n");
      print_term(stdout, t); printf(".\n\n");
      zap_term(t);
      *rcp = 0;
      return(NULL);
    }
    else {
      f = term_to_formula(t);
      zap_term(t);
      *rcp = 1;
      return(f);
    }
  }
}  /* read_formula */

/*************
 *
 *    struct term_ptr *read_formula_list(file_ptr, errors_ptr)
 *
 *    Read and return a list of quantified formulas.
 *
 *    The list must be terminated either with the term `end_of_list.'
 *    or with an actual EOF.
 *    Set errors_ptr to point to the number of errors found.
 *
 *************/

struct formula_ptr *read_formula_list(FILE *fp,
				      int *ep)
{
  struct formula_ptr *p1, *p2, *p3;
  struct formula *f;
  int rc;

  Internal_flags[REALLY_CHECK_ARITY] = 1;

  *ep = 0;
  p3 = NULL;
  p2 = NULL;
  f = read_formula(fp, &rc);
  while (rc == 0) {
    (*ep)++;
    f = read_formula(fp, &rc);
  }

  /* keep going until f == NULL || f is end marker */

  while (f && !(f->type == ATOM_FORM &&
		is_symbol(f->t, "end_of_list", 0))) {
    p1 = get_formula_ptr();
    p1->f = f;
    if (p2 == NULL)
      p3 = p1;
    else
      p2->next = p1;
    p2 = p1;
    f = read_formula(fp, &rc);
    while (rc == 0) {
      (*ep)++;
      f = read_formula(fp, &rc);
    }
  }
  if (f != NULL)
    zap_formula(f);

  Internal_flags[REALLY_CHECK_ARITY] = 0;

  return(p3);
}  /* read_formula_list */

/*************
 *
 *    print_formula_list(file_ptr, term_ptr)
 *
 *    Print a list of quantified formulas.
 *
 *    The list is printed with periods after each quantified formula, and
 *    the list is terminated with `end_of_list.' so that it can
 *    be read with read_formula_list.
 *
 *************/

void print_formula_list(FILE *fp,
			struct formula_ptr *p)
{
  while (p != NULL) {
    print_formula(fp, p->f); fprintf(fp, ".\n");
    p = p->next;
  }
  fprintf(fp, "end_of_list.\n");
}  /* print_formula_list */

/*************
 *
 *    struct formula *copy_formula(f)
 *
 *    Copy a formula.  copy_term is used to copy atoms and quantified vars.
 *
 *************/

struct formula *copy_formula(struct formula *f)
{
  struct formula *f_new, *f_sub, *f_prev, *f3;

  f_new = get_formula();
  f_new->type = f->type;

  if (f->type == ATOM_FORM)
    f_new->t = copy_term(f->t);
  else if (f->type == QUANT_FORM) {
    f_new->quant_type = f->quant_type;
    f_new->t = copy_term(f->t);
    f_new->first_child = copy_formula(f->first_child);
  }
  else {
    f_prev = NULL;
    for (f_sub = f->first_child; f_sub; f_sub = f_sub->next) {
      f3 = copy_formula(f_sub);
      if (f_prev)
	f_prev->next = f3;
      else
	f_new->first_child = f3;
      f_prev = f3;
    }
  }
  return(f_new);
	
}  /* copy_formula  */

/*************
 *
 *    void zap_formula(f)
 *
 *    Free a formula and all of its subformulas and subterms.
 *
 *************/

void zap_formula(struct formula *f)
{
  struct formula *f1, *f2;

  if (f->type == ATOM_FORM)
    zap_term(f->t);
  else {
    f1 = f->first_child;
    while (f1) {
      f2 = f1;
      f1 = f1->next;
      zap_formula(f2);
    }
    if (f->type == QUANT_FORM)
      zap_term(f->t);
  }
  free_formula(f);
}  /* zap_formula */

/*************
 *
 *    struct formula *negate_formula(f)
 *
 *    f is changed to its negation.  (Do not move negation signs inward.)
 *
 *************/

struct formula *negate_formula(struct formula *f)
{
  struct formula  *f1, *f_save;

  /* save next pointer */
  f_save = f->next; f->next = NULL;

  if (f->type == NOT_FORM) {
    f1 = f->first_child;
    free_formula(f);
  }
  else {
    f1 = get_formula();
    f1->type = NOT_FORM;
    f1->first_child = f;
  }
  /* restore next pointer */
  f1->next = f_save;
  return(f1);
}  /* negate_formula */

/*************
 *
 *    struct formula *nnf(f)
 *
 *    f is changed into its negation normal form (NNF) by removing
 *    -> and <-> and moving negation signs all the way in.
 *
 *     (A <-> B) (not negated) rewrites to ((-a | b) & (-b | a)).
 *    -(A <-> B)               rewrites to ((a | b) & (-a | -b)).
 *
 *    because conjunctions are favored.
 *
 *************/

struct formula *nnf(struct formula *f)
{
  struct formula *f1, *f2, *next, *prev, *fn;

  switch (f->type) {
  case ATOM_FORM:
    return(f);  /* f is atomic */
  case IFF_FORM:
    f1 = get_formula();
    f1->type = AND_FORM;
    f1->first_child = f;
    f1->next = f->next;

    f2 = copy_formula(f);
    f2->type = OR_FORM;
    f2->first_child->next = negate_formula(f2->first_child->next);

    f->type = OR_FORM;
    f->first_child = negate_formula(f->first_child);
    f->next = f2;
    return(nnf(f1));
  case IMP_FORM:
    f->type = OR_FORM;
    f->first_child = negate_formula(f->first_child);
    return(nnf(f));
  case QUANT_FORM:
    f->first_child = nnf(f->first_child);
    return(f);
  case AND_FORM:
  case OR_FORM:
    prev = NULL;
    f1 = f->first_child;
    while(f1) {
      next = f1->next;  f1->next = NULL;
      f2 = nnf(f1);
      if (prev)
	prev->next = f2;
      else
	f->first_child = f2;
      prev = f2;
      f1 = next;
    }
    return(f);

  case NOT_FORM:
    fn = f->first_child;
    switch (fn->type) {
    case ATOM_FORM:
      return(f);
    case IFF_FORM:
      f2 = copy_formula(fn);
      f2->type = OR_FORM;
      fn->type = OR_FORM;
      f2->first_child = negate_formula(f2->first_child);
      f2->first_child->next = negate_formula(f2->first_child->next);
      fn->next = f2;
      f->type = AND_FORM;
      f->first_child = fn;
      return(nnf(f));
    case IMP_FORM:
      fn->type = OR_FORM;
      fn->first_child = negate_formula(fn->first_child);
      return(nnf(f));
    case QUANT_FORM:
      fn->quant_type = (fn->quant_type == ALL_QUANT ? EXISTS_QUANT : ALL_QUANT);
      fn->first_child = nnf(negate_formula(fn->first_child));
      fn->next = f->next;
      free_formula(f);
      return(fn);
    case AND_FORM:
    case OR_FORM:
      prev = NULL;
      f1 = fn->first_child;
      while(f1) {
	next = f1->next;  f1->next = NULL;
	f2 = nnf(negate_formula(f1));
	if (prev)
	  prev->next = f2;
	else
	  fn->first_child = f2;
	prev = f2;
	f1 = next;
      }
      fn->type = (fn->type == AND_FORM ? OR_FORM : AND_FORM);
      fn->next = f->next;
      free_formula(f);
      return(fn);
	
    case NOT_FORM:    /* double negation */
      f1 = fn->first_child;
      f1->next = f->next;
      free_formula(f);
      free_formula(fn);
      return(nnf(f1));
    }
  }
  return(NULL);  /* ERROR */
}  /* nnf */

/*************
 *
 *    static void rename_free_formula(f, old_sn, new_sn)
 *
 *    Rename free occurrences of old_sn in NAMEs to new_sn.
 *    Recall that variables in formulas are really NAMEs.
 *
 *************/

static void rename_free_formula(struct formula *f,
				int old_sn,
				int new_sn)
{
  struct formula *f1;

  if (f->type == ATOM_FORM)
    subst_sn_term(old_sn, f->t, new_sn, NAME);
  else if (f->type == QUANT_FORM) {
    if (old_sn != f->t->sym_num)
      rename_free_formula(f->first_child, old_sn, new_sn);
  }
  else {
    for (f1 = f->first_child; f1; f1 = f1->next)
      rename_free_formula(f1, old_sn, new_sn);
  }
	
}  /* rename_free_formula  */

/*************
 *
 *    static struct formula *skolem(f, vars)
 *
 *    Skolemize f w.r.t universally quantified vars.
 *    Called by skolemize.
 *
 *************/

static struct formula *skolem(struct formula *f,
			      struct term *vars)
{
  struct formula *f1, *f2, *prev, *next;
  struct rel *end, *r2;
  int sn;

  if (f->type == NOT_FORM && f->first_child->type != ATOM_FORM) {
    printf("ERROR, skolem gets negated non-atom: ");
    print_formula(stdout, f);
    printf("\n");
  }
  else if (f->type == IMP_FORM || f->type == IFF_FORM) {
    printf("ERROR, skolem gets: ");
    print_formula(stdout, f);
    printf("\n");
  }
  else if (f->type == AND_FORM || f->type == OR_FORM) {
    prev = NULL;
    f1 = f->first_child;
    while(f1) {
      next = f1->next;  f1->next = NULL;
      f2 = skolem(f1, vars);
      if (prev)
	prev->next = f2;
      else
	f->first_child = f2;
      prev = f2;
      f1 = next;
    }
  }
  else if (f->type == QUANT_FORM) {
    if (f->quant_type == ALL_QUANT) {
      if (occurs_in(f->t, vars)) {
	/*
	  rename current variable, because we are already in the
	  scope of a universally quantified var with that name.
	*/
	sn = new_var_name();
	rename_free_formula(f->first_child, f->t->sym_num, sn);
	f->t->sym_num = sn;
      }
      r2 = get_rel();
      r2->argval = f->t;

      /* Install variable at end of vars. */
      for (end = vars->farg; end && end->narg; end = end->narg);
      if (end)
	end->narg = r2;
      else
	vars->farg = r2;

      f->first_child = skolem(f->first_child, vars);

      /* Remove variable from vars. */

      free_rel(r2);
      if (end)
	end->narg = NULL;
      else
	vars->farg = NULL;
    }
    else {  /* existential quantifier */
      /*
	must skolemize subformula first to avoid problem in
	Ax...Ey...Ex F(x,y).
      */
      f->first_child = skolem(f->first_child, vars);
	
      gen_sk_sym(vars);  /* fills in sym_num and assigns type */
      subst_free_formula(f->t, f->first_child, vars);
      vars->type = COMPLEX; /* so that occurs_in above works */

      f1 = f->first_child;
      zap_term(f->t);
      free_formula(f);
      f = f1;
    }
  }
  return(f);
}  /* skolem */

/*************
 *
 *    struct formula *skolemize(f) -- Skolemize a formula
 *
 *    This routine assumes that f is in negation normal form.
 *    The existential quantifiers are deleted.
 *
 *************/

struct formula *skolemize(struct formula *f)
{
  struct term *vars;

  vars = get_term();
  vars->type = COMPLEX;
  f = skolem(f, vars);
  free_term(vars);
  return(f);

}  /* skolemize */

/*************
 *
 *    struct formula *anti_skolemize(f) -- Anti-Skolemize a formula
 *
 *    The dual of skolemize:  universal quantifiers are removed.
 *
 *************/

struct formula *anti_skolemize(struct formula *f)
{
  return(nnf(negate_formula(skolemize(nnf(negate_formula(f))))));
}  /* anti_skolemize */

/*************
 *
 *    static void subst_free_term(var, t, sk)
 *
 *    Substitute free occurrences of var in t with copies of sk.
 *
 *************/

static void subst_free_term(struct term *var,
			    struct term *t,
			    struct term *sk)
{
  struct rel *r;

  if (t->type != COMPLEX)
    return;
  else {
    r = t->farg;
    for (r = t->farg; r; r = r->narg) {
      if (term_ident(var, r->argval)) {
	zap_term(r->argval);
	r->argval = copy_term(sk);
      }
      else
	subst_free_term(var, r->argval, sk);
    }
  }
}  /* subst_free_term */

/*************
 *
 *    void subst_free_formula(var, f, sk)

 *    Substitute free occurrences of var in f with copies of sk.
 *
 *************/

void subst_free_formula(struct term *var,
			struct formula *f,
			struct term *sk)
{
  struct formula *f1;

  if (f->type == ATOM_FORM)
    subst_free_term(var, f->t, sk);
  else if (f->type == QUANT_FORM) {
    if (!term_ident(f->t, var))
      subst_free_formula(var, f->first_child, sk);
  }
  else {
    for (f1 = f->first_child; f1; f1 = f1->next)
      subst_free_formula(var, f1, sk);
  }
	
}  /* subst_free_formula  */

/*************
 *
 *    gen_sk_sym(t) -- generate a fresh skolem symbol for term t.
 *
 *    Assign type field as well as sym_num field to term t.
 *
 *************/

void gen_sk_sym(struct term *t)
{
  int arity;
  struct rel *r;
  char s1[MAX_NAME], s2[MAX_NAME];

  arity = 0;
  r = t->farg;
  while (r != NULL) {
    arity++;
    r = r->narg;
  }

  if (arity == 0) {
    t->type = NAME;
    int_str(++Sk_const_num, s1);
    cat_str("$c", s1, s2);
  }
  else {
    t->type = COMPLEX;
    int_str(++Sk_func_num, s1);
    cat_str("$f", s1, s2);
  }

  t->sym_num = str_to_sn(s2, arity);
  mark_as_skolem(t->sym_num);

}  /* gen_sk_sym */

/*************
 *
 *    int skolem_symbol(sn) -- Is sn the symbol number of a skolem symbol?
 *
 *    Check if it is "$cn" or "$fn" for integer n.
 *    Do not check the skolem flag in the symbol node.
 *
 *************/

int skolem_symbol(int sn)
{
  char *s;
  int dummy;

  s = sn_to_str(sn);
  return(*s == '$' &&
	 (*(s+1) == 'c' || *(s+1) == 'f') &&
	 str_int(s+2,&dummy));
}  /* skolem_symbol */

/*************
 *
 *    int contains_skolem_symbol(t)
 *
 *    Check if any of the NAMEs in t are  "$cn" or "$fn", for integer n.
 *
 *************/

int contains_skolem_symbol(struct term *t)
{
  struct rel *r;

  if (t->type == VARIABLE)
    return(0);
  else if (t->type == NAME)
    return(skolem_symbol(t->sym_num));
  else {  /* COMPLEX */
    if (skolem_symbol(t->sym_num))
      return(1);
    else {
      for (r = t->farg; r; r = r->narg)
	if (contains_skolem_symbol(r->argval))
	  return(1);
      return(0);
    }
  }
}  /* contains_skolem_symbol */

/*************
 *
 *    int new_var_name() -- return a sym_num for a new VARIABLE symbol
 *
 *    Check and make sure that the new symbol does not occur in the
 *     symbol table.
 *
 *************/

int new_var_name(void)
{
  char s1[MAX_NAME], s2[MAX_NAME];

  static int var_num;
  char c[2];

  c[0] = (Flags[PROLOG_STYLE_VARIABLES].val ? 'X' : 'x');
  c[1] = '\0';

  int_str(++var_num, s1);
  cat_str(c, s1, s2);
  while (in_sym_tab(s2)) {
    int_str(++var_num, s1);
    cat_str(c, s1, s2);
  }

  return(str_to_sn(s2, 0));

}  /* new_var_name */

/*************
 *
 *    int new_functor_name(arity) -- return a sym_num for a new symbol.
 *
 *    Check and make sure that the new symbol does not occur in the symbol table.
 *
 *************/

int new_functor_name(int arity)
{
  char s1[MAX_NAME], s2[MAX_NAME];

  static int functor_num;

  int_str(++functor_num, s1);
  cat_str("k", s1, s2);
  while (in_sym_tab(s2)) {
    int_str(++functor_num, s1);
    cat_str("k", s1, s2);
  }

  return(str_to_sn(s2, arity));

}  /* new_functor_name */

/*************
 *
 *    static void uq_all(f, vars) -- called by unique_all
 *
 *************/

static void uq_all(struct formula *f,
		   struct term *vars)
{
  struct rel *r1;
  struct formula *f1;
  int sn;

  switch (f->type) {
  case ATOM_FORM: break;
  case NOT_FORM:
  case AND_FORM:
  case OR_FORM:
    for (f1 = f->first_child; f1; f1 = f1->next)
      uq_all(f1, vars);
    break;
  case QUANT_FORM:
    if (occurs_in(f->t, vars)) {
      /*
	rename current variable, because already have
	a quantified var with that name.
      */
      sn = new_var_name();
      rename_free_formula(f->first_child, f->t->sym_num, sn);
      f->t->sym_num = sn;
    }
    else {
      r1 = get_rel();
      r1->argval = f->t;
      r1->narg = vars->farg;
      vars->farg = r1;
    }
	
    /* recursive call on quantified formula */
    uq_all(f->first_child, vars);
    break;
  }
}  /* uq_all */

/*************
 *
 *    void unique_all(f) -- make all universally quantified variables unique
 *
 *    It is assumed that f is in negation normal form and is Skolemized (no
 *    existential quantifiers).
 *
 *************/

void unique_all(struct formula *f)
{
  struct term *vars;
  struct rel *r1, *r2;

  vars = get_term();
  vars->type = COMPLEX;
  uq_all(f, vars);
  r1 = vars->farg;
  while (r1 != NULL) {
    r2 = r1;
    r1 = r1->narg;
    free_rel(r2);
  }
  free_term(vars);
}  /* unique_all */

/*************
 *
 *    static mark_free_var_term(v, t) -- mark free occurrences of v in t
 *
 *    Each free NAME in t with sym_num == v->sym_num is marked as
 *    a VARIABLE by setting the type field to VARIABLE.
 *
 *************/

static void mark_free_var_term(struct term *v,
			       struct term *t)
{
  struct rel *r;
  struct term *t1;

  if (t->type != COMPLEX)
    return;
  else {
    r = t->farg;
    for (r = t->farg; r; r = r->narg) {
      t1 = r->argval;
      if (t1->type == NAME) {
	if (t1->sym_num == v->sym_num) {
	  t1->type = VARIABLE;
	  /*
	    bug fix 31-Jan-91. WWM.  The following line was added
	    because term-ident (called if simplify_fol) does not
	    check sym_num field for vars.  It is a trick.
	  */
	  t1->varnum = t1->sym_num;
	}
      }
      else
	mark_free_var_term(v, t1);
    }
  }
}  /* mark_free_var_term */

/*************
 *
 *    static void mark_free_var_formula(v, f)
 *
 *************/

static void mark_free_var_formula(struct term *v,
				  struct formula *f)
{
  struct formula *f1;

  if (f->type == ATOM_FORM)
    mark_free_var_term(v, f->t);
  else {
    for (f1 = f->first_child; f1; f1 = f1->next)
      mark_free_var_formula(v, f1);
  }
}  /* mark_free_var_formula */

/*************
 *
 *    struct term *zap_quant(f)
 *
 *    Delete quantifiers and mark quantified variables.
 *
 *    It is assumed that f is skolemized nnf with unique universally
 *    quantified variables.  For each universal quantifier,
 *    mark all occurrences of the quantified variable by setting the type field
 *    to VARIABLE, then delete the quantifier.
 *    All QUANT_FORM nodes are deleted as well.
 *
 *************/

struct formula *zap_quant(struct formula *f)
{

  struct formula *f1, *f2, *prev, *next;

  switch (f->type) {
  case ATOM_FORM:
    break;
  case NOT_FORM:
  case AND_FORM:
  case OR_FORM:
    prev = NULL;
    f1 = f->first_child;
    while(f1) {
      next = f1->next;  f1->next = NULL;
      f2 = zap_quant(f1);
      if (prev)
	prev->next = f2;
      else
	f->first_child = f2;
      prev = f2;
      f1 = next;
    }
    break;
  case QUANT_FORM:
    mark_free_var_formula(f->t, f->first_child);
    f1 = f->first_child;
    f1->next = f->next;
    free_formula(f);
    f = zap_quant(f1);
    break;
  }
  return(f);
}  /* zap_quant */

/*************
 *
 *    static void flatten_top_2(f, start, end_p) -- called by flatten_top.
 *
 *************/

static void flatten_top_2(struct formula *f,
			  struct formula *start,
			  struct formula **end_p)
{
  struct formula *f1, *f2;

  f1 = f->first_child;
  while (f1) {
    f2 = f1;
    f1 = f1->next;
    if (f2->type == f->type) {
      flatten_top_2(f2, start, end_p);
      free_formula(f2);
    }
    else {
      if (*end_p)
	(*end_p)->next = f2;
      else
	start->first_child = f2;
      *end_p = f2;
    }
  }
}  /* flatten_top_2 */

/*************
 *
 *    void flatten_top(f) -- flatten conjunctions or disjunctions
 *
 *    The top part of f is flattened.  Subtrees below
 *    a node of the oppposite type are not flattened.  For example, in
 *    (a or (b and (c or (d or e)))), the formula (c or (d or e)) is never
 *    flattened.
 *
 *************/

void flatten_top(struct formula *f)
{
  struct formula *end;

  if (f->type == AND_FORM || f->type == OR_FORM) {
    end = NULL;
    flatten_top_2(f, f, &end);
    if (end)
      end->next = NULL;
    else
      f->first_child = NULL;
  }
}  /* flatten_top */

/*************
 *
 *    static struct formula *distribute(f) -- distribute OR over AND.
 *
 *    f is an OR node whose subterms are in CNF.  This routine returns
 *    a CNF of f.
 *
 *************/

static struct formula *distribute(struct formula *f)
{
  struct formula *f_new, *f1, *f2, *f3, *f4, *f_prev, *f_save;
  int i, j;

  f_save = f->next; f->next = NULL;

  if (f->type != OR_FORM)
    return(f);
  else {

    flatten_top(f);
    if (Flags[SIMPLIFY_FOL].val) {
      conflict_tautology(f);
      f = subsume_disj(f);
    }
    if (f->type != OR_FORM)
      return(f);
    else {
	
      /* find first AND subformula */
      i = 1;
      f_prev = NULL;
      for (f1 = f->first_child; f1 && f1->type != AND_FORM; f1 = f1->next) {
	i++;
	f_prev = f1;
      }
      if (f1 == NULL)
	return(f);  /* nothing to distribute */
      else {
	/* unhook AND */
	if (f_prev)
	  f_prev->next = f1->next;
	else
	  f->first_child = f1->next;
	f2 = f1->first_child;
	f_new = f1;
	f_prev = NULL;
	while (f2) {
	  f3 = f2->next;
	  if (f3)
	    f1 = copy_formula(f);
	  else
	    f1 = f;
	  if (i == 1) {
	    f2->next = f1->first_child;
	    f1->first_child = f2;
	  }
	  else {
	    j = 1;
	    for (f4 = f1->first_child; j < i-1; f4 = f4->next)
	      j++;
	    f2->next = f4->next;
	    f4->next = f2;
	  }
	  f1 = distribute(f1);
	  if (f_prev)
	    f_prev->next = f1;
	  else
	    f_new->first_child = f1;
	  f_prev = f1;
	  f2 = f3;
	}
	f_new->next = f_save;
	flatten_top(f_new);
	if (Flags[SIMPLIFY_FOL].val) {
	  conflict_tautology(f_new);
	  f_new = subsume_conj(f_new);
	}
	return(f_new);
      }
    }
  }
}  /* distribute */

/*************
 *
 *    struct formula *cnf(f) -- convert nnf f to conjunctive normal form.
 *
 *************/

struct formula *cnf(struct formula *f)
{
  struct formula *f1, *f2, *f_prev, *f_next, *f_save;

  f_save = f->next; f->next = NULL;

  if (f->type == AND_FORM || f->type == OR_FORM) {
    /* first convert subterms to CNF */
    f_prev = NULL;
    f1 = f->first_child;
    while(f1) {
      f_next = f1->next;
      f2 = cnf(f1);
      if (f_prev)
	f_prev->next = f2;
      else
	f->first_child = f2;
      f_prev = f2;
      f1 = f_next;
    }

    if (f->type == AND_FORM) {
      flatten_top(f);
      if (Flags[SIMPLIFY_FOL].val) {
	conflict_tautology(f);
	f = subsume_conj(f);
      }
    }
    else
      f = distribute(f);  /* flatten and simplify in distribute */
  }

  f->next = f_save;
  return(f);

}  /* cnf */

/*************
 *
 *    struct formula *dnf(f) -- convert f to disjunctive normal form.
 *
 *************/

struct formula *dnf(struct formula *f)
{
  return(nnf(negate_formula(cnf(nnf(negate_formula(f))))));
}  /* dnf */

/*************
 *
 *    static void rename_syms_term(t, fr)
 *
 *    Called from rename_syms_formula.
 *
 *************/

static void rename_syms_term(struct term *t,
			     struct formula *fr)
{
  struct rel *r;
  int sn;

  if (t->type == NAME) {
    if (var_name(sn_to_str(t->sym_num))) {
      fprintf(stderr,"\nWARNING, the following formula has constant '%s', whose\nname may be misinterpreted by the user as a variable.\n", sn_to_str(t->sym_num));
      print_formula(stderr, fr);  fprintf(stderr, "\n");
#if 0  /* replaced 18 June 91 WWM */
      sn = new_functor_name(0);  /* with arity 0 */
      subst_sn_formula(t->sym_num, fr, sn, NAME);
#endif	
    }
  }
  else if (t->type == VARIABLE) {
    if (!var_name(sn_to_str(t->sym_num))) {
      sn = new_var_name();
      subst_sn_formula(t->sym_num, fr, sn, VARIABLE);
    }
  }
  else {
    r = t->farg;
    while(r != NULL) {
      rename_syms_term(r->argval, fr);
      r = r->narg;
    }
  }
}  /* rename_syms_term */

/*************
 *
 *    void rename_syms_formula(f, fr)
 *
 *    Rename VARIABLEs so that they conform to the rule for clauses.
 *
 *************/

void rename_syms_formula(struct formula *f,
			 struct formula *fr)
{
  struct formula *f1;

  if (f->type == ATOM_FORM)
    rename_syms_term(f->t, fr);
  else {
    for (f1 = f->first_child; f1; f1 = f1->next)
      rename_syms_formula(f1, fr);
  }
}  /* rename_syms_formula */

/*************
 *
 *    void subst_sn_term(old_sn, t, new_sn, type)
 *
 *************/

void subst_sn_term(int old_sn,
		   struct term *t,
		   int new_sn,
		   int type)
{
  struct rel *r;

  if (t->type == NAME) {
    if (type == NAME && t->sym_num == old_sn)
      t->sym_num = new_sn;
  }
  else if (t->type == VARIABLE) {
    if (type == VARIABLE && t->sym_num == old_sn)
      t->sym_num = new_sn;
  }
  else {
    for (r = t->farg; r; r = r->narg)
      subst_sn_term(old_sn, r->argval, new_sn, type);
  }
}  /* subst_sn_term */

/*************
 *
 *    void subst_sn_formula(old_sn, f, new_sn, type)
 *
 *************/

void subst_sn_formula(int old_sn,
		      struct formula *f,
		      int new_sn,
		      int type)
{
  struct formula *f1;

  if (f->type == ATOM_FORM)
    subst_sn_term(old_sn, f->t, new_sn, type);
  else {
    for (f1 = f->first_child; f1; f1 = f1->next)
      subst_sn_formula(old_sn, f1, new_sn, type);
  }
}  /* subst_sn_formula */

/*************
 *
 *    int gen_subsume_prop(c, d) -- does c gen_subsume_prop d?
 *
 *    This is generalized propositional subsumption.  If given
 *    quantified formulas, they are treated as atoms (formula_ident
 *    determines outcome).
 *
 *************/

int gen_subsume_prop(struct formula *c,
		     struct formula *d)
{
  struct formula *f;

  /* The order of these tests is important.  For example, if */
  /* the last test is moved to the front, c=(p|q) will not   */
  /* subsume d=(p|q|r).                                      */

  if (c->type == OR_FORM) {  /* return(each c_i subsumes d) */
    for (f = c->first_child; f && gen_subsume_prop(f, d); f = f->next);
    return(f == NULL);
  }
  else if (d->type == AND_FORM) {  /* return(c subsumes each d_i) */
    for (f = d->first_child; f && gen_subsume_prop(c, f); f = f->next);
    return(f == NULL);
  }
  else if (c->type == AND_FORM) {  /* return(one c_i subsumes d) */
    for (f = c->first_child; f && ! gen_subsume_prop(f, d); f = f->next);
    return(f != NULL);
  }
  else if (d->type == OR_FORM) {  /* return(c subsumes one d_i) */
    for (f = d->first_child; f && ! gen_subsume_prop(c, f); f = f->next);
    return(f != NULL);
  }
  else  /* c and d are NOT, ATOM, or QUANT */
    return(formula_ident(c, d));

}  /* gen_subsume_prop */

/*************
 *
 *    struct formula *subsume_conj(c)
 *
 *    Given a conjunction, discard weaker conjuncts.
 *    This is like deleting subsumed clauses.
 *    The result is equivalent.
 *
 *************/

struct formula *subsume_conj(struct formula *c)
{
  struct formula *f1, *f2, *f3, *prev;

  if (c->type != AND_FORM  || c->first_child == NULL)
    return(c);
  else {
    /* start with second child */
    prev = c->first_child;
    f1 = prev->next;
    while (f1) {
      /* first do forward subsumption of part already processed */
      f2 = c->first_child;
      while (f2 != f1 && ! gen_subsume_prop(f2, f1))
	f2 = f2->next;;
      if (f2 != f1) {  /* delete f1 */
	prev->next = f1->next;
	zap_formula(f1);
	f1 = prev;
      }
      else {
	/* back subsumption on part already processed */
	/* delete all previous that are subsumed by f1 */
	f2 = c->first_child;
	prev = NULL;
	while (f2 != f1) {
	  if (gen_subsume_prop(f1, f2)) {
	    if (prev == NULL)
	      c->first_child = f2->next;
	    else
	      prev->next = f2->next;
	    f3 = f2;
	    f2 = f2->next;
	    zap_formula(f3);
	  }
	  else {
	    prev = f2;
	    f2 = f2->next;
	  }
	}
      }
      prev = f1;
      f1 = f1->next;
    }
    /* If just one child left, replace input formula with child. */
    if (c->first_child->next == NULL) {
      f1 = c->first_child;
      f1->next = c->next;
      free_formula(c);
      return(f1);
    }
    else
      return(c);
  }
}  /* subsume_conj */

/*************
 *
 *    struct formula *subsume_disj(c)
 *
 *    Given a disjunction, discard stronger disjuncts.
 *    The result is equivalent.  This the dual of
 *    normal clause subsumption.
 *
 *************/

struct formula *subsume_disj(struct formula *c)
{
  struct formula *f1, *f2, *f3, *prev;

  if (c->type != OR_FORM  || c->first_child == NULL)
    return(c);
  else {
    /* start with second child */
    prev = c->first_child;
    f1 = prev->next;
    while (f1) {
      /* delete f1 if it subsumes anything previous */
      f2 = c->first_child;
      while (f2 != f1 && ! gen_subsume_prop(f1, f2))
	f2 = f2->next;;
      if (f2 != f1) {  /* delete f1 */
	prev->next = f1->next;
	zap_formula(f1);
	f1 = prev;
      }
      else {
	/* delete all previous that subsume f1 */
	f2 = c->first_child;
	prev = NULL;
	while (f2 != f1) {
	  if (gen_subsume_prop(f2, f1)) {
	    if (prev == NULL)
	      c->first_child = f2->next;
	    else
	      prev->next = f2->next;
	    f3 = f2;
	    f2 = f2->next;
	    zap_formula(f3);
	  }
	  else {
	    prev = f2;
	    f2 = f2->next;
	  }
	}
      }
      prev = f1;
      f1 = f1->next;
    }
    /* If just one child left, replace input formula with child. */
    if (c->first_child->next == NULL) {
      f1 = c->first_child;
      f1->next = c->next;
      free_formula(c);
      return(f1);
    }
    else
      return(c);
  }
}  /* subsume_disj */

/*************
 *
 *    int formula_ident(f1, f2)
 *
 *    Do not permute ANDs, ORs, or like quantifiers.
 *
 *************/

int formula_ident(struct formula *f,
		  struct formula *g)
{
  struct formula *f1, *g1;

  if (f->type != g->type)
    return(0);
  else if (f->type == ATOM_FORM)
    return(term_ident(f->t, g->t));
  else if (f->type == QUANT_FORM) {
    if (f->quant_type != g->quant_type || ! term_ident(f->t, g->t))
      return(0);
    else
      return(formula_ident(f->first_child, g->first_child));
  }
  else {  /* AND_FORM || OR_FORM || IFF_FORM || IMP_FORM || NOT_FORM */
    for (f1 = f->first_child, g1 = g->first_child; f1 && g1;
	 f1 = f1->next, g1 = g1->next)
      if (! formula_ident(f1, g1))
	return(0);
    return(f1 == NULL && g1 == NULL);
  }
}  /* formula_ident */

/*************
 *
 *    conflict_tautology(f)
 *
 *    If f is an AND_FORM, reduce to empty disjunction (FALSE)
 *    if conflicting conjuncts occur.
 *    If f is an OR_FORM,  reduce to empty conjunction (TRUE)
 *    if conflicting disjuncts occur.
 *
 *************/

void conflict_tautology(struct formula *f)
{
  struct formula *f1, *f2, *a1, *a2;
  int f1_sign, f2_sign;

  /* note possible return from inner loop */

  if (f->type == AND_FORM || f->type == OR_FORM) {
    for (f1 = f->first_child; f1; f1 = f1->next) {
      f1_sign = (f1->type != NOT_FORM);
      a1 = (f1_sign ? f1 : f1->first_child);
      for (f2 = f1->next; f2; f2 = f2->next) {
	f2_sign = (f2->type != NOT_FORM);
	if (f1_sign != f2_sign) {
	  a2 = (f2_sign ? f2 : f2->first_child);
	  if (formula_ident(a1, a2)) {
	    f1 = f->first_child;
	    while (f1) {
	      f2 = f1;
	      f1 = f1->next;
	      zap_formula(f2);
	    }
	    f->first_child = NULL;
	    /* switch types */
	    f->type = (f->type == AND_FORM ? OR_FORM : AND_FORM);
	    return;
	  }
	}
      }
    }
  }
}  /* conflict_tautology */

/*************
 *
 *   void ts_and_fs(f)
 *
 *   Simplify if f is AND or OR, and an immediate subformula is
 *   TRUE (empty AND) or FALSE (empty OR).
 *
 *************/

void ts_and_fs(struct formula *f)
{
  struct formula *f1, *f2, *f_prev;
  int f_type;

  f_type = f->type;
  if (f_type != AND_FORM && f_type != OR_FORM)
    return;
  else {
    f_prev = NULL;
    f1 = f->first_child;
    while (f1 != NULL) {
      if ((f1->type == AND_FORM || f1->type == OR_FORM) &&
	  f1->first_child == NULL) {
	if (f_type != f1->type) {
	  f->type = f1->type;
	  f1 = f->first_child;
	  while (f1) {
	    f2 = f1;
	    f1 = f1->next;
	    zap_formula(f2);
	  }
	  f->first_child = NULL;
	  /* switch types */
	  f->type = (f->type == AND_FORM ? OR_FORM : AND_FORM);
	  return;
	}
	else {
	  if (f_prev == NULL)
	    f->first_child = f1->next;
	  else
	    f_prev->next = f1->next;

	  f2 = f1;
	  f1 = f1->next;
	  free_formula(f2);
	}
      }
      else {
	f_prev = f1;
	f1 = f1->next;
      }
    }
  }
}  /* ts_and_fs */

/*************
 *
 *     static int set_vars_term_2(term, sn)
 *
 *     Called from set_vars_cl_2.
 *
 *************/

static int set_vars_term_2(struct term *t,
			   int *sn)
{
  struct rel *r;
  int i, rc;

  if (t->type == COMPLEX) {
    r = t->farg;
    rc = 1;
    while (rc && r != NULL) {
      rc = set_vars_term_2(r->argval, sn);
      r = r->narg;
    }
    return(rc);
  }
  else if (t->type == NAME)
    return(1);
  else {
    i = 0;
    while (i < MAX_VARS && sn[i] != -1 && sn[i] != t->sym_num)
      i++;
    if (i == MAX_VARS)
      return(0);
    else {
      if (sn[i] == -1)
	sn[i] = t->sym_num;
      t->varnum = i;
      /*  include following to destroy input variable names
	  t->sym_num = 0;
      */
      return(1);
    }
  }
}  /* set_vars_term_2 */

/*************
 *
 *    static int set_vars_cl_2(cl) -- give variables var_nums
 *
 *    This is different from set_vars_cl bacause variables have
 *    already been identified:  type==VARIABLE.  Identical
 *    variables have same sym_num.
 *
 *************/

static int set_vars_cl_2(struct clause *cl)
{
  struct literal *lit;
  int sn[MAX_VARS];
  int i;

  for (i=0; i<MAX_VARS; i++)
    sn[i] = -1;
  lit = cl->first_lit;
  while (lit != NULL) {
    if (set_vars_term_2(lit->atom, sn))
      lit = lit->next_lit;
    else
      return(0);
  }
  return(1);
}  /* set_vars_cl_2 */

/*************
 *
 *    static struct clause *disj_to_clause(f)
 *
 *************/

static struct clause *disj_to_clause(struct formula *f)
{
  struct formula *f1, *f2;
  struct clause *c;
  struct literal *lit, *prev;

  c = get_clause();
  if (f->type == ATOM_FORM || f->type == NOT_FORM) {
    lit = get_literal();
    lit->sign = (f->type == ATOM_FORM);
    lit->atom = (f->type == ATOM_FORM ? f->t : f->first_child->t);
    if (f->type == NOT_FORM)
      free_formula(f->first_child);
    free_formula(f);
    lit->atom->occ.lit = lit;
    lit->container = c;
    mark_literal(lit);  /* atoms have varnum > 0 */
    c->first_lit = lit;
  }
  else {  /* OR_FORM */
    prev = NULL;
    f1 = f->first_child;
    while (f1) {
      f2 = f1;
      f1 = f1->next;
	
      lit = get_literal();
      lit->sign = (f2->type == ATOM_FORM);
      lit->atom = (f2->type == ATOM_FORM ? f2->t : f2->first_child->t);
      if (f2->type == NOT_FORM)
	free_formula(f2->first_child);
      free_formula(f2);
      lit->atom->occ.lit = lit;
      lit->container = c;
      mark_literal(lit);  /* atoms have varnum > 0 */
	
      if (prev == NULL)
	c->first_lit = lit;
      else
	prev->next_lit = lit;
      prev = lit;
    }
    free_formula(f);
  }

  if (set_vars_cl_2(c) == 0) {
    char s[500];
    print_clause(stdout, c);
    sprintf(s, "disj_to_clause, too many variables in clause, max is %d.", MAX_VARS);
    abend(s);
  }
  cl_merge(c);  /* merge identical literals */
  return(c);
}  /* disj_to_clause */

/*************
 *
 *    static struct list *cnf_to_list(f)
 *
 *    Convert a CNF formula to a list of clauses.
 *    This includes assigning variable numbers to the varnum fileds of VARIABLES.
 *    An ABEND occurs if a clause has too many variables.
 *
 *************/

static struct list *cnf_to_list(struct formula *f)
{
  struct formula *f1, *f2;
  struct list *l;
  struct clause *c;

  l = get_list();
  if (f->type != AND_FORM) {
    c = disj_to_clause(f);
    append_cl(l, c);
  }
  else {  /* OR_FORM || ATOM_FORM || NOT_FORM */
    f1 = f->first_child;
    while (f1) {
      f2 = f1;
      f1 = f1->next;
      c = disj_to_clause(f2);  /* zaps f2 */
      append_cl(l, c);
    }
    free_formula(f);
  }
  return(l);
}  /* cnf_to_list */

/*************
 *
 *    struct list *clausify(f) -- Skolem/CNF tranformation.
 *
 *    Convert a quantified formula to a list of clauses.
 *
 *************/

struct list *clausify(struct formula *f)
{
  struct list *l;

  f = nnf(f);
  f = skolemize(f);
  unique_all(f);
  f = zap_quant(f);
  rename_syms_formula(f, f);
  f = cnf(f);
  l = cnf_to_list(f);
  return(l);

}  /* clausify */

/*************
 *
 *    struct list *clausify_formula_list(fp)
 *
 *    Clausify a set of formulas, and return a list of clauses.
 *    The set of formulas is deallocated.
 *
 *************/

struct list *clausify_formula_list(struct formula_ptr *fp)
{
  struct list *l, *l1;
  struct formula_ptr *fp1, *fp2;

  l = get_list();
  fp1 = fp;
  while (fp1 != NULL) {
    if (Flags[FORMULA_HISTORY].val) {
      int f_id;
      struct clause *c = get_clause();
      struct literal *lit = get_literal();
      struct ilist *ip1 = get_ilist();
      struct ilist *ip2 = get_ilist();
      c->first_lit = lit;
      lit->sign = 1;
      lit->atom = formula_to_term(fp1->f);
      assign_cl_id(c);
      f_id = c->id;
      hide_clause(c);

      l1 = clausify(fp1->f);

      for (c = l1->first_cl; c; c = c->next_cl) {
	ip1 = get_ilist();
	ip2 = get_ilist();
	c->parents = ip1;
	ip1->next = ip2;
	ip1->i = CLAUSIFY_RULE;
	ip2->i = f_id;
      }
    }
    else
      l1 = clausify(fp1->f);

    append_lists(l, l1);
    fp2 = fp1;
    fp1 = fp1->next;
    free_formula_ptr(fp2);
  }
  return(l);
}  /* clausify_formula_list */

/*************
 *
 *    struct formula *negation_inward(f)
 *
 *    If f is a negated conjunction, disjunction, or quantified formula,
 *    move the negation sign in one level.
 *
 *************/

struct formula *negation_inward(struct formula *f)
{
  struct formula *f1, *f2, *prev, *f_save;

  if (f->type == NOT_FORM) {
    f1 = f->first_child;
    if (f1->type == AND_FORM || f1->type == OR_FORM || f1->type == QUANT_FORM) {
      f_save = f->next;
      f = negate_formula(f);
      f->next = f_save;
	
      if (f->type == AND_FORM || f->type == OR_FORM) {
	/* apply DeMorgan's laws */
	f->type = (f->type == AND_FORM ? OR_FORM : AND_FORM);
	f1 = f->first_child;
	prev = NULL;
	while (f1) {
	  f2 = f1;
	  f1 = f1->next;
	  f2 = negate_formula(f2);
	  if (prev)
	    prev->next = f2;
	  else
	    f->first_child = f2;
	  prev = f2;
	}
      }
      else {  /* QUANT_FORM */
	f->quant_type = (f->quant_type==ALL_QUANT ? EXISTS_QUANT : ALL_QUANT);
	f->first_child = negate_formula(f->first_child);
      }
	
    }
  }
  return(f);
}  /* negation_inward */

/*************
 *
 *    struct formula *expand_imp(f)
 *
 *    Change (P -> Q) to (-P | Q).
 *
 *************/

struct formula *expand_imp(struct formula *f)
{
  if (f->type != IMP_FORM)
    return(f);
  else {
    f->type = OR_FORM;
    f->first_child = negate_formula(f->first_child);
    return(f);
  }
}  /* expand_imp */

/*************
 *
 *    struct formula *iff_to_conj(f)
 *
 *    Change (P <-> Q) to ((P -> Q) & (Q -> P)).
 *
 *************/

struct formula *iff_to_conj(struct formula *f)
{
  struct formula *f1, *f2, *f_save;

  if (f->type != IFF_FORM)
    return(f);
  else {
    f_save = f->next;

    f1 = copy_formula(f);
    f->type = f1->type = IMP_FORM;

    /* flip args of f1 */
	
    f2 = f1->first_child;
    f1->first_child = f2->next;
    f2->next = NULL;
    f1->first_child->next = f2;

    f->next = f1;
    f1->next = NULL;		

    /* build conjunction */
    f2 = get_formula();
    f2->type = AND_FORM;
    f2->first_child = f;

    f2->next = f_save;
    return(f2);
  }
}  /* iff_to_conj */

/*************
 *
 *    struct formula *iff_to_disj(f)
 *
 *    Change (P <-> Q) to ((P & Q) | (-Q & -P)).
 *
 *************/

struct formula *iff_to_disj(struct formula *f)
{
  struct formula *f1, *f2, *f_save;

  if (f->type != IFF_FORM)
    return(f);
  else {
    f_save = f->next;

    f1 = copy_formula(f);
    f->type = f1->type = AND_FORM;
    f1->first_child->next = negate_formula(f1->first_child->next);
    f1->first_child = negate_formula(f1->first_child);

    f->next = f1;
    f1->next = NULL;		

    /* build disjunction */
    f2 = get_formula();
    f2->type = OR_FORM;
    f2->first_child = f;

    f2->next = f_save;
    return(f2);
  }
}  /* iff_to_disj */

/*************
 *
 *    struct formula *nnf_cnf(f)
 *
 *************/

struct formula *nnf_cnf(struct formula *f)
{
  return(cnf(nnf(f)));
}  /* nnf_cnf */

/*************
 *
 *    struct formula *nnf_dnf(f)
 *
 *************/

struct formula *nnf_dnf(struct formula *f)
{
  return(dnf(nnf(f)));
}  /* nnf_dnf */

/*************
 *
 *    struct formula *nnf_skolemize(f)
 *
 *************/

struct formula *nnf_skolemize(struct formula *f)
{
  return(skolemize(nnf(f)));
}  /* nnf_skolemize */

/*************
 *
 *    struct formula *clausify_formed(f)
 *
 *************/

struct formula *clausify_formed(struct formula *f)
{
  f = nnf(f);
  f = skolemize(f);
  unique_all(f);
  f = zap_quant(f);
  rename_syms_formula(f, f);
  f = cnf(f);
  return(f);
}  /* clausify_formed */

/*************
 *
 *    rms_conflict_tautology(f)
 *
 *    If f is an AND_FORM, reduce to empty disjunction (FALSE)
 *    if conflicting conjuncts occur.
 *    If f is an OR_FORM,  reduce to empty conjunction (TRUE)
 *    if conflicting disjuncts occur.
 *
 *************/

void rms_conflict_tautology(struct formula *f)
{
  struct formula *f1, *f2;

  /* note possible return from inner loop */

  if (f->type == AND_FORM) {
    for (f1 = f->first_child; f1; f1 = f1->next) {
      for (f2 = f1->next; f2; f2 = f2->next) {
	if (gen_conflict(f1, f2)) {
	  f1 = f->first_child;
	  while (f1) {
	    f2 = f1;
	    f1 = f1->next;
	    zap_formula(f2);
	  }
	  f->first_child = NULL;
	  /* switch types */
	  f->type = OR_FORM;
	  return;
	}
      }
    }
  }

  else if (f->type == OR_FORM) {
    for (f1 = f->first_child; f1; f1 = f1->next) {
      for (f2 = f1->next; f2; f2 = f2->next) {
	if (gen_tautology(f1, f2)) {
	  f1 = f->first_child;
	  while (f1) {
	    f2 = f1;
	    f1 = f1->next;
	    zap_formula(f2);
	  }
	  f->first_child = NULL;
	  /* switch types */
	  f->type = AND_FORM;
	  return;
	}
      }
    }
  }
}  /* rms_conflict_tautology */

/*************
 *
 *    struct formula *rms_subsume_conj(c)
 *
 *    Given a conjunction, discard weaker conjuncts.
 *    This is like deleting subsumed clauses.
 *    The result is equivalent.
 *
 *************/

struct formula *rms_subsume_conj(struct formula *c)
{
  struct formula *f1, *f2, *f3, *prev;

  if (c->type != AND_FORM  || c->first_child == NULL)
    return(c);
  else {
    /* start with second child */
    prev = c->first_child;
    f1 = prev->next;
    while (f1) {
      /* first do forward subsumption of part already processed */
      f2 = c->first_child;
      while (f2 != f1 && ! gen_subsume(f2, f1))
	f2 = f2->next;;
      if (f2 != f1) {  /* delete f1 */
	prev->next = f1->next;
	zap_formula(f1);
	f1 = prev;
      }
      else {
	/* back subsumption on part already processed */
	/* delete all previous that are subsumed by f1 */
	f2 = c->first_child;
	prev = NULL;
	while (f2 != f1) {
	  if (gen_subsume(f1, f2)) {
	    if (prev == NULL)
	      c->first_child = f2->next;
	    else
	      prev->next = f2->next;
	    f3 = f2;
	    f2 = f2->next;
	    zap_formula(f3);
	  }
	  else {
	    prev = f2;
	    f2 = f2->next;
	  }
	}
      }
      prev = f1;
      f1 = f1->next;
    }
    /* If just one child left, replace input formula with child. */
    if (c->first_child->next == NULL) {
      f1 = c->first_child;
      f1->next = c->next;
      free_formula(c);
      return(f1);
    }
    else
      return(c);
  }
}  /* rms_subsume_conj */

/*************
 *
 *    struct formula *rms_subsume_disj(c)
 *
 *    Given a disjunction, discard stronger disjuncts.
 *    The result is equivalent.  This the dual of
 *    normal clause subsumption.
 *
 *************/

struct formula *rms_subsume_disj(struct formula *c)
{
  struct formula *f1, *f2, *f3, *prev;

  if (c->type != OR_FORM  || c->first_child == NULL)
    return(c);
  else {
    /* start with second child */
    prev = c->first_child;
    f1 = prev->next;
    while (f1) {
      /* delete f1 if it subsumes anything previous */
      f2 = c->first_child;
      while (f2 != f1 && ! gen_subsume(f1, f2))
	f2 = f2->next;;
      if (f2 != f1) {  /* delete f1 */
	prev->next = f1->next;
	zap_formula(f1);
	f1 = prev;
      }
      else {
	/* delete all previous that subsume f1 */
	f2 = c->first_child;
	prev = NULL;
	while (f2 != f1) {
	  if (gen_subsume(f2, f1)) {
	    if (prev == NULL)
	      c->first_child = f2->next;
	    else
	      prev->next = f2->next;
	    f3 = f2;
	    f2 = f2->next;
	    zap_formula(f3);
	  }
	  else {
	    prev = f2;
	    f2 = f2->next;
	  }
	}
      }
      prev = f1;
      f1 = f1->next;
    }
    /* If just one child left, replace input formula with child. */
    if (c->first_child->next == NULL) {
      f1 = c->first_child;
      f1->next = c->next;
      free_formula(c);
      return(f1);
    }
    else
      return(c);
  }
}  /* rms_subsume_disj */

/*************
 *
 *    int free_occurrence(v, f)
 *
 *    Does v have a free occurrence in f?
 *
 *************/

int free_occurrence(struct term *v,
		    struct formula *f)
{
  struct formula *f1;
  int free;

  switch (f->type) {
  case ATOM_FORM:
    free = occurs_in(v, f->t);
    break;
  case NOT_FORM:
  case AND_FORM:
  case OR_FORM:
  case IMP_FORM:
  case IFF_FORM:
    for (free = 0, f1 = f->first_child; f1 && ! free; f1 = f1->next)
      free = free_occurrence(v, f1);
    break;
  case QUANT_FORM:
    if (term_ident(v, f->t))
      free = 0;
    else
      free = free_occurrence(v, f->first_child);
    break;
  default: free = 0;
  }
  return(free);

}  /* free_occurrence */

/*************
 *
 *    struct formula *rms_distribute_quants(f)
 *
 *    f is universally quantified formula.
 *    Child is conjunction in RMS.
 *    Distribute quantifier to conjuncts.
 *    Return a RMS of f.
 *
 *************/

struct formula *rms_distribute_quants(struct formula *f_quant)
{
  struct formula *f_conj, *f1, *f2, *f3;

  f_conj = f_quant->first_child;
  f3 = NULL;
  f1 = f_conj->first_child;
  while (f1) {
    f2 = get_formula();
    f2->type = QUANT_FORM;
    f2->quant_type = ALL_QUANT;
    f2->first_child = f1;
    f2->t = copy_term(f_quant->t);
    f1 = f1->next;
    f2->first_child->next = NULL;
    f2 = rms_quantifiers(f2);  /* indirect recursive call */
    if (f3)
      f3->next = f2;
    else
      f_conj->first_child = f2;
    f3 = f2;
  }

  zap_term(f_quant->t);
  free_formula(f_quant);

  flatten_top(f_conj);
  rms_conflict_tautology(f_conj);
  f_conj = rms_subsume_conj(f_conj);
  return(f_conj);

}  /* rms_distribute_quants */

/*************
 *
 *     void separate_free(v, f, free, not_free)
 *
 *************/

static void separate_free(struct term *v,
			  struct formula *f,
			  struct formula **p_free,
			  struct formula **p_not_free)
{
  struct formula *f1, *not_free, *f2, *f3, *prev;

  not_free = f2 = f3 = prev = NULL;
  f1 = f->first_child;
  while (f1) {
    f2 = f1;
    f1 = f1->next;

    if (!free_occurrence(v, f2)) {
      f2->next = NULL;
      if (not_free)
	f3->next = f2;
      else
	not_free = f2;
      f3 = f2;

      if (prev == NULL)
	f->first_child = f1;
      else
	prev->next = f1;
    }
    else
      prev = f2;
  }

  if (f->first_child == NULL) {
    *p_free = NULL;
    free_formula(f);
  }
  else if (f->first_child->next == NULL) {
    *p_free = f->first_child;
    free_formula(f);
  }
  else
    *p_free = f;

  if (not_free == NULL)
    *p_not_free = NULL;
  else if (not_free->next == NULL)
    *p_not_free = not_free;
  else {
    f1 = get_formula();
    f1->type = OR_FORM;
    f1->first_child = not_free;
    *p_not_free = f1;
  }
}  /* separate_free */

/*************
 *
 *    struct formula *rms_push_free(f)
 *
 *    f is universally quantifierd formula.
 *    The child of f is a (simple) disjunction in RMS.
 *    Reduce scopes based on free variables.
 *    Result is in RMS, either a quantified formula or a disjunction.
 *
 *************/

struct formula *rms_push_free(struct formula *f)
{
  struct formula *f2, *free, *not_free;

  separate_free(f->t, f->first_child, &free, &not_free);

  if (!free) {  /* var doesn't occur free in any subformula. */
    abend("rms_push_free has extra quantifier.");
  }

  if (not_free) {

    f->first_child = free;
    f = rms_quantifiers(f);
    f->next = NULL;
    if (not_free->type == OR_FORM) {
      /* Install f as last disjunct. */
      for (f2 = not_free->first_child; f2->next; f2 = f2->next);
      f2->next = f;
      f2 = not_free;
    }
    else {
      f2 = get_formula();
      f2->type = OR_FORM;
      f2->first_child = not_free;
      not_free->next = f;
    }
    /* f2 is disjunction */
    rms_conflict_tautology(f2);
    f2 = rms_subsume_disj(f2);
    return(f2);
  }
  else
    return(f);

}  /* rms_push_free */

/*************
 *
 *    struct formula *rms_quantifiers(f)
 *
 *    f is a quantified formula whose child is in RMS.
 *    This function returns a RMS of f.
 *
 *************/

struct formula *rms_quantifiers(struct formula *f)
{
  struct formula *f1, *f2, *f_save;
  int negate_flag;

  f_save = f->next;
  f->next = NULL;

  if (!free_occurrence(f->t, f->first_child)) {
    f1 = f->first_child;
    zap_term(f->t);
    free_formula(f);
    f1->next = f_save;
    return(f1);
  }

  if (f->quant_type == EXISTS_QUANT) {
    f = nnf(negate_formula(f));
    negate_flag = 1;
    /* If f is an OR with and AND child, call rms to make conjunction. */
    if (f->first_child->type == OR_FORM) {
      for(f1 = f->first_child->first_child;
	  f1 && f1->type != AND_FORM;
	  f1 = f1->next);
      if (f1)
	f->first_child = rms(f->first_child);
    }
  }
  else
    negate_flag = 0;

  /* Now, "all" is the quantifier, and child is RMS. */

  if (f->first_child->type == AND_FORM)
    f = rms_distribute_quants(f);
  else if (f->first_child->type == OR_FORM)
    f = rms_push_free(f);

  /* else atomic or negated atomic, so do nothing */

  /* f is now not necessarily QUANT_FORM. */

  if (negate_flag) {
    f = nnf(negate_formula(f));
    if (f->type == QUANT_FORM)
      f2 = f->first_child;
    else
      f2 = f;
    /* If f2 is an OR with and AND child, call rms to make conjunction. */
    if (f2->type == OR_FORM) {
      for(f1 = f2->first_child;
	  f1 && f1->type != AND_FORM;
	  f1 = f1->next);
      if (f1) {
	if (f == f2)
	  f = rms(f2);
	else
	  f->first_child = rms(f2);
      }
    }
  }

  f->next= f_save;
  return(f);

}  /* rms_quantifiers */

/*************
 *
 *    static struct formula *rms_distribute(f) -- rms_distribute OR over AND.
 *
 *    f is an OR node whose subterms are in Reduced MiniScope (RMS).
 *    This routine returns a RMS of f.
 *
 *************/

static struct formula *rms_distribute(struct formula *f)
{
  struct formula *f_new, *f1, *f2, *f3, *f4, *f_prev, *f_save;
  int i, j;

  f_save = f->next; f->next = NULL;

  if (f->type != OR_FORM)
    return(f);
  else {

    flatten_top(f);
    rms_conflict_tautology(f);
    f = rms_subsume_disj(f);
    if (f->type != OR_FORM)
      return(f);
    else {
	
      /* find first AND subformula */
      i = 1;
      f_prev = NULL;
      for (f1 = f->first_child; f1 && f1->type != AND_FORM; f1 = f1->next) {
	i++;
	f_prev = f1;
      }
      if (f1 == NULL)
	return(f);  /* nothing to rms_distribute */
      else {
	/* unhook AND */
	if (f_prev)
	  f_prev->next = f1->next;
	else
	  f->first_child = f1->next;
	f2 = f1->first_child;
	f_new = f1;
	f_prev = NULL;
	while (f2) {
	  f3 = f2->next;
	  if (f3)
	    f1 = copy_formula(f);
	  else
	    f1 = f;
	  if (i == 1) {
	    f2->next = f1->first_child;
	    f1->first_child = f2;
	  }
	  else {
	    j = 1;
	    for (f4 = f1->first_child; j < i-1; f4 = f4->next)
	      j++;
	    f2->next = f4->next;
	    f4->next = f2;
	  }
	  f1 = rms_distribute(f1);
	  if (f_prev)
	    f_prev->next = f1;
	  else
	    f_new->first_child = f1;
	  f_prev = f1;
	  f2 = f3;
	}
	f_new->next = f_save;
	flatten_top(f_new);
	rms_conflict_tautology(f_new);
	f_new = rms_subsume_conj(f_new);
	return(f_new);
      }
    }
  }
}  /* rms_distribute */

/*************
 *
 *    struct formula *rms(f) -- convert f to Reduced MiniScope (RMS)
 *
 *************/

struct formula *rms(struct formula *f)
{
  struct formula *f1, *f2, *f_prev, *f_next, *f_save;

  f_save = f->next; f->next = NULL;

  if (f->type == AND_FORM || f->type == OR_FORM) {
    /* first convert subterms to RMS */
    f_prev = NULL;
    f1 = f->first_child;
    while(f1) {
      f_next = f1->next;
      f2 = rms(f1);
      if (f_prev)
	f_prev->next = f2;
      else
	f->first_child = f2;
      f_prev = f2;
      f1 = f_next;
    }

    if (f->type == AND_FORM) {
      flatten_top(f);
      rms_conflict_tautology(f);
      f = rms_subsume_conj(f);
    }
    else
      f = rms_distribute(f);  /* flatten and simplify in distribute */
  }

  else if (f->type == QUANT_FORM) {
    f->first_child = rms(f->first_child);
    f = rms_quantifiers(f);
  }

  /* else f is atomic or negated atomic, so do nothing; */

  f->next = f_save;
  return(f);

}  /* rms */

/*************
 *
 *    static void introduce_var_term(t, v, vnum)
 *
 *************/

static void introduce_var_term(struct term *t,
			       struct term *v,
			       int vnum)
{
  struct rel *r;

  switch (t->type) {
  case NAME:
    if (term_ident(t,v)) {
      t->type = VARIABLE;
      t->varnum = vnum;
      t->sym_num = 0;
    }
    break;
  case VARIABLE:
    break;
  case COMPLEX:
    for (r = t->farg; r; r = r->narg)
      introduce_var_term(r->argval, v, vnum);
    break;
  }

}  /* introduce_var_term */

/*************
 *
 *    static void introduce_var(f, t, vnum)
 *
 *    In formula f, replace all free occurrences of t with a variable
 *    (set type to VARIABLE) with number vnum.
 *
 *************/

static void introduce_var(struct formula *f,
			  struct term *t,
			  int vnum)
{
  struct formula *f1;

  switch (f->type) {
  case ATOM_FORM:
    introduce_var_term(f->t, t, vnum);
    break;
  case AND_FORM:
  case OR_FORM:
  case NOT_FORM:
    for (f1 = f->first_child; f1; f1 = f1->next)
      introduce_var(f1, t, vnum);
    break;
  case QUANT_FORM:
    if (!term_ident(t, f->t))
      introduce_var(f->first_child, t, vnum);
    break;
  default:
    abend("introduce_var, bad formula.");
  }

}  /* introduce_var */

/*************
 *
 *    struct formula *renumber_unique(f, vnum)
 *
 *    f is NNF, and all quantifiers are unique.
 *    This function renumbers variables, starting with *vnum_p and
 *    removes quantifiers.
 *
 *************/

struct formula *renumber_unique(struct formula *f,
				int *vnum_p)
{
  struct formula *f1, *f2, *f_prev, *f_next;

  switch (f->type) {
  case ATOM_FORM:
    return(f);
  case AND_FORM:
  case OR_FORM:
  case NOT_FORM:
    f_prev = NULL;
    f1 = f->first_child;
    while(f1) {
      f_next = f1->next;
      f2 = renumber_unique(f1, vnum_p);
      if (f_prev)
	f_prev->next = f2;
      else
	f->first_child = f2;
      f_prev = f2;
      f1 = f_next;
    }
    return(f);
  case QUANT_FORM:
    f1 = f->first_child;
    introduce_var(f1, f->t, *vnum_p);
    (*vnum_p)++;
    if (*vnum_p == MAX_VARS) {
      abend("renumber_unique, too many vars.");
    }
    f1->next = f->next;
    f->first_child = NULL;
    zap_formula(f);
    return(renumber_unique(f1, vnum_p));
  }

  abend("renumber_unique, bad formula.");
  return(f);  /* to quiet lint */
}  /* renumber_unique */

/*************
 *
 *    int gen_subsume_rec(c, cs, d, ds, tr_p) -- does c gen_subsume_rec d?
 *
 *    This is generalized subsumption on quantified formulas.  It is
 *    not as complete as the Prolog version, because there is no
 *    backtracking to try alternatives in cases 3 and 4 below.
 *
 *************/

int gen_subsume_rec(struct formula *c,
		    struct context *cs,
		    struct formula *d,
		    struct context *ds,
		    struct trail **tr_p)
{
  struct formula *f;

  /* The order of these tests is important.  For example, if */
  /* the last test is moved to the front, c=(p|q) will not   */
  /* subsume d=(p|q|r).                                      */

  if (c->type == OR_FORM) {  /* return(each c_i subsumes d) */
    for (f = c->first_child; f && gen_subsume_rec(f, cs, d, ds, tr_p); f = f->next);
    return(f == NULL);
  }
  else if (d->type == AND_FORM) {  /* return(c subsumes each d_i) */
    for (f = d->first_child; f && gen_subsume_rec(c, cs, f, ds, tr_p); f = f->next);
    return(f == NULL);
  }
  else if (c->type == AND_FORM) {  /* return(one c_i subsumes d) */
    for (f = c->first_child; f && ! gen_subsume_rec(f, cs, d, ds, tr_p); f = f->next);
    return(f != NULL);
  }
  else if (d->type == OR_FORM) {  /* return(c subsumes one d_i) */
    for (f = d->first_child; f && ! gen_subsume_rec(c, cs, f, ds, tr_p); f = f->next);
    return(f != NULL);
  }
  else if (c->type != d->type)
    return(0);
  else if (c->type == NOT_FORM)
    return(unify(c->first_child->t, cs, d->first_child->t, ds, tr_p));
  else  /* both ATOMs */
    return(unify(c->t, cs, d->t, ds, tr_p));

}  /* gen_subsume_rec */

/*************
 *
 *    int gen_subsume(c, d) -- generalized subsumption on RMS formulas.
 *
 *    If 1 is returned, (c -> d) holds.
 *
 *************/

int gen_subsume(struct formula *c,
		struct formula *d)
{
  struct formula *c1, *d1;
  int result, i;
  struct context *cs, *ds;
  struct trail *tr;

  Sk_const_num = Sk_func_num = 0;
  i = 6;
  c1 = renumber_unique(skolemize(copy_formula(c)),&i);
  i = 6;
  d1 = renumber_unique(anti_skolemize(copy_formula(d)),&i);

  cs = get_context();
  ds = get_context();
  tr = NULL;

  result = gen_subsume_rec(c1, cs, d1, ds, &tr);
  clear_subst_1(tr);
  free_context(cs);
  free_context(ds);
  zap_formula(c1);
  zap_formula(d1);
  return(result);
}  /* gen_subsume */

/*************
 *
 *    int gen_conflict(c, d)
 *
 *    Try to show (c & d) inconsistent by showing (c -> -d).
 *
 *    If 1 is returned, (c & d) is inconsistent.
 *
 *************/

int gen_conflict(struct formula *c,
		 struct formula *d)
{
  struct formula *c1, *d1;
  int result, i;
  struct context *cs, *ds;
  struct trail *tr;

  Sk_const_num = Sk_func_num = 0;
  i = 6;
  c1 = renumber_unique(skolemize(copy_formula(c)),&i);
  i = 6;
  /* can skip nnf of negate_formula, because anti-skolemize re-negates */
  d1 = renumber_unique(anti_skolemize(negate_formula(copy_formula(d))),&i);

  cs = get_context();
  ds = get_context();
  tr = NULL;

  result = gen_subsume_rec(c1, cs, d1, ds, &tr);
  clear_subst_1(tr);
  free_context(cs);
  free_context(ds);
  zap_formula(c1);
  zap_formula(d1);
  return(result);
}  /* gen_conflict */

/*************
 *
 *    int gen_tautology(c, d)
 *
 *    Try to show (c | d) a tautology by showing (-c -> d).
 *
 *    If 1 is returned, (c | d) is a tautology.
 *
 *************/

int gen_tautology(struct formula *c,
		  struct formula *d)
{
  struct formula *c1, *d1;
  int result, i;
  struct context *cs, *ds;
  struct trail *tr;

  Sk_const_num = Sk_func_num = 0;
  i = 6;
  c1 = renumber_unique(skolemize(nnf(negate_formula(copy_formula(c)))),&i);
  i = 6;
  d1 = renumber_unique(anti_skolemize(copy_formula(d)),&i);

  cs = get_context();
  ds = get_context();
  tr = NULL;

  result = gen_subsume_rec(c1, cs, d1, ds, &tr);
  clear_subst_1(tr);
  free_context(cs);
  free_context(ds);
  zap_formula(c1);
  zap_formula(d1);
  return(result);
}  /* gen_tautology */

/*************
 *
 *    struct formula *rms_cnf(f)
 *
 *************/

struct formula *rms_cnf(struct formula *f)
{
  return(rms(nnf(f)));
}  /* rms_cnf */

/*************
 *
 *    struct formula *rms_dnf(f)
 *
 *************/

struct formula *rms_dnf(struct formula *f)
{
  return(nnf(negate_formula(rms(nnf(negate_formula(f))))));
}  /* rms_dnf */

/*************
 *
 *    struct formula *push_free(f)
 *
 *    f is universally quantifierd formula
 *    The child of f is a disjunction.
 *    Reduce scopes 1 level based on free variables.
 *    Result is either a quantified formula or a disjunction.
 *
 *************/

static struct formula *push_free(struct formula *f)
{
  struct formula *f2, *free, *not_free;

  separate_free(f->t, f->first_child, &free, &not_free);

  if (!free) {  /* var doesn't occur free in any subformula. */
    not_free->next = f->next;
    free_term(f->t);
    free_formula(f);
    return(not_free);
  }

  else if (!not_free)  /* var occurs free in all subformulas */
    return(f);

  else {  /* at least one of each */	

    f->first_child = free;
    f->next = NULL;
    if (not_free->type == OR_FORM) {
      /* Install f as last disjunct. */
      for (f2 = not_free->first_child; f2->next; f2 = f2->next);
      f2->next = f;
      f2 = not_free;
    }
    else {
      f2 = get_formula();
      f2->type = OR_FORM;
      f2->first_child = not_free;
      not_free->next = f;
    }
    /* f2 is disjunction */
    return(f2);
  }

}  /* push_free */

/*************
 *
 *    struct formula *distribute_quantifier(f)
 *
 *    If f is (all x (f1 & ...)) or (exists x (f1 | ...)),
 *    distribute the quantifier to the subformulas (and delete
 *    the quantifier if the subformula has no free occurrences
 *    of the variable.
 *
 *************/

struct formula *distribute_quantifier(struct formula *f)
{
  struct formula *f1, *f2, *f3, *prev, *save_next;

  if (f->type == QUANT_FORM) {
    save_next = f->next;
    f->next = NULL;
    f1 = f->first_child;
    if ((f->quant_type == ALL_QUANT && f1->type == AND_FORM) ||
	(f->quant_type == EXISTS_QUANT && f1->type == OR_FORM)) {

      for (f2=f1->first_child, prev=NULL; f2; prev=f2, f2=f2->next) {
	if (free_occurrence(f->t, f2)) {
	  f3 = get_formula();
	  f3->type = QUANT_FORM;
	  f3->quant_type = f->quant_type;
	  f3->t = copy_term(f->t);  /* variable */
	  f3->next = f2->next;
	  f3->first_child = f2;
	  f2->next = NULL;
	  if (prev)
	    prev->next = f3;
	  else
	    f1->first_child = f3;
	  f2 = f3;
	}
      }
      free_term(f->t);
      free_formula(f);
      f = f1;
    }
    else if (f->quant_type == ALL_QUANT && f1->type == OR_FORM) {
      f = push_free(f);
    }
    else if (f->quant_type == EXISTS_QUANT && f1->type == AND_FORM) {
      f = nnf(negate_formula(f));
      f = push_free(f);
      f = nnf(negate_formula(f));
    }
		
    f->next = save_next;
  }
  return(f);
}  /* distribute_quantifier */

