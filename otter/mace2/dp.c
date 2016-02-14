#include "Mace2.h"

/* #define WORK1 */

/* This file has code for the basic Davis-Putnam propositional decision
 * procedure.
 */

/* #define TRACE */

/****** Macros ******/

#define SUBSUMED(c) (Subsume ? c->subsumer : subsumed(c))

/****** Shared Variables ******/

extern int First_order;     /* owned by mace.c */
extern int Models;          /* owned by mace.c */
extern int Domain_size;     /* owned by mace.c */

int Greatest_atom;          /* used by generate.c */

/****** Global Static Variables ******/

static int Subsume;            /* whether or not to mark subsumed clauses */
static int Check_time;         /* whether or not there is a time limit */

static int Num_clauses;
static Clause_ptr Clauses;
static Atom_ptr Atoms;         /* malloced when we know how many there are */
static int Num_selectable_clauses;
static Clause_ptr *Selectable_clauses;  /* for select_atom() */
 
static int *Unit_queue;          /* queue for unit propagation */
static int Unit_queue_size;      /* Actually, one less than this will fit. */
static int Unit_queue_first, Unit_queue_last;

static Gen_ptr_ptr Gen_ptr_avail;      /* list of available gen_ptr nodes */
static Clause_ptr Prev_dp;      /* previously DP-inserted clause */
static int Next_message = 1;    /* for printing models */

/* This set is for printing assignments (RECORD_ASSIGNMENTS) only */

static int Num_assignments;
static int *Assignments;        /* for RECORD_ASSIGNMENTS flag */
static int *Split_positions;

/*************
 *
 *    get_gen_ptr() -- allocate a gen_ptr node.
 *
 *************/

static Gen_ptr_ptr get_gen_ptr(void)
{
  Gen_ptr_ptr p;
    
  if (!Gen_ptr_avail)
    p = MACE_tp_alloc(sizeof(struct gen_ptr));
  else {
    p = Gen_ptr_avail;
    Gen_ptr_avail = Gen_ptr_avail->next;
  }
  p->next = NULL;
  return(p);
}  /* get_gen_ptr */

/*************
 *
 *    free_gen_ptr() -- free a gen_ptr node.
 *
 *************/

static void free_gen_ptr(Gen_ptr_ptr p)
{
  p->next = Gen_ptr_avail;
  Gen_ptr_avail = p;
}  /* free_gen_ptr */

/*************
 *
 *   init_dp()
 *
 *************/

void init_dp()
{
  MACE_init_options();
  init_stats();
  init_clocks();
}  /* init_dp */

/*************
 *
 *   exit_if_over_time_limit()
 *
 *************/

void exit_if_over_time_limit()
{
  int sec = run_time() / 1000;
  if (sec > MACE_Parms[MAX_TP_SECONDS].val)
    exit_with_message(MACE_MAX_SECONDS_EXIT, 1);
}  /* exit_if_over_time_limit */

/*************
 *
 *   subsumed()
 *
 *   Check if any of the literals is assigned TRUE.
 *
 *************/

int subsumed(Clause_ptr c)
{
  int i;
  for (i = 0; i < c->num_pos; i++)
    if (Atoms[c->pos[i]].value == ATOM_TRUE)
      return(1);
  for (i = 0; i < c->num_neg; i++)
    if (Atoms[c->neg[i]].value == ATOM_FALSE)
      return(1);
  return(0);
}  /* subsumed */

/*************
 *
 *   init_unit_queue()
 *
 *************/

static void init_unit_queue(void)
{
  Unit_queue_first = 0;
  Unit_queue_last = -1;
}  /* init_unit_queue */

/*************
 *
 *   unit_enqueue(c)
 *
 *   It is assumed that there is exactly 1 active literal.  The clause
 *   may be subsumed.  The queue is not allowed to have duplicate or
 *   complementary literals.
 *
 *************/

