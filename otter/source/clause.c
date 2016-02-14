/*
 *  clause.c -- This file has routines associated with the clause data type.
 *
 */

#include "header.h"

#define CLAUSE_TAB_SIZE 3793

/* hash table for accessing clauses by ID */
static struct clause_ptr *Clause_tab[CLAUSE_TAB_SIZE];

/* clause ID counter */
static int Clause_id_count;

/* back subsumed, demodulated put here, not deleted */
static struct clause *Hidden_clauses;

/* array to mark mapped literals during subsumption */
#define MAX_LITS 100
static char Map_array[MAX_LITS];

/*************
 *
 *   reset_clause_counter()
 *
 *************/

void reset_clause_counter(void)
{
  Clause_id_count = 0;
}  /* reset_clause_counter */

/*************
 *
 *    int next_cl_num()
 *
 *    What is the next clause number?  Do not increment the count.
 *
 *************/

int next_cl_num(void)
{
  return(Clause_id_count + 1);
}  /* next_cl_num */

/*************
 *
 *    assign_cl_id()
 *
 *    Assign a unique clause identifier and insert into the clause table.
 *
 *************/

void assign_cl_id(struct clause *c)
{
  c->id = ++Clause_id_count;
  cl_insert_tab(c);

  /* Turn debugging mode on when DEBUG_FIRST - 1 is assigned. */
  if ( (c->id == (Parms[DEBUG_FIRST].val - 1)) && !Flags[VERY_VERBOSE].val)
    {
      Flags[VERY_VERBOSE].val = 1;
      fprintf(stdout, "\n\n***** Turn Debugging Mode ON *****\n\n");
    }
    
  /* Turn debugging mode off when DEBUG_LAST + 1 is assigned. */
  if ( (c->id == (Parms[DEBUG_LAST].val + 1)) && Flags[VERY_VERBOSE].val)
    {
      Flags[VERY_VERBOSE].val = 0;
      fprintf(stdout, "\n\n***** Turn Debugging Mode OFF *****\n\n");
    }
}  /* assign_cl_id */

/*************
 *
 *   hot_cl_integrate(c)
 *
 *   Integrate a hot-list clause.  All this does is assign a clause ID.
 *   The subterms are not put into the shared data structures, because
 *   this interferes with the kludgy way I implemented hot-list inference.
 *   In particular, hot paramoudlation from the new clause can't 
 *   handle going into hot-list clauses that are integrated in the
 *   normal way.
 *
 *************/

void hot_cl_integrate(struct clause *c)
{
  struct literal *lit;

  assign_cl_id(c);

  for (lit = c->first_lit; lit; lit = lit->next_lit) {
    set_up_pointers(lit->atom);
    lit->atom->occ.lit = lit;
  }
}  /* hot_cl_integrate */

/*************
 *
 *    cl_integrate(c) -- integrate a clause
 *
 *    This routine integrates most subterms of the atoms. (Incoming clause must
 *    already have back pointers from literal to clause and atom to literal.)
 *
 *    The atoms are not shared, and arguments of positive equality literals
 *    are not shared.
 *
 *    A clause is integrated iff its ID is > 0.
 *
 *************/

void cl_integrate(struct clause *c)
{
  struct literal *lit;
  struct term *atom;
  struct rel *r, *r1;

  if (c->id != 0) {
    fprintf(stdout, "WARNING, cl_integrate gets clause with ID: ");
    print_clause(stdout, c);
  }
  else {
    if (multi_justifications()) {
      /* for proof-shortening experiment */
      possibly_append_parents(c, c->parents);
    }
    assign_cl_id(c);
    lit = c->first_lit;
    while (lit) {
      atom = lit->atom;
      if (atom->varnum == POS_EQ || atom->varnum == LEX_DEP_DEMOD ||
	  atom->varnum == CONDITIONAL_DEMOD) {
	/* do not share (condition), alpha, beta */
	r1 = atom->farg;
	while (r1) {  /* for alpha and beta */
	  if (Flags[BACK_DEMOD].val)
	    /* put it where back demod can find it */
	    bd_kludge_insert(r1->argval);
	  if (r1->argval->type == COMPLEX) {
	    r = r1->argval->farg;
	    while (r) {
	      r->argval = integrate_term(r->argval);
	      r->argof = r1->argval;
	      r->nocc = r->argval->occ.rel;
	      r->argval->occ.rel = r;
	      r = r->narg;
	    }
	  }
	  r1->argof = atom;
	  r1->argval->occ.rel = r1;
	  r1 = r1->narg;
	}
      }
      else if (atom->type == COMPLEX) {
	r = atom->farg;
	while (r) {
	  r->argval = integrate_term(r->argval);
	  r->argof = atom;
	  r->nocc = r->argval->occ.rel;
	  r->argval->occ.rel = r;
	  r = r->narg;
	}
      }
      lit = lit->next_lit;
    }
  }

}  /* cl_integrate */

/*************
 *
 *    cl_del_int(c) -- deallocate an integrated clause.
 *
 *************/

void cl_del_int(struct clause *c)
{
  struct literal *lit, *plit;
  struct rel *r, *r2, *pr, *r1;
  struct term *atom;
  struct ilist *ip1, *ip2;

  lit = c->first_lit;
  while (lit) {
    atom = lit->atom;
    if (atom->varnum == POS_EQ || atom->varnum == LEX_DEP_DEMOD ||
	atom->varnum == CONDITIONAL_DEMOD) {
      /* (condition), alpha, beta not shared */
      r1 = atom->farg;
      while(r1) {  /* for alpha and beta */
	if (Flags[BACK_DEMOD].val)
	  bd_kludge_delete(r1->argval);  /* back demod kludge */
	r = r1->argval->farg;
	while (r) {
	  r2 = r->argval->occ.rel;
	  pr = NULL;
	  while (r2 && r2 != r) {
	    pr = r2;
	    r2 = r2->nocc;
	  }
	  if (!r2) {
	    print_clause(stdout, c);
	    abend("cl_del_int, bad equality clause.");
	  }
	  if (pr)
	    pr->nocc = r->nocc;
	  else
	    r->argval->occ.rel = r->nocc;
	  if (!r->argval->occ.rel)
	    disintegrate_term(r->argval);
	  pr = r;
	  r = r->narg;
	  free_rel(pr);
	}
	free_term(r1->argval);  /* alpha or beta */
	pr = r1;
	r1 = r1->narg;
	free_rel(pr);
      }
    }
    else if (atom->type == COMPLEX) {
      r = atom->farg;
      while (r) {
	r2 = r->argval->occ.rel;
	pr = NULL;
	while (r2 && r2 != r) {
	  pr = r2;
	  r2 = r2->nocc;
	}
	if (!r2) {
	  print_clause(stdout, c);
	  abend("cl_del_int, bad clause.");
	}
	if (!pr)
	  r->argval->occ.rel = r->nocc;
	else
	  pr->nocc = r->nocc;
	if (!r->argval->occ.rel)
	  disintegrate_term(r->argval);
	pr = r;
	r = r->narg;
	free_rel(pr);
      }
    }
    free_term(atom);
    plit = lit;
    lit = lit->next_lit;
    free_literal(plit);
  }
  ip1 = c->parents;
  while (ip1) {
    ip2 = ip1;
    ip1 = ip1->next;
    free_ilist(ip2);
  }
  cl_delete_tab(c);
  /* If there is other memory associated with clause, free it here */
  delete_attributes(c);
  free_clause(c);
}  /* cl_del_int */

/*************
 *
 *    cl_del_non(c) -- deallocate a nonintegrated clause.
 *
 *************/

void cl_del_non(struct clause *c)
{
  struct literal *lit, *plit;
  struct ilist *ip1, *ip2;

  lit = c->first_lit;
  while (lit) {
    lit->atom->occ.lit = NULL;
    zap_term(lit->atom);
    plit = lit;
    lit = lit->next_lit;
    free_literal(plit);
  }
  ip1 = c->parents;
  while (ip1) {
    ip2 = ip1;
    ip1 = ip1->next;
    free_ilist(ip2);
  }
  /* If there is other memory associated with clause, free it here */
  delete_attributes(c);
  free_clause(c);
}  /* cl_del_non */

/*************
 *
 *    cl_int_chk(c) -- check structure of clause -- for debugging
 *
 *************/

void cl_int_chk(struct clause *c)
{
  struct literal *lit;

  printf("checking clause, address:%p " , (void *) c);
  print_clause(stdout, c);
  lit = c->first_lit;
  while (lit) {
    printf("    literal, address:%p sign:%d type:%d; atom:", (void *) lit, lit->sign, lit->atom->varnum);
    print_term(stdout, lit->atom); printf("\n");
    printf("    cont_cl:%p, atom container:%p\n", (void *) lit->container, (void *) lit->atom->occ.lit);
    lit = lit->next_lit;
  }
}  /* cl_int_chk */

/*************
 *
 *    struct term *literals_to_term(l)
 *
 *    Conver list of literals to right-associated "|" term.
 *
 *************/

static struct term *literals_to_term(struct literal *l)
{
  struct term *t, *t1, *t2;
  struct rel *r1, *r2;

  if (l->sign)
    t1 = copy_term(l->atom);
  else {
    t1 = get_term();
    r1 = get_rel();
    t1->farg = r1;
    t1->type = COMPLEX;
    t1->sym_num = str_to_sn("-", 1);
    r1->argval = copy_term(l->atom);
  }
  if (l->next_lit) {
    t2 = literals_to_term(l->next_lit);
    t = get_term(); r1 = get_rel(); r2 = get_rel();
    t->farg = r1; r1->narg = r2;
    r1->argval = t1; r2->argval = t2;
    t->type = COMPLEX;
    t->sym_num = str_to_sn("|", 2);
  }
  else
    t = t1;
  return(t);
}  /* literals_to_term */

/*************
 *
 *    struct term *clause_to_term(c)
 *
 *************/

struct term *clause_to_term(struct clause *c)
{
  struct term *t;

  if (c->first_lit)
    t = literals_to_term(c->first_lit);
  else {
    t = get_term();
    t->type = NAME;
    t->sym_num = str_to_sn("$F", 0);
  }
  return(t);
}  /* clause_to_term */

/*************
 *
 *    struct literal *term_to_literals(t, lits)
 *
 *************/

static struct literal *term_to_literals(struct term *t,
					struct literal *lits)
{
  struct literal *l;

  if (!is_symbol(t, "|", 2)) {
    l = get_literal();
    l->next_lit = lits;
    l->sign = !is_symbol(t, "-", 1);
    if (l->sign)
      l->atom = copy_term(t);
    else
      l->atom = copy_term(t->farg->argval);
    return(l);
  }
  else {
    l = term_to_literals(t->farg->narg->argval, lits);
    l = term_to_literals(t->farg->argval, l);
    return(l);
  }
}  /* term_to_literals */

/*************
 *
 *    struct clause *term_to_clause(t)
 *
 *    If error found, print message and return NULL.
 *
 *************/

struct clause *term_to_clause(struct term *t)
{
  struct clause *c;
  struct literal *l;

  c = get_clause();

  if (is_symbol(t, "#", 2)) {
    /* Right arg is attributes, left arg is clause. */
    c->attributes = term_to_attributes(t->farg->narg->argval);
    if (!c->attributes)
      return(NULL);
    else
      t = t->farg->argval;
  }

  c->first_lit = term_to_literals(t, (struct literal *) NULL);

  for (l = c->first_lit; l; l = l->next_lit) {
    if (l->atom->type == VARIABLE) {
      fprintf(stdout, "\nERROR, clause contains variable literal:\n");
      print_term(stdout, t); printf(".\n\n");
      return(NULL);
    }
    else {
      l->container = c;
      l->atom->occ.lit = l;
      mark_literal(l);
    }
  }
  return(c);
}  /* term_to_clause */

