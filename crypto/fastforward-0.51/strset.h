#ifndef STRSET_H
#define STRSET_H

#include "uint32.h"

typedef struct strset_list
 {
  uint32 h;
  int next;
 }
strset_list;

typedef struct
 {
  int mask; /* mask + 1 is power of 2, size of hash table */
  int n; /* number of entries used in list and x */
  int a; /* number of entries allocated in list and x */
  int *first; /* first[h] is front of hash list h */
  strset_list *p; /* p[i].next is next; p[i].h is hash of x[i] */
  char **x; /* x[i] is entry i */
 }
strset;

extern uint32 strset_hash();
extern int strset_init();
extern char *strset_in();
extern int strset_add();

#endif