static int unit_enqueue(Clause_ptr c)
{
  ATOM_INT *ip;
  int lit, ev, i;

  /* First find the unit (*ip gets atom, lit gets literal).
   * If subsumption is disabled, then a clause may have "true"
   * literals that are included in the "active" count; in this
   * case, c's "active" literal is already true, so it is not
   * enqueued.
   */

  if (c->active_pos == 1) {
    for (ip=c->pos, i = 0;
	 i < c->num_pos && Atoms[*ip].value != ATOM_NOT_ASSIGNED;
	 ip++, i++);
    if (i == c->num_pos) {
      /* Subsumption must be off, and c is really subsumed. */
#ifdef TRACE2
      printf("		ENQUEUE: subsumed: "); MACE_p_clause(c);
#endif
      return(NOT_DETERMINED);
    }
    else
      lit = *ip;
  }
  else {
    for (ip=c->neg, i = 0;
	 i < c->num_neg && Atoms[*ip].value != ATOM_NOT_ASSIGNED;
	 ip++, i++);
    if (i == c->num_neg) {
      /* Subsumption must be off, and c is really subsumed. */
#ifdef TRACE2
      printf("		ENQUEUE: subsumed: "); MACE_p_clause(c);
#endif
      return(NOT_DETERMINED);
    }
    else
      lit = -(*ip);
  }

  ev = Atoms[*ip].enqueued_value;

  if (ev == NOT_DETERMINED) {
    Atoms[*ip].enqueued_value = lit > 0 ? ATOM_TRUE : ATOM_FALSE;
    Unit_queue_last++;
    if (Unit_queue_last == Unit_queue_size)
      MACE_abend("unit_enqueue: queue too small");
    Unit_queue[Unit_queue_last] = lit;
#ifdef WORK0
    printf("		ENQUEUE: ");
    p_unit(lit);
    printf(" (%d) ", lit);
    MACE_pp_clause(c);
#endif
    return(NOT_DETERMINED);
  }
  else if ((ev == ATOM_TRUE  && lit < 0) ||
	   (ev == ATOM_FALSE && lit > 0)) {
#ifdef TRACE
    printf("		ENQUEUE: %d unsat.\n", lit);
#endif
    return(UNSATISFIABLE);
  }
  else {
#ifdef TRACE2
    printf("		ENQUEUE: %d already in queue.\n", lit);
#endif
    return(NOT_DETERMINED);
  }
}  /* unit_enqueue */

/*************
 *
 *   unit_dequeue()
 *
 *   Also reset the enqueued_value field of the atom.
 *
 *************/

static int unit_dequeue(void)
{
  int lit;

  if (Unit_queue_last < Unit_queue_first)
    return(0);
  else {
    lit = Unit_queue[Unit_queue_first];
    Unit_queue_first++;
    Atoms[abs(lit)].enqueued_value = NOT_DETERMINED;
#ifdef TRACE
    printf("		dequeue: ");
    p_unit(lit);
    printf(" (%d).\n", lit);
#endif
    return(lit);
  }
}  /* unit_dequeue */

/*************
 *
 *   MACE_p_clause()
 *
 *************/

void MACE_p_clause(Clause_ptr c)
{
  int j;
    
  printf("( ");
  for (j = 0; j < c->num_neg; j++)
    printf("-%d ", c->neg[j]);
  for (j = 0; j < c->num_pos; j++)
    printf("%d ", c->pos[j]);
  printf(") active_neg=%d, active_pos=%d, subsumer=%d\n", c->active_neg,
	 c->active_pos, c->subsumer);
}  /* MACE_p_clause */

/*************
 *
 *   MACE_pp_clause()
 *
 *************/

void MACE_pp_clause(Clause_ptr c)
{
  int j;
    
  printf("(");
  for (j = 0; j < c->num_neg; j++)
    p_unit(-(c->neg[j]));
  for (j = 0; j < c->num_pos; j++)
    p_unit(c->pos[j]);
  printf("), active=%d\n", c->active_pos+c->active_neg);
}  /* MACE_pp_clause */

/*************
 *
 *   MACE_pp_clause_active()
 *
 *************/

void MACE_pp_clause_active(Clause_ptr c)
{
  int j;
    
  for (j = 0; j < c->num_neg; j++) {
    int atom = c->neg[j];
    if (Atoms[atom].value == ATOM_NOT_ASSIGNED)
      p_unit(-atom);
  }
  for (j = 0; j < c->num_pos; j++) {
    int atom = c->pos[j];
    if (Atoms[atom].value == ATOM_NOT_ASSIGNED)
      p_unit(atom);
  }
  printf("\n");
}  /* MACE_pp_clause_active */

/*************
 *
 *   p_atom()
 *
 *************/

void p_atom(int i)
{
  int j;
  Atom_ptr a = Atoms + i;

  printf("Atom=%d, value=%d, pos_occ=", i, a->value);
  for (j = 0; j < a->num_pos_occ; j++)
    printf("%x,", (unsigned) a->pos_occ[j]);
  printf(" neg_occ=");
  for (j = 0; j < a->num_neg_occ; j++)
    printf("%x,", (unsigned) a->neg_occ[j]);
  printf(".\n");
}  /* p_atom */

/*************
 *
 *   insert_dp_clause()
 *
 *   Take a propositional clause (array of integers) and insert it into
 *   the set of DP clauses.
 *
 *************/