/*************
 *
 *    struct clause *read_sequent_clause(fp, retcd_ptr)
 *
 *    retcd - 0:  error (NULL returned)
 *            1:  ok    (NULL returned if EOF encountered)
 *
 *    a,b,c->d,e,f.
 *    ->d,e,f.
 *    a,b,c->.
 *    ->.
 *
 *    This is really ugly, kludgey code.  
 *
 *************/

struct clause *read_sequent_clause(FILE *fp,
				   int *rcp)
{
  struct clause *c;
  struct literal *l, *prev;
  struct rel *r1, *r2;
  char buf1[MAX_BUF+1];
  char buf2[MAX_BUF+6];
  int rc, i1, i2;
  struct term *hyps, *concs, *t;

  rc = read_buf(fp, buf1);
  if (rc == 0) {           /* error */
    *rcp = 0;
    return(NULL);
  }
  if (buf1[0] == '\0') {    /* ok. EOF */
    *rcp = 1;
    return(NULL);
  }

  /* Kludge - make it into a string readable by regular parser. */
  /* "a,b,c->d,e,f"  becomes  "$Hyps(a,b,c)->$Concs(d,e,f)" */
  /* "->d,e,f"  becomes  "$Hyps->$Concs(d,e,f)"  */
  /* "a,b,c->"  becomes  "$Hyps(a,b,c)->$Concs" */
  /* "->"  becomes  "$Hyps->$Concs" */

  i1 = 0;
  skip_white(buf1, &i1);

  /* first check for "end_of_list" */

  if (initial_str("end_of_list", buf1+i1)) {
    i1 += 11;
    skip_white(buf1, &i1);
    if (buf1[i1] == '.') {
      t = get_term(); t->type = NAME;
      t->sym_num = str_to_sn("end_of_list", 0);
      c = get_clause(); l = get_literal(); c->first_lit = l;
      l->atom = t;
      *rcp = 1;
      return(c);
    }
  }

  /* now reset and start again */

  i1 = 0;
  skip_white(buf1, &i1);

  i2 = 0;
  buf2[i2++] = '$';
  buf2[i2++] = 'H';
  buf2[i2++] = 'y';
  buf2[i2++] = 'p';
  buf2[i2++] = 's';

  if (buf1[i1] != '-' || buf1[i1+1] != '>') {
    /* Hyps not empty */
    buf2[i2++] = '(';
    while (buf1[i1] != '-' || buf1[i1+1] != '>') {
      if (buf1[i1] == '.') {
	fprintf(stdout, "\nERROR, arrow not found in sequent:\n");
	print_error(stdout, buf1, i1);
	*rcp = 0;
	return(NULL);
      }
      buf2[i2++] = buf1[i1++];
    }
    buf2[i2++] = ')';
  }
    
  buf2[i2++] = '-';
  buf2[i2++] = '>';
  buf2[i2++] = '$';
  buf2[i2++] = 'C';
  buf2[i2++] = 'o';
  buf2[i2++] = 'n';
  buf2[i2++] = 'c';
  buf2[i2++] = 's';

  i1 += 2;  /* skip over "->" */
  skip_white(buf1, &i1);

  if (buf1[i1] != '.') {
    /* concs not empty */
    buf2[i2++] = '(';
    while (buf1[i1] != '.') {
      buf2[i2++] = buf1[i1++];
    }
    buf2[i2++] = ')';
  }
  buf2[i2++] = '.';
  buf2[i2++] = '\0';

#if 0
  printf("before: %s\n", buf1);
  printf("after:  %s\n", buf2);
#endif    

  i2 = 0;
  t = str_to_term(buf2, &i2, 0);
  if (!t) {
    *rcp = 0;
    return(NULL);
  }
  else {
    skip_white(buf2, &i2);
    if (buf2[i2] != '.') {
      fprintf(stdout, "\nERROR, text after term:\n");
      print_error(stdout, buf2, i2);
      *rcp = 0;
      return(NULL);
    }
  }

  t = term_fixup(t);

  if (!set_vars(t)) {
    fprintf(stdout, "\nERROR, input clause contains too many variables:\n");
    print_term(stdout, t); printf(".\n\n");
    zap_term(t);
    *rcp = 0;
    return(NULL);  /* error */
  }
  else if (contains_skolem_symbol(t)) {
    fprintf(stdout, "\nERROR, input clause contains Skolem symbol:\n");
    print_term(stdout, t); printf(".\n\n");
    zap_term(t);
    *rcp = 0;
    return(NULL);  /* error */
  }

  hyps = t->farg->argval;
  concs = t->farg->narg->argval;
  free_rel(t->farg->narg);
  free_rel(t->farg);
  free_term(t);

  /* ok, now we have hypotheses and conclusion */

  c = get_clause();
    
  r1 = hyps->farg;
  prev = NULL;
  while (r1) {
    l = get_literal();
    if (prev)
      prev->next_lit = l;
    else
      c->first_lit = l;
    prev = l;
    l->sign = 0;
    l->atom = r1->argval;
    r2 = r1;
    r1 = r1->narg;
    free_rel(r2);
  }
  free_term(hyps);

  r1 = concs->farg;
  while (r1) {
    l = get_literal();
    if (prev)
      prev->next_lit = l;
    else
      c->first_lit = l;
    prev = l;
    l->sign = 1;
    l->atom = r1->argval;
    r2 = r1;
    r1 = r1->narg;
    free_rel(r2);
  }
  free_term(concs);

  for (l = c->first_lit; l; l = l->next_lit) {
    l->container = c;
    l->atom->occ.lit = l;
    mark_literal(l);
    if (contains_skolem_symbol(l->atom)) {
      fprintf(stdout,"\nERROR, input literal contains Skolem symbol:\n");
      print_term(stdout, l->atom); printf(".\n\n");
      *rcp = 0;
      return(NULL);
    }
  }
  *rcp = 1;
  return(c);

}  /* read_sequent_clause */

/*************
 *
 *    struct clause *read_clause(fp, retcd_ptr)
 *
 *    retcd - 0:  error (NULL returned)
 *            1:  ok    (NULL returned if EOF encountered)
 *
 *************/

struct clause *read_clause(FILE *fp,
			   int *rcp)
{
  struct term *t;
  struct clause *cl;
  int rc;

  if (Flags[INPUT_SEQUENT].val)
    return(read_sequent_clause(fp, rcp));

  t = read_term(fp, &rc);
  if (!rc) {
    *rcp = 0;
    return(NULL);  /* error reading term */
  }
  else if (!t) {
    *rcp = 1;
    return(NULL);  /* EOF */
  }
  else if (!set_vars(t)) {
    fprintf(stdout, "\nERROR, input clause contains too many variables:\n");
    print_term(stdout, t); printf(".\n\n");
    zap_term(t);
    *rcp = 0;
    return(NULL);  /* error */
  }
  else if (contains_skolem_symbol(t)) {
    fprintf(stdout, "\nERROR, input clause contains Skolem symbol:\n");
    print_term(stdout, t); printf(".\n\n");
    zap_term(t);
    *rcp = 0;
    return(NULL);  /* error */
  }
  else {
    cl = term_to_clause(t);
    zap_term(t);
    if (cl)
      *rcp = 1;
    else
      *rcp = 0;  /* error */
    return(cl);
  }

}  /* read_clause */

/*************
 *
 *    struct list *read_cl_list(fp, errors_ptr)
 *
 *    Read clauses until EOF or the term `end_of_list' is reached.
 *
 *************/

struct list *read_cl_list(FILE *fp,
			  int *ep)
{
  struct list *lst;
  struct clause *cl, *pcl;
  int rc;

  Internal_flags[REALLY_CHECK_ARITY] = 1;

  *ep = 0;
  lst = get_list();
  pcl = NULL;
  cl = read_clause(fp, &rc);
  while (rc == 0) {  /* while errors */
    (*ep)++;
    cl = read_clause(fp, &rc);
  }
  while (cl && !(cl->first_lit &&
		 is_symbol(cl->first_lit->atom, "end_of_list", 0))) {
    if (!pcl)
      lst->first_cl = cl;
    else
      pcl->next_cl = cl;
    cl->prev_cl = pcl;
    cl->container = lst;
    pcl = cl;
    cl = read_clause(fp, &rc);
    while (rc == 0) {  /* while errors */
      (*ep)++;
      cl = read_clause(fp, &rc);
    }
  }
  if (cl)
    cl_del_non(cl);  /* "end_of_list" term */
  lst->last_cl = pcl;

  Internal_flags[REALLY_CHECK_ARITY] = 0;
  return(lst);
}  /* read_cl_list */

/*************
 *
 *    int set_vars_cl(cl) -- decide which terms are variables
 *
 *************/

int set_vars_cl(struct clause *cl)
{
  struct literal *lit;
  char *varnames[MAX_VARS];
  int i;

  for (i=0; i<MAX_VARS; i++)
    varnames[i] = NULL;
  lit = cl->first_lit;
  while (lit) {
    if (set_vars_term(lit->atom, varnames))
      lit = lit->next_lit;
    else
      return(0);
  }
  return(1);
}  /* set_vars_cl */

/*************
 *
 *   print_sequent_clause()
 *
 *   Clause number and parents have already been printed.
 *
 *************/

void print_sequent_clause(FILE *fp,
			  struct clause *c)
{
  struct literal *l;
  int first;

  for (l = c->first_lit, first = 1; l; l = l->next_lit) {
    if (!l->sign) {
      if (!first)
	fprintf(fp, ", ");
      print_term(fp, l->atom);
      first = 0;
    }
  }
  fprintf(fp, " -> ");
  for (l = c->first_lit, first = 1; l; l = l->next_lit) {
    if (l->sign) {
      if (!first)
	fprintf(fp, ", ");
      print_term(fp, l->atom);
      first = 0;
    }
  }
    
}  /* print_sequent_clause */

/*************
 *
 *    print_justification(fp, clause)
 *
 *************/

void print_justification(FILE *fp,
		       struct ilist *just)
{
  struct ilist *ip;
  fprintf(fp,"[");
  for (ip = just; ip; ip = ip->next) {

    if (ip->i <= LIST_RULE) {
      /* LIST_RULE is a large negative number.
	 If ip->i is less than LIST_RULE, then a list
	 of length LIST_RULE-ip->i follows.
      */
      int i;
      int j = LIST_RULE - ip->i;

      for (i = 1; i <= j; i++) {
	ip = ip->next;
	fprintf(fp, ".%d", ip->i);
      }
    }
    else {
      if (ip != just)
	fprintf(fp, ",");

      switch (ip->i) {
      case BINARY_RES_RULE  : fprintf(fp, "binary"); break;
      case HYPER_RES_RULE   : fprintf(fp, "hyper"); break;
      case NEG_HYPER_RES_RULE   : fprintf(fp, "neg_hyper"); break;
      case UR_RES_RULE      : fprintf(fp, "ur"); break;
      case PARA_INTO_RULE   : fprintf(fp, "para_into"); break;
      case PARA_FROM_RULE   : fprintf(fp, "para_from"); break;
      case FACTOR_RULE      : fprintf(fp, "factor"); break;
      case FACTOR_SIMP_RULE : fprintf(fp, "factor_simp"); break;

      case NEW_DEMOD_RULE   : fprintf(fp, "new_demod"); break;
      case BACK_DEMOD_RULE  : fprintf(fp, "back_demod"); break;
      case DEMOD_RULE       : fprintf(fp, "demod"); break;

      case UNIT_DEL_RULE    : fprintf(fp, "unit_del"); break;
		
      case LINKED_UR_RES_RULE : fprintf(fp, "linked_ur"); break;
      case EVAL_RULE        : fprintf(fp, "eval"); break;
      case GEO_ID_RULE      : fprintf(fp, "gL-id"); break;
      case GEO_RULE         : fprintf(fp, "gL"); break;
      case COPY_RULE        : fprintf(fp, "copy"); break;
      case FLIP_EQ_RULE     : fprintf(fp, "flip"); break;
      case CLAUSIFY_RULE    : fprintf(fp, "clausify"); break;
      case BACK_UNIT_DEL_RULE : fprintf(fp, "back_unit_del"); break;
      case SPLIT_RULE       : fprintf(fp, "split"); break;
      case SPLIT_NEG_RULE   : fprintf(fp, "split_neg"); break;
      case PROPOSITIONAL_RULE : fprintf(fp, "propositional"); break;
      default               : fprintf(fp, "%d", ip->i); break;
      }
    }
  }
  fprintf(fp, "]");
}  /* print_justification */

