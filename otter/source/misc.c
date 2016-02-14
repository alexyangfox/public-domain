/*
 *  misc.c -- Miscellaneous routines.
 *
 */

#include "header.h"

/*************
 *
 *    init() -- initialize global variables
 *
 *************/

void init(void)
{
  Stats[INIT_WALL_SECONDS] = wall_seconds();
  clock_init();
  init_options();
  init_attributes();

  Null_output = fopen("/dev/null", "w");
  Bell = '\007';

  built_in_symbols();
  init_special_ops();

  declare_user_functions();

  strcpy(Float_format, "%.12f");

  Is_pos_lits = get_is_tree();   /* index for forward subsumption */
  Is_neg_lits = get_is_tree();   /* index for forward subsumption */
  Demod_imd   = get_imd_tree();  /* index for demodulation */

  Fpa_pos_lits      = alloc_fpa_index();
  Fpa_neg_lits      = alloc_fpa_index();
  Fpa_clash_pos_lits= alloc_fpa_index();
  Fpa_clash_neg_lits= alloc_fpa_index();
  Fpa_clash_terms   = alloc_fpa_index();
  Fpa_alphas        = alloc_fpa_index();
  Fpa_back_demod    = alloc_fpa_index();

}  /* init */

/*************
 *
 *    abend
 *
 *************/

void abend(char *str)
{
  output_stats(stdout, 3);

  fprintf(stderr, "\n%c********** ABNORMAL END **********\n\n", Bell);
  fprintf(stderr, "********** %s\n", str);

  fprintf(stdout, "\n********** ABNORMAL END **********\n\n");
  fprintf(stdout, "********** %s\n", str);

  exit(ABEND_EXIT);
    
}  /* abend */

/*************
 *
 *   read_a_file()
 *
 *************/

