/*
 *  lists.c -- utilities for ilist, glist, and g2list
 *
 */

#include "header.h"

/* ============================================================ ilist  */

/*************
 *
 *   free_ilist_list()
 *
 *************/

void free_ilist_list(struct ilist *p)
{
  if (p != NULL) {
    free_ilist_list(p->next);
    free_ilist(p);
  }
}  /* free_ilist_list */

/*************
 *
 *   ilist_tack_on()
 *
 *************/

struct ilist *ilist_tack_on(struct ilist *a, int i)
{
  if (a == NULL) {
    struct ilist *b = get_ilist();
    b->i = i;
    return b;
  }
  else {
    a->next = ilist_tack_on(a->next, i);
    return a;
  }
}  /* ilist_tack_on */

/*************
 *
 *   iset_add()
 *
 *************/

struct ilist *iset_add(int i, struct ilist *p)
{
  if (p == NULL) {
    p = get_ilist();
    p->i = i;
    return p;
  }
  else if (p->i == i)
    return p;
  else if (p->i > i) {
    struct ilist *q = get_ilist();
    q->i = i;
    q->next = p;
    return q;
  }
  else {
    p->next = iset_add(i, p->next);
    return p;
  }
}  /* iset_add */

/*************
 *
 *   iset_remove() -- assume ordered, remove first
 *
 *************/

struct ilist *iset_remove(int i, struct ilist *p)
{
  if (p == NULL) {
    return p;
  }
  else if (p->i == i) {
    struct ilist *q = p;
    p = p->next;
    free_ilist(q);
    return p;
  }
  else if (p->i > i) {
    return p;
  }
  else {
    p->next = iset_remove(i, p->next);
    return p;
  }
}  /* iset_remove */

/*************
 *
 *   ilist_member()
 *
 *************/

int ilist_member(int i, struct ilist *p)
{
  if (p == NULL)
    return 0;
  else if (p->i == i)
    return 1;
  else
    return ilist_member(i, p->next);
}  /* ilist_member */

/*************
 *
 *   iset_subset()
 *
 *************/

int iset_subset(struct ilist *a, struct ilist *b)
{
  if (a == NULL)
    return 1;
  else
    return (ilist_member(a->i, b) && iset_subset(a->next, b));
}  /* iset_subset */

/*************
 *
 *   iset_subtract()
 *
 *************/

struct ilist *iset_subtract(struct ilist *a, struct ilist *b)
{
  if (b == NULL)
    return copy_ilist(a);
  else {
    a = iset_subtract(a, b->next);
    return iset_remove(b->i, a);
  }
}  /* iset_subtract */

/*************
 *
 *   iset_sort() -- insertion sort -- remove duplicates
 *
 *************/

struct ilist *iset_sort(struct ilist *a)
{
  if (a == NULL)
    return NULL;
  else {
    int i = a->i;
    struct ilist *b = iset_sort(a->next);
    free_ilist(a);
    return iset_add(i, b);
  }
}  /* iset_sort */

/*************
 *
 *   idempot_ip()
 *
 *************/

struct ilist *idempot_ip(struct ilist *a)
{
  if (a == NULL)
    return NULL;
  else if (ilist_member(a->i, a->next)) {
    struct ilist *b = a->next;
    free_ilist(a);
    return idempot_ip(b);
  }
  else {
    a->next = idempot_ip(a->next);
    return a;
  }
}  /* idempot_ip */

/*************
 *
 *   reverse_ip()
 *
 *************/

struct ilist *reverse_ip(struct ilist *ip1, struct ilist *ip2)
{
  if (ip1 == NULL)
    return ip2;
  else {
    struct ilist *ip3 = ip1->next;
    ip1->next = ip2;
    return reverse_ip(ip3, ip1);
  }
}  /* reverse_ip */

/*************
 *
 *   ilist_append() -- This uses up the two inputs.
 *
 *************/

struct ilist *ilist_append(struct ilist *a, struct ilist *b)
{
  if (a == NULL)
    return b;
  else {
    a->next = ilist_append(a->next, b);
    return a;
  }
}  /* ilist_append */

/*************
 *
 *   copy_ilist()
 *
 *   Copy and return a list of pointers to integers.
 *
 *************/

struct ilist *copy_ilist(struct ilist *p)
{
  if (p == NULL)
    return NULL;
  else {
    struct ilist *q = get_ilist();
    q->i = p->i;
    q->next = copy_ilist(p->next);
    return q;
  }
}  /* copy_ilist */

