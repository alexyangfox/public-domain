/*

multijust.c -- proof-shortening experiment

*/

#include "header.h"

/* Alists is an array, indexed by true clause IDs.  Each entry
   contains a glist of justification lists.  The justification
   lists contain (true) clause IDs only, no inference rule codes.
   If an entry is NULL, then that clause is either input or
   irrelevant to the search for short proofs.

   Blists is a renumbering of Alists, starting with 0.  It is
   indexed by the renamed Ids.  There are no gaps in Blists,
   that is, Bsize is the number of relevant clauses, including
   input clauses.

   Arrays A_to_B and B_to_A give the correspondence.  The irrelevant
   entries in A_to_B are -1.  B_to_A is all relevant.
*/

static int Asize;
static int Bsize;
static struct glist **Alists;
static struct glist **Blists;
static int *A_to_B;
static int *B_to_A;

static struct ilist *Demod_map;  /* Those darn "25 [new_demod,24]" clauses. */

/*************
 *
 *   remove_those_containing()
 *
 *************/

static struct glist *remove_those_containing(struct glist *a, int i)
{
  if (a == NULL)
    return NULL;
  else if (ilist_member(i, a->v)) {
    struct glist *b = a->next;
    free_ilist_list(a->v);
    free_glist(a);
    return remove_those_containing(b, i);
  }
  else {
    a->next = remove_those_containing(a->next, i);
    return a;
  }
}  /* remove_those_containing */

/*************
 *
 *   keep_only()
 *
 *************/

struct glist *keep_only(struct glist *a, int i)
{
  if (a == NULL)
    return NULL;
  else if (i == 0) {
    free_glist_of_ilists(a);
    return NULL;
  }
  else {
    a->next = keep_only(a->next, i-1);
    return a;
  }
}  /* keep_only */

/*************
 *
 *   remove_supersets()
 *
 *************/

struct glist *remove_supersets(struct glist *a, struct ilist *s)
{
  if (a == NULL)
    return NULL;
  else if (iset_subset(s, a->v)) {
    struct glist *b = a->next;
    free_ilist_list(a->v);
    free_glist(a);
    return remove_supersets(b, s);
  }
  else {
    a->next = remove_supersets(a->next, s);
    return a;
  }
}  /* remove_supersets */

/*************
 *
 *   remove_all_supersets()
 *
 *************/

struct glist *remove_all_supersets(struct glist *a, struct glist *b)
{
  if (b == NULL)
    return a;
  else if (member_is_subset(a, b->v)) {
    struct glist *c = b->next;
    free_ilist_list(b->v);
    free_glist(b);
    return remove_all_supersets(a, c);
  }
  else {
    struct glist *c = b->next;
    a = remove_supersets(a, b->v);
    b->next = NULL;
    a = glist_append(a, b);
    return remove_all_supersets(a, c);
  }
}  /* remove_all_supersets */

/*************
 *
 *   g2_superset()
 *
 *   Is p a superset of any v2 (second component) of g2?
 *
 *************/

static int g2_superset(struct g2list *g2, struct ilist *p)
{
  if (g2 == NULL)
    return 0;
  else if (iset_subset(g2->v2, p))
    return 1;
  else
    return g2_superset(g2->next, p);
}  /* g2_superset */

/*************
 *
 *   g2_remove_supersets()
 *
 *   Foreach v2 (second compnent) of g2, if v2 is a superset of p,
 *   remove the g2 node.
 *
 *************/

struct g2list *g2_remove_supersets(struct g2list *g2, struct ilist *p)
{
  if (g2 == NULL)
    return NULL;
  else {
    g2->next = g2_remove_supersets(g2->next, p);
    if (!iset_subset(p, g2->v2))
      return g2;
    else {
      struct g2list *h2 = g2->next;
      free_ilist_list(g2->v1);
      free_ilist_list(g2->v2);
      free_g2list(g2);
      return h2;
    }
  }
}  /* g2_remove_supersets */

/*************
 *
 *   append_parents()
 *
 *************/

static void append_parents(struct clause *c,
			   struct ilist *j1,
			   struct ilist *j2)
{
  /* j1 is an ordinary justification (parent IDs, inf rules, positions) */
  /* j2 is a supporters list (parent IDs only) */

