/* MACE 2.1
 *
 * William McCune
 * Mathematics and Computer Science Division
 * Argonne National Laboratory
 * Argonne, IL  60439
 * U.S.A.
 *
 * E-mail: mccune@mcs.anl.gov
 * Web:    http://www.mcs.anl.gov/~mccune
 *         http://www.mcs.anl.gov/AR/mace
 */

#define VERSION "2.2f"
#define VDATE "August 2004"

#include "Mace2.h"

#include <unistd.h>  /* for getopt */

static int Domain_min = 2;    /* default starting size */
static int Domain_max = 0;    /* default will be Domain_min */

int Models = 0;        /* shared with dp.c, miscellany.c */
int First_order = 1;   /* shared with dp.c */
int Domain_size;       /* shared with stats.c */
int Init_wall_seconds; /* shared with stats.c */


/*************
 *
 *    void MACE_print_banner(argc, argv)
 *
 *************/

static void MACE_print_banner(int argc, char **argv)
{
  int i;

  printf("----- MACE %s, %s -----\n", VERSION, VDATE);
  printf("The process was started by %s on %s,\n%s",
	 username(), hostname(), get_time());

  printf("The command was \"");
  for(i = 0; i < argc; i++)
    printf("%s%s", argv[i], (i < argc-1 ? " " : ""));
  printf("\".\n");
    
}  /* MACE_print_banner */

/*************
 *
 *   usage_message()
 *
 *************/

static void usage_message(void)
{
  printf("MACE %s -- Search for finite models.\n"
	 "\n"
         "Input clauses are taken from standard input.\n"
	 "\n"
	 "Command-line options:\n"
	 "\n"
	 "  -n n  : Search for model of size n (default %d).\n"
	 "  -N n  : Iterate up to size n (default: don't iterate).\n"
	 "  -c    : Assume constants are distinct (up to domain size).\n"
	 "  -z n  : Apply isomorphism constraints to first n constants\n"
	 "          (default %d).  Overridden by -c.\n"
	 "  -p    : Print models in human format as they are found.\n"
	 "  -P    : Print models in portable format as they are found.\n"
	 "  -I    : Print models in IVY format as they are found.\n"
	 "  -m n  : Stop when the n-th model is found (default %d).\n"
	 "  -t n  : Stop after about n seconds.\n"
	 "  -k n  : Dynamically allocate at most n Kbytes (default %d).\n"
	 "  -s    : Perform unit subsumption. (Always done on input.)\n"
	 "  -C    : Split on clauses instead of on atoms.\n"
	 "  -r    : Record and print the order of assignments.\n"
	 "  -x    : Quasigroup constraint.\n"
	 "  -y    : Iso experiment (implies -C and -z0).\n"
	 "  -v n  : Try to part flattened clauses with n or more variables (default %d).\n"
	 "  -h    : Print this message.\n"
	 "\n"
	 "Examples of constraints given in input:\n"
	 "  list(mace_constraints).\n"
	 "  assign(a, 3).\n"
	 "  assign(f(2,3), 1).\n"
	 "  assign(P(0), F).\n"
	 "  assign(P(1), T).\n"
	 "  property(r(_,_), equality).\n"
	 "  property(lt(_,_), order).\n"
	 "  property(f(_,_), quasigroup).\n"
	 "  property(g(_), bijection).\n"
	 "  end_of_list.\n"
	 , VERSION,
	 Domain_min,
	 MACE_Parms[ISO_CONSTANTS].val,
	 MACE_Parms[MAX_MODELS].val,
	 MACE_Parms[MACE_MAX_MEM].val,
	 MACE_Parms[PART_VARS].val
	 );

}  /* usage_message */

/*************
 *
 *   process_command_line_args()
 *
 *************/

static int process_command_line_args(int argc, char **argv)
{
  extern char *optarg;

  int c, n;
  int error = 0;

  while ((c = getopt(argc, argv, "spPIhcxyCrm:t:k:n:N:d:z:v:")) != EOF) {
    switch (c) {
    case 'c':
      MACE_change_flag(stderr, "distinct_constants", 1);
      break;
    case 's':
      MACE_change_flag(stderr, "subsume", 1);
      break;
    case 'p':
      MACE_change_flag(stderr, "print_models", 1);
      break;
    case 'P':
      MACE_change_flag(stderr, "print_models_portable", 1);
      break;
    case 'I':
      MACE_change_flag(stderr, "print_models_ivy", 1);
      break;
    case 'x':
      MACE_change_flag(stderr, "qg_constraint", 1);
      break;
    case 'y':
      MACE_change_flag(stderr, "iso_x", 1);
      MACE_change_flag(stderr, "clause_split", 1);
      if (MACE_change_parm(stderr, "iso_constants", 0) == -1)
	error++;
      break;
    case 'C':
      MACE_change_flag(stderr, "clause_split", 1);
      break;
    case 'r':
      MACE_change_flag(stderr, "record_assignments", 1);
      break;
    case 'm':
      n = atoi(optarg);
      if (MACE_change_parm(stderr, "max_models", n) == -1)
	error++;
      break;
    case 't':
      n = atoi(optarg);
      if (MACE_change_parm(stderr, "max_seconds", n) == -1)
	error++;
      break;
    case 'k':
      n = atoi(optarg);
      if (MACE_change_parm(stderr, "max_mem", n) == -1)
	error++;
      break;
    case 'n':
      n = atoi(optarg);
      if (n < 1) {
	fprintf(stderr, "Domain size must be > 0.\n");
	error++;
      }
      else if (n > MAX_DOMAIN) {
	fprintf(stderr, "Domain size must be <= %d.\n", MAX_DOMAIN);
	error++;
      }
      else
	Domain_min = n;
      break;
    case 'N':
      n = atoi(optarg);
      if (n < 1) {
	fprintf(stderr, "Maximum domain size must be > 0.\n");
	error++;
      }
      else if (n > MAX_DOMAIN) {
	fprintf(stderr, "Maximum domain size must be <= %d.\n", MAX_DOMAIN);
	error++;
      }
      else
	Domain_max = n;
      break;
    case 'z':
      n = atoi(optarg);
      if (MACE_change_parm(stderr, "iso_constants", n) == -1)
	error++;
      break;
    case 'v':
      n = atoi(optarg);
      if (MACE_change_parm(stderr, "part_vars", n) == -1)
	error++;
      break;
    case 'h':
    case '?':
    default:
      error = 1;
      break;
    }
  }

  return(error == 0);

}  /* process_command_line_args */

