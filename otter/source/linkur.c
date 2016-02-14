/*
 *  linkur.c -- Linked hyperresolution inference rule.
 *
 */

#include "header.h"

/*
 *  As modified by SKW Apr 10 90 and May 15 90,
 *  from "release 2" Otter with linking.
 *  Added routines:  subsumable_unit, linked_unit_del.
 *  To see the changes, search for SKW.
 */

/* When I changed to new-style function prototypes (May 2000),
 * I put all of these static declarations here, because the
 * corresponding definitions aren't ordered nicely.  Not all
 * of these are necessary.
 */

static void construct_children_nodes(struct link_node *curr_node,
				     struct term *tp,
				     struct link_node **target,
				     char target_here);
static struct clause *subsumable_unit(char sign,
				      struct term *d);
static struct clause *linked_unit_del(char sign,
				      struct term *d);
static struct link_node *backward(struct link_node **tree,
				  struct link_node *node,
				  struct link_node **target,
				  int *nopen,
				  int *nres);
static struct ilist *build_parental_chain(struct link_node *node,
					    struct link_node *target);
static struct clause *build_ur_resolvent(struct link_node *target,
					 struct link_node *tree);
static BOOLEAN check_down_tree(struct link_node *node,
			       int my_depth);
static BOOLEAN check_up_tree(struct link_node *node,
			     int my_depth);
static struct term *first_unifiable(struct term *t,
				    struct fpa_index *index,
				    struct context *subst_t,
				    struct context *subst_ret,
				    struct fpa_tree **pos_ptr,
				    struct trail **tr_ptr,
				    struct link_node *curr_node,
				    struct link_node *target,
				    char *target_here,
				    int *hit_dp_count);
static struct link_node *forward(struct link_node *cn,
				 struct link_node *target);
static struct link_node *forward_from_resolved_tree(struct link_node *curr_node,
						    struct link_node **target,
						    int *nres,
						    int *nopen);
static void free_linked_node_tree(struct link_node *tree,
				  struct link_node **target);
static struct term *generate_resolvent(struct link_node *curr_node,
				       struct link_node *target,
				       char *target_here,
				       int *hit_dp_count);
static struct link_node *initialize_tree(struct clause *giv_cl);
static BOOLEAN is_in_ancestry(struct link_node *curr_node,
			      struct link_node *inf_tree);
static BOOLEAN keep_clause(struct link_node *node,
			   struct link_node **target,
			   char *target_here,
			   int *nopen);
static void linked_print_clause(struct clause *cp);
static void linked_print_link_node(struct link_node *lnp,
				   int lvl);
static void linked_print_link_node_tree(struct link_node *lnp,
					int lvl);
static BOOLEAN more_targets_here(struct term *tp);
static struct term *next_unifiable(struct term *t,
				   struct context *subst_t,
				   struct context *subst_ret,
				   struct fpa_tree **pos_ptr,
				   struct trail **tr_ptr,
				   struct link_node *curr_node,
				   struct link_node *target,
				   char *target_here,
				   int *hit_dp_count);
static BOOLEAN poss_nuc_link(struct link_node *lnp);
static BOOLEAN pass_parms_check(struct link_node *curr_node,
				int nres,
				int nopen,
				int *depth_count,
				int *ded_count,
				struct link_node *tar);
static BOOLEAN pass_target_depth_check(struct link_node *curr_node);
static BOOLEAN process_this_resolution(struct link_node *curr_node,
				       struct link_node *target,
				       struct term *tp,
				       char *target_here,
				       int *hit_dp_count);
static int term_ident_subst(struct term *t1,
			    struct context *c1,
			    struct term *t2,
			    struct context *c2);
static void write_down_tree(struct link_node *node,
			    int my_depth);
static void write_up_tree(struct link_node *node,
			  int my_depth);
static void write_target_distances(struct link_node *curr_node);
  
/************************************************************
 *
 * void construct_children_nodes(curr_node,tp, target, target_here)
 * struct link_node *curr_node;
 * struct term *tp;
 * struct link_node **target;
 * BOOLEAN target_here;
 *
 * It is assumed that the clause linked to tp may be added as children
 * to the curr_node.  This function writes the new depths in the inference
 * tree.  If target_here then re-writes the whole tree accordingly (the
 * test for that was done in process_this_resolution()), otherwise it
 * just writes the depths for the children nodes.
 *
 ************************************************************/

static void construct_children_nodes(struct link_node *curr_node,
				     struct term *tp,
				     struct link_node **target,
				     char target_here)
{

  struct link_node *lnp;
  struct literal *lp;

  if (Flags[LINKED_UR_TRACE].val == 1)
    printf("constructing nodes for remaining lits:\n");

  if (target_here && *target)
    {
      printf("ERROR: constructing children told target here with already target set\n");
      exit(ABEND_EXIT);
    } /* endif */

  /* constructing first child node of curr_node */
  if ((lnp = get_link_node()) == NULL)
    {
      printf("couldn't get link_node\n");
      exit(ABEND_EXIT);
    } /* endif */

  curr_node->first_child = lnp;
  lnp->parent = curr_node;
  lnp->prev_sibling = NULL;
  lnp->first = TRUE;

  /* loop to find first literal other than the one */
  /* that linked into this clause                  */
  for (lp = (curr_node->current_clause)->first_lit;
       lp && lp == tp->occ.lit; lp = lp->next_lit)
    ;

  if (!lp)
    {
      printf("ERROR ... non UNIT clause looks UNIT\n");
      exit(ABEND_EXIT);
    } /* endif */

  if (target_here && lp->target)
    *target = lnp;

  lnp->goal = lp->atom;

  if ((lnp->subst = get_context()) == NULL)
    {
      printf("couldn't get context\n");
      exit(ABEND_EXIT);
    } /* endif */

  (lnp->subst)->multiplier = lnp->subst->built_in_multiplier;

  if (Flags[LINKED_UR_TRACE].val == 1)
    {
      if (!(((lnp->goal)->occ.lit)->sign))
	printf("-");
      print_term(stdout, lnp->goal);
      printf(" from clause ");
      linked_print_clause(((lnp->goal)->occ.lit)->container);
      printf("\n");
    } /* endif */

  /* loop to construct link_node for */
  /* remaining lits in new clause    */
  for (lp = lp->next_lit; lp; lp = lp->next_lit)
    {
      if (lp != tp->occ.lit)
	{
	  if ((lnp->next_sibling = get_link_node()) == NULL)
	    {
	      printf("couldn't get link_node\n");
	      exit(ABEND_EXIT);
	    } /* endif */
	  (lnp->next_sibling)->prev_sibling = lnp;
	  lnp = lnp->next_sibling;
	  lnp->parent = curr_node;
	  lnp->first = TRUE;
	  if (target_here && lp->target && !(*target))
	    *target = lnp;
	  lnp->goal = lp->atom;
	  if ((lnp->subst = get_context()) == NULL)
	    {
	      printf("couldn't get context node\n");
	      exit(ABEND_EXIT);
	    } /* endif */
	  (lnp->subst)->multiplier = lnp->subst->built_in_multiplier;

	  if (Flags[LINKED_UR_TRACE].val == 1)
	    {
	      if (!(((lnp->goal)->occ.lit)->sign))
		printf("-");
	      print_term(stdout, lnp->goal);
	      printf(" from clause ");
	      fflush(stdout);
	      linked_print_clause(((lnp->goal)->occ.lit)->container);
	      printf("\n");
	    } /* endif */

	} /* endif */
    } /* endfor */
  lnp->next_sibling = NULL;

  if (target_here && !(*target))
    {
      printf("ERROR: constructing children told target here and target not found here\n");
      exit(ABEND_EXIT);
    } /* endif */

  /* loop to establish distances for the new clause */
  for (lnp = (lnp->parent)->first_child; lnp; lnp = lnp->next_sibling)
    {
      if (target_here)
	lnp->target_dist = -1;
      else
	{
	  /* target not in this clause */
	  if (*target)
	    /* target is elsewhere */
	    lnp->target_dist = (lnp->parent)->target_dist + 1;
	} /* endif */
      lnp->farthest_sat = (lnp->parent)->farthest_sat + 1;
      lnp->back_up = UNDEFINED;
      if (poss_nuc_link(lnp))
	lnp->near_poss_nuc = 0;
      else
	{
	  if (poss_nuc_link(lnp->parent))
	    lnp->near_poss_nuc = 1;
	  else
	    {
	      if ((lnp->parent)->near_poss_nuc == UNDEFINED)
		lnp->near_poss_nuc = UNDEFINED;
	      else
		lnp->near_poss_nuc = (lnp->parent)->near_poss_nuc + 1;
	    } /* endif */
	} /* endif */
    } /* endfor */

  if (target_here)
    {
      write_target_distances(curr_node);
    }

} /* end construct_children_nodes() */

/*********************************************************************
 *
 *    struct clause *subsumable_unit(sign,d)
 *
 *    Attempt to find a unit clause that subsumes the (unsigned) literal
 *    (atom) d with sign "sign".
 *
 *    SKW - Apr 11 90 - This entire routine added to do subsumable unit
 *    checking in linked resolution.
 *
 **********************************************************************/

static struct clause *subsumable_unit(char sign,
				      struct term *d)
{
  int subsumed;		/* flag set to 1 if literal is subsumed */
  struct clause *c = NULL; /* c is a ptr to a (possible) subsuming cl */

  struct term *c_atom, *d_atom;    /* c_atom and d_atom are ptrs to     */
  /* atoms (unsigned literals) for     */
  /* literal c and clause d resp.      */
  struct context *s;		/* a context is a substitution   */

  struct is_tree *is_db;	/* This is the index that we use.         */
				/* This will be set either to 		 */
				/* Is_pos_lits (global variable indicating */
				/* index for positive literals) or to      */
				/* Is_neg_lits (index for neg literals).    */

