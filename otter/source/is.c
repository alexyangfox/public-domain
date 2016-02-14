/*
 *    is.c -- This file contains routines for discrimination tree
 *    indexing for forward subsumption.
 *
 */

#include "header.h"

/*************
 *
 *    struct is_tree *insert_is_tree(t, is)
 *
 *************/

static struct is_tree *insert_is_tree(struct term *t,
				      struct is_tree *is)
{
  struct rel *r;
  struct is_tree *i1, *prev, *i3;
  int varnum, sym;

  if (t->type == VARIABLE) {
    i1 = is->u.kids;
    prev = NULL;
    varnum = t->varnum;
    while (i1 != NULL && i1->type == VARIABLE && (int) i1->lab < varnum) {
      prev = i1;
      i1 = i1->next;
    }
    if (i1 == NULL || i1->type != VARIABLE || i1->lab != varnum) {
      i3 = get_is_tree();
      i3->type = VARIABLE;
      i3->lab = t->varnum;
      i3->next = i1;
      if (prev == NULL)
	is->u.kids = i3;
      else
	prev->next = i3;
      return(i3);
    }
    else  /* found node */
      return(i1);
  }

  else {  /* NAME || COMPLEX */
    i1 = is->u.kids;
    prev = NULL;
    /* arities fixed: handle both NAME and COMPLEX */
    sym = t->sym_num;
    while (i1 != NULL && i1->type == VARIABLE) {  /* skip variables */
      prev = i1;
      i1 = i1->next;
    }
    while (i1 != NULL && (int) i1->lab < sym) {
      prev = i1;
      i1 = i1->next;
    }
    if (i1 == NULL || i1->lab != sym) {
      i3 = get_is_tree();
      i3->type = t->type;
      i3->lab = sym;
      i3->next = i1;
      i1 = i3;
    }
    else
      i3 = NULL;  /* new node not required at this level */

    if (t->type == COMPLEX && t->sym_num != Ignore_sym_num) {
      r = t->farg;
      while (r != NULL) {
	i1 = insert_is_tree(r->argval, i1);
	r = r->narg;
      }
    }
    if (i3 != NULL) {  /* link in new subtree (possibly a leaf) */
      if (prev == NULL)
	is->u.kids = i3;
      else
	prev->next = i3;
    }
	
    return(i1);  /* i1 is leaf corresp. to end of input term */
  }
}  /* insert_is_tree */

/*************
 *
 *    is_insert(t, root_is)
 *
 *    Insert a term into the discrimination tree index for
 *    forward subsumption.  (for finding more general terms)
 *
 *************/

void is_insert(struct term *t,
	       struct is_tree *root_is)
{
  struct is_tree *i1;
  struct term_ptr *tp;

  i1 = insert_is_tree(t, root_is);
  tp = get_term_ptr();
  tp->term = t;
  tp->next = i1->u.terms;
  i1->u.terms = tp;
}  /* is_insert */

/*************
 *
 *    struct is_tree *end_term_is(t, is, path_p)
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

static struct is_tree *end_term_is(struct term *t,
				   struct is_tree *is,
				   struct term_ptr **path_p)
{
  struct rel *r;
  struct is_tree *i1;
  struct term_ptr *isp;
  int varnum, sym;

  /* add current node to the front of the path list. */

  isp = get_term_ptr();
  isp->term = (struct term *) is;
  isp->next = *path_p;
  *path_p = isp;

  if (t->type == VARIABLE) {
    i1 = is->u.kids;
    varnum = t->varnum;
    while (i1 != NULL && i1->type == VARIABLE && (int) i1->lab < varnum)
      i1 = i1->next;

    if (i1 == NULL || i1->type != VARIABLE || i1->lab != varnum)
      return(NULL);
    else   /* found node */
      return(i1);
  }

  else {  /* NAME || COMPLEX */
    i1 = is->u.kids;
    sym = t->sym_num;  /* arities fixed: handle both NAME and COMPLEX */
    while (i1 != NULL && i1->type == VARIABLE)  /* skip variables */
      i1 = i1->next;
    while (i1 != NULL && (int) i1->lab < sym)
      i1 = i1->next;

    if (i1 == NULL || i1->lab != sym)
      return(NULL);
    else {
      if (t->type == NAME || t->sym_num == Ignore_sym_num)
	return(i1);
      else {
	r = t->farg;
	while (r != NULL && i1 != NULL) {
	  i1 = end_term_is(r->argval, i1, path_p);
	  r = r->narg;
	}
	return(i1);
      }
    }
  }
}  /* end_term_is */