void insert_dp_clause(int c[], int n)
{
  Clause_ptr d;
  int i, np, nn;

  /* Find out how many pos and neg literals, so that arrays can be allocated.
   */

  np = nn = 0;
  for (i = 0; i < n; i++) {
    if (abs(c[i]) > ATOM_INT_MAX)
      MACE_abend("insert_dp_clause, increase ATOM_INT_MAX and recompile.");
    if (abs(c[i]) > Greatest_atom)
      Greatest_atom = abs(c[i]);
    if (c[i] > 0)
      np++;
    else
      nn++;
  }

  /* Allocate the clause and the literal arrays. */
	
  d = MACE_tp_alloc(sizeof(struct MACE_clause));
  d->next = NULL; d->subsumer = 0;
  d->num_pos = np; d->active_pos = np;
  d->num_neg = nn; d->active_neg = nn;
  if (Prev_dp)
    Prev_dp->next = d;
  else
    Clauses = d;
  Prev_dp = d;

  d->pos = MACE_tp_alloc(np * sizeof(ATOM_INT));  /* ok if np==0 */
  d->neg = MACE_tp_alloc(nn * sizeof(ATOM_INT));  /* ok if nn==0 */

  /* Fill in the literal arrays (neg literals are stored positively). */

  np = nn = 0;
  for (i = 0; i < n; i++) {
    if (c[i] > 0)
      d->pos[np++] = c[i];
    else
      d->neg[nn++] = -c[i];
  }
  Num_clauses++;
  MACE_Stats[INPUT_CLAUSES] = Num_clauses;
  MACE_Stats[GREATEST_ATOM] = Greatest_atom;
#ifdef WORK1
  MACE_pp_clause(d);
#endif
}  /* insert_dp_clause */

/*************
 *
 *   read_one_clause(fp)
 *
 *   Read a propositional clause (sequence of nonzero integers, 
 *   terminated with 0) from a file, and insert it into the DP clause set.
 *   (This is not used for first-order problems.)
 *
 *   Return  EOF at EOF, 0 if error, 1 if clase read successfully.
 *
 *************/

static int read_one_clause(FILE *fp)
{
  int c[LIT_INT_MAX], num_lits, lit, rc;

  num_lits = 0;
  do {
    rc = fscanf(fp, "%d", &lit);
    if (rc == EOF)
      return(EOF);
    else if (rc == 0)
      return(0);
    else if (lit != 0)
      c[num_lits++] = lit;
  } while (lit != 0);

  insert_dp_clause(c, num_lits);

  return(1);
}  /* read_one_clause */

/*************
 *
 *   read_all_clauses(fp)
 *
 *   (This is not used for first-order problems.)
 *   Abend if an error is found.
 *
 *************/

int read_all_clauses(FILE *fp)
{
  int rc = 1;

  while (rc == 1)
    rc = read_one_clause(fp);

  if (rc == 0)
    MACE_abend("Propositional input must be all integers; if you have\nfirst-order input, the domain size must be given (-n).");

  return(Num_clauses);
}  /* read_all_clauses */

/*************
 *
 *   more_setup()
 *
 *   Assume all clauses have been inserted into the DP set.  This routine
 *   sets up the unit propagation queue, and the atom array (which is
 *   used for accessing clauses that contain a given atom).
 *
 *************/

int more_setup()
{
  int i, j;
  Clause_ptr c;
  Atom_ptr a;

#if 0
  for (c = Clauses; c; c = c->next)
    MACE_p_clause(c);
#endif
  /* Allocate queue for unit propagation (Unit_queue is a global variable). */

  Unit_queue_size = Greatest_atom + 1;
  i = Unit_queue_size * sizeof(int);
  Unit_queue = malloc((size_t) i);
  MACE_Stats[MEM_MALLOCED] += i;
  init_unit_queue();

  /* Now set up the atom array (indexed with 1--Greatest_atom).
   * Each atom struct has lists of positive occurrences and
   * negative occurrences.  To save space, the lists are stored
   * as arrays; but we don't know how big the arrays should be
   * until we process the clauses.  So we make two passes through
   * the clauses.
   */

  i = (Greatest_atom+1) * sizeof(struct atom);
  Atoms = malloc((size_t) i);
  MACE_Stats[MEM_MALLOCED] += i;

  for (i = 0, a = Atoms; i <= Greatest_atom; i++, a++) {
    a->value = ATOM_NOT_ASSIGNED;
    a->enqueued_value = ATOM_NOT_ASSIGNED;
    a->num_pos_occ = 0;
    a->pos_occ = NULL;
    a->num_neg_occ = 0;
    a->neg_occ = NULL;
  }

  /* For each literal of each clause, increment occurrence count
   * in atom structure.  (Pass 1 through the clauses.)
   */

  for (c = Clauses; c; c = c->next) {
    for (j = 0; j < c->num_pos; j++) {
      a = Atoms + c->pos[j];
      a->num_pos_occ++;
    }
    for (j = 0; j < c->num_neg; j++) {
      a = Atoms + c->neg[j];
      a->num_neg_occ++;
    }
  }

  /* For each atom, allocate occurrence arrays, and reset occurrence
   * counts to 0.
   */

  for (i = 0, a = Atoms; i <= Greatest_atom; i++, a++) {
    MACE_Stats[LIT_OCC_INPUT] += a->num_pos_occ;
    a->pos_occ = MACE_tp_alloc(a->num_pos_occ * sizeof(Clause_ptr));
    a->num_pos_occ = 0;

    MACE_Stats[LIT_OCC_INPUT] += a->num_neg_occ;
    a->neg_occ = MACE_tp_alloc(a->num_neg_occ * sizeof(Clause_ptr));
    a->num_neg_occ = 0;
  }

  exit_if_over_time_limit();

  /* For each literal of each clause, add clause to occurrence
   * array of atom.  (Pass 2 through the clauses.)
   */

  for (c = Clauses; c; c = c->next) {
    for (j = 0; j < c->num_pos; j++) {
      a = Atoms + c->pos[j];
      a->pos_occ[a->num_pos_occ++] = c;
    }
    for (j = 0; j < c->num_neg; j++) {
      a = Atoms + c->neg[j];
      a->neg_occ[a->num_neg_occ++] = c;
    }
  }

  return(Num_clauses);
}  /* more_setup */