  struct fsub_pos *pos;	/* Maintains a position in the sequence of */
				/* subsuming literals.  fsub position.     */
				/* Maintained from one call to the next of */
				/* the indexing routine.		*/

  /* BEGIN DEBUG SKW print out passed term (and possibly sign) */
  /* printf("entering subsumable_unit, passed term follows\n"); */
  /* print_term(stdout,d);  printf("\n"); */
  /* END DEBUG SKW */


  subsumed = 0;		/* initially flag says not subsumed */
  s = get_context();		/* s is initially a null context, void subst */

  /* The following code checks for subsumption of the passed literal "d"
   * by a clause in the list or lists selected by option flags.
   * If Flags[LINKED_SUB_UNIT_USABLE].val is 1,
   * via set(linked_sub_unit_usable), then subsumption by Usable list
   * clauses is checked.
   * If Flags[LINKED_SUB_UNIT_SOS].val is 1,
   * via set(linked_sub_unit_sos), then subsumption by Sos list
   * clauses is checked.
   * If BOTH flags are one, subsumption by both lists is checked.
   * The necessary flags are set up in "cos.h" and in "options.c",
   * in the standard manner used throughout Otter.
   */

  /* The following code for subsumable unit assumes that the option
   * "clear(for_sub_fpa)" is in effect, that is, that forward subsumption
   * by FPA is not in use.
   * In fact, forward subsumption by FPA has simply not been used
   * in the recent past, so this assumption is not a limiting one.
   */

  if (Flags[FOR_SUB_FPA].val == 0) {  /* if `is' indexing */
    /*	d_lit = d; *//* this setting allows the code in "forward_subsume" */
    d_atom = d;  /* this setting allows the code in "forward_subsume" */
    /* in "clause.c" to be copied in here.  */

    /* Is_pos_lits and Is_neg_lits are global variables */
    is_db = sign ? Is_pos_lits : Is_neg_lits;
    c_atom = fs_retrieve(d_atom, s, is_db, &pos);
    /* &pos is position returned for subsequent calls. */
    while (c_atom != NULL && subsumed == 0) {
      c = c_atom->occ.lit->container;
      /* want to get the clause that contains "c_atom". */

      /* if c is a unit, in the right list (usable or sos or either),
       * then set subsumed to 1.
       */

      if (num_literals(c) == 1 &&
	  (   (c->container == Usable
	       && Flags[LINKED_SUB_UNIT_USABLE].val == 1)
	      || (c->container == Sos
		  && Flags[LINKED_SUB_UNIT_SOS].val == 1)))
	subsumed = 1;

      if (subsumed == 0)
	c_atom = fs_retrieve((struct term *) NULL, s, is_db, &pos);
      else {
	/*  clear_subst_1(tr); ** Apr 12 90 SKW */
	/*  ("tr" is not being used in this simple application
	**  of subsumption testing for unit clauses.
	**  We don't need to do additional unifies on additional
	**  literals.  Therefore there is no need to keep a trail
	**  (tr) of what we did.
	**  Therefore we never initialized "tr" to NIL.
	**  Therefore we can't pass "tr" to clear_subst_1,
	**  and this is all right because we don't need to clear.
	*/
	canc_fs_pos(pos, s);
      }
    }

  }
  else
    printf("FPA indexing not supported for linked res subsumable unit");
  /* endif */
  free_context(s);

  /* BEGIN DEBUG SKW
  ** if (subsumed) {
  **      printf("exiting subsumable_unit, subsuming clause c follows\n");
  **      print_clause(stdout,c);  printf("\n");
  **      }
  **  else
  **      printf("exiting subsumable_unit, not subsumed\n");
  ** END DEBUG SKW */

  if (subsumed)
    return(c);
  else
    return(NULL);
}  /* end of subsumable_unit */

/*********************************************************************
 *
 *    struct clause *linked_unit_del(sign,d)
 *
 *    Attempt to find a unit clause that resolves the (unsigned) literal
 *    (atom) d with sign "sign", without instantiating "d".
 *
 *    SKW - Apr 11 90 - This entire routine added to do unit deletion
 *    checking in linked resolution.
 *
 *    This routine will not update the link_node data structure.
 *    This routine simply returns a pointer to a clause with sign opposite
 *    to the passed "sign".  The clause can be used to unit-delete "d".
 *
 *    If no unit deletion clause is found, a NULL pointer is returned.
 *
 *    A literal L can be unit deleted if and only if -L
 *    (L with sign reversed) is subsumed by a unit clause C.
 *    If such a clause C is found, it can be used to resolve off L without
 *    instantiating L.
 *    Hence the use of the forward subsumption routine "fs_retrieve".
 *    The subsuming unit clause C, or "NULL" if none, is returned.
 *
 *    Essentially the same subsumption logic is used as in "subsumable_unit".
 *
 **********************************************************************/

static struct clause *linked_unit_del(char sign,
				      struct term *d)
{
  int subsumed;		/* flag set to 1 if negation of
			 * literal is subsumed */
  BOOLEAN print_ud_trace;	/* Hard-code this to TRUE to print */
                                /* linked unit deletion trace msgs */
  struct clause *c = NULL;	/* c is a ptr to a (possible) subsuming cl */

  struct term *c_atom, *d_atom;    /* c_atom and d_atom are ptrs to     */
  /* atoms (unsigned literals) for     */
  /* clause c and literal d resp.      */
  struct context *s;		/* a context is a substitution   */

  struct is_tree *is_db;	/* This is the index that we use.         */
				/* This will be set either to 		 */
				/* Is_pos_lits (global variable indicating */
				/* index for positive literals) or to      */
				/* Is_neg_lits (index for neg literals).    */

  struct fsub_pos *pos;	/* Maintains a position in the sequence of */
				/* subsuming literals.  fsub position.     */
				/* Maintained from one call to the next of */
				/* the indexing routine.		*/

  print_ud_trace = FALSE;	/* Turn off trace */

  /* BEGIN DEBUG SKW print out passed term (and possibly sign) */
  if (print_ud_trace)
    {
      printf("UD entering linked_unit_del, passed term follows\n");
      printf("UD ");
      print_term(stdout,d);  printf("\n");
    } /* endif */
  /* END DEBUG SKW */


  subsumed = 0;		/* initially flag says not subsumed */
  s = get_context();		/* s is initially a null context, void subst */

  /* The following code checks for subsumption
   * of the negation of the passed literal "d"
   * by a unit clause in any list.
   */

  /* The following code for unit deletion assumes that the option
   * "clear(for_sub_fpa)" is in effect, that is, that forward subsumption
   * by FPA is not in use.
   * In fact, forward subsumption by FPA has simply not been used
   * in the recent past, so this assumption is not a limiting one.
   */

  if (Flags[FOR_SUB_FPA].val == 0) {  /* if `is' indexing */
    d_atom = d;  /* this setting allows the code in "forward_subsume" */
    /* in "clause.c" to be copied in here.  */

    /* Is_pos_lits and Is_neg_lits are global variables */
    /* that point to indexes for positive literals and  */
    /* negative literals respectively.  */
    is_db = sign ? Is_neg_lits : Is_pos_lits ;
    /* Note that if "sign" is positive, use NEGATIVE literal index,
     * if "sign" is negative, use POSITIVE literal index,
     * So that in fact a subsumer of the NEGATION
     * of the passed literal is sought.
     */
    c_atom = fs_retrieve(d_atom, s, is_db, &pos);
    /* &pos is position returned for subsequent calls. */
    while (c_atom != NULL && subsumed == 0) {
      c = c_atom->occ.lit->container;
      /* want to get the clause that contains "c_atom". */

      /* if c is a unit,
       * then set subsumed to 1.
       */

      if (num_literals(c) == 1)
	subsumed = 1;

      if (subsumed == 0)
	c_atom = fs_retrieve((struct term *) NULL, s, is_db, &pos);
      else {
	/*  No need to  clear_subst_1(tr); ** Apr 12 90 SKW */
	/*  ("tr" is not being used in this simple application
	**  of subsumption testing for unit clauses.
	**  We don't need to do additional unifies on additional
	**  literals.  Therefore there is no need to keep a trail
	**  (tr) of what we did.
	**  Therefore we never initialized "tr" to NIL.
	**  Therefore we can't pass "tr" to clear_subst_1,
	**  and this is all right because we don't need to clear.)
	*/
	canc_fs_pos(pos, s);
      }
    }

  }
  else
    printf("FPA indexing not supported for linked res linked_unit_del");
  /* endif */
  free_context(s);

  /* BEGIN DEBUG SKW */
  if (print_ud_trace)
    {
      if (subsumed)
	{
	  printf("UD exiting linked_unit_del, subsuming clause c follows\n");
	  printf("UD ");
	  print_clause(stdout,c);  printf("\n");
	} /* endif */
      else
	printf("UD exiting linked_unit_del, not subsumed\n");
    } /* endif */
  /* END DEBUG SKW */

  if (subsumed)
    {
      Stats[UNIT_DELETES]++;
      return(c);
    }
  else
    return(NULL);
}  /* end of linked_unit_del */

/************************************************************
 *
 * struct link_node *backward(tree, node, target, nopen, nres)
 * struct link_node **tree, *node, **target;
 * int *nopen, *nres;
 *
 ************************************************************/

