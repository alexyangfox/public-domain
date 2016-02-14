/*
 *  check.c -- code related to proof objects and proof checking
 *
 */

#include "header.h"
#include "check.h"

/* #define DEBUG */

#define ATTEMPT_FEB_9

/* Gen_tab is a general hash table method.  You can insert (id,pointer)
 * pairs and retrieve the pointer associated with a particular id.
 */

#define GEN_TAB_SIZE 100

struct gen_node {
  int id;
  void *v;
  struct gen_node *next;
};

struct gen_tab {
  struct gen_node *a[GEN_TAB_SIZE];
};

/* Global variables */

static struct gen_tab *Old_proof_tab;
static struct gen_tab *New_proof_old_id_tab;
static struct gen_tab *New_proof_tab;

/*************
 *
 *   get_gen_node()
 *
 *************/

struct gen_node *get_gen_node(void)
{
  struct gen_node *p;
  p = (struct gen_node *) tp_alloc((int) sizeof(struct gen_node));
  p->id = 0;
  p->v = NULL;
  return(p);
}  /* get_gen_node */

/*************
 *
 *   get_proof_object()
 *
 *************/

struct proof_object *get_proof_object(void)
{
  struct proof_object *p;
  p = (struct proof_object *) tp_alloc((int) sizeof(struct proof_object));
  p->steps = 0;
  p->first = NULL;
  p->last = NULL;
  return(p);
}  /* get_proof_object */

/*************
 *
 *   get_proof_object_node()
 *
 *************/

struct proof_object_node *get_proof_object_node(void)
{
  struct proof_object_node *p;
  int i;
  p = (struct proof_object_node *) tp_alloc((int) sizeof(struct proof_object_node));
  p->id = 0;
  p->old_id = 0;
  p->parent1 = 0;
  p->parent2 = 0;
  p->position1 = NULL;
  p->position2 = NULL;
  p->map = NULL;
  p->rule = P_RULE_UNDEFINED;
  p->backward_subst = FALSE;
  for (i = 0; i < 2*MAX_VARS; i++)
    p->subst[i] = NULL;
  p->next = NULL;
  return(p);
}  /* get_proof_object_node */

/*************
 *
 *   init_gen_tab()
 *
 *************/

static struct gen_tab *init_gen_tab(void)
{
  int i;
  struct gen_tab *p;
  p = (struct gen_tab *) tp_alloc(GEN_TAB_SIZE * (int) sizeof(void *));
  for (i = 0; i < GEN_TAB_SIZE; i++)
    p->a[i] = NULL;
  return(p);
}  /* init_gen_tab */

/*************
 *
 *   insert_into_gen_tab()
 *
 *   If ok, return 1; if already there, return 0.
 *
 *************/

static int insert_into_gen_tab(struct gen_tab *tab,
			       int id,
			       void *v)
{
  struct gen_node *p1, *p2, *p3;

  p1 = tab->a[id%GEN_TAB_SIZE];
  p2 = NULL;
  while (p1 && p1->id < id) {
    p2 = p1;
    p1 = p1->next;
  }
  if (p1 && p1->id == id)
    abend("insert_into_gen_tab, key already there");
  else {
    p3 = get_gen_node();
    p3->id = id;
    p3->v = v;
    p3->next = p1;
    if (p2)
      p2->next = p3;
    else
      tab->a[id%GEN_TAB_SIZE] = p3;
  }
  return(1);
}  /* insert_into_gen_tab */

/*************
 *
 *   retrieve_from_gen_tab()
 *
 *   Return NULL if it is not there.
 *
 *************/

static void * retrieve_from_gen_tab(struct gen_tab *tab,
				    int id)
{
  struct gen_node *p;

  p = tab->a[id%GEN_TAB_SIZE];
  while (p && p->id < id)
    p = p->next;
  if (p && p->id == id)
    return(p->v);
  else
    return((void *) NULL);
}  /* retrieve_from_gen_tab */

static void p_gen_tab(struct gen_tab *tab)
{
  struct gen_node *p;
  int i;
  for (i = 0; i < GEN_TAB_SIZE; i++) {
    for (p = tab->a[i]; p; p = p->next) {
      printf("%d: ", i);
      p_proof_object_node(p->v);
    }
  }
}  /* p_gen_tab */

/*************
 *
 *    int check_eq_lit(lit)
 *
 *************/

static int check_eq_lit(struct literal *lit)
{
  return(lit->atom->varnum == POS_EQ ||
	 lit->atom->varnum == NEG_EQ ||
	 lit->atom->varnum == LEX_DEP_DEMOD);
}  /* check_eq_lit */

/*************
 *
 *   trivial_subst()
 *
 *   A substitution is trivial if every (nonempty) entry is
 *         "vi -> vi (NULL context)"
 *
 *************/

int trivial_subst(struct context *c)
{
  int i, ok;

  if (c->multiplier != 0)
    ok = 0;
  else for (i = 0, ok = 1; i < MAX_VARS && ok; i++) {
    if (c->terms[i]) {
      if (c->terms[i]->type != VARIABLE)
	ok = 0;
      else if (c->terms[i]->varnum != i)
	ok = 0;
      else
	ok = (c->contexts[i] == NULL);
    }
  }
  return(ok);
}  /* trivial_subst */

/*************
 *
 *   connect_new_node()
 *
 *   Add a new node to the end of the proof object; return the node.
 *
 *************/

struct proof_object_node *connect_new_node(struct proof_object *new_proof)
{
  struct proof_object_node *pn;
  int rc;

  pn = get_proof_object_node();
  if (new_proof->steps == 0) {
    new_proof->first = pn;
    new_proof->last = pn;
  }
  else {
    new_proof->last->next = pn;
    new_proof->last = pn;
  }
  new_proof->steps++;
  pn->id = new_proof->steps;
  rc = insert_into_gen_tab(New_proof_tab, pn->id, pn);
  return(pn);
}  /* connect_new_node */

/*************
 *
 *   po_rule()
 *
 *   Return a poitner to the char string corresponding to the integer
 *   code for a proof object rule.
 *
 *************/

static char *po_rule(int rule)
{
  switch (rule) {
  case P_RULE_UNDEFINED: return("undefined");
  case P_RULE_INPUT: return("input");
  case P_RULE_EQ_AXIOM: return("eq-axiom");
  case P_RULE_INSTANTIATE: return("instantiate");
  case P_RULE_PROPOSITIONAL: return("propositional");
  case P_RULE_RESOLVE: return("resolve");
  case P_RULE_PARAMOD: return("paramod");
  case P_RULE_FLIP: return("flip");
  default: return("unknown");
  }
}  /* po_rule */

/*************
 *
 *   print_term_s()
 *
 *   Print a term, S-expression style.
 *
 *************/

void print_term_s(FILE *fp,
		  struct term *t)
{
  if (t->type == VARIABLE) {
    if (t->sym_num != 0)
      fprintf(fp, "%s", sn_to_str(t->sym_num));
    else
      fprintf(fp, "v%d", t->varnum);
  }
  else {
    struct rel *r;
    fprintf(fp, "(%s", sn_to_str(t->sym_num));
    for (r = t->farg; r; r = r->narg) {
      fprintf(fp, " ");
      print_term_s(fp, r->argval);
    }
    fprintf(fp, ")");
  }
}  /* print_term_s */

/*************
 *
 *   p_term_s()
 *
 *************/

void p_term_s(struct term *t)
{
  print_term_s(stdout, t);
}  /* print_term_s */

/*************
 *
 *   print_clause_s()
 *
 *   Print a clause, S-expression style (without a newline).
 *
 *************/

void print_clause_s(FILE *fp,
		    struct clause *c)
{
  struct literal *lit;
  fprintf(fp, "(");
  for (lit = c->first_lit; lit; lit = lit->next_lit) {
    if (!lit->sign)
      fprintf(fp, "(not ");
    print_term_s(fp, lit->atom);
    if (!lit->sign)
      fprintf(fp, ")");
    if (lit->next_lit)
      fprintf(fp, " ");
  }
  fprintf(fp, ")");
  fflush(fp);
}  /* print_clause_s */

/*************
 *
 *   p_clause_s()
 *
 *************/

void p_clause_s(struct clause *c)
{
  print_clause_s(stdout, c);
}  /* print_clause_s */

/*************
 *
 *   print_clause_s2()
 *
 *   Print a clause, S-expression style (without a newline).
 *
 *************/

void print_clause_s2(FILE *fp,
		     struct clause *c)
{
  if (c->first_lit == NULL)
    fprintf(fp, "false");
  else {
    struct literal *lit;

    for (lit = c->first_lit; lit; lit = lit->next_lit) {
      if (lit->next_lit)
	fprintf(fp, "(or ");
      if (!lit->sign)
	fprintf(fp, "(not ");

      if (is_symbol(lit->atom, "$T", 0))
	fprintf(fp, "true");
      else if (is_symbol(lit->atom, "$F", 0))
	fprintf(fp, "false");
      else
	print_term_s(fp, lit->atom);

      if (!lit->sign)
	fprintf(fp, ")");
      if (lit->next_lit)
	fprintf(fp, " ");
    }
    for (lit = c->first_lit->next_lit; lit; lit = lit->next_lit)
      fprintf(fp, ")");
  }
  fflush(fp);
}  /* print_clause_s2 */

/*************
 *
 *   p_clause_s2()
 *
 *************/

void p_clause_s2(struct clause *c)
{
  print_clause_s2(stdout, c);
}  /* print_clause_s2 */

/*************
 *
 *   print_proof_object_node()
 *
 *************/

void print_proof_object_node(FILE *fp,
			     struct proof_object_node *pn)
{
  int i;

  fprintf(fp, "(%d ", pn->id);
  fprintf(fp, "(%s", po_rule(pn->rule));