/*************
 *
 *    is_delete(t, root_is)
 *
 *************/

void is_delete(struct term *t,
	       struct is_tree *root_is)
{
  struct is_tree *end, *i2, *i3, *parent;
  struct term_ptr *tp1, *tp2;
  struct term_ptr *isp1, *path;

  /* First find the correct leaf.  path is used to help with  */
  /* freeing nodes, because nodes don't have parent pointers. */

  path = NULL;
  end = end_term_is(t, root_is, &path);
  if (end == NULL) {
    print_term_nl(stdout, t);
    abend("is_delete, can't find end.");
  }

  /* Free the pointer in the leaf-list */

  tp1 = end->u.terms;
  tp2 = NULL;
  while(tp1 != NULL && tp1->term != t) {
    tp2 = tp1;
    tp1 = tp1->next;
  }
  if (tp1 == NULL) {
    print_term_nl(stdout, t);
    abend("is_delete, can't find term.");
  }
  if (tp2 == NULL)
    end->u.terms = tp1->next;
  else
    tp2->next = tp1->next;
  free_term_ptr(tp1);

  if (end->u.terms == NULL) {
    /* free tree nodes from bottom up, using path to get parents */
    end->u.kids = NULL;  /* probably not necessary */
    isp1 = path;
    while (end->u.kids == NULL && end != root_is) {
      parent = (struct is_tree *) isp1->term;
      isp1 = isp1->next;
      i2 = parent->u.kids;
      i3 = NULL;
      while (i2 != end) {
	i3 = i2;
	i2 = i2->next;
      }
      if (i3 == NULL)
	parent->u.kids = i2->next;
      else
	i3->next = i2->next;
      free_is_tree(i2);
      end = parent;
    }
  }

  /* free path list */

  while (path != NULL) {
    isp1 = path;
    path = path->next;
    free_term_ptr(isp1);
  }

}  /* is_delete */

/*************
 *
 *    struct term *is_retrieve(term, subst, index_tree, position)
 *
 *        Return the first or next list of terms that subsumes `term'.
 *    Also return the substitution.  Return NULL if there are
 *    none or no more.  All terms in the returned list of terms
 *    are identical.
 *
 *    if (term != NULL)
 *        {This is the first call, so return the first, and also
 *        return a position for subsequent calls}
 *    else if (position != NULL)
 *        {return the next term, and update the position}
 *    else
 *        {there are no more terms that subsume}
 *
 *    If you don't want the entire set of subsuming terms, then
 *    cancel the position with `free_is_pos_list(position)'.
 *
 *************/

struct term_ptr *is_retrieve(struct term *t,
			     struct context *subst,
			     struct is_tree *is,
			     struct is_pos **is_pos)
{
  struct rel *rel_stack[MAX_FS_TERM_DEPTH];
  struct is_tree *i1 = NULL;
  struct is_pos *pos, *ip2;
  int found, backup, varnum, j, reset, sym;
  int top = 0;

  if (t != NULL) {  /* first call */
    pos = NULL;
    top = -1;
    i1 = is->u.kids;
    backup = 0;
  }
  else if (*is_pos != NULL) {  /* continuation with more to try */
    pos = *is_pos;  /* must remember to set is_pos on return */
    backup = 1;
  }
  else  /* continuation with nothing more to try */
    return(NULL);