  if (g2_superset(c->multi_parents, j2))
    return;
  else {
    struct g2list *p;
    c->multi_parents = g2_remove_supersets(c->multi_parents, j2);
    p = get_g2list();
    p->v1 = copy_ilist(j1);
    p->v2 = copy_ilist(j2);
    if (c->multi_parents == NULL)
      c->multi_parents = p;
    else {
      struct g2list *q = c->multi_parents;
      while (q->next != NULL)
	q = q->next;
      q->next = p;
    }
  }
}  /* append_parents */

/*************
 *
 *   input_clause()
 *
 *************/

int input_clause(int id)
{
  struct clause *c = cl_find(id);
  if (c == NULL)
    return 0;
  else if (c->parents != NULL)
    return 0;
  else
    return 1;
}  /* input_clause */

/*************
 *
 *   first_just_input_only()
 *
 *************/

int first_just_input_only(struct g2list *p)
{
  struct ilist *b;
  for (b = p->v2; b != NULL; b = b->next) {
    if (!input_clause(b->i))
      break;  /* fail */
  }
  return (b == NULL);
}  /* first_just_input_only */

/*************
 *
 *   all_supporters_less_than()
 *
 *************/

int all_supporters_less_than(struct clause *c, struct ilist *supporters)
{
  struct ilist *b;
  for (b = supporters; b != NULL; b = b->next) {
    if (b->i >= c->id)
      break;  /* fail */
  }
  return (b == NULL);
}  /* all_supporters_less_than */

/*************
 *
 *   derived_from_itself()
 *
 *************/

int derived_from_itself(struct clause *c, struct ilist *supporters)
{
  return (ilist_member(c->id, supporters));
}  /* derived_from_itself */

/*************
 *
 *   proof_not_longer_than()
 *
 *************/

int proof_not_longer_than(struct clause *c, struct ilist *p)
{
  /* Return TRUE if the proof length of some new clauses (justification p)
     is no longer than the proof length of c (using c->parents).

     The routine that gets the proof length takes a clause as input,
     so we construct a dummy clause for the justification p.
  */
  int clen, dlen;
  struct clause *d = get_clause();
  d->parents = p;
  dlen = proof_length(d);
  free_clause(d);
  clen = proof_length(c);
  return (dlen <= clen);
}  /* proof_not_longer_than */

/*************
 *
 *   possibly_append_parents()
 *
 *   c is a previously kept clause, and ip is the (ordinary)
 *   justification of a newly-derived copy of c.  This routine
 *   decides whether to add the new justification to c.
 *
 *************/

void possibly_append_parents(struct clause *c, struct ilist *ip)
{
  struct ilist *supporters = just_to_supporters(ip);

  if (c->parents == NULL)
    ; /* Don't add the new justification, because c is input. */

  else if (c->multi_parents == NULL)
    append_parents(c, ip, supporters);  /* First time we have derived c. */

  else if (Parms[MULTI_JUST_MAX].val != -1 &&
	   g2list_length(c->multi_parents) >= Parms[MULTI_JUST_MAX].val)
    ; /* Don't add new justification. */

  else if (first_just_input_only(c->multi_parents))
    ; /* Don't add new justification, because c can be derived from input. */

  else if (derived_from_itself(c, supporters))
    ; /* Don't add new justification. */

  else if (Flags[MULTI_JUST_LESS].val) {
    if (all_supporters_less_than(c, supporters))
      append_parents(c, ip, supporters);
  }

  else if (Flags[MULTI_JUST_SHORTER].val) {
    if (proof_not_longer_than(c, ip))
      append_parents(c, ip, supporters);
  }
  else
    append_parents(c, ip, supporters);
  free_ilist_list(supporters);
}  /* possibly_append_parents */

/*************
 *
 *   print_multi_supporters()
 *
 *************/

static void print_multi_supporters(FILE *fp, int id, struct glist *p)
{
  fprintf(fp, "CLAUSE %d", id);
  for ( ; p != NULL; p = p->next) {
    struct ilist *ip;
    fprintf(fp, "\n   ");
    for (ip = p->v; ip != NULL; ip = ip->next) {
      fprintf(fp, " %d", ip->i);
    }
    fprintf(fp, " -1");
  }
  fprintf(fp, " \n-2\n");
}  /* print_multi_supporters */

