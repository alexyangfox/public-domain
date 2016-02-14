/*
  This if the new code (April 2004) for inserting/deleting/traversing
   FPA lists.  It should function exactly the same as the old code,
   except that deletions should be much faster, and memory usage will
   be somewhat different (not much).

   Instead of a (singly-linked) list of pointers to terms, we have a
   (singly-linked) list of *arrays* of pointers to terms.  As before,
   the terms are kept in decreasing order.  Recall that in practice,
   terms being inserted will usually be greater than anything already
   in the list.
*/

#include "header.h"

/*
  The design is determined by the following properties of the
  application:  (1) items will nearly always be inserted in
  increasing order, (2) the lists will be traversed, and the
  items must be kept in decreasing order, and (3) deletions
  will be arbitrary and occasionally extensive.
*/

/* First and last items in chunk.  Items are right-justified. */

#define FLAST(f) (f)->d[FMAX-1]
#define FFIRST(f) (f)->d[FMAX-((f)->n)]

/*************
 *
 *   flist_insert()
 *
 *   If the item is greater than any in the list, insertion should
 *   be constant time.
 *
 *************/

Fnode flist_insert(Fnode f, FTYPE x)
{
  if (f == NULL) {
    f = get_fnode();
    FLAST(f) = x;
    f->n = 1;
  }
  else if (f->n == FMAX) {
    if (FLT(x,FLAST(f)))
      f->next = flist_insert(f->next, x);
    else if (FEQ(x,FLAST(f)))
      fprintf(stderr, "WARNING: flist_insert, item %d already here (1)!\n", x);
    else if (FGT(x,FFIRST(f))) {
      /* 
	 This special case isn't necessary.  It is to improve performance.
	 The application for which I'm writing this inserts items in
	 increasing order (most of the time), and this prevents a lot of
	 half-empty nodes in that case.
      */
      Fnode f2 = flist_insert(NULL, x);
      f2->next = f;
      f = f2;
    }
    else {
      /* split this node in half */
      Fnode f2 = get_fnode();
      int move = FMAX / 2;
      int i, j;
      for (i = 0, j = FMAX-move; i < move; i++, j++) {
	f2->d[j] = f->d[i];
	f->d[i] = FDEFAULT;
      }
      f2->n = move;
      f->n = FMAX - move;
      f2->next = f;
      f = flist_insert(f2, x);
    }
  }
  else {
    if (f->next && FLE(x,FFIRST(f->next)))
      f->next = flist_insert(f->next, x);
    else {
      /* insert into this node */
      int n = f->n;
      int i = FMAX - n;
      while (i < FMAX && FLT(x,f->d[i]))
	i++;
      if (i < FMAX && FEQ(x,f->d[i]))
	fprintf(stderr, "WARNING: flist_insert, item %d already here (2)!\n", x);
      else if (i == FMAX - n) {
	f->d[i-1] = x;
	f->n = n+1;
      }
      else {
	/* insert at i-1, shifting the rest */
	int j;
	for (j = FMAX-n; j < i; j++)
	  f->d[j-1] = f->d[j];
	f->d[i-1] = x;
	f->n = n+1;
      }
    }
  }
  return f;
}  /* flist_insert */

/*************
 *
 *   consolidate() - try to join f and f->next; not recursive
 *
 *************/

static
Fnode consolidate(Fnode f)
{
  if (f->next && f->n + f->next->n <= FMAX) {
    Fnode f2 = f->next;
    int i;
    for (i = 0; i < f->n; i++)
      f2->d[FMAX - (f2->n + i + 1)] = f->d[FMAX - (i+1)];
    f2->n = f->n + f2->n;
    free_fnode(f);
    return f2;
  }
  else
    return f;
}  /* consolidate */

/*************
 *
 *   flist_delete()
 *
 *************/

Fnode flist_delete(Fnode f, FTYPE x)
{
  if (f == NULL)
    fprintf(stderr, "WARNING: flist_delete, item %d not found (1)!\n", x);
  else if (FLT(x,FLAST(f)))
    f->next = flist_delete(f->next, x);
  else {
    int n = f->n;
    int i = FMAX - n;
    while (i < FMAX && FLT(x,f->d[i]))
      i++;
    if (FNE(x,f->d[i]))
      fprintf(stderr, "WARNING: flist_delete, item %d not found (2)!\n", x);
    else {
      /* delete and close the hole */
      int j;
      for (j = i; j > FMAX-n; j--)
	f->d[j] = f->d[j-1];
      f->d[j] = FDEFAULT;
      f->n = n-1;
      if (f->n == 0) {
	/* delete this node */
	Fnode next = f->next;
	free_fnode(f);
	f = next;
      }
      else {
	/* try to join this node with the next */
	f = consolidate(f);
      }
    }
  }
  return f;
}  /* flist_delete */

/*************
 *
 *   first_fpos()
 *
 *************/

struct fposition first_fpos(Fnode f)
{
  if (f == NULL)
    return (struct fposition) {NULL, 0};
  else
    return (struct fposition) {f, FMAX - f->n};
}  /* first_fpos */

/*************
 *
 *   next_fpos()
 *
 *************/

struct fposition next_fpos(struct fposition p)
{
  int i = p.i+1;
  if (i < FMAX)
    return (struct fposition) {p.f, i};
  else
    return first_fpos((p.f)->next);
}  /* next_fpos */
