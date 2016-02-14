/*
 *
 *  imd.c -- This file contains routines for discrimination
 *  tree indexing for demodulation.
 *
 */

#include "header.h"

/*************
 *
 *    struct imd_tree *insert_imd_tree(t, imd)  --  called by imd_insert
 *
 *************/

static struct imd_tree *insert_imd_tree(struct term *t,
					struct imd_tree *imd)
{
  struct rel *r;
  struct imd_tree *i1, *i2, *i3;
  int varnum, sym;

  if (t->type == VARIABLE) {
    i1 = imd->kids;
    i2 = NULL;
    varnum = t->varnum;
    while (i1 != NULL && i1->type == VARIABLE && (int) i1->lab < varnum) {
      i2 = i1;
      i1 = i1->next;
    }
    if (i1 == NULL || i1->type != VARIABLE || i1->lab != varnum) {
      i3 = get_imd_tree();
      i3->type = VARIABLE;
      i3->lab = varnum;
      i3->next = i1;
      if (i2 == NULL)
	imd->kids = i3;
      else
	i2->next = i3;
      return(i3);
    }
    else  /* found node */
      return(i1);
  }

  else {  /* NAME || COMPLEX */
    i1 = imd->kids;
    i2 = NULL;
    sym = t->sym_num;  /* arities fixed: handle both NAME and COMPLEX */
    while (i1 != NULL && i1->type == VARIABLE) {  /* skip variables */
      i2 = i1;
      i1 = i1->next;
    }
    while (i1 != NULL && (int) i1->lab < sym) {
      i2 = i1;
      i1 = i1->next;
    }
    if (i1 == NULL || i1->lab != sym) {
      i3 = get_imd_tree();
      i3->type = t->type;
      i3->lab = sym;
      i3->next = i1;
      i1 = i3;
    }
    else
      i3 = NULL;  /* new node not required at this level */

    if (t->type == COMPLEX) {
      r = t->farg;
      while (r != NULL) {
	i1 = insert_imd_tree(r->argval, i1);
	r = r->narg;
      }
    }
    if (i3 != NULL) {  /* link in new subtree (possibly a leaf) */
      if (i2 == NULL)
	imd->kids = i3;
      else
	i2->next = i3;
    }

    return(i1);  /* i1 is leaf corresp. to end of input term */
  }
}  /* insert_imd_tree */

/*************
 *
 *    imd_insert(demod, imd)
 *
 *    Insert the left argument of demod into the  discrimination
 *    tree index for demodulation.
 *
 *************/

void imd_insert(struct clause *demod,
		struct imd_tree *imd)
{
  struct imd_tree *i1;
  struct term *atom, *alpha, *beta;
  struct term_ptr *tp;
  int max;

  atom = ith_literal(demod,1)->atom;
  if (atom->varnum != CONDITIONAL_DEMOD) {
    alpha = atom->farg->argval;
    beta =  atom->farg->narg->argval;
  }
  else {  /* CONDITIONAL(cond, alpha, beta) */
    alpha = atom->farg->narg->argval->farg->argval;
    beta =  atom->farg->narg->argval->farg->narg->argval;
  }

  if (term_ident(alpha, beta)) {
    fprintf(stderr, "\nWARNING, instance of x=x cannot be inserted into demod_imd index: ");
    print_clause(stderr, demod);
    printf("\nWARNING, instance of x=x cannot be inserted into demod_imd index: ");
    print_clause(stdout, demod);
  }
  else {
    i1 = insert_imd_tree(alpha, imd);
    tp = get_term_ptr();
    tp->term = atom;
    tp->next = i1->atoms;
    if ((max = biggest_var(alpha)) == -1)
      i1->max_vnum = 0;  /* in case i->max_vnum is an unsigned char */
    else
      i1->max_vnum = max;
    i1->atoms = tp;
	
  }
}  /* imd_insert */

/*************
 *
 *    struct imd_tree *end_term_imd(t, imd, path_p)
 *
 *    Given a discrimination tree (or a subtree) and a term, return the
 *    node in the tree that corresponds to the last symbol in t (or NULL
 *    if the node doesn't exist).  *path_p is a list that is extended by
 *    this routine.  It is a list of pointers to the
 *    nodes in path from the parent of the returned node up to imd.
 *    (It is needed for deletions, because nodes do not have pointers to
 *    parents.)
 *
 *************/

static struct imd_tree *end_term_imd(struct term *t,
				     struct imd_tree *imd,
				     struct term_ptr **path_p)
{
  struct rel *r;
  struct imd_tree *i1;
  struct term_ptr *imdp;
  int varnum, sym;

  /* add current node to the front of the path list. */

  imdp = get_term_ptr();
  imdp->term = (struct term *) imd;
  imdp->next = *path_p;
  *path_p = imdp;

  if (t->type == VARIABLE) {
    i1 = imd->kids;
    varnum = t->varnum;
    while (i1 != NULL && i1->type == VARIABLE && (int) i1->lab < varnum)
      i1 = i1->next;

    if (i1 == NULL || i1->type != VARIABLE || i1->lab != varnum)
      return(NULL);
    else   /* found node */
      return(i1);
  }