/*************
 *
 *    print_clause_bare(fp, clause)
 *
 *    No ID, justification, period, or newline.
 *
 *************/

void print_clause_bare(FILE *fp,
		  struct clause *cl)
{
  struct term *t = clause_to_term(cl);
  t = term_fixup_2(t);  /* Change -(=(a,b)) to !=(a,b). */
  print_term(fp, t);
  zap_term(t);
}  /* print_clause_bare */

/*************
 *
 *    print_clause(fp, clause)
 *
 *************/

void print_clause(FILE *fp,
		  struct clause *cl)
{
  struct literal *lit;
  struct term *t;

  fprintf(fp, "%d ", cl->id);

  if (cl->heat_level > 0)
    fprintf(fp, "(heat=%d) ", (int) (cl->heat_level));

  if (! multi_justifications()) {
    print_justification(fp, cl->parents);
    fprintf(fp, " ");
  }
  else {
    if (cl->multi_parents == NULL)
      fprintf(fp, " [] ");
    else {
      struct g2list *p;
      for (p = cl->multi_parents; p != NULL; p = p->next) {
	if (p == cl->multi_parents)
	  fprintf(fp, " ");
	else
	  fprintf(fp, "\n        ");
	print_justification(fp, p->v1);
      }
      fprintf(fp, " ");
    }
  }
  
  
  if (Flags[PRETTY_PRINT].val) {
    int parens;
    
    fprintf(fp, "\n");
    lit = cl->first_lit;
    while (lit) {
      parens = !lit->sign && sn_to_node(lit->atom->sym_num)->special_op;
      if (!lit->sign)
	fprintf(fp, "-");
      if (parens)
	fprintf(fp, "(");
      pretty_print_term(fp, lit->atom, 0);
      if (parens)
	fprintf(fp, ")");
      lit = lit->next_lit;
      if (lit)
	fprintf(fp, " |\n");
    }
  }
  else if (Flags[OUTPUT_SEQUENT].val) {
    print_sequent_clause(fp, cl);
  }
  else {
#if 0
    struct rel *r;
    lit = cl->first_lit;
    while (lit) {
      if (!lit->sign) {
	/* This is so that lit gets correctly parenthesized. */
	t = get_term();
	r = get_rel();
	t->farg = r;
	t->type = COMPLEX;
	r->argval = lit->atom;
	t->sym_num = str_to_sn("-", 1);
	print_term(fp, t);
	free_rel(r);
	free_term(t);
      }
      else
	print_term(fp, lit->atom);
      lit = lit->next_lit;
      if (lit)
	fprintf(fp, " | ");
    }
#else
    t = clause_to_term(cl);
    t = term_fixup_2(t);  /* Change -(=(a,b)) to !=(a,b). */
    print_term(fp, t);
    zap_term(t);
#endif
  }
  if (cl->attributes)
    print_attributes(fp, cl->attributes);

  fprintf(fp, ".\n");
}  /* print_clause */

/*************
 *
 *    print_clause_without_just(fp, clause)
 *
 *************/

void print_clause_without_just(FILE *fp,
		  struct clause *cl)
{
  struct literal *lit;
  struct term *t;

  fprintf(fp, "%d ", cl->id);

  if (cl->heat_level > 0)
    fprintf(fp, "(heat=%d) ", (int) (cl->heat_level));

  if (Flags[PRETTY_PRINT].val) {
    int parens;
    
    fprintf(fp, "\n");
    lit = cl->first_lit;
    while (lit) {
      parens = !lit->sign && sn_to_node(lit->atom->sym_num)->special_op;
      if (!lit->sign)
	fprintf(fp, "-");
      if (parens)
	fprintf(fp, "(");
      pretty_print_term(fp, lit->atom, 0);
      if (parens)
	fprintf(fp, ")");
      lit = lit->next_lit;
      if (lit)
	fprintf(fp, " |\n");
    }
  }
  else if (Flags[OUTPUT_SEQUENT].val) {
    print_sequent_clause(fp, cl);
  }
  else {
#if 0
    struct rel *r;
    lit = cl->first_lit;
    while (lit) {
      if (!lit->sign) {
	/* This is so that lit gets correctly parenthesized. */
	t = get_term();
	r = get_rel();
	t->farg = r;
	t->type = COMPLEX;
	r->argval = lit->atom;
	t->sym_num = str_to_sn("-", 1);
	print_term(fp, t);
	free_rel(r);
	free_term(t);
      }
      else
	print_term(fp, lit->atom);
      lit = lit->next_lit;
      if (lit)
	fprintf(fp, " | ");
    }
#else
    t = clause_to_term(cl);
    t = term_fixup_2(t);  /* Change -(=(a,b)) to !=(a,b). */
    print_term(fp, t);
    zap_term(t);
#endif
  }
  if (cl->attributes)
    print_attributes(fp, cl->attributes);

  fprintf(fp, ".\n");
}  /* print_clause_without_just */

/*************
 *
 *    p_clause(clause)
 *
 *************/

void p_clause(struct clause *cl)
{
  print_clause(stdout, cl);
}  /* p_clause */

/*************
 *
 *    print_cl_list(fp, lst)
 *
 *************/

void print_cl_list(FILE *fp,
		   struct list *lst)
{
  struct clause *cl;

  if (!lst)
    fprintf(fp, "(list nil)\n");
  else {
    cl = lst->first_cl;
    while (cl) {
      print_clause(fp, cl);
      cl = cl->next_cl;
    }
    fprintf(fp, "end_of_list.\n");
  }
}  /* print_cl_list */

/*************
 *
 *    cl_merge(cl) -- merge identical literals (keep leftmost occurrence)
 *
 *************/

void cl_merge(struct clause *c)
{
  struct literal *l1, *l2, *l_prev;

  l1 = c->first_lit;
  while (l1) {
    l2 = l1->next_lit;
    l_prev = l1;
    while (l2)
      if (l1->sign == l2->sign && term_ident(l1->atom, l2->atom)) {
	l_prev->next_lit = l2->next_lit;
	l2->atom->occ.lit = NULL;
	zap_term(l2->atom);
	free_literal(l2);
	l2 = l_prev->next_lit;
      }
      else {
	l_prev = l2;
	l2 = l2->next_lit;
      }
    l1 = l1->next_lit;
  }
}  /* cl_merge */

/*************0
 *
 *     int tautology(c) -- Is clause c a tautology?
 *
 *************/

int tautology(struct clause *c)
{
  struct literal *l1, *l2;
  int taut;

  taut = 0;
  l1 = c->first_lit;
  while (l1 && !taut) {
    l2 = l1->next_lit;
    while (l2 && !taut) {
      taut = (l1->sign != l2->sign && term_ident(l1->atom, l2->atom));
      l2 = l2->next_lit;
    }
    l1 = l1->next_lit;
  }
  return(taut);
}  /* tautology */

/*************
 *
 *   prf_weight()
 *
 *   Return the number of leaves (i.e., occurrences of input clauses)
 *   in the proof tree.
 *
 *************/

int prf_weight(struct clause *c)
{
  struct ilist *ip;
  struct clause *d;
  int sum = 0;

  for (ip = c->parents; ip; ip = ip->next) {
    if (ip->i <= LIST_RULE) {
      /* LIST_RULE is a large negative number. */
      /* If ip->i is less than LIST_RULE, then a list follows. */
      int i;
      int j = LIST_RULE - ip->i;  /* size of list */
      /* Make ip point at the last element of the list. */
      for (i = 1; i <= j; i++)
	ip = ip->next;
    }
    else if (ip->i >= 0) {
      d = cl_find(ip->i);
      if (d)
	sum += prf_weight(d);
    }
  }
  return(sum == 0 ? 1 : sum);
}  /* prf_weight */

/*************
 *
 *    int proof_length(c)
 *
 *    Return length of proof.  If demod_history is clear, demodulation
 *    steps are not counted.  "new_demod" steps are not counted.
 *
 *************/

int proof_length(struct clause *c)
{
  struct clause_ptr *cp1, *cp2;
  struct ilist *ip1, *ip2;
  int count, level;

  cp1 = NULL; ip1 = NULL;
  level = get_ancestors(c, &cp1, &ip1);

  for (count = 0; cp1; ) {
    if (cp1->c->parents && cp1->c->parents->i != NEW_DEMOD_RULE)
      count++;
    cp2 = cp1; cp1 = cp1->next; free_clause_ptr(cp2);
    ip2 = ip1; ip1 = ip1->next; free_ilist(ip2);
  }
  return(count);
}  /* proof_length */

/*************
 *
 *    int subsume(c, d) -- does clause c subsume clause d?
 *
 *************/

int subsume(struct clause *c,
	    struct clause *d)
{
  struct context *s;
  struct trail *tr;
  int subsumed;

  s = get_context();
  tr = NULL;
  subsumed = map_rest(c, d, s, &tr);
  if (subsumed)
    clear_subst_1(tr);
  free_context(s);
  return(subsumed);
}  /* subsume */

/*************
 *
 *    int map_rest(c, d, s, trp) - map rest of literals - for subsumption
 *
 *************/

int map_rest(struct clause *c,
	     struct clause *d,
	     struct context *s,
	     struct trail **trp)
{
  struct literal *c_lit, *d_lit;
  struct term *c_atom, *d_atom;
  struct trail *t_pos;
  int subsumed, i;

  /* get the first unmarked literal */
  c_lit = c->first_lit;
  i = 0;
  while (c_lit && Map_array[i] == 1) {
    c_lit = c_lit->next_lit;
    i++;
  }

  if (!c_lit)
    return(1);  /* all lits of c mapped, so c subsumes d */
  else if (answer_lit(c_lit)) {   /* if answer literal, skip it */
    c_atom = c_lit->atom;
    Map_array[i] = 1;      /* mark as mapped */
    subsumed = map_rest(c, d, s, trp);
    Map_array[i] = 0;      /* remove mark */
    return(subsumed);
  }
  else {
    c_atom = c_lit->atom;
    Map_array[i] = 1;      /* mark as mapped */
    d_lit = d->first_lit;
    subsumed = 0;
    while (d_lit && !subsumed) {
      d_atom = d_lit->atom;
      t_pos = *trp;  /* save position in trail in case of failure */
      if (c_lit->sign == d_lit->sign && match(c_atom, s, d_atom, trp)) {
	if (map_rest(c, d, s, trp))
	  subsumed = 1;
	else {
	  clear_subst_2(*trp, t_pos);
	  *trp = t_pos;
	}
      }
      d_lit = d_lit->next_lit;
    }
    Map_array[i] = 0;      /* remove mark */
    return(subsumed);
  }
}  /* map_rest */

