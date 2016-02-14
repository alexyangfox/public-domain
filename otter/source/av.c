/*
 *  av.c -- This file has routines for memory management.
 *
 */

#include "header.h"

/* Size of chunk allocated by malloc */

#define TP_ALLOC_SIZE 1000000
#define ALLOC_ARG_T unsigned

static char *Alloc_block;    /* location returned by most recent malloc */
static char *Alloc_pos;      /* current position in block */

/*  a list of available nodes for each type of structure */

static struct term *term_avail;
static struct rel *rel_avail;
static struct sym_ent *sym_ent_avail;
static struct term_ptr *term_ptr_avail;
static struct formula_ptr_2 *formula_ptr_2_avail;
static struct fpa_tree *fpa_tree_avail;
static struct fpa_head *fpa_head_avail;
static struct context *context_avail;
static struct trail *trail_avail;
static struct imd_tree *imd_tree_avail;
static struct imd_pos *imd_pos_avail;
static struct is_tree *is_tree_avail;
static struct is_pos *is_pos_avail;
static struct fsub_pos *fsub_pos_avail;
static struct literal *literal_avail;
static struct clause *clause_avail;
static struct list *list_avail;
static struct clash_nd *clash_nd_avail;
static struct clause_ptr *clause_ptr_avail;
static struct ci_ptr *ci_ptr_avail;
static struct ilist *ilist_avail;

static struct link_node *link_node_avail;
static struct ans_lit_node *ans_lit_node_avail;
static struct formula_box *formula_box_avail;
static struct formula *formula_avail;
static struct formula_ptr *formula_ptr_avail;
static struct cl_attribute *cl_attribute_avail;
static struct glist *glist_avail;
static struct g2list *g2list_avail;
static struct fnode *fnode_avail;

static int Malloc_calls;  /* number of calls to malloc */

/* # of gets, frees, and size of avail list for each type of structure */

static unsigned long term_gets, term_frees, term_avails;
static unsigned long rel_gets, rel_frees, rel_avails;
static unsigned long sym_ent_gets, sym_ent_frees, sym_ent_avails;
static unsigned long term_ptr_gets, term_ptr_frees, term_ptr_avails;
static unsigned long formula_ptr_2_gets, formula_ptr_2_frees, formula_ptr_2_avails;
static unsigned long fpa_tree_gets, fpa_tree_frees, fpa_tree_avails;
static unsigned long fpa_head_gets, fpa_head_frees, fpa_head_avails;
static unsigned long context_gets, context_frees, context_avails;
static unsigned long trail_gets, trail_frees, trail_avails;
static unsigned long imd_tree_gets, imd_tree_frees, imd_tree_avails;
static unsigned long imd_pos_gets, imd_pos_frees, imd_pos_avails;
static unsigned long is_tree_gets, is_tree_frees, is_tree_avails;
static unsigned long is_pos_gets, is_pos_frees, is_pos_avails;
static unsigned long fsub_pos_gets, fsub_pos_frees, fsub_pos_avails;
static unsigned long literal_gets, literal_frees, literal_avails;
static unsigned long clause_gets, clause_frees, clause_avails;
static unsigned long list_gets, list_frees, list_avails;
static unsigned long clash_nd_gets, clash_nd_frees, clash_nd_avails;
static unsigned long clause_ptr_gets, clause_ptr_frees, clause_ptr_avails;
static unsigned long ci_ptr_gets, ci_ptr_frees, ci_ptr_avails;
static unsigned long ilist_gets, ilist_frees, ilist_avails;

static unsigned long link_node_gets, link_node_frees, link_node_avails;
static unsigned long ans_lit_node_gets, ans_lit_node_frees, ans_lit_node_avails;
static unsigned long formula_box_gets, formula_box_frees, formula_box_avails;
static unsigned long formula_gets, formula_frees, formula_avails;
static unsigned long formula_ptr_gets, formula_ptr_frees, formula_ptr_avails;
static unsigned long cl_attribute_gets, cl_attribute_frees, cl_attribute_avails;
static unsigned long glist_gets, glist_frees, glist_avails;
static unsigned long g2list_gets, g2list_frees, g2list_avails;
static unsigned long fnode_gets, fnode_frees, fnode_avails;

/*************
 *
 *    int **tp_alloc(n)
 *
 *    Allocate n contiguous bytes, aligned on pointer boundry.
 *
 *************/

int **tp_alloc(int n)
{
  char *return_block;
  int scale;

  /* if n is not a multiple of sizeof(int *), then round up so that it is */

  scale = sizeof(int *);
  if (n % scale != 0)
    n = n + (scale - (n % scale));

  if (Alloc_block == NULL || Alloc_block + TP_ALLOC_SIZE - Alloc_pos < n) {
    /* try to malloc a new block */
    if (n > TP_ALLOC_SIZE) {
      char s[100];
      sprintf(s, "tp_alloc, request too big: %d", n);
      abend(s);
    }
    else if (Parms[MAX_MEM].val != -1 &&
	     ((Malloc_calls+1)*TP_ALLOC_SIZE)/1024 > Parms[MAX_MEM].val) {
      fprintf(stdout, "\nSearch stopped in tp_alloc by max_mem option.\n");
      fprintf(stderr, "\n%cSearch stopped in tp_alloc by max_mem option.\n", Bell);
      if (Flags[FREE_ALL_MEM].val) {
	/* freeing memory can require additional memory */
	fprintf(stdout, "    (free_all_mem cleared).\n");
	Flags[FREE_ALL_MEM].val = 0;
      }
      if (Flags[PRINT_LISTS_AT_END].val) {  /* 2/5/92 WWM */
	/* printing can require additional memory */
	fprintf(stdout, "    (print_lists_at_end cleared).\n");
	Flags[PRINT_LISTS_AT_END].val = 0;
      }
      cleanup();
      exit(MAX_MEM_EXIT);
    }
    else {

      Alloc_pos = Alloc_block = (char *) malloc((ALLOC_ARG_T) TP_ALLOC_SIZE);

      Malloc_calls++;
      Stats[K_MALLOCED] = (Malloc_calls * (TP_ALLOC_SIZE / 1024.));
      if (Alloc_pos == NULL) {
	/* Don't call abend() so that we can exit with a value. */
	output_stats(stdout, 3);
	fprintf(stdout, "\nABEND, malloc returns NULL (out of memory).\n");
	fprintf(stderr, "%cABEND, malloc returns NULL (out of memory).\n", Bell);
	exit(MALLOC_NULL_EXIT);
      }
    }
  }
  return_block = Alloc_pos;
  Alloc_pos += n;
  return((int **) return_block);
}  /* tp_alloc */

