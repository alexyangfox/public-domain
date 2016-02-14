/*
 *  case.c - case splitting
 *
 */

#include "header.h"

#ifdef TP_FORK  /* for calls to fork() and wait() */
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

#define MAX_SPLIT_DEPTH  255  /* see SPLIT_DEPTH in options.c */

#define POS_CLAUSE 1
#define NEG_CLAUSE 2
#define MIX_CLAUSE 3

#define FORK_FAIL  0
#define PARENT     1
#define CHILD      2
#define CHILD_FAIL 3

/* Current_case is a sequence of integers, e.g., Case [2.1.3.2]. */

struct ilist *Current_case = NULL;

/* The literal_data structure is used for atom splitting.  When 
 * deciding which atom to split, all ground literal occurrences
 * are considered, and for each, the data in the structure is
 * collected.  See routines get_literal_data(), compare_literal_data(),
 * find_atom_to_split().
 */

struct literal_data {
  struct term *atom;
  int sign;
  int equality;
  int atom_weight;
  int clause_id;
  int clause_weight;
  int clause_type;
  int clause_variables;
  int pos_occurrences;
  int neg_occurrences;
  int pos_binary_occurrences;
  int neg_binary_occurrences;
};

/* These are file descriptors (from pipe()), which are used to
 * communicate with child and parent processes.  The main use is
 * for a child to tell its parent what case assumptions were
 * used for a refutation, which allows ancestors to sometimes
 * skip further cases.  See assumps_to_parent(), prover_forks().
 */

int To_parent,   From_parent;    /* pipe for communicating with parent */
int To_children, From_children;  /* pipe for communicating with children */

/*************
 *
 *   splitting() -- Is slitting enabled?
 *
 *************/

int splitting(void)
{
  return(Flags[SPLIT_CLAUSE].val ||
	 Flags[SPLIT_ATOM].val ||
	 Flags[SPLIT_WHEN_GIVEN].val);
}  /* splitting */

/*************
 *
 *   max_split_depth() -- Return the maximum depth allowed for splitting.
 *
 *************/

int max_split_depth(void)
{
  return(MAX_SPLIT_DEPTH);
}  /* max_split_depth */

/*************
 *
 *   splitable_literal(clause, lit) -- Is the atom splittable?
 *
 *   The test is done on an occurrence of a literal.
 *
 *************/

int splitable_literal(struct clause *c,
		      struct literal *l)
{
  if (num_literals(c) < 2 || !ground(l->atom))
    return 0;
  else {
    int ok = 1;
    if (ok && Flags[SPLIT_POS].val)
      ok = pos_clause(c);
    if (ok && Flags[SPLIT_NEG].val)
      ok = neg_clause(c);
    if (ok && Flags[SPLIT_NONHORN].val)
      ok = l->sign && !horn_clause(c);
    return ok;
  }
}  /* splitable_literal */

/*************
 *
 *   compare_literal_data(d1, d2) -- Compare two splittable literal occurrences.
 *
 *   Return the better literal_data.  If neither is better, return d1.
 *
 *************/

static struct literal_data compare_literal_data(struct literal_data d1,
					 struct literal_data d2)
{
  if (d1.atom == NULL)
    return d2;
  else if (d2.atom == NULL)
    return d1;
  else if (Flags[SPLIT_POPULAR].val) {
    if (d2.pos_occurrences + d2.neg_occurrences >
	d1.pos_occurrences + d1.neg_occurrences)
      return d2;
    else
      return d1;
  }
  else {
    if (d2.clause_weight < d1.clause_weight)
      return d2;
    else if (d1.clause_weight < d2.clause_weight)
      return d1;
    else if (d2.atom_weight < d1.atom_weight)
      return d2;
    else
      return d1;
  }
}  /* compare_literal_data */

/*************
 *
 *   splitable_clause(c) -- Is this clause splittable?
 *
 *************/

int splitable_clause(struct clause *c)
{
  if (!ground_clause(c))
    return 0;
  else if (num_literals(c) < 2)
    return 0;
  else {
    int ok = 1;
    if (ok && Flags[SPLIT_POS].val)
      ok = pos_clause(c);
    if (ok && Flags[SPLIT_NEG].val)
      ok = neg_clause(c);
    if (ok && Flags[SPLIT_NONHORN].val)
      ok = !horn_clause(c);
    return ok;
  }
}  /* splitable_clause */

/*************
 *
 *   compare_splitable_clauses(c, d) -- Compare two splittable clauses.
 *
 *   Return the better clause.  If neither is better, return c.
 *
 *************/