static struct link_node *backward(struct link_node **tree,
				  struct link_node *node,
				  struct link_node **target,
				  int *nopen,
				  int *nres)
{

  struct link_node *p, *start, *temp_p;
  BOOLEAN left_most, target_here;

  if (node->first_child)
    {
      printf("attempted to back up from a node with a child\n");
      exit(ABEND_EXIT);
    } /* endif */

  clear_subst_1(node->tr);

  node->first = TRUE;
  node->current_clause = NULL;
  /* At the time that you back up away from this node,
   * all information as to how the node's literal was instantiated,
   * and all information as to how this literal was resolved,
   * must be reset to NULL, unknown, etc.
   * In particular, unit deletion of the node no longer holds good.
   */
  node->unit_deleted = FALSE;
  if (node->goal_to_resolve)
    {
      zap_term(node->goal_to_resolve);
      /* SKW This is where the instantiated goal_to_resolve */
      /* is zapped (deleted from memory) when the backtracking process */
      /* is done with it. */
      node->goal_to_resolve = NULL;
    } /* endif */

  if (node->prev_sibling)
    {
      if (node->prev_sibling == *target)
	{
	  if ((*target)->prev_sibling)
	    {
	      left_most = FALSE;
	      start = node;
	    }
	  else
	    {
	      left_most = TRUE;
	      start = *target;
	    } /* endif */
	}
      else
	{
	  left_most = FALSE;
	  start = node;
	} /* endif */
    }
  else
    {
      left_most = TRUE;
      start = node;
    } /* endif */

  if (!left_most)
    {
      /* node has left sibling */
      node->farthest_sat = (node->parent)->farthest_sat + 1;
      p = start->prev_sibling;
      if (p == *target)
	p = p->prev_sibling;
      while (p->first_child)
	{
	  p = p->first_child;
	  /* loop to get the rightmost sibling at this level */
	  while (p->next_sibling)
	    p = p->next_sibling;
	} /* endwhile */
      if (p == *target)
	/* backed up to the target ... must back up again */
	p = backward(tree, p, target, nopen, nres);
      else
	{
	  *nopen = *nopen + 1;
	  *nres = *nres - 1;
	  p->current_clause = NULL;
	} /* endif */
    }
  else
    {
      /* node is leftmost sibling ... if the target clause is in here */
      /* an attempt must be made to re-position the target.  if the   */
      /* target cannot be re-postioned and this is NOT a BOTH clause  */
      /* it is trashed.  if the target is not in this clause then it  */
      /* is trashed.                                                  */
      if (keep_clause(start, target, &target_here, nopen))
	p = start;
      else
	{
	  /* node is leftmost sibling ... must     */
	  /* remove himself and all his siblings.  */
	  /* this simply means returning the nodes */
	  /* as the clear_subst have been done all */
	  /* along.                                */
	  p = start->parent;
	  /* removing nodes of deleted clause from open lit count */
	  if (target_here)
	    *nopen = *nopen - (num_literals(p->current_clause) - 2);
	  else
	    *nopen = *nopen - (num_literals(p->current_clause) - 1);
	  while (p->first_child)
	    {
	      temp_p = p->first_child;
	      p->first_child = temp_p->next_sibling;
	      free_context(temp_p->subst);
	      free_link_node(temp_p);
	    } /* endwhile */
	  if (p == *tree)
	    {
	      /* have removed all the nodes of the tree */
	      free_context(p->subst);
	      free_link_node(p);
	      p = NULL;
	      *tree = NULL;
	      *nopen = *nopen - 1;
	    }
	  else
	    {
	      /* p now points to a newly acquired open node */
	      *nres = *nres - 1;
	      *nopen = *nopen + 1;
	      p->current_clause = NULL;
	    } /* endif */
	} /* endif */
    } /* endif */

  return p;

} /* end backward() */

/*************
 *
 *    linked_ur_res(giv_cl)
 *
 *************/

void linked_ur_res(struct clause *giv_cl)
{

  struct link_node *inf_tree, *curr_node, *target, *lnp;
  struct term *tp, *temp_term;
  int hit_ur_limit_count, open_lit_count, res_count, i;
  int hit_ur_ded_count;
  struct clause *resolvent, *unit_cl;
  BOOLEAN target_here;
  BOOLEAN subsumable_unit_here;

  tp = NULL;   /* to quite -Wall */

  /* subsumable_unit_here is TRUE if the current
   * literal "curr_node->goal_to_resolve"
   * has been found to be a subsumable unit.
   * Otherwise subsumable_unit_here is FALSE.
   */

  /* only process given clauses that are unit */

  if (num_literals(giv_cl) > 1)
    return;

  CLOCK_START(LINKED_UR_TIME);

  if (Flags[LINKED_UR_TRACE].val == 1)
    {
      printf("********entering Linked UR\n");
      print_linked_ur_mem_stats();
      printf("***********\n");
    } /* endif */

  if ((inf_tree = initialize_tree(giv_cl)) == NULL)
    {
      printf("ERROR: unable to initialize linked UR inference tree\n");
      exit(ABEND_EXIT);
    } /* endif */

  hit_ur_limit_count = hit_ur_ded_count = 0;

  res_count = 0;
  open_lit_count = 1;

  if (Flags[LINKED_UR_TRACE].val == 1)
    {
      printf("********tree before BIG loop open lit %d nres %d\n",
	     open_lit_count, res_count);
      linked_print_link_node_tree(inf_tree, 0);
      printf("***********\n");
    } /* endif */

  /* BIG loop to process whole tree */
  target = NULL;
  curr_node = inf_tree->first_child;
  while (curr_node)
    {
      /* assumed that curr_node is correctly positioned */

      /* Print trace information if trace flag is turned on */

      if (Flags[LINKED_UR_TRACE].val == 1)
	{
	  printf("attempting to find clash for term ");
	  if (!(((curr_node->goal)->occ.lit)->sign))
	    printf("-");
	  fflush(stdout);
	  print_term(stdout, curr_node->goal);
	  fflush(stdout);
	  printf(" in clause ");
	  fflush(stdout);
	  linked_print_clause(((curr_node->goal)->occ.lit)->container);
	  printf(" first time = %c res_count = %d\n",
		 (curr_node->first ? 'T' : 'F'), res_count);
	} /* endif */

      /* Modifications by SKW Apr 10 90 and May 15 90 follow */

      subsumable_unit_here = FALSE;

      /* If the current node's literal has not been unit-deleted away,
       * and the various parameters say OK to continue linking,
       * find a first way or an additional way to resolve
       * this node's literal.
       */

      if ( ! (curr_node->unit_deleted)
	   &&
	   pass_parms_check(curr_node, res_count, open_lit_count,
			    &hit_ur_limit_count, &hit_ur_ded_count, target))
	{
	  /* Apply:
	  ** Do the apply here so that goal_to_resolve will be correctly
	  ** set to the instantiated form of the current (unsigned) literal
	  ** before "subsumable_unit" (which uses this instantiated term)
	  ** is called.
	  */

	  if (curr_node->first)
	    {
	      curr_node->goal_to_resolve =
		apply(curr_node->goal, (curr_node->parent)->subst);
	      curr_node->unit_deleted = FALSE;
	    } /* endif */

	  /* Subsumable Unit Test:
	  ** If this is the first time that this node of the linked
	  ** inference tree is being examined (after all, we only need
	  ** to check once whether the unit is subsumable)
	  ** and no target has been chosen yet
	  ** (subsumable unit is only applicable if there is just one
	  ** open (unresolved) literal and it is NOT the target)
	  ** and, finally, there is indeed exactly one open literal,
	  ** then perform the subsumable unit test.
	  */

	  if (curr_node->first && !target && open_lit_count == 1
	      && res_count >= 1)
	    {

	      /* SKW DEBUG BEGIN - SHOW WHAT res_count IS */
	      /* printf("ready to test for calling subsumable_unit\n"); */
	      /* printf("res_count = %d\n",res_count); */
	      /* SKW DEBUG END */

	      if ((Flags[LINKED_SUB_UNIT_USABLE].val == 1 ||
		   Flags[LINKED_SUB_UNIT_SOS].val == 1) &&
		  subsumable_unit(curr_node->goal->occ.lit->sign,
				  curr_node->goal_to_resolve))
		{
		  subsumable_unit_here = TRUE;
		  tp = NULL;

		  /* If the unit is indeed subsumable, set tp = NULL
		  ** which essentially means a "FAIL" has occurred
		  ** at this node, that is no linked resolvent can be
		  ** completed when this node is involved.
		  **
		  ** The sign of the literal is found by tracing the chain
		  ** of pointers "curr_node->goal->occ.lit->sign".
		  ** Here "goal" points to the UNINSTANTIATED and unsigned
		  ** form of the literal to be resolved.
		  ** The pointer "goal_to_resolve" points to the
		  ** INSTANTIATED unsigned form of the literal to be
		  ** resolved.  That is, "goal_to_resolve" is
		  ** "goal"/(current unifying substitution so far).
		  */
		}
	    }

	  /* The following nested "if" statement has the following effect.
	   * If a resolving clause can be found, point "tp" to
	   * the resolving unsigned literal (struct term) in that clause.
	   * Otherwise, set "tp" to NULL.
	   */

	  if ( ! subsumable_unit_here)
	    {
	      /* goal_to_resolve is not subsumable */
	      /* So, test for unit deletion of goal_to_resolve */
	      /* Note that we only test for unit deletion if at least
	       * one resolution has occurred.
	       * That is, we don't test the unit "given clause" for
	       * unit deletion.
	       */
	      if ((Flags[LINKED_UNIT_DEL].val) && (res_count >= 1)
		  &&
		  (unit_cl = linked_unit_del(curr_node->goal->occ.lit->sign,
					     curr_node->goal_to_resolve)))
		{
		  /* unit deletion succeeded */

		  curr_node->unit_deleted = TRUE;
		  tp = unit_cl->first_lit->atom;
		  target_here = FALSE;
		  /* new resolving clause is a unit, and so
		   *  does not contain the target
		   */
		}
	      else
		tp = generate_resolvent(curr_node, target,
					&target_here, &hit_ur_limit_count);
	    } /* endif */
	} /* endif */
      else
	/* did not pass parms check */
	tp = NULL;

      /* At this point, "tp" is either a pointer to a clause, that will
       * resolve the literal "curr_node->goal_to_resolve",
       * or is "NULL" indicating that no resolving clause could be found.
       *
       * If "curr_node->unit_deleted" is TRUE, then the resolving clause
       * is a unit, the resolution is a "unit deletion",
       * and the resolution does not instantiate the literal
       * "curr_node->goal_to_resolve".
       * In this case there is no need to try any additional ways of
       * resolving the literal.
       *
       * Otherwise ("curr_node->unit_deleted" is FALSE),
       * other possible resolutions will need to be tried.
       */

      if (tp)
	{
	  /* found (another) resolvent */

	  if (Flags[LINKED_UR_TRACE].val == 1)
	    {
	      printf("clashed against term: ");
	      if (!((tp->occ.lit)->sign))
		printf("-");
	      print_term(stdout, tp);
	      printf("  in clause  ");
	      linked_print_clause((tp->occ.lit)->container);
	      printf("\n");
	    } /* endif */

	  /* this resolution may be added to the inference tree */
	  res_count ++;
	  open_lit_count --;
	  curr_node->current_clause = (tp->occ.lit)->container;
	  i = num_literals(curr_node->current_clause);
	  if (target_here)
	    open_lit_count = open_lit_count + (i - 2);
	  else
	    open_lit_count = open_lit_count + (i - 1);
	  if (i == 1)
	    /* new clause is UNIT */
	    curr_node->first_child = NULL;
	  else
	    {
	      /* new clause is non-UNIT */
	      construct_children_nodes(curr_node, tp,
				       &target, target_here);
	    } /* endif */
	  if ((lnp = forward(curr_node, target)) != NULL)
	    {
	      /* was able to move forward */
	      if (!(lnp->first))
		{
		  printf("ERROR: moved forward to %p first = FALSE\n", (void *) lnp);
		  exit(ABEND_EXIT);
		} /* endif */
	      curr_node = lnp;
	      /* was able to move forward to select new goal */
	      /* must check that new goal is not in ancestry */
	      if (is_in_ancestry(curr_node, inf_tree))
		{
		  /* literal in curr_node is in ancestry */
		  /* attempting to resolve it away would */
		  /* start a loop.  must back up.        */

		  if (Flags[LINKED_UR_TRACE].val == 1)
		    {
		      printf("did not attempt to resolve ");
		      if (!(((curr_node->goal)->occ.lit)->sign))
			printf("-");
		      fflush(stdout);
		      print_term(stdout, curr_node->goal);
		      printf(" ... appears in ancestry\n");
		    } /* endif */

		  curr_node = backward(&inf_tree, curr_node, &target,
				       &open_lit_count, &res_count);
		}
	      else
		{
		  /* literal in curr_node is not in ancestry  */
		  /* and may be chosen as the next literal    */
		  /* to be resolved away.                     */
		  /* have moved forward and there's no target */
		  /* this node's farthest_sat must be         */
		  /* re-calculated as MAX(his farthest_sat,   */
		  /* all left siblings' back_up distances)    */
		  i = curr_node->farthest_sat;
		  for (lnp = (curr_node->parent)->first_child;
		       lnp && lnp != curr_node;
		       lnp = lnp->next_sibling)
		    {
		      if (lnp->back_up == UNDEFINED && lnp != target)
			{
			  printf("ERROR: found non_target back_up < 0\n");
			  exit(ABEND_EXIT);
			}
		      else
			{
			  if (i < lnp->back_up)
			    i = lnp->back_up;
			} /* endif */
		    } /* endfor */
		  if (!lnp)
		    {
		      printf("ERROR: left_sibling chain to NULL\n");
		      exit(ABEND_EXIT);
		    } /* endif */
		  curr_node->farthest_sat = i;
		} /* endif */
	    }
	  else
	    {
	      /* resolved away whole tree */

	      if (Flags[LINKED_UR_TRACE].val == 1)
		{
		  printf("***RESOLVED AWAY TREE curr_node at %p target at %p open lit %d nres %d\n",
			 (void *) curr_node, (void *) target, open_lit_count, res_count);
		  linked_print_link_node_tree(inf_tree, 0);
		  printf("***********\n");
		  printf("resolvent =  ");
		  fflush(stdout);
		  if (target)
		    {
		      if (!(((target->goal)->occ.lit)->sign))
			printf("-");
		      fflush(stdout);
		      temp_term = apply(target->goal,
					(target->parent)->subst);
		      print_term(stdout, temp_term);
		      printf("\n");
		      zap_term(temp_term);
		    }
		  else
		    printf("[]\n");
		} /* endif */

	      resolvent = build_ur_resolvent(target,
					     inf_tree->first_child);
	      Stats[CL_GENERATED]++;
	      Stats[LINKED_UR_RES_GEN]++;
	      CLOCK_STOP(LINKED_UR_TIME);
	      pre_process(resolvent, 0, Sos);
	      CLOCK_START(LINKED_UR_TIME);

	      curr_node = forward_from_resolved_tree(curr_node,
						     &target, &res_count, &open_lit_count);
	    } /* endif */
	}
      else
	{
	  /* couldn't find anything (else) to unify */
	  /* or failed parms check                  */

	  if (Flags[LINKED_UR_TRACE].val == 1)
	    printf("-- FAIL --\n");

	  /* must back up from curr_node ... when */
	  /* trying this node again it will be    */
	  /* the first time.                      */
	  curr_node = backward(&inf_tree, curr_node, &target,
			       &open_lit_count, &res_count);
	} /* endif */

      if (Flags[LINKED_UR_TRACE].val == 1)
	{
	  printf("***tree end BIG loop curr_node %p tar %p open lit %d\n",
		 (void *) curr_node, (void *) target, open_lit_count);
	  linked_print_link_node_tree(inf_tree, 0);
	  printf("***********\n");
	} /* endif */

    } /* endwhile */

  if (Flags[LINKED_UR_TRACE].val == 1)
    {
      printf("********tree leaving linked UR open lit %d nres %d\n",
	     open_lit_count, res_count);
      linked_print_link_node_tree(inf_tree, 0);
      printf("***********\n");
      print_linked_ur_mem_stats();
      printf("***********\n");
    } /* endif */

  if (hit_ur_limit_count)
    printf("** HIT maximum linked UR depth %d times\n", hit_ur_limit_count);

  if (hit_ur_ded_count)
    printf("** HIT maximum linked UR deduction size %d times\n",
	   hit_ur_ded_count);

  Stats[LINKED_UR_DEPTH_HITS] += hit_ur_limit_count;
  Stats[LINKED_UR_DED_HITS] += hit_ur_ded_count;

  CLOCK_STOP(LINKED_UR_TIME);

}  /* end linked_ur_res() */