/*************
 *
 *   select_init()
 *
 *   Initialize the mechanism for selecting atoms for splitting;
 *
 *************/

static void select_init(void)
{
  Clause_ptr c;
  int j;
  struct glist *selectable = NULL;
  struct glist *p;
    
  /* We will select literals from (smallest) positive clauses for splitting.
   * To avoid scanning all clauses, we build an array of pointers
   * to all of the nonsubsumed clauses with 2 or more positive literals.
   * (Horn clauses will be handled by unit propagation.)
   *
   * We'll first collect them into a list, then allocate an array
   * of the right size, then copy them into the array.
   */

  for (c = Clauses, Num_selectable_clauses = 0; c; c = c->next) {
    /* Recall that even if subsumption is disabled, it is done
     * during preprocessing, which has just occurred.
     */
    if (!c->subsumer && c->num_pos >= 2) {
      Num_selectable_clauses++;
      selectable = glist_prepend(c, selectable);
    }
  }

  /* Now we have the selectable clauses in reverse order. */

  MACE_Stats[SELECTABLE_CLAUSES] = Num_selectable_clauses;

  /* Set up array of selectable clauses for select_atom(). */

  j = Num_selectable_clauses * sizeof(Clause_ptr *);
  Selectable_clauses = malloc((size_t) j);
  MACE_Stats[MEM_MALLOCED] += j;

  /* Copy the list of pointers into the array and free the list. */

  for (j = Num_selectable_clauses-1, p = selectable;
       j >= 0 && p != NULL;
       j--, p = p->next) {
    Selectable_clauses[j] = p->v;
  }

  free_glist_list(selectable);

  if (MACE_Flags[RECORD_ASSIGNMENTS].val) {
    printf("\nSelectable clauses:\n");
    for (j = 0; j < Num_selectable_clauses; j++)
      MACE_pp_clause(Selectable_clauses[j]);
  }
}  /* select_init */

/*************
 *
 *   select_atom()
 *
 *   Go through the array of selectable clauses, and select the first active
 *   literal in the first shortest nonsubsumed clause.
 *
 *************/

static int select_atom(void)
{
  ATOM_INT *ip;
  int min, atom, i;
  Clause_ptr c;

  min = LIT_INT_MAX;
  atom = 0;
#if 1
  for (i = 0; i < Num_selectable_clauses; i++)
#else
    for (i = Num_selectable_clauses-1; i >= 0; i--)
#endif
      {
	c = Selectable_clauses[i];
	if (c->active_neg == 0 && c->active_pos < min && !SUBSUMED(c)) {
	  min = c->active_pos;
	  for (ip = c->pos; Atoms[*ip].value != ATOM_NOT_ASSIGNED; ip++);
	  atom = *ip;
	  if (atom == 0)
	    MACE_abend("HERE it is!!");
	}
      }
  return(atom);
}  /* select_atom */

/*************
 *
 *   select_clause()
 *
 *   Go through the array of selectable clauses, and select the first
 *   shortest nonsubsumed clause.
 *
 *************/

static Clause_ptr select_clause(void)
{
  int min, i;
  Clause_ptr c, min_clause;

  min = LIT_INT_MAX;
  min_clause = 0;
#if 1
  for (i = 0; i < Num_selectable_clauses; i++)  /* forward */
#else
  for (i = Num_selectable_clauses-1; i >= 0; i--)  /* backward */
#endif
    {
      c = Selectable_clauses[i];
      if (c->active_neg == 0 && c->active_pos < min && !SUBSUMED(c)) {
	min_clause = c;
	min = c->active_pos;
      }
    }
  return(min_clause);
}  /* select_clause */

/*************
 *
 *   atom_value()
 *
 *   Return the value of an atom.  This can be called from outside code,
 *   (i.e., first-order model printer) so the defined symbols are not used.
 *
 *************/

int atom_value(int atom)
{
  int rc = -1;
  if (atom < 1 || atom > Greatest_atom)
    MACE_abend("atom_value, atom out of range");
  switch(Atoms[atom].value) {
  case ATOM_FALSE: rc = 0; break;
  case ATOM_TRUE:  rc = 1; break;
  case ATOM_NOT_ASSIGNED: rc = 2; break;
  default: MACE_abend("atom_value, bad value");
  }
  return(rc);
}  /* atom_value */

/*************
 *
 *   print_initial_assignments()
 *
 *************/