struct clause *compare_splitable_clauses(struct clause *c,
					 struct clause *d)
{
  if (c == NULL)
    return d;
  else if (d == NULL)
    return c;
  else if (Flags[SPLIT_MIN_MAX].val) {
    
    /* Return the clause with the smaller maximum literal.
     * If maxes are the same, return the smaller clause
     * if clauses are the same weight, return c.
     */

    int cm = max_literal_weight(c, Weight_pick_given_index);
    int dm = max_literal_weight(c, Weight_pick_given_index);
    /* printf("maxes: %4d=%4d, %4d=%4d\n", c->id, cm, d->id, dm); */
    if (cm < dm) {
      return c;
    }
    else if (dm < cm) {
      return d;
    }
    else 
      return (d->pick_weight < c->pick_weight ? d : c);
  }
  else {
    /* Return smaller clause; if clauses are the same weight, return c. */
    return (d->pick_weight < c->pick_weight ? d : c);
  }
}  /* compare_splitable_clauses */

/*************
 *
 *   init_literal_data(p) -- Initialize a literal_data structure.
 *
 *************/

static void init_literal_data(struct literal_data *p)
{
  p->atom = NULL;
}  /* init_literal_data */

/*************
 *
 *   p_literal_data(data) -- Print a literal_data structure to stdout.
 *
 *************/

static void p_literal_data(struct literal_data data)
{
  printf("Atom: "); print_term(stdout, data.atom);
  printf(" sign=%d, equality=%d, atom_wt=%d, cl_id=%d, cl_wt=%d, cl_type=%d, variables=%d, pos=%d, neg=%d, pos_binary=%d, neg_binary=%d\n",
	 data.sign,data.equality,data.atom_weight,data.clause_id,
	 data.clause_weight,
	 data.clause_type,data.clause_variables,
	 data.pos_occurrences,data.neg_occurrences,
	 data.pos_binary_occurrences,data.neg_binary_occurrences);
}  /* p_literal_data */

/*************
 *
 *   get_literal_data(lit, p)
 *
 *   Given a ground literal occurrence, fill in the data.
 *
 *************/

static void get_literal_data(struct literal *lit,
		      struct literal_data *p)
{
  struct clause *c = lit->container;
  struct term *a;
  struct fpa_tree *ut;
  int n, m;

  p->atom = lit->atom;
  p->sign = lit->sign;
  p->equality = is_eq(lit->atom->sym_num);
  p->atom_weight = weight(lit->atom, Weight_pick_given_index);

  p->clause_id = c->id;
  p->clause_weight = weight_cl(c, Weight_pick_given_index);
  p->clause_variables = distinct_vars(c);
  if (pos_clause(c))
    p->clause_type = POS_CLAUSE;
  else if (neg_clause(c))
    p->clause_type = NEG_CLAUSE;
  else
    p->clause_type = MIX_CLAUSE;

  if (!Flags[SPLIT_POPULAR].val) {
    p->pos_occurrences = 0;
    p->neg_occurrences = 0;
    p->pos_binary_occurrences = 0;
    p->neg_binary_occurrences = 0;
  }
  else {
    ut = build_tree(lit->atom, INSTANCE, Parms[FPA_LITERALS].val, Fpa_pos_lits);
    n = 0; m = 0;
    a = next_term(ut, 0);
    while (a != NULL) {
      n++;
      if (num_literals(a->occ.lit->container) == 2)
	m++;
      a = next_term(ut, 0);
    }
    p->pos_occurrences = n;
    p->pos_binary_occurrences = m;

    ut = build_tree(lit->atom, INSTANCE, Parms[FPA_LITERALS].val, Fpa_neg_lits);
    n = 0; m = 0;
    a = next_term(ut, 0);
    while (a != NULL) {
      n++;
      if (num_literals(a->occ.lit->container) == 2)
	m++;
      a = next_term(ut, 0);
    }
    p->neg_occurrences = n;
    p->neg_binary_occurrences = m;
  }
}  /* get_literal_data */

/*************
 *
 *   print_case() -- print the current case, e.g., [2.1.3], to a file
 *
 *************/

void print_case(FILE *fp)
{
  struct ilist *ip;
  fprintf(fp, "[");
  for (ip = Current_case; ip; ip = ip->next)
    fprintf(fp, "%d%s", ip->i, ip->next == NULL ? "" : ".");
  fprintf(fp, "]");
}  /* print_case */

/*************
 *
 *   p_case()
 *
 *************/

void p_case(void)
{
  print_case(stdout);
}  /* p_case */

/*************
 *
 *   print_case_n() -- Like print_case, but add the argument.
 *
 *************/