/*************
 *
 *   MACE_sig_handler()
 *
 *************/

#ifdef TP_SIGNAL
#include <signal.h>

static void MACE_sig_handler(int condition)
{
  if (condition == SIGSEGV) {

    char message[] =
      "\n"
      "+----------------------------------------------------------+\n"
      "| SEGMENTATION FAULT!!  This is probably caused by a bug   |\n"
      "| in MACE.  Please send copy of the input file to          |\n"
      "| otter@mcs.anl.gov, let us know what version of MACE you  |\n"
      "| are using, and send any other info that might be useful. |\n"
      "+----------------------------------------------------------+\n\n";

    fprintf(stderr, "%s%c", message, Bell);

    exit_with_message(MACE_SEGV_EXIT, 1);
  }
  else if (condition == SIGINT) {
    exit_with_message(MACE_SIGINT_EXIT, 1);
  }
  else {
    char s[100];
    sprintf(s, "MACE_sig_handler, cannot handle signal %d.\n", condition);
    MACE_abend(s);
  }
}  /* MACE_sig_handler */

#endif  /* TP_SIGNAL */

/*************
 *
 *    reinit
 *
 *************/

static void reinit(void)
{
  reinit_mem();
  reinit_dp();
  reinit_generate();

  /* Don't call init_dp here, because we don't want to reinitialize the
     options, because we're not going to read them again.
  */
  init_clocks();
  init_stats();
}  /* reinit */

/*************
 *
 *    try_domain_size
 *
 *************/

static void try_domain_size(void)
{
  int rc;  /* we can ignore the return codes */

  MACE_CLOCK_START(GENERATE_TIME);
  rc = dp_trans(Domain_size,
		MACE_Flags[DISTINCT_CONSTANTS].val,
		MACE_Flags[QG_CONSTRAINT].val);

  if (!rc)
    return;  /* Unsatisfiability detected by dp_trans */
  else {
    rc = more_setup();
    MACE_CLOCK_STOP(GENERATE_TIME);

    MACE_CLOCK_START(DECIDE_TIME);
    rc = dp_prover();
    MACE_CLOCK_STOP(DECIDE_TIME);
  }
}  /* try_domain_size */

/*************
 *
 *    main
 *
 *************/

int main(int argc, char **argv)
{
  int rc, errors;
  struct list *usable_raw, *usable_flattened, *usable_parted;

#ifdef TP_SIGNAL
  signal(SIGINT, MACE_sig_handler);
  signal(SIGSEGV, MACE_sig_handler);
#endif

  MACE_print_banner(argc, argv);
  init_dp();  /* This is MACE's initialization. */
  Init_wall_seconds = wall_seconds();
  rc = process_command_line_args(argc, argv);
  if (!rc) {
    usage_message();
    exit(MACE_INPUT_ERROR_EXIT);
  }

  init();  /* This is Otter's initialization. */

  read_all_mace_input(argc, argv);
  errors = Stats[INPUT_ERRORS];
  if (errors != 0)
    exit_with_message(MACE_INPUT_ERROR_EXIT, 1);

  usable_raw = Usable;
  usable_flattened=flatten_clauses(usable_raw);
  usable_parted=variable_optimize(usable_flattened,MACE_Parms[PART_VARS].val);
				    
  printf("\nlist(flattened_and_parted_clauses).\n");
  print_cl_list(stdout, usable_parted);
  Usable = usable_parted;

  if (Domain_max == 0)  /* if it hasn't been set, make it Domain_min */
    Domain_max = Domain_min;

  for (Domain_size = Domain_min; Domain_size <= Domain_max; Domain_size++) {
    printf("\n--- Starting search for models of size %d ---\n\n", Domain_size);
    try_domain_size();
    MACE_print_stats(stdout);
    reinit();
  }

  if (Models == 0)
    exit_with_message(UNSATISFIABLE_EXIT, 0);
  else
    exit_with_message(ALL_MODELS_EXIT, 0);

  exit(99);  /* This won't be called.  This is to shut up the compiler. */

}  /* main */