/*************
 *
 *   struct term *get_term()
 *
 *************/

struct term *get_term(void)
{
  struct term *p;

  term_gets++;
  if (term_avail == NULL)
    p = (struct term *) tp_alloc((int) sizeof(struct term));
  else {
    term_avails--;
    p = term_avail;
    term_avail = (struct term *) term_avail->farg;
  }
  p->sym_num = 0;
  p->farg = NULL;
  p->occ.rel = NULL;
  p->varnum = 0;
  p->bits = 0;
  p->fpa_id = 0;
  return(p);
}  /* get_term */

/*************
 *
 *    free_term()
 *
 *************/

void free_term(struct term *p)
{
  term_frees++;
  term_avails++;
  p->farg = (struct rel *) term_avail;
  term_avail = p;
}  /* free_term */

/*************
 *
 *    struct rel *get_rel()
 *
 *************/

struct rel *get_rel(void)
{
  struct rel *p;

  rel_gets++;
  if (rel_avail == NULL)
    p = (struct rel *) tp_alloc((int) sizeof(struct rel));
  else {
    rel_avails--;
    p = rel_avail;
    rel_avail = rel_avail->narg;
  }
  p->argval = NULL;
  p->argof = NULL;
  p->narg = NULL;
  p->nocc = NULL;
  p->path = 0;
  p->clashable = 0;
  return(p);
}  /* get_rel */

/*************
 *
 *    free_rel()
 *
 *************/

void free_rel(struct rel *p)
{
  rel_frees++;
  rel_avails++;
  p->narg = rel_avail;
  rel_avail = p;
}  /* free_rel */

/*************
 *
 *    struct sym_ent *get_sym_ent()
 *
 *************/

struct sym_ent *get_sym_ent(void)
{
  struct sym_ent *p;

  sym_ent_gets++;
  if (sym_ent_avail == NULL)
    p = (struct sym_ent *) tp_alloc((int) sizeof(struct sym_ent));
  else {
    sym_ent_avails--;
    p = sym_ent_avail;
    sym_ent_avail = sym_ent_avail->next;
  }
  p->eval_code = 0;
  p->lex_val = MAX_INT;
  p->skolem = 0;
  p->special_unary = 0;
  p->lex_rpo_status = LRPO_LR_STATUS;
  p->special_op = 0;
  p->op_type = 0;
  p->op_prec = 0;

  p->next = NULL;
  return(p);
}  /* get_sym_ent */

/*************
 *
 *    free_sym_ent()
 *
 *************/

void free_sym_ent(struct sym_ent *p)
{
  sym_ent_frees++;
  sym_ent_avails++;
  p->next = sym_ent_avail;
  sym_ent_avail = p;
}  /* free_sym_ent */

/*************
 *
 *    struct term_ptr *get_term_ptr()
 *
 *************/

struct term_ptr *get_term_ptr(void)
{
  struct term_ptr *p;

  term_ptr_gets++;
  if (term_ptr_avail == NULL)
    p = (struct term_ptr *) tp_alloc((int) sizeof(struct term_ptr));
  else {
    term_ptr_avails--;
    p = term_ptr_avail;
    term_ptr_avail = term_ptr_avail->next;
  }
  p->term = NULL;
  p->next = NULL;
  return(p);
}  /* get_term_ptr */

/*************
 *
 *    free_term_ptr()
 *
 *************/

void free_term_ptr(struct term_ptr *p)
{
  term_ptr_frees++;
  term_ptr_avails++;
  p->next = term_ptr_avail;
  term_ptr_avail = p;
}  /* free_term_ptr */

/*************
 *
 *    struct formula_ptr_2 *get_formula_ptr_2()
 *
 *************/

struct formula_ptr_2 *get_formula_ptr_2(void)
{
  struct formula_ptr_2 *p;

  formula_ptr_2_gets++;
  if (formula_ptr_2_avail == NULL)
    p = (struct formula_ptr_2 *) tp_alloc((int) sizeof(struct formula_ptr_2));
  else {
    formula_ptr_2_avails--;
    p = formula_ptr_2_avail;
    formula_ptr_2_avail = formula_ptr_2_avail->next;
  }
  p->f = NULL;
  p->next = NULL;
  p->prev = NULL;
  p->left = NULL;
  p->right = NULL;
  p->up = NULL;
  p->down = NULL;
  return(p);
}  /* get_formula_ptr_2 */

/*************
 *
 *    free_formula_ptr_2()
 *
 *************/

void free_formula_ptr_2(struct formula_ptr_2 *p)
{
  formula_ptr_2_frees++;
  formula_ptr_2_avails++;
  p->next = formula_ptr_2_avail;
  formula_ptr_2_avail = p;
}  /* free_formula_ptr_2 */

/*************
 *
 *    struct fpa_tree *get_fpa_tree()
 *
 *************/

struct fpa_tree *get_fpa_tree(void)
{
  struct fpa_tree *p;