static void print_initial_assignments(void)
{
  int i;
  printf("\nInitial assigments: ");
  for (i = 0; i < Num_assignments; i++) {
    p_unit(Assignments[i]);
  }
  printf("\n");
}  /* print_initial_assignments */

/*************
 *
 *   print_assignments(depth)
 *
 *************/

static void print_assignments(int depth)
{
  int i, k, stop;

  printf("\nOrder of assigments for the following model:\n");

  for (i = 0; i < depth; i++) {
    k = Split_positions[i];
    p_unit(Assignments[k]);
    printf(": ");
    stop = (i < depth-1 ? Split_positions[i+1] : Num_assignments);
    for (k = k+1; k < stop; k++) {
      p_unit(Assignments[k]);
    }
    printf("\n\n");
  }
}  /* print_assignments */

/*************
 *
 *   model(depth)
 *
 *   this routine is called when a model has been found.
 *
 *************/

static void model(int depth)
{
  int i;

  if (MACE_Flags[RECORD_ASSIGNMENTS].val)
    print_assignments(depth);

  Models++;
  if (MACE_Flags[PRINT_MODELS].val) {
    if (!First_order) {
      printf("\nModel #%d:", Models);
      for (i = 1; i <= Greatest_atom; i++)
	if (Atoms[i].value == ATOM_TRUE)
	  printf(" %d", i);
      printf("\n");
      printf("end_of_model\n");
    }
    else
      print_model(stdout);  /* print first_order model */
  }
    
  if (MACE_Flags[PRINT_MODELS_PORTABLE].val) {
    if (!First_order) {
      printf("\nModel #%d:", Models);
      for (i = 1; i <= Greatest_atom; i++)
	if (Atoms[i].value == ATOM_TRUE)
	  printf(" %d", i);
      printf("\n");
      printf("end_of_model\n");
    }
    else
      print_model_portable(stdout);  /* print first_order model */
  }
    
  if (MACE_Flags[PRINT_MODELS_IVY].val) {
    if (!First_order) {
      printf("\nModel #%d:", Models);
      for (i = 1; i <= Greatest_atom; i++)
	if (Atoms[i].value == ATOM_TRUE)
	  printf(" %d", i);
      printf("\n");
      printf("end_of_model\n");
    }
    else
      print_model_ivy(stdout);  /* print first_order model */
  }
    
  if (!MACE_Flags[PRINT_MODELS_PORTABLE].val && !MACE_Flags[PRINT_MODELS].val) {
    if (Models == Next_message) {
      printf("\nThe %d%s model has been found.\n", Models,
	     Next_message == 1 ? "st" : "th");
      Next_message *= 10;
    }
  }

  fflush(stdout);

  if (Models >= MACE_Parms[MAX_MODELS].val)
    exit_with_message(MAX_MODELS_EXIT, 1);
}  /* model */

/*************
 *
 *   assign(lit) - make a literal true
 *
 *   1. Make the assignment in the atom array.
 *   2. (optional) Mark nonsubsumed clauses that contain it as subsumed by it.
 *   3. Unit resolution on clauses that are not marked as subsumed.
 *      If a unit is found, enqueue it for unit propagation.
 *   4. If empty clause is found, return UNSATISFIABLE else NOT_DETERMINED.
 *
 *************/

static int assign(int lit)
{
  Atom_ptr a;
  Clause_ptr c;
  int atom, value, i, rc, n, np, nn;

#ifdef WORK0
  p_unit(lit); printf("\n");
#endif

  value = (lit > 0 ? ATOM_TRUE : ATOM_FALSE);
  atom = abs(lit);

  rc = NOT_DETERMINED;
  a = Atoms + atom;

  np = a->num_pos_occ;
  nn = a->num_neg_occ;

  if (a->value != ATOM_NOT_ASSIGNED)
    MACE_abend("assign: atom already assigned");
  else
    a->value = value;

  if (MACE_Flags[RECORD_ASSIGNMENTS].val)
    Assignments[Num_assignments++] = lit;

  if (value == ATOM_TRUE) {  /* Assign true to atom */
    if (Subsume) 
      for (i = 0; i < np; i++) {
	c = a->pos_occ[i];
	if (!c->subsumer)
	  c->subsumer = atom;
      }
    for (i = 0; i < nn; i++) {
      c = a->neg_occ[i];
      if (!c->subsumer) {
	c->active_neg--;
	n = c->active_pos + c->active_neg;
	if (n == 0)
	  rc = UNSATISFIABLE;
	else if (n == 1 && unit_enqueue(c) == UNSATISFIABLE)
	  rc = UNSATISFIABLE;
      }
    }
  }

  else {  /* Assign false to atom */
    if (Subsume)
      for (i = 0; i < nn; i++) {
	c = a->neg_occ[i];
	if (!c->subsumer)
	  c->subsumer = atom;
      }
    for (i = 0; i < np; i++) {
      c = a->pos_occ[i];
      if (!c->subsumer) {
	c->active_pos--;
	n = c->active_neg + c->active_pos;
	if (n == 0)
	  rc = UNSATISFIABLE;
	else if (n == 1 && unit_enqueue(c) == UNSATISFIABLE)
	  rc = UNSATISFIABLE;
      }
    }
  }
  if (rc == UNSATISFIABLE)
    MACE_Stats[FAILED_PATHS]++;
  return(rc);
}  /* assign */