  switch (pn->rule) {
  case P_RULE_INPUT:
  case P_RULE_EQ_AXIOM:
    fprintf(fp, ") ");
    break;
  case P_RULE_INSTANTIATE:
    fprintf(fp, " %d (", pn->parent1);
    for (i = 0; i < 2*MAX_VARS; i++) {
      if (pn->subst[i]) {
	if (pn->backward_subst) {
	  fprintf(fp, "(");
	  print_term_s(fp, pn->subst[i]);
	  fprintf(fp, " . v%d)", i);
	}
	else {
	  fprintf(fp, "(v%d . ", i);
	  print_term_s(fp, pn->subst[i]);
	  fprintf(fp, ")");
	}
      }
    }
    fprintf(fp, ")) ");
    break;
  case P_RULE_RESOLVE:
  case P_RULE_PARAMOD:
    fprintf(fp, " %d ", pn->parent1);
    print_ilist(fp, pn->position1);
    fprintf(fp, " %d ", pn->parent2);
    print_ilist(fp, pn->position2);
    fprintf(fp, ") ");
    break;
  case P_RULE_PROPOSITIONAL:
    fprintf(fp, " %d) ", pn->parent1);
    break;
  case P_RULE_FLIP:
    fprintf(fp, " %d ", pn->parent1);
    print_ilist(fp, pn->position1);
    fprintf(fp, ") ");
    break;
  default:
    abend("print_proof_object_node, bad rule");
    break;
  }

  if (Flags[BUILD_PROOF_OBJECT_2].val)
    print_clause_s2(fp, pn->c);
  else
    print_clause_s(fp, pn->c);
  if (pn->old_id != 0)
    fprintf(fp, " (%d)", pn->old_id);      /* Otter ID */
  else
    fprintf(fp, " NIL");      /* Otter ID */
  fprintf(fp, ")\n");
  fflush(fp);
}  /* print_proof_object_node */

/*************
 *
 *   p_proof_object_node()
 *
 *************/

void p_proof_object_node(struct proof_object_node *pn)
{
  print_proof_object_node(stdout, pn);
}  /* print_proof_object_node */

/*************
 *
 *   print_proof_object()
 *
 *************/

void print_proof_object(FILE *fp,
			struct proof_object *po)
{
  struct proof_object_node *pn;
  fprintf(fp, "(\n");
  for (pn = po->first; pn; pn = pn->next)
    print_proof_object_node(fp, pn);
  fprintf(fp, ")\n");
}  /* print_proof_object */

/*************
 *
 *   p_proof_object()
 *
 *************/

void p_proof_object(struct proof_object *po)
{
  print_proof_object(stdout, po);
}  /* print_proof_object */

/*************
 *
 *   new_literal_index()
 *
 *   Given a list of integers and a value v, return the position of v.
 *
 *************/

static int new_literal_index(struct ilist *ip,
			     int v)
{
  int i;
  for (i = 1; ip && abs(ip->i) != v; ip = ip->next, i++);
  if (!ip)
    abend("new_literal_index, bad map");
  return(i);
}  /* new_literal_index */

/*************
 *
 *   copy_subst_to_proof_object()
 *
 *************/

static void copy_subst_to_proof_object(struct context *subst,
				       struct proof_object_node *p)
{
  struct term *t;
  int i;

  t = get_term();
  t->type = VARIABLE;

  /* If multiplier=0, only need to copy nonNULL entries, because the
   * variables won't change.
   */

  if (subst->multiplier == 0) {
    for (i = 0; i < MAX_VARS; i++) {
      if (subst->terms[i]) {
	t->varnum = i;
	p->subst[i] = apply(t, subst);
      }
    }
  }
  else {
    int max;
    struct proof_object_node *parent;

    parent = retrieve_from_gen_tab(New_proof_tab, p->parent1);
    max = biggest_var_clause(parent->c);
    for (i = 0; i <= max; i++) {
      t->varnum = i;
      p->subst[i] = apply(t, subst);
    }

  }
  free_term(t);
}  /* copy_subst_to_proof_object */

/*************
 *
 *   cl_copy_delete_literal()
 *
 *************/

struct clause *cl_copy_delete_literal(struct clause *c,
				      int n)
{
  struct clause *d;
  struct literal *l1, *l2;
  int i;

  d = cl_copy(c);
  for (l1 = d->first_lit, l2 = NULL, i = 1;
       i < n; 
       l2 = l1, l1 = l1->next_lit, i++);

  if (l2)
    l2->next_lit = l1->next_lit;
  else 
    d->first_lit = l1->next_lit;
  return(d);
}  /* cl_copy_delete_literal */

/*************
 *
 *    int variant(t1, c1, t2, c2, trail_address, flip) -- alphabetic variant
 *
 *    I take a shortcut and just call `match' twice.  If speed is a
 *    concern, this routine should be rewritten.
 *
 *    if (flip), t1 has arity 2 and should be flipped before test.
 *
 *    WARNING!! if you use the substitutions for anything, use either one,
 *    but don't use both.  This is because, for example, when given
 *    p(x), p(y), x is bound to y and y is bound to x!
 *
 *    The use of the trail is the same as in `unify'.
 *
 *************/

int variant(struct term *t1,
	    struct context *c1,
	    struct term *t2,
	    struct context *c2,
	    struct trail **trp,
	    int flip)
{
  struct trail *tpos;
  struct term *tt;
  int rc;

  if (flip) {
    t1 = copy_term(t1);
    tt = t1->farg->argval;
    t1->farg->argval = t1->farg->narg->argval;
    t1->farg->narg->argval = tt;
  }
  tpos = *trp;
  if (match(t1, c1, t2, trp)) {
    if (match(t2, c2, t1, trp))
      rc = 1;
    else {
      clear_subst_2(*trp, tpos);
      *trp = tpos;
      rc = 0;
    }
  }
  else
    rc = 0;
    
  if (flip)
    zap_term(t1);
  return(rc);
}  /* variant */

/*************
 *
 *   match_literals()
 *
 *   Note that literals are indexed starting with 1, not 0.
 *
 *************/

static int match_literals(struct clause *c1,
			  struct context *s1,
			  int *m1,
			  struct clause *c2,
			  struct context *s2,
			  int *m2,
			  struct trail **trp)
{
  struct literal *l1, *l2;
  int i1, i2, matched, flip;
  struct trail *t_pos;

  /* Find the first unmatched literal of c2. */
  for (l2 = c2->first_lit, i2=1; l2 && m2[i2]!=0; l2 = l2->next_lit, i2++);
  if (!l2)
    return(1);  /* Success.  All literals matched. */
  else {
    matched = 0;
    flip = 0;
    i1 = 1;
    l1 = c1->first_lit;
    while (l1 && !matched) {
      t_pos = *trp;  /* save position in trail in case of failure */
      if (m1[i1]==0 && l1->sign == l2->sign &&
	  variant(l1->atom, s1, l2->atom, s2, trp, flip)) {
	m1[i1] = (flip ? -i2: i2);
	m2[i2] = (flip ? -i1: i1);
	if (match_literals(c1, s1, m1, c2, s2, m2, trp))
	  matched = 1;
	else {
	  m1[i1] = 0;
	  m2[i2] = 0;
	  clear_subst_2(*trp, t_pos);
	  *trp = t_pos;
	}
      }
      /* increment */
      if (check_eq_lit(l1) && check_eq_lit(l2) && !flip) {
	flip = 1;
      }
      else {
	l1 = l1->next_lit;
	i1++;
	flip = 0;
      }
    }
    return(matched);
  }
}  /* match_literals */

/*************
 *
 *   match_clauses(c1, c2)
 *
 *   This routine takes 2 clauses that are supposed to be
 *   alphabetic variants (if not, return NULL).  The literals
 *   may be in different orders, and equality literals may
 *   be flipped.  We find the correspondence and
 *   return it as a list of integers: the i-th integer is the
 *   index in c1 of the  i-th literal in c2.  If an equality
 *   literal is flipped, the index is negated.
 *
 *************/

struct ilist *match_clauses(struct clause *c1,
			      struct clause *c2)
{
  int m1[MAX_LITS+1], m2[MAX_LITS+1];
  struct context *s1, *s2;
  struct trail *tr;
  int i, rc, n1, n2;
  struct ilist *ip1, *ip2;

  n1 = num_literals(c1);
  n2 = num_literals(c2);
  if (n1 != n2)
    abend("match_clauses, different numbers of literals");

  s1 = get_context();
  s2 = get_context();
    
  for (i = 1; i <= MAX_LITS; i++) {
    m1[i] = 0;
    m2[i] = 0;
  }

  tr = NULL;
  rc = match_literals(c1, s1, m1, c2, s2, m2, &tr);
  if (!rc) {
    abend("match_clauses, literals don't match");
  }
  clear_subst_1(tr);
  free_context(s1);
  free_context(s2);

#ifdef DEBUG
  printf("\nmatch_clauses rc=%d\n", rc);
  for (i = 1; i <= MAX_LITS; i++)
    printf("%3d", m1[i]);
  p_clause(c1);
  for (i = 1; i <= MAX_LITS; i++)
    printf("%3d", m2[i]);
  p_clause(c2);
#endif

  ip1 = NULL;
  for (i = n1; i > 0; i--) {
    ip2 = get_ilist();
    ip2->i = m2[i];
    ip2->next = ip1;
    ip1 = ip2;
  }
  return(ip1);
}  /* match_clauses */

/*************
 *
 *   cl_append()
 *
 *************/

struct clause *cl_append(struct clause *c1,
			 struct clause *c2)
{
  struct literal *curr, *prev;

  for (curr = c1->first_lit, prev = NULL;
       curr;
       prev = curr, curr = curr->next_lit);

  if (prev)
    prev->next_lit = c2->first_lit;
  else
    c1->first_lit = c2->first_lit;

  for (curr = c2->first_lit; curr; curr = curr->next_lit)
    curr->container = c1;

  free_clause(c2);
  return(c1);
}  /* cl_append */

/*************
 *
 *   identity_resolve()
 *
 *************/

struct clause *identity_resolve(struct clause *c1,
				int i1,
				struct clause *c2,
				int i2)
{
  struct clause *d1, *d2, *res;

  d1 = cl_copy_delete_literal(c1, i1);
  d2 = cl_copy_delete_literal(c2, i2);
  res = cl_append(d1, d2);
  return(res);
    
}  /* identity_resolve */

/*************
 *
 *   identity_paramod()
 *
 *************/