  fpa_tree_gets++;
  if (fpa_tree_avail == NULL)
    p = (struct fpa_tree *) tp_alloc((int) sizeof(struct fpa_tree));
  else {
    fpa_tree_avails--;
    p = fpa_tree_avail;
    fpa_tree_avail = fpa_tree_avail->left;
  }
  p->position = (struct fposition) {NULL, 0};
  p->left = NULL;
  p->right = NULL;
  p->left_term = NULL;
  p->right_term = NULL;
  p->path = NULL;
  return(p);
}  /* get_fpa_tree */

/*************
 *
 *    free_fpa_tree()
 *
 *************/

void free_fpa_tree(struct fpa_tree *p)
{
  fpa_tree_frees++;
  fpa_tree_avails++;
  p->left = fpa_tree_avail;
  fpa_tree_avail = p;
}  /* free_fpa_tree */

/*************
 *
 *    struct fpa_head *get_fpa_head()
 *
 *************/

struct fpa_head *get_fpa_head(void)
{
  struct fpa_head *p;

  fpa_head_gets++;
  if (fpa_head_avail == NULL)
    p = (struct fpa_head *) tp_alloc((int) sizeof(struct fpa_head));
  else {
    fpa_head_avails--;
    p = fpa_head_avail;
    fpa_head_avail = fpa_head_avail->next;
  }
  p->terms = NULL;
  p->next = NULL;
  p->path = NULL;
  return(p);
}  /* get_fpa_head */

/*************
 *
 *    free_fpa_head()
 *
 *************/

void free_fpa_head(struct fpa_head *p)
{
  fpa_head_frees++;
  fpa_head_avails++;
  p->next = fpa_head_avail;
  fpa_head_avail = p;
}  /* free_head */

/*************
 *
 *    struct context *get_context()
 *
 *************/

struct context *get_context(void)
{
  struct context *p;
  int i;
  static int count=0;

  context_gets++;
  if (context_avail == NULL) {
    p = (struct context *) tp_alloc((int) sizeof(struct context));
    for (i=0; i<MAX_VARS; i++) {
      p->terms[i] = NULL;
      p->status[i] = 0;
    }
    p->built_in_multiplier = count++;  /* never change */
  }
  else {
    context_avails--;
    p = context_avail;
    context_avail = context_avail->contexts[0];
  }
  p->multiplier = -1;
  return(p);
}  /* get_context */

/*************
 *
 *    free_context()
 *
 *************/

void free_context(struct context *p)
{

#if 1
  int i;
  for (i=0; i<MAX_VARS; i++) {
    if (p->terms[i] != NULL) {
      printf("ERROR, context %x, var %d not null.\n", (unsigned) p->contexts[i], i);
      print_term_nl(stdout, p->terms[i]);
      p->terms[i] = NULL;
    }
  }
#endif
  context_frees++;
  context_avails++;
  p->contexts[0] = context_avail;
  context_avail = p;
}  /* free_context */

/*************
 *
 *    struct trail *get_trail()
 *
 *************/

struct trail *get_trail(void)
{
  struct trail *p;

  trail_gets++;
  if (trail_avail == NULL)
    p = (struct trail *) tp_alloc((int) sizeof(struct trail));
  else {
    trail_avails--;
    p = trail_avail;
    trail_avail = trail_avail->next;
  }
  p->next = NULL;
  return(p);
}  /* get_trail */

/*************
 *
 *    free_trail()
 *
 *************/

void free_trail(struct trail *p)
{
  trail_frees++;
  trail_avails++;
  p->next = trail_avail;
  trail_avail = p;
}  /* free_trail */

/*************
 *
 *    struct imd_tree *get_imd_tree()
 *
 *************/

struct imd_tree *get_imd_tree(void)
{
  struct imd_tree *p;

  imd_tree_gets++;
  if (imd_tree_avail == NULL)
    p = (struct imd_tree *) tp_alloc((int) sizeof(struct imd_tree));
  else {
    imd_tree_avails--;
    p = imd_tree_avail;
    imd_tree_avail = imd_tree_avail->next;
  }
  p->next = NULL;
  p->kids = NULL;
  p->type = 0;
  p->lab = 0;
  p->atoms = NULL;
  return(p);
}  /* get_imd_tree */

/*************
 *
 *    free_imd_tree()
 *
 *************/

void free_imd_tree(struct imd_tree *p)
{
  imd_tree_frees++;
  imd_tree_avails++;
  p->next = imd_tree_avail;
  imd_tree_avail = p;
}  /* free_imd_tree */

/*************
 *
 *    struct imd_pos *get_imd_pos()
 *
 *************/

struct imd_pos *get_imd_pos(void)
{
  struct imd_pos *p;

  imd_pos_gets++;
  if (imd_pos_avail == NULL)
    p = (struct imd_pos *) tp_alloc((int) sizeof(struct imd_pos));
  else {
    imd_pos_avails--;
    p = imd_pos_avail;
    imd_pos_avail = imd_pos_avail->next;
  }
  p->next = NULL;
  return(p);
}  /* get_imd_pos */

/*************
 *
 *    free_imd_pos()
 *
 *************/

void free_imd_pos(struct imd_pos *p)
{
  imd_pos_frees++;
  imd_pos_avails++;
  p->next = imd_pos_avail;
  imd_pos_avail = p;
}  /* free_imd_pos */

/*************
 *
 *    struct is_tree *get_is_tree()
 *
 *************/

struct is_tree *get_is_tree(void)
{
  struct is_tree *p;

  is_tree_gets++;
  if (is_tree_avail == NULL)
    p = (struct is_tree *) tp_alloc((int) sizeof(struct is_tree));
  else {
    is_tree_avails--;
    p = is_tree_avail;
    is_tree_avail = is_tree_avail->next;
  }
  p->next = NULL;
  p->type = 0;
  p->lab = 0;
  p->u.kids = NULL;
  return(p);
}  /* get_is_tree */

/*************
 *
 *    free_is_tree()
 *
 *************/