/*************
 *
 *    int anc_subsume(c, d)
 *
 *    We already know that c subsumes d.  Check if d subsumes c and
 *    ancestors(c) <= ancestors(d).
 *
 *************/

int anc_subsume(struct clause *c,
		struct clause *d)
{
  if (subsume(d,c)) {
    if (Flags[PROOF_WEIGHT].val)
      return(prf_weight(c) <= prf_weight(d));
    else
      return(proof_length(c) <= proof_length(d));
  }
  else
    return(1);
}  /* anc_subsume */

/*************
 *
 *    struct clause *for_sub_prop(d)
 *
 *    Attempt to find a clause that propositionally subsumes d.
 *
 *************/

struct clause *for_sub_prop(struct clause *d)
{
  struct clause *c;

  for (c = Usable->first_cl; c; c = c->next_cl)
    if (ordered_sub_clause(c, d))
      return(c);

  for (c = Sos->first_cl; c; c = c->next_cl)
    if (ordered_sub_clause(c, d))
      return(c);

  return(NULL);

}  /* for_sub_prop */

/*************
 *
 *    struct clause *forward_subsume(d)
 *
 *    Attempt to find a clause that subsumes d.
 *
 *************/

struct clause *forward_subsume(struct clause *d)
{
  int subsumed;
  struct literal *d_lit;
  struct clause *c = NULL;
  struct term *c_atom, *d_atom;
  struct context *s;
  struct trail *tr;
  struct is_tree *is_db;
  struct fsub_pos *pos;
  struct fpa_index *fpa_db;
  struct fpa_tree *ut;
  int c_size, factor, i;
  int d_size = -1;
  struct literal *lit;

  if (Flags[PROPOSITIONAL].val)
    return(for_sub_prop(d));

  subsumed = 0;
  s = get_context();

  factor = Flags[FACTOR].val;
  if (factor)  /* if factor don't let long clauses subsume short */
    d_size = num_literals(d);

  if (!Flags[FOR_SUB_FPA].val) {  /* if `is' indexing */

    d_lit = d->first_lit;

    while (d_lit && !subsumed) {
      /* Is_pos_lits and Is_neg_lits are global variables */
      is_db = d_lit->sign ? Is_pos_lits : Is_neg_lits;
      c_atom = fs_retrieve(d_lit->atom, s, is_db, &pos);
      while (c_atom && !subsumed) {
	c = c_atom->occ.lit->container;
	c_size = num_literals(c);
	if (c_size > MAX_LITS) {
	  abend("forward_subsume, MAX_LITS too small.");
	}

	if (literal_number(c_atom->occ.lit) == 1 && (!factor || c_size <= d_size)) {
	  for (i = 0, lit = c->first_lit;
	       lit->atom != c_atom;
	       i++, lit = lit->next_lit);  /* empty body */
	  Map_array[i] = 1;      /* mark as mapped*/
	  tr = NULL;
	  subsumed = map_rest(c, d, s, &tr);
	  Map_array[i] = 0;      /* remove mark */
	  clear_subst_1(tr);
	}

	if (subsumed && multi_justifications()) {
	  if (subsume(d,c)) {
	    /* c (old) and d (new) are equivalent, so add d's just to c. */
	    if (d->parents != NULL)
	      possibly_append_parents(c, d->parents);
	  }
	}

	if (subsumed && Flags[ANCESTOR_SUBSUME].val) {
	  /* Removed variable renumbering 4/4/2001; shouldn't be necessary */
	  subsumed = anc_subsume(c,d);
	  if (!subsumed)
	    Stats[CL_NOT_ANC_SUBSUMED]++;
	}

	/* BV(970327) : forward sub only if sub goes both ways */
	if (subsumed && Flags[FOR_SUB_EQUIVALENTS_ONLY].val)
	  subsumed = subsume(d,c);

	if (!subsumed)
	  c_atom = fs_retrieve((struct term *) NULL, s, is_db, &pos);
	else
	  canc_fs_pos(pos, s);
      }
      d_lit = d_lit->next_lit;
    }
  }
  else {  /* fpa indexing */

    d_lit = d->first_lit;
    while (d_lit && !subsumed) {
      fpa_db = (d_lit->sign ? Fpa_pos_lits : Fpa_neg_lits);
      d_atom = d_lit->atom;
      ut = build_tree(d_atom, MORE_GEN, Parms[FPA_LITERALS].val, fpa_db);
      c_atom = next_term(ut, 0);
      while (c_atom && !subsumed) {
	tr = NULL;
	c = c_atom->occ.lit->container;
	c_size = num_literals(c);
	if (c_size > MAX_LITS) {
	  abend("forward_subsume, MAX_LITS too small.");
	}
	if (literal_number(c_atom->occ.lit) == 1 &&
	    (!factor || c_size <= d_size) &&
	    match(c_atom, s, d_atom, &tr)) {
		
	  for (i = 0, lit = c->first_lit;
	       lit->atom != c_atom;
	       i++, lit = lit->next_lit);  /* empty body */
	  Map_array[i] = 1;      /* mark as mapped*/
	  subsumed = map_rest(c, d, s, &tr);
	  Map_array[i] = 0;      /* remove mark */
	  clear_subst_1(tr);
	}

	if (subsumed && Flags[ANCESTOR_SUBSUME].val) {
	  /* Removed variable renumbering 4/4/2001; shouldn't be necessary */
	  subsumed = anc_subsume(c,d);
	  if (!subsumed)
	    Stats[CL_NOT_ANC_SUBSUMED]++;
	}

	/* BV(970327) : forward sub only if sub goes both ways */
	if (subsumed && Flags[FOR_SUB_EQUIVALENTS_ONLY].val)
	  subsumed = subsume(d,c);

	if (!subsumed)
	  c_atom = next_term(ut, 0);
	else
	  zap_prop_tree(ut);
      }
      d_lit = d_lit->next_lit;
    }
  }
  free_context(s);
  if (subsumed)
    return(c);
  else
    return(NULL);
}  /* forward_subsume */

/*************
 *
 *    struct clause_ptr *back_subsume(c)
 *
 *    Get the list of clauses subsumed by c.
 *
 *************/

struct clause_ptr *back_subsume(struct clause *c)
{
  int subsumed, c_size, factor, i;
  struct literal *c_lit;
  struct clause *d;
  struct clause_ptr *subsumed_clauses;
  struct term *c_atom, *d_atom;
  struct context *s;
  struct fpa_tree *ut;
  struct trail *tr;

  factor = Flags[FACTOR].val;

  c_size = num_literals(c);

  if (c_size > MAX_LITS) {
    abend("back_subsume, MAX_LITS too small.");
  }

  s = get_context();
  c_lit = c->first_lit;
  /* get first non-answer literal */
  i = 0;
  while (c_lit && answer_lit(c_lit)) {
    c_lit = c_lit->next_lit;
    i++;
  }

  if (!c_lit) {
    fprintf(stdout, "\nNOTE: back_subsume called with empty clause.\n");
    return(NULL);
  }

  c_atom = c_lit->atom;
  ut = build_tree(c_atom, INSTANCE, Parms[FPA_LITERALS].val,
		  c_lit->sign ? Fpa_pos_lits : Fpa_neg_lits);
  /* Fpa_pos_lits and Fpa_neg_lits are global variables */

  subsumed_clauses = NULL;
  d_atom = next_term(ut, 0);
  while (d_atom) {
    d = d_atom->occ.lit->container;
    tr = NULL;
    if (c != d && (!factor || c_size <= num_literals(d))
	&& match(c_atom, s, d_atom, &tr)) {
      Map_array[i] = 1;  /* mark as mapped */
      subsumed = map_rest(c, d, s, &tr);
      Map_array[i] = 0;    /* remove mark */
      clear_subst_1(tr);
      if (subsumed && Flags[ANCESTOR_SUBSUME].val)
	subsumed = anc_subsume(c, d);
      if (subsumed)
      insert_clause(d, &subsumed_clauses);
    }
    d_atom = next_term(ut, 0);
  }
  free_context(s);
  return(subsumed_clauses);
}  /* back_subsume */

/*************
 *
 *    struct clause_ptr *unit_conflict(c)
 *
 *    Search for unit conflict.  Return empty clause if found,
 *    return NULL if not found.
 *
 *    IT IS ASSUMED THAT c IS A UNIT CLAUSE!!
 *
 *************/

struct clause_ptr *unit_conflict(struct clause *c)
{
  struct clause *d, *e;
  struct fpa_tree *ut;
  struct term *f_atom;
  struct literal *lit;
  int go, mp, ec;
  struct context *c1, *c2;
  struct trail *tr;
  struct clause_ptr *cp_return, *cp_prev, *cp_curr;

  c1 = get_context();
  c1->multiplier = 0;
  c2 = get_context();
  c2->multiplier = 1;
  lit = c->first_lit;
  while (answer_lit(lit))  /* skip answer literals */
    lit = lit->next_lit;
  ut = build_tree(lit->atom, UNIFY, Parms[FPA_LITERALS].val,
		  lit->sign ? Fpa_neg_lits : Fpa_pos_lits);
  f_atom = next_term(ut, 0);
  go = 1;
  cp_return = cp_prev = NULL;
  while (go && f_atom) {
    tr = NULL;
    d = f_atom->occ.lit->container;
    if (num_literals(d) == 1 && unify(lit->atom, c1, f_atom, c2, &tr)) {

      e = build_bin_res(lit->atom, c1, f_atom, c2);
      clear_subst_1(tr);
      cl_merge(e);  /* answer literals */
      cp_curr = get_clause_ptr();
      cp_curr->c = e;
      if (cp_prev)
	cp_prev->next = cp_curr;
      else
	cp_return = cp_curr;
      cp_prev = cp_curr;

      ec = ++Stats[EMPTY_CLAUSES];
      mp = Parms[MAX_PROOFS].val;

      if (mp != -1 && ec >= mp)
	/* do not look for more proofs */
	go = 0;
    }
    if (go)
      f_atom = next_term(ut, 0);
    else
      zap_prop_tree(ut);
  }
  free_context(c1);
  free_context(c2);
  return(cp_return);
}  /* unit_conflict */
	
/*************
 *
 *    int propositional_clause(c) 
 *
 *    Is this a propositional clause?
 *
 *************/

int propositional_clause(struct clause *c)
{
  struct literal *lit;

  lit = c->first_lit;
  while (lit)
    if (lit->atom->type != NAME)
      return(0);
    else
      lit = lit->next_lit;
  return(1);
}  /* propositional_clause */

/*************
 *
 *    int xx_resolvable(c) 
 *
 *    Does the nonunit clause c have a literal that can resolve with x = x?
 *
 *************/

int xx_resolvable(struct clause *c)
{
  if (unit_clause(c))
    return(0);
  else {
    struct literal *lit;

    lit = c->first_lit;
    while (lit) {
      if (!lit->sign && is_eq(lit->atom->sym_num)) {
	struct term *a1 = lit->atom->farg->argval;
	struct term *a2 = lit->atom->farg->narg->argval;
	if (a1->type == VARIABLE && !occurs_in(a1, a2))
	  return(1);
	else if (a2->type == VARIABLE && !occurs_in(a2, a1))
	  return(1);
      }
      lit = lit->next_lit;
    }
    return(0);
  }
}  /* xx_resolvable */

/*************
 *
 *    int pos_clause(c) 
 *
 *    Is this a positive clause (excluding answer lits) ?
 *
 *************/

int pos_clause(struct clause *c)
{
  struct literal *lit;

  lit = c->first_lit;
  while (lit)
    if (!lit->sign && !answer_lit(lit))
      return(0);  /* fail because found negative non-anser literal */
    else
      lit = lit->next_lit;
  return(1);
}  /* pos_clause */

