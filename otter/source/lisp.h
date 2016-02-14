/*
  This is a dirty little parser for LISP-like expressions.  It is meant
  to to parse expressions written by machine.

  * Comments are not recognized.
  * Almost any token is accepted as an atom.

  * Dot expressions are ok.
  * Extra whitespace is ok.

  * If anything goes wrong, exit with a message goes to stderr.

  */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>  /* for malloc, free */

#if 1
#define BOOLEAN char
#define FALSE 0
#define TRUE 1
#else
typedef enum { FALSE=0, TRUE=1 } BOOL;
#endif

typedef struct bnode * Bnode;

struct bnode {
  Bnode car;
  Bnode cdr;
  BOOLEAN atom;
  char *label;
};

/* Prototypes from lisp.c */

void zap_btree(Bnode p);
BOOLEAN true_listp(Bnode p);
void fprint_btree(FILE *fp, Bnode p);
void p_btree(Bnode p);

BOOLEAN nullp(Bnode p);
Bnode parse_lisp(FILE *fp);
int atom(Bnode p);
Bnode car(Bnode p);
Bnode cdr(Bnode p);
Bnode cadr(Bnode p);
Bnode caddr(Bnode p);
int length(Bnode p);