void print_case_n(FILE *fp,
		  int n)
{
  struct ilist *ip;
  fprintf(fp, "[");
  for (ip = Current_case; ip; ip = ip->next)
    fprintf(fp, "%d.", ip->i);
  fprintf(fp, "%d]", n);
}  /* print_case_n */

/*************
 *
 *   p_case_n()
 *
 *************/

void p_case_n(int n)
{
  print_case_n(stdout, n);
}  /* p_case_n */

/*************
 *
 *   p_assumption_depths()
 *
 *************/

void p_assumption_depths(char assumptions[])
{
#if 0
  int i;
  printf("Assumptions at the following depths were used to refute this branch:");
  for (i = 0; i <= MAX_SPLIT_DEPTH; i++) {
    if (assumptions[i])
      printf("  %d", i);
  }
  printf(".\n");
#endif
}  /* p_assumption_depths */
                
/*************
 *
 *   current_case() -- Return Current_case.
 *
 *************/

struct ilist *current_case(void)
{
  return(Current_case);
}  /* current_case */

/*************
 *
 *   add_subcase(i) -- Append an integer to Current_case.
 *
 *************/

void add_subcase(int i)
{
  struct ilist *p1, *p2;

  p1 = get_ilist();
  p1->i = i;
  if (Current_case == NULL)
    Current_case = p1;
  else {
    for (p2 = Current_case; p2->next != NULL; p2 = p2->next);
    p2->next = p1;
  }
}  /* add_subcase */

/*************
 *
 *   case_depth() -- What is the depth of the current case?
 *
 *************/

int case_depth(void)
{
  return ilist_length(Current_case);
}  /* case_depth */

/*************
 *
 *   find_clause_to_split()
 *
 *   Go through Usable, then Sos, and find the best splittable clause.
 *
 *************/

struct clause *find_clause_to_split(void)
{
  struct clause *c;
  struct clause *best_so_far = NULL;

  for (c = Usable->first_cl; c != NULL; c = c->next_cl) {
    if (splitable_clause(c)) {
      best_so_far = compare_splitable_clauses(best_so_far, c);
    }
  }

  for (c = Sos->first_cl; c != NULL; c = c->next_cl) {
    if (splitable_clause(c)) {
      best_so_far = compare_splitable_clauses(best_so_far, c);
    }
  }

  return(best_so_far);  /* may be NULL */

}  /* find_clause_to_split */

/*************
 *
 *   find_atom_to_split()
 *
 *   Go through all literal occurrences in Usable+Sos, and return
 *   the atom of the best splittable literal occurrence.
 *
 *************/

struct term *find_atom_to_split(void)
{
  if (Split_atoms != NULL) {
    int i;
    struct term *t;

    /* Split_atoms is a proper list.  If the case_depth is n,
     * return the n-th member of split_atoms.
     */
    for (t = Split_atoms, i = 0;
	 t->sym_num != Nil_sym_num && i < case_depth();
	 t = t->farg->narg->argval, i++);
    return (t->sym_num == Nil_sym_num ? NULL : t->farg->argval);
  }
  else {
    struct clause *c;
    struct literal *lit;
    struct literal_data min, curr;

    init_literal_data(&min);

    for (c = Usable->first_cl; c != NULL; c = c->next_cl) {
      for (lit = c->first_lit; lit != NULL; lit = lit->next_lit) {
	if (splitable_literal(c, lit)) {
	  get_literal_data(lit, &curr);
	  min = compare_literal_data(min, curr);
	}
      }
    }

    for (c = Sos->first_cl; c != NULL; c = c->next_cl) {
      for (lit = c->first_lit; lit != NULL; lit = lit->next_lit) {
	if (splitable_literal(c, lit)) {
	  get_literal_data(lit, &curr);
	  min = compare_literal_data(min, curr);
	}
      }
    }
    return min.atom;  /* NULL if no ground lits found */
  }
}  /* find_atom_to_split */

/*************
 *
 *   prover_forks(int n, int *ip, char assumptions[])
 *
 *   This is the guts of the splitting.  It is used for both clause
 *   splitting and atom splitting.  Parameter n tells how many cases
 *   to do.  This routine also takes care of skipping redundant cases
 *   when assumptions are not used.  For example, if we split on
 *   clause p|q, and the p case is refuted without using p, then we
 *   skip the q case.
 *
 *   This routine does not return when a child returns without
 *   a proof.  When this happens, we just exit,
 *   sending the same exit code to the parent.
 *
 *   When this routine does return to its caller, the return value is:
 *
 *     CHILD       Return as child process ready to do its case.
 *                 Also, integer *ip is set to the case number.
 *     PARENT      Return as parent process---all children succeeded.
 *                 Also, fill in the Boolean array assumptions, which
 *                 tells which ancestor case assumptions were used to
 *                 refute the child cases.
 *     FORK_FAIL   Operating system would not allow process to fork.
 *     CHILD_FAIL  A child did not exit normally (WIFEXITED(status) nonzero).
 *
 *************/