static struct clause *identity_paramod(struct clause *from_cl,
				       struct ilist *from_pos,
				       struct clause *into_cl,
				       struct ilist *into_pos)
{
  struct clause *into_prime, *from_prime, *para;
  struct literal *from_lit, *into_lit;
  struct term *beta, *t;
  struct ilist *ip;
  struct rel *r;
  int i;

  from_lit = ith_literal(from_cl, from_pos->i);
  if (from_pos->next->i == 1)
    beta = from_lit->atom->farg->narg->argval;
  else
    beta = from_lit->atom->farg->argval;

  into_prime = cl_copy(into_cl);
  into_lit = ith_literal(into_prime, into_pos->i);

  /* Get the into term. */

  for (ip = into_pos->next, t = into_lit->atom; ip; ip = ip->next) {
    for (i = 1, r = t->farg; i < ip->i; i++, r = r->narg);
    t = r->argval;
  }

  /* Now r points at into term t. */

  r->argval = copy_term(beta);

  from_prime = cl_copy_delete_literal(from_cl, from_pos->i);
  para = cl_append(from_prime, into_prime);

  return(para);

}  /* identity_paramod */

/*************
 *
 *    void renumber_vars_subst()
 *
 *************/

void renumber_vars_subst(struct clause *c,
			 struct term **terms)
{
  struct literal *lit;
  int varnums[MAX_VARS];
  int i, ok;
  struct term *t;

  ok = 1;
  for (i = 0; i < MAX_VARS; i++)
    varnums[i] = -1;

  lit = c->first_lit;
  while (lit) {
    if (renum_vars_term(lit->atom, varnums) == 0)
      ok = 0;
    lit = lit->next_lit;
  }

  for (i = 0; i < MAX_VARS; i++) {
    if (varnums[i] != -1 && varnums[i] != i) {
      t = get_term();
      t->type = VARIABLE;
      t->varnum = i;
      terms[varnums[i]] = t;
    }
  }
}  /* renumber_vars_subst */

/*************
 *
 *   translate_unit_deletion()
 *
 *************/

static int translate_unit_deletion(struct proof_object_node *current,
				   struct proof_object_node *unit,
				   struct proof_object *new_proof)
{
  struct literal *l1, *l2;
  struct proof_object_node *instance, *resolvent;
  struct context *subst;
  struct trail *tr;
  int found, index;
  struct ilist *ip1, *ip2;

  /* First, find the literal that is deleted. */

  subst = get_context(); subst->multiplier = 0;
  l2 = unit->c->first_lit;
  for (l1 = current->c->first_lit, found = 0, index = 0;
       l1 && !found;
       l1 = l1->next_lit, index++) {
    tr = NULL;
    if (l2->sign != l1->sign && match(l2->atom,subst,l1->atom,&tr)) {
      found = 1;
    }
  }
  if (!found) 
    abend("translate_unit_deletion, unit deletion not found");

  /* Set up a new proof object node for the instantiation. */

  if (trivial_subst(subst))
    instance = unit;
  else {
    instance = connect_new_node(new_proof);
    instance->rule = P_RULE_INSTANTIATE;
    instance->parent1 = unit->id;
    instance->c = apply_clause(unit->c, subst);
    copy_subst_to_proof_object(subst, instance);
  }

  clear_subst_1(tr);
  free_context(subst);

  /* Set up a node for the resolution (negative lit. always first parent). */

  resolvent = connect_new_node(new_proof);
  resolvent->rule = P_RULE_RESOLVE;
  ip1 = get_ilist(); resolvent->position1 = ip1;
  ip2 = get_ilist(); resolvent->position2 = ip2;
  if (l2->sign) {  /* unit positive */
    resolvent->parent1 = current->id;
    resolvent->parent2 = instance->id;
    ip1->i = index;
    ip2->i = 1;
  }
  else {
    resolvent->parent1 = instance->id;
    resolvent->parent2 = current->id;
    ip1->i = 1;
    ip2->i = index;
  }

  /* Copy the clause then delete the correct literal. */

  resolvent->c = cl_copy_delete_literal(current->c, index);

  return(1);

} /* translate_unit_deletion */

/*************
 *
 *   translate_factor_simp()
 *
 *   Apply the first factor_simp operation.  Note that a sequence of 
 *   factor_simps may be applied in a different order in the new proof,
 *   because the order of literals can be different.  This should be OK.
 *
 *************/

static int translate_factor_simp(struct proof_object_node *current,
				 struct proof_object *new_proof)
{
  struct literal *lit1, *lit2;
  struct clause *factor;
  struct context *subst;
  struct trail *tr;
  struct proof_object_node *previous, *instance, *merge;
  int rc;

  /* May have to renumber the variables. */

  if (biggest_var_clause(current->c) >= MAX_VARS) {
    previous = current;
    current = connect_new_node(new_proof);
    current->rule = P_RULE_INSTANTIATE;
    current->parent1 = previous->id;
    current->c = cl_copy(previous->c);
    renumber_vars_subst(current->c, current->subst);
  }

  lit1 = NULL; lit2 = NULL;
  factor = first_or_next_factor(current->c, &lit1, &lit2);
  while (factor && ! subsume(factor, current->c)) {
    cl_del_non(factor);
    factor = first_or_next_factor(current->c, &lit1, &lit2);
  }
  if (!factor)
    abend("translate_factor_simp, factor not found");

  cl_del_non(factor);

  subst = get_context();
  subst->multiplier = 0;
  tr = NULL;
  rc = unify(lit1->atom, subst, lit2->atom, subst, &tr);
  if (!rc)
    abend("translate_factor_simp, literals don't unify");
    
  if (trivial_subst(subst))
    instance = current;
  else {
    instance = connect_new_node(new_proof);
    instance->rule = P_RULE_INSTANTIATE;
    instance->parent1 = current->id;
    instance->c = apply_clause(current->c, subst);
    copy_subst_to_proof_object(subst, instance);
  }

  clear_subst_1(tr);
  free_context(subst);

  /* Build the merge node. */

  merge = connect_new_node(new_proof);
  merge->rule = P_RULE_PROPOSITIONAL;
  merge->parent1 = instance->id;
  merge->position1 = get_ilist();
  merge->position1->i = literal_number(lit2);
  merge->c = cl_copy_delete_literal(instance->c, literal_number(lit2));

  if (num_literals(instance->c)-1 != num_literals(merge->c))
    abend("translate_factor_simp: merge failed");

  return(1);
	
}  /* translate_factor_simp */

/*************
 *
 *   first_rewrite()
 *
 *************/

static struct ilist *first_rewrite(struct term *t,
				     struct ilist *pos,
				     struct clause *c,
				     struct clause_ptr *demods,
				     struct context *subst,
				     struct trail **trp,
				     int *demod_id)
{
  struct ilist *prev, *last, *pos_ok;
  struct rel *r;
  struct clause_ptr *cp;
  int ok;
  struct term *atom;
    
  if (t->type == COMPLEX) {
    for (prev = pos; prev->next; prev = prev->next);
    last = get_ilist();
    last->i = 1;
    prev->next = last;
    for (r = t->farg; r; r = r->narg, last->i++) {
      pos_ok = first_rewrite(r->argval,pos,c,demods,subst,trp,demod_id);
      if (pos_ok)
	return(pos_ok);
    }
    prev->next = NULL;
    free_ilist(last);
  }
  if (t->type != VARIABLE) {
    for (cp = demods, ok = 0; cp && !ok; cp = cp->next) {
      atom = cp->c->first_lit->atom;
      ok = match(atom->farg->argval, subst, t, trp);

      if (ok && atom->varnum == LEX_DEP_DEMOD) {
	int mult_flag = 0;
	struct term *replacement;

	replacement = apply_demod(atom->farg->narg->argval, subst, &mult_flag);
	if (Flags[LRPO].val)
	  ok = lrpo_greater(t, replacement);
	else
	  ok = lex_check(replacement, t) == LESS_THAN;
	zap_term_special(replacement);
	if (!ok) {
	  clear_subst_1(*trp);
	  *trp = NULL;
	}
      }

      if (ok) {
	*demod_id = cp->c->id;
	return(pos);
      }
    }
  }
  return(NULL);
}  /* first_rewrite */

/*************
 *
 *   first_rewrite_clause()
 *
 *************/

static struct ilist *first_rewrite_clause(struct clause *c,
					    struct clause_ptr *demods,
					    struct context *subst,
					    struct trail **trp,
					    int *demod_id)
{
  struct ilist *ip1, *ip2;
  struct literal *lit;

  ip1 = get_ilist();
  ip1->i = 1;
  for (lit = c->first_lit; lit; lit = lit->next_lit, ip1->i++) {
    ip2 = first_rewrite(lit->atom, ip1, c, demods, subst, trp, demod_id);
    if (ip2)
      return(ip2);
  }
  free_ilist(ip1);
  return(NULL);
}  /* first_rewrite_clause */

/*************
 *
 *   translate_demod_nonunit()
 *
 *   The sequence of demodulators that apply to the original clause
 *   might not apply to the new clause in the same order, because
 *   the literals might be rearranged.  So we collect all of the
 *   demodulators and keep applying the set.  This method is still
 *   not perfect, because at a given term, the set of demodulators
 *   might be tried in a different order; an abend is possible.
 *
 *************/

static int translate_demod_nonunit(struct proof_object_node *current,
				   struct ilist *ip,
				   struct proof_object *new_proof)
{
  int count1, count2, demod_id;
  struct ilist *ip0, *ip1, *ip2, *ip3;
  struct clause_ptr *demods;
  struct clause *c;
  struct context *subst;
  struct trail *tr;
  struct proof_object_node *instance, *paramod, *pn;

#if 0
  printf("\ncurrent="); p_clause(current->c);
#endif
  demods = NULL;
  for (ip1 = ip, count1 = 0; ip1 && ip1->i > 0; ip1 = ip1->next, count1++) {
    c = cl_find(ip1->i);
    /* The new versions of the demodulators have to be used, because
       the variables might be numbered differently.  We have to copy
       the ID and the lex-dep flag into the new version.  Sorry this
       is so messy.
    */
    pn = retrieve_from_gen_tab(New_proof_old_id_tab, c->id);
    pn->c->id = pn->id;
    pn->c->first_lit->atom->varnum = c->first_lit->atom->varnum;
#if 0
    printf("old demod:"); p_clause(c);
    printf("new demod:"); p_clause(pn->c);
#endif
    insert_clause(pn->c, &demods);  /* If not already there. */
  }
    