/*************
 *
 *   just_lists_to_supporters_lists()
 *
 *************/

static struct glist *just_lists_to_supporters_lists(struct g2list *p)
{
  if (p == NULL)
    return NULL;
  else {
    struct glist *a = get_glist();
    a->v = copy_ilist(p->v2);
    a->next = just_lists_to_supporters_lists(p->next);
    return a;
  }
}  /* just_lists_to_supporters_lists */

/*************
 *
 *   build_support_lists()
 *
 *************/

static void build_support_lists(struct clause *c)
{
  if (Alists[c->id] == NULL) {
    struct glist *p;
    p = just_lists_to_supporters_lists(c->multi_parents);
#if 0  /* These operations are now done when adding justifications. */
    p = remove_all_supersets(NULL, p);
    p = remove_those_containing(p, c->id);
    if (Parms[MULTI_JUST_MAX].val != -1)
      p = keep_only(p, Parms[MULTI_JUST_MAX].val);
#endif
    Alists[c->id] = p;
    for (p = Alists[c->id]; p != NULL; p = p->next) {  /* foreach just list */
      struct ilist *ip;
      for (ip = p->v; ip != NULL; ip = ip->next) {  /* foreach ID */
	struct clause *d = cl_find(ip->i);
	if (d == NULL)
	  abend("build_support_lists: clause not found");
	build_support_lists(d);
      }      
    }
  }    
}  /* build_support_lists */

/*************
 *
 *   map_demod()
 *
 *************/

int map_demod(struct ilist *p, int i)
{
  if (p == NULL)
    return 0;
  else if (p->i == i)
    return (p->next != NULL ? p->next->i : 0);
  else
    return map_demod(p->next->next, i);
}  /* map_demod */

/*************
 *
 *   collapse_new_demod2()
 *
 *   We do this because of the darn "25 [new_demod,24]" clauses.
 *
 *   We remove clause 25 from Alists; and in the justifications,
 *   change all occurrences of 25 to 24.
 *
 *************/

static void collapse_new_demod2(void)
{
  int i;
  for (i = 0; i < Asize; i++) {
    if (Alists[i] != NULL) {
      struct clause *c = cl_find(i);
      if (c->multi_parents != NULL &&
	  c->multi_parents->next == NULL) {
	struct ilist *p = c->multi_parents->v1;
	if (p->i == NEW_DEMOD_RULE) {
	  int id = p->next->i;
	  int j;
	  printf("\nremoving %d and changing references to %d\n", i, id);
	  Alists[i] = NULL;
	  for (j = 0; j < Asize; j++) {
	    struct glist *g;
	    for (g = Alists[j]; g != NULL; g = g->next) {
	      struct ilist *p;
	      for (p = g->v; p != NULL; p = p->next) {
		if (p->i == i) {
		  p->i = id;
		  Demod_map = ilist_tack_on(Demod_map, i);
		  Demod_map = ilist_tack_on(Demod_map, id);
		}
	      }
	    }
	  }
	}
      }
    }
  }
}  /* collapse_new_demod2 */

/*************
 *
 *   multi_map()
 *
 *************/