/************************************************************
 *
 * struct ilist *build_parental_chain(node, target)
 * struct link_node *node, *target;
 *
 ************************************************************/

static struct ilist *build_parental_chain(struct link_node *node,
					    struct link_node *target)
{

  struct ilist *chain_head, *last_one, *tail, *unit_del_flag_node;
  struct link_node *lnp;

  chain_head = get_ilist();
  chain_head->i = (((node->goal)->occ.lit)->container)->id;
  chain_head->next = NULL;
  last_one = chain_head;

  for (lnp = node; lnp; lnp = lnp->next_sibling)
    {
      if (lnp->first_child)
	tail = build_parental_chain(lnp->first_child, target);
      else
	{
	  if (lnp != target)
	    {
	      tail = get_ilist();
	      tail->i = (lnp->current_clause)->id;
	      if (lnp->unit_deleted)
		{
		  unit_del_flag_node = get_ilist();
		  unit_del_flag_node->i = UNIT_DEL_RULE;
		  unit_del_flag_node->next = tail;
		  tail = unit_del_flag_node;
		}
	    }
	  else
	    tail = NULL;
	} /* endif */
      last_one->next = tail;
      while (last_one->next)
	last_one = last_one->next;
    } /* endfor */

  return chain_head;

} /* end build_parental_chain() */

/************************************************************
 *
 * struct clause *build_ur_resolvent(target, tree)
 * struct link_node *target, *tree;
 *
 ************************************************************/

static struct clause *build_ur_resolvent(struct link_node *target,
					 struct link_node *tree)
{

  struct clause *resolvent;
  struct literal *from_lit, *to_lit;

  resolvent = get_clause();

  if (target)
    {
      /* linked ur generated a unit clause */
      resolvent->first_lit = to_lit = get_literal();
      from_lit = (target->goal)->occ.lit;
      to_lit->container = resolvent;
      to_lit->sign = from_lit->sign;
      to_lit->atom = apply(target->goal, (target->parent)->subst);
      (to_lit->atom)->occ.lit = to_lit;
      (to_lit->atom)->varnum = (from_lit->atom)->varnum;
    }
  else
    /* linked ur generated the empty clause */
    resolvent->first_lit = NULL;

  resolvent->parents = get_ilist();
  (resolvent->parents)->i = LINKED_UR_RES_RULE;
  (resolvent->parents)->next = build_parental_chain(tree, target);

  return resolvent;

} /* end build_ur_resolvent() */

/************************************************************
 *
 * BOOLEAN check_down_tree(node, my_depth)
 * struct link_node *node;
 * int my_depth;
 *
 * This function assumes the target rests above node in the tree
 * at a distance of my_depth.
 *
 * if (node)
 *    if (my_depth is OK)
 *       for (each of my left siblings SIB && still OK)
 *           OK = check_down_tree(SIB->first_child, my_depth+1)
 *       endfor
 *    else
 *       OK = FALSE
 *    endif
 * else
 *    if (mydepth is OK)
 *       OK = TRUE
 *    else
 *       OK = FALSE
 *    endif
 * endif
 *
 * return OK
 *
 ************************************************************/

static BOOLEAN check_down_tree(struct link_node *node,
			       int my_depth)
{

  BOOLEAN rc;
  struct link_node *lnp;

  if (my_depth <= Parms[MAX_UR_DEPTH].val)
    {
      rc = TRUE;
      if (node)
	{
	  /* I am not a SATELLITE with no link node */
	  for (lnp = node; rc && lnp; lnp = lnp->next_sibling)
	    rc = check_down_tree(lnp->first_child, my_depth+1);
	} /* endif */
    }
  else
    rc = FALSE;

  return rc;

} /* end check_down_tree */