/*************
 *
 *   unassign(lit) -- undo an assignment
 *
 *************/

static void unassign(int lit)
{
  Atom_ptr a;
  Clause_ptr c;
  int i, atom, np, nn;
    
  atom = abs(lit);
  a = Atoms + atom;

  if (a->value == ATOM_NOT_ASSIGNED)
    MACE_abend("unassign: atom not assigned");

  if (MACE_Flags[RECORD_ASSIGNMENTS].val)
    Num_assignments--;

  np = a->num_pos_occ;
  nn = a->num_neg_occ;
    
  if (a->value == ATOM_TRUE) {  /* Unassign true. */
    if (Subsume)
      for (i = 0; i < np; i++) {
	c = a->pos_occ[i];
	if (c->subsumer == atom)
	  c->subsumer = 0;
      }
    for (i = 0; i < nn; i++) {
      c = a->neg_occ[i];
      if (!c->subsumer)
	c->active_neg++;
    }
  }
  else {  /* Unassign false. */
    if (Subsume)
      for (i = 0; i < nn; i++) {
	c = a->neg_occ[i];
	if (c->subsumer == atom)
	  c->subsumer = 0;
      }
    for (i = 0; i < np; i++) {
      c = a->pos_occ[i];
      if (!c->subsumer)
	c->active_pos++;
    }
  }
  a->value = ATOM_NOT_ASSIGNED;
}  /* unassign */

/*************
 *
 *   dp(depth) -- the kernel of the Davis-Putnam procedure
 *
 *   depth: current recursion level---for debugging only.
 *
 *************/

static void dp(int depth)
{
  int lit, unit, rc, j;
  Gen_ptr_ptr unit_assignments, p1;

  if (Check_time)
    exit_if_over_time_limit();

  lit = select_atom();

  if (lit == 0) {
    model(depth);  /* Nothing to select, so we have a model. */
    return;
  }

  MACE_Stats[SPLITS]++;

  for (j = 0; j < 2; j++) {
    init_unit_queue();

    if (MACE_Flags[RECORD_ASSIGNMENTS].val)
      Split_positions[depth] = Num_assignments;

#ifdef TRACE
    { 
      int i;
      for (i = 0; i < depth; i++) printf("   ");
      p_unit(lit); printf(" %3d\n", lit);
    }
#endif

    rc = assign(lit);

    /* Unit propagation */

    unit_assignments = NULL;
    while (rc == NOT_DETERMINED && (unit = unit_dequeue()) != 0) {
      MACE_Stats[UNIT_ASSIGNS]++;
#ifdef TRACE
      { 
	int i;
	for (i = 0; i < depth; i++) printf("   ");
	p_unit(unit); printf(" %3d*\n", unit);
      }
#endif
      rc = assign(unit);
      /* Save the assignment so that it can be undone. */
      p1 = get_gen_ptr();
      p1->u.i = unit;
      p1->next = unit_assignments;
      unit_assignments = p1;
    }

    if (rc == NOT_DETERMINED)
      dp(depth+1);
    else  /* rc == UNSATISFIABLE, so empty queue to reset any enqueued_values */
      while ((unit = unit_dequeue()) != 0);

    /* Undo unit propagation assignments */

    while (unit_assignments) {
#ifdef TRACE
      { 
	int i;
	for (i = 0; i < depth; i++) printf("   ");
	p_unit(unit_assignments->u.i);
	printf("  [%3d]*\n", unit_assignments->u.i);
      }
#endif
      unassign(unit_assignments->u.i);
      p1 = unit_assignments;
      unit_assignments = unit_assignments->next;
      free_gen_ptr(p1);
    }

#ifdef TRACE
    { 
      int i;
      for (i = 0; i < depth; i++) printf("   ");
      p_unit(lit);
      printf("  [%3d]\n", lit);
    }
#endif
    unassign(lit);

    if (j == 0)
      lit = -lit;
  }
}  /* dp */

/* ISO_X experiment */

/* #define ISO_PRINT */

#define ISO_PRUNE   0
#define SPECIFIED   1
#define UNSPECIFIED 2

/*************
 *
 *   function_value()
 *
 *************/

int function_value(int atom)
{
  char *func;
  int arity;
  int args[4];
  decode_int(atom, &func, &arity, args);
  return args[arity-1];
}  /* function_value */

/*************
 *
 *   iso_prune()
 *
 *************/