void free_is_tree(struct is_tree *p)
{
  is_tree_frees++;
  is_tree_avails++;
  p->next = is_tree_avail;
  is_tree_avail = p;
}  /* free_is_tree */

/*************
 *
 *    struct is_pos *get_is_pos()
 *
 *************/

struct is_pos *get_is_pos(void)
{
  struct is_pos *p;

  is_pos_gets++;
  if (is_pos_avail == NULL)
    p = (struct is_pos *) tp_alloc((int) sizeof(struct is_pos));
  else {
    is_pos_avails--;
    p = is_pos_avail;
    is_pos_avail = is_pos_avail->next;
  }
  p->next = NULL;
  return(p);
}  /* get_is_pos */

/*************
 *
 *    free_is_pos()
 *
 *************/

void free_is_pos(struct is_pos *p)
{
  is_pos_frees++;
  is_pos_avails++;
  p->next = is_pos_avail;
  is_pos_avail = p;
}  /* free_is_pos */

/*************
 *
 *    struct fsub_pos *get_fsub_pos()
 *
 *************/

struct fsub_pos *get_fsub_pos(void)
{
  struct fsub_pos *p;

  fsub_pos_gets++;
  if (fsub_pos_avail == NULL)
    p = (struct fsub_pos *) tp_alloc((int) sizeof(struct fsub_pos));
  else {
    fsub_pos_avails--;
    p = fsub_pos_avail;
    fsub_pos_avail = (struct fsub_pos *) fsub_pos_avail->terms;
  }
  return(p);
}  /* get_fsub_pos */

/*************
 *
 *    free_fsub_pos()
 *
 *************/

void free_fsub_pos(struct fsub_pos *p)
{
  fsub_pos_frees++;
  fsub_pos_avails++;
  p->terms = (struct term_ptr *) fsub_pos_avail;
  fsub_pos_avail = p;
}  /* free_fsub_pos */

/*************
 *
 *    struct literal *get_literal()
 *
 *************/

struct literal *get_literal(void)
{
  struct literal *p;

  literal_gets++;
  if (literal_avail == NULL)
    p = (struct literal *) tp_alloc((int) sizeof(struct literal));
  else {
    literal_avails--;
    p = literal_avail;
    literal_avail = literal_avail->next_lit;
  }
  p->container = NULL;
  p->next_lit = NULL;
  p->sign = 0;
  p->atom = NULL;
  return(p);
}  /* get_literal */

/*************
 *
 *    free_literal()
 *
 *************/

void free_literal(struct literal *p)
{
  literal_frees++;
  literal_avails++;
  p->next_lit = literal_avail;
  literal_avail = p;
}  /* free_literal */

/*************
 *
 *    struct clause *get_clause()
 *
 *************/

struct clause *get_clause(void)
{
  struct clause *p;

  clause_gets++;
  if (clause_avail == NULL)
    p = (struct clause *) tp_alloc((int) sizeof(struct clause));
  else {
    clause_avails--;
    p = clause_avail;
    clause_avail = clause_avail->next_cl;
  }
  p->id = 0;
  p->parents = NULL;
  p->multi_parents = NULL;
  p->container = NULL;
  p->next_cl = NULL;
  p->prev_cl = NULL;
  p->first_lit = NULL;
  p->pick_weight = 0;
  p->type = NOT_SPECIFIED;
  p->bits = 0;
  p->heat_level = 0;
  p->attributes = NULL;

  return(p);
}  /* get_clause */

/*************
 *
 *    free_clause()
 *
 *************/

void free_clause(struct clause *p)
{
  clause_frees++;
  clause_avails++;
  p->next_cl = clause_avail;
  clause_avail = p;
}  /* free_clause */

/*************
 *
 *    struct list *get_list()
 *
 *************/

struct list *get_list(void)
{
  struct list *p;

  list_gets++;
  if (list_avail == NULL)
    p = (struct list *) tp_alloc((int) sizeof(struct list));
  else {
    list_avails--;
    p = list_avail;
    list_avail = (struct list *) list_avail->first_cl;
  }
  p->first_cl = NULL;
  p->last_cl = NULL;
  p->name[0] = '\0';
  return(p);
}  /* get_list */

/*************
 *
 *    free_list()
 *
 *************/

void free_list(struct list *p)
{
  list_frees++;
  list_avails++;
  p->first_cl = (struct clause *) list_avail;
  list_avail = p;
}  /* free_list */

/*************
 *
 *    struct clash_nd *get_clash_nd()
 *
 *************/

struct clash_nd *get_clash_nd(void)
{
  struct clash_nd *p;

  clash_nd_gets++;
  if (clash_nd_avail == NULL)
    p = (struct clash_nd *) tp_alloc((int) sizeof(struct clash_nd));
  else {
    clash_nd_avails--;
    p = clash_nd_avail;
    clash_nd_avail = clash_nd_avail->next;
  }
  p->next = NULL;
  p->prev = NULL;
  p->evaluable = 0;
  return(p);
}  /* get_clash_nd */

/*************
 *
 *    free_clash_nd()
 *
 *************/

void free_clash_nd(struct clash_nd *p)
{
  clash_nd_frees++;
  clash_nd_avails++;
  p->next = clash_nd_avail;
  clash_nd_avail = p;
}  /* free_clash_nd */

/*************
 *
 *    struct clause_ptr *get_clause_ptr()
 *
 *************/

struct clause_ptr *get_clause_ptr(void)
{
  struct clause_ptr *p;

  clause_ptr_gets++;
  if (clause_ptr_avail == NULL)
    p = (struct clause_ptr *) tp_alloc((int) sizeof(struct clause_ptr));
  else {
    clause_ptr_avails--;
    p = clause_ptr_avail;
    clause_ptr_avail = clause_ptr_avail->next;
  }
  p->next = NULL;
  p->c = NULL;
  return(p);
}  /* get_clause_ptr */

/*************
 *
 *    free_clause_ptr()
 *
 *************/

