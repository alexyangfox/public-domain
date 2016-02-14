/*
 *  share.c -- routines to manage the shared data structures
 *
 */

#include "header.h"

#define TERM_TAB_SIZE 3793

/* Hash table of shared terms */

static struct term_ptr *Term_tab[TERM_TAB_SIZE];

/* alphas and betas of pos eq lits are not shared, so they are put here */
/* so that back dedmodulation can find them                             */

static struct term_ptr *Bd_kludge;

/*************
 *
 *    int hash_term(term)
 *
 *        Return the hash value of a term.  It is assumed that
 *    the subterms are already integrated.  The hash value is
 *    constructed from the function symbol and the addresses of the
 *    subterms.
 *
 *************/

static int hash_term(struct term *t)
{
  struct rel *r;
  int hashval;

  if (t->type == NAME)   /* name */
    hashval = t->sym_num;
  else if (t->type == VARIABLE)  /* variable */
    hashval = t->sym_num + t->varnum + 25;
  else {  /* complex */
    hashval = t->sym_num;
    r = t->farg;
    while (r != NULL) {
      hashval >>= 1;  /* shift right one bit */
      hashval ^= (int) r->argval; /* exclusive or */
      r = r->narg;
    }
  }
  return(abs(hashval) % TERM_TAB_SIZE);
}  /* hash_term */

/*************
 *
 *    int term_compare(t1, t2)
 *
 *        Special purpose term comparison for term integration.
 *    Succeed iff functors are identical and subterm pointers are
 *    identical.  (Note that this routine is not recursive.)
 *    (For general purpose routine, see `term_ident'.)
 *
 *************/

static int term_compare(struct term *t1,
			struct term *t2)
{
  struct rel *r1, *r2;

  if (t1->type != t2->type)
    return(0);
  else if (t1->type == NAME) /* name */
    return(t1->sym_num == t2->sym_num);
  else if (t1->type == VARIABLE) /* variable */
    return(t1->varnum == t2->varnum && t1->sym_num == t2->sym_num);
  else if (t1->sym_num != t2->sym_num)
    return(0);  /* both complex with different functors */
  else {
    r1 = t1->farg;
    r2 = t2->farg;
    while (r1 != NULL && r2 != NULL) {
      if (r1->argval != r2->argval)
	return(0);
      else {
	r1 = r1->narg;
	r2 = r2->narg;
      }
    }
    return(r1 == NULL && r2 == NULL);
  }
}  /* term_compare */

/*************
 *
 *    struct term *integrate_term(term)
 *
 *        Incorporate a term into the shared data structures.
 *    Either ther term itself is integrated and returned, or
 *    the term is deallocated and a previously integrated copy
 *    is returned.  A good way to invoke it is
 *
 *           t = integrate(t)
 *
 *************/

struct term *integrate_term(struct term *t)
{
  int hashval, found;
  struct term_ptr *p, *prev;
  struct rel *r, *r2;

  if (t->type == COMPLEX) {  /* complex term */
    r = t->farg;
    while (r != NULL) {
      r->argval = integrate_term(r->argval);
      r = r->narg;
    }
  }

  hashval = hash_term(t);

  p = Term_tab[hashval];
  prev = NULL;

  found = 0;
  while (found == 0 && p != NULL)
    if (term_compare(t, p->term))
      found = 1;
    else {
      prev = p;
      p = p->next;
    }

  if (found) {    /* p->term is integrated copy */
    if (t == p->term) {
      print_term_nl(stdout, t);
      abend("integrate_term, already integrated.");
    }
    if (t->type == COMPLEX) { /* if complex, free rels */
      r = t->farg;
      while (r != NULL) {
	r2 = r;
	r = r->narg;
	free_rel(r2);
      }
    }
    free_term(t);
    return(p->term);
  }
  else {    /* not in bucket, so insert at end of bucket and return */
    if (t->type == COMPLEX) { /* if complex, set up containment lists */
      r = t->farg;
      while (r != NULL) {
	r->argof = t;
	r->nocc = r->argval->occ.rel;
	r->argval->occ.rel = r;
	r = r->narg;
      }
    }
    p = get_term_ptr();
    p->term = t;
    p->next = NULL;
    if (prev == NULL)
      Term_tab[hashval] = p;
    else
      prev->next = p;

    if (Flags[BACK_DEMOD].val && Flags[INDEX_FOR_BACK_DEMOD].val)
      fpa_insert(t, Parms[FPA_TERMS].val, Fpa_back_demod);
    return(t);
  }
}  /* integrate_term */

/*************
 *
 *    disintegrate_term(term)
 *
 *       Remove a previously integrated term from the shared data
 *    structures.  A warning is printed if the term has a list of
 *    superterms.
 *
 *************/