static void multi_map(void)
{
  int i;
  struct ilist *inputs = NULL;
  struct ilist *ip;

  Bsize = 0;

  /* Alists has lots of gaps in the numbering.  This routine
     builds Blists, numbered 0,1,2,...  
     Arrays A_to_B and B_to_A give the correspondence.

     There is ambiguity in Alists: NULL means either "clause is irrelelvant"
     or "clause is input".  In Blists, all are relevant, and NULL means
     "input".  So we go through Alists, finding input clauses (goals without
     0 in Alist), and initialize Blists to those.
  */

  for (i = 0; i < Asize; i++) {
    if (Alists[i] != NULL) {
      struct glist *d;
      for (d = Alists[i]; d != NULL; d= d->next) {
	for (ip = d->v; ip != NULL; ip = ip->next) {
	  if (Alists[ip->i] == NULL)
	    inputs = iset_add(ip->i, inputs);
	}
      }
    }
  }
  
  A_to_B = calloc(Asize, sizeof(int));

  for (i = 0; i < Asize; i++)
    A_to_B[i] = -1;

  for (ip = inputs; ip != NULL; ip = ip->next) {
    A_to_B[ip->i] = Bsize++;
  }
  free_ilist_list(inputs);
  
  /* Finish setting up the A_to_B map. */

  for (i = 0; i < Asize; i++) {
    if (Alists[i] != NULL) {
      A_to_B[i] = Bsize++;
    }
  }

  /* Now we know how many Bs there are, so allocate the B arrays. */
    
  Blists = calloc(Bsize, sizeof(struct glist *));
  B_to_A = calloc(Bsize, sizeof(int));

  for (i = 0; i < Asize; i++) {
    if (Alists[i] != NULL) {
      struct glist *d;
      Blists[A_to_B[i]] = copy_glist(Alists[i]);
      for (d = Blists[A_to_B[i]]; d != NULL; d= d->next) {
	d->v = copy_ilist(d->v);
	for (ip = d->v; ip != NULL; ip = ip->next) {
	  ip->i = A_to_B[ip->i];
	}
      }
    }
  }

  /* Finally, set up the B_to_A map. */
  for (i = 0; i < Asize; i++)
    if (A_to_B[i] != -1)
      B_to_A[A_to_B[i]] = i;

}  /* multi_map */


/*

======================================================================
SET OPERATIONS
======================================================================

*/

#define INT_BIT 32
static int Set_size;          /* in integers */
static unsigned Compares;

/*************
 *
 *   set_jset_size()
 *
 *************/

void set_jset_size(int n)
{
  Set_size = ((n-1) / INT_BIT) + 1;
}  /* set_jset_size */

/*************
 *
 *   get_jset()
 *
 *************/

int *get_jset(void)
{
  return calloc(Set_size, sizeof(int));
}  /* get_jset */

/*************
 *
 *   copy_jset()
 *
 *************/

int *copy_jset(int *a)
{
  int *b = get_jset();
  int i;
  for (i = 0; i < Set_size;i++)
    b[i] = a[i];
  return b;
}  /* copy_jset */

/*************
 *
 *   jset_member()
 *
 *************/

int jset_member(int *s, int n)
{
  int i = n / INT_BIT;
  int j = n % INT_BIT;
  return s[i] & (1<<j);
}  /* jset_member */

/*************
 *
 *   add_to_jset()
 *
 *************/

void add_to_jset(int *s, int n)
{
  int i = n / INT_BIT;
  int j = n % INT_BIT;
  s[i] = s[i] | (1<<j);
}  /* add_to_jset */

/*************
 *
 *   remove_from_jset()
 *
 *************/

void remove_from_jset(int *s, int n)
{
  int i = n / INT_BIT;
  int j = n % INT_BIT;
  s[i] = s[i] & ~(1<<j);
}  /* remove_from_jset */

/*************
 *
 *   j_compare()
 *
 *   LESS_THAN, GREATER_THAN, SAME_AS
 *
 *************/

static int j_compare(int *s1, int *s2)
{
  int i = 0;
  Compares++;
  while (i < Set_size && s1[i] == s2[i])
    i++;
  if (i == Set_size)
    return SAME_AS;
  else if (s1[i] < s2[i])
    return LESS_THAN;
  else
    return GREATER_THAN;
}  /* j_compare */

/*

======================================================================
HASH TABLE
======================================================================

*/

/* #define JHASH_SIZE 2000000 */
#define JHASH_SIZE 1999993        /* prime */

static struct glist **Jhash;
static unsigned int Lookups;
static unsigned int Crashes;

/*************
 *
 *   jhash_init()
 *
 *************/

static void jhash_init(void)
{
  Jhash = calloc(JHASH_SIZE, sizeof(struct glist *));
}  /* jhash_init */

/*************
 *
 *   jhash()
 *
 *************/

static int jhash(int *s)
{
  int i;
  int val = 0;
  for (i = 0, val=0; i < Set_size; i++)
    val += s[i];
  return abs(val % JHASH_SIZE);
}  /* jhash */

/*************
 *
 *   jhash_member_recurse()
 *
 *************/

static int jhash_member_recurse(int *s, struct glist *p)
{
  if (p == NULL)
    return 0;
  else {
    int comp = j_compare(p->v, s);
    if (comp == SAME_AS)
      return 1;
    else if (comp == GREATER_THAN)
      return 0;
    else {
      Crashes++;
      return jhash_member_recurse(s, p->next);
    }
  }
}  /* jhash_member_recurse */