  subst = get_context();  subst->multiplier = 0;
  tr = NULL;
  count2 = 0;
  ip0 = first_rewrite_clause(current->c,demods,subst,&tr,&demod_id);
  while (ip0 && count2 < count1) {
    count2++;
    pn = retrieve_from_gen_tab(New_proof_tab, demod_id);

    if (trivial_subst(subst))
      instance = pn;
    else {
      instance = connect_new_node(new_proof);
      instance->parent1 = pn->id;
      instance->rule = P_RULE_INSTANTIATE;
      instance->c = apply_clause(pn->c, subst);
      copy_subst_to_proof_object(subst, instance);
    }

    paramod = connect_new_node(new_proof);
    paramod->rule = P_RULE_PARAMOD;
    paramod->parent1 = instance->id;
    ip2 = get_ilist(); ip2->i = 1; paramod->position1 = ip2;
    ip3 = get_ilist(); ip3->i = 1; ip2->next = ip3;
    paramod->parent2 = current->id;
    paramod->position2 = ip0;
    paramod->c = identity_paramod(instance->c, paramod->position1,
				  current->c, paramod->position2);

    /* If into literal is negated, must add element to position.
     * The position vector will no longer be valid for Otter terms.
     */

    if (ith_literal(current->c, ip0->i)->sign == 0) {
      ip1 = get_ilist();
      ip1->i = 1;
      ip1->next = ip0->next;
      ip0->next = ip1;
    }

    current = paramod;
#if 0
    printf("after rewrite with %d: ", demod_id); p_clause(current->c);
#endif
    clear_subst_1(tr);
    tr = NULL;
    ip0 = first_rewrite_clause(current->c, demods, subst, &tr, &demod_id);
  }

  if (ip0 || count1 != count2) {
#if 0
    fprintf(stdout, "%d rewrites in proof, %d trans.\n", count1, count2);
    fprintf(stdout, "clause is "); print_clause(stdout, current->c);
#endif
    abend("translate_demod_nonunit, wrong number of rewrites");
  }

  return(1);
}  /* translate_demod_nonunit */

/*************
 *
 *   translate_demod_unit()
 *
 *   With units, we can apply the demodulators in the same order.
 *
 *************/

static int translate_demod_unit(struct proof_object_node *current,
				struct ilist *ip,
				struct proof_object *new_proof)
{
  int count1, demod_id;
  struct ilist *ip0, *ip1, *ip2, *ip3;
  struct clause_ptr *demods;
  struct clause *c;
  struct context *subst;
  struct trail *tr;
  struct proof_object_node *instance, *paramod, *pn;

#if 0
  printf("\ncurrent="); p_clause(current->c);
#endif
  subst = get_context();  subst->multiplier = 0;
  tr = NULL;
  for (ip1 = ip, count1 = 0; ip1 && ip1->i > 0; ip1 = ip1->next, count1++) {
    c = cl_find(ip1->i);
    /* The new versions of the demodulators have to be used, because
       the variables might be numbered differently.  We have to copy
       the ID and the lex-dep flag into the new version.  Sorry this
       is so messy.
    */
    pn = retrieve_from_gen_tab(New_proof_old_id_tab, c->id);
    pn->c->id = pn->id;
    pn->c->first_lit->atom->varnum = c->first_lit->atom->varnum;
#if 0
    printf("old demod:"); p_clause(c);
    printf("new demod:"); p_clause(pn->c);
#endif
    demods = NULL;
    insert_clause(pn->c, &demods);
    ip0 = first_rewrite_clause(current->c,demods,subst,&tr,&demod_id);
    free_clause_ptr(demods);

    if (!ip0) {
      printf("\nABOUT TO ABEND!\n");
      printf("old demod: "); p_clause(c);
      printf("new demod: "); p_clause(pn->c);
      printf("clause: "); p_clause(current->c);
      abend("translate_demod_unit: cannot rewrite");
    }

    pn = retrieve_from_gen_tab(New_proof_tab, demod_id);

    if (trivial_subst(subst))
      instance = pn;
    else {
      instance = connect_new_node(new_proof);
      instance->parent1 = pn->id;
      instance->rule = P_RULE_INSTANTIATE;
      instance->c = apply_clause(pn->c, subst);
      copy_subst_to_proof_object(subst, instance);
    }

    paramod = connect_new_node(new_proof);
    paramod->rule = P_RULE_PARAMOD;
    paramod->parent1 = instance->id;
    ip2 = get_ilist(); ip2->i = 1; paramod->position1 = ip2;
    ip3 = get_ilist(); ip3->i = 1; ip2->next = ip3;
    paramod->parent2 = current->id;
    paramod->position2 = ip0;
    paramod->c = identity_paramod(instance->c, paramod->position1,
				  current->c, paramod->position2);

    /* If into literal is negated, must add element to position.
     * The position vector will no longer be valid for Otter terms.
     */

    if (ith_literal(current->c, ip0->i)->sign == 0) {
      ip2 = get_ilist();
      ip2->i = 1;
      ip2->next = ip0->next;
      ip0->next = ip2;
    }

    current = paramod;
#if 0
    printf("after rewrite with %d: ", demod_id); p_clause(current->c);
#endif
    clear_subst_1(tr);
    tr = NULL;
  }

  return(1);
}  /* translate_demod_unit */

/*************
 *
 *   finish_translating()
 *
 *   c: the clause from the original proof; The result of this routine
 *      should be equivalent to c.
 *
 *************/

int finish_translating(struct clause *c,
		       struct ilist *rest_of_history,
		       struct proof_object_node *current,
		       struct proof_object *new_proof)
{
  int i, j, rc, ok;
  struct proof_object_node *previous, *pn;
  struct literal *l1, *l2, *prev_lit;
  struct ilist *ip1;
  struct term *t;

  /* Process rest of history here.  This code depends on the order in
   * which Otter processes generated clauses.
   * If there is any demodulation, it is first.
   */

  if (rest_of_history && rest_of_history->i == DEMOD_RULE) {
    rest_of_history = rest_of_history->next;
    if (num_literals(c) == 1)
      rc = translate_demod_unit(current, rest_of_history, new_proof);
    else
      rc = translate_demod_nonunit(current, rest_of_history, new_proof);
    while (rest_of_history && rest_of_history->i > 0)
      rest_of_history = rest_of_history->next;
    current = new_proof->last;
  }

  /* Equality reordering. */

#ifndef ATTEMPT_FEB_9
  /* I think all equality reordering is reflected in the
     justification, so we don't have to do any of it here.
  */
  if (Flags[ORDER_EQ].val) {
    struct clause *copy;
	
    copy = cl_copy(current->c);
    if (Flags[LRPO].val)
      order_equalities_lrpo(copy);
    else
      order_equalities(copy);
    for(l1 = copy->first_lit, i = 1; l1; l1 = l1->next_lit, i++) {
      if (TP_BIT(l1->atom->bits, SCRATCH_BIT)) {
	previous = current;
	current = connect_new_node(new_proof);
	current->parent1 = previous->id;
	current->position1 = get_ilist();
	current->position1->i = i;
	current->rule = P_RULE_FLIP;
	current->c = cl_copy(previous->c);
	for (l2=current->c->first_lit, j=1; j<i; l2=l2->next_lit, j++);
	t = l2->atom->farg->argval;
	l2->atom->farg->argval = l2->atom->farg->narg->argval;
	l2->atom->farg->narg->argval = t;
      }
    }
    cl_del_non(copy);
  }
#endif
    
  while (rest_of_history) {
    switch (rest_of_history->i) {

    case UNIT_DEL_RULE:
      rest_of_history = rest_of_history->next;
      while (rest_of_history && rest_of_history->i > 0) {
	pn = retrieve_from_gen_tab(New_proof_old_id_tab,
				   rest_of_history->i);
	rc = translate_unit_deletion(current, pn, new_proof);
	current = new_proof->last;
	rest_of_history = rest_of_history->next;
      }
      break;

    case FACTOR_SIMP_RULE:
      rc = translate_factor_simp(current, new_proof);
      current = new_proof->last;
      rest_of_history = rest_of_history->next;
      break;

    case FLIP_EQ_RULE:
      /* Handled elsewhere (ORDER_EQ above). */
#ifdef ATTEMPT_FEB_9
      /* This is an attempted fix. */
      i = rest_of_history->next->next->i;
      l1 = ith_literal(current->c, i);
      previous = current;
      current = connect_new_node(new_proof);
      current->parent1 = previous->id;
      current->position1 = get_ilist();
      current->position1->i = 1;
      current->rule = P_RULE_FLIP;
      current->c = cl_copy(previous->c);
      l2 = ith_literal(current->c, i);
      t = l2->atom->farg->argval;
      l2->atom->farg->argval = l2->atom->farg->narg->argval;
      l2->atom->farg->narg->argval = t;
#endif
      /* Move past flip,LIST_RULE-1,n. */
      rest_of_history = rest_of_history->next->next->next;
      break;

    case DEMOD_RULE:
      /* There are cases like this:
       *       [...,demod,...,flip,1,demod,...]
       * that arise when equality units are flipped.
       * Demodulation is handled at the beginning of this routine,
       * so the following should handle things.
       */
      rc = finish_translating(c, rest_of_history, current, new_proof);
      return(rc);

    default:
      abend("finish_translating, bad rule");
    }
  }

  /* If not ground, renumber variables. */

  if (!ground_clause(current->c)) {
    struct clause *d;
    d = cl_copy(current->c);
    rc = renumber_vars(d);
    if (!clause_ident(current->c, d)) {
      previous = current;
      current = connect_new_node(new_proof);
      current->rule = P_RULE_INSTANTIATE;
      current->parent1 = previous->id;
      current->c = cl_copy(previous->c);
      renumber_vars_subst(current->c, current->subst);
    }
    cl_del_non(d);
  }