/*************
 *
 *    int answer_lit(lit)  --  Is this an answer literal?
 *
 *************/

int answer_lit(struct literal *lit)
{
  return(lit->atom->varnum == ANSWER);
}  /* answer_lit */

/*************
 *
 *    int pos_eq_lit(lit)  --  Is this a positive equality literal?
 *
 *************/

int pos_eq_lit(struct literal *lit)
{
  return(lit->atom->varnum == POS_EQ);
}  /* pos_eq_lit */

/*************
 *
 *    int neg_eq_lit(lit)  --  Is this a negative equality literal?
 *
 *************/

int neg_eq_lit(struct literal *lit)
{
  return(lit->atom->varnum == NEG_EQ);
}  /* neg_eq_lit */

/*************
 *
 *    int eq_lit(lit)  --  Is this an equality literal (pos or neg)?
 *
 *************/

int eq_lit(struct literal *lit)
{
  return(pos_eq_lit(lit) || neg_eq_lit(lit));
}  /* eq_lit */

/*************
 *
 *    int neg_clause(c)
 *
 *    Is this a negative clause (excluding answer lits) ?
 *
 *************/

int neg_clause(struct clause *c)
{
  struct literal *lit;

  lit = c->first_lit;
  while (lit)
    if (lit->sign && !answer_lit(lit))
      return(0);  /* fail because found positive non-answer literal */
    else
      lit = lit->next_lit;
  return(1);
}  /* neg_clause */

/*************
 *
 *    int num_literals(c)  --  How many literals (excluding answer literals)?
 *
 *************/

int num_literals(struct clause *c)
{
  int i;
  struct literal *lit;

  i = 0;
  lit = c->first_lit;
  while (lit) {
    if (!answer_lit(lit))  /* if not answer literal */
      i++;
    lit = lit->next_lit;
  }
  return(i);
}  /* num_literals */

/*************
 *
 *    int num_answers(c)  --  How many answer literals?
 *
 *************/

int num_answers(struct clause *c)
{
  int i;
  struct literal *lit;

  i = 0;
  lit = c->first_lit;
  while (lit) {
    if (answer_lit(lit))
      i++;
    lit = lit->next_lit;
  }
  return(i);
}  /* num_answers */

/*************
 *
 *    int num_literals_including_answers(c)  --  How many literals?
 *
 *************/

int num_literals_including_answers(struct clause *c)
{
  int i;
  struct literal *lit;

  for (i = 0, lit = c->first_lit; lit; i++, lit = lit->next_lit);
  return(i);
}  /* num_literals_including_answers */

/*************
 *
 *    int literal_number(lit)
 *
 *    lit is which literal (excluding answers) in the clause that contains it.
 *
 *************/

int literal_number(struct literal *lit)
{
  int i;
  struct literal *l;

  i = 1;
  l = lit->container->first_lit;
  while (l != lit) {
    if (!answer_lit(l))
      i++;
    l = l->next_lit;
  }
  return(i);
}  /* literal_number */

/*************
 *
 *    int unit_clause(c)  -- Is it a unit clause (don't count answers)?
 *
 *************/

int unit_clause(struct clause *c)
{
  return(num_literals(c) == 1);
}  /* unit_clause */

/*************
 *
 *    int horn_clause(c)
 *
 *    Is c a Horn clause (at most one positive literal)?
 *
 *    Ignore answer literals.
 *
 *************/

int horn_clause(struct clause *c)
{
  struct literal *lit;
  int i;

  for (lit = c->first_lit, i = 0; lit; lit = lit->next_lit)
    if (lit->sign && !answer_lit(lit))
      i++;
  return(i <= 1);
}  /* horn_clause */

/*************
 *
 *    int equality_clause(c)
 *
 *    Does c contain any equality literals (pos or neg)?
 *
 *************/

int equality_clause(struct clause *c)
{
  struct literal *lit;

  for (lit = c->first_lit; lit; lit = lit->next_lit)
    if (pos_eq_lit(lit) || neg_eq_lit(lit))
      return(1);
  return(0);
}  /* equality_clause */

/*************
 *
 *    int symmetry_clause(c)
 *
 *    Is c a clause for symmetry of equality?
 *
 *************/

int symmetry_clause(struct clause *c)
{
  struct literal *l1, *l2;

  if (num_literals(c) != 2)
    return(0);
  else {
    l1 = c->first_lit; l2 = l1->next_lit;
    if (l1->sign == l2->sign)
      return(0);
    else if (!eq_lit(l1) || l1->atom->sym_num != l2->atom->sym_num)
      return(0);
    else if (l1->atom->farg->argval->type != VARIABLE)
      return(0);
    else if (l2->atom->farg->argval->type != VARIABLE)
      return(0);
    else if (!term_ident(l1->atom->farg->argval,
			 l2->atom->farg->narg->argval))
      return(0);
    else if (!term_ident(l1->atom->farg->narg->argval,
			 l2->atom->farg->argval))
      return(0);
    else
      return(1);
  }
}  /* symmetry_clause */

/*************
 *
 *   struct literal *ith_literal(c, n)
 *
 *   Return the i-th (non-answer) literal.
 *
 *************/

struct literal *ith_literal(struct clause *c,
			    int n)
{
  int i;
  struct literal *lit;

  lit = c->first_lit;
  i = 0;
  while(lit) {
    if (!answer_lit(lit))
      i++;
    if (i == n)
      return(lit);
    else
      lit = lit->next_lit;
  }
  return(lit);
}  /* ith_literal */
 
/*************
 *
 *    append_cl(lst, cl)
 *
 *************/

void append_cl(struct list *l,
	       struct clause *c)
{
  c->next_cl = NULL;
  c->prev_cl = l->last_cl;

  if (!l->first_cl)
    l->first_cl = c;
  else
    l->last_cl->next_cl = c;
  l->last_cl = c;
  c->container = l;

  if (l == Usable)
    Stats[USABLE_SIZE]++;
  else if (l == Sos)
    Stats[SOS_SIZE]++;
  else if (l == Demodulators)
    Stats[DEMODULATORS_SIZE]++;

}  /* append_cl */

/*************
 *
 *    prepend_cl(lst, cl)
 *
 *************/

void prepend_cl(struct list *l,
		struct clause *c)
{
  c->prev_cl = NULL;
  c->next_cl = l->first_cl;
  if (!l->last_cl)
    l->last_cl = c;
  else
    l->first_cl->prev_cl = c;
  l->first_cl = c;
  c->container = l;

  if (l == Usable)
    Stats[USABLE_SIZE]++;
  else if (l == Sos)
    Stats[SOS_SIZE]++;
  else if (l == Demodulators)
    Stats[DEMODULATORS_SIZE]++;
}  /* prepend_cl */

/*************
 *
 *    insert_before_cl(c, c_new)
 *
 *************/

void insert_before_cl(struct clause *c,
		      struct clause *c_new)
{
  struct list *l;

  l = c->container;

  c_new->next_cl = c;
  c_new->prev_cl = c->prev_cl;
  c->prev_cl = c_new;
  if (!c_new->prev_cl)
    l->first_cl = c_new;
  else
    c_new->prev_cl->next_cl = c_new;

  c_new->container = l;

  if (l == Usable)
    Stats[USABLE_SIZE]++;
  else if (l == Sos)
    Stats[SOS_SIZE]++;
  else if (l == Demodulators)
    Stats[DEMODULATORS_SIZE]++;

}  /* insert_before_cl */

/*************
 *
 *    insert_after_cl(c, c_new)
 *
 *************/

void insert_after_cl(struct clause *c,
		     struct clause *c_new)
{
  struct list *l;

  l = c->container;

  c_new->prev_cl = c;
  c_new->next_cl = c->next_cl;
  c->next_cl = c_new;
  if (!c_new->next_cl)
    l->last_cl = c_new;
  else
    c_new->next_cl->prev_cl = c_new;

  c_new->container = l;

  if (l == Usable)
    Stats[USABLE_SIZE]++;
  else if (l == Sos)
    Stats[SOS_SIZE]++;
  else if (l == Demodulators)
    Stats[DEMODULATORS_SIZE]++;

}  /* insert_after_cl */

/*************
 *
 *    rem_from_list(c)
 *
 *************/

void rem_from_list(struct clause *c)
{
  struct clause *p, *n;

  p = c->prev_cl;
  n = c->next_cl;
  if (!n)
    c->container->last_cl = p;
  else
    n->prev_cl = p;
  if (!p)
    c->container->first_cl = n;
  else
    p->next_cl = n;

  if (c->container == Usable)
    Stats[USABLE_SIZE]--;
  else if (c->container == Sos)
    Stats[SOS_SIZE]--;
  else if (c->container == Demodulators)
    Stats[DEMODULATORS_SIZE]--;

  c->container = NULL;
  c->prev_cl = NULL;
  c->next_cl = NULL;
}  /* rem_from_list */

/*************
 *
 *    insert_clause(clause, *clause_ptr)
 *
 *    If not already there, insert clause into list of clause pointers.
 *
 *************/

void insert_clause(struct clause *c,
		   struct clause_ptr **cpp)
{
  struct clause_ptr *curr, *prev, *new;

  curr = *cpp;
  prev = NULL;
  while (curr && curr->c->id > c->id) {
    prev = curr;
    curr = curr->next;
  }
  if (!curr || curr->c->id != c->id) {
    new = get_clause_ptr();
    new->c = c;
    new->next = curr;
    if (prev)
      prev->next = new;
    else
      *cpp = new;
  }
}  /* insert_clause */

/*************
 *
 *   max_literal_weight()
 *
 *************/

int max_literal_weight(struct clause *c,
		       struct is_tree *wt_index)
{
  struct literal *lit;
  int wt, max;
  max = -INT_MAX;
  for (lit = c->first_lit; lit != NULL; lit = lit->next_lit) {
    if (!answer_lit(lit)) {
      wt = weight(lit->atom, wt_index);
      max = (wt > max ? wt : max);
    }
  }
  return(max);
}  /* max_literal_weight */

/*************
 *
 *    int weight_cl(c, wt_index)  --  Weigh a clause.
 *
 *    Also weigh answer lits, which have default weight 0.
 *
 *************/

int weight_cl(struct clause *c,
	      struct is_tree *wt_index)
{
  if (Flags[LITERALS_WEIGH_ONE].val)
    return num_literals(c);
  else {
    int w, neg_weight;
    struct literal *lit;

    neg_weight = Parms[NEG_WEIGHT].val;
    w = 0;
    lit = c->first_lit;
    while (lit) {
      w += weight(lit->atom, wt_index);
      if (!answer_lit(lit) && !lit->sign)
	w += neg_weight;
      lit = lit->next_lit;
    }
    return(w);
  }
}  /* weight_cl */

/*************
 *
 *    hide_clause(c) --  c must be integrated
 *
 *    Clauses can be hidden instead of deallocated so that they can
 *    be printed later on (mostly so that a child can know its parent).
 *
 *************/

void hide_clause(struct clause *c)
{
  c->next_cl = Hidden_clauses;
  Hidden_clauses = c;
}  /* hide_clause */

/*************
 *
 *   proof_last_hidden_empty()
 *
 *************/

struct clause *proof_last_hidden_empty(void)
{
  struct clause *c;
  struct clause *e = NULL;

  for (c = Hidden_clauses; c != NULL; c = c->next_cl) {
    if (num_literals(c) == 0)
      e = c;
  }
#if 0
  if (e != NULL) {
    printf("\n=========== HIDDEN PROOF ===========\n");
    print_proof(stdout, e);
    printf("\n=========== end hidden proof ===========\n");
  }
#endif
  return e;
}  /* proof_last_hidden_empty */