/*************
 *
 *   jhash_member()
 *
 *************/

static int jhash_member(int *s)
{
  int h = jhash(s);
  Lookups++;
  return jhash_member_recurse(s, Jhash[h]);
}  /* jhash_member */

/*************
 *
 *   jhash_insert_recurse()
 *
 *************/

static struct glist *jhash_insert_recurse(int *s, struct glist *p)
{
  if (p == NULL) {
    p = get_glist();
    p->v = s;
    return p;
  }
  else {
    int comp = j_compare(p->v, s);
    if (comp == SAME_AS) {
      abend("jhash_inset_recurse: already there");
      return p;  /* to avoid compiler warning */
    }
    else if (comp == GREATER_THAN) {
      struct glist *q = get_glist();
      q->v = s;
      q->next = p;
      return q;
    }
    else {
      p->next = jhash_insert_recurse(s, p->next);
      return p;
    }
  }
}  /* jhash_insert_recurse */

/*************
 *
 *   jhash_insert()
 *
 *   Do not copy the set.  If already there, ABEND.
 *
 *************/

static void jhash_insert(int *s)
{
  int h = jhash(s);
  Jhash[h] = jhash_insert_recurse(s, Jhash[h]);
}  /* jhash_insert */

/*************
 *
 *   print_set()
 *
 *************/

void print_set(int *s)
{
  int i, j;
  printf("(");
  for (i = 0; i < Set_size; i++) {
    int k = s[i];
    for (j = 0; j < INT_BIT; j++) {
      if ((k>>j) & 1)
	printf("%d ", (INT_BIT * i) + j);
    }
  }
  printf(") ");
}  /* print_set */

/*************
 *
 *   print_set_b_to_a()
 *
 *************/

void print_set_b_to_a(int *s)
{
  int i, j;
  printf("(");
  for (i = 0; i < Set_size; i++) {
    int k = s[i];
    for (j = 0; j < INT_BIT; j++) {
      if ((k>>j) & 1)
	printf("%d ", B_to_A[(INT_BIT * i) + j]);
    }
  }
  printf(") ");
}  /* print_set_b_to_a */

/*************
 *
 *   print_jhash()
 *
 *************/

static void print_jhash(void)
{
  int i;
  printf("\n========== Jhash ==========\n");
  for (i = 0; i < JHASH_SIZE; i++) {
    if (Jhash[i] != NULL) {
      struct glist *p = Jhash[i];
      printf("%d: ", i);
      for (; p != NULL; p = p->next) {
	print_set(p->v);
      }
      printf("\n");
    }
  }
}  /* print_jhash */


/*

======================================================================
JNODE STRUCTURE
======================================================================

*/

struct jnode {
  int id;                /* clause id */
  int justification;     /* which justification (count from 1) */
  int *goals;            /* current set of goals */
  int *resolved;         /* atoms resolved so far */
  struct jnode *parent;
  struct jnode *next;
};

static struct jnode *Jstart;
static struct jnode *Jend;
static struct jnode *Jcurrent;

static int Expanded;
static int Generated;
static int Pruned;
static int Subsumed;
static int Kept;

/*************
 *
 *   get_jnode()
 *
 *************/

static struct jnode *get_jnode(void)
{
  struct jnode *p = (struct jnode *) tp_alloc((int) sizeof(struct jnode));
  p->id = -1;
  p->justification = -1;
  p->goals = NULL;
  p->parent = NULL;
  p->next = NULL;
  return p;
}  /* get_jnode */

/*************
 *
 *   b_input()
 *
 *************/

static int b_input(int i)
{
  return Blists[i] == NULL;
}  /* b_input */

/*************
 *
 *   a_input()
 *
 *************/

static int a_input(int i)
{
  return Blists[A_to_B[i]] == NULL;
}  /* a_input */

/*************
 *
 *   print_jnode()
 *
 *************/

static void print_jnode(struct jnode *j)
{
    printf("%d ", j->id);
    printf("[%d] ", j->justification);
    print_set(j->goals);
    printf(" resolved: ");
    print_set(j->resolved);
    printf("\n");
}  /* print_jnode */