int prover_forks(int n,
		 int *ip,
		 char assumptions[])
{
#ifndef TP_FORK
  return FORK_FAIL;
#else
  int child_status, rc;
  int parent = 1;
  int i = 1;
  int fd[2];
  char assumptions_descendents[MAX_SPLIT_DEPTH+1];
  int j;

  for (j = 0; j <= MAX_SPLIT_DEPTH; j++)
    assumptions[j] = 0;

  /* Set up pipe for communicating with children.  The child processes
   * will inherit these values and immediately use them to set up a
   * pipe to the parent (that is, copy them to To_parent and From_parent).
   */

  rc = pipe(fd); From_children = fd[0]; To_children = fd[1];
  if (rc != 0) {
    return FORK_FAIL;
  }
  
  while (i <= n && parent) {
    fflush(stdout); fflush(stderr);
    rc = fork();
    if (rc < 0) {
      return FORK_FAIL;
    }
    else if (rc > 0) {
      /* This is the parent process */
      int depth = case_depth();

      wait(&child_status);
      if (WIFEXITED(child_status)) {
	int child_exit_code = WEXITSTATUS(child_status);

	if (child_exit_code==PROOF_EXIT) {
	  /* all is well---the child proved its case */
	  printf("Refuted case ");
	  p_case_n(i);
	  printf(".\n");
	  fflush(stdout);

	  if (Flags[REALLY_DELETE_CLAUSES].val) {
	    /* Really_delete_clauses is incompatable with the assumption
	       redundancy check.  We'll just go on to the next case
	       in stead of checking if the assumption for the previous
	       case was used for the refutation.
	    */
	    i++;
	  }
	  else {
	    rc = read(From_children, assumptions_descendents,MAX_SPLIT_DEPTH+1);
	    if (assumptions_descendents[depth+1])
	      i++;
	    else if (i == n)
	      i++;  /* assumption for last case was not used */

	    else {
	  
	      printf("\nThe Assumption for case ");
	      p_case_n(i);
	      printf(" was not used;\n");
	      printf("therefore we skip case%s", (i == n-1 ? ": " : "s:"));
	      for (j = i+1; j <= n; j++) {
		printf(" ");
		p_case_n(j);
	      }
	      printf(".\n");

	      i = n+1;
	    }

	    /* "or" in the assumptions used. */
	    for (j = 0; j <= depth; j++)
	      assumptions[j] = (assumptions[j] | assumptions_descendents[j]);
	  }
	}  /* child found proof */
	else {
	  /* Child exited without a proof.  Exit with same code to parent. */
	  output_stats(stdout, Parms[STATS_LEVEL].val);
	  printf("\nProcess %d finished %s", my_process_id(), get_time());
	  exit(child_exit_code);
	}
      }  /* WIFEXITED */
      else {
	/* Child fails for some other reason. */
	return CHILD_FAIL;
      }
    }  /* if parent */

    else {
      /* This is the child process. */
      /* Set up pipe to parent. */
      To_parent = To_children; From_parent = From_children;
      /* Exit loop and do the case. */
      parent = 0;
    }
  } /* while */
  
  *ip = i;
  return (parent ? PARENT : CHILD);
#endif  
}  /* prover_forks */

/*************
 *
 *   split_clause(c)
 *
 *   If (c == NULL), look for a clause to split.
 *   If (c != NULL), split on c.
 *
 *   If success (i.e., split, and each child refutes its case), exit process.
 *
 *   Return value:
 *        0: no split
 *        1: split, child returns
 *        2: split, parent returns failure (this might not be used)
 *
 *************/