void free_clause_ptr(struct clause_ptr *p)
{
  clause_ptr_frees++;
  clause_ptr_avails++;
  p->next = clause_ptr_avail;
  clause_ptr_avail = p;
}  /* free_clause_ptr */

/*************
 *
 *    struct ci_ptr *get_ci_ptr()
 *
 *************/

struct ci_ptr *get_ci_ptr(void)
{
  struct ci_ptr *p;

  ci_ptr_gets++;
  if (ci_ptr_avail == NULL)
    p = (struct ci_ptr *) tp_alloc((int) sizeof(struct ci_ptr));
  else {
    ci_ptr_avails--;
    p = ci_ptr_avail;
    ci_ptr_avail = ci_ptr_avail->next;
  }
  p->next = NULL;
  p->c = NULL;
  p->v = NULL;
  return(p);
}  /* get_ci_ptr */

/*************
 *
 *    free_ci_ptr()
 *
 *************/

void free_ci_ptr(struct ci_ptr *p)
{
  ci_ptr_frees++;
  ci_ptr_avails++;
  p->next = ci_ptr_avail;
  ci_ptr_avail = p;
}  /* free_ci_ptr */

/*************
 *
 *    struct ilist *get_ilist()
 *
 *************/

struct ilist *get_ilist(void)
{
  struct ilist *p;

  ilist_gets++;
  if (ilist_avail == NULL)
    p = (struct ilist *) tp_alloc((int) sizeof(struct ilist));
  else {
    ilist_avails--;
    p = ilist_avail;
    ilist_avail = ilist_avail->next;
  }
  p->next = NULL;
  p->i = 0;
  return(p);
}  /* get_ilist */

/*************
 *
 *    free_ilist()
 *
 *************/

void free_ilist(struct ilist *p)
{
  ilist_frees++;
  ilist_avails++;
  p->next = ilist_avail;
  ilist_avail = p;
}  /* free_ilist */

/*************
 *
 *    struct ans_lit_node *get_ans_lit_node()
 *
 *************/

struct ans_lit_node *get_ans_lit_node(void)
{
  struct ans_lit_node *p;

  ans_lit_node_gets++;
  if (ans_lit_node_avail == NULL)
    p = (struct ans_lit_node *) tp_alloc((int) sizeof(struct ans_lit_node));
  else {
    ans_lit_node_avails--;
    p = ans_lit_node_avail;
    ans_lit_node_avail = ans_lit_node_avail->next;
  }

  p->next = NULL;
  p->parent = NULL;
  p->lit = NULL;

  return(p);
}  /* get_ans_lit_node */

/*************
 *
 *    void free_ans_lit_node()
 *
 *************/

void free_ans_lit_node(struct ans_lit_node *p)
{
  ans_lit_node_frees++;
  ans_lit_node_avails++;
  p->next = ans_lit_node_avail;
  ans_lit_node_avail = p;
}  /* free_ans_lit_node */

/*************
 *
 *    struct formula_box *get_formula_box()
 *
 *************/

struct formula_box *get_formula_box(void)
{
  struct formula_box *p;

  formula_box_gets++;
  if (formula_box_avail == NULL)
    p = (struct formula_box *) tp_alloc((int) sizeof(struct formula_box));
  else {
    formula_box_avails--;
    p = formula_box_avail;
    formula_box_avail = formula_box_avail->next;
  }

  p->first_child = p->next = p->parent = NULL;
  p->f = NULL;
  p->str[0] = '\0';
  p->type = p->subtype = p->length = p->height = p->x_off = p->y_off = 0;
  p->abs_x_loc = p->abs_y_loc = 0;

  return(p);
}  /* get_formula_box */

/*************
 *
 *    void free_formula_box()
 *
 *************/

void free_formula_box(struct formula_box *p)
{
  formula_box_frees++;
  formula_box_avails++;
  p->next = formula_box_avail;
  formula_box_avail = p;
}  /* free_formula_box */

/*************
 *
 *    struct formula *get_formula()
 *
 *************/

struct formula *get_formula(void)
{
  struct formula *p;

  formula_gets++;
  if (formula_avail == NULL)
    p = (struct formula *) tp_alloc((int) sizeof(struct formula));
  else {
    formula_avails--;
    p = formula_avail;
    formula_avail = formula_avail->next;
  }

  p->type = 0;
  p->quant_type = 0;
  p->parent = p->first_child = p->next = NULL;
  p->t = NULL;
  return(p);
}  /* get_formula */

/*************
 *
 *    void free_formula()
 *
 *************/

void free_formula(struct formula *p)
{
  formula_frees++;
  formula_avails++;
  p->next = formula_avail;
  formula_avail = p;
}  /* free_formula */

/*************
 *
 *    struct formula_ptr *get_formula_ptr()
 *
 *************/

struct formula_ptr *get_formula_ptr(void)
{
  struct formula_ptr *p;

  formula_ptr_gets++;
  if (formula_ptr_avail == NULL)
    p = (struct formula_ptr *) tp_alloc((int) sizeof(struct formula_ptr));
  else {
    formula_ptr_avails--;
    p = formula_ptr_avail;
    formula_ptr_avail = formula_ptr_avail->next;
  }

  p->f = NULL;
  p->next = NULL;
  return(p);
}  /* get_formula_ptr */

/*************
 *
 *    void free_formula_ptr()
 *
 *************/

void free_formula_ptr(struct formula_ptr *p)
{
  formula_ptr_frees++;
  formula_ptr_avails++;
  p->next = formula_ptr_avail;
  formula_ptr_avail = p;
}  /* free_formula_ptr */

/*************
 *
 *    struct cl_attribute *get_cl_attribute()
 *
 *************/

struct cl_attribute *get_cl_attribute(void)
{
  struct cl_attribute *p;