int iso_prune(int atom, int iso_set[], int iso_flag, int depth)
{
  if (!MACE_Flags[ISO_X].val)
    return SPECIFIED;
  else {
    int fval = function_value(atom);
    int rc;

    if (jset_member(iso_set, fval))
      rc = SPECIFIED;
    else if (iso_flag)
      rc = ISO_PRUNE;
    else {
      add_to_jset(iso_set, fval);
      rc = UNSPECIFIED;
    }
#ifdef ISO_PRINT
    {
      int i;
      for (i = 0; i < depth; i++) printf(" ");
      printf("atom, depth %d:", depth);
      p_unit(atom);
      switch (rc) {
      case SPECIFIED: printf(" specified\n"); break;
      case UNSPECIFIED: printf(" unspecified\n"); break;
      case ISO_PRUNE: printf(" pruned\n"); break;
      }
    }
#endif
    return rc;
  }
}  /* iso_prune */

/*************
 *
 *   add_args_to_set()
 *
 *************/

void add_args_to_set(Clause_ptr c, int *iso_set)
{
  int atom = c->pos[0];

  char *func;
  int arity;
  int args[4];
  int function_value;
  int i;

  decode_int(atom, &func, &arity, args);
  
  for (i = 0; i < (arity-1); i++)
    add_to_jset(iso_set, args[i]);
  
}  /* add_args_to_set */

/*************
 *
 *   clause_split(depth) -- the kernel of the clause-splitting procedure
 *
 *   depth: current recursion level---for debugging only.
 *
 *************/

static void clause_split(int depth, int *iso_set)
{
  int unit, rc, j;
  Gen_ptr_ptr unit_assignments, p1;
  Clause_ptr c;
  int iso_flag = 0;  /* Have we done a non specified element yet? */
  int *iso_seta;

  if (Check_time)
    exit_if_over_time_limit();

  c = select_clause();

#ifdef WORK0
  if (c)
    { printf("Splitting clause" );  MACE_pp_clause_active(c); }
#endif

  if (c == NULL) {
    model(depth);  /* Nothing to select, so we have a model. */
    return;
  }

  if (MACE_Flags[ISO_X].val) {
    iso_seta = copy_jset(iso_set);
    add_args_to_set(c, iso_seta);  /* assume it is a closure clause */
#ifdef ISO_PRINT
    printf("\nIncoming set, depth %d: ", depth); print_set(iso_set);
    printf("Splitting clause" );  MACE_pp_clause_active(c);
    printf("Updated set: "); print_set(iso_seta);
    printf("\n");
#endif
  }

  MACE_Stats[SPLITS]++;

  /* We cannot assume that c is positive. */

  for (j = 0; j < c->num_pos+c->num_neg; j++) {
    int sign = (j < c->num_pos);
    int atom = (j < c->num_pos ? c->pos[j] : c->neg[j - c->num_pos]);
    int lit = (sign ? atom : -atom);

    if (Atoms[atom].value == ATOM_NOT_ASSIGNED) {  /* process active literals only */

      int iso = iso_prune(atom, iso_seta, iso_flag, depth);

      if (iso == ISO_PRUNE) {
	/* do nothing */
      }
      else {
	if (MACE_Flags[RECORD_ASSIGNMENTS].val)
	  Split_positions[depth] = Num_assignments;

	init_unit_queue();
	rc = assign(lit);

	/* Unit propagation */

	unit_assignments = NULL;
	while (rc == NOT_DETERMINED && (unit = unit_dequeue()) != 0) {
	  MACE_Stats[UNIT_ASSIGNS]++;
	  rc = assign(unit);
	  /* Save the assignment so that it can be undone. */
	  p1 = get_gen_ptr();
	  p1->u.i = unit;
	  p1->next = unit_assignments;
	  unit_assignments = p1;
	}

	if (rc == NOT_DETERMINED)
	  clause_split(depth+1, iso_seta);
	else  /* rc == UNSATISFIABLE, so empty queue to reset any enqueued_values */
	  while ((unit = unit_dequeue()) != 0);

	/* Undo unit propagation assignments */

	while (unit_assignments) {
	  unassign(unit_assignments->u.i);
	  p1 = unit_assignments;
	  unit_assignments = unit_assignments->next;
	  free_gen_ptr(p1);
	}

	unassign(lit);

	if (iso == UNSPECIFIED) {
	  remove_from_jset(iso_seta, function_value(atom));
	  iso_flag = 1;
	}
      }  /* not pruned */
    }  /* active literals only */
  }  /* for each literal */
  if (MACE_Flags[ISO_X].val)
    free(iso_seta);
}  /* clause_split */

/*************
 *
 *   delete_pointers_to_subsumed_clauses()
 *
 *   This routine is called after the initial unit propagation (before
 *   any splitting).  It goes through the occurrence lists in the
 *   atom array and deletes references to subsumed clauses.  The reason
 *   for this is simply to speed traversals during assign() and unassign().
 *
 *   This is intended for use only after the initial unit propagation,
 *   because this operation is not undoable.
 *
 *************/