/************************************************************
 *
 * BOOLEAN check_up_tree(node, my_depth)
 * struct link_node *node;
 * int my_depth;
 *
 * This function assumes the target has been brought into node
 * from below at a distance of my_depth.
 *
 * if (i am not the dummy node)
 *    if (my_depth is OK)
 *       OK = TRUE
 *       for (each of my siblings to the left && still OK)
 *           OK = check the depth of their children (my_depth+1)
 *       endfor
 *       if (still OK)
 *          OK = check_up_tree(leftmost sibling parent, my_depth+1)
 *       endif
 *    else
 *       OK = FALSE
 *    endif
 * else
 *    OK = TRUE
 * endif
 *
 * return OK
 *
 ************************************************************/

static BOOLEAN check_up_tree(struct link_node *node,
			     int my_depth)
{

  BOOLEAN rc;
  struct link_node *lnp;

  if (node->parent)
    {
      /* I am NOT the dummy node at the top of the inference tree */
      if (my_depth <= Parms[MAX_UR_DEPTH].val)
	{
	  /* this level clause is OK ... must check */
	  /* all the children of the siblings to my */
	  /* left (the ones to the right have not   */
	  /* been processed yet) then go UP on my   */
	  /* leftmost sibling.                      */
	  rc = TRUE;
	  lnp = node;
	  while (rc && lnp->prev_sibling)
	    {
	      lnp = lnp->prev_sibling;
	      rc = check_down_tree(lnp->first_child, my_depth+1);
	    } /* endwhile */
	  if (rc)
	    rc = check_up_tree(lnp->parent, my_depth+1);
	}
      else
	rc = FALSE;
    }
  else
    /* I am the dummy node at the top of the inference tree */
    rc = TRUE;

  return rc;

} /* end check_up_tree() */

/*************
 *
 * struct term *first_unifiable(t,index,subst_t,subst_ret,pos_ptr,tr_ptr,
 *                              curr_node, target, target_here)
 * struct term *t;
 * struct fpa_head **index;
 * struct context *subst_t, *subst_ret;
 * struct fpa_tree **pos_ptr;
 * struct trail **tr_ptr;
 * struct link_node *curr_node, *target;
 * BOOLEAN *target_here;
 *
 * This function finds the first term that can resolve away the literal
 * pointed at by curr_node.
 *
 *************/

static struct term *first_unifiable(struct term *t,
				    struct fpa_index *index,
				    struct context *subst_t,
				    struct context *subst_ret,
				    struct fpa_tree **pos_ptr,
				    struct trail **tr_ptr,
				    struct link_node *curr_node,
				    struct link_node *target,
				    char *target_here,
				    int *hit_dp_count)
{

  int var_nums[MAX_VARS], i;

  /*    curr_node->goal_to_resolve = apply(t, subst_t); ** SKW Apr 12 90 */

  for (i = 0; i < MAX_VARS; i ++)
    var_nums[i] = -1;

  if (!(renum_vars_term(curr_node->goal_to_resolve, var_nums)))
    {
      printf("unable to renumber vas in fist_unifiable()\n");
      exit(ABEND_EXIT);
    } /* endif */

  *pos_ptr = build_tree(curr_node->goal_to_resolve, UNIFY,
			Parms[FPA_LITERALS].val, index);

  return(next_unifiable(t, subst_t, subst_ret, pos_ptr, tr_ptr,
			curr_node, target, target_here, hit_dp_count));

}  /* first_unifiable */

/************************************************************
 *
 * struct link_node *forward(cn, target)
 * struct link_node *cn, *target;
 *
 ************************************************************/

static struct link_node *forward(struct link_node *cn,
				 struct link_node *target)
{

  struct link_node *p, *lnp;
  BOOLEAN done;
  int max_back_up;

  done = FALSE;
  p = NULL;  /* to quite -Wall */

  if (cn->first_child)
    {
      /* has a first child */
      if (cn->first_child != target)
	{
	  /* first child is not the target */
	  p = cn->first_child;
	  done = TRUE;
	}
      else
	{
	  /* has first child that is the target */
	  if ((cn->first_child)->next_sibling)
	    {
	      /* target has a next_sibling */
	      p = (cn->first_child)->next_sibling;
	      done = TRUE;
	    } /* endif */
	} /* endif */
    } /* endif */

  if (!done)
    {
      /* cn picked off by SATELLITE */
      cn->back_up = 0;
      if (cn->next_sibling)
	{
	  /* has a next sibling */
	  if (cn->next_sibling != target)
	    {
	      /* the next_sibling is not the target */
	      p = cn->next_sibling;
	      done = TRUE;
	    }
	  else
	    {
	      /* the next_sibling is the target */
	      if ((cn->next_sibling)->next_sibling)
		{
		  /* the target has a next_sibling */
		  p = (cn->next_sibling)->next_sibling;
		  done = TRUE;
		} /* endif */
	    } /* endif */
	} /* endif */
    } /* endif */

  if (!done)
    {
      /* time to go back up in the tree */
      /* must traverse up tree until I find a parent that has */
      /* a sibling to the right that is NOT the target        */
      p = cn->parent;
      while (!done && p)
	{
	  /* must find the longest back_up value from */
	  /* all my children to establish my back_up  */
	  for (max_back_up = -1, lnp = p->first_child;
	       lnp; lnp = lnp->next_sibling)
	    if (max_back_up < lnp->back_up)
	      max_back_up = lnp->back_up;
	  p->back_up = max_back_up + 1;
	  if (p->next_sibling)
	    {
	      /* has a next_sibling */
	      if (p->next_sibling != target)
		{
		  /* the next sibling is NOT the target */
		  p = p->next_sibling;
		  done = TRUE;
		}
	      else
		{
		  /* the next_sibling is the target */
		  if ((p->next_sibling)->next_sibling)
		    {
		      /* the target has a next_sibling */
		      p = (p->next_sibling)->next_sibling;
		      done = TRUE;
		    }
		  else
		    {
		      /* the target does not have a next_sibling */
		      p = p->parent;
		    } /* endif */
		} /* endif */
	    }
	  else
	    p = p->parent;
	} /* endwhile */
    } /* endif */


  return p;

} /* end forward() */

/************************************************************
 *
 * struct link_node *forward_from_resolved_tree(curr_node, target, nres, nopen)
 * struct link_node *curr_node, **target;
 * int *nres, *nopen;
 *
 * The inference tree has been resolved away and curr_node points
 * to the last node that was resolved away.  There is a special case
 * that must be considered.  It is in fact possible for curr_node
 * to have a first_child.  This occurs when the target is in a
 * two literal clause and that clause was just brought in as the
 * child of curr_node.
 *
 * When this happens, the target must be removed and one of two
 * things must occur:
 * 1) The first_child is of type BOTH.  The first child must be converted
 *    to a link.  The first_child is then chosen as the curr_node first = T.
 * 2) The first_child is of type NUCLEUS.  It must be removed from the
 *    inference tree and the curr_node becomes the curr_node first = F.
 *
 ************************************************************/

static struct link_node *forward_from_resolved_tree(struct link_node *curr_node,
						    struct link_node **target,
						    int *nres,
						    int *nopen)
{

  struct link_node *lnp;

  lnp = curr_node;

  if (lnp->first_child)
    {
      /* the current node has a child ... better be the only */
      /* sibling at that level and it better be the target   */
      if (lnp->first_child != *target)
	{
	  printf("ERROR: forward failed, has first_child that's NOT tar\n");
	  exit(ABEND_EXIT);
	}
      else
	{
	  /* this target node better not have any */
	  /* children or siblings                 */
	  if ((lnp->first_child)->next_sibling ||
	      (lnp->first_child)->first_child)
	    {
	      printf("ERROR: forward failed with target child (has sibs)\n");
	      exit(ABEND_EXIT);
	    }
	  else
	    {
	      /* the target must be cleared ... either */
	      /* tossing this nucleus or both is       */
	      /* converted from a nuclues to a link.   */
	      *target = NULL;
	      if ((lnp->current_clause)->type == BOTH)
		{
		  /* bottom node is BOTH and must */
		  /* converted from a nucleus to  */
		  /* a link.                      */
		  lnp = lnp->first_child;
		  *nopen = *nopen + 1;
		}
	      else
		{
		  /* bottom node is NUCLEUS and must be tossed */
		  free_linked_node_tree(lnp->first_child, target);
		  lnp->first_child = NULL;
		  *nres = *nres - 1;
		  *nopen = *nopen + 1;
		  lnp->current_clause = NULL;
		} /* endif */
	    } /* endif */
	} /* endif */
    }
  else
    {
      /* curr_node has no children */
      *nres = *nres - 1;
      *nopen = *nopen + 1;
      lnp->current_clause = NULL;
    } /* endif */

  return lnp;

} /* end forward_from_resolved_tree() */

/************************************************************
 *
 * void free_linked_node_tree(tree, target)
 * struct link_node *tree, **target;
 *
 ************************************************************/

static void free_linked_node_tree(struct link_node *tree,
				  struct link_node **target)
{

  if (tree)
    {
      free_linked_node_tree(tree->next_sibling, target);
      free_linked_node_tree(tree->first_child, target);
      if (tree == *target)
	*target = NULL;
      if (tree->subst)
	free_context(tree->subst);
      free_link_node(tree);
    } /* endif */

} /* end free_linked_node_tree() */

/************************************************************
 *
 * struct term *generate_resolvent(curr_node, target, target_here)
 * struct link_node *curr_node, *target;
 * BOOLEAN *target_here;
 *
 * This function attempts to find the next term that can resolve against
 * the goal term in the node pointed at by curr_node.  If there are
 * no more terms that resolve against this given term, the NULL pointer
 * is returned.
 *
 ************************************************************/