/*************
 *
 *   print_jnode_b_to_a()
 *
 *************/

static void print_jnode_b_to_a(struct jnode *j)
{
    printf("%d ", j->id >= 0 ? B_to_A[j->id] : j->id);
    printf("[%d] ", j->justification);
    print_set_b_to_a(j->goals);
    printf(" resolved: ");
    print_set_b_to_a(j->resolved);
    printf("\n");
}  /* print_jnode_b_to_a */

/*************
 *
 *   subset_or_input()
 *
 *************/

int subset_or_input(struct ilist *a, struct ilist *b)
{
  if (a == NULL)
    return 1;
  else if (ilist_member(a->i, b))
    return subset_or_input(a->next, b);
  else if (a_input(a->i))
    return subset_or_input(a->next, b);
  else if (ilist_member(map_demod(Demod_map, a->i), b))
    return subset_or_input(a->next, b);
  else
    return 0;
}  /* subset_or_input */

/*************
 *
 *   jproof()
 *
 *************/

static void jproof(struct jnode *j)
{
  struct ilist *ids = NULL;
  struct glist *derived = NULL;
  struct glist *justs = NULL;
  struct ilist *input = NULL;
  struct glist *p1, *p2;
  struct ilist *ip1, *ip2;
  struct clause *c;
  int n;

  struct jnode *p;
  printf("\n=== JPROOF ===\n  Expanded %d, Generated %d, Pruned %d, Subsumed %d, Kept %d.\n",
	 Expanded, Generated, Pruned, Subsumed, Kept);
#if 0
  printf("  Lookups %u, Crashes %u, Compares %u.\n",
	 Lookups, Crashes, Compares);
  for (p = j; p != NULL; p = p->parent)
    print_jnode(p);
  printf("==== unmapped =====\n");
#endif
  for (p = j; p != NULL; p = p->parent)
    print_jnode_b_to_a(p);

  /* For each clause, look through justifications and use the
     first one all of whose members are previously seen or input.
  */

  ids = NULL;  /* List of IDs seen so far. */

  for (p = j; p->parent != NULL; p = p->parent) {  /* for each jnode */
    struct g2list *g;
    int real_id = B_to_A[p->id];
    c = cl_find(real_id);
    if (c == NULL)
      abend("jproof: derived clause not found");
      
    for (g = c->multi_parents; g != NULL; g = g->next) {  /* for each just */
      ip1 = g->v2;
      if (subset_or_input(ip1, ids)) {
	/* We found it.  Collect any input clauses in the justification. */
	for (ip2 = ip1; ip2 != NULL; ip2 = ip2->next) {
	  if (a_input(ip2->i))
	    input = iset_add(ip2->i, input);
	}
	break;
      }
    }
    if (g == NULL) {
      print_clause(stdout, c);
      abend("jproof: justification not found");
    }
    derived = glist_tack_on(derived, c);
    justs = glist_tack_on(justs, g->v1);
    ids = iset_add(real_id, ids);
  }

  /* c now contains the last clause (empty clause) in the original proof. */

  n = proof_length(c);

  /* When printing the proof lengths, don't count the empty clause. */

  printf("\n==== SHORT_PROOF ===== Length %d (starting length %d)\n",
	 glist_length(derived)-1, n-1);

  fprintf(stderr, "\n==== SHORT_PROOF ===== Length %d (starting length %d)\n",
	  glist_length(derived)-1, n-1);

  for (ip1 = input; ip1 != NULL; ip1 = ip1->next) {
    struct clause *c = cl_find(ip1->i);
    if (c == NULL)
      abend("jproof: input clause not found");
    printf("[] ");
    print_clause_without_just(stdout, c);
  }

  for (p1 = derived, p2 = justs; p1 && p2; p1 = p1->next, p2 = p2->next) {
    print_justification(stdout, p2->v);
    printf(" ");
    print_clause_without_just(stdout, p1->v);
  }
  free_glist_list(derived);
  free_glist_list(justs);

  {
    struct ilist *id1 = get_ancestors2(c);
    struct ilist *id2 = iset_sort(ilist_append(input, ids));

    struct ilist *id2_id1, *id1_id2;

    printf("IDs in original proof: "); p_ilist(id1);
    printf("IDs in short proof:    "); p_ilist(id2);

    id2_id1 = iset_subtract(id2, id1);
    id1_id2 = iset_subtract(id1, id2);

    printf("original - short: "); p_ilist(id1_id2);
    printf("short - original: "); p_ilist(id2_id1);

    free_ilist_list(id1);
    free_ilist_list(id2);
    free_ilist_list(id1_id2);
    free_ilist_list(id2_id1);
  }

  printf("==== end jproof =====\n");
}  /* jproof */