/*************
 *
 *   ilist_length()
 *
 *************/

int ilist_length(struct ilist *a)
{
  if (a == NULL)
    return 0;
  else
    return 1 + ilist_length(a->next);
}  /* ilist_length */

/*************
 *
 *   copy_ilist_segment() -- copy the first n members
 *
 *************/

struct ilist *copy_ilist_segment(struct ilist *p, int n)
{
  if (n == 0 || p == NULL)
    return NULL;
  else {
    struct ilist *q = get_ilist();
    q->i = p->i;
    q->next = copy_ilist_segment(p->next, n-1);
    return q;
  }
}  /* copy_ilist_segment */

/*************
 *
 *   print_ilist(fp, ip)
 *
 *************/

void print_ilist(FILE *fp,
		    struct ilist *ip)
{
  struct ilist *p;
  fprintf(fp, "(");
  for (p = ip; p; p = p->next)
    fprintf(fp, "%d%s", p->i, (p->next ? " " : ""));
  fprintf(fp, ")");
}  /* print_ilist */

/*************
 *
 *   p_ilist(ip)
 *
 *************/

void p_ilist(struct ilist *ip)
{
  print_ilist(stdout, ip);
  printf("\n");
}  /* p_ilist */

/* ============================================================ glist  */

/*************
 *
 *   glist_length()
 *
 *************/

int glist_length(struct glist *a)
{
  if (a == NULL)
    return 0;
  else
    return 1 + glist_length(a->next);
}  /* glist_length */

/*************
 *
 *   copy_glist() -- don't copy objects (how could we??)
 *
 *************/

struct glist *copy_glist(struct glist *p)
{
  if (p == NULL)
    return NULL;
  else {
    struct glist *q = get_glist();
    q->v = p->v;
    q->next = copy_glist(p->next);
    return q;
  }
}  /* copy_glist */

/*************
 *
 *   glist_append() -- this uses up both arguments
 *
 *************/

struct glist *glist_append(struct glist *a, struct glist *b)
{
  if (a == NULL)
    return b;
  else {
    a->next = glist_append(a->next, b);
    return a;
  }
}  /* glist_append */

/*************
 *
 *   glist_prepend() -- put a pointer onto the front.
 *
 *************/

struct glist *glist_prepend(void *p, struct glist *a)
{
  struct glist *b = get_glist();
  b->v = p;
  b->next = a;
  return b;
}  /* glist_prepend */

/*************
 *
 *   glist_tack_on()
 *
 *************/

struct glist *glist_tack_on(struct glist *a, void *v)
{
  if (a == NULL) {
    struct glist *b = get_glist();
    b->v = v;
    return b;
  }
  else {
    a->next = glist_tack_on(a->next, v);
    return a;
  }
}  /* glist_tack_on */

/*************
 *
 *   free_glist_list()
 *
 *************/

void free_glist_list(struct glist *p)
{
  if (p != NULL) {
    free_glist_list(p->next);
    free_glist(p);
  }
}  /* free_glist_list */

/* ============================================================ g2list  */

/*************
 *
 *   g2list_length()
 *
 *************/

int g2list_length(struct g2list *a)
{
  if (a == NULL)
    return 0;
  else
    return 1 + g2list_length(a->next);
}  /* g2list_length */

/* ===================================================== glist and ilist  */

/*************
 *
 *   member_is_subset()
 *
 *************/

int member_is_subset(struct glist *a, struct ilist *s)
{
  if (a == NULL)
    return 0;
  else if (iset_subset(a->v, s))
    return 1;
  else
    return member_is_subset(a->next, s);
}  /* member_is_subset */

/*************
 *
 *   copy_glist_of_ilists()
 *
 *************/

struct glist *copy_glist_of_ilists(struct glist *a)
{
  if (a == NULL)
    return NULL;
  else {
    struct glist *b = get_glist();
    b->v = copy_ilist(a->v);
    b->next = copy_glist_of_ilists(a->next);
    return b;
  }
}  /* copy_glist_of_ilists */

/*************
 *
 *   free_glist_of_ilists()
 *
 *************/

void free_glist_of_ilists(struct glist *p)
{
  if (p != NULL) {
    free_glist_of_ilists(p->next);
    free_ilist_list(p->v);
    free_glist(p);
  }
}  /* free_glist_of_ilists */

