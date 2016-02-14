/*
 *  ivy.c - part of the otter/ivy interface.
 *
 */

#include "header.h"
#include "lisp.h"
#include "check.h"

static struct proof_object *Initial_proof_object = NULL;

/*************************************************************************/

static int bnode_to_natural(Bnode b)
{
  if (!atom(b))
    return -1;
  else {
    int i;
    if (str_int(b->label, &i))
      return (i < 0 ? -1 : i);
    else
      return -1;
  }
}  /* bnode_to_natural */

/*************************************************************************/

static struct term *bnode_to_otterterm(Bnode b,
				       char **varnames)
{
  if (atom(b)) {
    int i = 0;
    char *str;
    struct term *t = get_term();
    t->type = VARIABLE;
    t->sym_num = str_to_sn(b->label, 0);
    str = sn_to_str(t->sym_num);
    while (i < MAX_VARS && varnames[i] != NULL &&
	   varnames[i] != sn_to_str(t->sym_num))
      i++;
    if (i == MAX_VARS)
      return NULL;
    else {
      if (varnames[i] == NULL)
	varnames[i] = sn_to_str(t->sym_num);
      t->varnum = i;
    }
    return t;
  }
  else if (length(b) == 1) {
    struct term *t = get_term();
    t->type = NAME;
    t->sym_num = str_to_sn(car(b)->label, 0);
    return t;
  }
  else {
    struct rel *r1, *r2;
    char *label = car(b)->label;
    int arity = length(b) - 1;
    struct term *t = get_term();
    t->type = COMPLEX;
    t->sym_num = str_to_sn(label, arity);
    
    r2 = NULL;
    for (b = cdr(b) ; !atom(b); b = cdr(b)) {
      r1 = get_rel();
      r1->argval = bnode_to_otterterm(b->car, varnames);
      if (r2 == NULL)
	t->farg = r1;
      else
	r2->narg = r1;
      r2 = r1;
    }
    return t;
  }
}  /* bnode_to_otterterm */

/*************************************************************************/
/* This is different from is_symbol in that it works for variables. */

int special_is_symbol(struct term *t, char *str, int arity)
{
  return(sn_to_arity(t->sym_num) == arity &&
	 str_ident(sn_to_str(t->sym_num), str));
}  /* special_is_symbol */

/*************************************************************************/

void trans_logic_symbols(struct term *t)
{
  if (special_is_symbol(t, "TRUE", 0)) {
    t->sym_num = str_to_sn("$T", 0);
    t->type = NAME;
  }
  else if (special_is_symbol(t, "FALSE", 0)) {
    t->sym_num = str_to_sn("$F", 0);
    t->type = NAME;
  }
  else if (is_symbol(t, "NOT", 1)) {
    t->sym_num = str_to_sn("-", 1);
    trans_logic_symbols(t->farg->argval);
  }
  else if (is_symbol(t, "OR", 2)) {
    t->sym_num = str_to_sn("|", 2);
    trans_logic_symbols(t->farg->argval);
    trans_logic_symbols(t->farg->narg->argval);
  }
}  /* trans_logic_symbols */

/*************************************************************************/

static struct clause *bnode_to_clause(Bnode b)
{
  struct term *t;
  char *varnames[MAX_VARS];
  int i;

  for (i=0; i<MAX_VARS; i++)
    varnames[i] = NULL;
  t = bnode_to_otterterm(b, varnames);
  if (t == NULL)
    return NULL;
  else {
    struct clause *c;
    trans_logic_symbols(t);
    c = term_to_clause(t);
    /* lit_t_f_reduce(c);  don't do this, because of test in derive */
    return c;
  }
}  /* bnode_to_clause */

/*************************************************************************/

struct proof_object *parse_initial_proof_object(FILE *fp)
{
  struct proof_object *po = get_proof_object();
  Bnode lisp_proof_object = parse_lisp(fp);
  Bnode b;
  Bnode lisp_step;
  if (lisp_proof_object == NULL)
    abend("parse_proof_object: parse_listp returns NULL");
  if (!true_listp(lisp_proof_object))
    abend("parse_proof_object: parse_listp nonlist");

  for (b = lisp_proof_object; !nullp(b); b = b->cdr) {
    struct proof_object_node *pn = connect_new_node(po);
    Bnode e1, e2, e3;
    char *label;
    lisp_step = b->car;
    if (length(lisp_step) < 3)
      abend("parse_proof_object: step length < 3");
    e1 = car(lisp_step);
    e2 = cadr(lisp_step);
    e3 = caddr(lisp_step);
    pn->id = bnode_to_natural(e1);
    if (length(e2) < 1 || !atom(car(e2)))
      abend("parse_proof_object: bad justification (1)");
    label = car(e2)->label;
    if (str_ident(label, "INPUT"))
      pn->rule = P_RULE_INPUT;
    else if (str_ident(label, "EQ-AXIOM"))
      pn->rule = P_RULE_EQ_AXIOM;
    else
      abend("parse_proof_object: bad justification (2)");
    pn->c = bnode_to_clause(e3);
    if (!pn->c)
      abend("parse_proof_object: NULL clause");
  }  /* for each step */
  return po;
}  /* parse_proof_object */

/*************************************************************************/

struct list *init_proof_object(FILE *fin,
			       FILE *fout)
{
  struct proof_object *obj;
  init_proof_object_environment();
  obj = parse_initial_proof_object(fin);
  if (obj == NULL) {
    fprintf(fout, "error parsing initial proof object\n");
    return NULL;
  }
  else {
    struct proof_object_node *pn;
    struct list *lst = get_list();
    print_proof_object(fout, obj);
    for (pn = obj->first; pn != NULL; pn = pn->next)
      append_cl(lst, cl_copy(pn->c));  /* this gets rid of variable names */
    Initial_proof_object = obj;  /* save it for construction of final version */
    return lst;
  }
}  /* init_proof_object */

/*************************************************************************/

struct proof_object *retrieve_initial_proof_object(void)
{
  return Initial_proof_object;
}  /* retrieve_initial_proof_object */
