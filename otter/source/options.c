/*
 *  options.c -- Routines to manage flags and parameters.
 *
 */

#include "header.h"

/*************
 *
 *    init_options()
 *
 *************/

void init_options(void)
{
  int i;

  for (i = 0; i < MAX_FLAGS; i++)
    Flags[i].name = "";
  for (i = 0; i < MAX_PARMS; i++)
    Parms[i].name = "";

  /* flags are boolean valued options */

  Flags[BINARY_RES].name = "binary_res";
  Flags[BINARY_RES].val = 0;

  Flags[HYPER_RES].name = "hyper_res";
  Flags[HYPER_RES].val = 0;

  Flags[NEG_HYPER_RES].name = "neg_hyper_res";
  Flags[NEG_HYPER_RES].val = 0;

  Flags[UR_RES].name = "ur_res";
  Flags[UR_RES].val = 0;

  Flags[PARA_FROM].name = "para_from";
  Flags[PARA_FROM].val = 0;

  Flags[PARA_INTO].name = "para_into";
  Flags[PARA_INTO].val = 0;

  Flags[PARA_FROM_LEFT].name = "para_from_left";
  Flags[PARA_FROM_LEFT].val = 1;

  Flags[PARA_FROM_RIGHT].name = "para_from_right";
  Flags[PARA_FROM_RIGHT].val = 1;

  Flags[PARA_FROM_VARS].name = "para_from_vars";
  Flags[PARA_FROM_VARS].val = 0;

  Flags[PARA_INTO_VARS].name = "para_into_vars";
  Flags[PARA_INTO_VARS].val = 0;

  Flags[PARA_INTO_LEFT].name = "para_into_left";
  Flags[PARA_INTO_LEFT].val = 1;

  Flags[PARA_INTO_RIGHT].name = "para_into_right";
  Flags[PARA_INTO_RIGHT].val = 1;

  Flags[PARA_ONES_RULE].name = "para_ones_rule";
  Flags[PARA_ONES_RULE].val = 0;

  Flags[PARA_ALL].name = "para_all";
  Flags[PARA_ALL].val = 0;

  Flags[DEMOD_INF].name = "demod_inf";
  Flags[DEMOD_INF].val = 0;

  Flags[DEMOD_LINEAR].name = "demod_linear";
  Flags[DEMOD_LINEAR].val = 0;

  Flags[VERY_VERBOSE].name = "very_verbose";
  Flags[VERY_VERBOSE].val = 0;

  Flags[FOR_SUB_FPA].name = "for_sub_fpa";
  Flags[FOR_SUB_FPA].val = 0;

  Flags[FOR_SUB].name = "for_sub";
  Flags[FOR_SUB].val = 1;

  Flags[BACK_SUB].name = "back_sub";
  Flags[BACK_SUB].val = 1;

  Flags[FREE_ALL_MEM].name = "free_all_mem";
  Flags[FREE_ALL_MEM].val = 0;

  Flags[NO_FAPL].name = "no_fapl";
  Flags[NO_FAPL].val = 0;

  Flags[NO_FANL].name = "no_fanl";
  Flags[NO_FANL].val = 0;

  Flags[FACTOR].name = "factor";
  Flags[FACTOR].val = 0;

  Flags[PRINT_KEPT].name = "print_kept";
  Flags[PRINT_KEPT].val = 1;

  Flags[DEMOD_HISTORY].name = "demod_history";
  Flags[DEMOD_HISTORY].val = 1;

  Flags[UNIT_DELETION].name = "unit_deletion";
  Flags[UNIT_DELETION].val = 0;

  Flags[SORT_LITERALS].name = "sort_literals";
  Flags[SORT_LITERALS].val = 0;

  Flags[PRINT_GIVEN].name = "print_given";
  Flags[PRINT_GIVEN].val = 1;

  Flags[PRINT_BACK_SUB].name = "print_back_sub";
  Flags[PRINT_BACK_SUB].val = 1;

  Flags[CHECK_ARITY].name = "check_arity";
  Flags[CHECK_ARITY].val = 1;

  Flags[SOS_QUEUE].name = "sos_queue";
  Flags[SOS_QUEUE].val = 0;

  Flags[ATOM_WT_MAX_ARGS].name = "atom_wt_max_args";
  Flags[ATOM_WT_MAX_ARGS].val = 0;

  Flags[TERM_WT_MAX_ARGS].name = "term_wt_max_args";
  Flags[TERM_WT_MAX_ARGS].val = 0;

  Flags[PRINT_LISTS_AT_END].name = "print_lists_at_end";
  Flags[PRINT_LISTS_AT_END].val = 0;

  Flags[ORDER_EQ].name = "order_eq";
  Flags[ORDER_EQ].val = 0;

  Flags[DYNAMIC_DEMOD].name = "dynamic_demod";
  Flags[DYNAMIC_DEMOD].val = 0;

  Flags[BACK_DEMOD].name = "back_demod";
  Flags[BACK_DEMOD].val = 0;

  Flags[PRINT_NEW_DEMOD].name = "print_new_demod";
  Flags[PRINT_NEW_DEMOD].val = 1;

  Flags[PRINT_BACK_DEMOD].name = "print_back_demod";
  Flags[PRINT_BACK_DEMOD].val = 1;

  Flags[DEMOD_OUT_IN].name = "demod_out_in";
  Flags[DEMOD_OUT_IN].val = 0;

  Flags[PROCESS_INPUT].name = "process_input";
  Flags[PROCESS_INPUT].val = 0;

  Flags[SIMPLIFY_FOL].name = "simplify_fol";
  Flags[SIMPLIFY_FOL].val = 1;

  Flags[ANL_EQ].name = "anl_eq";
  Flags[ANL_EQ].val = 0;

  Flags[KNUTH_BENDIX].name = "knuth_bendix";
  Flags[KNUTH_BENDIX].val = 0;

  Flags[REWRITER].name = "rewriter";
  Flags[REWRITER].val = 0;

  Flags[PRINT_PROOFS].name = "print_proofs";
  Flags[PRINT_PROOFS].val = 1;

  Flags[SYMBOL_ELIM].name = "symbol_elim";
  Flags[SYMBOL_ELIM].val = 0;

  Flags[LEX_ORDER_VARS].name = "lex_order_vars";
  Flags[LEX_ORDER_VARS].val = 0;

  Flags[DYNAMIC_DEMOD_ALL].name = "dynamic_demod_all";
  Flags[DYNAMIC_DEMOD_ALL].val = 0;

  Flags[DELETE_IDENTICAL_NESTED_SKOLEM].name = "delete_identical_nested_skolem";
  Flags[DELETE_IDENTICAL_NESTED_SKOLEM].val = 0;

  Flags[PARA_FROM_UNITS_ONLY].name = "para_from_units_only";
  Flags[PARA_FROM_UNITS_ONLY].val = 0;

  Flags[PARA_INTO_UNITS_ONLY].name = "para_into_units_only";
  Flags[PARA_INTO_UNITS_ONLY].val = 0;

  Flags[REALLY_DELETE_CLAUSES].name = "really_delete_clauses";
  Flags[REALLY_DELETE_CLAUSES].val = 0;

  Flags[LRPO].name = "lrpo";
  Flags[LRPO].val = 0;

  Flags[PROLOG_STYLE_VARIABLES].name = "prolog_style_variables";
  Flags[PROLOG_STYLE_VARIABLES].val = 0;

  Flags[SOS_STACK].name = "sos_stack";
  Flags[SOS_STACK].val = 0;

  Flags[DYNAMIC_DEMOD_LEX_DEP].name = "dynamic_demod_lex_dep";
  Flags[DYNAMIC_DEMOD_LEX_DEP].val = 0;

  Flags[PROG_SYNTHESIS].name = "prog_synthesis";
  Flags[PROG_SYNTHESIS].val = 0;

  Flags[ANCESTOR_SUBSUME].name = "ancestor_subsume";
  Flags[ANCESTOR_SUBSUME].val = 0;

  Flags[INPUT_SOS_FIRST].name = "input_sos_first";
  Flags[INPUT_SOS_FIRST].val = 0;

  Flags[LINKED_UR_RES].name = "linked_ur_res";
  Flags[LINKED_UR_RES].val = 0;

  Flags[LINKED_UR_TRACE].name = "linked_ur_trace";
  Flags[LINKED_UR_TRACE].val = 0;

  Flags[PARA_SKIP_SKOLEM].name = "para_skip_skolem";
  Flags[PARA_SKIP_SKOLEM].val = 0;

  Flags[INDEX_FOR_BACK_DEMOD].name = "index_for_back_demod";
  Flags[INDEX_FOR_BACK_DEMOD].val = 1;

  Flags[LINKED_SUB_UNIT_USABLE].name = "linked_sub_unit_usable";
  Flags[LINKED_SUB_UNIT_USABLE].val = 0;

  Flags[LINKED_SUB_UNIT_SOS].name = "linked_sub_unit_sos";
  Flags[LINKED_SUB_UNIT_SOS].val = 0;

  Flags[LINKED_UNIT_DEL].name = "linked_unit_del";
  Flags[LINKED_UNIT_DEL].val = 0;

  Flags[LINKED_TARGET_ALL].name = "linked_target_all";
  Flags[LINKED_TARGET_ALL].val = 0;

  Flags[LINKED_HYPER_RES].name = "linked_hyper_res";
  Flags[LINKED_HYPER_RES].val = 0;

  Flags[CONTROL_MEMORY].name = "control_memory";
  Flags[CONTROL_MEMORY].val = 0;

  Flags[ORDER_HISTORY].name = "order_history";
  Flags[ORDER_HISTORY].val = 0;

  Flags[DISPLAY_TERMS].name = "display_terms";
  Flags[DISPLAY_TERMS].val = 0;

  Flags[GEOMETRIC_RULE].name = "geometric_rule";
  Flags[GEOMETRIC_RULE].val = 0;

  Flags[GEOMETRIC_REWRITE_BEFORE].name = "geometric_rewrite_before";
  Flags[GEOMETRIC_REWRITE_BEFORE].val = 0;

  Flags[GEOMETRIC_REWRITE_AFTER].name = "geometric_rewrite_after";
  Flags[GEOMETRIC_REWRITE_AFTER].val = 0;

  Flags[PRETTY_PRINT].name = "pretty_print";
  Flags[PRETTY_PRINT].val = 0;

  Flags[INPUT_SEQUENT].name = "input_sequent";
  Flags[INPUT_SEQUENT].val = 0;

  Flags[OUTPUT_SEQUENT].name = "output_sequent";
  Flags[OUTPUT_SEQUENT].val = 0;

  Flags[ECHO_INCLUDED_FILES].name = "echo_included_files";
  Flags[ECHO_INCLUDED_FILES].val = 1;

  Flags[INTERACTIVE_GIVEN].name = "interactive_given";
  Flags[INTERACTIVE_GIVEN].val = 0;

  Flags[DETAILED_HISTORY].name = "detailed_history";
  Flags[DETAILED_HISTORY].val = 1;

  Flags[ORDER_HYPER].name = "order_hyper";
  Flags[ORDER_HYPER].val = 1;

  Flags[PROPOSITIONAL].name = "propositional";
  Flags[PROPOSITIONAL].val = 0;

  Flags[AUTO].name = "auto";
  Flags[AUTO].val = 0;

  Flags[AUTO1].name = "auto1";
  Flags[AUTO1].val = 0;

  Flags[AUTO2].name = "auto2";
  Flags[AUTO2].val = 0;

  Flags[EQ_UNITS_BOTH_WAYS].name = "eq_units_both_ways";
  Flags[EQ_UNITS_BOTH_WAYS].val = 0;

  Flags[BIRD_PRINT].name = "bird_print";
  Flags[BIRD_PRINT].val = 0;

  Flags[BUILD_PROOF_OBJECT].name = "build_proof_object";
  Flags[BUILD_PROOF_OBJECT].val = 0;

  Flags[BUILD_PROOF_OBJECT_1].name = "build_proof_object_1";
  Flags[BUILD_PROOF_OBJECT_1].val = 0;

  Flags[BUILD_PROOF_OBJECT_2].name = "build_proof_object_2";
  Flags[BUILD_PROOF_OBJECT_2].val = 0;

  Flags[LOG_FOR_X_SHOW].name = "log_for_x_show";
  Flags[LOG_FOR_X_SHOW].val = 0;

  Flags[FORMULA_HISTORY].name = "formula_history";
  Flags[FORMULA_HISTORY].val = 0;

  Flags[KEEP_HINT_SUBSUMERS].name = "keep_hint_subsumers";
  Flags[KEEP_HINT_SUBSUMERS].val = 0;

  Flags[KEEP_HINT_EQUIVALENTS].name = "keep_hint_equivalents";
  Flags[KEEP_HINT_EQUIVALENTS].val = 0;

  Flags[PROOF_WEIGHT].name = "proof_weight";
  Flags[PROOF_WEIGHT].val = 0;

  Flags[HYPER_SYMMETRY_KLUDGE].name = "hyper_symmetry_kludge";
  Flags[HYPER_SYMMETRY_KLUDGE].val = 0;

  Flags[GL_DEMOD].name = "gl_demod";
  Flags[GL_DEMOD].val = 0;

  Flags[DISCARD_NON_ORIENTABLE_EQ].name = "discard_non_orientable_eq";
  Flags[DISCARD_NON_ORIENTABLE_EQ].val = 0;

  Flags[DISCARD_XX_RESOLVABLE].name = "discard_xx_resolvable";
  Flags[DISCARD_XX_RESOLVABLE].val = 0;

  Flags[TPTP_EQ].name = "tptp_eq";
  Flags[TPTP_EQ].val = 0;

  Flags[BELL].name = "bell";
  Flags[BELL].val = 1;  /* Do not set default to 0 */

  Flags[BACK_UNIT_DELETION].name = "back_unit_deletion";
  Flags[BACK_UNIT_DELETION].val = 0;

  Flags[SPLIT_CLAUSE].name = "split_clause";
  Flags[SPLIT_CLAUSE].val = 0;

  Flags[SPLIT_POS].name = "split_pos";
  Flags[SPLIT_POS].val = 0;

  Flags[SPLIT_NEG].name = "split_neg";
  Flags[SPLIT_NEG].val = 0;

  Flags[SPLIT_NONHORN].name = "split_nonhorn";
  Flags[SPLIT_NONHORN].val = 0;

  Flags[SPLIT_MIN_MAX].name = "split_min_max";
  Flags[SPLIT_MIN_MAX].val = 0;

  Flags[SPLIT_ATOM].name = "split_atom";
  Flags[SPLIT_ATOM].val = 0;

  Flags[SPLIT_POPULAR].name = "split_popular";
  Flags[SPLIT_POPULAR].val = 0;

  Flags[SPLIT_WHEN_GIVEN].name = "split_when_given";
  Flags[SPLIT_WHEN_GIVEN].val = 0;

  Flags[UNIT_RES].name = "unit_res";
  Flags[UNIT_RES].val = 0;

  Flags[SIGINT_INTERACT].name = "sigint_interact";
  Flags[SIGINT_INTERACT].val = 1;

  Flags[UR_LAST].name = "ur_last";
  Flags[UR_LAST].val = 0;

  Flags[LITERALS_WEIGH_ONE].name = "literals_weigh_one";
  Flags[LITERALS_WEIGH_ONE].val = 0;

  Flags[PICK_DIFF_SIM].name = "pick_diff_sim";
  Flags[PICK_DIFF_SIM].val = 0;

  Flags[PICK_RANDOM_LIGHTEST].name = "pick_random_lightest";
  Flags[PICK_RANDOM_LIGHTEST].val = 0;

  Flags[PICK_LAST_LIGHTEST].name = "pick_last_lightest";
  Flags[PICK_LAST_LIGHTEST].val = 0;

  Flags[PICK_MID_LIGHTEST].name = "pick_mid_lightest";
  Flags[PICK_MID_LIGHTEST].val = 0;

  Flags[FOR_SUB_EQUIVALENTS_ONLY].name = "for_sub_equivalents_only";
  Flags[FOR_SUB_EQUIVALENTS_ONLY].val = 0;

  Flags[MULTI_JUST].name = "multi_just";
  Flags[MULTI_JUST].val = 0;

  Flags[MULTI_JUST_LESS].name = "multi_just_less";
  Flags[MULTI_JUST_LESS].val = 0;

  Flags[MULTI_JUST_SHORTER].name = "multi_just_shorter";
  Flags[MULTI_JUST_SHORTER].val = 0;

  Flags[PRINT_PROOF_AS_HINTS].name = "print_proof_as_hints";
  Flags[PRINT_PROOF_AS_HINTS].val = 0;

  Flags[DISCARD_COMMUTATIVITY_CONSEQUENCES].name = "discard_commutativity_consequences";
  Flags[DISCARD_COMMUTATIVITY_CONSEQUENCES].val = 0;

  Flags[DEGRADE_HINTS2].name = "degrade_hints2";
  Flags[DEGRADE_HINTS2].val = 0;

  Flags[CLOCKS].name = "clocks";
  Flags[CLOCKS].val = 0;

  /* parms are integer valued options */

  Parms[FPA_LITERALS].name = "fpa_literals";
  Parms[FPA_LITERALS].val = 8;
  Parms[FPA_LITERALS].min = 0;
  Parms[FPA_LITERALS].max = 100;  /* check MAX_PATH before increasing */

  Parms[FPA_TERMS].name = "fpa_terms";
  Parms[FPA_TERMS].val = 8;
  Parms[FPA_TERMS].min = 0;
  Parms[FPA_TERMS].max = 100;  /* check MAX_PATH before increasing */

  Parms[DEMOD_LIMIT].name = "demod_limit";
  Parms[DEMOD_LIMIT].val = 1000;
  Parms[DEMOD_LIMIT].min = -1;
  Parms[DEMOD_LIMIT].max = MAX_INT;

  Parms[MAX_WEIGHT].name = "max_weight";
  Parms[MAX_WEIGHT].val = MAX_INT;
  Parms[MAX_WEIGHT].min = -MAX_INT;
  Parms[MAX_WEIGHT].max =  MAX_INT;

  Parms[MAX_GIVEN].name = "max_given";
  Parms[MAX_GIVEN].val = -1;
  Parms[MAX_GIVEN].min = -1;
  Parms[MAX_GIVEN].max = MAX_INT;

  Parms[MAX_LEVELS].name = "max_levels";
  Parms[MAX_LEVELS].val = -1;
  Parms[MAX_LEVELS].min = -1;
  Parms[MAX_LEVELS].max = MAX_INT;

  Parms[MAX_SECONDS].name = "max_seconds";
  Parms[MAX_SECONDS].val = -1;
  Parms[MAX_SECONDS].min = -1;
  Parms[MAX_SECONDS].max = MAX_INT;

  Parms[NEG_WEIGHT].name = "neg_weight";
  Parms[NEG_WEIGHT].val = 0;
  Parms[NEG_WEIGHT].min = -MAX_INT;
  Parms[NEG_WEIGHT].max =  MAX_INT;

  Parms[MAX_KEPT].name = "max_kept";
  Parms[MAX_KEPT].val = -1;
  Parms[MAX_KEPT].min = -1;
  Parms[MAX_KEPT].max = MAX_INT;

  Parms[MAX_GEN].name = "max_gen";
  Parms[MAX_GEN].val = -1;
  Parms[MAX_GEN].min = -1;
  Parms[MAX_GEN].max = MAX_INT;

  Parms[MAX_MEM].name = "max_mem";
  Parms[MAX_MEM].val = -1;
  Parms[MAX_MEM].min = -1;
  Parms[MAX_MEM].max = MAX_INT;

  Parms[MAX_LITERALS].name = "max_literals";
  Parms[MAX_LITERALS].val = -1;
  Parms[MAX_LITERALS].min = -1;
  Parms[MAX_LITERALS].max = MAX_INT;

  Parms[REPORT].name = "report";
  Parms[REPORT].val = -1;
  Parms[REPORT].min = -1;
  Parms[REPORT].max = MAX_INT;

  Parms[MAX_PROOFS].name = "max_proofs";
  Parms[MAX_PROOFS].val = 1;
  Parms[MAX_PROOFS].min = -1;
  Parms[MAX_PROOFS].max = MAX_INT;

  Parms[STATS_LEVEL].name = "stats_level";
  Parms[STATS_LEVEL].val = 2;
  Parms[STATS_LEVEL].min = 0;
  Parms[STATS_LEVEL].max = 4;

  Parms[MAX_UR_DEPTH].name = "max_ur_depth";
  Parms[MAX_UR_DEPTH].val = 5;
  Parms[MAX_UR_DEPTH].min = 0;
  Parms[MAX_UR_DEPTH].max = 100;

  Parms[MAX_UR_DEDUCTION_SIZE].name = "max_ur_deduction_size";
  Parms[MAX_UR_DEDUCTION_SIZE].val = 20;
  Parms[MAX_UR_DEDUCTION_SIZE].min = 0;
  Parms[MAX_UR_DEDUCTION_SIZE].max = 100;

  Parms[MAX_DISTINCT_VARS].name = "max_distinct_vars";
  Parms[MAX_DISTINCT_VARS].val = -1;
  Parms[MAX_DISTINCT_VARS].min = -1;
  Parms[MAX_DISTINCT_VARS].max = MAX_INT;

  Parms[PICK_GIVEN_RATIO].name = "pick_given_ratio";
  Parms[PICK_GIVEN_RATIO].val = -1;
  Parms[PICK_GIVEN_RATIO].min = -1;
  Parms[PICK_GIVEN_RATIO].max = MAX_INT;

  Parms[CHANGE_LIMIT_AFTER].name = "change_limit_after";
  Parms[CHANGE_LIMIT_AFTER].val = 0;
  Parms[CHANGE_LIMIT_AFTER].min = 0;
  Parms[CHANGE_LIMIT_AFTER].max = MAX_INT;

  Parms[NEW_MAX_WEIGHT].name = "new_max_weight";
  Parms[NEW_MAX_WEIGHT].val = MAX_INT;
  Parms[NEW_MAX_WEIGHT].min = -MAX_INT;
  Parms[NEW_MAX_WEIGHT].max = MAX_INT;

  Parms[GEO_GIVEN_RATIO].name = "geo_given_ratio";
  Parms[GEO_GIVEN_RATIO].val = 1;
  Parms[GEO_GIVEN_RATIO].min = -1;
  Parms[GEO_GIVEN_RATIO].max = MAX_INT;

  Parms[PRETTY_PRINT_INDENT].name = "pretty_print_indent";
  Parms[PRETTY_PRINT_INDENT].val = 4;
  Parms[PRETTY_PRINT_INDENT].min = 0;
  Parms[PRETTY_PRINT_INDENT].max = 16;

  Parms[MIN_BIT_WIDTH].name = "min_bit_width";
  Parms[MIN_BIT_WIDTH].val = sizeof(unsigned long) * CHAR_BIT;
  Parms[MIN_BIT_WIDTH].min = 1;
  Parms[MIN_BIT_WIDTH].max = sizeof(unsigned long) * CHAR_BIT;

  Parms[INTERRUPT_GIVEN].name = "interrupt_given";
  Parms[INTERRUPT_GIVEN].val = -1;
  Parms[INTERRUPT_GIVEN].min = -1;
  Parms[INTERRUPT_GIVEN].max = MAX_INT;

  Parms[HEAT].name = "heat";
  Parms[HEAT].val = 1;
  Parms[HEAT].min = 0;
  Parms[HEAT].max = 100;

  Parms[DYNAMIC_HEAT_WEIGHT].name = "dynamic_heat_weight";
  Parms[DYNAMIC_HEAT_WEIGHT].val = -MAX_INT;
  Parms[DYNAMIC_HEAT_WEIGHT].min = -MAX_INT;
  Parms[DYNAMIC_HEAT_WEIGHT].max = MAX_INT;

  Parms[MAX_ANSWERS].name = "max_answers";
  Parms[MAX_ANSWERS].val = -1;
  Parms[MAX_ANSWERS].min = -1;
  Parms[MAX_ANSWERS].max = MAX_INT;

  Parms[DEBUG_FIRST].name = "debug_first";
  Parms[DEBUG_FIRST].val = 0;
  Parms[DEBUG_FIRST].min = 0;
  Parms[DEBUG_FIRST].max = MAX_INT;

  Parms[DEBUG_LAST].name = "debug_last";
  Parms[DEBUG_LAST].val = -1;
  Parms[DEBUG_LAST].min = -1;
  Parms[DEBUG_LAST].max = MAX_INT;

  Parms[FSUB_HINT_ADD_WT].name = "fsub_hint_add_wt";
  Parms[FSUB_HINT_ADD_WT].val = 0;
  Parms[FSUB_HINT_ADD_WT].min = -MAX_INT;
  Parms[FSUB_HINT_ADD_WT].max = MAX_INT;

  Parms[BSUB_HINT_ADD_WT].name = "bsub_hint_add_wt";
  Parms[BSUB_HINT_ADD_WT].val = 0;
  Parms[BSUB_HINT_ADD_WT].min = -MAX_INT;
  Parms[BSUB_HINT_ADD_WT].max = MAX_INT;

  Parms[EQUIV_HINT_ADD_WT].name = "equiv_hint_add_wt";
  Parms[EQUIV_HINT_ADD_WT].val = 0;
  Parms[EQUIV_HINT_ADD_WT].min = -MAX_INT;
  Parms[EQUIV_HINT_ADD_WT].max = MAX_INT;

  Parms[FSUB_HINT_WT].name = "fsub_hint_wt";
  Parms[FSUB_HINT_WT].val = MAX_INT;
  Parms[FSUB_HINT_WT].min = -MAX_INT;
  Parms[FSUB_HINT_WT].max = MAX_INT;

  Parms[BSUB_HINT_WT].name = "bsub_hint_wt";
  Parms[BSUB_HINT_WT].val = MAX_INT;
  Parms[BSUB_HINT_WT].min = -MAX_INT;
  Parms[BSUB_HINT_WT].max = MAX_INT;

  Parms[EQUIV_HINT_WT].name = "equiv_hint_wt";
  Parms[EQUIV_HINT_WT].val = MAX_INT;
  Parms[EQUIV_HINT_WT].min = -MAX_INT;
  Parms[EQUIV_HINT_WT].max = MAX_INT;

  Parms[VERBOSE_DEMOD_SKIP].name = "verbose_demod_skip";
  Parms[VERBOSE_DEMOD_SKIP].val = 0;
  Parms[VERBOSE_DEMOD_SKIP].min = 0;
  Parms[VERBOSE_DEMOD_SKIP].max = MAX_INT;

  Parms[DYNAMIC_DEMOD_DEPTH].name = "dynamic_demod_depth";
  Parms[DYNAMIC_DEMOD_DEPTH].val = -1;
  Parms[DYNAMIC_DEMOD_DEPTH].min = -1;
  Parms[DYNAMIC_DEMOD_DEPTH].max = MAX_INT;

  Parms[DYNAMIC_DEMOD_RHS].name = "dynamic_demod_rhs";
  Parms[DYNAMIC_DEMOD_RHS].val = 1;
  Parms[DYNAMIC_DEMOD_RHS].min = -MAX_INT;
  Parms[DYNAMIC_DEMOD_RHS].max = MAX_INT;

  Parms[AGE_FACTOR].name = "age_factor";
  Parms[AGE_FACTOR].val = 0;
  Parms[AGE_FACTOR].min = -MAX_INT;
  Parms[AGE_FACTOR].max = MAX_INT;

  Parms[DISTINCT_VARS_FACTOR].name = "distinct_vars_factor";
  Parms[DISTINCT_VARS_FACTOR].val = 0;
  Parms[DISTINCT_VARS_FACTOR].min = -MAX_INT;
  Parms[DISTINCT_VARS_FACTOR].max = MAX_INT;

  Parms[NEW_SYMBOL_LEX_POSITION].name = "new_symbol_lex_position";
  Parms[NEW_SYMBOL_LEX_POSITION].val = MAX_INT / 2;
  Parms[NEW_SYMBOL_LEX_POSITION].min = 1;
  Parms[NEW_SYMBOL_LEX_POSITION].max = MAX_INT / 2;

  Parms[WARN_MEM].name = "warn_mem";
  Parms[WARN_MEM].val = -1;
  Parms[WARN_MEM].min = -1;
  Parms[WARN_MEM].max = MAX_INT;

  Parms[WARN_MEM_MAX_WEIGHT].name = "warn_mem_max_weight";
  Parms[WARN_MEM_MAX_WEIGHT].val = MAX_INT;
  Parms[WARN_MEM_MAX_WEIGHT].min = -MAX_INT;
  Parms[WARN_MEM_MAX_WEIGHT].max = MAX_INT;

  Parms[SPLIT_SECONDS].name = "split_seconds";
  Parms[SPLIT_SECONDS].val = -1;
  Parms[SPLIT_SECONDS].min = -1;
  Parms[SPLIT_SECONDS].max = MAX_INT;

  Parms[SPLIT_GIVEN].name = "split_given";
  Parms[SPLIT_GIVEN].val = 5;
  Parms[SPLIT_GIVEN].min = -1;
  Parms[SPLIT_GIVEN].max = MAX_INT;

  Parms[SPLIT_DEPTH].name = "split_depth";
  Parms[SPLIT_DEPTH].val = max_split_depth();
  Parms[SPLIT_DEPTH].min = 1;
  Parms[SPLIT_DEPTH].max = max_split_depth();

  Parms[PICK_DIFF].name = "pick_diff";
  Parms[PICK_DIFF].val = -1;
  Parms[PICK_DIFF].min = -1;
  Parms[PICK_DIFF].max = MAX_INT;

  Parms[PICK_DIFF_RANGE].name = "pick_diff_range";
  Parms[PICK_DIFF_RANGE].val = 0;
  Parms[PICK_DIFF_RANGE].min = 0;
  Parms[PICK_DIFF_RANGE].max = MAX_INT;

  Parms[MULTI_JUST_MAX].name = "multi_just_max";
  Parms[MULTI_JUST_MAX].val = -1;
  Parms[MULTI_JUST_MAX].min = -1;
  Parms[MULTI_JUST_MAX].max = MAX_INT;

}  /* init_options */