int split_clause(struct clause *giv_cl)
{
#ifndef TP_FORK
  return 0;
#else
  struct clause *c;
  char assumptions[MAX_SPLIT_DEPTH+1];

  if (giv_cl == NULL)
    c = find_clause_to_split();
  else
    c = giv_cl;

  if (c == NULL) {
    printf("\nI tried to split, but I could not find a suitable clause.\n");
    return 0;
  }
  else {
    int rc, n, case_number;

    printf("\nSplitting on clause "); p_clause(c);
    n = num_literals(c);
    rc = prover_forks(n, &case_number, assumptions);

    if (rc == FORK_FAIL) {
      printf("Case splitting (fork) failed.  Returning to search.\n");
      return 0;
    }
    else if (rc == PARENT) {
      if (Current_case == NULL) {
	printf("\nThat finishes the proof of the theorem.\n");
	fprintf(stderr, "%c\nThat finishes the proof of the theorem.\n", Bell);
      }
      else {
	/* Tell the parent the assumptions used to refute this
	 * branch.  We don't send the actual assumptions; instead,
	 * we send a set of integers giving the depths of the
	 * assumptions.  This is implemented as a Boolean array
	 * indexed by depth.
	 */

	rc = write(To_parent, assumptions, MAX_SPLIT_DEPTH+1);
	p_assumption_depths(assumptions);
      }
      output_stats(stdout, Parms[STATS_LEVEL].val);
      printf("\nProcess %d finished %s", my_process_id(), get_time());
      exit(PROOF_EXIT);
    }
    else if (rc == CHILD) {
      /* We are the child. Assert units for this case, then continue search. */
      int j;
      struct literal *c_lit, *d_lit;
      struct clause *d, *sos_pos;

      clock_init();    /* reset all clocks to 0 */
      add_subcase(case_number);  /* Update the case vector. */
      printf("\nCase "); p_case();
      printf("   (process %d):\n", my_process_id()); 

      /* Disable the clause being split. */

      un_index_lits_all(c);
      if (c->container == Usable)
	un_index_lits_clash(c);
      rem_from_list(c);
      hide_clause(c);

      /* Add negated units for cases already done. */
      /* Then add the unit for this case. */

      sos_pos = Sos->last_cl;  /* save position for post processing */

      for (j = 1; j <= case_number; j++) {
	c_lit = ith_literal(c, j);
	d = get_clause();
	d_lit = get_literal();
	d->first_lit = d_lit;
	d_lit->container = d;
	d_lit->atom = copy_term(c_lit->atom);
	d_lit->atom->occ.lit = d_lit;
	d_lit->sign = c_lit->sign;
	d_lit->atom->varnum = c_lit->atom->varnum;  /* copy type of atom */
	if (j != case_number) {
	  /* negate literal */
	  d_lit->sign = !d_lit->sign;
	  if (d_lit->atom->varnum == POS_EQ)
	    d_lit->atom->varnum = NEG_EQ;
	  else if (d_lit->atom->varnum == NEG_EQ)
	    d_lit->atom->varnum = POS_EQ;
	}

	d->parents = get_ilist();
	d->parents->i = c->id;

	d->parents->next = get_ilist();
	d->parents->next->i = (j == case_number ? SPLIT_RULE : SPLIT_NEG_RULE);
	d->parents->next->next = get_ilist();
	d->parents->next->next->i = LIST_RULE - ilist_length(Current_case);;
	d->parents->next->next->next = copy_ilist(Current_case);

	pre_process(d, 0, Sos);
	if (j == case_number && d->container == Sos) {
	  printf("Assumption: ");
	  p_clause(d);
	}
      }
      post_proc_all(sos_pos, 0, Sos);
      return 1;
    }
    else {  /* rc == CHILD_FAIL */
      abend("case failure");
      return -1;
    }
  }
#endif
}  /* split_clause */

/*************
 *
 *   split_atom()
 *
 *   If success (i.e., split, and each child refutes its case), exit process.
 *
 *   Return value:
 *        0: no split
 *        1: split, child returns
 *        2: split, parent returns failure (this might not be used)
 *
 *************/

