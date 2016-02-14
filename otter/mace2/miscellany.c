#include "Mace2.h"  /* for ABEND_EXIT */

extern int Models;  /* owned by mace.c */

/*************
 *
 *    exit_with_message(exit_code)
 *
 *************/

void exit_with_message(int exit_code, int stats)
{
  char *m1, m2[100];

  switch (exit_code) {
  case MACE_ABEND_EXIT:       m1 = "Abnormal end."; break;
  case UNSATISFIABLE_EXIT:    m1 = "The search is complete."; break;
  case MACE_MAX_SECONDS_EXIT: m1 = "Exit by max_seconds parameter."; break;
  case MACE_MAX_MEM_EXIT:     m1 = "Exit by max_mem parameter."; break;
  case MAX_MODELS_EXIT:       m1 = "Exit by max_models parameter."; break;
  case ALL_MODELS_EXIT:       m1 = "The search is complete."; break;
  case MACE_SIGINT_EXIT:      m1 = "Killed by SIGINT signal."; break;
  case MACE_SEGV_EXIT:        m1 = "Killed by SIGSEGV signal."; break; 
  case MACE_INPUT_ERROR_EXIT: m1 = "Input error."; break;
  default:                    m1 = "Exit with unknown code."; break;
  }

  if (exit_code == MACE_INPUT_ERROR_EXIT)
    fprintf(stderr, "\nInput error.  See the output file.\n\n\007");

  if (Models > 0)
    sprintf(m2, "The set is satisfiable (%d model(s) found).", Models);
  else
    sprintf(m2, "No models were found.");

  if (stats)
    MACE_print_stats(stdout);

  MACE_print_times(stdout);

  printf("\n%s  %s\n\n", m1, m2);
  fprintf(stderr, "\007\n%s  %s\n\n", m1, m2);

  printf("The job finished %s", get_time());

  exit(exit_code);
}  /* exit_with_message */

/*************
 *
 *    MACE_abend
 *
 *************/

void MACE_abend(char *str)
{
  fprintf(stderr, "\007");
  fprintf(stderr, "\n\n********** ABNORMAL END **********\n");
  fprintf(stderr, "********** %s\n", str);

  fprintf(stdout, "\n\n********** ABNORMAL END **********\n");
  fprintf(stdout, "********** %s\n", str);
  
  exit_with_message(MACE_ABEND_EXIT, 1);
}  /* MACE_abend */