  cl_attribute_gets++;
  if (cl_attribute_avail == NULL)
    p = (struct cl_attribute *) tp_alloc((int) sizeof(struct cl_attribute));
  else {
    cl_attribute_avails--;
    p = cl_attribute_avail;
    cl_attribute_avail = cl_attribute_avail->next;
  }

  p->name = -1;
  p->next = NULL;
  return(p);
}  /* get_cl_attribute */

/*************
 *
 *    void free_cl_attribute()
 *
 *************/

void free_cl_attribute(struct cl_attribute *p)
{
  cl_attribute_frees++;
  cl_attribute_avails++;
  p->next = cl_attribute_avail;
  cl_attribute_avail = p;
}  /* free_cl_attribute */

/*************
 *
 *    struct link_node *get_link_node()
 *
 *************/

struct link_node *get_link_node(void)
{
  struct link_node *p;

  link_node_gets++;
  if (link_node_avail == NULL)
    p = (struct link_node *) tp_alloc((int) sizeof(struct link_node));
  else {
    link_node_avails--;
    p = link_node_avail;
    link_node_avail = link_node_avail->next_sibling;
  }

  p->parent = NULL;
  p->first_child = NULL;
  p->child_first_ans = NULL;
  p->child_last_ans = NULL;
  p->next_sibling = NULL;
  p->prev_sibling = NULL;
  p->first = TRUE;
  p->unit_deleted = FALSE;  /* Initially literal has not been unit deleted */
  p->goal = NULL;
  p->goal_to_resolve = NULL;
  p->current_clause = NULL;
  p->subst = NULL;
  p->unif_position = NULL;
  p->tr = NULL;
  p->near_poss_nuc = UNDEFINED;
  p->farthest_sat = 0;
  p->target_dist = 0;
  p->back_up = UNDEFINED;

  return(p);
}  /* get_link_node */

/*************
 *
 *    void free_link_node()
 *
 *************/

void free_link_node(struct link_node *p)
{
  link_node_frees++;
  link_node_avails++;
  p->next_sibling = link_node_avail;
  link_node_avail = p;
}  /* free_link_node */

/*************
 *
 *    free_imd_pos_list(imd_pos) -- free a list of imd_pos nodes.
 *
 *************/

void free_imd_pos_list(struct imd_pos *p)
{
  struct imd_pos *q;

  if (p != NULL) {
    q = p;
    imd_pos_frees++;
    imd_pos_avails++;
    while (q->next != NULL) {
      imd_pos_frees++;
      imd_pos_avails++;
      q = q->next;
    }
    q->next = imd_pos_avail;
    imd_pos_avail = p;
  }
}  /* free_imd_pos_list */

/*************
 *
 *    free_is_pos_list(is_pos) -- free a list of is_pos nodes.
 *
 *************/

void free_is_pos_list(struct is_pos *p)
{
  struct is_pos *q;

  if (p != NULL) {
    q = p;
    is_pos_frees++;
    is_pos_avails++;
    while (q->next != NULL) {
      is_pos_frees++;
      is_pos_avails++;
      q = q->next;
    }
    q->next = is_pos_avail;
    is_pos_avail = p;
  }
}  /* free_is_pos_list */

/*************
 *
 *    struct glist *get_glist()
 *
 *************/

struct glist *get_glist(void)
{
  struct glist *p;

  glist_gets++;
  if (glist_avail == NULL)
    p = (struct glist *) tp_alloc((int) sizeof(struct glist));
  else {
    glist_avails--;
    p = glist_avail;
    glist_avail = glist_avail->next;
  }
  p->v = NULL;
  p->next = NULL;
  return(p);
}  /* get_glist */

/*************
 *
 *    free_glist()
 *
 *************/

void free_glist(struct glist *p)
{
  glist_frees++;
  glist_avails++;
  p->next = glist_avail;
  glist_avail = p;
}  /* free_glist */

/*************
 *
 *    struct g2list *get_g2list()
 *
 *************/

struct g2list *get_g2list(void)
{
  struct g2list *p;

  g2list_gets++;
  if (g2list_avail == NULL)
    p = (struct g2list *) tp_alloc((int) sizeof(struct g2list));
  else {
    g2list_avails--;
    p = g2list_avail;
    g2list_avail = g2list_avail->next;
  }
  p->v1 = NULL;
  p->v2 = NULL;
  p->next = NULL;
  return(p);
}  /* get_g2list */

/*************
 *
 *    free_g2list()
 *
 *************/

void free_g2list(struct g2list *p)
{
  g2list_frees++;
  g2list_avails++;
  p->next = g2list_avail;
  g2list_avail = p;
}  /* free_g2list */

/*************
 *
 *    Fnode get_fnode()
 *
 *************/

struct fnode *get_fnode(void)
{
  struct fnode *p;

  fnode_gets++;
  if (fnode_avail == NULL)
    p = (void *) tp_alloc(sizeof(struct fnode));
  else {
    fnode_avails--;
    p = fnode_avail;
    fnode_avail = fnode_avail->next;
  }
  /* initialization */	
  if (0) {
    int i;
    for (i = 0; i < FMAX; i++)
      p->d[i] = FDEFAULT;
  }
  p->next = NULL;
  /* end of initialization */	
  return(p);
}  /* get_fnode */

/*************
 *
 *    free_fnode()
 *
 *************/

void free_fnode(struct fnode *p)
{
  fnode_frees++;
  fnode_avails++;
  p->next = fnode_avail;
  fnode_avail = p;
}  /* free_fnode */

/*************
 *
 *    print_mem()
 *
 *************/

