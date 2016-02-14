/*
 *   fpa.c
 *
 *   This file has the routines for FPA-path indexing.  The indexing is
 *   similar to the FPA indexing in LMA/ITP, except that the properties
 *   are Stickel's "path properties".  (An old property is something like
 *   "the term has symbol b in position 2 1 3", and a path property is
 *   something like
 *   "the term has a path p 2 h 1 f 3 b".)
 *
 *   FPA indexing is used when searching for unifiable terms, as in inference
 *   rules and in unit conflict, and it is used when searching for instances,
 *   as in back subsumption.  (It can also be used when searching for
 *   more general terms, as in forward subsumption, demodulation,
 *   and unit_deletion, but discrimination tree indexing is usually better.)
 *
 */

/*
 *
 *   A property is a sequence of integers which alternate between symbol
 *   identifiers and argument positions:
 *
 *     <sym_num arg_pos sym_num arg_pos ... sym_num arg_pos sym_num>
 *
 *   The last sym_num can be 0, indicating a variable.
 *
 *   For example, the a in p(g(b),f(g(a),c)) has property <p 2 f 1 g 1 a>.
 *
 */

/*
 *
 * MESSY IMPLEMENTATION DETAIL:  Paths have one byte per member, plus
 * a word of 0s to mark the end.  When accessing members of a path,
 * we treat a path as an array of unsigned chars.  When comparing,
 * copying, and hashing paths, we treat them as arrays of ints (for
 * speed).  The "official" form (argument passing, etc) is as an array
 * of ints, because lint complains about possible alignment errors when
 * casting (unsigned char *) to (int *).
 *
 * The current position in the path (usually variable j) counts in bytes.
 *
 */

#include "header.h"

#define AND 1
#define OR 2
#define LEAF 3

/* MAX_PATH is in ints.  Overflow is not checked.  If fpa_depth is n, */
/* paths can be 2n+1 bytes. */
#define MAX_PATH 110

/*************
 *
 *   alloc_fpa_index()
 *
 *************/

struct fpa_index *alloc_fpa_index(void)
{
  struct fpa_index *p;
  int i;

  p = (struct fpa_index *) tp_alloc((int) sizeof(struct fpa_index));

  for (i = 0; i < FPA_SIZE; i++)
    p->table[i] = NULL;

  return(p);
}  /* alloc_fpa_index */

/*************
 *
 *    static void path_mark_end(path, j)
 *
 *    j (which counts bytes) is one past last entry.
 *
 *************/

static void path_mark_end(int *path,
			  int j)
{
  int i, k, m;
  unsigned char *cpath;

  cpath = (unsigned char *) path;

  /* make sure the rest of the integer, starting with j, and the */
  /* whole next integer (unless j is at beginning) are all 0. */

  m = j % sizeof(int);  /* position of j in an int */

  if (m == 0)
    i = sizeof(int);  /* just fill int with 0s */
  else
    i = (2 * sizeof(int)) - m;  /* 0 rest of int and next int */

  for (k = 0; k < i; k++)
    cpath[j++] = 0;

}  /* path_mark_end */

/*************
 *
 *    static int hash_path(path)
 *
 *************/

static int hash_path(int *path)
{
  int i, val;

  val = 0;

  for (i = 0; path[i] != 0; i++)
    val += path[i];

  return((unsigned) val % FPA_SIZE);
}  /* hash_path */

/*************
 *
 *    static int path_comp(p1, p2)
 *
 *************/

static int path_comp(int *p1,
		     int *p2)
{
  while (*p1 == *p2 && *p1 != 0 && *p2 != 0) {
    p1++;
    p2++;
  }

  if (*p1 < *p2)
    return(-1);
  else if (*p1 > *p2)
    return(1);
  else
    return(0);

}  /* path_comp */

/*************
 *
 *    static int path_size(path) -- in ints, including 0 word at end
 *
 *************/

static int path_size(int *path)
{
  int i;
  int *p1;

  for (i = 1, p1 = path; *p1 != 0; p1++, i++);

  return(i);


}  /* path_size */

/*************
 *
 *    static int *path_copy(path)
 *
 *************/

static int *path_copy(int *path)
{
  int i, j;
  int *p2;

  i = path_size(path);

  p2 = (int *) tp_alloc(i * (int) sizeof(int));

  for (j = 0; j < i; j++)
    p2[j] = path[j];

  return(p2);

}  /* path_copy */

/*************
 *
 *    static insert_fpa_tab(term, path, index)
 *
 *        Insert a term into an FPA indexing list.  Create a new list
 *    if necessary.  The path is something like "1 f 2 g 4 h 3 a".
 *
 *************/