static struct term *generate_resolvent(struct link_node *curr_node,
				       struct link_node *target,
				       char *target_here,
				       int *hit_dp_count)
{

  struct term *tp;
  struct fpa_index *db;

  if ((((curr_node->goal)->occ.lit)->sign))
    db = Fpa_clash_neg_lits;
  else
    db = Fpa_clash_pos_lits;

  if (curr_node->first)
    {
      curr_node->first = FALSE;
      tp = first_unifiable(curr_node->goal, db, (curr_node->parent)->subst,
			   curr_node->subst, &(curr_node->unif_position),
			   &(curr_node->tr), curr_node, target, target_here,
			   hit_dp_count);
    }
  else
    {
      clear_subst_1(curr_node->tr);
      tp = next_unifiable(curr_node->goal, (curr_node->parent)->subst,
			  curr_node->subst, &(curr_node->unif_position),
			  &(curr_node->tr), curr_node, target, target_here,
			  hit_dp_count);
    } /* endif */

  return tp;

} /* end generate_resolvent() */

/************************************************************
 *
 * struct link_node *initialize_tree(giv_cl)
 * struct clause *giv_cl;
 *
 ************************************************************/

static struct link_node *initialize_tree(struct clause *giv_cl)
{

  struct link_node *tree, *given, *dummy_target;

  dummy_target = NULL;

  if ((tree = get_link_node()) != NULL)
    {
      /* initializing top dummy node */
      tree->parent = NULL;
      tree->next_sibling = NULL;
      tree->prev_sibling = NULL;
      tree->near_poss_nuc = UNDEFINED;
      tree->farthest_sat = -2;
      tree->target_dist = 0;
      tree->back_up = UNDEFINED;
      tree->goal = NULL;
      tree->current_clause = giv_cl;
      tree->tr = NULL;
      tree->first = TRUE;
      tree->unif_position = NULL;
      if ((tree->subst = get_context()) != NULL)
	(tree->subst)->multiplier = tree->subst->built_in_multiplier;
      else
	{
	  printf("ERROR: couldn't get context for dummy node\n");
	  free_linked_node_tree(tree, &dummy_target);
	  tree = NULL;
	} /* endif */
    }
  else
    printf("ERROR: couldn't get link node for dummy\n");

  if (tree)
    {
      /* getting node that represents given clause */
      if ((given = get_link_node()) != NULL)
	{
	  /* initializing node that represents given clause */
	  given->parent = tree;
	  tree->first_child = given;
	  given->next_sibling = NULL;
	  given->prev_sibling = NULL;
	  given->near_poss_nuc = UNDEFINED;
	  given->farthest_sat = -1;
	  given->target_dist = 0;
	  given->back_up = UNDEFINED;
	  given->goal = (giv_cl->first_lit)->atom;
	  given->current_clause = NULL;
	  given->tr = NULL;
	  given->first = TRUE;
	  given->unif_position = NULL;
	  if ((given->subst = get_context()) != NULL)
	    (given->subst)->multiplier = given->subst->built_in_multiplier;
	  else
	    {
	      printf("ERROR: couldn't get context for given node\n");
	      free_linked_node_tree(tree, &dummy_target);
	      tree = NULL;
	    } /* endif */
	}
      else
	{
	  printf("couldn't get given link_node\n");
	  free_linked_node_tree(tree, &dummy_target);
	  tree = NULL;
	} /* endif */
    } /* endif */

  return tree;

} /* end initialize_tree() */

/************************************************************
 *
 * BOOLEAN is_in_ancestry(curr_node, inf_tree)
 * struct link_node *curr_node, *inf_tree;
 *
 ************************************************************/

static BOOLEAN is_in_ancestry(struct link_node *curr_node,
			      struct link_node *inf_tree)
{
  struct link_node *lnp;
  struct term *cand;
  struct context *cand_subst;
  BOOLEAN rc;
  char sign;

  sign = ((curr_node->goal)->occ.lit)->sign;
  cand = curr_node->goal;
  cand_subst = (curr_node->parent)->subst;

  if (Flags[LINKED_UR_TRACE].val == 1)
    {
      printf("checking is_in_ancestry on lit ");
      if (!sign)
	printf("-");
      fflush(stdout);
      print_term(stdout, cand);
      printf("\n");
    } /* endif */

  for (lnp = curr_node->parent, rc = FALSE; !rc && lnp != inf_tree;
       lnp = lnp->parent)
    rc = ( (sign == ((lnp->goal)->occ.lit)->sign)
	   &&
	   term_ident_subst(lnp->goal, (lnp->parent)->subst,
			    cand, cand_subst)
	   );

  return rc;

} /* end is_in_ancestry() */

/************************************************************
 *
 * BOOLEAN keep_clause(node, target, target_here, nopen)
 * struct link_node *node, **target;
 * BOOLEAN *target_here;
 * int *nopen;
 *
 * This function is called by backward() when the node that is
 * being backed up from is the leftmost sibling.  A decision must
 * be made whether to keep the clause represented by this node and
 * its siblings.
 *
 * if (target in this clause)
 *    if (there is another target candidate literal to the right)
 *       re-define the target as the next target literal candidate
 *       keep clause
 *    else
 *       clear target pointer (target = NULL)
 *       if (clause is of type BOTH)
 *          keep clause (BOTH from NUCLEUS to LINK)
 *       else
 *          don't keep clause
 *       endif
 *    endif
 * else
 *    don't keep clause
 * endif
 *
 ************************************************************/

static BOOLEAN keep_clause(struct link_node *node,
			   struct link_node **target,
			   char *target_here,
			   int *nopen)
{

  BOOLEAN keep, both_to_link;
  struct link_node *temp_p;

  if (*target)
    {
      /* the target has been established ... may be here */
      /* loop to determine if target is in this clause */
      for (temp_p = node;
	   temp_p && temp_p != *target;
	   temp_p = temp_p->next_sibling)
	;
      if (temp_p)
	{
	  /* target is in this clause */
	  *target_here = TRUE;
	  both_to_link = FALSE;
	  /* loop to find another target in this clause */
	  for (temp_p = temp_p->next_sibling;
	       temp_p && !(((temp_p->goal)->occ.lit)->target);
	       temp_p = temp_p->next_sibling)
	    ;
	  if (temp_p)
	    {
	      /* there is another target */
	      *target = temp_p;
	      keep = TRUE;
	    }
	  else
	    {
	      /* there are no other targets */
	      if (((((*target)->goal)->occ.lit)->container)->type == BOTH)
		{
		  /* clause is of type BOTH and may */
		  /* now be treated as  a LINK      */
		  keep = TRUE;
		  both_to_link = TRUE;
		}
	      else
		/* clause is of type NUC with no more targets */
		keep = FALSE;
	      *target = NULL;
	    } /* endif */
	  if (both_to_link)
	    *nopen = *nopen + 1;
	}
      else
	{
	  /* target is not in this clause */
	  keep = FALSE;
	  *target_here = FALSE;
	} /* endif */
    }
  else
    {
      /* the target has not been established anywhere */
      keep = FALSE;
      *target_here = FALSE;
    } /* endif */

  return keep;

} /* end keep_clause() */

/************************************************************
 *
 * void linked_print_clause(cp)
 * struct clause *cp;
 *
 ************************************************************/

static void linked_print_clause(struct clause *cp)
{

  struct literal *lp;

  printf("at %p >>", (void *) cp);
  fflush(stdout);
  switch (cp->type)
    {
    case NOT_SPECIFIED:
      printf("NOT_SPECIFIED: ");
      break;
    case NUCLEUS:
      printf("NUCLEUS: ");
      break;
    case LINK:
      printf("LINK: ");
      break;
    case BOTH:
      printf("BOTH: ");
      break;
    case SATELLITE:
      printf("SATELLITE: ");
      break;
    default:
      printf("** UNKNOWN **: ");
      break;
    } /* end switch() */
  fflush(stdout);
  for (lp = cp->first_lit; lp; lp = lp->next_lit)
    {
      if (lp->target)
	printf("**");
      fflush(stdout);
      printf("-");
      fflush(stdout);
      print_term(stdout, lp->atom);
      fflush(stdout);
      if (lp->target)
	printf("**");
      fflush(stdout);
      printf("  ");
      fflush(stdout);
    } /* endfor */
  printf("<< ");

} /* end linked_print_clause() */

/************************************************************
 *
 * void linked_print_link_node(lnp, lvl)
 * struct link_node *lnp;
 * int lvl;
 *
 ************************************************************/

static void linked_print_link_node(struct link_node *lnp,
				   int lvl)
{

  int i;

  for (i = 0; i < lvl; i ++)
    printf("   ");
  printf("-----start node at %p------\n", (void *) lnp);
  for (i = 0; i < lvl; i ++)
    printf("   ");
  printf("parent = %p prev_sibling = %p next_sibling = %p first_child = %p\n",
	 (void *) lnp->parent, (void *) lnp->prev_sibling,
	 (void *) lnp->next_sibling, (void *) lnp->first_child);
  for (i = 0; i < lvl; i ++)
    printf("   ");
  printf("current_clause >> ");
  if (lnp->current_clause)
    linked_print_clause(lnp->current_clause);
  else
    printf("(NIL)");
  printf(" <<\n");
  for (i = 0; i < lvl; i ++)
    printf("   ");
  printf("first = %c unit_deleted = %c near_poss_nuc = %d farthest_sat = %d target_dist = %d back_up = %d\n",
	 (lnp->first ? 'T' : 'F'), (lnp->unit_deleted ? 'T' : 'F'),
	 lnp->near_poss_nuc, lnp->farthest_sat,
	 lnp->target_dist, lnp->back_up);
  for (i = 0; i < lvl; i ++)
    printf("   ");
  printf("goal to resolve ");
  if (lnp->goal_to_resolve)
    {
      if (!(((lnp->goal)->occ.lit)->sign))
	printf("-");
      print_term(stdout, lnp->goal_to_resolve);
    }
  else
    printf("(NIL)");
  printf(" ... from literal ");
  if (lnp->goal)
    {
      if (!(((lnp->goal)->occ.lit)->sign))
	printf("-");
      print_term(stdout, lnp->goal);
    }
  else
    printf("(NIL)");
  printf("\n");
  if (lnp->goal)
    {
      for (i = 0; i < lvl; i ++)
	printf("   ");
      printf("from clause ");
      fflush(stdout);
      linked_print_clause(((lnp->goal)->occ.lit)->container);
      printf("\n");
    } /* endif */
  for (i = 0; i < lvl; i ++)
    printf("   ");
  printf("subst = %p subst->multiplier = %d tr = %p unif_position = %p\n",
	 (void *) lnp->subst, (lnp->subst)->multiplier,
	 (void *) lnp->tr, (void *) lnp->unif_position);
  for (i = 0; i < lvl; i ++)
    printf("   ");
  printf("-----end node at %p------\n", (void *) lnp);

} /* end linked_print_link_node() */