void print_mem(FILE *fp)
{
  fprintf(fp, "\n------------- memory usage ------------\n");

  fprintf(fp, "%d mallocs of %d bytes each, %.1f K.\n",
	  Malloc_calls, TP_ALLOC_SIZE, (Malloc_calls * (TP_ALLOC_SIZE / 1024.)));

  fprintf(fp, "  type (bytes each)        gets      frees     in use      avail      bytes\n");
  fprintf(fp, "sym_ent (%4d)      %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct sym_ent), sym_ent_gets, sym_ent_frees, sym_ent_gets - sym_ent_frees, sym_ent_avails, (((sym_ent_gets - sym_ent_frees) + sym_ent_avails) * sizeof(struct sym_ent)) / 1024.);
  fprintf(fp, "term (%4d)         %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct term), term_gets, term_frees, term_gets - term_frees, term_avails, (((term_gets - term_frees) + term_avails) * sizeof(struct term)) / 1024.);
  fprintf(fp, "rel (%4d)          %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct rel), rel_gets, rel_frees, rel_gets - rel_frees, rel_avails, (((rel_gets - rel_frees) + rel_avails) * sizeof(struct rel)) / 1024.);
  fprintf(fp, "term_ptr (%4d)     %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct term_ptr), term_ptr_gets, term_ptr_frees, term_ptr_gets - term_ptr_frees, term_ptr_avails, (((term_ptr_gets - term_ptr_frees) + term_ptr_avails) * sizeof(struct term_ptr)) / 1024.);
  fprintf(fp, "formula_ptr_2 (%4d)%11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct formula_ptr_2), formula_ptr_2_gets, formula_ptr_2_frees, formula_ptr_2_gets - formula_ptr_2_frees, formula_ptr_2_avails, (((formula_ptr_2_gets - formula_ptr_2_frees) + formula_ptr_2_avails) * sizeof(struct formula_ptr_2)) / 1024.);
  fprintf(fp, "fpa_head (%4d)     %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct fpa_head), fpa_head_gets, fpa_head_frees, fpa_head_gets - fpa_head_frees, fpa_head_avails, (((fpa_head_gets - fpa_head_frees) + fpa_head_avails) * sizeof(struct fpa_head)) / 1024.);
  fprintf(fp, "fnode (%4d)        %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct fnode), fnode_gets, fnode_frees, fnode_gets - fnode_frees, fnode_avails, (((fnode_gets - fnode_frees) + fnode_avails) * sizeof(struct fnode)) / 1024.);
  fprintf(fp, "fpa_tree (%4d)     %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct fpa_tree), fpa_tree_gets, fpa_tree_frees, fpa_tree_gets - fpa_tree_frees, fpa_tree_avails, (((fpa_tree_gets - fpa_tree_frees) + fpa_tree_avails) * sizeof(struct fpa_tree)) / 1024.);
  fprintf(fp, "context (%4d)      %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct context), context_gets, context_frees, context_gets - context_frees, context_avails, (((context_gets - context_frees) + context_avails) * sizeof(struct context)) / 1024.);
  fprintf(fp, "trail (%4d)        %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct trail), trail_gets, trail_frees, trail_gets - trail_frees, trail_avails, (((trail_gets - trail_frees) + trail_avails) * sizeof(struct trail)) / 1024.);
  fprintf(fp, "imd_tree (%4d)     %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct imd_tree), imd_tree_gets, imd_tree_frees, imd_tree_gets - imd_tree_frees, imd_tree_avails, (((imd_tree_gets - imd_tree_frees) + imd_tree_avails) * sizeof(struct imd_tree)) / 1024.);
  fprintf(fp, "imd_pos (%4d)      %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct imd_pos), imd_pos_gets, imd_pos_frees, imd_pos_gets - imd_pos_frees, imd_pos_avails, (((imd_pos_gets - imd_pos_frees) + imd_pos_avails) * sizeof(struct imd_pos)) / 1024.);
  fprintf(fp, "is_tree (%4d)      %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct is_tree), is_tree_gets, is_tree_frees, is_tree_gets - is_tree_frees, is_tree_avails, (((is_tree_gets - is_tree_frees) + is_tree_avails) * sizeof(struct is_tree)) / 1024.);
  fprintf(fp, "is_pos (%4d)       %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct is_pos), is_pos_gets, is_pos_frees, is_pos_gets - is_pos_frees, is_pos_avails, (((is_pos_gets - is_pos_frees) + is_pos_avails) * sizeof(struct is_pos)) / 1024.);
  fprintf(fp, "fsub_pos (%4d)     %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct fsub_pos), fsub_pos_gets, fsub_pos_frees, fsub_pos_gets - fsub_pos_frees, fsub_pos_avails, (((fsub_pos_gets - fsub_pos_frees) + fsub_pos_avails) * sizeof(struct fsub_pos)) / 1024.);
  fprintf(fp, "literal (%4d)      %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct literal), literal_gets, literal_frees, literal_gets - literal_frees, literal_avails, (((literal_gets - literal_frees) + literal_avails) * sizeof(struct literal)) / 1024.);
  fprintf(fp, "clause (%4d)       %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct clause), clause_gets, clause_frees, clause_gets - clause_frees, clause_avails, (((clause_gets - clause_frees) + clause_avails) * sizeof(struct clause)) / 1024.);
  fprintf(fp, "list (%4d)         %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct list), list_gets, list_frees, list_gets - list_frees, list_avails, (((list_gets - list_frees) + list_avails) * sizeof(struct list)) / 1024.);
  fprintf(fp, "clash_nd (%4d)     %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct clash_nd), clash_nd_gets, clash_nd_frees, clash_nd_gets - clash_nd_frees, clash_nd_avails, (((clash_nd_gets - clash_nd_frees) + clash_nd_avails) * sizeof(struct clash_nd)) / 1024.);
  fprintf(fp, "clause_ptr (%4d)   %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct clause_ptr), clause_ptr_gets, clause_ptr_frees, clause_ptr_gets - clause_ptr_frees, clause_ptr_avails, (((clause_ptr_gets - clause_ptr_frees) + clause_ptr_avails) * sizeof(struct clause_ptr)) / 1024.);
  fprintf(fp, "ilist (%4d)        %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct ilist), ilist_gets, ilist_frees, ilist_gets - ilist_frees, ilist_avails, (((ilist_gets - ilist_frees) + ilist_avails) * sizeof(struct ilist)) / 1024.);
  fprintf(fp, "ci_ptr (%4d)       %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct ci_ptr), ci_ptr_gets, ci_ptr_frees, ci_ptr_gets - ci_ptr_frees, ci_ptr_avails, (((ci_ptr_gets - ci_ptr_frees) + ci_ptr_avails) * sizeof(struct ci_ptr)) / 1024.);
  fprintf(fp, "link_node (%4d)    %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct link_node), link_node_gets, link_node_frees, link_node_gets - link_node_frees, link_node_avails, (((link_node_gets - link_node_frees) + link_node_avails) * sizeof(struct link_node)) / 1024.);
  fprintf(fp, "ans_lit_node(%4d)  %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct ans_lit_node), ans_lit_node_gets, ans_lit_node_frees, ans_lit_node_gets - ans_lit_node_frees, ans_lit_node_avails, (((ans_lit_node_gets - ans_lit_node_frees) + ans_lit_node_avails) * sizeof(struct ans_lit_node)) / 1024.);
  fprintf(fp, "formula_box(%4d)   %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct formula_box), formula_box_gets, formula_box_frees, formula_box_gets - formula_box_frees, formula_box_avails, (((formula_box_gets - formula_box_frees) + formula_box_avails) * sizeof(struct formula_box)) / 1024.);
  fprintf(fp, "formula(%4d)       %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct formula), formula_gets, formula_frees, formula_gets - formula_frees, formula_avails, (((formula_gets - formula_frees) + formula_avails) * sizeof(struct formula)) / 1024.);
  fprintf(fp, "formula_ptr(%4d)   %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct formula_ptr), formula_ptr_gets, formula_ptr_frees, formula_ptr_gets - formula_ptr_frees, formula_ptr_avails, (((formula_ptr_gets - formula_ptr_frees) + formula_ptr_avails) * sizeof(struct formula_ptr)) / 1024.);
  fprintf(fp, "cl_attribute(%4d)  %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct cl_attribute), cl_attribute_gets, cl_attribute_frees, cl_attribute_gets - cl_attribute_frees, cl_attribute_avails, (((cl_attribute_gets - cl_attribute_frees) + cl_attribute_avails) * sizeof(struct cl_attribute)) / 1024.);
  fprintf(fp, "glist (%4d)        %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct glist), glist_gets, glist_frees, glist_gets - glist_frees, glist_avails, (((glist_gets - glist_frees) + glist_avails) * sizeof(struct glist)) / 1024.);
  fprintf(fp, "g2list (%4d)       %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct g2list), g2list_gets, g2list_frees, g2list_gets - g2list_frees, g2list_avails, (((g2list_gets - g2list_frees) + g2list_avails) * sizeof(struct g2list)) / 1024.);

}  /* print_mem */

/*************
 *
 *    print_mem_brief()
 *
 *************/

void print_mem_brief(FILE *fp)
{
  fprintf(fp, "\n------------- memory usage ------------\n");

  fprintf(fp, "%d mallocs of %d bytes each, %.1f K.\n",
	  Malloc_calls, TP_ALLOC_SIZE, (Malloc_calls * (TP_ALLOC_SIZE / 1024.)));

  fprintf(fp, "  type (bytes each)     gets      frees     in use      avail      bytes\n");
  fprintf(fp, "term (%4d)      %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct term), term_gets, term_frees, term_gets - term_frees, term_avails, (((term_gets - term_frees) + term_avails) * sizeof(struct term)) / 1024.);
  fprintf(fp, "rel (%4d)       %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct rel), rel_gets, rel_frees, rel_gets - rel_frees, rel_avails, (((rel_gets - rel_frees) + rel_avails) * sizeof(struct rel)) / 1024.);
  fprintf(fp, "term_ptr (%4d)  %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct term_ptr), term_ptr_gets, term_ptr_frees, term_ptr_gets - term_ptr_frees, term_ptr_avails, (((term_ptr_gets - term_ptr_frees) + term_ptr_avails) * sizeof(struct term_ptr)) / 1024.);
  fprintf(fp, "is_tree (%4d)   %11lu%11lu%11lu%11lu%9.1f K\n", (int) sizeof(struct is_tree), is_tree_gets, is_tree_frees, is_tree_gets - is_tree_frees, is_tree_avails, (((is_tree_gets - is_tree_frees) + is_tree_avails) * sizeof(struct is_tree)) / 1024.);
}  /* print_mem_brief */

/*************
 *
 *    int total_mem() -- How many K have been dynamically allocated?
 *
 *************/

int total_mem(void)
{
  return( (int) (Malloc_calls * (TP_ALLOC_SIZE / 1024.)));
}  /* total_mem */

/*************
 *
 *   total_mem_calls()
 *
 *************/

int total_mem_calls(void)
{
  return((int) (Malloc_calls));
}  /* total_mem_calls */

/*************
 *
 *    void print_linked_ur_mem_stats()
 *
 *************/

void print_linked_ur_mem_stats(void)
{

  printf("context gets=%lu frees=%lu inuse=%lu\n",context_gets, context_frees, context_gets-context_frees);
  printf("trail gets=%lu frees=%lu inuse=%lu\n",trail_gets, trail_frees, trail_gets-trail_frees);
  printf("fpa_tree gets=%lu frees=%lu inuse=%lu\n",fpa_tree_gets, fpa_tree_frees, fpa_tree_gets-fpa_tree_frees);
  printf("term gets=%lu frees=%lu inuse=%lu\n",term_gets, term_frees, term_gets-term_frees);
  printf("link_node gets=%lu frees=%lu inuse=%lu\n",link_node_gets,link_node_frees, link_node_gets-link_node_frees);

}  /* end print_linked_ur_mem_stats */