static void insert_fpa_tab(struct term *t,
			   int *path,
			   struct fpa_index *index)
{
  int hashval, c;
  struct term_ptr *tp1, *tp2, *tp3;
  struct fpa_head *fp1, *fp2, *fp3;

  /* Treat path as integers here. */

  hashval = hash_path(path);
  fp1 = index->table[hashval];
  fp2 = NULL;

  while (fp1 != NULL &&
	 (c = path_comp(fp1->path, path)) == -1) {
    fp2 = fp1;
    fp1 = fp1->next;
  }

  if (fp1 == NULL || c != 0) { /* need new fpa_head */
    fp3 = get_fpa_head();
    fp3->path = path_copy(path);
    fp3->terms = flist_insert(NULL, t);

    if (fp2 == NULL) {
      /* insert at beginning */
      fp3->next = index->table[hashval];
      index->table[hashval] = fp3;
    }
    else {  /* insert after fp2 */
      fp3->next = fp1;
      fp2->next = fp3;
    }
  }

  else { /* we have a matching fpa_head, so insert t in its list */
    fp1->terms = flist_insert(fp1->terms, t);
  }
}  /* insert_fpa_tab */

/*************
 *
 *    static delete_fpa_tab(term, path, database)
 *
 *        Delete a term from an FPA indexing list.  It is assumed that
 *    the corresponding `insert_fpa_tab' was previously made.
 *
 *************/

static void delete_fpa_tab(struct term *t,
			   int *path,
			   struct fpa_index *index)
{
  int hashval;
  struct term_ptr *tp1, *tp2;
  struct fpa_head *fp1, *fp2;

  /* Treat path as integers here. */

  hashval = hash_path(path);
  fp1 = index->table[hashval];
  fp2 = NULL;

  while (fp1 != NULL && path_comp(fp1->path, path) != 0) {
    fp2 = fp1;
    fp1 = fp1->next;
  }

  if (fp1 == NULL)
    Stats[FPA_UNDERLOADS]++;  /* fpa list not found */
  else { /* we have a matching fpa_head, so look for t in its list */
    fp1->terms = flist_delete(fp1->terms, t);
    if (fp1->terms == NULL) { /* delete fpa_head also */
      if (fp2 == NULL)
	index->table[hashval] = fp1->next;
      else
	fp2->next = fp1->next;
      free_fpa_head(fp1);
      /* don't worry about fp1->path; let it be lost forever */
    }
  }
}  /* delete_fpa_tab */

/*************
 *
 *   void term_fpa_rec
 *
 *   Recursive procedure called by fpa_insert and fpa_delete.
 *
 *************/

void term_fpa_rec(int insert,
		  struct term *t,
		  struct term *super_term,
		  struct fpa_index *index,
		  int *path,
		  int j,
		  int bound)
{
  int i;
  struct rel *r;
  unsigned char *cpath;

  cpath = (unsigned char *) path;

  /* `path' has the path from super_term to t */

  if (t->type == VARIABLE) /* variable contributes nothing */
    cpath[j++] = 0;
  else
    cpath[j++] = t->sym_num;

  /* insert or delete path */

  path_mark_end(path, j);
  if (insert)
    insert_fpa_tab(super_term, path, index);
  else
    delete_fpa_tab(super_term, path, index);

  if (t->type == COMPLEX && bound > 0) {
    i = 1;
    r = t->farg;
    while (r != NULL) {
      cpath[j] = i++;
      term_fpa_rec(insert, r->argval, super_term, index, path, j+1, bound-1);
      r = r->narg;
    }
  }
}  /* term_fpa_rec */

/*************
 *
 *    void fpa_insert(term, level, database)
 *
 *        Insert a term into an FPA indexing database.  Level == 0
 *    gives indexing on functor only.  With the term f(a,x,g(b)),
 *    Level == 1 gives indexing on f, a, x, and g.
 *
 *************/

void fpa_insert(struct term *t,
		int level,
		struct fpa_index *index)
{
  static int atom_count, term_count;

  int path[MAX_PATH];

  /* t->fpa_id is used to order FPA lists.  Assign one if necessary. */
  if (t->fpa_id == 0) {
    if (t->type != VARIABLE && t->varnum != 0)
      t->fpa_id = ++atom_count;
    else
      t->fpa_id = ++term_count;
  }
  term_fpa_rec(1, t, t, index, path, 0, level);
}  /* fpa_insert */

/*************
 *
 *    void fpa_delete(term, level, database)
 *
 *        Delete a term from an FPA indexing database.   The level
 *    must be the same as when the term was given to fpa_insert.
 *
 *************/