int split_atom(void)
{
#ifndef TP_FORK
  return 0;
#else
  struct term *atom;
  char assumptions[MAX_SPLIT_DEPTH+1];

  atom = find_atom_to_split();

  if (atom == NULL) {
    printf("\nI tried to split, but I could not find a suitable atom.\n");
    return 0;
  }
  else {
    int rc, case_number;

    printf("\nSplitting on atom "); p_term(atom);

    rc = prover_forks(2, &case_number, assumptions);

    if (rc == FORK_FAIL) {
      printf("Case splitting (fork) failed.  Returning to search.\n");
      return 0;
    }
    else if (rc == PARENT) {
      if (Current_case == NULL) {
	printf("\nThat finishes the proof of the theorem.\n");
	fprintf(stderr, "%c\nThat finishes the proof of the theorem.\n", Bell);
      }
      else {
	rc = write(To_parent, assumptions, MAX_SPLIT_DEPTH+1);
	p_assumption_depths(assumptions);
      }
      output_stats(stdout, Parms[STATS_LEVEL].val);
      printf("\nProcess %d finished %s", my_process_id(), get_time());
      exit(PROOF_EXIT);
    }  /* parent */
    else if (rc == CHILD) {
      /* We are the child. Assert units for this case, then continue search. */
      struct literal *d_lit;
      struct clause *d, *sos_pos;

      clock_init();    /* reset all clocks to 0 */
      add_subcase(case_number);  /* Update the case vector. */
      printf("\nCase "); p_case();
      printf("   (process %d):\n", my_process_id()); 

      sos_pos = Sos->last_cl;  /* save position for post processing */

      d = get_clause();
      d_lit = get_literal();
      d->first_lit = d_lit;
      d_lit->container = d;
      d_lit->atom = copy_term(atom);
      d_lit->atom->occ.lit = d_lit;
      d_lit->sign = (case_number == 1 ? 1 : 0);
      if (is_eq(atom->sym_num))
	d_lit->atom->varnum = d_lit->sign ? POS_EQ : NEG_EQ;
      else
	d_lit->atom->varnum = NORM_ATOM;

      d->parents = get_ilist();
      d->parents->i = SPLIT_RULE;
      d->parents->next = get_ilist();
      d->parents->next->i = LIST_RULE - ilist_length(Current_case);
      d->parents->next->next = copy_ilist(Current_case);

      pre_process(d, 0, Sos);
      if (d->container == Sos) {
	printf("Assumption: ");
	p_clause(d);
      }
      post_proc_all(sos_pos, 0, Sos);
      return 1;
    }  /* child */
    else {  /* rc == CHILD_FAIL */
      abend("case failure");
      return -1;
    }
  }
#endif
}  /* split_atom */

/*************
 *
 *   possible_split()
 *
 *   Check if it is time to split, and if so, try to split.
 *
 *   If a split occurs, children return to continue searching.
 *   If all children find proofs, parent calls exit(PROOF_EXIT).
 *   If any child fails, parent abends.  (This may change.)
 *
 *************/

void possible_split(void)
{
  static int next_attempt = 0;
  int ok = 0;

#ifndef TP_FORK
  abend("case splitting is not compiled into this Otter");
#endif

  if (Flags[SPLIT_CLAUSE].val || Flags[SPLIT_ATOM].val) {
    if (Parms[SPLIT_SECONDS].val != -1) {
      int runtime = run_time() / 1000;
      if (next_attempt == 0)
	next_attempt = Parms[SPLIT_SECONDS].val;
      if (runtime >= next_attempt) {
	ok = 1;
	next_attempt += Parms[SPLIT_SECONDS].val;
      }
    }
    else if (Parms[SPLIT_GIVEN].val != -1) {
      int n = Parms[SPLIT_GIVEN].val;
      if (n == 0 || Stats[CL_GIVEN] % n == 0) {
	ok = 1;
      }
    }

    if (ok) {
      int rc;
      if (case_depth() < Parms[SPLIT_DEPTH].val) {
	if (Flags[SPLIT_ATOM].val)
	  rc = split_atom();
	else
	  rc = split_clause((struct clause *) NULL);
      }	  
    }
  }
}  /* possible_split */

/*************
 *
 *   always_split()
 *
 *   Unconditional splitting, and keep splitting as long as possible.
 *
 *************/

void always_split(void)
{
  int rc;

#ifndef TP_FORK
  abend("case splitting is not compiled into this Otter");
#endif

  if (Flags[SPLIT_ATOM].val)
    rc = split_atom();
  else
    rc = split_clause((struct clause *) NULL);

  if (rc == 1)
    always_split();  /* We are the child; all is well; split again. */
  else {
    printf("\nalways_split: returning because no splitting is possible at this time.\n");
    return;
  }
}  /* always_split */

/*************
 *
 *   possible_given_split(c)
 *
 *   c has just been selected as the given clause.
 *
 *************/

void possible_given_split(struct clause *c)
{
#ifndef TP_FORK
  abend("case splitting is not compiled into this Otter");
#endif

  if (Flags[SPLIT_WHEN_GIVEN].val && ground_clause(c) && num_literals(c) > 1) {
    int ok = 1;

    if (ok && Flags[SPLIT_POS].val)
      ok = pos_clause(c);
    if (ok && Flags[SPLIT_NEG].val)
      ok = neg_clause(c);
    if (ok && Flags[SPLIT_NONHORN].val)
      ok = !horn_clause(c);

    if (ok) {
      int rc;
      if (case_depth() < Parms[SPLIT_DEPTH].val) {
	if (Flags[SPLIT_ATOM].val) {
	  /* This is a little strange.  We're allowing splitting on an
	     atom.  We'll first move the clause back to Sos.
	  */
	  un_index_lits_clash(c);
	  rem_from_list(c);
	  append_cl(Sos, c);
	  rc = split_atom();
	}
	else
	  rc = split_clause(c);
      }
    }
  }
}  /* possible_given_split */

