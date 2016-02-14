/*
 *  cos.h -- preprocessor definitions of indices for arrays of
 *  flags, parameters, statistics, clocks, and internal flags.
 *
 */

/*************
 *
 *    Flags are boolean valued options.  To install a new flag, append
 *    a new name and index to the end of this list, then insert code to
 *    initialize it in the routine `init_options'.
 *    Example access:  if (Flags[PARA_FROM_LEFT].val) {
 *    See routine `init_options' for defaults.
 *
 *************/

#define MAX_FLAGS 200  /* This must hold the following list! */

enum {
  SOS_QUEUE,  /* pick first clause on sos as given clause */
  SOS_STACK,  /* pick last sos clause as given clause */
  INPUT_SOS_FIRST,  /* use input sos before generated sos */
  INTERACTIVE_GIVEN,  /* user selects given clause interactively */
  PRINT_GIVEN,  /* print given clauses */
  PRINT_LISTS_AT_END,  /* print clause lists at end of run */

  BINARY_RES,  /* binary resolution */
  HYPER_RES,  /* hyperresolution */
  NEG_HYPER_RES,  /* negatve hyperresolution inference rule */
  UR_RES,  /* UR-resolution */
  PARA_INTO,  /* `into' paramodulation inference rule */
  PARA_FROM,  /* `from' paramodulation inference rule */
  DEMOD_INF,  /* apply demodulation as an inference rule */

  ORDER_HYPER,  /* ordered hyperresolution (satellites) */
  UNIT_RES,  /* Unit resolution restriction */
  UR_LAST,  /* restrict UR: target literal is last */

  PARA_FROM_LEFT,  /* allow paramodulation from left sides */
  PARA_FROM_RIGHT,  /* allow paramodulation from right sides */
  PARA_INTO_LEFT,  /* allow paramodulation into left args of = */
  PARA_INTO_RIGHT,  /* allow paramodulation into right args of = */
  PARA_FROM_VARS,  /* allow paramodulation from variables */
  PARA_INTO_VARS,  /* allow paramodulation into variables */
  PARA_FROM_UNITS_ONLY,  /* from clause must be unit */
  PARA_INTO_UNITS_ONLY,  /* into clause must be unit */
  PARA_SKIP_SKOLEM,  /* Skolem function restriction strategy */
  PARA_ONES_RULE,  /* paramod only into first args of terms */
  PARA_ALL,  /* paramodulate all occurrences of into term */

  DETAILED_HISTORY,  /* store literal numbers and position vectors */
  ORDER_HISTORY,  /* Nucleus number first for hyper, UR. */
  UNIT_DELETION,  /* unit deletion processing */
  BACK_UNIT_DELETION,  /* like back demodulation, but for literals */
  DELETE_IDENTICAL_NESTED_SKOLEM,  /* delete clauses containing */
  SORT_LITERALS,  /* sort literals in pre_process */
  FOR_SUB,  /* forward subsumption */
  BACK_SUB,  /* back subsumption */
  FACTOR,  /* factor during post_process */

  DEMOD_HISTORY,  /* build history in demodulation */
  ORDER_EQ,  /* flip equalities (+ and -) if right arg heavier */
  EQ_UNITS_BOTH_WAYS,  /* nonoriented eq units both ways */
  DEMOD_LINEAR,  /* use linear search instead of index tree */
  DEMOD_OUT_IN,  /* demodulate outside-in, (leftmost) */
  DYNAMIC_DEMOD,  /* dynamic addition of demodulators */
  DYNAMIC_DEMOD_ALL,  /* try to make all equalities into demodulators */
  DYNAMIC_DEMOD_LEX_DEP,  /* allow lex-dep dynamic demodulators */
  BACK_DEMOD,  /* back demodulation */
  ANL_EQ,  /* meta-option for standard equational strategy */
  KNUTH_BENDIX,  /* Alias for ANL_EQ */
  LRPO,  /* lexicographic recursive path ordering */
  LEX_ORDER_VARS,  /* consider variables when lex_checking terms */
  SYMBOL_ELIM,  /* orient equalities to eliminate symbols */
  REWRITER, /* meta-option for demodulation only */

  CHECK_ARITY,  /* require symbols to have fixed arities */
  PROLOG_STYLE_VARIABLES,  /* vars start with A-Z */
  ECHO_INCLUDED_FILES,  /* echo input from included files */
  SIMPLIFY_FOL,  /* attempt to simplify during cnf translation */
  PROCESS_INPUT,  /* process input usable and sos */
  TPTP_EQ,  /* for TPTP: "equal" is the one and only equality */