void disintegrate_term(struct term *t)
{
  int hashval;
  struct rel *r1, *r2, *r3;
  struct term_ptr *p1, *p2;

  if (t->occ.rel != NULL) {
    fprintf(stderr, "WARNING, disintegrate_term, contained term.\n");
    printf("WARNING, disintegrate_term, contained term: ");
    print_term_nl(stdout, t);
  }
  else {
    hashval = hash_term(t);
    p1 = Term_tab[hashval];
    p2 = NULL;

    while (p1 != NULL && p1->term != t) {
      p2 = p1;
      p1 = p1->next;
    }
    if (p1 == NULL)
      abend("disintegrate_term, cannot find term.");
    else {
      if (p2 == NULL)
	Term_tab[hashval] = p1->next;
      else
	p2->next = p1->next;
      free_term_ptr(p1);
      if (Flags[BACK_DEMOD].val && Flags[INDEX_FOR_BACK_DEMOD].val) {
	CLOCK_START(UN_INDEX_TIME);
	fpa_delete(t, Parms[FPA_TERMS].val, Fpa_back_demod);
	CLOCK_STOP(UN_INDEX_TIME);
      }
      if (t->type == COMPLEX) {
	r1 = t->farg;
	while (r1 != NULL) {
	  r2 = r1->argval->occ.rel;
	  r3 = NULL;
	  while (r2 != NULL && r2 != r1) {
	    r3 = r2;
	    r2 = r2->nocc;
	  }
	  if (r2 == NULL) {
	    print_term_nl(stdout, t);
	    abend("disintegrate_term, bad containment.");
	  }
	  else {
	    if (r3 == NULL)
	      r1->argval->occ.rel = r1->nocc;
	    else
	      r3->nocc = r1->nocc;
	    if (r1->argval->occ.rel == NULL)
	      disintegrate_term(r2->argval);
	  }
	  r3 = r1;
	  r1 = r1->narg;
	  free_rel(r3);
	}
      }
      free_term(t);
    }
  }
}  /* disintegrate_term */

/*************
 *
 *   set_up_pointers(t)
 *
 *************/

void set_up_pointers(struct term *t)
{
  struct rel *r;

  for (r = t->farg; r; r = r->narg) {
    r->argof = t;
    r->argval->occ.rel = r;
    set_up_pointers(r->argval);
  }
}  /* set_up_pointers */


/*************
 *
 *    zap_term(term)
 *
 *        Deallocate a nonshared term.  A warning is printed
 *    the term or any of its subterms contains a list of superterms.
 *
 *************/

void zap_term(struct term *t)
{
  struct rel *r1, *r2;

  if (t->occ.rel != NULL) {
    fprintf(stderr, "WARNING, zap_term, contained term.\n");
    printf("WARNING, zap_term, contained term: ");
    print_term_nl(stdout, t);
    printf("\n");
  }
  else {
    if (t->type == COMPLEX) { /* complex term */
      r1 = t->farg;
      while (r1 != NULL) {
	zap_term(r1->argval);
	r2 = r1;
	r1 = r1->narg;
	free_rel(r2);
      }
    }
    free_term(t);
  }
}  /* zap_term */

/*************
 *
 *    print_term_tab(file_ptr) -- Print the table of integrated terms.
 *
 *************/

void print_term_tab(FILE *fp)
{
  int i;
  struct term_ptr *p;

  for(i=0; i<TERM_TAB_SIZE; i++)
    if(Term_tab[i] != NULL) {
      fprintf(fp, "bucket %d: ",i);
      p = Term_tab[i];

      while(p != NULL) {
	print_term(fp, p->term);
	fprintf(fp, " ");
	p = p->next;
      }
      fprintf(fp, "\n");
    }
}  /* print_term_tab */

/*************
 *
 *    p_term_tab()
 *
 *************/

void p_term_tab(void)
{
  print_term_tab(stdout);
}  /* p_term_tab */

/*************
 *
 *    test_terms(file_ptr)
 *
 *        Print the list of integrated terms.  For each term, list its
 *    subterms and superterms.
 *
 *************/

void test_terms(FILE *fp)
{
  int i;
  struct term_ptr *p;
  struct rel *r;

  for(i=0; i<TERM_TAB_SIZE; i++)
    if(Term_tab[i] != NULL) {
      fprintf(fp, "    bucket %d:\n",i);
      p = Term_tab[i];
      while(p != NULL) {
	print_term(fp, p->term);
	fprintf(fp, " containing terms: ");
	r = p->term->occ.rel;
	while (r != NULL) {
	  print_term(fp, r->argof);
	  fprintf(fp, " ");
	  r = r->nocc;
	}
	fprintf(fp, "\n");
	p = p->next;
      }
    }
}  /* test_terms */

/*************
 *
 *    struct term_ptr *all_instances(atom)
 *
 *    Get all terms (in table of shared terms) that can be rewritten
 *    with demodulator (atom).  Handles lex-dependent demod correctly.
 *
 *************/

