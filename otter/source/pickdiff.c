#include "header.h"

/*************
 *
 *   copy_rels() - copy a list of rel nodes and the associated pointers.
 *
 *************/

static struct rel *copy_rels(struct rel *a)
{
  if (a == NULL)
    return NULL;
  else {
    struct rel *r = get_rel();
    r->argval = a->argval;
    r->narg = copy_rels(a->narg);
    return r;
  }
}  /* copy_rels */

/*************
 *
 *   zap_rels() - free a list of rel nodes (but not the terms they point to)
 *
 *************/

static void zap_rels(struct rel *r)
{
  if (r != NULL) {
    struct rel *a = r->narg;
    free_rel(r);
    zap_rels(a);
  }
}  /* zap_rels */

/*************
 *
 *   remove1() - remove (and free) a rel node from a list
 *
 *************/

static struct rel *remove1(struct term *x, struct rel *a)
{
  if (a == NULL)
    return NULL;
  else if (x == a->argval) {
    struct rel *r = a->narg;
    free_rel(a);
    return r;
  }
  else {
    a->narg = remove1(x, a->narg);
    return a;
  }
}  /* remove1 */

/*************
 *
 *   add_vecs() - Add two vectors, creating a new vector.  The old vectors
 *   are freed.
 *
 *************/

static struct ilist *add_vecs(struct ilist *v1, struct ilist *v2)
{
  if (v1 == NULL)
    return v2;
  else if (v2 == NULL)
    return v1;
  else {
    struct ilist *v3 = get_ilist();
    v3->i = v1->i + v2->i;
    v3->next = add_vecs(v1->next, v2->next);
    free_ilist(v1);
    free_ilist(v2);
    return v3;
  }
}  /* add_vecs */

/*************
 *
 *   le_vecs(v1, v2) - is vector v1 <= vector v2?
 *
 *************/

static int le_vecs(struct ilist *v1, struct ilist *v2)
{
  if (v1 == NULL)
    return 1;
  else if (v2 == NULL)
    return 0;
  else if (v1->i < v2->i)
    return 1;
  else if (v1->i > v2->i)
    return 0;
  else
    return le_vecs(v1->next, v2->next);
}  /* le_vecs */

static struct ilist *diff(struct term *s, struct term *r);

/*************
 *
 *   diff_lists() - mutually recursive with diff()
 *
 *************/

static struct ilist *diff_lists(struct rel *a1, struct rel *a2)
{
  if (a1 == NULL && a2 == NULL) {
    struct ilist *v = get_ilist();
    v->i = 0;
    return v;
  }
  else if (a1 == NULL || a2 == NULL) {
    struct ilist *v = get_ilist();
    v->i = 1;
    return v;
  }
  else {
    return add_vecs(diff(a1->argval, a2->argval),
		    diff_lists(a1->narg, a2->narg));
  }
}  /* diff_lists */

/*************
 *
 *   diff() - return a vector representing the difference between two terms.
 *
 *   This version does not try to minimize differences by permuting arguments.
 *
 *************/

static struct ilist *diff(struct term *s, struct term *r)
{
  if (s->type == VARIABLE && r->type == VARIABLE) {
    struct ilist *v = get_ilist();
    v->i = 0;
    return v;
  }
  else if (s->type == VARIABLE || r->type == VARIABLE ||
	   s->sym_num != r->sym_num) {
    struct ilist *v = get_ilist();
    v->i = 1;
    return v;
  }
  else {
    struct ilist *v = get_ilist();
    v->i = 0;
    v->next = diff_lists(s->farg, r->farg);
    return v;
  }
}  /* diff */

static struct ilist *diff2(struct term *s, struct term *r);

/*************
 *
 *   min_diff(+x, +a, -m) - mutually recursive with diff2 and diff2_lists
 *
 *   Given term x and list of terms a, find the member of a that is
 *   most similar to x.  Set m to that member, and return the
 *   difference vector.
 *
 *************/

static struct ilist *min_diff(struct term *x, struct rel *a, struct term **m)
{
  if (a == NULL) {
    struct ilist *v = get_ilist();
    v->i = 1000;
    *m = NULL;
    return v;
  }
  else {
    struct ilist *v1 = diff2(x, a->argval);
    struct term *t;
    struct ilist *v2 = min_diff(x, a->narg, &t);
    if (le_vecs(v1, v2)) {
      *m = a->argval;
      free_ilist_list(v2);
      return v1;
    }
    else {
      *m = t;
      free_ilist_list(v1);
      return v2;
    }
  }
}  /* min_diff */

/*************
 *
 *   diff2_lists(a1, a2) - mutually recursive with diff2 and min_diff
 *
 *   The nodes of a2 are freed.
 *
 *************/