  VERY_VERBOSE,  /* print generated clauses */
  PRINT_KEPT,  /* print kept clauses */
  PRINT_PROOFS,  /* print all proofs found */
  BUILD_PROOF_OBJECT,    /* alias to build_proof_object_1 */
  BUILD_PROOF_OBJECT_1,  /* build proof to be checked elsewhere */
  BUILD_PROOF_OBJECT_2,  /* build new kind of proof object */
  PRINT_NEW_DEMOD,  /* print new demodultors */
  PRINT_BACK_DEMOD,  /* print back demodulated clauses */
  PRINT_BACK_SUB,  /* print back subsumed clauses */
  DISPLAY_TERMS,  /* print terms in internal format */
  PRETTY_PRINT,  /* Pretty print requested by Boyle */
  BIRD_PRINT,  /* output a(_,_) terms in combinatory logic notation */

  INDEX_FOR_BACK_DEMOD,  /* index (FPA) all terms for back demod */
  FOR_SUB_FPA,  /* forward subsump with FPA, not discrim. tree */
  NO_FAPL,  /* don't FPA index all positive literals */
  NO_FANL,  /* don't FPA index all negative literals */

  /* misc 1 */

  CONTROL_MEMORY,  /* automatically adjust max_weight */
  PROPOSITIONAL,  /* some propositional optimizations */
  REALLY_DELETE_CLAUSES,  /* delete back demod and back_subed cls */
  ATOM_WT_MAX_ARGS,  /* weight of atom is max of weights of arguments */
  TERM_WT_MAX_ARGS,  /* weight of term is max of weights of arguments */
  FREE_ALL_MEM,  /* free all memory to avail lists at end of run */

  /*********************** Fringe ******************************/

  KEEP_HINT_SUBSUMERS,  /* do not delete if it subsumes a hint */
  KEEP_HINT_EQUIVALENTS,  /* see hint_keep_test() */
  PRINT_PROOF_AS_HINTS,  /* constructd from proof object */
  DEGRADE_HINTS2,   /* Bob's Hint degradation */

  INPUT_SEQUENT,  /* input clauses in sequent notation */
  OUTPUT_SEQUENT,  /* output clauses in sequent notation */
  SIGINT_INTERACT,  /* interact on SIGINT */
  ANCESTOR_SUBSUME,  /* ancestor subsumption */

  AUTO,  /* select the current AUTO mode (see AUTO*) (sets auto1) */
  AUTO1,  /* original AUTO mode (3.0.4) */
  AUTO2,  /* revised AUTO mode (3.0.5) */

  GEOMETRIC_RULE,  /* RP's inference rule, with unification */
  GEOMETRIC_REWRITE_BEFORE,  /* RP's inference rule as a rewrite */
  GEOMETRIC_REWRITE_AFTER,  /* RP's inference rule as a rewrite */
  GL_DEMOD,  /* Delay demodulation. */

  SPLIT_CLAUSE,  /* case splitting with fork */
  SPLIT_WHEN_GIVEN,  /* Split clauses when given */
  SPLIT_ATOM,  /* Split on atoms instead of clauses */
  SPLIT_POS,  /* Split on positive clauses only */
  SPLIT_NEG,  /* Split on negatvie clauses only */
  SPLIT_NONHORN,  /* Split on negatvie clauses only */
  SPLIT_MIN_MAX,  /* Split on clause with min max-literal */
  SPLIT_POPULAR,  /* Split on most popular atoms */

  LINKED_UR_RES,  /* linked UR resolution inference rule */
  LINKED_UR_TRACE,  /* trace linked UR res inference rule */
  LINKED_SUB_UNIT_USABLE,  /* use Usable list to subsume subsumable */
  LINKED_SUB_UNIT_SOS,  /* use Sos list to subsume subsumable */
  LINKED_UNIT_DEL,  /* use Unit Deletion during linked UR resolution. */
  LINKED_TARGET_ALL,  /* If set, all literals are targets. */
  LINKED_HYPER_RES,  /* Linked hyper inference rule */

  /* not documented */

  FOR_SUB_EQUIVALENTS_ONLY,  /* forward subsumption iff equivalent */
  PROOF_WEIGHT,  /* Calculate proof weight (ancestor bag). */
  FORMULA_HISTORY,  /* Make input clauses point at formula parent */
  BELL,  /* Ring the bell for important events? */

  DISCARD_COMMUTATIVITY_CONSEQUENCES,  /* experimental */
  LITERALS_WEIGH_ONE,
  HYPER_SYMMETRY_KLUDGE,  /* secret flag */
  DISCARD_NON_ORIENTABLE_EQ,  /* secret flag */
  DISCARD_XX_RESOLVABLE,  /* secret flag */

  LOG_FOR_X_SHOW,  /* log some events for X display */