  /* Merge identical literals (keep leftmost occurrence). */

  while (num_literals(c) < num_literals(current->c)) {
    previous = current;
    current = connect_new_node(new_proof);
    current->parent1 = previous->id;
    current->rule = P_RULE_PROPOSITIONAL;
    current->c = cl_copy(previous->c);
    ok = 0; i = 1;
    for (l1 = current->c->first_lit; l1 && !ok; l1 = l1->next_lit, i++) {
      prev_lit = l1; j = i+1;
      for (l2 = l1->next_lit; l2 && !ok; prev_lit = l2, l2 = l2->next_lit, j++) {
	if (l1->sign == l2->sign && term_ident(l1->atom, l2->atom)) {
	  ok = 1;
	  prev_lit->next_lit = l2->next_lit;
	  current->position1 = get_ilist();
	  current->position1->i = j; /* position of deleted literal */
	}
      }
    }
    if (!ok)
      abend("finish_translating, merge not found.\n");
  }

  /* Now we have to match up the original clause and the new one.
   * If it's an eq unit, it may be flipped; nonunits should be ok.
   */
     
  ip1 = match_clauses(c, current->c);

  if (num_literals(current->c) == 1 && check_eq_lit(current->c->first_lit) &&
      ip1->i == -1) {
    ip1->i = 1;
    previous = current;
    current = connect_new_node(new_proof);
    current->parent1 = previous->id;
    current->position1 = get_ilist();
    current->position1->i = 1;
    current->rule = P_RULE_FLIP;
    current->c = cl_copy(previous->c);
    l2 = current->c->first_lit;
    t = l2->atom->farg->argval;
    l2->atom->farg->argval = l2->atom->farg->narg->argval;
    l2->atom->farg->narg->argval = t;
  }

  /* OK, finally we have the final clause corresponding to old clause c. */

  current->map = ip1;
  current->old_id = c->id;

  /* rc = insert_into_gen_tab(New_proof_tab, current->id, current); */
  rc = insert_into_gen_tab(New_proof_old_id_tab, c->id, current);

#if 1
  /* Sanity check: no other literals flipped, and each subsumes the other.
   */
  for (ip1 = current->map; ip1; ip1 = ip1->next)
    if (ip1->i < 0)
      abend("finish_translating, literal flipped");
  if (!subsume(c, current->c) || !subsume(current->c, c))
    abend("finish_translating, subsumption failure");
#endif

  return(1);
    
}  /* finish_translating */

/*************
 *
 *   translate_resolution()
 *
 *************/

static int translate_resolution(struct clause *c,
				struct proof_object *new_proof)
{
  struct ilist *ip, *ip_save;
  struct proof_object_node *par1_node, *par2_node;
  int old_index1, old_index2, new_index1, new_index2;
  struct literal *new_lit1, *new_lit2;
  int i, rc;
  struct context *s1, *s2;
  struct trail *tr;
  struct proof_object_node *par1_instance_node, *par2_instance_node;
  struct proof_object_node *resolvent_node;

  /* First get info from parent list of clause: */
  /* [binary, par1, list-1, lit1, par2, list-1, lit2, ...] */

  ip = c->parents->next;
  par1_node = retrieve_from_gen_tab(New_proof_old_id_tab, ip->i);
  ip = ip->next;
  if (ip->i != LIST_RULE-1)
    abend("translate_resolution, can't find first list");
  ip = ip->next;
  old_index1 = ip->i;

  ip = ip->next;
  par2_node = retrieve_from_gen_tab(New_proof_old_id_tab, ip->i);
  ip = ip->next;
  if (ip->i != LIST_RULE-1)
    abend("translate_resolution, can't find second list");
  ip = ip->next;
  old_index2 = ip->i;

  ip_save = ip->next;  /* save place for processing rest of parent list. */

  /* Now find the corresp. indexes in the new_parents. */

  new_index1 = new_literal_index(par1_node->map, old_index1);
  new_index2 = new_literal_index(par2_node->map, old_index2);

  /* Get the resolving literals from the new_parents. */

  new_lit1 = ith_literal(par1_node->c, new_index1);
  new_lit2 = ith_literal(par2_node->c, new_index2);

#ifdef DEBUG
  printf("\nBinary resolution: "); p_clause(c);
  printf("old_par1=%d, old_index1=%d, par1_node->c=%d, new_index1=%d\n",
	 par1_node->old_id, old_index1, par1_node->id, new_index1);
  printf("old_par2=%d, old_index2=%d, par2_node->c=%d, new_index2=%d\n",
	 par2_node->old_id, old_index2, par2_node->id, new_index2);
#endif

  /* If negative literal is par2, swap par1, par2. */

  if (!new_lit2->sign) {
    struct literal *temp_lit;
    struct proof_object_node *temp_node;
	
    temp_lit = new_lit1; new_lit1 = new_lit2; new_lit2 = temp_lit;
    temp_node = par1_node; par1_node = par2_node; par2_node = temp_node;
    i = new_index1; new_index1 = new_index2; new_index2 = i;

    /* We shouldn't refer to old again, so don't bother to swap them. */
  }

  if (new_lit1->sign || !new_lit2->sign)
    abend("translate_resolution: signs wrong.\n");

  /* Unify the atoms. */

  s1 = get_context(); s1->multiplier = 0;
  s2 = get_context(); s2->multiplier = 1;
  tr = NULL;

  rc = unify(new_lit1->atom, s1, new_lit2->atom, s2, &tr);
  if (!rc) {
    p_term(new_lit1->atom);
    p_term(new_lit2->atom);
    abend("translate_resolution: unify fails on the preceding.\n");
  }

  /* Buid the instance nodes. */

  if (trivial_subst(s1))
    par1_instance_node = par1_node;
  else {
    par1_instance_node = connect_new_node(new_proof);
    par1_instance_node->parent1 = par1_node->id;
    par1_instance_node->rule = P_RULE_INSTANTIATE;
    par1_instance_node->c = apply_clause(par1_node->c, s1);
    copy_subst_to_proof_object(s1, par1_instance_node);
  }

  if (ground_clause(par2_node->c))
    par2_instance_node = par2_node;
  else {
    par2_instance_node = connect_new_node(new_proof);
    par2_instance_node->parent1 = par2_node->id;
    par2_instance_node->rule = P_RULE_INSTANTIATE;
    par2_instance_node->c = apply_clause(par2_node->c, s2);
    copy_subst_to_proof_object(s2, par2_instance_node);
  }

  clear_subst_1(tr);  /* clears both substitution tables */
  free_context(s1);
  free_context(s2);


  /* Build the resolvent node. */

  resolvent_node = connect_new_node(new_proof);
  resolvent_node->rule = P_RULE_RESOLVE;

  resolvent_node->parent1 = par1_instance_node->id;
  resolvent_node->parent2 = par2_instance_node->id;
    
  ip = get_ilist(); ip->i = new_index1; resolvent_node->position1 = ip;
  ip = get_ilist(); ip->i = new_index2; resolvent_node->position2 = ip;

  resolvent_node->c = identity_resolve(par1_instance_node->c, new_index1,
				       par2_instance_node->c, new_index2);

#ifdef DEBUG
  p_proof_object_node(par1_instance_node);
  p_proof_object_node(par2_instance_node);
  p_proof_object_node(resolvent_node);
#endif

  rc = finish_translating(c, ip_save, resolvent_node, new_proof);

  return(1);
}  /* translate_resolution */

/*************
 *
 *   order_new_lits_for_hyper()
 *
 *   Return a permutation of 1--n, where n is the number of negative literals.
 *   The permutation gives the relative order of negative literals in the
 *   old clause to those in the new.  Example:
 *
 *   new:   -q2 -q1 -q3 p1 p2 p3 p4.
 *   map:     4   2   6  1  3  5  7
 *
 *   return:  2 1 3.
 *
 *************/

static struct ilist *order_new_lits_for_hyper(struct proof_object_node *pn)
{
  struct ilist *ip1, *ip2, *ip3, *ip4, *ip_min;
  struct literal *lit;
  int n, i;

  ip4 = NULL;
  for (lit = pn->c->first_lit, ip3 = pn->map, n = 0;
       lit && ip3;
       lit = lit->next_lit, ip3 = ip3->next) {
    if (!lit->sign) {
      n++;
      ip2 = get_ilist();
      ip2->i = -ip3->i;
      if (!ip4)
	ip1 = ip2;
      else
	ip4->next = ip2;
      ip4 = ip2;
    }
  }

  for (i = 0; i < n; i++) {
    ip_min = NULL;
    for (ip2 = ip1; ip2; ip2 = ip2->next)
      if (!ip_min || ip2->i < ip_min->i)
	ip_min = ip2;
    ip_min->i = n-i;
  }

#if 0
  printf("\nCheck_order: "); p_clause(pn->c);
  for (ip2 = ip1; ip2; ip2 = ip2->next)
    printf("%d ", ip2->i);
  printf("\n\n");
#endif
  return(ip1);
}  /* order_new_lits_for_hyper */

/*************
 *
 *   translate_hyper()
 *
 *************/

static int translate_hyper(struct clause *c,
			   struct proof_object *new_proof)
{
  struct ilist *ip, *ip1, *ip2, *ip_save;
  struct proof_object_node *nuc_node, *sat_node, *result_node;
  struct proof_object_node *nuc_i_node, *sat_i_node, *result_i_node; /* instances */
  int i, sat_index, nuc_index, rc;
  struct literal *nuc_lit, *sat_lit;
  struct context *s1, *s2;
  struct trail *tr;

  s1 = get_context(); s1->multiplier = 0;
  s2 = get_context(); s2->multiplier = 1;

  /* [hyper, nuc, sat1, -1001, sat1_index, sat2, -1001, sat2_index, ... */

  ip = c->parents->next;
  nuc_node = retrieve_from_gen_tab(New_proof_old_id_tab, ip->i);
  ip_save = ip->next;