  else {  /* NAME || COMPLEX */
    i1 = imd->kids;
    sym = t->sym_num;  /* arities fixed: handle both NAME and COMPLEX */
    while (i1 != NULL && i1->type == VARIABLE)  /* skip variables */
      i1 = i1->next;
    while (i1 != NULL && (int) i1->lab < sym)
      i1 = i1->next;

    if (i1 == NULL || i1->lab != sym)
      return(NULL);
    else {
      if (t->type == NAME)
	return(i1);
      else {
	r = t->farg;
	while (r != NULL && i1 != NULL) {
	  i1 = end_term_imd(r->argval, i1, path_p);
	  r = r->narg;
	}
	return(i1);
      }
    }
  }
}  /* end_term_imd */

/*************
 *
 *    imd_delete(demod, root_imd)
 *
 *    Delete the left argument of demod from the demodulation discrimination tree.
 *
 *************/

void imd_delete(struct clause *demod,
		struct imd_tree *root_imd)
{
  struct imd_tree *end, *i2, *i3, *parent;
  struct term_ptr *tp1, *tp2;
  struct term_ptr *imdp, *path;
  struct term *atom, *alpha;

  /* First find the correct leaf.  path is used to help with  */
  /* freeing nodes, because nodes don't have parent pointers. */

  path = NULL;
  atom = ith_literal(demod,1)->atom;

  if (atom->varnum == CONDITIONAL_DEMOD)
    alpha = atom->farg->narg->argval->farg->argval;
  else
    alpha = atom->farg->argval;

  end = end_term_imd(alpha, root_imd, &path);

  if (end == NULL) {
    print_term_nl(stdout, alpha);
    abend("imd_delete, can't find alpha.");
  }

  tp1 = end->atoms;
  tp2 = NULL;
  while (tp1 != NULL && tp1->term != atom) {
    tp2 = tp1;
    tp1 = tp1->next;
  }

  if (tp1 == NULL) {
    print_term_nl(stdout, atom);
    abend("imd_delete, can't find atom.");
  }

  if (tp2 == NULL)
    end->atoms = tp1->next;
  else
    tp2->next = tp1->next;
  free_term_ptr(tp1);

  if (end->atoms == NULL) {
    /* free tree nodes from bottom up, using path to get parents */
    imdp = path;
    while (end->kids == NULL && end != root_imd) {
      parent = (struct imd_tree *) imdp->term;
      imdp = imdp->next;
      i2 = parent->kids;
      i3 = NULL;
      while (i2 != end) {
	i3 = i2;
	i2 = i2->next;
      }
      if (i3 == NULL)
	parent->kids = i2->next;
      else
	i3->next = i2->next;
      free_imd_tree(i2);
      end = parent;
    }
  }

  /* free path list */

  while (path != NULL) {
    imdp = path;
    path = path->next;
    free_term_ptr(imdp);
  }

}  /* imd_delete */

/*************
 *
 *    struct term *contract_imd(t_in, demods, subst, demod_id_p)
 *
 *    Attempt to contract (rewrite one step) a term (t_in) using demodulators
 *    in a disckrimination tree index (demods).  NULL is returned if t_in
 *    cannot be contracted.  subst is an empty substitution.
 *    If success, *demod_id_p is set to the ID of the rewrite rule.
 *
 *************/