/*************
 *
 *    del_hidden_clauses() -- deallocate all hidden clauses
 *
 *************/

void del_hidden_clauses(void)
{
  struct clause *c;

  while (Hidden_clauses) {
    c = Hidden_clauses;
    Hidden_clauses = Hidden_clauses->next_cl;
    cl_del_int(c);
  }
}  /* del_hidden_clauses */

/*************
 *
 *    struct clause *cl_copy(c)
 *
 *    Do not copy the list of parents.
 *
 *************/

struct clause *cl_copy(struct clause *c)
{
  struct clause *d;
  struct literal *l, *l1, *l2;

  d = get_clause();
  d->type = c->type;
  l = c->first_lit;
  l2 = NULL;
  while (l) {
    l1 = get_literal();
    l1->target = l->target;
    l1->container = d;
    l1->sign = l->sign;
    l1->atom = copy_term(l->atom);
    remove_var_syms(l1->atom);  /* nullify variable symbols (if present) */
    l1->atom->occ.lit = l1;
    if (!l2)
      d->first_lit = l1;
    else
      l2->next_lit = l1;
    l2 = l1;
    l = l->next_lit;
  }
  return(d);
}  /* cl_copy */

/*************
 *
 *    clause_ident(c1, c2)
 *
 *    Don't check permutations.
 *
 *************/

int clause_ident(struct clause *c1,
		 struct clause *c2)
{
  struct literal *l1, *l2;
  int ok;

  for (l1 = c1->first_lit, l2 = c2->first_lit, ok = 1;
       l1 && l2 && ok;
       l1 = l1->next_lit, l2 = l2->next_lit)
    ok = (l1->sign == l2->sign && term_ident(l1->atom, l2->atom));

  return(ok && l1 == NULL && l2 == NULL);
}  /* cl_ident */

/*************
 *
 *    remove_var_syms(t)
 *
 *    Variable terms normally do not have sym_nums.  This
 *    routine removes any that are present.
 *
 *************/

void remove_var_syms(struct term *t)
{
  struct rel *r;

  if (t->type == VARIABLE)
    t->sym_num = 0;
  else if (t->type == COMPLEX)
    for (r = t->farg; r; r = r->narg)
      remove_var_syms(r->argval);
}  /* remove_var_syms */

/*************
 *
 *    cl_insert_tab(c)
 *
 *************/

void cl_insert_tab(struct clause *c)
{
  struct clause_ptr *cp1, *cp2, *cp3;
  int hashval, id;

  id = c->id;
  hashval = id % CLAUSE_TAB_SIZE;
  cp1 = Clause_tab[hashval];
  cp2 = NULL;

  /* keep the chains ordered--increasing id */

  while (cp1 && cp1->c->id < id) {
    cp2 = cp1;
    cp1 = cp1->next;
  }
  if (cp1 && cp1->c->id == id) {
    print_clause(stdout, c);
    abend("cl_insert_tab, clause already there.");
  }
  else {
    cp3 = get_clause_ptr();
    cp3->c = c;
    cp3->next = cp1;
    if (!cp2)
      Clause_tab[hashval] = cp3;
    else
      cp2->next = cp3;
  }
}  /* cl_insert_tab */

/*************
 *
 *    cl_delete_tab(c)
 *
 *************/

void cl_delete_tab(struct clause *c)
{
  struct clause_ptr *cp1, *cp2;
  int hashval, id;

  id = c->id;
  hashval = id % CLAUSE_TAB_SIZE;
  cp1 = Clause_tab[hashval];
  cp2 = NULL;

  /* chains are ordered--increasing id */

  while (cp1 && cp1->c->id < id) {
    cp2 = cp1;
    cp1 = cp1->next;
  }
  if (!cp1 || cp1->c->id != id) {
    print_clause(stdout, c);
    abend("cl_delete_tab, clause not found.");
  }
  else {
    if (!cp2)
      Clause_tab[hashval] = cp1->next;
    else
      cp2->next = cp1->next;
    free_clause_ptr(cp1);
  }
}  /* cl_delete_tab */

/*************
 *
 *    struct clause *cl_find(id)
 *
 *************/

struct clause *cl_find(int id)
{
  struct clause_ptr *cp1;
  int hashval;

  hashval = id % CLAUSE_TAB_SIZE;
  cp1 = Clause_tab[hashval];

  /* lists are ordered--increasing id */

  while (cp1 && cp1->c->id < id)
    cp1 = cp1->next;
  if (!cp1 || cp1->c->id != id)
    return(NULL);
  else
    return(cp1->c);
}  /* cl_find */

/*************
 *
 *     int lit_compare(l1, l2)
 *
 *     1. positive > negative
 *     2. answer > nonanswer
 *     3. lex
 *
 *************/

int lit_compare(struct literal *l1,
		struct literal *l2)
{
  if (l1->sign > l2->sign)
    return(GREATER_THAN);
  else if (l1->sign < l2->sign)
    return(LESS_THAN);
  else if (answer_lit(l1) && !answer_lit(l2))
    return(GREATER_THAN);
  else if (!answer_lit(l1) && answer_lit(l2))
    return(LESS_THAN);

  else {
    if (Flags[PROPOSITIONAL].val) {
      if (l1->atom->sym_num == l2->atom->sym_num)
	return(SAME_AS);
      else if (l1->atom->sym_num > l2->atom->sym_num)
	return(GREATER_THAN);
      else
	return(LESS_THAN);	    
    }
    else
      return(lex_order_vars(l1->atom, l2->atom));
  }
}  /* lit_compare */

/*************
 *
 *     int ordered_sub_clause(c, d)
 *
 *     True iff each literal of c occurs in d.
 *     Literals assumed to be ordered by lit_compare.
 *
 *     This routine treats any answer literals as ordinary literals.
 *     Although this might be considered a bug, I decided to write it
 *     this way, because it is designed for propositional clauses
 *     which usually don't have answer literals, and checking for
 *     answer literals will slow it too much.
 *
 *************/

int ordered_sub_clause(struct clause *c1,
		       struct clause *c2)
{
  struct literal *l1, *l2;
  int i;

  l1 = c1->first_lit;
  l2 = c2->first_lit;

  while (l1 && l2) {
    i = lit_compare(l1, l2);
    if (i == SAME_AS) {
      l1 = l1->next_lit;
      l2 = l2->next_lit;
    }
    else if (i == GREATER_THAN)
      l2 = l2->next_lit;
    else if (i == LESS_THAN)
      l2 = NULL;
    else
      abend("ordered_sub_clause: not total.");
  }

#if 0
  printf("\n c1:%d ", l1 == NULL); p_clause(c1);
  printf("\n c2:  "); p_clause(c2);
#endif    
  return(l1 == NULL);
}  /* ordered_sub_clause */

/*************
 *
 *     int sub_clause(c, d)
 *
 *     True iff each literal of c occurs in d.
 *     Literals are not assumed to be ordered.
 *
 *     This routine treats any answer literals as ordinary literals.
 *     Although this might be considered a bug, I decided to write it
 *     this way, because it is designed for propositional clauses
 *     which usually don't have answer literals, and checking for
 *     answer literals will slow it too much.
 *
 *************/

int sub_clause(struct clause *c1,
	       struct clause *c2)
{
  struct literal *l1, *l2;
  int found;

  for(l1 = c1->first_lit; l1; l1 = l1->next_lit) {
    l2 = c2->first_lit;
    found = 0;
    while (l2 && !found) {
      if (l1->sign == l2->sign && l1->atom->sym_num == l2->atom->sym_num)
	found = 1;
      else
	l2 = l2->next_lit;
    }
    if (!found)
      return(0);
  }
  return(1);
}  /* sub_clause */

/*************
 *
 *    sort_lits(c)  --  sort literals
 *
 *************/

int sort_lits(struct clause *c)
{
  struct literal *sorted, *prev, *curr, *next, *insert;
  int changed = 0;

  /* This is an insertion sort.  Use lit_compare */

  if (c->first_lit) {
    sorted = c->first_lit;
    insert = sorted->next_lit;
    sorted->next_lit = NULL;

    while(insert) {
      prev = NULL;
      curr = sorted;
      while (curr && lit_compare(insert, curr) == GREATER_THAN) {
	prev = curr;
	curr = curr->next_lit;
      }
      if (curr != NULL)
	changed = 1;
      next = insert->next_lit;
      insert->next_lit = curr;
      if (prev)
	prev->next_lit = insert;
      else
	sorted = insert;
      insert = next;
    }
    c->first_lit = sorted;
  }
  return changed;
}  /* sort lits */

/*************
 *
 *    all_cont_cl(t, cpp) - insert containing clauses of t into *cpp
 *
 *************/

void all_cont_cl(struct term *t,
		 struct clause_ptr **cpp)
{
  struct rel *r;
  struct clause *c;
  struct list *l;

  if (t->type != VARIABLE && t->varnum != 0) {  /* atom */
    c = t->occ.lit->container;
    l = c->container;
    if (l == Usable || l == Sos || l == Demodulators)
      insert_clause(c, cpp);
  }
  else {  /* term */
    r = t->occ.rel;
    while (r) {
      all_cont_cl(r->argof, cpp);
      r = r->nocc;
    }
  }
}  /* all_cont_cl */

/*************
 *
 *    zap_cl_list(lst)
 *
 *************/

void zap_cl_list(struct list *lst)
{
  struct clause *c1, *c2;

  c1 = lst->first_cl;
  while (c1) {
    c2 = c1;
    c1 = c1->next_cl;
    cl_del_non(c2);
  }
  free_list(lst);
}  /* zap_cl_list */

/*************
 *
 *   is_eq()
 *
 *************/

int is_eq(int sym_num)
{
  char *name;

  name = sn_to_str(sym_num);

  if (Flags[TPTP_EQ].val)
    return(sn_to_arity(sym_num) == 2 && str_ident("equal", name));
  else
    return(sn_to_arity(sym_num) == 2 &&
	   (initial_str("EQ", name) ||
	    initial_str("Eq", name) ||
	    initial_str("eq", name) ||
	    str_ident("=", name)));
}  /* is_eq */

/*************
 *
 *    mark_literal(lit)
 *
 *    Atoms have varnum > 0.  This routine inserts the appropriate code.
 *
 *************/

void mark_literal(struct literal *lit)
{
  char *name;
  struct term *a;

  a = lit->atom;

  name = sn_to_str(a->sym_num);

  if (initial_str("$ANS", name) || initial_str("$Ans", name) ||
      initial_str("$ans", name))
    a->varnum = ANSWER;  /* answer literal */

  else if (is_eq(a->sym_num)) {
    if (lit->sign)
      a->varnum = POS_EQ;  /* positive equality */
    else
      a->varnum = NEG_EQ;  /* negative equality */
  }

  else if (sn_to_ec(a->sym_num) > 0)
    a->varnum = EVALUABLE;  /* $ID, $LE, $AND, ... */

  else if (is_symbol(a, "->", 2) && is_eq(a->farg->narg->argval->sym_num))
    a->varnum = CONDITIONAL_DEMOD;

  else
    a->varnum = NORM_ATOM;  /* normal atom */

}  /* mark_literal */

/*************
 *
 *    int get_ancestors(c, cpp, ipp)
 *
 *    cpp is the list under construction, sorted by ID.
 *    ipp is a corresponding list of the level of each clause.
 *
 *    Return the level of the proof.
 *
 *  The justification list of a clause (c->parents) is a list
 *  of integers.  Usually, negative integers represent inference
 *  rules, and positive integers are the IDs of parent clauses.
 *  Exception:  LIST_RULE is a large negative integer.  If a member
 *  is <= LIST_RULE, then a list of length (LIST_RULE-member) follows
 *  (and typically represents a position in a clause).
 *
 *************/