  while (1) {  /* loop until a leaf is found or done with tree */
    if (backup) {
      if (pos == NULL)
	return(NULL);
      else {  /* pop top of stack (most recent variable node)
		 and restore state */
	top = pos->stack_pos;
	for (j = 0; j <= top; j++)
	  rel_stack[j] = pos->rel_stack[j];
	i1 = pos->is;
	t = subst->terms[i1->lab];
	if (pos->reset)  /* undo variable binding */
	  subst->terms[i1->lab] = NULL;
	i1 = i1->next;
	ip2 = pos;
	pos = pos->next;
	free_is_pos(ip2);
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
	ip2 = get_is_pos();
	ip2->next = pos;
	pos = ip2;
	pos->is = i1;
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
	else if (t->type == COMPLEX && t->sym_num != Ignore_sym_num) {
	  top++;
	  if (top >= MAX_FS_TERM_DEPTH)
	    abend("is_retrieve, increase MAX_FS_TERM_DEPTH.");
	  rel_stack[top] = t->farg;  /* save pointer to subterms */
	}
      }
    }

    if (backup == 0) {  /* get next term from rel_stack */
      while (top >= 0 && rel_stack[top] == NULL)
	top--;
      if (top == -1) {  /* found a term */
	*is_pos = pos;
	return(i1->u.terms);
      }
      else {  /* pop a term and continue */
	t = rel_stack[top]->argval;
	rel_stack[top] = rel_stack[top]->narg;
	i1 = i1->u.kids;
      }
    }
  }  /* end of while(1) loop */

}  /* is_retrieve */

/*************
 *
 *    struct term *fs_retrieve(t,  c, is, fs_pos)
 *
 *    Get the first or next term that subsumes t.   (t != NULL)
 *    for first call, and (t == NULL) for subsequent calls.
 *
 *    If you want to stop calls before a NULL is returned,
 *    call canc_fs_pos(fs_pos, context) to reclaim memory.
 *
 *************/

struct term *fs_retrieve(struct term *t,
			 struct context *subst,
			 struct is_tree *is,
			 struct fsub_pos **fs_pos)
{
  struct term_ptr *tp;
  struct is_pos *i_pos;
  struct fsub_pos *f_pos;

  if (t != NULL) {  /* if first call */
    tp = is_retrieve(t, subst, is, &i_pos);
    if (tp == NULL)
      return(NULL);
    else {
      f_pos = get_fsub_pos();
      f_pos->pos = i_pos;
      f_pos->terms = tp;
      *fs_pos = f_pos;
      return(tp->term);
    }
  }
  else {  /* subsequent call */
    f_pos = *fs_pos;
    tp = f_pos->terms->next;
    if (tp != NULL) {  /* if any more terms in current leaf */
      f_pos->terms = tp;
      return(tp->term);
    }
    else {  /* try for another leaf */
      tp = is_retrieve((struct term *) NULL, subst, is, &(f_pos->pos));
      if (tp == NULL) {
	free_fsub_pos(f_pos);
	return(NULL);
      }
      else {
	f_pos->terms = tp;
	return(tp->term);
      }
    }
  }
}  /* fs_retrieve */

/*************
 *
 *    canc_fs_pos(pos, subst)
 *
 *************/

void canc_fs_pos(struct fsub_pos *pos,
		 struct context *subst)
{
  int i;

  if (pos->pos != NULL) {
    for (i = 0; i < MAX_VARS; i++)
      subst->terms[i] = NULL;
  }

  free_is_pos_list(pos->pos);
  free_fsub_pos(pos);
}  /* canc_fs_pos */

/*************
 *
 *    print_is_tree(fp, is)
 *
 *        Display an index-subsumption tree.
 *
 *************/

void print_is_tree(FILE *fp,
		   struct is_tree *is)
{
  fprintf(fp, "don't know how to print is tree %p\n", (void *) is);
}  /* print_is_tree */

/*************
 *
 *    p_is_tree(is)
 *
 *************/

void p_is_tree(struct is_tree *is)
{
  print_is_tree(stdout, is);
}  /* p_is_tree */