/*************
 *
 *    print_options(fp)
 *
 *************/

void print_options(FILE *fp)
{
  int i, j;

  fprintf(fp, "\n--------------- options ---------------\n");

  j = 0;
  for (i = 0; i < MAX_FLAGS; i++)  /* print set flags */
    if (Flags[i].name[0] != '\0') {
      fprintf(fp, "%s", Flags[i].val ? "set(" : "clear(");
      fprintf(fp, "%s). ", Flags[i].name);
      j++;
      if (j % 3 == 0)
	fprintf(fp, "\n");
    }

  fprintf(fp, "\n\n");

  j = 0;
  for (i = 0; i < MAX_PARMS; i++)  /* print parms */
    if (Parms[i].name[0] != '\0') {
      fprintf(fp, "assign(");
      fprintf(fp, "%s, %d). ", Parms[i].name, Parms[i].val);
      j++;
      if (j % 3 == 0)
	fprintf(fp, "\n");
    }
  fprintf(fp, "\n");

}  /* print_options */

/*************
 *
 *    p_options()
 *
 *************/

void p_options(void)
{
  print_options(stdout);
}  /* p_options */

/*************
 *
 *   auto_change_flag()
 *
 *************/

void auto_change_flag(FILE *fp,
		      int index,
		      int val)
{
  if (Flags[index].val != val) {
    fprintf(fp, "   dependent: %s(%s).\n",
	    val ? "set" : "clear", Flags[index].name);
    Flags[index].val = val;
    dependent_flags(fp, index);
  }
}  /* auto_change_flag */