struct term *contract_imd(struct term *t_in,
			  int *demods,
			  struct context *subst,
			  int *demod_id_p)
{
  struct rel *rel_stack[MAX_AL_TERM_DEPTH];
  struct imd_tree *imd, *i1;
  struct imd_pos *pos, *ip2;
  struct term *t, *t2, *t3, *atom;
  struct term *replacement = NULL;
  struct term_ptr *tp;
  int top, found, backup, varnum, j, reset, mult_flag, sym, ok, dummy;

  imd = (struct imd_tree *) demods;
  if (imd == NULL)
    return(NULL);
  pos = NULL;
  top = -1;
  backup = 0;
  i1 = imd->kids;
  t = t_in;

  while(1) {
    if (backup) {
      if (pos == NULL)
	return(NULL);
      else {  /* pop top of stack (most recent variable node)
		 and restore state */
	top = pos->stack_pos;
	for (j = 0; j <= top; j++)
	  rel_stack[j] = pos->rel_stack[j];
	i1 = pos->imd;
	t = subst->terms[i1->lab];
	if (pos->reset)  /* undo variable binding */
	  subst->terms[i1->lab] = NULL;
	i1 = i1->next;
	ip2 = pos;
	pos = pos->next;
	free_imd_pos(ip2);
      }
    }

    /* at this point, i1 is the next node to try */

    found = 0;
    /* first try to match input term t with a variable node */
    while (found == 0 && i1 != NULL && i1->type == VARIABLE) {
      varnum = i1->lab;
      if (subst->terms[varnum] == NULL) { /*if not bound, bind it */
	subst->terms[varnum] = t;
	subst->contexts[varnum] = NULL;
	found = 1;
	reset = 1;
      }
      else {  /* bound variable, succeed iff identical */
	found = term_ident(subst->terms[varnum], t);
	reset = 0;
      }

      if (found) {  /* save state */
	ip2 = get_imd_pos();
	ip2->next = pos;
	pos = ip2;
	pos->imd = i1;
	pos->reset = reset;
	for (j = 0; j <= top; j++)
	  pos->rel_stack[j] = rel_stack[j];
	pos->stack_pos = top;
      }
      else  /* try next variable */
	i1 = i1->next;
    }

    backup = 0;
    if (found == 0) {  /* couldn't match t with (another) variable */
      if (t->type == VARIABLE)
	backup = 1;  /* because we can't instantiate given term */
      else {  /* NAME or COMPLEX */
	sym = t->sym_num;
	while (i1 != NULL && (int) i1->lab < sym)
	  i1 = i1->next;
	if (i1 == NULL || i1->lab != sym)
	  backup = 1;
	else if (t->type == COMPLEX) {
	  top++;
	  if (top >= MAX_AL_TERM_DEPTH) {
	    abend("contract_imd, increase MAX_AL_TERM_DEPTH.");
	    return(NULL);  /* to quiet lint */
	  }
	  rel_stack[top] = t->farg;  /* save pointer to subterms */
	}
      }
    }

    if (backup == 0) {  /* get next term from rel_stack */
      while (top >= 0 && rel_stack[top] == NULL)
	top--;

      if (top == -1) {  /* found potential demods */
	tp = i1->atoms;
	ok = 0;
	while(tp != NULL && ok == 0) {
	  atom = tp->term;
	  mult_flag = 0;
	  if (atom->varnum == LEX_DEP_DEMOD) {
	    replacement = apply_demod(atom->farg->narg->argval, subst, &mult_flag);
	    if (Flags[LRPO].val)
	      ok = lrpo_greater(t_in, replacement);
	    else
	      ok = lex_check(replacement, t_in) == LESS_THAN;
	    if (ok == 0) {
	      zap_term_special(replacement);
	      tp = tp->next;
	    }
	  }
	  else if (atom->varnum == CONDITIONAL_DEMOD) {
	    /* apply subst to condition, then demodulate */
	    t2 = apply_demod(atom->farg->argval, subst, &dummy);
	    un_share_special(t2);
	    t3 = convenient_demod(t2);
	    ok = is_symbol(t3, "$T", 0);
	    zap_term_special(t3);
	    if (ok)
	      replacement = apply_demod(atom->farg->narg->argval->farg->narg->argval, subst, &mult_flag);
	    else
	      tp = tp->next;
	  }
	  else {  /* redular demoulator */
	    replacement = apply_demod(atom->farg->narg->argval, subst, &mult_flag);
	    ok = 1;
	  }
		
	}
		
	if (ok) {
	  if (mult_flag)
	    subst->multiplier++;
	  for (j = 0; j <= (int) i1->max_vnum; j++) /* clear substitution */
	    subst->terms[j] = NULL;
	  free_imd_pos_list(pos);
	  zap_term_special(t_in);
	  *demod_id_p = tp->term->occ.lit->container->id;
	  return(replacement);
	}
	else  /* failed lex_checks, so prepare to back up */
	  backup = 1;
      }

      else {  /* pop a term and continue */
	t = rel_stack[top]->argval;
	rel_stack[top] = rel_stack[top]->narg;
	i1 = i1->kids;
      }
    }
  }  /* end of while(1) loop */

}  /* contract_imd */

/*************
 *
 *    print_imd_tree(file_pointer, imd_tree, level)
 *
 *        Display an imd tree.  Level == 0 on initial call.
 *
 *************/

void print_imd_tree(FILE *fp,
		    struct imd_tree *imd,
		    int level)
{
  struct imd_tree *i1;
  int i;

  fprintf(fp, "%p ", (void *) imd);
  for (i = 0; i < level; i++)
    fprintf(fp, "  ");
  if (imd->type == 0)
    fprintf(fp, "start of index-match-demodulate tree");
  else if (imd->type == VARIABLE)
    fprintf(fp, "v%d ", imd->lab);
  else
    fprintf(fp, "%s ", sn_to_str((int) imd->lab));

  if (imd->atoms != NULL) {
    fprintf(fp, " demod=");
    print_term(fp, imd->atoms->term);
  }
  fprintf(fp, "\n");

  i1 = imd->kids;
  while (i1 != NULL) {
    print_imd_tree(fp, i1, level + 1);
    i1 = i1->next;
  }

}  /* print_imd_tree */

/*************
 *
 *    p_imd_tree(imd_tree)
 *
 *        Display an imd tree.  Level == 0 on initial call.
 *
 *************/

void p_imd_tree(struct imd_tree *imd)
{
  print_imd_tree(stdout, imd, 0);
}  /* p_imd_tree */