/*************
 *
 *   assumps_to_parent()
 *
 *   This routine is called when a proof is found during case splitting.
 *
 *   Tell the parent the assumptions used to refute this  
 *   leaf.  We don't send the actual assumptions; instead,
 *   we send a set of integers giving the depths of the   
 *   assumptions.  This is implemented as a Boolean array 
 *   indexed by depth.
 *
 *************/

void assumps_to_parent(struct clause *e)
{
  struct clause_ptr *p, *q;
  struct ilist *r;
  int i;
  char assumptions[MAX_SPLIT_DEPTH+1];

  p = NULL;
  i = get_ancestors(e, &p, &r);  /* i (level), r (level list) won't be used */

  for (i = 0; i <= MAX_SPLIT_DEPTH; i++)
    assumptions[i] = 0;

  for (q = p; q != NULL; q = q->next) {
    r = q->c->parents;
    /* SPLIT_RULE code is either first (atom split) or second (clause split) */
    if (r != NULL && r->i == SPLIT_RULE) {
      i = LIST_RULE - r->next->i;
    }
    else if (r!= NULL && r->next != NULL && r->next->i == SPLIT_RULE) {
      i = LIST_RULE - r->next->next->i;
    }
    else
      i = 0;
    if (i != 0) {
      /* The current clause is a split assumption, and i is the
       * depth.  This does not include SPLIT_NEG assumptions from
       * previous sibling cases.
       */
      assumptions[i] = 1;
    }
  }
  i = write(To_parent, assumptions, MAX_SPLIT_DEPTH+1);
  printf("\n\n"); p_assumption_depths(assumptions);
}  /* assumps_to_parent */

/*************
 *
 *   exit_with_possible_model()
 *
 *************/

void exit_with_possible_model(void)
{
  printf("\nPossible model detected on branch ");
  p_case();  printf(".\n");
  fprintf(stderr, "\n%cPossible model detected on branch ", Bell);
  print_case(stderr);    fprintf(stderr, ".\n");

  printf("\nHere are the clauses in Usable and SoS.  It seems that no more\n");
  printf("inferences or splitting can be done.  If the search strategy is\n");
  printf("complete, these clauses should lead to a model of the input.\n");

  printf("\nlist(usable).\n");
  print_cl_list(stdout, Usable);
  printf("\nlist(sos).\n");
  print_cl_list(stdout, Sos);

  output_stats(stdout, Parms[STATS_LEVEL].val);
  
  printf("\nProcess %d finished %s", my_process_id(), get_time());
  exit(POSSIBLE_MODEL_EXIT);
}  /* exit_with_possible_model */

/*************************************************************************/
/* The rest of this file is some documentation on the code in this file. */
/*************************************************************************/