  for (ip1 = order_new_lits_for_hyper(nuc_node); ip1; ip1 = ip1->next) {
    for (ip2 = ip_save, i = 1; i < ip1->i; ip2 = ip2->next->next->next, i++);

    sat_node = retrieve_from_gen_tab(New_proof_old_id_tab, ip2->i);
    sat_index = new_literal_index(sat_node->map, ip2->next->next->i);
    sat_lit = ith_literal(sat_node->c, sat_index);

    /* Get index of first negative literal in nuc clause. */

    for (nuc_lit = nuc_node->c->first_lit, nuc_index = 1;
	 nuc_lit->sign;
	 nuc_lit = nuc_lit->next_lit, nuc_index++);

    tr = NULL;
	
    rc = unify(nuc_lit->atom, s1, sat_lit->atom, s2, &tr);
    if (nuc_lit->sign == sat_lit->sign || !rc) {
      p_term(nuc_lit->atom);
      p_term(sat_lit->atom);
      abend("translate_hyper: unify fails on the preceding.\n");
    }
	
    if (trivial_subst(s1))
      nuc_i_node = nuc_node;
    else {
      nuc_i_node = connect_new_node(new_proof);
      nuc_i_node->parent1 = nuc_node->id;
      nuc_i_node->rule = P_RULE_INSTANTIATE;
      nuc_i_node->c = apply_clause(nuc_node->c, s1);
      copy_subst_to_proof_object(s1, nuc_i_node);
    }
    if (ground_clause(sat_node->c))
      sat_i_node = sat_node;
    else {
      sat_i_node = connect_new_node(new_proof);
      sat_i_node->parent1 = sat_node->id;
      sat_i_node->rule = P_RULE_INSTANTIATE;
      sat_i_node->c = apply_clause(sat_node->c, s2);
      copy_subst_to_proof_object(s2, sat_i_node);
    }
    clear_subst_1(tr);  /* clears both substitution tables */

    /* Build the resolvent node. */

    result_node = connect_new_node(new_proof);
    result_node->rule = P_RULE_RESOLVE;

    result_node->parent1 = nuc_i_node->id;
    result_node->parent2 = sat_i_node->id;
    
    ip = get_ilist(); ip->i = nuc_index; result_node->position1 = ip;
    ip = get_ilist(); ip->i = sat_index; result_node->position2 = ip;

    result_node->c = identity_resolve(nuc_i_node->c, nuc_index,
				      sat_i_node->c, sat_index);

    if (ground_clause(result_node->c))
      result_i_node = result_node;
    else {
      struct clause *d;
      d = cl_copy(result_node->c);
      rc = renumber_vars(d);
      if (clause_ident(result_node->c, d))
	result_i_node = result_node;
      else {
	result_i_node = connect_new_node(new_proof);
	result_i_node->rule = P_RULE_INSTANTIATE;
	result_i_node->parent1 = result_node->id;
	result_i_node->c = cl_copy(result_node->c);
	renumber_vars_subst(result_i_node->c, result_i_node->subst);
      }
      cl_del_non(d);
    }

    nuc_node = result_i_node;
  }

  free_context(s1);
  free_context(s2);

  while (ip_save && ip_save->i > 0)
    ip_save = ip_save->next->next->next;

  rc = finish_translating(c, ip_save, nuc_node, new_proof);

  return(1);
}  /* translate_hyper */

/*************
 *
 *   ipx()
 *
 *************/

int ipx(struct ilist *ip,
	int n)
{
  int i;
  for (i=1; ip && i < n; ip = ip->next, i++);
  if (ip)
    return(ip->i);
  else
    return(MAX_INT);
}  /* ipx */

/*************
 *
 *   translate_ur()
 *
 *************/

static int translate_ur(struct clause *c,
			struct proof_object *new_proof)
{
  struct ilist *ip, *ip1, *ip2, *ip3, *sat_ids, *sat_map;
  struct proof_object_node *nuc_node, *sat_node, *result_node;
  struct proof_object_node *nuc_i_node, *sat_i_node, *result_i_node; /* instances */
  int box_pos, rc, i, skip_first, nuc_index;
  struct literal *nuc_lit, *sat_lit;
  struct context *s1, *s2;
  struct trail *tr;

  s1 = get_context(); s1->multiplier = 0;
  s2 = get_context(); s2->multiplier = 1;

  /* [ur, nuc, -1001, box_pos, sat1, sat2, sat3, ... */

  ip = c->parents->next;
  nuc_node = retrieve_from_gen_tab(New_proof_old_id_tab, ip->i);
  ip = ip->next->next;
  box_pos = ip->i;

  /* Get satellite ids from history list. */

  ip2 = NULL; i = 0;
  for (ip = ip->next; ip && ip->i > 0; ip = ip->next) { /* for each sat. ID */
    i++;
    if (i == box_pos) {  /* insert empty slot */
      ip1 = get_ilist();
      ip1->i = 0;
      if (ip2)
	ip2->next = ip1;
      else
	sat_ids = ip1;
      ip2 = ip1;
    }
    ip1 = get_ilist();
    ip1->i = ip->i;
    if (ip2)
      ip2->next = ip1;
    else
      sat_ids = ip1;
    ip2 = ip1;
  }
    
  i++;
  if (i == box_pos) {  /* insert empty slot */
    ip1 = get_ilist();
    ip1->i = 0;
    if (ip2)
      ip2->next = ip1;
    else
      sat_ids = ip1;
    ip2 = ip1;
  }

  /* Use the map to permute sat_ids so that it matches the new nucleus. */

  ip2 = NULL;
  for (ip3 = nuc_node->map; ip3; ip3 = ip3->next) {
    ip1 = get_ilist();
    ip1->i = ipx(sat_ids, ip3->i);
    if (ip2)
      ip2->next = ip1;
    else
      sat_map = ip1;
    ip2 = ip1;
  }

#if 1
  printf("\ntranslate_ur: sat_map for box=%d, orig= ", box_pos);
  p_clause(c);
  for (ip1 = sat_map; ip1; ip1 = ip1->next) printf("%d ", ip1->i);
  p_clause(nuc_node->c);
#endif
    

  skip_first = 0;
  for (ip1 = sat_map; ip1; ip1 = ip1->next) {
    if (ip1->i == 0)
      skip_first = 1;
    else {

      sat_node = retrieve_from_gen_tab(New_proof_old_id_tab, ip1->i);
      sat_lit = ith_literal(sat_node->c, 1);

      /* Get index of clash literal in nuc clause. */
      nuc_index = (skip_first ? 2 : 1);

      nuc_lit = ith_literal(nuc_node->c, nuc_index);

      tr = NULL;
	
      rc = unify(nuc_lit->atom, s1, sat_lit->atom, s2, &tr);
      if (nuc_lit->sign == sat_lit->sign || !rc) {
	p_term(nuc_lit->atom);
	p_term(sat_lit->atom);
	abend("translate_ur: unify fails on the preceding.\n");
      }
	
      if (trivial_subst(s1))
	nuc_i_node = nuc_node;
      else {
	nuc_i_node = connect_new_node(new_proof);
	nuc_i_node->parent1 = nuc_node->id;
	nuc_i_node->rule = P_RULE_INSTANTIATE;
	nuc_i_node->c = apply_clause(nuc_node->c, s1);
	copy_subst_to_proof_object(s1, nuc_i_node);
      }
      if (ground_clause(sat_node->c))
	sat_i_node = sat_node;
      else {
	sat_i_node = connect_new_node(new_proof);
	sat_i_node->parent1 = sat_node->id;
	sat_i_node->rule = P_RULE_INSTANTIATE;
	sat_i_node->c = apply_clause(sat_node->c, s2);
	copy_subst_to_proof_object(s2, sat_i_node);
      }
      clear_subst_1(tr);  /* clears both substitution tables */
	    
      /* Build the resolvent node. */
	    
      result_node = connect_new_node(new_proof);
      result_node->rule = P_RULE_RESOLVE;
	    
      result_node->parent1 = nuc_i_node->id;
      result_node->parent2 = sat_i_node->id;
	    
      ip2 =get_ilist(); ip2->i =nuc_index; result_node->position1 =ip2;
      ip2 =get_ilist(); ip2->i =1;         result_node->position2 =ip2;
	    
      result_node->c = identity_resolve(nuc_i_node->c, nuc_index,
					sat_i_node->c, 1);
	    
      if (ground_clause(result_node->c))
	result_i_node = result_node;
      else {
	struct clause *d;
	d = cl_copy(result_node->c);
	rc = renumber_vars(d);
	if (clause_ident(result_node->c, d))
	  result_i_node = result_node;
	else {
	  result_i_node = connect_new_node(new_proof);
	  result_i_node->rule = P_RULE_INSTANTIATE;
	  result_i_node->parent1 = result_node->id;
	  result_i_node->c = cl_copy(result_node->c);
	  renumber_vars_subst(result_i_node->c, result_i_node->subst);
	}
	cl_del_non(d);
      }
	    
      nuc_node = result_i_node;
    }
  }

  free_context(s1);
  free_context(s2);

  rc = finish_translating(c, ip, nuc_node, new_proof);

  return(1);
}  /* translate_ur */

/*************
 *
 *   translate_factor()
 *
 *************/

static int translate_factor(struct clause *c,
			    struct proof_object *new_proof)
{
  int i1, i2, j, k1, k2;
  struct literal *lit1, *lit2;
  struct context *subst;
  struct trail *tr;
  struct proof_object_node *parent, *instance, *factor;

  /* Retrieve the proof object of the parent. */

  parent = retrieve_from_gen_tab(New_proof_old_id_tab, c->parents->next->i);

  /* Get the literal indexes in the old_parent. */

  i1 = c->parents->next->next->next->i;
  i2 = c->parents->next->next->next->next->i;

  /* Get corresponding indexes in new parent. */

  k1 = new_literal_index(parent->map, i1);
  k2 = new_literal_index(parent->map, i2);

  /* Get new literals. */

  lit1 = ith_literal(parent->c, k1);
  lit2 = ith_literal(parent->c, k2);

  /* Unify the literals. */

  subst = get_context(); subst->multiplier = 0;
  tr = NULL;
  j = unify(lit1->atom, subst, lit2->atom, subst, &tr);
  if (!j)
    abend("translate_factor, literals  don't unify");

  /* Build the instance node. */
    