/*************
 *
 *   void dependent_flags(FILE *fp, int index)
 *
 *   Flag[index] has just been changed.  Change any flags or parms that
 *   depend on it.  Write actions to *fp.
 *
 *   Mutually recursive with auto_change_flag and auto_change_parm.
 *
 *************/

void dependent_flags(FILE *fp,
		     int index)
{
  /* This part handles flags that have just been set. */

  if (Flags[index].val) {

    switch (index) {
    case KNUTH_BENDIX:
      auto_change_flag(fp, ANL_EQ, 1);
      break;
    case ANL_EQ:
      auto_change_flag(fp, PARA_FROM, 1);
      auto_change_flag(fp, PARA_INTO, 1);
      auto_change_flag(fp, PARA_FROM_LEFT, 1);
      auto_change_flag(fp, PARA_FROM_RIGHT, 0);
      auto_change_flag(fp, PARA_INTO_LEFT, 1);
      auto_change_flag(fp, PARA_INTO_RIGHT, 0);
      auto_change_flag(fp, PARA_FROM_VARS, 1);
      auto_change_flag(fp, EQ_UNITS_BOTH_WAYS, 1);
      auto_change_flag(fp, DYNAMIC_DEMOD_ALL, 1);
      auto_change_flag(fp, BACK_DEMOD, 1);
      auto_change_flag(fp, PROCESS_INPUT, 1);
      auto_change_flag(fp, LRPO, 1);
      break;
    case BACK_DEMOD:
      auto_change_flag(fp, DYNAMIC_DEMOD, 1);
      break;
    case DYNAMIC_DEMOD_ALL:
      auto_change_flag(fp, DYNAMIC_DEMOD, 1);
      break;
    case DYNAMIC_DEMOD:
      auto_change_flag(fp, ORDER_EQ, 1);
      break;
    case BINARY_RES:
      auto_change_flag(fp, FACTOR, 1);
      auto_change_flag(fp, UNIT_DELETION, 1);
      break;
    case UNIT_RES:
      auto_change_flag(fp, BINARY_RES, 1);
      break;
    case VERY_VERBOSE:
      auto_change_flag(fp, PRINT_KEPT, 1);
      break;
    case PARA_ALL:
      auto_change_flag(fp, DETAILED_HISTORY, 0);
      break;
    case PROPOSITIONAL:
      auto_change_flag(fp, SORT_LITERALS, 1);
      auto_change_flag(fp, PROCESS_INPUT, 1);
      break;
    case AUTO1:  /* original auto mode (version 3.0.4) */
      auto_change_flag(fp, PROCESS_INPUT, 1);
      auto_change_flag(fp, PRINT_KEPT, 0);
      auto_change_flag(fp, PRINT_NEW_DEMOD, 0);
      auto_change_flag(fp, PRINT_BACK_DEMOD, 0);
      auto_change_flag(fp, PRINT_BACK_SUB, 0);
      auto_change_flag(fp, CONTROL_MEMORY, 1);
      auto_change_parm(fp, MAX_MEM, 12000);
      auto_change_parm(fp, PICK_GIVEN_RATIO, 4);
      auto_change_parm(fp, STATS_LEVEL, 1);
      auto_change_parm(fp, MAX_SECONDS, 10800);
      /* other options are set after clauses are read */
      break;
    case AUTO2:  /* revised auto mode (version 3.0.5) */
      auto_change_flag(fp, PROCESS_INPUT, 1);
      auto_change_flag(fp, PRINT_KEPT, 0);
      auto_change_flag(fp, PRINT_NEW_DEMOD, 0);
      auto_change_flag(fp, PRINT_BACK_DEMOD, 0);
      auto_change_flag(fp, PRINT_BACK_SUB, 0);
      auto_change_flag(fp, CONTROL_MEMORY, 1);
      auto_change_parm(fp, MAX_MEM, 20000);
      auto_change_parm(fp, PICK_GIVEN_RATIO, 4);
      auto_change_parm(fp, STATS_LEVEL, 1);
      auto_change_parm(fp, MAX_SECONDS, 10800);
      /* other options are set after clauses are read */
      break;
    case AUTO:  /* selects current auto mode */
      auto_change_flag(fp, AUTO1, 1);
      break;
    case BUILD_PROOF_OBJECT_1:
    case BUILD_PROOF_OBJECT_2:
      auto_change_flag(fp, ORDER_HISTORY, 1);
      auto_change_flag(fp, DETAILED_HISTORY, 1);
      break;
    case BUILD_PROOF_OBJECT:
      auto_change_flag(fp, BUILD_PROOF_OBJECT_1, 1);
      break;
    case SPLIT_CLAUSE:
    case SPLIT_ATOM:
    case SPLIT_WHEN_GIVEN:
      auto_change_flag(fp, BACK_UNIT_DELETION, 1);
      auto_change_parm(fp, REPORT, -1);
      break;
    case BACK_UNIT_DELETION:
      auto_change_flag(fp, UNIT_DELETION, 1);
      break;
    case PRINT_PROOF_AS_HINTS:
      auto_change_flag(fp, BUILD_PROOF_OBJECT_1, 1);
      break;
    case REWRITER:
      auto_change_flag(fp, DEMOD_INF, 1);
      auto_change_parm(fp, MAX_LEVELS, 1);
      auto_change_flag(fp, FOR_SUB, 0);
      auto_change_flag(fp, BACK_SUB, 0);
      auto_change_parm(fp, STATS_LEVEL, 0);
      break;
    }
  }

  /* This part handles flags that have just been cleared. */

  if (!Flags[index].val) {
    switch (index) {
    }
  }

  /* This part handles flags that have just been cleared. */

}  /* dependent_flags */