static struct ilist *diff2_lists(struct rel *a1, struct rel *a2)
{
  if (a1 == NULL && a2 == NULL) {
    struct ilist *v = get_ilist();
    v->i = 0;
    zap_rels(a2);
    return v;
  }
  else if (a1 == NULL) {
    struct ilist *v = get_ilist();
    v->i = 1;
    return v;
  }
  else {
    struct term *mint;
    struct ilist *minv = min_diff(a1->argval, a2, &mint);
    struct ilist *v = diff2_lists(a1->narg, remove1(mint, a2));
    return add_vecs(minv, v);
  }
}  /* diff2_lists */

/*************
 *
 *   diff2() - return a vector representing the difference between two terms.
 *
 *   This version tries to minimize differences by permuting arguments.
 *
 *************/

static struct ilist *diff2(struct term *s, struct term *r)
{
  if (s->type == VARIABLE && r->type == VARIABLE) {
    struct ilist *v = get_ilist();
    v->i = 0;
    return v;
  }
  else if (s->type == VARIABLE || r->type == VARIABLE ||
	   s->sym_num != r->sym_num) {
    struct ilist *v = get_ilist();
    v->i = 1;
    return v;
  }
  else {
    struct ilist *v = get_ilist();
    v->i = 0;
    v->next = diff2_lists(s->farg, copy_rels(r->farg));
    return v;
  }
}  /* diff2 */

/*************
 *
 *   cldiff() - return a vector representing the difference between two clauses.
 *
 *   This version applies to just the first literals, and ignores the sign.
 *
 *************/

struct ilist *cldiff(struct clause *c, struct clause *d)
{
  if (c->first_lit == NULL || d->first_lit == NULL ||
      c->first_lit->sign != d->first_lit->sign) {
    struct ilist *v = get_ilist();
    v->i = 1;
    return v;
  }
  else if (Parms[PICK_DIFF].val == 1)
    return diff(c->first_lit->atom, d->first_lit->atom);
  else if (Parms[PICK_DIFF].val == 2)
    return diff2(c->first_lit->atom, d->first_lit->atom);
  else {
    abend("cldiff, bad PICK_DIFF");
    return NULL;
  }
}  /* cldiff */

/*************
 *
 *   get_ci_of_wt_range()
 *
 *************/

static struct ci_ptr *get_ci_of_wt_range(struct clause *c,
					 int min, int max)
{
  if (c == NULL)
    return NULL;
  else if (c->pick_weight >= min && c->pick_weight <= max) {
    struct ci_ptr *p = get_ci_ptr();
    p->c = c;
    p->next = get_ci_of_wt_range(c->next_cl, min, max);
    return p;
  }
  else
    return get_ci_of_wt_range(c->next_cl, min, max);
}  /* get_ci_of_wt_range */

/*************
 *
 *   zap_ci_ptr_list(p)
 *
 *   Free the nodes and the vectors, but not the clauses.
 *
 *************/

void zap_ci_ptr_list(struct ci_ptr *p)
{
  if (p == NULL)
    return;
  else {
    zap_ci_ptr_list(p->next);
    free_ilist_list(p->v);
    free_ci_ptr(p);
  }
}  /* zap_ci_ptr_list */

/*************
 *
 *   find_pickdiff_cl()
 *
 *************/

struct clause *find_pickdiff_cl(struct list *sos, struct list *usable)
{
  struct clause *c = find_lightest_cl(sos);
  if (c == NULL)
    return NULL;
  else {
    int min = c->pick_weight;
    int max = min + Parms[PICK_DIFF_RANGE].val;
    struct ci_ptr *s = get_ci_of_wt_range(sos->first_cl, min, max);
    struct ci_ptr *u = get_ci_of_wt_range(usable->first_cl, min, max);

    struct ci_ptr *p1, *p2;
    struct ilist *v;

    /* Find the member of s that is "most different" from members of u. */

    for (p1 = s; p1 != NULL; p1 = p1->next) {
#if 0	
      printf("\nSos clause: "); p_clause(p1->c);
#endif
      p1->v = NULL;
      for (p2 = u; p2 != NULL; p2 = p2->next) {
	v = cldiff(p1->c, p2->c);
#if 0	
	printf("    Usable clause: "); print_ilist(stdout, v);
	print_clause(stdout, p2->c);
#endif
	p1->v = add_vecs(p1->v, v);
      }
#if 0	
      printf("Total difference: "); p_ilist(p1->v);
#endif
    }

    /* Now, get the Sos clause with the greatest (least?) total difference. */
    /* We know that Sos (therefore s) is not empty. */

    v = s->v; c = s->c;
    for (p1 = s->next; p1 != NULL; p1 = p1->next) {
      int better;
      if (Flags[PICK_DIFF_SIM].val)
	better = !le_vecs(v, p1->v);  /* looking for least diff */
      else
	better = !le_vecs(p1->v, v);  /* looking for most diff */
      
      if (better) {
	v = p1->v;
	c = p1->c;
      }
    }

    zap_ci_ptr_list(s);
    zap_ci_ptr_list(u);

    return c;
  }
}  /* find_pickdiff_cl */