  instance = connect_new_node(new_proof);
  instance->rule = P_RULE_INSTANTIATE;
  instance->parent1 = parent->id;
  instance->c = apply_clause(parent->c, subst);
  copy_subst_to_proof_object(subst, instance);

  clear_subst_1(tr);
  free_context(subst);

  /* Build the merge node. */

  factor = connect_new_node(new_proof);
  factor->rule = P_RULE_PROPOSITIONAL;
  factor->parent1 = instance->id;
  factor->position1 = get_ilist(); factor->position1->i = k2;
  factor->c = cl_copy_delete_literal(instance->c, k2);

  j = finish_translating(c, c->parents->next->next->next->next->next,
			 factor, new_proof);
  return(1);

}  /* translate_factor */

/*************
 *
 *   para_position()
 *
 *************/

static struct term *para_position(struct clause *c,
				  struct ilist *ip)
{
  struct literal *l;
  struct term *t;
  struct rel *r;
  int i;

  l = ith_literal(c, ip->i);
  t = l->atom;
  for (ip = ip->next; ip; ip = ip->next) {
    for (i = 1, r = t->farg; i < ip->i; i++, r = r->narg);
    t = r->argval;
  }
  return(t);
}  /* para_position */

/*************
 *
 *   translate_paramod()
 *
 *************/

static int translate_paramod(struct clause *c,
			     struct proof_object *new_proof)
{
  struct ilist *ip, *ip_save, *from_pos, *into_pos;
  struct proof_object_node *from_node, *into_node;
  int i, rc, n, para_from;
  struct context *s1, *s2;
  struct trail *tr;
  struct proof_object_node *from_instance_node, *into_instance_node;
  struct proof_object_node *para_node;
  struct term *from_term, *into_term;

  /* First get info from parent list of clause: */
  /* [rule, par1, list-1, pos1,..., par2, list-1, pos2,..., rest */

  para_from = (c->parents->i == PARA_FROM_RULE);

  ip = c->parents->next;
  if (para_from)
    from_node = retrieve_from_gen_tab(New_proof_old_id_tab, ip->i);
  else
    into_node = retrieve_from_gen_tab(New_proof_old_id_tab, ip->i);
  ip = ip->next;
  if (ip->i >= LIST_RULE)
    abend("translate_paramod, can't find first list");
  n = LIST_RULE - ip->i;  /* length of position vector */
  ip = ip->next;
  if (para_from)
    from_pos = copy_ilist_segment(ip, n);
  else
    into_pos = copy_ilist_segment(ip, n);
  for (i = 0; i < n; i++, ip = ip->next);

  if (para_from)
    into_node = retrieve_from_gen_tab(New_proof_old_id_tab, ip->i);
  else
    from_node = retrieve_from_gen_tab(New_proof_old_id_tab, ip->i);
  ip = ip->next;
  if (ip->i >= LIST_RULE)
    abend("translate_paramod, can't find second list");
  n = LIST_RULE - ip->i;  /* length of position vector */
  ip = ip->next;
  if (para_from)
    into_pos = copy_ilist_segment(ip, n);
  else
    from_pos = copy_ilist_segment(ip, n);
  for (i = 0; i < n; i++, ip = ip->next);

  ip_save = ip;  /* save place for processing rest of parent list. */

  /* Now modify the positions so that they corresp. to the new clauses.
   * Must translate literal index.  (Eq literals should be in same direction.)
   * Same for both from and into.
   */

  from_pos->i = new_literal_index(from_node->map, from_pos->i);

  into_pos->i = new_literal_index(into_node->map, into_pos->i);

  /* Get the unifying terms from the new_parents. */

  from_term = para_position(from_node->c, from_pos);
  into_term = para_position(into_node->c, into_pos);

#ifdef DEBUG
  printf("\nParamodulation: "); p_clause(c);
#endif

  /* Unify the terms. */

  s1 = get_context(); s1->multiplier = 0;
  s2 = get_context(); s2->multiplier = 1;
  tr = NULL;

  rc = unify(from_term, s1, into_term, s2, &tr);
  if (!rc) {
    p_term(from_term);
    p_term(into_term);
    abend("translate_paramod: unify fails on the preceding.\n");
  }

  /* Buid the instance nodes. */
    
  if (trivial_subst(s1))
    from_instance_node = from_node;
  else {
    from_instance_node = connect_new_node(new_proof);
    from_instance_node->parent1 = from_node->id;
    from_instance_node->rule = P_RULE_INSTANTIATE;
    from_instance_node->c = apply_clause(from_node->c, s1);
    copy_subst_to_proof_object(s1, from_instance_node);
  }

  if (ground_clause(into_node->c))
    into_instance_node = into_node;
  else {
    into_instance_node = connect_new_node(new_proof);
    into_instance_node->parent1 = into_node->id;
    into_instance_node->rule = P_RULE_INSTANTIATE;
    into_instance_node->c = apply_clause(into_node->c, s2);
    copy_subst_to_proof_object(s2, into_instance_node);
  }

  clear_subst_1(tr);  /* clears both substitution tables */
  free_context(s1);
  free_context(s2);

  /* Build the para node. */

  para_node = connect_new_node(new_proof);
  para_node->rule = P_RULE_PARAMOD;

  para_node->parent1 = from_instance_node->id;
  para_node->parent2 = into_instance_node->id;
    
  para_node->position1 = from_pos;
  para_node->position2 = into_pos;

  para_node->c = identity_paramod(from_instance_node->c, from_pos,
				  into_instance_node->c, into_pos);

  /* If into literal is negated, must add element to position.
   * Note that the position vector will no longer be valid for Otter terms.
   */

  if (ith_literal(into_node->c, into_pos->i)->sign == 0) {
    ip = get_ilist();
    ip->i = 1;
    ip->next = into_pos->next;
    into_pos->next = ip;
  }

#ifdef DEBUG
  p_proof_object_node(from_instance_node);
  p_proof_object_node(into_instance_node);
  p_proof_object_node(para_node);
#endif

  rc = finish_translating(c, ip_save, para_node, new_proof);

  return(1);
}  /* translate_paramod */

/************************************************************/

static void varmap(struct term *t,
		   struct term **vars)
{
  if (t->type == VARIABLE)
    vars[t->varnum] = t;
  else if (t->type == COMPLEX) {
    struct rel *r;
    for (r = t->farg; r; r = r->narg)
      varmap(r->argval, vars);
  }
}  /* varmap */

/************************************************************/

static BOOLEAN match2(struct clause *c1,
		      struct clause *c2,
		      struct term **vars)
{
  int ok;
  struct literal *l1, *l2;

  /* For variables, term_ident does not check sym_num field.
     So, we first check if the clauses are identical, then,
     get the "substitution".
  */

  for (l1 = c1->first_lit, l2 = c2->first_lit, ok = 1;
       l1 && l2 && ok;
       l1 = l1->next_lit, l2 = l2->next_lit) {
    if (l1->sign != l2->sign)
      ok = 0;
    else
      ok = term_ident(l1->atom, l2->atom);
  }
  if (!ok || l1 || l2)
    return 0;
  else {
    int i;
    for (i = 0; i < MAX_VARS; i++)
      vars[i] = NULL;
    for (l1 = c1->first_lit; l1; l1 = l1->next_lit)
      varmap(l1->atom, vars);
    return 1;
  }
}  /* match2 */

/************************************************************/

struct proof_object_node *find_match2(struct clause *c,
				      struct proof_object *obj,
				      struct term **vars)
{
  struct proof_object_node *pn = obj->first;
  while (pn && pn->rule == P_RULE_INPUT && !match2(pn->c, c, vars))
    pn = pn->next;
  return (pn && pn->rule == P_RULE_INPUT ? pn : NULL);
}  /* find_match2 */

/************************************************************/

/*************
 *
 *   translate_step()
 *
 *   Translate one step in an Otter proof into a sequene of detailed steps
 *   and add them to New_proof.
 *
 *   return (ok ? 1 : 0)
 *
 *************/

static int translate_step(struct clause *c,
			  struct proof_object *new_proof)
{
  int rc, rule, id;
  struct proof_object_node *pn;

#ifdef DEBUG
  printf("translating: "); p_clause(c);
#endif

  if (!c->parents) {
    pn = connect_new_node(new_proof);
    pn->c = cl_copy(c);
    pn->old_id = c->id;
    pn->map = match_clauses(c, pn->c);  /*  wasteful, but convenient */
    rc = insert_into_gen_tab(New_proof_old_id_tab, c->id, pn);

    if (!retrieve_initial_proof_object()) {
      pn->rule = P_RULE_INPUT;
      rc = 1;
    }
    else {
      struct term *vars[MAX_VARS];
      struct proof_object_node *initial_step;
      initial_step = find_match2(c, new_proof, vars);
      if (!initial_step)
	abend("translate_step, clauses don't match");
      else {
	int i;
	pn->rule = P_RULE_INSTANTIATE;
	pn->parent1 = initial_step->id;
	for (i = 0; i < MAX_VARS; i++)
	  pn->subst[i] = vars[i];
	pn->backward_subst = TRUE;
      }
    }
  }
  else if (c->parents->i == COPY_RULE &&
	   c->parents->next->next->i == PROPOSITIONAL_RULE) {
    /* special case */
    struct proof_object_node *prop_node;
    id = c->parents->next->i;
    pn = retrieve_from_gen_tab(New_proof_old_id_tab, id);
    prop_node = connect_new_node(new_proof);
    prop_node->parent1 = pn->id;
    prop_node->c = cl_copy(c);
    prop_node->rule = P_RULE_PROPOSITIONAL;
    prop_node->old_id = c->id;
    prop_node->map = match_clauses(c, prop_node->c);
    rc = insert_into_gen_tab(New_proof_old_id_tab,c->id,prop_node);
  }
  else {
    rule = c->parents->i;
    switch (rule) {
    case NEW_DEMOD_RULE   :
      /*
       * This is kludgey.  The proof object doesn't have separate
       * steps for NEW_DEMOD, so we just refer to the regular copy.
       * This means that the old_id field is not quite correct.
       * Recall that new_demod has old_id 1 greater than its parent.
       */
      id = c->parents->next->i;  /* ID of regular copy */
      pn = retrieve_from_gen_tab(New_proof_old_id_tab, id);
      rc = insert_into_gen_tab(New_proof_old_id_tab, c->id, pn);
      break;
    case COPY_RULE  :
      id = c->parents->next->i;
      pn = retrieve_from_gen_tab(New_proof_old_id_tab, id);
      rc = finish_translating(c, c->parents->next->next,
			      pn, new_proof);
      break;
    case BINARY_RES_RULE  :
    case BACK_UNIT_DEL_RULE  :
      rc = translate_resolution(c, new_proof);
      break;
    case HYPER_RES_RULE  :
      rc = translate_hyper(c, new_proof);
      break;
    case UR_RES_RULE  :
      rc = translate_ur(c, new_proof);
      break;
    case FACTOR_RULE      :
      rc = translate_factor(c, new_proof);
      break;
    case PARA_INTO_RULE   :
    case PARA_FROM_RULE   :
      rc = translate_paramod(c, new_proof);
      break;
    case BACK_DEMOD_RULE  :
      id = c->parents->next->i;
      pn = retrieve_from_gen_tab(New_proof_old_id_tab, id);
      rc = finish_translating(c, c->parents->next->next,
			      pn, new_proof);
      break;

    default               :
      fprintf(stderr, "translate_step: rule %d not handled.\n", rule);
      rc = 0;
      break;
    }
  }
  return(rc);
}  /* translate_step */