/*

CASE SPLITTING IN OTTER

William McCune, Dale Myers, Rusty Lusk, Mohammed Alumlla
December 1997

This implementation uses the UNIX fork() command to make copies of the
current state of the process for the cases.  This avoids having to
explicitly save the state and restore it for the next case.  (That is,
it was easy to implement.)

When enabled, splitting occurs periodically during the search.  The
default is to split after every 5 given clauses.  This can be changed
to some other number of given clauses, say 10, with the command

  assign(split_given, 10).    % default 5

Instead, one can split after a specified number of seconds, with
a command such as

  assign(split_seconds, 10).    % default infinity

which asks Otter to attempt a split every (approximately) 10 seconds.
(Jobs that use split_seconds are usually not repeatable.)

A third method is to split when a nonunit ground clause is selected
as the given clause.  This option is specified with the command

  set(split_when_given).

which causes splitting on the given clause if (1) it is ground, (2) it
is within the split_depth bound (see below), and (3) it satisfies the
split_pos and split_neg flags (see below).

If you wish to limit the depth of splitting, use a command such as

  assign(split_depth, 3).   % default 256 (which is also the maximum)

which will not allow a case such as "Case [1.1.1.1]".

There are two kinds of splitting: on ground clauses and on ground
atoms.  Clause splitting constructs one case for each literal, and
atom splitting, say on p, constructs two cases: p is true; p is false
(or, as Larry Wos says, splitting on a tautology).

Sequence of Splitting Processes

Say Otter decides to split the search into n cases.  For each case,
the fork() command creates a child process for the case, then the
parent process waits for the child to exit.  If any child fails to
refute its case, the parent exits in failure (causing all
ancestor processes to fail as well).  If each child refutes its case,
the parent exits with success.

The Output File

Various messages about the splitting events are sent to the output file.
To get an overview of the search from an output file, say problem.out,
one can use the following command.

  egrep "Splitting|Assumption|Refuted|skip|That" problem.out

Splitting Clauses

The following commands enables splitting on clauses after some number
of given clauses have been used (see split_given), or after some
number of seconds (see split_seconds).

  set(split_clause).     % default clear

The following command simply asks Otter to split on ground given clauses.

  set(split_when_given). % default clear

(I suppose one could use both of the preceding commands at the same
time---I haven't tried it.)

If Otter finds a suitable (see flags below) nonunit ground clause
for splitting, say "p | q | r", the assumptions for three cases
are

  Case 1: p.
  Case 2: -p & q.
  Case 3: -p & -q & r.

Eligible Clauses for Splitting

Otter splits on ground nonunit clauses only.  They can
occur in Usable or in Sos.  The following commands can be used
to specify the type of clause to split.

  set(split_pos).     % split on positive clauses only (default clear)
  set(split_neg).     % split on negative clauses only (default clear)
  set(split_nonhorn). % split on nonhorn clauses only (default clear)

If none of the preceding flags are set, all ground clauses are eligible.

Selecting the Best Eligible Clause

The default method for selecting the best eligible clause for
splitting is simply to take the first, lowest weight (using the
pick_given scale) clause from Usable+Sos.

Instead, one can use The command

  set(split_min_max).  % default clear

which says to use the following method to compare two eligible clauses
C and D.  Prefer the clause with the lighter heaviest literal
(pick_given scale);  if the heaviest literals have the same
weight, use the lighter clause;  if the clauses have the same
weight, use the first in Usable+Sos.

Splitting Atoms

The following commands enables splitting on atoms after some number
of given clauses have been used (see split_given), or after some
number of seconds (see split_seconds).

  set(split_atom).

(For propositional problems with assign(split_given, 0), this will
cause Otter to perform a (not very speedy) Davis-Putnam search.)
To select an atom for splitting, we consider OCCURRENCES of
atoms within clauses.

Eligible Atoms

Otter splits on atoms that occur in nonunit ground clauses.
The command 

  set(split_pos).   % default clear

says to split on atoms the occur only in positive clauses,

  set(split_neg).   % default clear

says to split on atoms the occur only in negative clauses, and

  set(split_nonhorn).   % default clear

says to split on atoms that occur positively in nonHorn clauses.

Selecting the Best Eligible Atom

Default method for comparing two eligible atom-occurrences:
Prefer the atom that occurs in the lower weight clause.
If the clauses have the same weight, prefer the atom
with the lower weight.

An optional method for selecting an atom considers the number
of occurrences of the atom.  The command

  set(split_popular).  % default clear

says to prefer the atom that occurs in the greatest number
of clauses.  All clauses in Usable+Sos containing the atom
are counted.

Another Way to Split Atoms

If the user has an idea of how atom splitting should occur, he/she
can give a sequence of atoms in the input file, and Otter will
split accordingly.  (As above, the TIME of splitting is determined
as above with the split_given and split_seconds parameters.)

For example, with the commands

  set(split_atom).
  split_atoms([P, a=b, R]).
  assign(split_given, 0).

Otter will immediately (because of split_given) split the
search into 8 cases (because there are 3 atoms), then do no
more splitting.

Problems With This Implementation

Splitting is permanent.  That is, if a case fails, the whole
search fails (no backing up to try a different split).

If Otter fails to find a proof for a particular case (e.g., the Sos
empties or some limit is reached), the whole attempt fails.  If
the search strategy is complete, then an empty Sos indicates
satisfiability, and the set of assumptions introduced by splitting
give you a partial model; however, it is up to the user to figure this
out.

When splitting is enabled, max_seconds (for the initial process and
all descendent processes) is checked against the wall clock (from the
start of the initial process) instead of against the process clock.
This is problematic if the computer is busy with other processes.

Getting the Total Process Time

The process clock ("user CPU" statistic) is initialized at the
start of the process, and each process prints statistics once at the
end of its life.  Therefore, one can get the total process by
summing all of the "user CPU" times in the output file.  The command

  grep "user CPU" problem.out | awk '{sum += $4}END{print sum}'

can be used to get the approximate total process time from an
output file problem.out.

Advice on Using Otter's Splitting (from McCune)

At this time, we don't have much data.  A general strategy
for nonground problems is the following.

  set(split_when_given).
  set(split_pos).            % Also try it without this command.
  assign(split_depth, 10).

For ground (propositional) problems, try the following, which 
is essentially a Davis-Putnam procedure.

  set(split_atom).
  set(split_pos).
  assign(split_given, 0).

*/