/*************
 *
 *   auto_change_parm()
 *
 *************/

void auto_change_parm(FILE *fp,
		      int index,
		      int val)
{
  if (Parms[index].val != val) {
    fprintf(fp, "   dependent: assign(%s, %d).\n",
	    Parms[index].name, val);
		
    Parms[index].val = val;
    dependent_parms(fp, index);
  }
}  /* auto_change_parm */

/*************
 *
 *   void dependent_parms(FILE *fp, int index)
 *
 *   Parms[index] has just been changed.  Change any flags or parms that
 *   depend on it.  Write actions to *fp.
 *
 *   Mutually recursive with auto_change_flag and auto_change_parm.
 *
 *   This routine may be empty.
 *
 *************/

void dependent_parms(FILE *fp,
		     int index)
{
  switch (index) {
    case MAX_LEVELS:
      auto_change_flag(fp, SOS_QUEUE, 1);
      break;
  }
}  /* dependent_parms */

/*************
 *
 *    int change_flag(fp, term, set)
 *
 *    Assume term is COMPLEX, with either `set' or `clear' as functor.
 *
 *    If success, return index of flag, if fail, return -1.
 *    Warning and error messages go to file fp.
 *
 *************/

int change_flag(FILE *fp,
		struct term *t,
		int set)
{
  char *flag_name;
  int index, found;

  if (t->farg == NULL || t->farg->narg != NULL ||
      t->farg->argval->type == COMPLEX) {
    fprintf(fp, "ERROR: ");
    print_term(fp, t);
    fprintf(fp, " must have one simple argument.\n");
    Stats[INPUT_ERRORS]++;
    return(-1);
  }
  else {
    flag_name = sn_to_str(t->farg->argval->sym_num);
    found = 0;
    index = 0;
    while (index < MAX_FLAGS && !found)
      if (str_ident(flag_name, Flags[index].name))
	found = 1;
      else
	index++;
    if (!found) {
      fprintf(fp, "ERROR: ");
      print_term(fp, t);
      fprintf(fp, " flag name not found.\n");
      Stats[INPUT_ERRORS]++;
      if (str_ident(flag_name, "lex_rpo"))
	fprintf(stderr, "\nERROR, flag `lex_rpo\' has been changed to `lrpo\'.\n");
      else if (str_ident(flag_name, "print_level"))
	fprintf(stderr, "\nERROR, flag `print_level\' no longer exists.\n");
      else if (str_ident(flag_name, "new_functions"))
	fprintf(stderr, "\nERROR, flag `new_functions\' no longer exists.\n");
      else if (str_ident(flag_name, "bird_print"))
	fprintf(stderr, "\nERROR, flag `bird_print\' no longer exists.\n");
      return(-1);
    }
    else if (Flags[index].val == set) {
      fprintf(fp, "WARNING: ");
      print_term(fp, t);
      if (set)
	fprintf(fp, " flag already set.\n");
      else
	fprintf(fp, " flag already clear.\n");
      return(index);
    }
    else {
      Flags[index].val = set;
      if (index == BELL)
	Bell = set ? '\007' : '\000';
      return(index);
    }
  }
}  /* change_flag */