/*************
 *
 *   contains_answer_literal()
 *
 *************/

int contains_answer_literal(struct clause *c)
{
  struct literal *lit;
  for (lit = c->first_lit; lit; lit = lit->next_lit)
    if (answer_lit(lit))
      return(1);
  return(0);
}  /* contains_answer_literal */

/*************
 *
 *   contains_rule()
 *
 *************/

int contains_rule(struct clause *c,
		  int rule)
{
  struct ilist *ip;

  for (ip = c->parents; ip; ip = ip->next)
    if (ip->i == rule)
      return(1);
  return(0);
}  /* contains_rule */

/*************
 *
 *   trans_2_pos()
 *
 *************/

struct ilist *trans_2_pos(int id,
			    struct ilist *pos)
{
  int n, i;
  struct clause *c;
  struct proof_object_node *pn;
  struct ilist *new, *p1;

  if (pos == NULL)
    abend("trans_2_pos: NULL pos");
  pn = retrieve_from_gen_tab(New_proof_tab, id);
  if (pn == NULL)
    abend("trans_2_pos: proof node not found");
  c = pn->c;
  if (c == NULL)
    abend("trans_2_pos: clause not found");
  if (num_literals(c) == 0)
    abend("trans_2_pos: empty clause");

  n = num_literals(c);
  new = copy_ilist(pos->next);  /* copy all but first */
  if (pos->i != n) {
    p1 = get_ilist();
    p1->i = 1;
    p1->next = new;
    new = p1;
  }
  for (i = 2; i <= pos->i; i++) {
    p1 = get_ilist();
    p1->i = 2;
    p1->next = new;
    new = p1;
  }
  return new;
}  /* trans_2_pos */

/*************
 *
 *   type_2_trans()
 *
 *   Change the given proof object from type 1 to a type 2 by
 *   changing the position vectors in the justifications.
 *
 *************/

void type_2_trans(struct proof_object *po)
{
  struct proof_object_node *pn;
  struct ilist *ip;
  for (pn = po->first; pn; pn = pn->next) {
    if (pn->parent1 != 0 && pn->position1) {
      ip = trans_2_pos(pn->parent1, pn->position1);
      free_ilist_list(pn->position1);
      pn->position1 = ip;
    }
    if (pn->parent2 != 0 && pn->position2) {
      ip = trans_2_pos(pn->parent2, pn->position2);
      free_ilist_list(pn->position2);
      pn->position2 = ip;
    }
  }
}  /* type_2_trans */

/*************
 *
 *   glist_subsume(c, g)
 *
 *************/

int glist_subsume(struct clause *c, struct glist *g)
{
  if (g == NULL)
    return 0;
  else if (subsume(g->v, c))
    return 1;
  else
    return (glist_subsume(c, g->next));
}  /* glist_subsume */

/*************
 *
 *   p_proof_object_as_hints()
 *
 *************/

void p_proof_object_as_hints(struct proof_object *po)
{
  struct proof_object_node *pn;
  struct glist *h = NULL;
  struct glist *p;

  for (pn = po->first; pn; pn = pn->next) {
    struct clause *d = cl_copy(pn->c);

    if (Flags[ORDER_EQ].val) {
      if (Flags[LRPO].val)
	order_equalities_lrpo(d);
      else
	order_equalities(d);
    }
    renumber_vars(d);
    if (!glist_subsume(d, h)) {
      h = glist_tack_on(h, d);
    }
    else
      cl_del_non(d);
  }
  printf("list(%s).  %% Hints from process %d, %s",
	 Internal_flags[HINTS2_PRESENT] ? "hints2" : "hints",
	 my_process_id(), get_time());
  for (p = h; p != NULL; p = p->next) {
    print_clause_bare(stdout, p->v); printf(".\n");
    cl_del_non(p->v);
  }
  printf("end_of_list.\n");
  free_glist_list(h);
}  /* p_proof_object_as_hints */

/*************
 *
 *   remove_answer_literals()
 *
 *************/

struct literal *remove_answer_literals(struct literal *lit)
{
  if (lit == NULL)
    return NULL;
  else if (answer_lit(lit)) {
    struct literal *lit2 = lit->next_lit;
    lit->atom->occ.lit = NULL;
    zap_term(lit->atom);
    free_literal(lit);
    return remove_answer_literals(lit2);
  }
  else {
    lit->next_lit = remove_answer_literals(lit->next_lit);
    return lit;
  }
}  /* remove_answer_literals */

/*************
 *
 *   build_proof_object()
 *
 *   Given a clause (not necessarily empty), build and print the proof
 *   object corresponding to the clause.
 *
 *************/

void build_proof_object(struct clause *c)
{
  struct clause_ptr *cp1, *cp2;
  struct ilist *ip1;
  int level, i, rc;
  struct clause *d;
  struct proof_object *new_proof;

  if (!Flags[ORDER_HISTORY].val)
    abend("build_proof_object: flag order_history must be set");
  else if (!Flags[DETAILED_HISTORY].val)
    abend("build_proof_object: flag detailed_history must be set");

  cp1 = NULL;
  level = get_ancestors(c, &cp1, &ip1);

  for (cp2 = cp1; cp2; cp2 = cp2->next) {
    if (contains_answer_literal(cp2->c)) {
      /* Copy the clause and remove answer literals.
       * We won't worry about freeing this copy when we're done.
       * This small memory leak  shouldn't make much difference.
       */
      struct clause *c = cl_copy(cp2->c);
      c->parents = copy_ilist(cp2->c->parents);
      c->id = cp2->c->id;
      c->first_lit = remove_answer_literals(c->first_lit);
      cp2->c = c;
    }
  }

  for (cp2 = cp1; cp2; cp2 = cp2->next) {
    if (contains_answer_literal(cp2->c))
      abend("build_proof_object: proof objects cannot contain answer literals");
    else if (contains_rule(cp2->c, NEG_HYPER_RES_RULE))
      abend("build_proof_object: neg_hyper_res not allowed");
    else if (contains_rule(cp2->c, GEO_RULE))
      abend("build_proof_object: gL rule not allowed");
    else if (contains_rule(cp2->c, GEO_ID_RULE))
      abend("build_proof_object: gL-id not allowed");
    else if (contains_rule(cp2->c, LINKED_UR_RES_RULE))
      abend("build_proof_object: linked_ur_res not allowed");
    else if (contains_rule(cp2->c, EVAL_RULE))
      abend("build_proof_object: eval rule not allowed");
    else if (contains_rule(cp2->c, CLAUSIFY_RULE))
      abend("build_proof_object: clausify rule not allowed");
  }

  /* Old_proof_tab has the original clauses, indexed by their own IDs.
     New_proof_old_id_tab has proof nodes, indexed by original IDs.
     New_proof_tab has proof nodes, indexed by their own IDs.
  */

  new_proof = retrieve_initial_proof_object();

  /* If an intial proof object already exists, then New_proof_tab
     already exists also (it was built at the same time).  Unfortunately,
     we have a problem if there is an initial proof object and this
     is the second call to build_proof object:  New_proof_tab has
     been changed, so we cannot use it as we start building from
     the initial proof object.  Therefore, we allow at most one proof
     when we are using initial proof objects.  See abend below.
     (I think this can be fixed by keeping a copy of New_proof_tab
     when the initial proof object is created.
  */

  /* If we don't have an initial proof object, start a new one. */

  if (new_proof == NULL) {
    new_proof = get_proof_object();
    New_proof_tab = init_gen_tab();
  }
  else {
    if (Old_proof_tab != NULL)
      abend("build_proof_object, at most one proof object can be built when an initial proof object is given");
  }
  Old_proof_tab = init_gen_tab();
  New_proof_old_id_tab = init_gen_tab();

  for (cp2 = cp1; cp2; cp2 = cp2->next) {
    i = insert_into_gen_tab(Old_proof_tab, cp2->c->id, cp2->c);
  }

  for (cp2 = cp1, rc = 1; cp2 && rc; cp2 = cp2->next) {
    d = (struct clause *) retrieve_from_gen_tab(Old_proof_tab, cp2->c->id);
    rc = translate_step(cp2->c, new_proof);
  }

  if (Flags[PRINT_PROOF_AS_HINTS].val)
    p_proof_object_as_hints(new_proof);
  else {
    if (Flags[BUILD_PROOF_OBJECT_2].val)
      type_2_trans(new_proof);  /* translate to type 2 proof object */
    printf("\n;; BEGINNING OF PROOF OBJECT\n");
    p_proof_object(new_proof);
    printf(";; END OF PROOF OBJECT\n");
  }
}  /* build_proof_object */

/************************************************************/

void init_proof_object_environment(void)
{
  New_proof_tab = init_gen_tab();
}