void read_a_file(FILE *in_fp,
		 FILE *out_fp)
{
  struct list *l;
  struct term *t, *t1;
  struct clause *c;
  int rc, error, list_errors, i, j;
  struct formula_ptr *formp;
  char *s;
  int quit_early = 0;

  t = read_term(in_fp, &rc);
  while ((t || rc == 0) && !quit_early) {
    error = 0;
    if (!t)
      error = 1;
    else if (t->type != COMPLEX)
      error = 1;
    else if (str_ident("include", sn_to_str(t->sym_num))) {
      t1 = t->farg->argval;
      if (t1->type == COMPLEX || t->farg->narg) {
	fprintf(out_fp, "ERROR, bad argument to include: ");
	print_term_nl(out_fp, t); 
	Stats[INPUT_ERRORS]++;
      }
      else {
	char fn[MAX_NAME];
	FILE *local_in_fp;

	s = sn_to_str(t1->sym_num);
	/* If filename is quoted, get rid of the quotes. */
	if (s[0] == '\"' || s[0] == '\'') {
	  strcpy(fn, s+1);
	  fn[strlen(fn)-1] = '\0';
	}
	else
	  strcpy(fn, s);
	local_in_fp = fopen(fn, "r");
	if (local_in_fp == NULL) {
	  fprintf(out_fp, "ERROR, cannot open file %s.\n", fn);
	  Stats[INPUT_ERRORS]++;
	}
	else {
	  print_term_nl(out_fp, t); 
	  if (Flags[ECHO_INCLUDED_FILES].val)
	    {
	      fprintf(out_fp, "------- start included file %s-------\n", fn);
	      read_a_file(local_in_fp, out_fp);
	      fprintf(out_fp, "------- end included file %s-------\n", fn);
	    }
	  else
	    read_a_file(local_in_fp, Null_output);
	  fclose(local_in_fp);
	}
      }
    }

    else if (str_ident("set", sn_to_str(t->sym_num))) {
      i = change_flag(out_fp, t, 1);
      if (i != -1) {
	print_term_nl(out_fp, t); 
	dependent_flags(out_fp, i);
      }
    }
    else if (str_ident("clear", sn_to_str(t->sym_num))) {
      i = change_flag(out_fp, t, 0);
      if (i != -1) {
	print_term_nl(out_fp, t); 
	dependent_flags(out_fp, i);
      }
    }
    else if (str_ident("assign", sn_to_str(t->sym_num))) {
      i = change_parm(out_fp, t);
      if (i != -1) {
	print_term_nl(out_fp, t); 
	dependent_parms(out_fp, i);
      }
    }
    else if (str_ident("list", sn_to_str(t->sym_num))) {
      t1 = t->farg->argval;
      if (t1->type == COMPLEX || t->farg->narg) {
	fprintf(out_fp, "ERROR, bad argument to list: ");
	print_term_nl(out_fp, t); 
	Stats[INPUT_ERRORS]++;
      }
      else if (str_ident("usable", sn_to_str(t1->sym_num)) ||
	       str_ident("axioms", sn_to_str(t1->sym_num))) {
	if (str_ident("axioms", sn_to_str(t1->sym_num)))
	  fprintf(stderr, "NOTICE: Please change 'axioms' to 'usable'.\n");
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	l = read_cl_list(in_fp, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	else if (Flags[PROCESS_INPUT].val == 0) {
	  c = l->first_cl;
	  while (c) {
	    Stats[INPUT_ERRORS] += process_linked_tags(c);
	    cl_integrate(c);
	    c = c->next_cl;
	  }
	}
	print_cl_list(out_fp, l);
	append_lists(Usable,l);
      }
      else if (str_ident("sos", sn_to_str(t1->sym_num))) {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	l = read_cl_list(in_fp, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	else if (Flags[PROCESS_INPUT].val == 0) {
	  c = l->first_cl;
	  while (c) {
	    cl_integrate(c);
	    c = c->next_cl;
	  }
	}
	print_cl_list(out_fp, l);
	append_lists(Sos,l);
      }
      else if (str_ident("demodulators", sn_to_str(t1->sym_num))) {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	l = read_cl_list(in_fp, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	c = l->first_cl;
	while (c) {
	  cl_integrate(c);
	  c = c->next_cl;
	}
	print_cl_list(out_fp, l);
	append_lists(Demodulators,l);
      }
      else if (str_ident("passive", sn_to_str(t1->sym_num))) {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	l = read_cl_list(in_fp, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	c = l->first_cl;
	/* always integrate, because never pre_processed */
	while (c) {
	  cl_integrate(c);
	  c = c->next_cl;
	}
	print_cl_list(out_fp, l);
	append_lists(Passive,l);
      }
      else if (str_ident("hot", sn_to_str(t1->sym_num))) {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	l = read_cl_list(in_fp, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	c = l->first_cl;
	/* always integrate, because never pre_processed */
	while (c) {
	  hot_cl_integrate(c);
	  c = c->next_cl;
	}
	print_cl_list(out_fp, l);
	append_lists(Hot,l);
      }
      else if (str_ident("hints", sn_to_str(t1->sym_num))) {
	Internal_flags[HINTS_PRESENT] = 1;
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	l = read_cl_list(in_fp, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	/* always integrate, because never pre_processed */
	for (c = l->first_cl; c; c = c->next_cl)
	  hint2_integrate(c);
	print_hints_cl_list(out_fp, l);
	append_lists(Hints,l);
      }
      else if (str_ident("hints2", sn_to_str(t1->sym_num))) {
	Internal_flags[HINTS2_PRESENT] = 1;
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	l = read_cl_list(in_fp, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	/* always integrate, because never pre_processed */
	for (c = l->first_cl; c; c = c->next_cl)
	  hint2_integrate(c);
	print_hints_cl_list(out_fp, l);
	append_lists(Hints2,l);
      }
      else if (str_ident("mace_constraints", sn_to_str(t1->sym_num))) {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	l = read_cl_list(in_fp, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	/* don't integrate */
	print_cl_list(out_fp, l);
	append_lists(Mace_constraints,l);
      }
      else if (str_ident("foreach", sn_to_str(t1->sym_num))) {
	Internal_flags[FOREACH_SOS] = 1;
	quit_early = 1;
      }
      else {
	if (str_ident("axioms", sn_to_str(t1->sym_num)))
	  fprintf(stderr, "Name of axioms list is now 'usable'.\n");
	fprintf(out_fp, "ERROR, unknown list: ");
	print_term_nl(out_fp, t); 
	l = read_cl_list(in_fp, &list_errors);
	print_cl_list(out_fp, l);
	Stats[INPUT_ERRORS]++;
      }
    }
    else if (str_ident("formula_list", sn_to_str(t->sym_num))) {
      t1 = t->farg->argval;
      if (t1->type == COMPLEX || t->farg->narg) {
	fprintf(out_fp, "ERROR, bad argument to list: ");
	print_term_nl(out_fp, t); 
	Stats[INPUT_ERRORS]++;
      }
      else if (str_ident("usable", sn_to_str(t1->sym_num)) ||
	       str_ident("axioms", sn_to_str(t1->sym_num))) {
	if (str_ident("axioms", sn_to_str(t1->sym_num)))
	  fprintf(stderr, "NOTICE: Please change 'axioms' to 'usable'.\n");
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	formp = read_formula_list(in_fp, &list_errors);
	print_formula_list(out_fp,formp);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	else {
	  CLOCK_START(CLAUSIFY_TIME);
	  l = clausify_formula_list(formp);
	  CLOCK_STOP(CLAUSIFY_TIME);
	  if (Flags[PROCESS_INPUT].val == 0) {
	    c = l->first_cl;
	    while (c) {
	      cl_integrate(c);
	      c = c->next_cl;
	    }
	  }
	  fprintf(out_fp, "\n-------> usable clausifies to:\n\nlist(usable).\n");
	  print_cl_list(out_fp, l);
	  append_lists(Usable,l);
	}
      }
      else if (str_ident("sos", sn_to_str(t1->sym_num))) {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	formp = read_formula_list(in_fp, &list_errors);
	print_formula_list(out_fp,formp);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	else {
	  CLOCK_START(CLAUSIFY_TIME);
	  l = clausify_formula_list(formp);
	  CLOCK_STOP(CLAUSIFY_TIME);
	  if (Flags[PROCESS_INPUT].val == 0) {
	    c = l->first_cl;
	    while (c) {
	      cl_integrate(c);
	      c = c->next_cl;
	    }
	  }
	  fprintf(out_fp, "\n-------> sos clausifies to:\n\nlist(sos).\n");
	  print_cl_list(out_fp, l);
	  append_lists(Sos,l);
	}
      }
      else if (str_ident("passive", sn_to_str(t1->sym_num))) {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	formp = read_formula_list(in_fp, &list_errors);
	print_formula_list(out_fp,formp);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	else {
	  CLOCK_START(CLAUSIFY_TIME);
	  l = clausify_formula_list(formp);
	  CLOCK_STOP(CLAUSIFY_TIME);
	  c = l->first_cl;
	  /* always integrate, because never pre_processed */
	  while (c) {
	    cl_integrate(c);
	    c = c->next_cl;
	  }
		
	  fprintf(out_fp, "\n-------> passive clausifies to:\n\nlist(passive).\n");
	  print_cl_list(out_fp, l);
	  append_lists(Passive,l);
	}
      }
      else if (str_ident("hot", sn_to_str(t1->sym_num))) {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	formp = read_formula_list(in_fp, &list_errors);
	print_formula_list(out_fp,formp);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	else {
	  CLOCK_START(CLAUSIFY_TIME);
	  l = clausify_formula_list(formp);
	  CLOCK_STOP(CLAUSIFY_TIME);
	  c = l->first_cl;
	  /* always integrate, because never pre_processed */
	  while (c) {
	    hot_cl_integrate(c);
	    c = c->next_cl;
	  }
		
	  fprintf(out_fp, "\n-------> hot list clausifies to:\n\nlist(hot).\n");
	  print_cl_list(out_fp, l);
	  append_lists(Hot,l);
	}
      }
      else {
	if (str_ident("axioms", sn_to_str(t1->sym_num)))
	  fprintf(stderr, "Name of axioms list is now 'usable'.\n");
	fprintf(out_fp, "ERROR, unknown formula_list: ");
	print_term_nl(out_fp, t); 
	l = read_cl_list(in_fp, &list_errors);
	print_cl_list(out_fp, l);
	Stats[INPUT_ERRORS]++;
      }
    }
    else if (str_ident("weight_list", sn_to_str(t->sym_num))) {
      t1 = t->farg->argval;
      if (t1->type != NAME || t->farg->narg) {
	fprintf(out_fp, "ERROR, bad argument to Weight_list: ");
	print_term_nl(out_fp, t); 
	Stats[INPUT_ERRORS]++;
      }
      else if (str_ident("purge_gen", sn_to_str(t1->sym_num))) {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	if (Weight_purge_gen) {
	  fprintf(out_fp, "----> ERROR, already have purge weight list.\n");
	  Stats[INPUT_ERRORS] ++;
	}
	Weight_purge_gen = read_wt_list(in_fp, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	Weight_purge_gen_index = get_is_tree();
	set_wt_list(Weight_purge_gen, Weight_purge_gen_index, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	print_list(out_fp, Weight_purge_gen);
      }
      else if (str_ident("pick_given", sn_to_str(t1->sym_num))) {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	if (Weight_pick_given) {
	  fprintf(out_fp, "----> ERROR, already have pick weight list.\n");
	  Stats[INPUT_ERRORS] ++;
	}
	Weight_pick_given = read_wt_list(in_fp, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	Weight_pick_given_index = get_is_tree();
	set_wt_list(Weight_pick_given, Weight_pick_given_index, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	print_list(out_fp, Weight_pick_given);
      }
      else if (str_ident("pick_and_purge", sn_to_str(t1->sym_num))) {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	if (Weight_pick_given || Weight_purge_gen) {
	  fprintf(out_fp, "----> ERROR, already have pick weight list or purge weight list.\n");
	  Stats[INPUT_ERRORS] ++;
	}
	Weight_pick_given = Weight_purge_gen = read_wt_list(in_fp, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	Weight_pick_given_index = Weight_purge_gen_index = get_is_tree();
	set_wt_list(Weight_pick_given, Weight_pick_given_index, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	print_list(out_fp, Weight_pick_given);
      }
      else if (str_ident("terms", sn_to_str(t1->sym_num))) {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	if (Weight_terms) {
	  fprintf(out_fp, "----> ERROR, already have term weight list.\n");
	  Stats[INPUT_ERRORS] ++;
	}
	Weight_terms = read_wt_list(in_fp, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	Weight_terms_index = get_is_tree();
	set_wt_list(Weight_terms, Weight_terms_index, &list_errors);
	if (list_errors != 0)
	  Stats[INPUT_ERRORS] += list_errors;
	print_list(out_fp, Weight_terms);
      }
      else {
	fprintf(out_fp, "ERROR, unknown Weight_list: ");
	print_term_nl(out_fp, t); 
	Weight_pick_given = read_wt_list(in_fp, &list_errors);
	print_list(out_fp, Weight_pick_given);
	Stats[INPUT_ERRORS]++;
      }
    }
    else if (str_ident("lex", sn_to_str(t->sym_num))) {
      if (t->farg == NULL || t->farg->narg || proper_list(t->farg->argval) == 0) {
	fprintf(out_fp, "ERROR, argument of lex term is not a list: ");
	print_term_nl(out_fp, t);
	Stats[INPUT_ERRORS]++;
      }
      else {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	set_lex_vals(t);
	Internal_flags[LEX_VALS_SET] = 1;
      }
    }
    else if (str_ident("lrpo_multiset_status", sn_to_str(t->sym_num))) {
      if (t->farg == NULL || t->farg->narg || proper_list(t->farg->argval) == 0) {
	fprintf(out_fp, "ERROR, argument of lrpo_status term is not a list: ");
	print_term_nl(out_fp, t);
	Stats[INPUT_ERRORS]++;
      }
      else {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
		
	set_lrpo_status(t, LRPO_MULTISET_STATUS);
      }
    }
    else if (str_ident("lrpo_lr_status", sn_to_str(t->sym_num))) {
      fprintf(out_fp, "\nERROR, the command lrpo_lr_status no longer exists.\n");

      fprintf(stderr, "\nERROR, the command lrpo_lr_status no longer exists.\n");
      fprintf(stderr, "Symbols have lr status by default.  The command\n");
      fprintf(stderr, "lrpo_multiset_status gives symbols multiset status.\n");
      Stats[INPUT_ERRORS]++;
    }
    else if (str_ident("skolem", sn_to_str(t->sym_num))) {
      if (t->farg == NULL || t->farg->narg || proper_list(t->farg->argval) == 0) {
	fprintf(out_fp, "ERROR, argument of skolem term is not a list: ");
	print_term_nl(out_fp, t);
	Stats[INPUT_ERRORS]++;
      }
      else {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
	set_skolem(t);
      }
    }
    else if (str_ident("overbeek_terms", sn_to_str(t->sym_num))) {
      if (t->farg == NULL || t->farg->narg || proper_list(t->farg->argval) == 0) {
	fprintf(out_fp, "ERROR, argument of overbeek_terms is not a list: ");
	print_term_nl(out_fp, t);
	Stats[INPUT_ERRORS]++;
      }
      else {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
	Overbeek_terms = copy_term(t->farg->argval);
	set_vars(Overbeek_terms);
      }
    }
    else if (str_ident("split_atoms", sn_to_str(t->sym_num))) {
      if (t->farg == NULL || t->farg->narg || proper_list(t->farg->argval) == 0) {
	fprintf(out_fp, "ERROR, argument of split_atoms is not a list: ");
	print_term_nl(out_fp, t);
	Stats[INPUT_ERRORS]++;
      }
      else {
	Split_atoms = copy_term(t->farg->argval);
	set_vars(Split_atoms);
	if (!ground(Split_atoms)) {
	  fprintf(out_fp, "ERROR, split_atoms must be gound: ");
	  print_term_nl(out_fp, t);
	  Stats[INPUT_ERRORS]++;
	}
	else {
	  fprintf(out_fp, "\n");
	  print_term_nl(out_fp, t);
	}
      }
    }
    else if (str_ident("special_unary", sn_to_str(t->sym_num))) {
      if (t->farg == NULL || t->farg->narg || proper_list(t->farg->argval) == 0) {
	fprintf(out_fp, "ERROR, argument of special_unary term is not a list: ");
	print_term_nl(out_fp, t);
	Stats[INPUT_ERRORS]++;
      }
      else {
	fprintf(out_fp, "\n");
	print_term_nl(out_fp, t);
	set_special_unary(t);
	Internal_flags[SPECIAL_UNARY_PRESENT] = 1;
      }
    }
    else if (str_ident("op", sn_to_str(t->sym_num))) {
      print_term_nl(out_fp, t);
	    
      if (!process_op_command(t))
	Stats[INPUT_ERRORS]++;
    }
    else if (str_ident("float_format", sn_to_str(t->sym_num))) {
      if (t->farg == NULL || t->farg->narg) {
	fprintf(out_fp, "ERROR, float_format term must have one argument.\n");
	print_term_nl(out_fp, t);
	Stats[INPUT_ERRORS]++;
      }
      else {
	print_term_nl(out_fp, t);
	s = sn_to_str(t->farg->argval->sym_num);
	/* Assume it's well formed.  Remove quotes. */
	i = strlen(s);
	for (j = 1; j < i-1 ; j++)
	  Float_format[j-1] = s[j];
	Float_format[i-2] = '\0';
      }
    }

    else if (str_ident("make_evaluable", sn_to_str(t->sym_num))) {
      if (sn_to_arity(t->sym_num) != 2) {
	fprintf(out_fp, "ERROR, make_evaluable term must have two arguments:\n");
	print_term_nl(out_fp, t);
	Stats[INPUT_ERRORS]++;
      }
      else {
	int i1, i2;
	struct sym_ent *p1, *p2;

	i1 = t->farg->argval->sym_num; i2 = t->farg->narg->argval->sym_num;
	p1 = sn_to_node(i1); p2 = sn_to_node(i2);
	if (p2->eval_code < 1) {
	  fprintf(out_fp, "ERROR, second arg is not evaluable:\n");
	  print_term_nl(out_fp, t);
	  Stats[INPUT_ERRORS]++;
	}
	else if (p1->arity != p2->arity) {
	  fprintf(out_fp, "ERROR, args have different arities:\n");
	  print_term_nl(out_fp, t);
	  Stats[INPUT_ERRORS]++;
	}
	else {
	  print_term_nl(out_fp, t);
	  p1->eval_code = p2->eval_code;
	}
      }
    }
    else if (str_ident("initial_proof_object", sn_to_str(t->sym_num))) {
      fprintf(out_fp, "\n");
      print_term_nl(out_fp, t);
      l = init_proof_object(in_fp, out_fp);
      if (l == NULL) 
	Stats[INPUT_ERRORS]++;
      else {
	if (Flags[PROCESS_INPUT].val == 0) {
	  c = l->first_cl;
	  while (c) {
	    Stats[INPUT_ERRORS] += process_linked_tags(c);
	    cl_integrate(c);
	    c = c->next_cl;
	  }
	}
	print_cl_list(out_fp, l);
	append_lists(Usable,l);
      }
    }  /* initial_proof_object */
    else if (str_ident("overbeek_world", sn_to_str(t->sym_num))) {
      int errors = 0;
      struct term_ptr *p1, *p2;
      fprintf(out_fp, "\n");
      print_term_nl(out_fp, t);
      p1 = read_list(in_fp, &errors, 0);  /* don't integrate */
      if (errors > 0)
	abend("overbeek_world: term errors");
      for (p2 = p1; p2; p2 = p2->next) {
	int ok = set_vars(p2->term);
	if (!ok)
	  abend("overbeek_world: too many variables");
	renumber_vars_term(p2->term);
	zap_variable_names(p2->term);
	overbeek_insert(p2->term);
	print_term_nl(out_fp, p2->term);
      }
      fprintf(out_fp, "end_of_list.\n");
#if 0
      check_overbeek_world();
#endif
      
    }  /* overbeek_world */
    else
      error = 1;

    if (error) {
      Stats[INPUT_ERRORS]++;
      if (t) {
	fprintf(out_fp, "ERROR, command not found: ");
	print_term_nl(out_fp, t); 
      }
    }
    if (t)
      zap_term(t);
    if (!quit_early)
      t = read_term(in_fp, &rc);
  }
}  /* read_a_file */

/*************
 *
 *    sos_argument()
 *
 *************/

void sos_argument(char *buf)
{
  struct term *t;
  int p = 0;

  t = str_to_term(buf, &p, 0);
  if (t == NULL) {
    Stats[INPUT_ERRORS]++;
  }
  else {
    skip_white(buf, &p);
    if (buf[p] == '.')
      p++;
    skip_white(buf, &p);
    if (buf[p] != '\0') {
      fprintf(stdout, "\nERROR, characters after term:\n");
      print_error(stdout, buf, p);
      Stats[INPUT_ERRORS]++;
    }
    else {
      t = term_fixup(t);
      if (!set_vars(t)) {
	fprintf(stdout, "\nERROR, clause contains too many variables:\n");
	print_term(stdout, t); printf(".\n\n");
	zap_term(t);
	Stats[INPUT_ERRORS]++;
      }
      else if (contains_skolem_symbol(t)) {
	fprintf(stdout, "\nERROR, input clause contains Skolem symbol:\n");
	print_term(stdout, t); printf(".\n\n");
	zap_term(t);
	Stats[INPUT_ERRORS]++;
      }
      else {
	struct clause *c = term_to_clause(t);
	zap_term(t);
	if (c == NULL)
	  Stats[INPUT_ERRORS]++;

	if (Flags[PROCESS_INPUT].val == 0) {
	  cl_integrate(c);
	}

	fprintf(stdout, "\nlist(sos).\n");
	print_clause(stdout, c);
	fprintf(stdout, "end_of_list.\n");

	append_cl(Sos, c);
	Stats[SOS_SIZE]++;
      }
    }
  }
}  /* sos_argument */

/*************
 *
 *    read_all_input()
 *
 *************/

void read_all_input(int argc,
		    char **argv)
{
  struct list *l;
  struct clause *c, *c2;
  FILE *in_fp, *out_fp;

  Usable = get_list();
  Sos = get_list();
  Demodulators = get_list();
  Passive = get_list();
  Hot = get_list();
  Hints = get_list();
  Hints2 = get_list();
  Mace_constraints = get_list();
  
  in_fp  = stdin;
  out_fp = stdout;

  /* 
     otter                            < in > out
     otter                -f f1 f2 f3      > out
     otter -s 'f(x,x)=x.'             < in > out
     otter -s 'f(x,x)=x.' -f f1 f2 f3      > out
  */

  /* Always turn on input clock, in case it is set in the input.
     This prevents a warning message about trying to stop a clock
     that is not running.
  */

  {
    int clock_flag = Flags[CLOCKS].val;  /* save */
    Flags[CLOCKS].val = 1;               /* set */
    CLOCK_START(INPUT_TIME);
    Flags[CLOCKS].val = clock_flag;      /* restore */
  }

  if ((argc > 1 && str_ident(argv[1], "-f")) ||
      (argc > 3 && str_ident(argv[3], "-f"))) {
    /* read from files given in rest of arguments */
    int n = (argc > 1 && str_ident(argv[1], "-f")) ? 2 : 4;
    int i;
    for (i = n; i < argc; i++) {
      in_fp = fopen(argv[i], "r");
      if (in_fp == NULL) {
	printf("\nERROR, input file %s not found.\n", argv[i]);
	Stats[INPUT_ERRORS]++;
      }
      else {
	printf("\n%% Reading file %s.\n", argv[i]);
	read_a_file(in_fp, out_fp);  /* read from file arg */
      }
    }
  }
  else
    read_a_file(in_fp, out_fp);  /* read from stdin */

  CLOCK_STOP(INPUT_TIME);

  if (argc > 1 && str_ident(argv[1], "-s"))
    if (argc < 3) {
      printf("ERROR, missing sos (-s) argument.\n");
      Stats[INPUT_ERRORS]++;
    }
    else {
      printf("\n%% Reading sos clause from the command line.\n");
      sos_argument(argv[2]);  /* get sos clause from command line */
    }
      
  if (Internal_flags[FOREACH_SOS])
    foreach_sos();  /* fork children to do rest of clauses on stdin */

  if (Stats[INPUT_ERRORS] == 0) {

    if (!Internal_flags[LEX_VALS_SET])
      auto_lex_order();

    if (Flags[AUTO2].val)
      automatic_2_settings();
    else if (Flags[AUTO1].val)
      automatic_1_settings();

    check_options();

    /* process demodulators */

    for (c = Demodulators->first_cl; c; c = c->next_cl) {
      if (check_input_demod(c)) {
	if (!Flags[DEMOD_LINEAR].val)
	  imd_insert(c, Demod_imd);
      }
      else {
	Stats[INPUT_ERRORS]++;
	printf("ERROR, bad demodulator: ");
	print_clause(stdout, c);
      }
    }
	
    /* Index passive list; don't pre_process, even if flag is set. */
	
    for (c = Passive->first_cl; c; c = c->next_cl)
      index_lits_all(c);

    /* Index hot list. */

    if (Hot->first_cl) {
      init_hot();
      for (c = Hot->first_cl; c; c = c->next_cl)
	hot_index_clause(c);
    }

    /* Compile and index hints. */

    if (Internal_flags[HINTS_PRESENT] && Internal_flags[HINTS2_PRESENT])
      abend("list(hints) and list(hints2) are incompatible.");
	
    if (Internal_flags[HINTS_PRESENT])
      compile_hints();
    else if (Internal_flags[HINTS2_PRESENT])
      compile_hints2();

    /* Process input if flag is set. */

    if (Flags[PROCESS_INPUT].val) {
      CLOCK_START(PROCESS_INPUT_TIME);
      printf("\n------------> process usable:\n");
      l = Usable;
      Usable = get_list();
      Stats[USABLE_SIZE] = 0;
      c = l->first_cl;
      while (c) {
	c2 = c;
	c = c->next_cl;
	cl_clear_vars(c2);  /* destroy input variable names */
	pre_process(c2, 1, Usable);
      }
      free_list(l);
      c2 = NULL;
      post_proc_all((struct clause *) NULL, 1, Usable);
      printf("\n------------> process sos:\n");
      l = Sos;
      Sos = get_list();
      Stats[SOS_SIZE] = 0;
      c = l->first_cl;
      while (c) {
	c2 = c;
	c = c->next_cl;
	cl_clear_vars(c2);  /* destroy input variable names */
	pre_process(c2, 1, Sos);
      }
      free_list(l);
      c2 = NULL;
      post_proc_all((struct clause *) NULL, 1, Sos);
      
      if (Flags[INPUT_SOS_FIRST].val)
	for (c = Sos->first_cl; c; c = c->next_cl)
	  c->pick_weight = -MAX_INT;

      CLOCK_STOP(PROCESS_INPUT_TIME);
    }
    else {  /* index usable and sos (not passive) */
      for (c = Usable->first_cl; c; c = c->next_cl) {
	index_lits_clash(c);
	index_lits_all(c);
      }
      for (c = Sos->first_cl; c; c = c->next_cl) {
        index_lits_all(c);
	if (Flags[INPUT_SOS_FIRST].val)
	  c->pick_weight = -MAX_INT;
	else {
	  c->pick_weight = weight_cl(c, Weight_pick_given_index);
	  if (Internal_flags[HINTS_PRESENT])
	    adjust_weight_with_hints(c);
	  else if (Internal_flags[HINTS2_PRESENT])
	    adjust_weight_with_hints2(c);
	}
      }
    }
  }

  printf("\n======= end of input processing =======\n"); fflush(stdout);

  Max_input_id = next_cl_num() - 1;

  fflush(stdout);

}  /* read_all_input */

/*************
 *
 *    set_lex_vals(t)
 *
 *    t is a lex term with a list as its one and only argument.
 *    Set lexical values of the members to 1, 2, 3, ... .
 *
 *************/

void set_lex_vals(struct term *t)
{
  struct rel *r;
  int i;
  struct sym_ent *p;

  /* Symbols get lex vals 2, 4, 6, 8, ... . */

  for (  r = t->farg, i = 0;
	 r->argval->sym_num != Nil_sym_num;
	 r = r->argval->farg->narg, i++) {
    p = sn_to_node(r->argval->farg->argval->sym_num);
    p->lex_val = i*2 + 2;
  }
}  /* set_lex_vals */

/*************
 *
 *    set_lrpo_status(t, val) -
 
 *    t is a lex term with a list as its one and only argument.
 *    Set lrpo_status values of the members to 1.
 *
 *************/

void set_lrpo_status(struct term *t,
		     int val)
{
  struct rel *r;
  struct sym_ent *p;

  for (r = t->farg; r->argval->sym_num != Nil_sym_num; r = r->argval->farg->narg) {
    p = sn_to_node(r->argval->farg->argval->sym_num);
    p->lex_rpo_status = val;
  }
}  /* set_lrpo_status */

/*************
 *
 *    set_special_unary(t)
 *
 *    t is a lex term with a list as its one and only argument.
 *    Set special_unary values of the members to 1.
 *
 *************/

void set_special_unary(struct term *t)
{
  struct rel *r;
  struct sym_ent *p;

  for (r = t->farg; r->argval->sym_num != Nil_sym_num; r = r->argval->farg->narg) {
    p = sn_to_node(r->argval->farg->argval->sym_num);
    p->special_unary = 1;
  }
}  /* set_special_unary */

/*************
 *
 *    set_skolem(t)
 *
 *    t is a lex term with a list as its one and only argument.
 *    Set the major function symbol (including constants) of each member of the
 *    list to be a skolem symbol.  (This is called only when skolem symbols
 *    are not created by skolemization by OTTER.)
 *
 *************/

void set_skolem(struct term *t)
{
  struct rel *r;
  struct sym_ent *p;

  for (r = t->farg; r->argval->sym_num != Nil_sym_num; r = r->argval->farg->narg) {
    p = sn_to_node(r->argval->farg->argval->sym_num);
    p->skolem = 1;
  }
}  /* set_skolem */

/*************
 *
 *    free_all_mem()
 *
 *************/

void free_all_mem(void)
{
  struct clause *c;

  c = find_last_cl(Usable);
  while (c) {
    rem_from_list(c);
    un_index_lits_clash(c);
    un_index_lits_all(c);
    cl_del_int(c);
    c = find_last_cl(Usable);
  }
  free_list(Usable);
  Usable = NULL;

  c = find_last_cl(Sos);
  while (c) {
    rem_from_list(c);
    un_index_lits_all(c);
    cl_del_int(c);
    c = find_last_cl(Sos);
  }
  free_list(Sos);
  Sos = NULL;

  c = find_last_cl(Passive);
  while (c) {
    rem_from_list(c);
    un_index_lits_all(c);
    cl_del_int(c);
    c = find_last_cl(Passive);
  }
  free_list(Passive);
  Passive = NULL;

  c = find_last_cl(Demodulators);
  while (c) {
    rem_from_list(c);
    if (Flags[DEMOD_LINEAR].val == 0)  /* if imd indexing */
      imd_delete(c, Demod_imd);
    cl_del_int(c);
    c = find_last_cl(Demodulators);
  }
  free_list(Demodulators);
  Demodulators = NULL;

  zap_hints2();

  free_imd_tree(Demod_imd);

  /* Weight_purge_gen and Weight_pick_given might point to the same list */

  if (Weight_purge_gen) {
    weight_index_delete(Weight_purge_gen_index);
    zap_list(Weight_purge_gen);
    if (Weight_purge_gen == Weight_pick_given) {
      Weight_pick_given = NULL;
      Weight_pick_given_index = NULL;
    }
    Weight_purge_gen = NULL;
    Weight_purge_gen_index = NULL;
  }

  if (Weight_pick_given) {
    weight_index_delete(Weight_pick_given_index);
    zap_list(Weight_pick_given);
    Weight_pick_given = NULL;
    Weight_pick_given_index = NULL;
  }

  if (Weight_terms) {
    weight_index_delete(Weight_terms_index);
    zap_list(Weight_terms);
    Weight_terms = NULL;
    Weight_terms_index = NULL;
  }
  free_is_tree(Is_pos_lits);
  free_is_tree(Is_neg_lits);
  Is_pos_lits = Is_neg_lits = NULL;

  del_hidden_clauses();
  free_sym_tab();
}  /* free_all_mem */

/*************
 *
 *    output_stats(fp, level) -- print memory, clause, and time stats
 *
 *************/

void output_stats(FILE *fp,
		  int level)
{
  if (level >= 4)
    print_options(fp);

  if (level >= 3)
    print_mem(fp);

  if (level >= 2) {
    print_stats(fp);
    print_times(fp);
  }
  else if (level == 1) {
    print_stats_brief(fp);
    print_times_brief(fp);
  }

#if 1
  if (level >= 3) {
    int i, j;
    fprintf(fp, "\nForward subsumption counts, subsumer:number_subsumed.\n");
    for (i = 0; i < 10; i++) {
      for (j = 1; j < 10; j++)
	fprintf(fp, "%2d:%-4d ", 10*i+j, Subsume_count[10*i+j]);
      if (i < 9)  /* don't do 100 */
	fprintf(fp, "%2d:%-4d\n", 10*i+10, Subsume_count[10*i+10]);
      else
	fprintf(fp, "\n");
    }
    fprintf(fp, "All others: %d.\n", Subsume_count[0]);
  }
#endif
}  /* output_stats */

/*************
 *
 *    print_stats(fp)
 *
 *************/

void print_stats(FILE *fp)
{
  if (splitting())
    fprintf(fp, "\n------- statistics (process %d) -------\n", my_process_id());
  else
    fprintf(fp, "\n-------------- statistics -------------\n");

#if 0
  fprintf(fp, "clauses input            %7ld\n", Stats[CL_INPUT]);
#endif
  fprintf(fp, "clauses given            %7ld\n", Stats[CL_GIVEN]);
  fprintf(fp, "clauses generated        %7ld\n", Stats[CL_GENERATED]);
  if (Hot->first_cl)
    fprintf(fp, "  (hot clauses generated)%7ld\n", Stats[HOT_GENERATED]);
  if (Flags[BINARY_RES].val)
    fprintf(fp, "  binary_res generated   %7ld\n", Stats[BINARY_RES_GEN]);
  if (Flags[HYPER_RES].val)
    fprintf(fp, "  hyper_res generated    %7ld\n", Stats[HYPER_RES_GEN]);
  if (Flags[NEG_HYPER_RES].val)
    fprintf(fp, "  neg_hyper_res generated%7ld\n", Stats[NEG_HYPER_RES_GEN]);
  if (Flags[PARA_FROM].val)
    fprintf(fp, "  para_from generated    %7ld\n", Stats[PARA_FROM_GEN]);
  if (Flags[PARA_INTO].val)
    fprintf(fp, "  para_into generated    %7ld\n", Stats[PARA_INTO_GEN]);
  if (Flags[FACTOR].val)
    fprintf(fp, "  factors generated      %7ld\n", Stats[FACTOR_GEN]);
  if (Flags[GEOMETRIC_RULE].val)
    fprintf(fp, "  gL rule generated      %7ld\n", Stats[GEO_GEN]);
  if (Flags[DEMOD_INF].val)
    fprintf(fp, "  demod_inf generated    %7ld\n", Stats[DEMOD_INF_GEN]);
  if (Flags[UR_RES].val)
    fprintf(fp, "  ur_res generated       %7ld\n", Stats[UR_RES_GEN]);
  if (Flags[LINKED_UR_RES].val)
    fprintf(fp, "  linked_ur_res generated%7ld\n", Stats[LINKED_UR_RES_GEN]);
  if (Flags[BACK_UNIT_DELETION].val)
    fprintf(fp, "  back unit del. gen.    %7ld\n", Stats[BACK_UNIT_DEL_GEN]);
  
  fprintf(fp, "demod & eval rewrites    %7ld\n", Stats[REWRITES]);
  fprintf(fp, "clauses wt,lit,sk delete %7ld\n", Stats[CL_WT_DELETE]);
  fprintf(fp, "tautologies deleted      %7ld\n", Stats[CL_TAUTOLOGY]);
  fprintf(fp, "clauses forward subsumed %7ld\n", Stats[CL_FOR_SUB]);
  if (Flags[ANCESTOR_SUBSUME].val)
    fprintf(fp, "cl not subsumed due to ancestor_subsume %7ld\n", Stats[CL_NOT_ANC_SUBSUMED]);
  fprintf(fp, "  (subsumed by sos)      %7ld\n", Stats[FOR_SUB_SOS]);
  fprintf(fp, "unit deletions           %7ld\n", Stats[UNIT_DELETES]);
  fprintf(fp, "factor simplifications   %7ld\n", Stats[FACTOR_SIMPLIFICATIONS]);
  fprintf(fp, "clauses kept             %7ld\n", Stats[CL_KEPT]);
  if (Hot->first_cl)
    fprintf(fp, "  (hot clauses kept)     %7ld\n", Stats[HOT_KEPT]);

  fprintf(fp, "new demodulators         %7ld\n", Stats[NEW_DEMODS]);
  fprintf(fp, "empty clauses            %7ld\n", Stats[EMPTY_CLAUSES]);
  fprintf(fp, "clauses back demodulated %7ld\n", Stats[CL_BACK_DEMOD]);
  fprintf(fp, "clauses back subsumed    %7ld\n", Stats[CL_BACK_SUB]);

  fprintf(fp, "usable size              %7ld\n", Stats[USABLE_SIZE]);
  fprintf(fp, "sos size                 %7ld\n", Stats[SOS_SIZE]);
  fprintf(fp, "demodulators size        %7ld\n", Stats[DEMODULATORS_SIZE]);
  fprintf(fp, "passive size             %7ld\n", Stats[PASSIVE_SIZE]);
  fprintf(fp, "hot size                 %7ld\n", Stats[HOT_SIZE]);
  fprintf(fp, "Kbytes malloced          %7ld\n", Stats[K_MALLOCED]);

  if (Flags[LINKED_UR_RES].val) {
    fprintf(fp, "linked UR depth hits     %7ld\n", Stats[LINKED_UR_DEPTH_HITS]);
    fprintf(fp, "linked UR deduct hits    %7ld\n", Stats[LINKED_UR_DED_HITS]);
  }

  /* The following are output only if not 0. */
  /* They aren't errors, but they are anomalies. */

  if (Stats[CL_VAR_DELETES] != 0)
    fprintf(fp, "cl deletes, too many vars      %7ld\n", Stats[CL_VAR_DELETES]);
  if (Stats[FPA_OVERLOADS] != 0)
    fprintf(fp, "fpa argument overloads         %7ld\n", Stats[FPA_OVERLOADS]);
  if (Stats[FPA_UNDERLOADS] != 0)
    fprintf(fp, "fpa argument underloads        %7ld\n", Stats[FPA_UNDERLOADS]);
  if (Stats[DEMOD_LIMITS] != 0)
    fprintf(fp, "demodulations stopped by limit %7ld\n", Stats[DEMOD_LIMITS]);
}  /* print_stats */

/*************
 *
 *    print_stats_brief(fp)
 *
 *************/

void print_stats_brief(FILE *fp)
{
  if (splitting())
    fprintf(fp, "\n------- statistics (process %d) -------\n", my_process_id());
  else
    fprintf(fp, "\n-------------- statistics -------------\n");

  fprintf(fp, "clauses given            %7ld\n", Stats[CL_GIVEN]);
  fprintf(fp, "clauses generated        %7ld\n", Stats[CL_GENERATED]);
  if (Hot && Hot->first_cl)
    fprintf(fp, "  (hot clauses generated)%7ld\n", Stats[HOT_GENERATED]);
  fprintf(fp, "clauses kept             %7ld\n", Stats[CL_KEPT]);
  if (Hot && Hot->first_cl)
    fprintf(fp, "  (hot clauses kept)%7ld\n", Stats[HOT_KEPT]);
  fprintf(fp, "clauses forward subsumed %7ld\n", Stats[CL_FOR_SUB]);
  fprintf(fp, "clauses back subsumed    %7ld\n", Stats[CL_BACK_SUB]);
  fprintf(fp, "Kbytes malloced          %7ld\n", Stats[K_MALLOCED]);
}  /* print_stats_brief */

/*************
 *
 *    p_stats()
 *
 *************/

void p_stats(void)
{
  print_stats(stdout);
}  /* p_stats */

/*************
 *
 *    print_times(fp)
 *
 *************/

void print_times(FILE *fp)
{
  long t, min, hr;

  fprintf(fp, "\n----------- times (seconds) -----------\n");

  t = run_time();
  fprintf(fp, "user CPU time    %10.2f  ", t / 1000.);
  t = t / 1000; hr = t / 3600; t = t % 3600; min = t / 60; t = t % 60;
  fprintf(fp, "        (%ld hr, %ld min, %ld sec)\n", hr, min, t);

  t = system_time();
  fprintf(fp, "system CPU time  %10.2f  ", t / 1000.);
  t = t / 1000; hr = t / 3600; t = t % 3600; min = t / 60; t = t % 60;
  fprintf(fp, "        (%ld hr, %ld min, %ld sec)\n", hr, min, t);

  t = wall_seconds() - Stats[INIT_WALL_SECONDS];
  fprintf(fp, "wall-clock time  %7ld      ", t);
  hr = t / 3600; t = t % 3600; min = t / 60; t = t % 60;
  fprintf(fp, "       (%ld hr, %ld min, %ld sec)\n", hr, min, t);

 if (Flags[CLOCKS].val) {
#ifndef TP_ABSOLUTELY_NO_CLOCKS

  fprintf(fp, "input time       %10.2f\n", clock_val(INPUT_TIME) / 1000.);
  fprintf(fp, "  clausify time  %10.2f\n", clock_val(CLAUSIFY_TIME) / 1000.);
  if (Flags[PROCESS_INPUT].val)
    fprintf(fp, "  process input  %10.2f\n", clock_val(PROCESS_INPUT_TIME) / 1000.);
  fprintf(fp, "pick given time  %10.2f\n", clock_val(PICK_GIVEN_TIME) / 1000.);
  if (Flags[BINARY_RES].val)
    fprintf(fp, "binary_res time  %10.2f\n", clock_val(BINARY_TIME) / 1000.);
  if (Flags[HYPER_RES].val)
    fprintf(fp, "hyper_res time   %10.2f\n", clock_val(HYPER_TIME) / 1000.);
  if (Flags[NEG_HYPER_RES].val)
    fprintf(fp, "neg_hyper_res time%9.2f\n", clock_val(NEG_HYPER_TIME) / 1000.);
  if (Flags[UR_RES].val)
    fprintf(fp, "ur_res time      %10.2f\n", clock_val(UR_TIME) / 1000.);
  if (Flags[PARA_INTO].val)
    fprintf(fp, "para_into time   %10.2f\n", clock_val(PARA_INTO_TIME) / 1000.);
  if (Flags[PARA_FROM].val)
    fprintf(fp, "para_from time   %10.2f\n", clock_val(PARA_FROM_TIME) / 1000.);
  if (Flags[LINKED_UR_RES].val)
    fprintf(fp, "linked_ur time   %10.2f\n", clock_val(LINKED_UR_TIME) / 1000.);
  if (Flags[BACK_UNIT_DELETION].val)
    fprintf(fp, "back unit del time%9.2f\n", clock_val(BACK_UNIT_DEL_TIME) / 1000.);

  fprintf(fp, "pre_process time %10.2f\n", clock_val(PRE_PROC_TIME) / 1000.);
  fprintf(fp, "  renumber time  %10.2f\n", clock_val(RENUMBER_TIME) / 1000.);
  fprintf(fp, "  demod time     %10.2f\n", clock_val(DEMOD_TIME) / 1000.);
  fprintf(fp, "  order equalities%9.2f\n", clock_val(ORDER_EQ_TIME) / 1000.);
  fprintf(fp, "  unit deleletion%10.2f\n", clock_val(UNIT_DEL_TIME) / 1000.);
  fprintf(fp, "  factor simplify%10.2f\n", clock_val(FACTOR_SIMP_TIME) / 1000.);
  fprintf(fp, "  weigh cl time  %10.2f\n", clock_val(WEIGH_CL_TIME) / 1000.);
  fprintf(fp, "  hints keep time%10.2f\n", clock_val(HINTS_KEEP_TIME) / 1000.);
  fprintf(fp, "  sort lits time %10.2f\n", clock_val(SORT_LITS_TIME) / 1000.);
  fprintf(fp, "  forward subsume%10.2f\n", clock_val(FOR_SUB_TIME) / 1000.);
  fprintf(fp, "  delete cl time %10.2f\n", clock_val(DEL_CL_TIME) / 1000.);
  fprintf(fp, "  keep cl time   %10.2f\n", clock_val(KEEP_CL_TIME) / 1000.);
  fprintf(fp, "    hints time   %10.2f\n", clock_val(HINTS_TIME) / 1000.);
  fprintf(fp, "  print_cl time  %10.2f\n", clock_val(PRINT_CL_TIME) / 1000.);
  fprintf(fp, "  conflict time  %10.2f\n", clock_val(CONFLICT_TIME) / 1000.);
  fprintf(fp, "  new demod time %10.2f\n", clock_val(NEW_DEMOD_TIME) / 1000.);
  fprintf(fp, "post_process time%10.2f\n", clock_val(POST_PROC_TIME) / 1000.);
  fprintf(fp, "  back demod time%10.2f\n", clock_val(BACK_DEMOD_TIME) / 1000.);
  fprintf(fp, "  back subsume   %10.2f\n", clock_val(BACK_SUB_TIME) / 1000.);
  fprintf(fp, "  factor time    %10.2f\n", clock_val(FACTOR_TIME) / 1000.);
  if (Hot->first_cl)
    fprintf(fp, "  hot list time  %10.2f\n", clock_val(HOT_TIME) / 1000.);
  fprintf(fp, "  unindex time   %10.2f\n", clock_val(UN_INDEX_TIME) / 1000.);

#endif  /* ndef TP_ABSOLUTELY_NO_CLOCKS */
 }
}  /* print_times */

/*************
 *
 *    print_times_brief(fp)
 *
 *************/

void print_times_brief(FILE *fp)
{
  long t, min, hr;

  fprintf(fp, "\n----------- times (seconds) -----------\n");

  t = run_time();
  fprintf(fp, "user CPU time    %10.2f  ", t / 1000.);
  t = t / 1000; hr = t / 3600; t = t % 3600; min = t / 60; t = t % 60;
  fprintf(fp, "        (%ld hr, %ld min, %ld sec)\n", hr, min, t);

  t = system_time();
  fprintf(fp, "system CPU time  %10.2f  ", t / 1000.);
  t = t / 1000; hr = t / 3600; t = t % 3600; min = t / 60; t = t % 60;
  fprintf(fp, "        (%ld hr, %ld min, %ld sec)\n", hr, min, t);

  t = wall_seconds() - Stats[INIT_WALL_SECONDS];
  fprintf(fp, "wall-clock time  %7ld      ", t);
  hr = t / 3600; t = t % 3600; min = t / 60; t = t % 60;
  fprintf(fp, "       (%ld hr, %ld min, %ld sec)\n", hr, min, t);

 if (Flags[CLOCKS].val) {

  if (Flags[BINARY_RES].val)
    fprintf(fp, "binary_res time  %10.2f\n", clock_val(BINARY_TIME) / 1000.);
  if (Flags[HYPER_RES].val)
    fprintf(fp, "hyper_res time   %10.2f\n", clock_val(HYPER_TIME) / 1000.);
  if (Flags[NEG_HYPER_RES].val)
    fprintf(fp, "neg_hyper   time %10.2f\n", clock_val(NEG_HYPER_TIME) / 1000.);
  if (Flags[UR_RES].val)
    fprintf(fp, "UR_res time      %10.2f\n", clock_val(UR_TIME) / 1000.);
  if (Flags[PARA_INTO].val)
    fprintf(fp, "para_into time   %10.2f\n", clock_val(PARA_INTO_TIME) / 1000.);
  if (Flags[PARA_FROM].val)
    fprintf(fp, "para_from time   %10.2f\n", clock_val(PARA_FROM_TIME) / 1000.);
  if (Flags[LINKED_UR_RES].val)
    fprintf(fp, "linked_ur time   %10.2f\n", clock_val(LINKED_UR_TIME) / 1000.);

  fprintf(fp, "for_sub time     %10.2f\n", clock_val(FOR_SUB_TIME) / 1000.);
  fprintf(fp, "back_sub time    %10.2f\n", clock_val(BACK_SUB_TIME) / 1000.);
  fprintf(fp, "conflict time    %10.2f\n", clock_val(CONFLICT_TIME) / 1000.);
  if ((Demodulators && Demodulators->first_cl) || Internal_flags[DOLLAR_PRESENT])
    fprintf(fp, "demod time       %10.2f\n", clock_val(DEMOD_TIME) / 1000.);
  if (Hot && Hot->first_cl)
    fprintf(fp, "  hot list time  %10.2f\n", clock_val(HOT_TIME) / 1000.);
 }
}  /* print_times_brief */

/*************
 *
 *    p_times()
 *
 *************/

void p_times(void)
{
  print_times(stdout);
}  /* p_times */

/*************
 *
 *    append_lists(l1, l2) -- append l2 to l1 and free the header node l2
 *
 *************/

void append_lists(struct list *l1,
		  struct list *l2)
{
  struct clause *c;
  int i;

  if (l1->last_cl)  /* if l1 not empty */
    l1->last_cl->next_cl = l2->first_cl;
  else
    l1->first_cl = l2->first_cl;

  if (l2->first_cl) {  /* if l2 not empty */
    l2->first_cl->prev_cl = l1->last_cl;
    l1->last_cl = l2->last_cl;
  }

  for (c = l2->first_cl, i = 0; c; c = c->next_cl, i++)
    c->container = l1;

  if (l1 == Usable)
    Stats[USABLE_SIZE] += i;
  else if (l1 == Sos)
    Stats[SOS_SIZE] += i;
  else if (l1 == Demodulators)
    Stats[DEMODULATORS_SIZE] += i;
  else if (l1 == Passive)
    Stats[PASSIVE_SIZE] += i;
  else if (l1 == Hot)
    Stats[HOT_SIZE] += i;
  if (l2 == Usable)
    Stats[USABLE_SIZE] -= i;
  else if (l2 == Sos)
    Stats[SOS_SIZE] -= i;
  else if (l2 == Demodulators)
    Stats[DEMODULATORS_SIZE] -= i;
  else if (l2 == Passive)
    Stats[PASSIVE_SIZE] -= i;
  else if (l2 == Hot)
    Stats[HOT_SIZE] -= i;
  free_list(l2);
}  /* append_lists */

/*************
 *
 *    struct term *copy_term(term) -- Return a copy of the term.
 *
 *    The bits field is not copied.
 *
 *************/

struct term *copy_term(struct term *t)
{
  struct rel *r, *r2, *r3;
  struct term *t2;

  t2 = get_term();
  t2->type = t->type;
  t2->sym_num = t->sym_num;
  t2->varnum = t->varnum;
  if (t->type != COMPLEX)
    return(t2);
  else {
    r3 = NULL;
    r = t->farg;
    while (r) {
      r2 = get_rel();
      if (r3 == NULL)
	t2->farg = r2;
      else
	r3->narg = r2;
      r2->argval = copy_term(r->argval);
      r3 = r2;
      r = r->narg;
    }
    return(t2);
  }
}  /* copy_term */

/*************
 *
 *    int biggest_var(term)  --  return largest variable number (-1 if none)
 *
 *************/

int biggest_var(struct term *t)
{
  struct rel *r;
  int i, j;

  if (t->type == VARIABLE)
    return(t->varnum);
  else if (t->type == NAME)
    return(-1);
  else {
    r = t->farg;
    i = -1;
    while (r) {
      j = biggest_var(r->argval);
      if (j > i)
	i = j;
      r = r->narg;
    }
    return(i);
  }
}  /* biggest_var */

/*************
 *
 *    int biggest_var_clause(c)  --  return largest variable number (-1 if none)
 *
 *************/

int biggest_var_clause(struct clause *c)
{
  struct literal *lit;
  int i, max;

  max = -1;
  for (lit = c->first_lit; lit; lit = lit->next_lit) {
    i = biggest_var(lit->atom);
    max = (i > max ? i : max);
  }
  return(max);
}  /* biggest_var_clause */

/*************
 *
 *    int ground_clause(c)
 *
 *************/

int ground_clause(struct clause *c)
{
  return(biggest_var_clause(c) == -1);
}  /* ground_var_clause */

/*************
 *
 *    zap_list(term_ptr) -- Free a list of nonintegrated terms.
 *
 *************/

void zap_list(struct term_ptr *p)
{
  struct term_ptr *q;

  while (p) {
    zap_term(p->term);
    q = p;
    p = p->next;
    free_term_ptr(q);
  }
}  /* zap_list */

/*************
 *
 *     int occurs_in(t1, t2) -- Does t1 occur in t2?
 *
 *     term_ident is used to check identity.
 *
 *************/

int occurs_in(struct term *t1,
	      struct term *t2)
{
  struct rel *r;

  if (term_ident(t1, t2))
    return(1);
  else if (t2->type != COMPLEX)
    return(0);
  else {
    r = t2->farg;
    while (r && occurs_in(t1, r->argval) == 0)
      r = r->narg;
    return(r != NULL);
  }
}  /* occurs_in */

/*************
 *
 *   occurrences(s, t)
 *
 *   How many occurrences of s are there in t.
 *
 *************/

int occurrences(struct term *s,
		struct term *t)
{
  if (term_ident(s, t))
    return(1);
  else if (t->type != COMPLEX)
    return(0);
  else {
    struct rel *r;
    int count;

    for (r = t->farg, count = 0; r; r = r->narg)
      count += occurrences(s, r->argval);
    return(count);
  }
}  /* occurrences */

/*************
 *
 *    int sn_occur(sn, t)
 *
 *    Is sn the sym_num of t or any subterms of t?
 *
 *************/

int sn_occur(int sn,
	     struct term *t)
{
  struct rel *r;
  int occurs;

  if (t->type != COMPLEX)
    return(t->sym_num == sn);
  else if (t->sym_num == sn)
    return(1);
  else {
    occurs = 0;
    r = t->farg;
    while (r && occurs == 0) {
      occurs = sn_occur(sn, r->argval);
      r = r->narg;
    }
    return(occurs);
  }
}  /* sn_occur */

/*************
 *
 *    is is_atom(t) -- Is t an atom?
 *
 *    A term is an atom iff it is not a variable and varnum != 0.
 *    (The varnum field of an atom gives its type---equality, answer, evaluable, etc.)
 *
 *************/

int is_atom(struct term *t)
{
  return(t->type != VARIABLE && t->varnum != 0);
}  /* is_atom */

/*************
 *
 *    int id_nested_skolems(t)
 *
 *    Does t or any of its subterms have the identical_nested_skolems property?
 *
 *************/

static int id_nested_skolems(struct term *t)
{
  struct rel *r;
  int occurs;

  if (t->type != COMPLEX)
    return(0);
  else {
    occurs = 0;
    if (is_skolem(t->sym_num)) {
      r = t->farg;
      while (r && occurs == 0) {
	occurs = sn_occur(t->sym_num, r->argval);
	r = r->narg;
      }
    }
    if (occurs)
      return(1);
    else {
      occurs = 0;
      r = t->farg;
      while (r && occurs == 0) {
	occurs = id_nested_skolems(r->argval);
	r = r->narg;
      }
      return(occurs);
    }
  }
}  /* id_nested_skolems */

/*************
 *
 *    int ident_nested_skolems(c)
 *
 *    Do any of the terms in clause c have the
 *    identical_nested_skolems property?
 *
 *************/

int ident_nested_skolems(struct clause *c)
{
  struct literal *l;
  int occurs;

  l = c->first_lit;
  occurs = 0;
  while (l && occurs == 0) {
    occurs = id_nested_skolems(l->atom);
    l = l->next_lit;
  }
  return(occurs);
}  /* ident_nested_skolems */

/*************
 *
 *    int ground(t) -- is a term ground?
 *
 *************/

int ground(struct term *t)
{
  struct rel *r;
  int ok;

  if (t->type == NAME)
    return(1);
  else if (t->type == VARIABLE)
    return(0);
  else { /* COMPLEX */
    ok = 1;
    for (r = t->farg; r && ok; r = r->narg)
      ok = ground(r->argval);
    return(ok);
  }
}  /* ground */

/*************
 *
 *    void cleanup()
 *
 *************/

void cleanup(void)
{
  printf("\n============ end of search ============\n");

  if (Flags[PRINT_LISTS_AT_END].val) {
    printf("\nlist(usable).\n"); print_cl_list(stdout, Usable);
    printf("\nlist(sos).\n"); print_cl_list(stdout, Sos);
    if (Demodulators) {
      printf("\nlist(demodulators).\n");
      print_cl_list(stdout, Demodulators);
    }
    printf("\n");
  }

  if (Flags[FREE_ALL_MEM].val && ! multi_justifications())
    free_all_mem();

  output_stats(stdout, Parms[STATS_LEVEL].val);

  if (Stats[EMPTY_CLAUSES] > 0 &&
      (!splitting() || current_case() == NULL))
    printf("\nThat finishes the proof of the theorem.\n");

  printf("\nProcess %d finished %s", my_process_id(), get_time());

}  /* cleanup */

/*************
 *
 *    int check_stop()  --  Should the search be terminated?
 *
 *    return:
 *        KEEP_SEARCHING if we should not stop;
 *        MAX_GIVEN_EXIT if we should stop because of max_given option;
 *        MAX_SECONDS_EXIT if we should stop because of max_seconds option;
 *        MAX_GEN_EXIT if we should stop because of max_gen option;
 *        MAX_KEPT_EXIT if we should stop because of max_kept option.
 *        MAX_LEVELS_EXIT if we should stop because of max_levels option.
 *
 *************/

int check_stop(void)
{
  long given, seconds, gen, kept;
  int max_given, max_seconds, max_gen, max_kept;

  given = Stats[CL_GIVEN];
  gen = Stats[CL_GENERATED];
  kept = Stats[CL_KEPT];

  if (splitting())
    seconds = wall_seconds() - Stats[INIT_WALL_SECONDS];
  else
    seconds = run_time() / 1000;

  max_given = Parms[MAX_GIVEN].val;
  max_seconds = Parms[MAX_SECONDS].val;
  max_gen = Parms[MAX_GEN].val;
  max_kept = Parms[MAX_KEPT].val;

  if (max_given != -1 && given >= max_given)
    return(MAX_GIVEN_EXIT);
  else if(max_seconds != -1 && seconds >= max_seconds)
    return(MAX_SECONDS_EXIT);
  else if (max_gen != -1 && gen >= max_gen)
    return(MAX_GEN_EXIT);
  else if (max_kept != -1 && kept >= max_kept)
    return(MAX_KEPT_EXIT);
  else
    return(KEEP_SEARCHING);
}  /* check_stop */

/*************
 *
 *    report() -- possibly report statistics and times
 *
 *************/

void report(void)
{
  static int next_report;
  double runtime;

  if (next_report == 0)
    next_report = Parms[REPORT].val;

  runtime = run_time() / 1000.;

  if (runtime >= next_report) {
    printf("\n----- report at %9.2f seconds ----- %s", runtime, get_time());
    output_stats(stdout, Parms[STATS_LEVEL].val);
    fprintf(stderr, "%cA report (%.2f seconds) has been sent to the output file.\n", Bell, runtime);
    while (runtime >= next_report)
      next_report += Parms[REPORT].val;
  }
}  /* report */

/*************
 *
 *    void control_memory()
 *
 *************/

void control_memory(void)
{
  static int next_control_point = 0;
  int sos_distribution[500];
  int i, j, wt, n, control, size;
  struct clause *c;

  j = total_mem();

  if (Parms[MAX_MEM].val != 0 && j*3 > Parms[MAX_MEM].val) {
    if (!next_control_point)
      control = 1;
    else if (next_control_point == Stats[CL_GIVEN])
      control = 1;
    else
      control = 0;
  }
  else
    control = 0;

  if (control) {
    next_control_point = Stats[CL_GIVEN] + 20;
    for (i = 0; i < 500; i++)
      sos_distribution[i] = 0;
    for (c = Sos->first_cl, size = 0; c; c = c->next_cl, size++) {
      if (c->pick_weight < 0)
	wt = 0;
      else if (c->pick_weight >= 500)
	wt = 499;
      else
	wt = c->pick_weight;
      sos_distribution[wt]++;
    }

    i = 0; n = 0;
    while (i < 500 && n*20 <= size) {
      n += sos_distribution[i];
      i++;
    }
    i--;
	
    /* reset weight limit to i */

    if (i < Parms[MAX_WEIGHT].val || Parms[MAX_WEIGHT].val == 0) {
      Parms[MAX_WEIGHT].val = i;
      fprintf(stderr, "%c\n\nResetting weight limit to %d.\n\n", Bell, i);
      printf("\nResetting weight limit to %d.\n\n", i);
      printf("sos_size=%d\n", size);
      fflush(stdout);
#if 0
      printf("weight: number of sos clauses with that weight\n");
      for (j = 0; j < 100; j++)
	printf("%d:  %d\n", j, sos_distribution[j]);
#endif
    }
  }
	
}  /* control_memory */

/*************
 *
 *    proof_message(c) - print a  message to stderr
 *
 *    If clause c has any (probably answer) literals, print c.
 *
 *************/

static void proof_message(struct clause *c)
{
  char *user = username();
  long i = run_time();  /* i is milliseconds */

  if (Stats[EMPTY_CLAUSES] == 1)
    fprintf(stderr, "\n");

  if (splitting() && current_case() != NULL) {
    fprintf(stderr, "%c--- refuted case ", Bell);
    print_case(stderr);
  }

  else if (i > 10000) {
    /* If more than 10 seconds, print excitedly. */
    fprintf(stderr, "%c-- HEY %s, WE HAVE A PROOF!! -- ", Bell, user);
  }
  else
    fprintf(stderr, "%c-------- PROOF -------- ", Bell);
    
  if (c->first_lit)
    print_clause(stderr, c);
  else
    fprintf(stderr, "\n");
}  /* proof_message */

/*************
 *
 *    print_proof(fp, c)
 *
 *************/

void print_proof(FILE *fp,
		 struct clause *c)
{
  struct clause_ptr *cp1, *cp2, *cp3;
  struct ilist *ip1, *ip2;
  int length, level;
  
  cp1 = NULL; ip1 = NULL;
  level = get_ancestors(c, &cp1, &ip1);
  
  for (length = 0, cp2 = cp1; cp2; cp2 = cp2->next)
    if (cp2->c->parents && cp2->c->parents->i != NEW_DEMOD_RULE) {
      length++;
    }
  
  proof_message(c);  /* to stderr */
  fprintf(fp, "Length of proof is %d.", length-1);
  fprintf(fp, "  Level of proof is %d.", level-1);

  if (splitting()) {
    struct ilist *p = current_case();
    if (p != NULL) {
      fprintf(fp, "  Case ");
      print_case(fp);
    }
  }
  if (Flags[PROOF_WEIGHT].val)
    fprintf(fp, "  Weight of proof is %d.", prf_weight(c));
  fprintf(fp, "\n\n---------------- PROOF ----------------\n\n");

  cp2 = cp1;
  while (cp2) {
    cp3 = cp2->next;
    if (cp3 && cp3->c->parents &&
	cp3->c->parents->i == NEW_DEMOD_RULE &&
	cp3->c->parents->next->i == cp2->c->id) {
      /* skip over dynamic demodulator copy */
      fprintf(fp, "%d,", cp3->c->id);
      print_clause(fp, cp2->c);
      cp2 = cp3->next;
    }
    else {
      print_clause(fp, cp2->c);
      cp2 = cp2->next;
    }
  }
  fprintf(fp, "\n------------ end of proof -------------\n\n");
  fflush(fp);

  while (cp1 != NULL) {
    cp2 = cp1; cp1 = cp1->next; free_clause_ptr(cp2);
    ip2 = ip1; ip1 = ip1->next; free_ilist(ip2);
  }
}  /* print_proof */

/*************
 *
 *    struct clause *check_for_proof(c)
 *
 *    Check for EMPTY CLAUSE proof and UNIT CONFLICT proof.
 *
 *************/

struct clause *check_for_proof(struct clause *c)
{
  struct clause *e;
  struct clause_ptr *cp1, *cp2;
  int number_of_lits;

  e = NULL;
  number_of_lits = num_literals(c);
  if (number_of_lits == 0) {
    printf("\n-----> EMPTY CLAUSE at %6.2f sec ----> ",
	   run_time() / 1000.);

    print_clause(stdout, c);
    printf("\n");
    Stats[CL_KEPT]--;  /* don't count empty clauses */
                       /* pre_process has already KEPT it */
    rem_from_list(c);
    hide_clause(c);
    Stats[EMPTY_CLAUSES]++;
    e = c;
    if (Flags[PRINT_PROOFS].val)
      print_proof(stdout, e);
    if (Flags[BUILD_PROOF_OBJECT_1].val || Flags[BUILD_PROOF_OBJECT_2].val)
      build_proof_object(e);
  }
  else if (number_of_lits == 1) {
    cp1 = unit_conflict(c);
    while (cp1) {  /* empty clause from unit conflict */
      e = cp1->c;
      cp2 = cp1->next;
      free_clause_ptr(cp1);
      cp1 = cp2;

      cl_integrate(e);
      printf("\n----> UNIT CONFLICT at %6.2f sec ----> ",
	     run_time() / 1000.);
      print_clause(stdout, e);
      printf("\n");
      hide_clause(e);
      if (Flags[PRINT_PROOFS].val)
	print_proof(stdout, e);
      if (Flags[BUILD_PROOF_OBJECT_1].val || Flags[BUILD_PROOF_OBJECT_2].val)
	build_proof_object(e);
#if 0
      multi_test(e);
#endif
    }
  }

  return(e);  /* NULL if no proof was found */

}  /* check_for_proof */

/*************
 *
 *    int proper_list(t)
 *
 *    Is term t a proper list
 *
 *************/

int proper_list(struct term *t)
{
  if (t->type == VARIABLE)
    return(0);
  else if (t->type == NAME)
    return(t->sym_num == str_to_sn("$nil", 0));
  else if (t->sym_num != str_to_sn("$cons", 2))
    return(0);
  else
    return(proper_list(t->farg->narg->argval));
}  /* proper_list */

/*************
 *
 *   move_clauses()
 *
 *   Move clauses satisfying given routine from one list to another.
 *
 *************/

void move_clauses(int (*clause_proc)(struct clause *c),
		  struct list *source,
		  struct list *destination)
{
  struct clause *c1, *c2;

  c1 = source->first_cl;
  while (c1) {
    c2 = c1->next_cl;
    if ((*clause_proc)(c1)) {
      rem_from_list(c1);
      append_cl(destination, c1);
    }
    c1 = c2;
  }
}  /* move_clauses */

/*************
 *
 *   automatic_1_settings()
 *
 *   Original version (Otter 3.0.4)
 *
 *   Do a very simple syntactic analysis of the clauses and decide on
 *   a simple strategy.  Print a message about the strategy.
 *
 *************/

void automatic_1_settings(void)
{
  if (Passive->first_cl)
    abend("Passive list not accepted in auto1 mode.");
  else if (Demodulators->first_cl)
    abend("Demodulators list not accepted in auto1 mode.");
  else if (Weight_pick_given || Weight_purge_gen || Weight_terms)
    abend("Weight lists not accepted in auto1 mode.");

  else {
    struct clause *c;
    int propositional, horn, equality, max_lits, i, symmetry;

    if (Sos->first_cl) {
      printf("WARNING: Sos list not accepted in auto1 mode:\n");
      printf("         sos clauses are being moved to usable list.\n");
      append_lists(Usable, Sos);
      Sos = get_list();
    }

    /* Find out some basic properties. */

    /* All input clauses are in Usable; move to Sos. */

    for (c=Usable->first_cl, propositional=1; c&&propositional; c=c->next_cl)
      propositional = propositional_clause(c);
    for (c=Usable->first_cl, horn = 1; c && horn; c=c->next_cl)
      horn = horn_clause(c);
    for (c=Usable->first_cl, equality = 0; c && !equality; c=c->next_cl)
      equality = equality_clause(c);
    for (c=Usable->first_cl, symmetry = 0; c && !symmetry; c=c->next_cl)
      symmetry = symmetry_clause(c);
    for (c=Usable->first_cl, max_lits = 0; c; c=c->next_cl) {
      i = num_literals(c);
      max_lits = (i > max_lits ? i : max_lits);
    }

    printf("\nSCAN INPUT: prop=%d, horn=%d, equality=%d, symmetry=%d, max_lits=%d.\n",
	   propositional, horn, equality, symmetry, max_lits);
	
    if (propositional) {
      printf("\nThe clause set is propositional; the strategy will be\n");
      printf("ordered hyperresolution with the propositional\n");
      printf("optimizations, with satellites in sos and nuclei in usable.\n\n");
      auto_change_flag(stdout, HYPER_RES, 1);
      auto_change_flag(stdout, PROPOSITIONAL, 1);
      move_clauses(pos_clause, Usable, Sos);
    }
    else if (equality && max_lits == 1) {
      printf("\nAll clauses are units, and equality is present; the\n");
      printf("strategy will be Knuth-Bendix with positive clauses in sos.\n\n");
      auto_change_flag(stdout, KNUTH_BENDIX, 1);
      for (c = Usable->first_cl; c && pos_clause(c); c = c->next_cl);
      if (!c) {
	printf("\nThere is no negative clause, so all clause lists will\n");
	printf("be printed at the end of the search.\n\n");
	auto_change_flag(stdout, PRINT_LISTS_AT_END, 1);
      }
      move_clauses(pos_clause, Usable, Sos);
    }
    else if (!horn && !equality) {
      printf("\nThis is a non-Horn set without equality.  The strategy will\n");
      printf("be ordered hyper_res, unit deletion, and factoring, with\n");
      printf("satellites in sos and with nuclei in usable.\n\n");
      auto_change_flag(stdout, HYPER_RES, 1);
      auto_change_flag(stdout, FACTOR, 1);
      auto_change_flag(stdout, UNIT_DELETION, 1);
      move_clauses(pos_clause, Usable, Sos);
    }
    else if (horn && !equality) {
      printf("\nThis is a Horn set without equality.  The strategy will\n");
      printf("be hyperresolution, with satellites in sos and nuclei\n");
      printf("in usable.\n\n");
      auto_change_flag(stdout, HYPER_RES, 1);
      auto_change_flag(stdout, ORDER_HYPER, 0);
      move_clauses(pos_clause, Usable, Sos);
    }
    else if (!horn && equality) {
      printf("\nThis ia a non-Horn set with equality.  The strategy will be\n");
      printf("Knuth-Bendix, ordered hyper_res, factoring, and unit\n");
      printf("deletion, with positive clauses in sos and nonpositive\n");
      printf("clauses in usable.\n\n");
      auto_change_flag(stdout, KNUTH_BENDIX, 1);
      auto_change_flag(stdout, HYPER_RES, 1);
      auto_change_flag(stdout, UNIT_DELETION, 1);
      auto_change_flag(stdout, FACTOR, 1);
      if (symmetry) {
	printf("\nThere is a clause for symmetry of equality, so it is\n");
	printf("assumed that equality is fully axiomatized; therefore,\n");
	printf("paramodulation is disabled.\n\n");

	auto_change_flag(stdout, PARA_FROM, 0);
	auto_change_flag(stdout, PARA_INTO, 0);
      }
      move_clauses(pos_clause, Usable, Sos);
    }
    else if (horn && equality) {
      printf("\nThis is a Horn set with equality.  The strategy will be\n");
      printf("Knuth-Bendix and hyper_res, with positive clauses in\n");
      printf("sos and nonpositive clauses in usable.\n\n");
      auto_change_flag(stdout, KNUTH_BENDIX, 1);
      auto_change_flag(stdout, HYPER_RES, 1);
      auto_change_flag(stdout, ORDER_HYPER, 0);
      if (symmetry) {
	printf("\nThere is a clause for symmetry of equality is, so it is\n");
	printf("assumed that equality is fully axiomatized; therefore,\n");
	printf("paramodulation is disabled.\n\n");

	auto_change_flag(stdout, PARA_FROM, 0);
	auto_change_flag(stdout, PARA_INTO, 0);
      }
      move_clauses(pos_clause, Usable, Sos);
    }
  }
}  /* automatic_1_settings */

/*************
 *
 *   sos_has_pos_nonground()
 *
 *************/
int sos_has_pos_nonground(void)
{
  struct clause *c;

  for (c = Sos->first_cl; c; c = c->next_cl) {
    if (pos_clause(c) && !ground_clause(c))
      return(1);
  }
  return (0);
}  /* sos_has_pos_nonground */

/*************
 *
 *   automatic_2_settings()
 *
 *   Revised version (Otter 3.0.5)
 *
 *   Do a very simple syntactic analysis of the clauses and decide on
 *   a simple strategy.  Print a message about the strategy.
 *
 *   This version accepts input sos clauses.  Also, input usable clauses
 *   can be moved to sos.  See below.
 *
 *************/

void automatic_2_settings(void)
{
  if (Passive->first_cl)
    abend("Passive list not accepted in auto2 mode.");
  else if (Demodulators->first_cl)
    abend("Demodulators list not accepted in auto2 mode.");
  else if (Weight_pick_given || Weight_purge_gen || Weight_terms)
    abend("Weight lists not accepted in auto2 mode.");

  else {

    struct clause *c;
    int propositional, horn, equality, max_lits, i, symmetry;

    if (sos_has_pos_nonground())
      printf("Sos has positive nonground clause; therefore it is not changed.\n");
    else {
      printf("\nEvery positive clause in sos is ground (or sos is empty);\n");
      printf("therefore we move all positive usable clauses to sos.\n");
      move_clauses(pos_clause, Usable, Sos);
    }

    /* Find out some basic properties. */

    for (c=Usable->first_cl, propositional=1; c&&propositional; c=c->next_cl)
      propositional = propositional_clause(c);
    for (c=Sos->first_cl; c&&propositional; c=c->next_cl)
      propositional = propositional_clause(c);

    for (c=Usable->first_cl, horn = 1; c && horn; c=c->next_cl)
      horn = horn_clause(c);
    for (c=Sos->first_cl; c && horn; c=c->next_cl)
      horn = horn_clause(c);

    for (c=Usable->first_cl, equality = 0; c && !equality; c=c->next_cl)
      equality = equality_clause(c);
    for (c=Sos->first_cl; c && !equality; c=c->next_cl)
      equality = equality_clause(c);

    for (c=Usable->first_cl, symmetry = 0; c && !symmetry; c=c->next_cl)
      symmetry = symmetry_clause(c);
    for (c=Sos->first_cl; c && !symmetry; c=c->next_cl)
      symmetry = symmetry_clause(c);

    for (c=Usable->first_cl, max_lits = 0; c; c=c->next_cl) {
      i = num_literals(c);
      max_lits = (i > max_lits ? i : max_lits);
    }

    for (c=Sos->first_cl; c; c=c->next_cl) {
      i = num_literals(c);
      max_lits = (i > max_lits ? i : max_lits);
    }

    printf("\nProperties of input clauses: prop=%d, horn=%d, equality=%d, symmetry=%d, max_lits=%d.\n\n",
	   propositional, horn, equality, symmetry, max_lits);
	
    if (propositional) {
      printf("\nAll clauses are propositional; therefore we set the\n");
      printf("propositional flag and use ordered hyperresolution.\n");
      auto_change_flag(stdout, PROPOSITIONAL, 1);
      auto_change_flag(stdout, HYPER_RES, 1);
    }
    else {

      /* nonpropositional */
	    
      if (max_lits == 1) {
	printf("Setting pick_given_ratio to 2, because all clauses are units.\n");
	auto_change_parm(stdout, PICK_GIVEN_RATIO, 2);
      }
      else {
	/* Nonunit */
	printf("Setting hyper_res, because there are nonunits.\n");
	auto_change_flag(stdout, HYPER_RES, 1);
	if (equality || !horn) {
	  printf("Setting ur_res, because this is a nonunit set containing\n");
          printf("either equality literals or non-Horn clauses.\n");
	  auto_change_flag(stdout, UR_RES, 1);
	}

	if (horn) {
	  printf("Clearing order_hyper, because all clauses are Horn.\n");
	  auto_change_flag(stdout, ORDER_HYPER, 0);
	}
	else {
	  /* Non-Horn */
	  printf("Setting factor and  unit_deletion, because there are non-Horn clauses.\n");
	  auto_change_flag(stdout, FACTOR, 1);
	  auto_change_flag(stdout, UNIT_DELETION, 1);
	}
      }

      if (equality) {
	printf("Equality is present, so we set the knuth_bendix flag.\n");
	auto_change_flag(stdout, KNUTH_BENDIX, 1);
	if (max_lits > 1) {
	  printf("As an incomplete heuristic, we paramodulate with units only.\n");
	  auto_change_flag(stdout, PARA_FROM_UNITS_ONLY, 1);
	  auto_change_flag(stdout, PARA_INTO_UNITS_ONLY, 1);
	}
	if (symmetry) {
	  printf("There is a clause for symmetry of equality, so it is\n");
	  printf("assumed that equality is fully axiomatized; therefore,\n");
	  printf("paramodulation is disabled.\n\n");
	  auto_change_flag(stdout, PARA_FROM, 0);
	  auto_change_flag(stdout, PARA_INTO, 0);
	}
      }
    }
  }
}  /* automatic_2_settings */

/*************
 *
 *   log_for_x_show()
 *
 *   Print some statistics to a file.  This is intended to be used by
 *   some other program that displays statistics about the search in real time.
 *
 *************/

void log_for_x_show(FILE *fp)
{
  fprintf(fp, "given %ld\n", Stats[CL_GIVEN]);
  fprintf(fp, "generated %ld\n", Stats[CL_GENERATED]);
  fprintf(fp, "kept %ld\n", Stats[CL_KEPT]);
  fprintf(fp, "usable %ld\n", Stats[USABLE_SIZE]);
  fprintf(fp, "sos %ld\n", Stats[SOS_SIZE]);
  fprintf(fp, "demods %ld\n", Stats[DEMODULATORS_SIZE]);
  fprintf(fp, "passive %ld\n", Stats[PASSIVE_SIZE]);
  fprintf(fp, "hot %ld\n", Stats[HOT_SIZE]);
  fprintf(fp, "kbytes %ld\n", Stats[K_MALLOCED]);
  fprintf(fp, "wall-time %.2f\n", (double) wall_seconds() - Stats[INIT_WALL_SECONDS]);
  fprintf(fp, "user-time %.2f\n", run_time() / 1000.);
  fprintf(fp, "sys-time %.2f\n", system_time() / 1000.);
  fflush(fp);
}  /* log_for_x_show */

/*************
 *
 *    int same_structure(t1, t2)
 *
 *    Similar to lex_order_vars, except that variables are identical.
 *
 *************/

int same_structure(struct term *t1,
		   struct term *t2)
{
  struct rel *r1, *r2;
  int i;

  if (t1->type == VARIABLE)
    return(t2->type == VARIABLE);
  else if (t2->type == VARIABLE)
    return(0);
  else if (t1->sym_num == t2->sym_num) {
    r1 = t1->farg;
    r2 = t2->farg;
    i = 1;
    while (r1 && (i = same_structure(r1->argval,r2->argval))) {
      r1 = r1->narg;
      r2 = r2->narg;
    }
    return(i);
  }
  else
    return(0);
}  /* same_structure */

/*************
 *
 *    void zap_variable_names(t);
 *
 *************/

void zap_variable_names(struct term *t)
{
  if (t->type == VARIABLE)
    t->sym_num = 0;
  else if (t->type == COMPLEX) {
    struct rel *r;
    for (r = t->farg; r != NULL; r = r->narg)
      zap_variable_names(r->argval);
  }
}  /* zap_variable_names */

/*************
 *
 *    commuted_terms()
 *
 *************/

int commuted_terms(struct term *t1, struct term *t2)
{
  if (t1->type == VARIABLE)
    return (t2->type == VARIABLE && t1->varnum == t2->varnum);
  else if (t1->type == VARIABLE)
    return 0;
  else if (t1->sym_num != t2->sym_num)
    return 0;
  else {
    if (sn_to_arity(t1->sym_num) != 2) {
      struct rel *r1, *r2;
      for (r1 = t1->farg, r2 = t2->farg; r1; r1 = r1->narg, r2 = r2->narg) {
	if (!commuted_terms(r1->argval, r2->argval))
	  return 0;
      }
      return 1;
    }
    else {
      struct term *t1a = t1->farg->argval;
      struct term *t1b = t1->farg->narg->argval;
      struct term *t2a = t2->farg->argval;
      struct term *t2b = t2->farg->narg->argval;
      return
	(commuted_terms(t1a, t2a) && commuted_terms(t1b, t2b)) ||
	(commuted_terms(t1a, t2b) && commuted_terms(t1b, t2a));
    }
  }
}  /* commuted_terms */

/*************
 *
 *    int symbol_count(t);
 *
 *************/

int symbol_count(struct term *t)
{
  if (t->type == VARIABLE || t->type == NAME)
    return 1;
  else {
    struct rel *r;
    int n = 1;
    for (r = t->farg; r != NULL; r = r->narg)
      n += symbol_count(r->argval);
    return n;
  }
}  /* symbol_count */

/*************
 *
 *    commutativity_consequence(struct clause *c)
 *
 *************/

int commutativity_consequence(struct clause *c)
{
  if (!unit_clause(c))
    return 0;
  else {
    struct term *atom = ith_literal(c,1)->atom;
    if (atom->varnum != POS_EQ)
      return 0;
    else {
      struct term *alpha = atom->farg->argval;
      struct term *beta  = atom->farg->narg->argval;
      if (!commuted_terms(alpha, beta))
	return 0;
      else if (symbol_count(alpha) <= 3) {
	return 0;  /* commutativity itself? or x=x */
      }
      else {
	/* printf("discarding: "); p_clause(c); */
	return 1;
      }
    }
  }
}  /* commutativity_consequence */