int get_ancestors(struct clause *c,
		  struct clause_ptr **cpp,
		  struct ilist **ipp)
{
  struct clause_ptr *cp1, *cp2, *cp3;
  struct ilist *ip1, *ip2, *ip3;
  struct ilist *ip;
  struct clause *d;
  int max, lev, n;

  cp1 = *cpp; ip1 = *ipp;
  cp3 = NULL; ip3 = NULL;
  /* First check to see if the clause has already been processed. */
  while (cp1 && cp1->c->id < c->id) {
    cp3 = cp1; cp1 = cp1->next;
    ip3 = ip1; ip1 = ip1->next;
  }
  if (!cp1 || cp1->c->id > c->id) {
    /*  Process the clause. */
    cp2 = get_clause_ptr();
    ip2 = get_ilist();
    cp2->c = c;
    if (!cp3) {
      cp2->next = *cpp; *cpp = cp2;
      ip2->next = *ipp; *ipp = ip2;
    }
    else {
      cp2->next = cp3->next; cp3->next = cp2;
      ip2->next = ip3->next; ip3->next = ip2;
    }

    max = -1;
    for (ip = c->parents; ip; ip = ip->next) {

      if (ip->i <= LIST_RULE) {
	/* LIST_RULE is a large negative number. */
	/* If ip->i is less than LIST_RULE, then a list follows. */
	int i;
	int j = LIST_RULE - ip->i;  /* size of list */
	/* Make ip point at the last element of the list. */
	for (i = 1; i <= j; i++)
	  ip = ip->next;
      }

      else if (ip->i >= 0) {
	/* < 0 means it's a code for an inference rule */
	d = cl_find(ip->i);
	if (!d)
	  printf("WARNING, clause %d not found, proof is incomplete.\n", ip->i);
			   
	else {
	  n = get_ancestors(d, cpp, ipp);
	  max = (n > max ? n : max);
	}
      }
    }

    if (!c->parents)
      lev = 0;
    else if (c->parents->i == NEW_DEMOD_RULE)
      lev = max;
    else
      lev = max + 1;
    ip2->i = lev;
#if 0
    printf("level %d: ", lev); p_clause(c);
#endif
    return(lev);
  }
  else {
    /* The clause has already been processed, so just return its level. */
    return(ip1->i);
  }
}  /* get_ancestors */

/*************
 *
 *   clauses_to_ids()
 *
 *************/

struct ilist *clauses_to_ids(struct clause_ptr *p)
{
  if (p == NULL)
    return NULL;
  else {
    struct ilist *ip = get_ilist();
    ip->i = p->c->id;
    ip->next = clauses_to_ids(p->next);
    return ip;
  }
}  /* clauses_to_ids */

/*************
 *
 *   free_clause_ptr_list()
 *
 *************/

void free_clause_ptr_list(struct clause_ptr *p)
{
  if (p != NULL) {
    free_clause_ptr_list(p->next);
    free_clause_ptr(p);
  }
}  /* free_clause_ptr_list */

/*************
 *
 *    get_ancestors2(c)
 *
 *    This is a more convenient version of get_ancestors.
 *
 *************/

struct ilist *get_ancestors2(struct clause *c)
{
  struct clause_ptr *cp = NULL;
  struct ilist *ip = NULL;
  int level;

  level = get_ancestors(c, &cp, &ip);
  free_ilist_list(ip);
  ip = clauses_to_ids(cp);
  free_clause_ptr_list(cp);
  return ip;
}  /* get_ancestors2 */

/*************
 *
 *    int just_to_supporters(ip)
 *
 *    Given a justification list, return a set of the parents.
 *
 *************/

struct ilist *just_to_supporters(struct ilist *ip)
{
  struct ilist *ip_construct = NULL;

  for ( ; ip; ip = ip->next) {

    if (ip->i <= LIST_RULE) {
      /* LIST_RULE is a large negative number. */
      /* If ip->i is less than LIST_RULE, then a list follows. */
      int i;
      int j = LIST_RULE - ip->i;  /* size of list */
      /* Make ip point at the last element of the list. */
      for (i = 1; i <= j; i++)
	ip = ip->next;
    }

    else if (ip->i >= 0) {
      /* < 0 means it's a code for an inference rule */
#if 1
      ip_construct = iset_add(ip->i, ip_construct);
#else
      struct ilist *ip_new = get_ilist();
      ip_new->i = ip->i;
      ip_new->next = ip_construct;
      ip_construct = ip_new;
#endif
    }
  }
#if 1
  return ip_construct;
#else
  return reverse_ip(ip_construct, NULL);
#endif
}  /* just_to_supporters */

/*************
 *
 *    int renumber_vars_term(c)
 *
 *        Renumber the variables of a term, starting with 0.  `c' must
 *    be nonintegrated.  return(0) if more than MAXVARS distinct variables.
 *
 *    This is very special-purpose.  Ordinarily, you'll call renumber_vars(c)
 *    on clauses.
 *
 *************/

int renumber_vars_term(struct term *t)
{
  int varnums[MAX_VARS];
  int i;

  for (i = 0; i < MAX_VARS; i++)
    varnums[i] = -1;

  return renum_vars_term(t, varnums);

}  /* renumber_vars_term */

/*************
 *
 *    int renumber_vars(c)
 *
 *        Renumber the variables of a clause, starting with 0.  `c' must
 *    be nonintegrated.  return(0) if more than MAXVARS distinct variables.
 *
 *************/

int renumber_vars(struct clause *c)
{
  struct literal *lit;
  int varnums[MAX_VARS];
  int i, ok;

  ok = 1;
  for (i = 0; i < MAX_VARS; i++)
    varnums[i] = -1;

  lit = c->first_lit;
  while (lit) {
    if (renum_vars_term(lit->atom, varnums) == 0)
      ok = 0;
    lit = lit->next_lit;
  }

  return(ok);

}  /* renumber_vars */

/*************
 *
 *    int renum_vars_term(term, varnums) -- called from renumber_vars.
 *
 *************/

int renum_vars_term(struct term *t,
		    int *varnums)
{
  struct rel *r;
  int i, ok;

  if (t->type == NAME)
    return(1);
  else if (t->type == COMPLEX) {
    ok = 1;
    r = t->farg;
    while (r) {
      if (renum_vars_term(r->argval, varnums) == 0)
	ok = 0;
      r = r->narg;
    }
    return(ok);
  }
  else {
    i = 0;
    while (i < MAX_VARS && varnums[i] != -1 && varnums[i] != t->varnum)
      i++;
    if (i == MAX_VARS)
      return(0);
    else {
      if (varnums[i] == -1) {
	varnums[i] = t->varnum;
	t->varnum = i;
      }
      else
	t->varnum = i;
      return(1);
    }
  }
}  /* renum_vars_term */

/*************
 *
 *    clear_var_names(t) -- set sym_num field of all variables to NULL
 *
 *************/

void clear_var_names(struct term *t)
{
  struct rel *r;

  if (t->type == VARIABLE)
    t->sym_num = 0;
  else {
    for (r = t->farg; r; r = r->narg)
      clear_var_names(r->argval);
  }
}  /* clear_var_names */

/*************
 *
 *    cl_clear_vars(c)
 *
 *************/

void cl_clear_vars(struct clause *c)
{
  struct literal *lit;

  for (lit = c->first_lit; lit; lit = lit->next_lit)
    clear_var_names(lit->atom);
}

/*************
 *
 *    void distinct_vars_rec(t, a, max) -- called by distinct_vars
 *
 *************/

static void distinct_vars_rec(struct term *t,
			      int *a,
			      int *max)
{
  struct rel *r;
  int i, vn;

  if (t->type == VARIABLE) {
    vn = t->varnum;
    for (i = 0; i < MAX_VARS && a[i] != -1 && a[i] != vn; i++);
    if (i != MAX_VARS && a[i] == -1) {
      a[i] = vn;
      *max = i+1;
    }
  }
  else if (t->type == COMPLEX) {
    for (r = t->farg; r && *max < MAX_VARS; r = r->narg)
      distinct_vars_rec(r->argval, a, max);
  }
}  /* distinct_vars_rec */

/*************
 *
 *    int distinct_vars(c) -- number of variables in a clause.
 *
 *    if >= MAX_VARS, return MAX_VARS.
 *
 *************/

int distinct_vars(struct clause *c)
{
  struct literal *lit;
  int a[MAX_VARS], i, max;

  for (i = 0; i < MAX_VARS; i++)
    a[i] = -1;

  for (lit = c->first_lit, max = 0; lit; lit = lit->next_lit)
    distinct_vars_rec(lit->atom, a, &max);

  return(max);

}  /* distinct_vars */

/*************
 *
 *    struct clause *find_first_cl(l)
 *
 *************/

struct clause *find_first_cl(struct list *l)
{
  struct clause *c;

  if (!l->first_cl)
    return(NULL);
  else {
    c = l->first_cl;
    return(c);
  }
}  /* find_first_cl */

/*************
 *
 *    struct clause *find_last_cl(l)
 *
 *************/

struct clause *find_last_cl(struct list *l)
{
  struct clause *c;

  if (!l->last_cl)
    return(NULL);
  else {
    c = l->last_cl;
    return(c);
  }
}  /* find_last_cl */

/*************
 *
 *    struct clause *find_random_cl(l)
 *
 *************/

struct clause *find_random_cl(struct list *l)
{
  struct clause *c;
  int i, j;

  if (l->first_cl == NULL)
    return(NULL);
  else {
    j = (rand() % Stats[SOS_SIZE]) + 1;
    c = l->first_cl;
    i = 1;
    while (i < j && c != NULL) {
      c = c->next_cl;
      i++;
    }
    if (c == NULL)
      abend("find_random_cl, sos bad.");
    return(c);
  }
}  /* find_random_cl */

/*************
 *
 *   get_clauses_of_wt_range()
 *
 *************/

struct clause_ptr *get_clauses_of_wt_range(struct clause *c,
					   int min, int max)
{
  if (c == NULL)
    return NULL;
  else if (c->pick_weight >= min && c->pick_weight <= max) {
    struct clause_ptr *p = get_clause_ptr();
    p->c = c;
    p->next = get_clauses_of_wt_range(c->next_cl, min, max);
    return p;
  }
  else
    return get_clauses_of_wt_range(c->next_cl, min, max);
}  /* get_clauses_of_wt_range */

/*************
 *
 *   clause_ptr_list_size()
 *
 *************/

int clause_ptr_list_size(struct clause_ptr *p)
{
  if (p == NULL)
    return 0;
  else
    return (1 + clause_ptr_list_size(p->next));
}  /* clause_ptr_list_size */

/*************
 *
 *   nth_clause() -- this counts from 1.
 *
 *************/

struct clause *nth_clause(struct clause_ptr *p, int n)
{
  if (p == NULL)
    return NULL;
  else if (n == 1)
    return p->c;
  else
    return nth_clause(p->next, n-1);
}  /* nth_clause */

/*************
 *
 *   zap_clause_ptr_list(p)
 *
 *   Free the nodes, but not the clauses they point to.
 *
 *************/

void zap_clause_ptr_list(struct clause_ptr *p)
{
  if (p != NULL) {
    zap_clause_ptr_list(p->next);
    free_clause_ptr(p);
  }
}  /* zap_clause_ptr_list */

/*************
 *
 *    struct clause *find_random_lightest_cl(l)
 *
 *************/