void fpa_delete(struct term *t,
		int level,
		struct fpa_index *index)
{
  int path[MAX_PATH];

  term_fpa_rec(0, t, t, index, path, 0, level);
}  /* fpa_delete */

/*************
 *
 *    static struct fpa_tree *get_leaf_node(path, index)
 *
 *        Given a path, if an FPA list exists, then return it in a
 *    leaf node; else return(NULL).
 *
 *************/

static struct fpa_tree *get_leaf_node(int *path,
				      struct fpa_index *index)
{
  struct fpa_head *fp;
  struct fpa_tree *pp;
  int c;

  fp = index->table[hash_path(path)];
  while (fp && (c = path_comp(fp->path,path)) == -1)
    fp = fp->next;
  if (!fp || c != 0)
    return(NULL);
  else {
    pp = get_fpa_tree();
    pp->type = LEAF;
    pp->path = path;  /* Note that whole path is not being copied. */
    pp->position = first_fpos(fp->terms);
    return(pp);
  }
}  /* get_leaf_node */

/*************
 *
 *    static int all_args_vars(t) -- are all subterms variables?
 *
 *************/

static int all_args_vars(struct term *t)
{
  struct rel *r;

  if (t->type != COMPLEX)
    return(0);
  else {
    r = t->farg;
    while (r != NULL) {
      if (r->argval->type != VARIABLE)
	return(0);
      r = r->narg;
    }
    return(1);
  }
}  /* all_args_vars */

/*************
 *
 *    static struct fpa_tree *build_tree_local(term, unif_type, path, bound, database)
 *
 *        Return an FPA indexing tree--to be used with a sequence
 *    of get_next calls.
 *
 *        term:       An error if (term->type == VARIABLE && unif_type != 3)
 *                    because everything satisfies that query.
 *        unif_type:  UNIFY, INSTANCE, MORE_GEN
 *        path:   must be 0 on initial call
 *        bound:      indexing bound (must be <= fpa_insert bound)
 *        database:
 *
 *    Note:  If an appathriate fpa list does not exit, then part of
 *    the tree can sometimes be deleted.  For example, if you want
 *    a tree to find unifiers for p(a), then normally, the tree will be
 *
 *                       OR
 *                     /    \
 *            `variable'     \
 *                            AND
 *                          /     \
 *                        /         \
 *                     `p a'         \
 *                               `p variable'
 *
 *    But if the fpa list for `variable' does not exist, then this
 *    routine will produce
 *
 *                            AND
 *                          /     \
 *                        /         \
 *                     `p a'         \
 *                               `p variable'
 *
 *************/

static struct fpa_tree *build_tree_local(struct term *t,
					 int u_type,
					 int *path,
					 int j,
					 int bound,
					 struct fpa_index *index)
{
  int i, empty;
  struct rel *r;
  struct fpa_tree *p1, *p2, *p3;
  unsigned char *cpath;

  cpath = (unsigned char *) path;

  /* `path' has the path to `t' */

  if (t->type == VARIABLE) { /* variable */
    if (u_type != MORE_GEN) { /* error if not "more general" */
      abend("build_tree_local, var and not more general.");
      return(NULL);  /* to quiet lint */
    }
    else {
      cpath[j++] = 0;
      path_mark_end(path, j);
      p1 = get_leaf_node(path, index);
      return(p1);
    }
  }
  else {  /* NAME or COMPLEX */
    cpath[j++] = t->sym_num;
    if (t->type == NAME || bound == 0 || (u_type != MORE_GEN && all_args_vars(t))) {
      path_mark_end(path, j);
      p2 = get_leaf_node(path, index);
    }
    else {

      i = 1;
      empty = 0;
      p2 = NULL;
      r = t->farg;
      while (r != NULL && empty == 0) {
	cpath[j] = i++;
	/* skip this arg if var and "unify" or "instance" */
	if (r->argval->type != VARIABLE || u_type == MORE_GEN) {
	  p3 = build_tree_local(r->argval, u_type, path, j+1, bound-1, index);
	  if (p3 == NULL) {
	    if (p2 != NULL) {
	      zap_prop_tree(p2);
	      p2 = NULL;
	    }
	    empty = 1;
	  }
	  else if (p2 == NULL)
	    p2 = p3;
	  else {
	    p1 = get_fpa_tree();
	    p1->type = AND; /* and */
	    p1->left = p2;
	    p1->right = p3;
	    p2 = p1;
	  }
	}
	r = r->narg;
      }
    }

    if (u_type != INSTANCE) { /* if we don't want instances only, */
      cpath[j-1] = 0;
      path_mark_end(path, j);
      p3 = get_leaf_node(path, index); /* variable */
    }
    else
      p3 = NULL;

    if (p2 == NULL)
      return(p3);
    else if (p3 == NULL)
      return(p2);
    else {  /* OR them together */
      p1 = get_fpa_tree();
      p1->type = OR; /* OR */
      p1->left = p2;
      p1->right = p3;
      return(p1);
    }
  }
}  /* build_tree_local */