/*************
 *
 *    int change_parm(fp, term)
 *
 *    Assume term is COMPLEX, with either `assign' as functor.
 *
 *    If success, return index of parm, if fail, return -1.
 *    Warning and error messages go to file fp.
 *
 *************/

int change_parm(FILE *fp,
		struct term *t)
{
  char *parm_name, *int_name;
  int index, found, new_val, rc;

  if (t->farg == NULL || t->farg->narg == NULL ||
      t->farg->narg->narg != NULL ||
      t->farg->argval->type == COMPLEX ||
      t->farg->narg->argval->type == COMPLEX) {
    fprintf(fp, "ERROR: ");
    print_term(fp, t);
    fprintf(fp, " must have two simple arguments.\n");
    Stats[INPUT_ERRORS]++;
    return(-1);
  }
  else {
    parm_name = sn_to_str(t->farg->argval->sym_num);
    found = 0;
    index = 0;
    while (index < MAX_PARMS && !found)
      if (str_ident(parm_name, Parms[index].name))
	found = 1;
      else
	index++;
    if (!found) {
      fprintf(fp, "ERROR: ");
      print_term(fp, t);
      fprintf(fp, " parameter name not found.\n");
      Stats[INPUT_ERRORS]++;
      if (str_ident(parm_name, "reduce_weight_limit")) {
	fprintf(stderr, "\nERROR, parameter `reduce_weight_limit\' has been changed to\n");
	fprintf(stderr, "the pair `change_limit_after\' and `new_max_weight\'.\n");
      }
      return(-1);
    }
    else {
      int_name = sn_to_str(t->farg->narg->argval->sym_num);
      rc = str_int(int_name, &new_val);
      if (rc == 0) {
	fprintf(fp, "ERROR: ");
	print_term(fp, t);
	fprintf(fp, " second argument must be integer.\n");
	Stats[INPUT_ERRORS]++;
	return(-1);
      }
      else if (new_val < Parms[index].min || new_val > Parms[index].max) {
	fprintf(fp, "ERROR: ");
	print_term(fp, t);
	fprintf(fp, " integer must be in range [%d,%d].\n",
		Parms[index].min, Parms[index].max);
	Stats[INPUT_ERRORS]++;
	return(-1);
      }
      else if (new_val == Parms[index].val) {
	fprintf(fp, "WARNING: ");
	print_term(fp, t);
	fprintf(fp, " already has that value.\n");
	return(index);
      }
      else {
	Parms[index].val = new_val;
	return(index);
      }
    }
  }
}  /* change_parm */

