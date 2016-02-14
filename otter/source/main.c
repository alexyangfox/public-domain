/* Otter 3.3
 *
 * William McCune
 * Mathematics and Computer Science Division
 * Argonne National Laboratory
 * Argonne, IL  60439
 * U.S.A.
 *
 * E-mail: mccune@mcs.anl.gov
 * Web:    http://www.mcs.anl.gov/~mccune
 *         http://www.mcs.anl.gov/AR/otter
 */

#define OTTER_VERSION "3.3f"
#define VERSION_DATE  "August 2004"

#define IN_MAIN  /* so that global vars in header.h will not be external */
#include "header.h"

/**/ int main(int argc, char **argv)
{
  struct clause *giv_cl;
  int errors, status, level, first_of_next_level;
  char *str;
  FILE *xlog_fp = NULL;
    
  non_portable_init(argc, argv);
  print_banner(argc, argv);
  init();
  
  read_all_input(argc, argv);
  errors = Stats[INPUT_ERRORS];
  if (errors != 0) {
    fprintf(stderr, "\n%d input errors were found.%c\n\n", errors, Bell);
    printf("%d input errors were found.\n", errors);
    exit(INPUT_ERROR_EXIT);
  }
  else {
    status = check_stop();
    if (status == KEEP_SEARCHING && Parms[MAX_LEVELS].val == 0)
      status = MAX_LEVELS_EXIT;
    if (status == KEEP_SEARCHING) {
      if (splitting() && Parms[SPLIT_GIVEN].val == 0)
	always_split();  /* does not return */
      giv_cl = extract_given_clause();
    }
    else
      giv_cl = NULL;
    level = 0;
    first_of_next_level = 0;

    if (Flags[LOG_FOR_X_SHOW].val)
      xlog_fp = init_log_for_x_show();

    /* --------------------- MAIN LOOP STARTS HERE --------------------- */

    printf("\n=========== start of search ===========\n"); fflush(stdout);

    while (giv_cl != NULL && status == KEEP_SEARCHING) {

      if (Flags[SOS_QUEUE].val && giv_cl->id >= first_of_next_level) {
	level++;
	first_of_next_level = next_cl_num();
	printf("\nStarting on level %d, last kept clause of level %d is %d.\n",
	       level, level-1, first_of_next_level-1);
	fprintf(stderr, "\n%cStarting on level %d, last kept clause "
		"of level %d is %d.\n",
		Bell, level, level-1, first_of_next_level-1);

      }

      if (Flags[LOG_FOR_X_SHOW].val)
	log_for_x_show(xlog_fp);

      Stats[CL_GIVEN]++;
      if (Flags[PRINT_GIVEN].val) {
	printf("\ngiven clause #%ld: ", Stats[CL_GIVEN]);
	printf("(wt=%d) ", giv_cl->pick_weight);
	print_clause(stdout, giv_cl); fflush(stdout);
      }
      index_lits_clash(giv_cl);
      append_cl(Usable, giv_cl);
      if (splitting())
	possible_given_split(giv_cl);
      infer_and_process(giv_cl);

      if (Parms[INTERRUPT_GIVEN].val > 0 &&
	  Stats[CL_GIVEN] % Parms[INTERRUPT_GIVEN].val == 0) {
	fprintf(stderr, "\n%c%ld clauses have been given.\n", Bell, Stats[CL_GIVEN]);
	interact();
      }

      status = check_stop();
      if (status == KEEP_SEARCHING &&
	  Flags[SOS_QUEUE].val &&
	  level == Parms[MAX_LEVELS].val &&
	  Sos->first_cl != NULL &&
	  Sos->first_cl->id >= first_of_next_level)
	status = MAX_LEVELS_EXIT;
      
      if (status == KEEP_SEARCHING) {
	if (Parms[CHANGE_LIMIT_AFTER].val == Stats[CL_GIVEN]) {
	  int new_limit;
	  new_limit = Parms[NEW_MAX_WEIGHT].val;
	  Parms[MAX_WEIGHT].val = new_limit;
	  printf("\nreducing weight limit to %d.\n", new_limit);
	}
	if (splitting())
	  possible_split();  /* parent does not return if successful */
	giv_cl = extract_given_clause();
      }

      if (status == KEEP_SEARCHING && giv_cl && Parms[REPORT].val > 0)
	report();

    }  /* end of main loop */

    /* --------------------- MAIN LOOP ENDS HERE --------------------- */

    /* print the reason the search ended */

    if (status == KEEP_SEARCHING) {
      if (splitting() && current_case() != NULL)
	exit_with_possible_model();  /* this call does not return here */
      status = SOS_EMPTY_EXIT;
      fprintf(stderr, "\n%cSearch stopped because sos empty.\n\n", Bell);
      printf("\nSearch stopped because sos empty.\n");
    }
    else {
      switch (status) {
      case MAX_GIVEN_EXIT: str = "max_given"; break;
      case MAX_GEN_EXIT: str = "max_gen"; break;
      case MAX_KEPT_EXIT: str = "max_kept"; break;
      case MAX_SECONDS_EXIT: str = "max_seconds"; break;
      case MAX_LEVELS_EXIT: str = "max_levels"; break;
      default: str = "???"; break;
      }

      fprintf(stderr, "\n%cSearch stopped by %s option.\n\n", Bell, str);
      printf("\nSearch stopped by %s option.\n", str);
    }

    cleanup();

    if (multi_justifications()) {
      struct clause *ee = proof_last_hidden_empty();
      if (ee != NULL) {
	multi_just_process(ee);
	output_stats(stdout, Parms[STATS_LEVEL].val);
      }
    }
    exit(status);
  }

}  /* main */

/*************
 *
 *    void print_banner(argc, argv)
 *
 *************/

void print_banner(int argc,
		  char **argv)
{
  int i;
  int pid = my_process_id();
    
  printf("----- Otter %s, %s -----\n", OTTER_VERSION, VERSION_DATE);
  printf("The process was started by %s on %s,\n%s",
	 username(), hostname(), get_time());

  printf("The command was \"");
  for(i = 0; i < argc; i++)
    printf("%s%s", argv[i], (i < argc-1 ? " " : ""));
  printf("\".");
  if (pid != 0)
    printf("  The process ID is %d.\n\n", pid);
  else
    printf("\n\n");
    
}  /* print_banner */