/************************************************************
 *
 * void linked_print_link_node_tree(lnp, lvl)
 * struct link_node *lnp;
 * int lvl;
 *
 ************************************************************/

static void linked_print_link_node_tree(struct link_node *lnp,
					int lvl)
{

  struct link_node *l;

  for (l = lnp; l; l = l->next_sibling)
    {
      linked_print_link_node(l, lvl);
      linked_print_link_node_tree(l->first_child, lvl+1);
    } /* endfor */

} /* end linked_print_link_node_tree() */

/************************************************************
 *
 * BOOLEAN more_targets_here(tp)
 * struct term *tp;
 *
 * This function returns TRUE iff there exists a target literal in
 * the clause that contains tp OTHER than the literal that
 * contains tp.
 *
 ************************************************************/

static BOOLEAN more_targets_here(struct term *tp)
{

  BOOLEAN more;
  struct literal *lp;

  for (more = FALSE, lp = ((tp->occ.lit)->container)->first_lit;
       !more && lp;
       lp = lp->next_lit)
    if (lp != (tp->occ.lit) && lp->target)
      more = TRUE;
	
  return more;

} /* end more_targets_here() */

/*************
 *
 * struct term *next_unifiable(t, subst_t, subst_ret, pos_ptr, tr_ptr,
 *                             curr_node, target, target_here)
 * struct term *t;
 * struct context *subst_t, *subst_ret;
 * struct fpa_tree **pos_ptr;
 * struct trail **tr_ptr;
 * struct link_node *curr_node, *target;
 * BOOLEAN *target_here;
 *
 * This function finds a term that can be used to resolve away the node
 * pointed at by curr_node.
 *
 *************/

static struct term *next_unifiable(struct term *t,
				   struct context *subst_t,
				   struct context *subst_ret,
				   struct fpa_tree **pos_ptr,
				   struct trail **tr_ptr,
				   struct link_node *curr_node,
				   struct link_node *target,
				   char *target_here,
				   int *hit_dp_count)
{

  struct term *term_to_return;
  int rc;
  BOOLEAN try_find_another;

  term_to_return = next_term(*pos_ptr, 0);

  *tr_ptr = NULL;

  /*
    while (term_to_return != NULL &&
    (!process_this_resolution(curr_node, target, term_to_return, target_here)
    ||
    unify(t, subst_t, term_to_return, subst_ret, tr_ptr) == 0))
    term_to_return = next_term(*pos_ptr, 0);
  */

  if (term_to_return == NULL)
    try_find_another = FALSE;
  else
    {
      if (process_this_resolution(curr_node, target,
				  term_to_return, target_here, hit_dp_count))
	{
	  rc = unify(t, subst_t, term_to_return, subst_ret, tr_ptr);
	  if (rc == 0)
	    {
	      try_find_another = TRUE;
	    }
	  else
	    {
	      try_find_another = FALSE;
	    } /* endif */
	}
      else
	try_find_another = TRUE;
    } /* endif */

  while (try_find_another)
    {
      term_to_return = next_term(*pos_ptr, 0);
      if (term_to_return == NULL)
	try_find_another = FALSE;
      else
	{
	  if (process_this_resolution(curr_node, target,
				      term_to_return, target_here, hit_dp_count))
	    {
	      rc = unify(t, subst_t, term_to_return, subst_ret, tr_ptr);
	      if (rc == 0)
		{
		  try_find_another = TRUE;
		}
	      else
		{
		  try_find_another = FALSE;
		} /* endif */
	    }
	  else
	    try_find_another = TRUE;
	} /* endif */
    } /* endwhile */

  return(term_to_return);

}  /* next_unifiable */

/************************************************************
 *
 * BOOLEAN poss_nuc_link(lnp)
 * struct link_node *lnp;
 *
 * This function tests the clause associated with the node pointed
 * at by lnp.  It tests whether this clause has the potential to
 * bring in a NUCLEUS somewhere to the right of lnp.  This is
 * TRUE if lnp has a right sibling and the farthest satellite
 * from lnp is < max link depth away.
 *
 ************************************************************/

static BOOLEAN poss_nuc_link(struct link_node *lnp)
{

  BOOLEAN rc;

  if (lnp->next_sibling)
    {
      if (lnp->farthest_sat < Parms[MAX_UR_DEPTH].val)
	rc = TRUE;
      else
	rc = FALSE;
    }
  else
    rc = FALSE;

  return rc;

} /* end poss_nuc_link() */

/************************************************************
 *
 * BOOLEAN pass_parms_check(curr_node, nres, nopen, depth_count, ded_count, tar)
 * struct link_node *curr_node;
 * int nres, nopen, *depth_count, *ded_count;
 * struct link_node tar;
 *
 * Just about to resolve away the node pointed at by curr_node.
 * Checking the parms set for linked UR deduction.  Currently checks
 * depth of tree and deduction size (number of resolutions).
 *
 ************************************************************/

static BOOLEAN pass_parms_check(struct link_node *curr_node,
				int nres,
				int nopen,
				int *depth_count,
				int *ded_count,
				struct link_node *tar)
{

  BOOLEAN ok_to_resolve;

  ok_to_resolve = TRUE;

  /* check maximum linked UR depth */
  if (tar)
    {
      /* a target has been established somewhere */
      if (curr_node->target_dist >= Parms[MAX_UR_DEPTH].val)
	{
	  ok_to_resolve = FALSE;
	  *depth_count = *depth_count + 1;

	  if (Flags[LINKED_UR_TRACE].val == 1)
	    printf("max ur depth hit # %d  ", *depth_count);
	}
    }
  else
    {
      /* a target has not been established yet */
      if (curr_node->near_poss_nuc != UNDEFINED)
	{
	  if (curr_node->near_poss_nuc >= Parms[MAX_UR_DEPTH].val)
	    {
	      ok_to_resolve = FALSE;
	      *depth_count = *depth_count + 1;

	      if (Flags[LINKED_UR_TRACE].val == 1)
		printf("max ur depth hit # %d  ", *depth_count);
	    } /* endif */
	}
      else
	{
	  if (curr_node->farthest_sat >= Parms[MAX_UR_DEPTH].val)
	    {
	      ok_to_resolve = FALSE;
	      *depth_count = *depth_count + 1;

	      if (Flags[LINKED_UR_TRACE].val == 1)
		printf("max ur depth hit # %d  ", *depth_count);
	    } /* endif */
	} /* endif */
    } /* endif */

  /* check maximum linked UR deduction size */
  if (ok_to_resolve)
    {
      if ((nres + nopen) > Parms[MAX_UR_DEDUCTION_SIZE].val)
	/* SKW DEBUG COMMENT HERE IN pass_parms_check, */
	/* SKW DEBUG COMMENT "nres + nopen" IS COMPARED TO MAX DEDUCT SIZE */
	{
	  ok_to_resolve = FALSE;
	  *ded_count = *ded_count + 1;

	  if (Flags[LINKED_UR_TRACE].val == 1)
	    printf("max ur ded size hit # %d  ", *ded_count);
	} /* endif */
    } /* endif */

  return ok_to_resolve;

} /* end pass_parms_check() */

/************************************************************
 *
 * BOOLEAN pass_target_depth_check(curr_node)
 * struct link_node *curr_node;
 *
 * A potential target has been brought in as a child of this curr_node.
 * This function returns TRUE iff bringing in this target as a child to
 * curr_node is consistent with the depth check relative to the existing
 * inference tree.
 *
 ************************************************************/

static BOOLEAN pass_target_depth_check(struct link_node *curr_node)
{

  return (curr_node->farthest_sat < Parms[MAX_UR_DEPTH].val);

  /*
    return check_up_tree(curr_node, 0);
  */

} /* end pass_target_depth_check() */

/************************************************************
 *
 * int process_linked_tags(cp)
 * struct clause *cp;
 *
 ************************************************************/

int process_linked_tags(struct clause *cp)
{

  struct literal *lp, *tag;
  struct term *tp;
  int errors, i, rc, num_lits, j;
  struct rel *r;

  if (cp->first_lit == NULL)
    return(0);

  /* first set target field to the default value */

  for (lp = cp->first_lit, num_lits = 0; lp; lp = lp->next_lit, num_lits++)
    lp->target = Flags[LINKED_TARGET_ALL].val;

  tp = cp->first_lit->atom;

  if (str_ident("$NUCLEUS", sn_to_str(tp->sym_num)))
    cp->type = NUCLEUS;
  else if (str_ident("$LINK", sn_to_str(tp->sym_num)))
    cp->type = LINK;
  else if (str_ident("$BOTH", sn_to_str(tp->sym_num)))
    cp->type = BOTH;
  else if (str_ident("$SATELLITE", sn_to_str(tp->sym_num)))
    cp->type = SATELLITE;
  else {
    cp->type = SATELLITE;
    return(0);
  }

  errors = 0;
	
  if (tp->farg == NULL || tp->farg->narg != NULL || proper_list(tp->farg->argval) == 0) {
    printf("ERROR, argument of link tag is not a list: ");
    print_term_nl(stdout, tp);
    errors++;
  }
  else {
    /* remove tag literal */
    tag = cp->first_lit;
    cp->first_lit = tag->next_lit;
    num_lits--;
    /* process tag list */
    for (r = tag->atom->farg;
	 r->argval->sym_num != Nil_sym_num;
	 r = r->argval->farg->narg) {
      rc = str_int(sn_to_str(r->argval->farg->argval->sym_num), &i);
      if (rc == 0 || i > num_lits || i < 1) {
	printf("ERROR, list member has bad literal number: %s\n",
	       sn_to_str(r->argval->farg->argval->sym_num));
	errors++;
      }
      else {
	for (lp=cp->first_lit, j = 1; j != i; lp = lp->next_lit, j++)
	  ; /* empty body */
	lp->target = (Flags[LINKED_TARGET_ALL].val ? 0 : 1);

      }
    }
    /* delete tag literal */
    tag->atom->occ.lit = NULL;
    zap_term(tag->atom);
    free_literal(tag);
  }

  return(errors);

} /* end process_linked_tags() */