static void delete_pointers_to_subsumed_clauses(void)
{
  int i, j, k;
  Atom_ptr a;

  /* The set of occurrences, say positive, of an atom, is kept as
   * an array of pointers to clauses.  Pointers to subsumed clauses
   * are removed, and the the others are shifted left the apropriate
   * amount.  Same for negative occurrences.
   */

  for (i = 1; i <= Greatest_atom; i++) {
    a = Atoms + i;
    for (j = 0, k = 0; j < a->num_pos_occ; j++)
      if (a->pos_occ[j]->subsumer == 0)
	a->pos_occ[k++] = a->pos_occ[j];
    a->num_pos_occ = k;
    MACE_Stats[LIT_OCC_AFTER_SUB] += k;

    for (j = 0, k = 0; j < a->num_neg_occ; j++)
      if (a->neg_occ[j]->subsumer == 0)
	a->neg_occ[k++] = a->neg_occ[j];
    a->num_neg_occ = k;
    MACE_Stats[LIT_OCC_AFTER_SUB] += k;
  }
}  /* delete_pointers_to_subsumed_clauses */

/*************
 *
 *   dp_prover()
 *
 *   This is the top (nonrecursive) routine of the Davis-Putnam procedure.
 *
 *************/

int dp_prover()
{
  int rc, unit, n;
  Clause_ptr c;

  Check_time = (MACE_Parms[MAX_TP_SECONDS].val < INT_MAX);

  if (MACE_Flags[RECORD_ASSIGNMENTS].val) {
    Assignments = malloc((size_t) Greatest_atom * sizeof(int));
    Split_positions = malloc((size_t) Greatest_atom * sizeof(int));
    Num_assignments = 0;
  }

  /* Initial unit propagation and subsumption. */

  Subsume = 1;  /* Always mark subsumed clauses while preprocessing. */

  for (c=Clauses, rc=NOT_DETERMINED; c && rc == NOT_DETERMINED; c = c->next)
    if (c->num_pos + c->num_neg == 1) {
      rc = unit_enqueue(c);
    }

  while (rc == NOT_DETERMINED && (unit = unit_dequeue()) != 0) {
    MACE_Stats[PREPROCESS_UNIT_ASSIGNS]++;
    rc = assign(unit);  /* This can enqueue more units */
  }

  if (MACE_Stats[PREPROCESS_UNIT_ASSIGNS] > 0)
    delete_pointers_to_subsumed_clauses();
  else
    MACE_Stats[LIT_OCC_AFTER_SUB] = MACE_Stats[LIT_OCC_INPUT];

  for (c = Clauses, n=0; c; c = c->next)
    if (c->subsumer)
      n++;

  MACE_Stats[CLAUSES_AFTER_SUB] = MACE_Stats[INPUT_CLAUSES] - n;

  Subsume = MACE_Flags[SUBSUME].val;

  /* Done with initial unit propagation and subsumption. */

  select_init();  /* Initialize the selection mechanism for splitting. */

  if (MACE_Flags[RECORD_ASSIGNMENTS].val)
    print_initial_assignments();

  printf("\nAfter all unit preprocessing, %ld atoms are still unassigned;\n"
	 "%ld clauses remain; %ld of those are non-Horn (selectable);\n"
	 "%ld K allocated; cpu time so far for this domain size: %.2f sec.\n",
	 Greatest_atom - MACE_Stats[PREPROCESS_UNIT_ASSIGNS],
	 MACE_Stats[CLAUSES_AFTER_SUB], MACE_Stats[SELECTABLE_CLAUSES],
	 MACE_Stats[MEM_MALLOCED]/1024 + MACE_total_mem(),
	 (MACE_clock_val(DECIDE_TIME) + MACE_clock_val(GENERATE_TIME)) / 1000.);
  fflush(stdout);
 
  if (rc == NOT_DETERMINED) {

    if (MACE_Flags[CLAUSE_SPLIT].val) {
      int *iso_set;
      if (MACE_Flags[ISO_X].val) {
	set_jset_size(Domain_size);
	iso_set = get_jset();
      }
      clause_split(0, iso_set);     /* recursive clause-splitting routine */
    }
    else
      dp(0);                /* recursive Davis-Putnam routine */
  }

  if (Models == 0)
    return(UNSATISFIABLE_EXIT);
  else
    return(ALL_MODELS_EXIT);
}  /* dp_prover */

/*************
 *
 *    reinit_dp
 *
 *    Free memory and reinitialize global variables in this file.
 *
 *************/

void reinit_dp(void)
{
  free(Unit_queue);
  free(Atoms);
  free(Selectable_clauses);
  if (MACE_Flags[RECORD_ASSIGNMENTS].val) {
    free(Assignments);
    free(Split_positions);
    Num_assignments = 0;
  }

  Greatest_atom = 0;
  Subsume = 0;
  Check_time = 0;
  Num_clauses = 0;
  Clauses = NULL;
  Atoms = NULL;
  Num_selectable_clauses = 0;
  Selectable_clauses = NULL;
  Unit_queue = NULL;
  Unit_queue_size = 0;
  Unit_queue_first = 0;
  Unit_queue_last = 0;
  Prev_dp = NULL;
  Gen_ptr_avail = NULL;
  Next_message = 1;
}  /* reinit_dp */