/*************
 *
 *    check_options()  --  check for inconsistent or odd settings
 *
 *    If a bad combination of settings is found, either a warning
 *    message is printed, or an ABEND occurs.
 *
 *************/

void check_options(void)
{
  if (Flags[BINARY_RES].val == 0 &&
      Flags[HYPER_RES].val == 0 &&
      Flags[NEG_HYPER_RES].val == 0 &&
      Flags[UR_RES].val == 0 &&
      Flags[PARA_FROM].val == 0 &&
      Flags[PARA_INTO].val == 0 &&
      Flags[DEMOD_INF].val == 0 &&
      Flags[GEOMETRIC_RULE].val == 0 &&
      Flags[LINKED_UR_RES].val == 0
      )
    fprintf(stderr, "\nWARNING: no inference rules are set.\n");
  if (Flags[PARA_FROM].val &&
      Flags[PARA_FROM_RIGHT].val == 0 && Flags[PARA_FROM_LEFT].val == 0) {
    fprintf(stderr, "\nWARNING: PARA_FROM is set, but PARA_FROM_LEFT and\nPARA_FROM_RIGHT are both clear.\n");
  }
  if (Flags[PARA_INTO].val &&
      Flags[PARA_FROM_RIGHT].val == 0 && Flags[PARA_FROM_LEFT].val == 0) {
    fprintf(stderr, "\nWARNING: PARA_INTO is set, but PARA_FROM_LEFT and\n");
    fprintf(stderr, "PARA_FROM_RIGHT are both clear.\n");
  }

  if (Flags[PARA_FROM].val == 0 && Flags[PARA_INTO].val == 0 && Flags[PARA_ONES_RULE].val)
    fprintf(stderr, "\nWARNING: PARA_FROM, PARA_INTO rules are clear, but PARA_ONES_RULE is set.\n");

  if (Flags[NO_FAPL].val && Flags[HYPER_RES].val == 0)
    fprintf(stderr, "\nWARNING: NO_FAPL is set, but HYPER_RES is clear.\n");
  if (Flags[NO_FAPL].val && Flags[FOR_SUB_FPA].val)
    fprintf(stderr, "\nWARNING: NO_FAPL and FOR_SUB_FPA are both set.\n");
  if (Flags[NO_FAPL].val && Flags[BACK_SUB].val)
    fprintf(stderr, "\nWARNING: NO_FAPL and BACK_SUB are both set.\n");
  if (Flags[ANL_EQ].val && Flags[LRPO].val == 0)
    fprintf(stderr, "\nWARNING: ANL_EQ is set and LRPO is clear.\n");

  if (Parms[DEMOD_LIMIT].val == 0)
    fprintf(stderr, "\nWARNING: demod_limit=0; set it to -1 for no limit.\n");

  if (Parms[MAX_LITERALS].val == 0)
    fprintf(stderr, "\nWARNING: max_literals=0; set it to -1 for no limit.\n");

  if (Parms[MAX_PROOFS].val == 0)
    fprintf(stderr, "\nWARNING: max_proofs=0; set it to -1 for no limit.\n");

  /* selecting the given clause */

  if (Flags[INTERACTIVE_GIVEN].val) {
    if ( (Parms[PICK_GIVEN_RATIO].val != -1) ||
	 (Flags[SOS_STACK].val) ||
	 (Flags[SOS_QUEUE].val) ) {
      fprintf(stderr,"WARNING: INTERACTIVE_GIVEN has highest precedence\n");
      fprintf(stderr,"         for picking given clause.\n");
    }
  }

  if (Parms[PICK_GIVEN_RATIO].val != -1) {
    if (Flags[SOS_STACK].val)
      fprintf(stderr,"\nWARNING: SOS_STACK has priority over PICK_GIVEN_RATIO.\n");
    else if (Flags[SOS_QUEUE].val)
      fprintf(stderr,"\nWARNING: SOS_QUEUE has priority over PICK_GIVEN_RATIO.\n");
  }

  if (Flags[SOS_STACK].val && Flags[SOS_QUEUE].val)
    fprintf(stderr, "\nWARNING, SOS_QUEUE has priority over SOS_STACK.\n");

  if (Flags[SOS_STACK].val && Flags[INPUT_SOS_FIRST].val)
    fprintf(stderr, "\nWARNING, INPUT_SOS_FIRST ignored, because SOS_STACK is set.\n");

  if (Flags[PARA_ALL].val && Flags[DETAILED_HISTORY].val)
    fprintf(stderr, "\nWARNING, detailed paramod history is ignored when para_all is set.\n");
  if ((Flags[SPLIT_CLAUSE].val || 
       Flags[SPLIT_ATOM].val || 
       Flags[SPLIT_WHEN_GIVEN].val)
      && Parms[MAX_SECONDS].val != -1)
    fprintf(stderr, "\nWARNING, with splitting, max_seconds is checked against the wall clock.\n");

  /* BV(970327) */
  if (Flags[KEEP_HINT_SUBSUMERS].val && Flags[KEEP_HINT_EQUIVALENTS].val)
    fprintf(stderr, "\nWARNING, keep_hint_subsumers is ignored when keep_hint_equivalents is set.\n");

  if (Flags[ANCESTOR_SUBSUME].val && Flags[MULTI_JUST].val)
    abend("ancestor_subsume and multi_just are incompatible");
}  /* check_options */