/************************************************************
 *
 * BOOLEAN process_this_resolution(curr_node, target, tp, target_here)
 * struct link_node *curr_node, *target;
 * struct term *tp;
 * BOOLEAN *target_here
 *
 * The node pointed at by curr_node can be resolved away by the term
 * pointed at by tp.  This function primarily checks that constructing
 * nodes for the remaining literals in the clause that holds tp doesn't
 * violate any of the rules associated with the choice of the target ... i.e.
 * bringing in a nucleus when the target is chosen, choosing the target
 * here violates the depth check, etc.
 *
 * if (clause type is LINK)
 *    target_here = FALSE
 *    process = TRUE;
 * else
 *     if (target has been chosen)
 *        target_here = FALSE
 *        if (clause type is NUCLEUS)
 *           process = FALSE
 *        else
 *           process = TRUE
 *        endif
 *     else
 *        if (clause type is NUCLEUS)
 *           if (candidate targets left in this clause)
 *               if (pass target depth check)
 *                  target_here = TRUE
 *                  process = TRUE
 *               else
 *                  target_here = FALSE
 *                  process = FASLE
 *               endif
 *           else
 *              target_here = FALSE
 *              process = FALSE
 *           endif
 *        else
 *           process = TRUE
 *           if (candidate targets left in this clause)
 *               if (pass target depth check)
 *                  target_here = TRUE
 *               else
 *                  target_here = FALSE
 *               endif
 *           else
 *               target_here = FALSE
 *           endif
 *        endif
 *     endif
 * endif
 *
 * ------------------------------------------------------------------
 * Here is an alternate logic.  ### represents a boolean expression.
 * ### iff bringing in this clause and attempting to resolve away its
 * other literals will NOT violate the depth check nor the deduction size.
 * Very much like conducting pass_parms_check() on the clause first.
 *
 * switch (tp type)
 *    case NUCLEUS:
 *         if (!(target has been chosen)
 *               && candidate targets left in this clause
 *                  && pass target depth check)
 *            target_here = FALSE
 *            process = FALSE
 *         else
 *            target_here = FALSE
 *            process = FALSE
 *         endif
 *    case LINK:
 *         target_here = FALSE
 *         if (###)
 *            process = TRUE
 *         else
 *            process = FALSE
 *         endif
 *    case BOTH:
 *         if (!(target has been chosen) && pass target depth check)
 *            target_here = TRUE
 *            process = TRUE
 *         else
 *            target_here = FALSE
 *            if (###)
 *               process = TRUE
 *            else
 *               process = FALSE
 *            endif
 *         endif
 *    case SATELLITE:
 *         target_here = FALSE
 *         process = TRUE
 * end switch
 *
 ************************************************************/

static BOOLEAN process_this_resolution(struct link_node *curr_node,
				       struct link_node *target,
				       struct term *tp,
				       char *target_here,
				       int *hit_dp_count)
{

  BOOLEAN process, temp;

  if (((tp->occ.lit)->container)->type == LINK)
    {
      process = TRUE;
      *target_here = FALSE;
    }
  else
    {
      if (target)
	{
	  /* the target has been previously chosen */
	  *target_here = FALSE;
	  if (((tp->occ.lit)->container)->type == NUCLEUS)
	    {
	      /* clause type is NUCLEUS */
	      process = FALSE;

	      if (Flags[LINKED_UR_TRACE].val == 1)
		printf("brought NUC is TRASHED .. already have tar\n");
	    }
	  else
	    /* clause type is BOTH */
	    process = TRUE;
	}
      else
	{
	  /* the target has not been previously chosen */
	  if (((tp->occ.lit)->container)->type == NUCLEUS)
	    {
	      /* clause type is NUCLEUS */
	      if (more_targets_here(tp))
		{
		  temp = pass_target_depth_check(curr_node);
		  if (temp)
		    {
		      *target_here = TRUE;
		      process = TRUE;
		    }
		  else
		    {
		      *target_here = FALSE;
		      process = FALSE;
		      *hit_dp_count = *hit_dp_count + 1;

		      if (Flags[LINKED_UR_TRACE].val == 1)
			printf("NUC failed depth check TRASHED\n");

		    } /* endif */
		}
	      else
		{
		  /* NUCLEUS brought in on only target */
		  *target_here = FALSE;
		  process = FALSE;

		  if (Flags[LINKED_UR_TRACE].val == 1)
		    printf("NUC in on only tar TRASHED\n");

		} /* endif */
	    }
	  else
	    {
	      /* clause type is BOTH */
	      process = TRUE;
	      if (more_targets_here(tp))
		{
		  temp = pass_target_depth_check(curr_node);
		  if (temp)
		    *target_here = TRUE;
		  else
		    {
		      *target_here = FALSE;
		      *hit_dp_count = *hit_dp_count + 1;
		    } /* endif */
		}
	      else
		*target_here = FALSE;
	    } /* endif */
	} /* endif */
    } /* endif */

  return process;

} /* end process_this_resolution() */

/*************
 *
 *    int term_ident_subst(t1, c1, t2, c2)
 *
 *    Is t1 under substitution c1 identical to t2 under substitution c2?
 *
 *************/

static int term_ident_subst(struct term *t1,
			    struct context *c1,
			    struct term *t2,
			    struct context *c2)
{
  struct rel *r1, *r2;
  int vn1, vn2;

  /* dereference if variables */

  while (t1->type == VARIABLE && c1->terms[t1->varnum] != NULL) {
    vn1 = t1->varnum;
    t1 = c1->terms[vn1];
    c1 = c1->contexts[vn1];
  }

  while (t2->type == VARIABLE && c2->terms[t2->varnum] != NULL) {
    vn2 = t2->varnum;
    t2 = c2->terms[vn2];
    c2 = c2->contexts[vn2];
  }

  if (t1->type == VARIABLE)
    return(t2->type == VARIABLE && t1->varnum == t2->varnum && c1 == c2);

  else if (t2->type == VARIABLE)
    return(0);

  else {  /* neither term is a variable */

    if (t1->sym_num != t2->sym_num)
      return(0);  /* fail because of symbol clash */

    else {  /* following handles both names and complex terms */
      r1 = t1->farg;
      r2 = t2->farg;
      /* arities are the same, becuase sym_num's are the same */
      while (r1 != NULL && term_ident_subst(r1->argval, c1, r2->argval, c2)) {
	r1 = r1->narg;
	r2 = r2->narg;
      }

      if (r1 == NULL)
	return(1);
      else
	return(0);
    }
  }
}  /* term_ident_subst */

/************************************************************
 *
 * void write_down_tree(node, my_depth)
 * struct link_node *node;
 * int my_depth;
 *
 * The target has been brought in somewhere above node in the
 * inference tree.  I must write my_depth into target_dist
 * of all the nodes at my level and process the child of
 * myself and all my siblings' children.
 *
 ************************************************************/

static void write_down_tree(struct link_node *node,
			    int my_depth)
{

  struct link_node *lnp;

  if (node)
    {
      for (lnp = node; lnp; lnp = lnp->next_sibling)
	{
	  lnp->target_dist = my_depth;
	  write_down_tree(lnp->first_child, my_depth+1);
	} /* endfor */
    } /* endif */

} /* write_down_tree */

/************************************************************
 *
 * void write_up_tree(node, my_depth)
 * struct link_node *node;
 * int my_depth;
 *
 * The target has been brought into a node below this one at
 * a distance of my_depth.  I must write my_depth into all the
 * siblings' target_dist at this level, write all the depths
 * of all the children of the siblings to my left, and go up
 * the inference tree at the left_most sibling.
 *
 ************************************************************/

static void write_up_tree(struct link_node *node,
			  int my_depth)
{

  struct link_node *lnp;

  if (node->parent)
    {
      /* I am not the dummy node at the top of the inference tree */
      /* ... writing my_depth to myself and all to right          */
      for (lnp = node; lnp; lnp = lnp->next_sibling)
	lnp->target_dist = my_depth;
      /* writing my_depth to all my left siblings and */
      /* writing depths to their children             */
      lnp = node;
      while (lnp->prev_sibling)
	{
	  lnp = lnp->prev_sibling;
	  lnp->target_dist = my_depth;
	  write_down_tree(lnp->first_child, my_depth+1);
	} /* endwhile */
      write_up_tree(lnp->parent, my_depth+1);
    } /* endif */

} /* end write_up_tree() */

/************************************************************
 *
 * void write_target_distances(curr_node)
 * struct link_node *curr_node;
 *
 * curr_node has resolved with a clause that has the target.
 * The acquitistion of this target has passed all the depth
 * and legality checks and now the entire inference tree
 * must have all their target_dist updated.
 *
 ************************************************************/

static void write_target_distances(struct link_node *curr_node)
{

  write_up_tree(curr_node, 0);

} /* end write_target_distances() */