/*************
 *
 *    struct fpa_tree *build_tree(t, u_type, bound, index)
 *
 *************/

struct fpa_tree *build_tree(struct term *t,
			    int u_type,
			    int bound,
			    struct fpa_index *index)
{
  int path[MAX_PATH];

  return(build_tree_local(t, u_type, path, 0, bound, index));

}  /* build_tree */


/*************
 *
 *    struct term *next_term(tree, maximum)
 *
 *        Get the first or next term that satisfies a unification condition.
 *    (Unification conditions are provided by `build_tree'.)
 *    `maximum' must be 0 on nonresursive calls.  A return of NULL indicates
 *    that there are none or no more terms that satisfy (and the tree has
 *    been deallocated).  If you want to stop getting terms before a NULL
 *    is returned, then please deallocate the tree with zap_prop_tree(tree).
 *
 *    Warning: a return of NULL means that the tree has been deallocated
 *
 *************/

struct term *next_term(struct fpa_tree *n,
		       int max)
{
  struct term_ptr *tp;
  struct term *t1, *t2;

  if (n == NULL)
    return(NULL);

  else if (n->type == LEAF) {  /* LEAF node */
    struct term *t = FTERM(n->position);
    while (t != NULL && max != 0 && t->fpa_id > max) {
      n->position = next_fpos(n->position);
      t = FTERM(n->position);
    }
    if (t == NULL) {
      zap_prop_tree(n);
      return(NULL);
    }
    else {
      n->position = next_fpos(n->position);
      return(t);
    }
  }

  else if (n->type == AND) {  /* AND node */
    t1 = next_term(n->left, max);
    if (t1 != NULL)
      t2 = next_term(n->right, t1->fpa_id);
    else
      t2 = (struct term *) 1;  /* anything but NULL */
    while (t1 != t2 && t1 != NULL && t2 != NULL) {
      if (t1->fpa_id > t2->fpa_id)
	t1 = next_term(n->left, t2->fpa_id);
      else
	t2 = next_term(n->right, t1->fpa_id);
    }
    if (t1 == NULL || t2 == NULL) {
      if (t1 == NULL)
	n->left = NULL;
      if (t2 == NULL)
	n->right = NULL;
      zap_prop_tree(n);
      return(NULL);
    }
    else
      return(t1);
  }

  else {  /* OR node */
    /* first get the left term */
    t1 = n->left_term;
    if (t1 == NULL) {
      /* it must be brought up */
      if (n->left != NULL) {
	t1 = next_term(n->left, max);
	if (t1 == NULL)
	  n->left = NULL;
      }
    }
    else  /* it was saved from a previous call */
      n->left_term = NULL;
    /* at this point, n->left_term == NULL */

    /* now do the same for the right side */
    t2 = n->right_term;
    if (t2 == NULL) {
      if (n->right != NULL) {
	t2 = next_term(n->right, max);
	if (t2 == NULL)
	  n->right = NULL;
      }
    }
    else
      n->right_term = NULL;

    /* now decide which of of t1 and t2 to return */
    if (t1 == NULL) {
      if (t2 == NULL) {
	zap_prop_tree(n);
	return(NULL);
      }
      else
	return(t2);
    }
    else if (t2 == NULL)
      return(t1);
    else if (t1 == t2)
      return(t1);
    else if (t1->fpa_id > t2->fpa_id) {
      n->right_term = t2;  /* save t2 for next time */
      return(t1);
    }
    else {
      n->left_term = t1;  /* save t1 for next time */
      return(t2);
    }
  }
}  /* next_term */

/*************
 *
 *    struct fpa_tree *build_for_all(db)
 *
 *    For those times when one must have everything (paramodulation
 *    from a variable, and paramodulation into a variable).
 *    (Build a tree that OR's together all of the FPA lists in db.)
 *
 *************/

struct fpa_tree *build_for_all(struct fpa_index *index)
{
  struct fpa_head *h;
  struct fpa_tree *p1, *p2, *p3;
  int i;