/*************
 *
 *   j_subsumed_recurse()
 *
 *************/

static int j_subsumed_recurse(int *s, struct ilist *p)
{
  if (p == NULL) {
#if DEBUG    
    printf("         Checking hash membership: "); print_set(s); printf("\n");
#endif
    return jhash_member(s);
  }
#if 0  /* try empty set first */
  else if (j_subsumed_recurse(s, p->next))
    return 1;
  else {
    int subsumed;
    add_to_jset(s, p->i);
    subsumed = j_subsumed_recurse(s, p->next);
    remove_from_jset(s, p->i);
    return subsumed;
  }
#else  /* try full set first */
  else {
    int subsumed;
    add_to_jset(s, p->i);
    subsumed = j_subsumed_recurse(s, p->next);
    remove_from_jset(s, p->i);
    if (subsumed)
      return 1;
    else if (j_subsumed_recurse(s, p->next))
      return 1;
    else
      return 0;
  }
#endif
}  /* j_subsumed_recurse */

/*************
 *
 *   j_subsumed()
 *
 *************/

static int j_subsumed(struct ilist *p, int *s)
{
  /* p has the list (set) of goals.
     s has been allocated, but it is empty.
  */
  int subsumed;

#if DEBUG    
  printf("Subsume check: "); p_ilist(p);
#endif
  subsumed = j_subsumed_recurse(s, p);

  if (subsumed)
    return 1;
  else {
    struct ilist *q;
    for (q = p ; q != NULL; q = q->next)
      add_to_jset(s, q->i);
    jhash_insert(s);
#if DEBUG
    printf("                                   KEPT: "); print_set(s);
#endif
    Kept++;
    return 0;
  }
}  /* j_subsumed */

/*************
 *
 *   jset_to_ilist()
 *
 *************/

struct ilist *jset_to_ilist(int *s)
{
  int i, j;
  struct ilist *p = NULL;
  for (i = 0; i < Set_size; i++) {
    int k = s[i];
    for (j = 0; j < INT_BIT; j++) {
      if ((k>>j) & 1)
	p = iset_add((INT_BIT * i) + j, p);
    }
  }
  return p;
}  /* jset_to_ilist */

/*************
 *
 *   ilist_to_jset()
 *
 *************/

int *ilist_to_jset(struct ilist *p)
{
  int *s = get_jset();
  for ( ; p != NULL; p = p->next)
    add_to_jset(s, p->i);
  return s;
}  /* ilist_to_jset */

/*************
 *
 *   iset_jset_disjoint()
 *
 *************/

static int iset_jset_disjoint(struct ilist *i, int *j)
{
  if (i == NULL)
    return 1;
  else if (jset_member(j, i->i))
    return 0;
  else
    return iset_jset_disjoint(i->next, j);
}  /* iset_jset_disjoint */

/*************
 *
 *   j_expand()
 *
 *************/