  PICK_DIFF_SIM,  /* selection of given clause */
  PICK_RANDOM_LIGHTEST,  /* selection of given clause */
  PICK_LAST_LIGHTEST,  /* selection of given clause */
  PICK_MID_LIGHTEST,  /* selection of given clause */

  MULTI_JUST,  /* for proof-shortening experiment */
  MULTI_JUST_LESS,  /* for proof-shortening experiment */
  MULTI_JUST_SHORTER,  /* for proof-shortening experiment */

  PROG_SYNTHESIS,  /* program synthesis mode */

  CLOCKS  /* detailed timing */
};

/* end of Flags */

/*************
 *
 *    Parms are integer valued options.  To install a new parm, append
 *    a new name and index to the end of this list, then insert code to
 *    initialize it in the routine `init_options'.
 *    Example access:  if (Parms[FPA_LITERALS].val == 4) {
 *    See routine `init_options' for defaults.
 *
 *************/

#define MAX_PARMS 100  /* This must hold the following list! */

enum {
  REPORT,  /* output stats and times every n seconds */

  MAX_SECONDS,  /* stop search after this many seconds */
  MAX_GEN,  /* stop search after this many generated clauses */
  MAX_KEPT,  /* stop search after this many kept clauses */
  MAX_GIVEN,  /* stop search after this many given clauses */
  MAX_LEVELS,  /* with sos_queue, stop after this many levels */
  MAX_MEM,  /* stop search after this many K bytes allocated */

  MAX_LITERALS,  /* max # of lits in kept clause (0 -> no limit) */
  MAX_WEIGHT,  /* maximum weight of kept clauses */
  MAX_DISTINCT_VARS,  /* max # of variables in kept clause */
  MAX_ANSWERS,  /* maximum number of answer literals */

  FPA_LITERALS,  /* FPA indexing depth for literals */
  FPA_TERMS,  /* FPA indexing depth for terms */

  PICK_GIVEN_RATIO,  /* pick lightest n times, then pick first */
  AGE_FACTOR,  /* to adjust the pick-given weight */
  DISTINCT_VARS_FACTOR,  /* to adjust the pick-given weight */
  INTERRUPT_GIVEN,  /* call interact after this many given cls */
  DEMOD_LIMIT,  /* Limit on number of rewrites per clause */
  MAX_PROOFS,  /* stop search after this many empty clauses */
  MIN_BIT_WIDTH,  /* minimum field for bit strings */
  NEG_WEIGHT,  /* add this value to wight of negative literals */
  PRETTY_PRINT_INDENT,  /* indent for pretty print */
  STATS_LEVEL,  /* higher stats_level -> output more statistics */
  DYNAMIC_DEMOD_DEPTH,  /* deciding dynamic demoulators (ad hoc) */
  DYNAMIC_DEMOD_RHS,  /* deciding dynamic demoulators (ad hoc) */

  FSUB_HINT_ADD_WT,  /* add to pick-given wt     */
  BSUB_HINT_ADD_WT,  /* add to pick-given wt     */
  EQUIV_HINT_ADD_WT,  /* add to pick-given wt     */
  FSUB_HINT_WT,  /* pick-given wt     */
  BSUB_HINT_WT,  /* pick-given wt     */
  EQUIV_HINT_WT,  /* pick-given wt     */

/* fringe */

  CHANGE_LIMIT_AFTER,  /* replace reduce_weight_limit */
  NEW_MAX_WEIGHT,  /* replace reduce_weight_limit */

  GEO_GIVEN_RATIO,  /* like pick_given_ratio, for geo children */

  DEBUG_FIRST,  /* turn debugging on        */
  DEBUG_LAST,  /* turn debugging off       */
  VERBOSE_DEMOD_SKIP,  /* debugging option   */

  HEAT,  /* maximum heat level */
  DYNAMIC_HEAT_WEIGHT,  /* max weigth of dynamic hot clause */

  MAX_UR_DEPTH,  /* max depth for linked UR (normal depth = 0) */
  MAX_UR_DEDUCTION_SIZE,  /* max resolutions in a single linked UR */

  SPLIT_SECONDS,  /* time to search before splitting */
  SPLIT_GIVEN,  /* given clauses before splitting */
  SPLIT_DEPTH,  /* maximum splitting depth */

  /* not documented */

  NEW_SYMBOL_LEX_POSITION,

  WARN_MEM,  /* reset max_weight at this memory usage */
  WARN_MEM_MAX_WEIGHT,  /* new max_weight */

  PICK_DIFF,  /* selection of given clause */
  PICK_DIFF_RANGE,  /* selection of given clause */

  MULTI_JUST_MAX   /* for proof-shortening experiment */
};

/* end of Parms */