struct term_ptr *all_instances(struct term *atom)
{
  struct term *alpha, *beta, *t;
  struct term_ptr *tp, *tp1, *instances;
  struct context *subst;
  struct trail *tr;
  int i, lex_dependent, ok;

  alpha = atom->farg->argval;
  beta = atom->farg->narg->argval;
  lex_dependent = (atom->varnum == LEX_DEP_DEMOD);
  instances = NULL;
  subst = get_context();
  subst->multiplier = 1;
  for (i = 0; i <= TERM_TAB_SIZE; i++) {
    tp = (i == TERM_TAB_SIZE ? Bd_kludge : Term_tab[i]);
    while (tp != NULL) {
      tr = NULL;
      if (match(alpha, subst, tp->term, &tr)) {

	if (lex_dependent == 0)
	  ok = 1;
	else {
	  t = apply(beta, subst);
	  if (Flags[LRPO].val)
	    ok = lrpo_greater(tp->term, t);
	  else
	    ok = (lex_check(t, tp->term) == LESS_THAN);
	  zap_term(t);
	}

	if (ok) {
	  tp1 = get_term_ptr();
	  tp1->term = tp->term;
	  tp1->next = instances;
	  instances = tp1;
	}

	clear_subst_1(tr);
      }
      tp = tp->next;
    }
  }
  free_context(subst);
  return(instances);
}  /* all_instances */

/*************
 *
 *    struct term_ptr *all_instances_fpa(atom)
 *
 *    Get all terms (in table of shared terms) that can be rewritten
 *    with demodulator (atom).  Handles lex-dependent demod correctly.
 *
 *************/

struct term_ptr *all_instances_fpa(struct term *atom, struct fpa_index *fpa)
{
  struct term *alpha, *beta, *t, *found;
  struct term_ptr *tp1, *instances;
  struct context *subst;
  struct trail *tr;
  int lex_dependent, ok;
  struct fpa_tree *ut;

  alpha = atom->farg->argval;
  beta = atom->farg->narg->argval;
  lex_dependent = (atom->varnum == LEX_DEP_DEMOD);
  instances = NULL;
  subst = get_context();
  subst->multiplier = 1;

  ut = build_tree(alpha, INSTANCE, Parms[FPA_TERMS].val, fpa);
		
  found = next_term(ut, 0);
  while (found != NULL) {
    tr = NULL;
    if (match(alpha, subst, found, &tr)) {
	
      if (lex_dependent == 0)
	ok = 1;
      else {
	t = apply(beta, subst);
	if (Flags[LRPO].val)
	  ok = lrpo_greater(found, t);
	else
	  ok = (lex_check(t, found) == LESS_THAN);
	zap_term(t);
      }
	
      if (ok) {
	tp1 = get_term_ptr();
	tp1->term = found;
	tp1->next = instances;
	instances = tp1;
      }
	
      clear_subst_1(tr);
    }
    found = next_term(ut, 0);
  }
  free_context(subst);
  return(instances);
}  /* all_instances_fpa */

/*************
 *
 *    bd_kludge_insert(t)
 *
 *    This has to do with finding terms that can be back demodulated.
 *    Terms are made available (either indexed or in table of shared
 *    terms) when integrated.  However, alphas and betas are not shared,
 *    so this routine makes them available, either indexed or inserted
 *    into the Bd_kludge list.
 *
 *************/

void bd_kludge_insert(struct term *t)
{
  struct term_ptr *tp;

  if (Flags[INDEX_FOR_BACK_DEMOD].val)
    fpa_insert(t, Parms[FPA_TERMS].val, Fpa_back_demod);
  else {

    tp = get_term_ptr();
    tp->term = t;

    tp->next = Bd_kludge;
    Bd_kludge = tp;
  }
}  /* bd_kludge_insert */

/*************
 *
 *    bd_kludge_delete(t)
 *
 *    See Bd_kludge_insert.
 *
 *************/

void bd_kludge_delete(struct term *t)
{
  struct term_ptr *tp1, *tp2;

  if (Flags[INDEX_FOR_BACK_DEMOD].val) {
    CLOCK_START(UN_INDEX_TIME);
    fpa_delete(t, Parms[FPA_TERMS].val, Fpa_back_demod);
    CLOCK_STOP(UN_INDEX_TIME);
  }
  else
    {
      tp1 = Bd_kludge;
      tp2 = NULL;
      while (tp1 != NULL && tp1->term != t) {
	tp2 = tp1;
	tp1 = tp1->next;
      }
      if (tp1 == NULL) {
	fprintf(stderr, "WARNING, bd_kludge_delete, term not found.\n");
	printf("WARNING, bd_kludge_delete, term not found: ");
	print_term_nl(stdout, t);
      }
      else if (tp2 != NULL)
	tp2->next = tp1->next;
      else
	Bd_kludge = tp1->next;

      free_term_ptr(tp1);
    }

}  /* bd_kludge_delete */