struct clause *find_random_lightest_cl(struct list *l)
{
  struct clause *c = find_lightest_cl(l);
  if (c == NULL)
    return NULL;
  else {
    int wt = c->pick_weight;
    struct clause_ptr *p = get_clauses_of_wt_range(l->first_cl, wt, wt);
    int n = clause_ptr_list_size(p);
    int j = (rand() % n) + 1;

    c = nth_clause(p, j);
    zap_clause_ptr_list(p);
    return c;
  }
}  /* find_random_lightest_cl */

/*************
 *
 *    struct clause *find_mid_lightest_cl(l)
 *
 *************/

struct clause *find_mid_lightest_cl(struct list *l)
{
  struct clause *c = find_lightest_cl(l);
  if (c == NULL)
    return NULL;
  else {
    int wt = c->pick_weight;
    struct clause_ptr *p = get_clauses_of_wt_range(l->first_cl, wt, wt);
    int n = clause_ptr_list_size(p);
    int j = (n / 2) + 1;

    c = nth_clause(p, j);
    zap_clause_ptr_list(p);
    return c;
  }
}  /* find_mid_lightest_cl */

/*************
 *
 *    struct clause *find_lightest_cl(l)
 *
 *    If more than one of mimimum weight, return first or last of those,
 *    according to the flag PICK_LAST_LIGHTEST.
 *
 *    Input sos clauses might have weight field set to -MAX_INT so that
 *    they are returned first (in order).
 *
 *************/

struct clause *find_lightest_cl(struct list *l)
{
  if (l->first_cl == NULL)
    return NULL;
  else {
    struct clause *cm = l->first_cl;
    int wm = cm->pick_weight;
    struct clause *c = cm->next_cl;
    while (c) {
      int w = c->pick_weight;
      if (Flags[PICK_LAST_LIGHTEST].val ? (w <= wm) : (w < wm)) {
	wm = w;
	cm = c;
      }
      c = c->next_cl;
    }
    return cm;
  }
}  /* find_lightest_cl */

/*************
 *
 *    struct clause *find_lightest_geo_child(l)
 *
 *    Find the lightest clause c for which child_of_geometry(c) holds.
 *    If there are no children of geometry, return NULL.
 *    If more than one of mimimum weight, return first of those.
 *
 *************/

struct clause *find_lightest_geo_child(struct list *l)
{
  struct clause *c, *cmin;
  int w, wmin;

  for (c = l->first_cl, cmin = NULL, wmin = MAX_INT; c; c = c->next_cl) {
    if (child_of_geometry(c)) {
      w = c->pick_weight;
      if (!cmin || w < wmin) {
	wmin = w;
	cmin = c;
      }
    }
  }
  return(cmin);
}  /* find_lightest_geo_child */

/*************
 *
 *    struct clause *find_interactive_cl()
 *
 *************/

struct clause *find_interactive_cl(void)
{
  FILE *fin, *fout;

  fin  = fopen("/dev/tty", "r");
  fout = fopen("/dev/tty", "w");

  if (!fin || !fout) {
    printf("interaction failure: cannot find tty.\n");
    fprintf(stderr, "interaction failure: cannot find tty.\n");
    return(NULL);
  }
  else {
    int id;
    struct clause *c = NULL;
    int done = 0;
    char s[256];

    while (!done) {
      fprintf(fout,"\nEnter clause number of next given clause, or 0 to terminate\ninteractive_given mode, or -1 to print list sos.\n? ");
      fscanf(fin, "%s", s);
      if (!str_int(s, &id)) {
	fprintf(fout, "%c\nNot an integer: \"%s\", try again.\n", Bell, s);
      }
      else if (id == 0) {
	printf("\nTurning off interactive_given mode.\n");
	fprintf(fout, "\nTurning off interactive_given mode.\n");
	Flags[INTERACTIVE_GIVEN].val = 0;
	c = NULL;
	done = 1;
      }
      else if (id == -1) {
	struct clause *c;
	for (c = Sos->first_cl; c; c = c->next_cl)
	  print_clause(fout, c);
      }
      else {
	c = cl_find(id);
	if (!c)
	  fprintf(fout, "%c\nClause %d not found.\n", Bell, id);
	else if (c->container != Sos)
	  fprintf(fout, "%c\nClause %d not in sos.\n", Bell, id);
	else {
	  done = 1;
	  fprintf(fout, "\nOk, clause %d will be given.\n", id);
	}
      }
    }
    fclose(fin);
    fclose(fout);
    return(c);
  }
}  /* find_interactive_cl */

/*************
 *
 *    struct clause *find_given_clause()
 *
 *************/

struct clause *find_given_clause(void)
{
  struct clause *giv_cl;

  if (Flags[INTERACTIVE_GIVEN].val) {
    giv_cl = find_interactive_cl();
    if (giv_cl)
      return(giv_cl);
  }
  if (Flags[SOS_QUEUE].val)
      giv_cl = find_first_cl(Sos);

  else if (Flags[SOS_STACK].val)
    giv_cl = find_last_cl(Sos);

  else if (Parms[PICK_GIVEN_RATIO].val != -1 &&
	   Stats[CL_GIVEN] % (Parms[PICK_GIVEN_RATIO].val + 1) == 0)
    giv_cl = find_first_cl(Sos);

  else if (Parms[PICK_DIFF].val != -1)
    giv_cl = find_pickdiff_cl(Sos, Usable);

  else if (Flags[PICK_RANDOM_LIGHTEST].val)
    giv_cl = find_random_lightest_cl(Sos);

  else if (Flags[PICK_MID_LIGHTEST].val)
    giv_cl = find_mid_lightest_cl(Sos);

  else if (Parms[GEO_GIVEN_RATIO].val != -1) {
    if (Stats[CL_GIVEN] % (Parms[GEO_GIVEN_RATIO].val + 1) == 0) {
      giv_cl = find_lightest_geo_child(Sos);
      if (!giv_cl)
	giv_cl = find_lightest_cl(Sos);
    }
    else
      giv_cl = find_lightest_cl(Sos);
  }

  else  /* this is the default */
    giv_cl = find_lightest_cl(Sos);

  return(giv_cl);
}  /* find_given_clause */

/*************
 *
 *    struct clause *extract_given_clause()
 *
 *************/

struct clause *extract_given_clause(void)
{
  struct clause *giv_cl;

  CLOCK_START(PICK_GIVEN_TIME);
  giv_cl = find_given_clause();
  if (giv_cl) {
    rem_from_list(giv_cl);
  }
  CLOCK_STOP(PICK_GIVEN_TIME);
  return(giv_cl);
}  /* extract_given_clause */

/*************
 *
 *    int unit_del(c)  --  unit deletion
 *
 *    Delete any literals that are subsumed by a unit with opposite sign.
 *
 *    Answer literals on the units complicate things.  In particular, 
 *    if they contain variables not in the regular literal.
 *
 *    This assumes that FOR_SUB_FPA is clear, because the discrimination
 *    index is used.
 *
 *    Return 1 if any deletions occur.
 *
 *************/

int unit_del(struct clause *c)
{
  struct clause *d;
  struct literal *prev, *curr, *next, *answers, *l1, *l2;
  struct term *d_atom;
  struct context *s;
  struct is_tree *is_db;
  struct fsub_pos *pos;
  struct ilist *ip0, *ip, *lp;
  int deleted, return_val;

  return_val = 0;
  s = get_context();
  s->multiplier = 1;

  /* first get last parent */
  lp = c->parents;
  if (lp)
    while (lp->next)
      lp = lp->next;

  ip0 = lp;  /* save position to insert "ud" if any deleted */

  answers = NULL;
  prev = NULL;
  next = c->first_lit;
  while (next) {
    curr = next;
    next = next->next_lit;
    is_db = curr->sign ? Is_neg_lits : Is_pos_lits;
    d_atom = fs_retrieve(curr->atom, s, is_db, &pos);
    deleted = 0;
    while (d_atom && !deleted) {
      d = d_atom->occ.lit->container;
      if (d->container != Passive && num_literals(d) == 1) {
	return_val = 1;
	if (prev)
	  prev->next_lit = next;
	else
	  c->first_lit = next;
	ip = get_ilist();  /* append to history */
	ip->i = d->id;
	if (!lp)
	  c->parents = ip;
	else
	  lp->next = ip;
	lp = ip;

	l2 = d->first_lit;  /* now append any answer literals */
	while (l2) {
	  if (answer_lit(l2)) {
	    l1 = get_literal();
	    l1->container = c;
	    l1->sign = l2->sign;
	    l1->atom = apply(l2->atom, s);
	    s->multiplier++;  /* in case answer has lone vars */
	    l1->atom->varnum = ANSWER;
	    l1->atom->occ.lit = l1;
	    l1->next_lit = answers;
	    answers = l1;
	  }
	  l2 = l2->next_lit;
	}

	curr->atom->occ.lit = NULL;  /* so zap_term won't complain */
	zap_term(curr->atom);
	free_literal(curr);

	canc_fs_pos(pos, s);
	Stats[UNIT_DELETES]++;
	deleted = 1;
      }
      else
	d_atom = fs_retrieve((struct term *) NULL, s, is_db, &pos);
    }
    if (!deleted)
      prev = curr;
  }
  if (!prev)
    c->first_lit = answers;
  else
    prev->next_lit = answers;
  if (lp != ip0) {  /* at least one deletion occurred */
    if (s->multiplier != 1) {
      /* Answer lits added; renumber in case new vars introduced. */
      if (renumber_vars(c) == 0) {
	print_clause(stdout, c);
	abend("unit_del, too many variables introduced.");
      }
    }
    ip = get_ilist();
    ip->i = UNIT_DEL_RULE;
    if (!ip0) {
      ip->next = c->parents;
      c->parents = ip;
    }
    else {
      ip->next = ip0->next;
      ip0->next = ip;
    }
  }
  free_context(s);
  return(return_val);
}  /* unit_del */

/*************
 *
 *   back_unit_deletion()
 *
 *************/

void back_unit_deletion(struct clause *c,
			int input)
{
  struct clause *d, *resolvent;
  struct literal *c_lit;
  struct term *c_atom, *d_atom;
  struct context *c_subst, *d_subst;
  struct fpa_tree *ut;
  struct trail *tr;
  struct list *source_list;

  c_lit = ith_literal(c,1);
  c_atom = c_lit->atom;

  ut = build_tree(c_lit->atom, INSTANCE, Parms[FPA_LITERALS].val,
		  c_lit->sign ? Fpa_neg_lits : Fpa_pos_lits);

  d_atom = next_term(ut, 0);
  c_subst = get_context();
  d_subst = get_context();  /* This will stay empty */

  while (d_atom) {
    d = d_atom->occ.lit->container;
    source_list = d->container;
    tr = NULL;
    if (source_list == Usable || source_list == Sos) {
      if (match(c_atom, c_subst, d_atom, &tr)) {
	resolvent = build_bin_res(c_atom, c_subst, d_atom, d_subst);
	resolvent->parents->i = BACK_UNIT_DEL_RULE;
	clear_subst_1(tr);
	Stats[CL_GENERATED]++;
	Stats[BACK_UNIT_DEL_GEN]++;

#if 1
	if (source_list == Usable) {
	  SET_BIT(resolvent->bits, SCRATCH_BIT);
	  /* printf("Clause destined for Usable:\n"); */
	  p_clause(resolvent);
	}
#endif

	CLOCK_STOP(BACK_UNIT_DEL_TIME);
        pre_process(resolvent, input, Sos);
	CLOCK_START(BACK_UNIT_DEL_TIME);
      }
    }
    d_atom = next_term(ut, 0);
  }
  free_context(c_subst);
  free_context(d_subst);
  
}  /* back_unit_deletion */