static int j_expand(struct jnode *n)
{
  int proof = 0;
  struct ilist *g, *igoals;
#if DEBUG    
  printf("Expanding: "); print_jnode(n);
#endif
  Expanded++;
  igoals = jset_to_ilist(n->goals);
  for (g = igoals; g != NULL; g = g->next) {  /* for each goal */
    int goal = g->i;
    struct glist *j;
    int jcount = 0;
    for (j = Blists[goal]; j != NULL; j = j->next) {  /* for each just */
      Generated++;
      if (0 && !iset_jset_disjoint(j->v, n->resolved))
	/* I believe that all nodes pruned here would be subsumed below.
	   But this test is much faster than the subsumption test.
	*/
	Pruned++;
      else {
	struct ilist *x;
	struct ilist *new_igoals = copy_ilist(igoals);
	int *new_jgoals = NULL;
	jcount++;
	new_igoals = iset_remove(goal, new_igoals);
	for (x = j->v; x != NULL; x = x->next) {

	  if (!b_input(x->i))
	    new_igoals = iset_add(x->i, new_igoals);
	}

	/* Check later if new_igoals is empty. */

	new_jgoals = get_jset();

	if (j_subsumed(new_igoals, new_jgoals)) {
	  Subsumed++;
	  free(new_jgoals);
	}
	else {
	  struct jnode *n2 = get_jnode();
	  n2->id = goal;
	  n2->justification = jcount;
	  n2->goals = new_jgoals;  /* set up by j_subsume; shared with hash */
	  n2->resolved = copy_jset(n->resolved);
	  add_to_jset(n2->resolved, goal);

	  n2->parent = n;
	  Jend->next = n2;
	  Jend = n2;
	
	  if (new_igoals == NULL) {
	    jproof(n2);
	    proof = 1;
	    return proof;
	  }
	}
	free_ilist_list(new_igoals);
      }
    }
  }
  free_ilist_list(igoals);
  return proof;
}  /* j_expand */

/*************
 *
 *   j_search()
 *
 *************/

static int j_search(int b_goal)
{
  int level = 0;
  int end_of_level = 0;
  int proof = 0;
  Kept = 1;

  jhash_init();
  set_jset_size(Bsize);
  Jstart = get_jnode();
  Jstart->goals = get_jset();
  Jstart->resolved = get_jset();
  add_to_jset(Jstart->goals, b_goal);
  Jend = Jstart;
  for (Jcurrent = Jstart; Jcurrent != NULL; Jcurrent = Jcurrent->next) {
    if (Expanded == end_of_level) {
      double seconds = run_time() / 1000.;
      level++;
      end_of_level = Kept;
      fprintf(stderr, "Level %d, last kept clause of level %d is %d.\n",
	     level, level-1, end_of_level);
      printf("\nLevel %d, last kept clause of level %d is %d, %.2f seconds.\n",
	     level, level-1, end_of_level, seconds);
      fflush(stdout);
    }
    if (Expanded % 10000 == 0) {
      double seconds = run_time() / 1000.;
      printf("\n    expanded %d, generated %d, pruned %d, subsumed %d, kept %d, \n",
	     Expanded, Generated, Pruned, Subsumed, Kept);
#if 0
      printf("    lookups %u, crashes %u, compares %u,\n",
	     Lookups, Crashes, Compares);
#endif
      printf("    user time %.2f, lookups/second %d\n",
	     seconds, (int) (Lookups / seconds));
      fflush(stdout);
    }
    if (j_expand(Jcurrent)) {
      proof = 1;
      return proof;
    }
  }
  return proof;
}  /* j_search */

/*************
 *
 *   multi_just_process()
 *
 *************/

void multi_just_process(struct clause *c)
{
  int i;

  printf("\nLooking through the multi justifications for a short proof.\n");
  fprintf(stderr, "\nLooking through the multi justifications for a short proof.\n\n");

  Asize = next_cl_num();

  Alists = calloc(Asize, sizeof(struct glist *));

  build_support_lists(c);

  collapse_new_demod2();

#if 1
  printf("============ MULTIJUST  ============\n");
  for (i = 0; i < Asize; i++) {
    if (Alists[i] != NULL) {
      print_multi_supporters(stdout, i, Alists[i]);
    }
  }
  printf("============ end multijust ============\n");
#endif

  multi_map();

#if 0
  printf("============ MULTI_RENUMBERED_JUST ============\n");
  for (i = 0; i < Bsize; i++) {
    if (Blists[i] != NULL) {
      print_multi_supporters(stdout, i, Blists[i]);
    }
  }
  printf("============ end multi_renumbered_just ============\n");
#endif

  i = j_search(A_to_B[c->id]);

}  /* multi_just_process */

/*************
 *
 *   multi_justifications()
 *
 *   Are we keeping multiple justifications?
 *
 *************/

int multi_justifications()
{
  return (Flags[MULTI_JUST].val ||
	  Flags[MULTI_JUST_LESS].val ||
	  Flags[MULTI_JUST_SHORTER].val ||
	  Parms[MULTI_JUST_MAX].val != -1);
}  /* multijust */