/*************
 *
 *    Statistics.  To install a new statistic, append a new name and index
 *    to the end of this list, then insert the code to output it in the
 *    routine `print_stats'.
 *    Example access:  Stats[INPUT_ERRORS]++;
 *
 *************/

#define MAX_STATS 50  /* This must hold the following list! */

enum {
  INPUT_ERRORS,
  CL_INPUT,
  CL_GENERATED,
  CL_KEPT,
  CL_FOR_SUB,
  CL_BACK_SUB,
  CL_TAUTOLOGY,
  CL_GIVEN,
  CL_WT_DELETE,
  REWRITES,
  UNIT_DELETES,
  EMPTY_CLAUSES,
  FPA_OVERLOADS,  /* not output if 0 */
  FPA_UNDERLOADS,  /* not output if 0 */
  CL_VAR_DELETES,  /* not output if 0 */
  FOR_SUB_SOS,
  NEW_DEMODS,
  CL_BACK_DEMOD,
  LINKED_UR_DEPTH_HITS,
  LINKED_UR_DED_HITS,
  SOS_SIZE,
  K_MALLOCED,
  CL_NOT_ANC_SUBSUMED,
  USABLE_SIZE,
  DEMODULATORS_SIZE,
  DEMOD_LIMITS,  /* not output if 0 */
  INIT_WALL_SECONDS,
  BINARY_RES_GEN,
  HYPER_RES_GEN,
  NEG_HYPER_RES_GEN,
  UR_RES_GEN,
  PARA_INTO_GEN,
  PARA_FROM_GEN,
  LINKED_UR_RES_GEN,
  GEO_GEN,
  DEMOD_INF_GEN,
  FACTOR_GEN,
  HOT_GENERATED,
  HOT_KEPT,
  FACTOR_SIMPLIFICATIONS,
  HOT_SIZE,
  PASSIVE_SIZE,
  BACK_UNIT_DEL_GEN
};

/* end of Stats */

/*************
 *
 *    Clocks.  To install a new clock, append a new name and index
 *    to the end of this list, then insert the code to output it in the
 *    routine `print_times'.  Example of use: CLOCK_START(INPUT_TIME),
 *    CLOCK_STOP(INPUT_TIME),  micro_sec = clock_val(INPUT_TIME);.
 *    See files macros.h and clocks.c.
 *
 *************/

#define MAX_CLOCKS 50  /* This must hold the following list! */

enum {
  INPUT_TIME,
  CLAUSIFY_TIME,
  PROCESS_INPUT_TIME,

  BINARY_TIME,
  HYPER_TIME,
  NEG_HYPER_TIME,
  UR_TIME,
  PARA_INTO_TIME,
  PARA_FROM_TIME,
  LINKED_UR_TIME,

  PRE_PROC_TIME,
  RENUMBER_TIME,
  DEMOD_TIME,
  ORDER_EQ_TIME,
  UNIT_DEL_TIME,
  WEIGH_CL_TIME,
  SORT_LITS_TIME,
  FOR_SUB_TIME,
  DEL_CL_TIME,
  KEEP_CL_TIME,
  PRINT_CL_TIME,
  CONFLICT_TIME,
  NEW_DEMOD_TIME,

  POST_PROC_TIME,
  BACK_DEMOD_TIME,
  BACK_SUB_TIME,
  FACTOR_TIME,

  UN_INDEX_TIME,
  HOT_TIME,
  FACTOR_SIMP_TIME,

  HINTS_TIME,
  HINTS_KEEP_TIME,

  BACK_UNIT_DEL_TIME,
  PICK_GIVEN_TIME
};

/* end of Clocks */

/*************
 *
 *    internal flags--invisible to users
 *
 *************/

#define MAX_INTERNAL_FLAGS 10  /* This must hold the following list! */

enum {
  SPECIAL_UNARY_PRESENT,
  DOLLAR_PRESENT,
  LEX_VALS_SET,
  REALLY_CHECK_ARITY,
  FOREACH_SOS,
  HINTS_PRESENT,
  HINTS2_PRESENT
};

/*************
 *
 *    clause attributes
 *
 *************/

/* attribute types */

enum {
  INT_ATTR,
  BOOL_ATTR,
  DOUBLE_ATTR,
  STRING_ATTR,
  TERM_ATTR
};

/* attributes */

#define MAX_ATTRIBUTES 20  /* This must hold the following list! */

enum {
  BSUB_HINT_WT_ATTR,
  FSUB_HINT_WT_ATTR,
  EQUIV_HINT_WT_ATTR,
  BSUB_HINT_ADD_WT_ATTR,
  FSUB_HINT_ADD_WT_ATTR,
  EQUIV_HINT_ADD_WT_ATTR,
  LABEL_ATTR
};