  p1 = NULL;
  for (i = 0; i < FPA_SIZE; i++) {
    h = index->table[i];
    while (h != NULL) {
      p2 = get_fpa_tree();
      p2->type = LEAF;
      p2->path = h->path;
      p2->position = first_fpos(h->terms);
      if (p1 == NULL)
	p1 = p2;
      else {
	p3 = get_fpa_tree();
	p3->type = OR;
	p3->left = p1;
	p3->right = p2;
	p1 = p3;
      }
      h = h->next;
    }
  }
  return(p1);
}  /* build_for_all */

/*************
 *
 *    zap_prop_tree(tree) -- Dealocate an FPA indexing tree.
 *
 *       `next_term' deallocates the tree as it proceeds, so it is not
 *    necessary to call zap_prop_tree if the most recent call to
 *    `next_term' returned NULL.
 *
 *************/

void zap_prop_tree(struct fpa_tree *n)
{
  if (n != NULL) {
    zap_prop_tree(n->left);
    zap_prop_tree(n->right);
    free_fpa_tree(n);
  }
}  /* zap_prop_tree */

/*************
 *
 *    print_fpa_tab(file_ptr, database) --  Display all FPA lists in database.
 *
 *************/

void print_fpa_tab(FILE *fp,
		   struct fpa_index *index)
{
  int i;
  struct fpa_head *f;

  fprintf(fp, "\nfpa index %p\n", (void *) index);
  for (i=0; i<FPA_SIZE; i++)
    if (index->table[i] != NULL) {
      fprintf(fp, "bucket %d\n", i);
      f = index->table[i];
      while (f != NULL) {
	struct fposition fpos;
	struct term *t;
	print_path(fp, f->path);
	fpos = first_fpos(f->terms);
	t = FTERM(fpos);
	while (t != NULL) {
	  fprintf(fp, " ");
	  print_term(fp, t);
	  fpos = first_fpos(f->terms);
	  t = FTERM(fpos);
	}
	fprintf(fp, "\n");
	f = f->next;
      }
    }
}  /* print_fpa_tab */

/*************
 *
 *    p_fpa_tab(index)
 *
 *************/

void p_fpa_tab(struct fpa_index *index)
{
  print_fpa_tab(stdout, index);
}  /* p_fpa_tab */

/*************
 *
 *    print_prop_tree(file_ptr, tree, level)
 *
 *        Display an FPA lookup tree that has been returned from
 *    build_tree.  Level should be 0 on initial call.
 *
 *************/

void print_prop_tree(FILE *fp,
		     struct fpa_tree *n,
		     int level)
{
  int i;

  if (n != NULL) {

    for (i=0; i<level; i++)
      fprintf(fp, "  ");
    if (n->type == AND)
      fprintf(fp, "and\n");
    else if (n->type == OR)
      fprintf(fp, "or\n");
    else
      print_path(fp, n->path);
    print_prop_tree(fp, n->left, level+1);
    print_prop_tree(fp, n->right, level+1);
  }
}  /* print_prop_tree */

/*************
 *
 *    p_prop_tree(t)
 *
 *************/

void p_prop_tree(struct fpa_tree *n)
{
  print_prop_tree(stdout, n, 0);
}  /* p_prop_tree */

/*************
 *
 *    print_path(fp, path) -- print an fpa path to a file
 *
 *************/

void print_path(FILE *fp,
		int *path)
{
  int i;
  char *sym;
  unsigned char *cpath;

  cpath = (unsigned char *) path;

  /* example [f,2,g,1,f,1,h,1,a] */

  fprintf(fp, "[");
  for (i = 0; cpath[i] != 0 || cpath[i+1] != 0 ; i++) {
    if (i % 2 == 0) {
      sym = sn_to_str( (short) cpath[i]);
      if (sym[0] == '\0')
	sym = "*";
      fprintf(fp, "%s", sym);
    }
    else
      fprintf(fp, "%d", cpath[i]);
    if (cpath[i+1] != 0 || cpath[i+1] != 0)
      fprintf(fp, ",");
    else
      fprintf(fp, "]\n");
  }
}  /* print_path */

/*************
 *
 *    p_path(path) -- print an fpa path
 *
 *************/

void p_path(int *path)
{
  print_path(stdout, path);
}  /* p_path */

/*************
 *
 *   int new_sym_num()
 *
 *   Return the next available symbol number.
 *
 *   The rightmost 8 bits will not be all zero.
 *   This is so that fpa will not map sym_nums to 0 (the
 *   code for variables).
 *
 *************/

int new_sym_num(void)
{
  static int sym_ent_count;

  sym_ent_count++;

  if (sym_ent_count % 256 == 0)
    sym_ent_count++;

  if (sym_ent_count > MAX_UNSIGNED_SHORT)
    abend("new_sym_num: too many symbols requested.");

  return(sym_ent_count);

}  /* new_sym_num */

