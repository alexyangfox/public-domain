/*
 *  nonport.c -- nonportable features; see unix makefile.
 *
 */

#include "header.h"

#ifdef TP_SIGNAL  /* for call to signal() */
#  include <signal.h>
#endif

#ifdef TP_FORK  /* for calls to fork() and wait() */
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

#ifdef TP_NAMES  /* for calls to getpwuid() and gethostname() */
#  include <sys/types.h>
#  include <pwd.h>
#  include <unistd.h>
#endif

/*************
 *
 *   non_portable_init()
 *
 *************/

void non_portable_init(int argc,
		       char **argv)
{
#ifdef TP_SIGNAL
  signal(SIGINT, sig_handler);
  signal(SIGSEGV, sig_handler);
  signal(SIGUSR1, sig_handler);
#endif

}  /* non_portable_init */

#ifdef TP_SIGNAL

/*************
 *
 *   sig_handler()
 *
 *************/

void sig_handler(int condition)
{
  if (condition == SIGSEGV) {
    char message[] =
      "\nSEGMENTATION FAULT!!  This is probably caused by a\n"
      "bug in Otter.  Please send copy of the input file to\n"
      "otter@mcs.anl.gov, let us know what version of Otter you are\n"
      "using, and send any other info that might be useful.\n\n";

    output_stats(stdout, Parms[STATS_LEVEL].val);

    fprintf(stdout, "%s", message);
    fprintf(stderr, "%s%c", message, Bell);

    printf("\nThe job finished %s", get_time());
    exit(SEGV_EXIT);
  }
  else if (condition == SIGINT) {
    signal(SIGINT, sig_handler);  /* for subsequent interrupts */
    interact();

  }
  else if (condition == SIGUSR1) {
    output_stats(stdout, Parms[STATS_LEVEL].val);
    printf("\nOtter killed by SIGUSR1 signal.\n");
    fprintf(stderr, "\nOtter killed by SIGUSR1 signal.\n");
    exit(USR1_EXIT);
  }
  else {
    char s[100];
    sprintf(s, "sig_handler, cannot handle signal %d.\n", condition);
    abend(s);
  }
}  /* sig_handler */

#endif  /* TP_SIGNAL */

/*************
 *
 *   username()
 *
 *************/

char *username(void)
{
#ifdef TP_NAMES
  /* cuserid is not on some machines;
   * getlogin doesn't always work if user is not logged in;
   * getlogin broken on Linux.  Following seems to be ok.
   */
  struct passwd *p;
  p = getpwuid(getuid());
  return(p ? p->pw_name : "???");
#else
  return("???");
#endif
}  /* username */

/*************
 *
 *   hostname()
 *
 *************/

char *hostname(void)
{
  static char host[64];

#ifdef TP_NAMES
  if (gethostname(host, 64) != 0)
    strcpy(host, "???");
#else
  strcpy(host, "???");
#endif
  return(host);
}  /* hostname */

/*************
 *
 *    void interact()
 *
 *    This routine provides some primitive interaction with the user.
 *
 *************/

void interact(void)
{
  FILE *fin, *fout;
  struct term *t;
  int rc, go_back;
  char *help_string;
  static int fork_level = 0;

  if (!Flags[SIGINT_INTERACT].val) {
    fprintf(stderr, "\n%cSearch stopped by SIGINT.\n\n", Bell);
    fprintf(stdout, "\nSearch stopped by SIGINT.\n");
    exit(INTERACTIVE_EXIT);
  }

  fin  = fopen("/dev/tty", "r");
  fout = fopen("/dev/tty", "w");

  help_string = "Commands are help, kill, continue, set(_), clear(_), assign(_,_),\n    usable, sos, demodulators, passive, stats, fork, and options.\n    All commands end with a period.\n";

  if (!fin || !fout) {
    printf("interaction failure: cannot find tty.\n");
    fprintf(stderr, "interaction failure: cannot find tty.\n");
  }
  else {
    printf("\n--- Begin interaction (level %d) ---\n\n", fork_level);
    fprintf(fout, "\n--- Begin interaction (level %d) ---\n\n", fork_level);
    fprintf(fout, "Type `help.' for the list of commands.\n> ");
    fflush(fout);
    t = read_term(fin, &rc);
    go_back = 0;
    while (!go_back) {
      if (!t) {
	if (rc == 1) {
	  fprintf(fout, " Received end-of-file character.\n");
	  go_back = 1;
	}
	else
	  fprintf(fout, " Malformed term.\n");
      }
      else if (t->type == NAME) {
		
	if (str_ident("help", sn_to_str(t->sym_num))) {
	  fprintf(fout, "%s", help_string);
	}
	else if (str_ident("fork", sn_to_str(t->sym_num))) {
#ifdef TP_FORK
	  int fork_status;
	  fflush(stdout); fflush(fout);
	  fork_status = fork();
	  if (fork_status < 0) {
	    fprintf(fout, "%c\nFork failed.\n", Bell);
	    printf("\nFork failed.\n");
	  }
	  else if (fork_status == 0) {  /* child process */
	    fork_level++;
	    fprintf(fout, "\nLevel %d process started and running (waiting for commands);\nlevel %d process will resume when %d finishes.\n", fork_level, fork_level-1, fork_level);
	    printf("\nLevel %d process started and running.\n", fork_level);
	  }
	  else {  /* parent process */
#ifdef TP_SIGNAL
	    /* Ignore interrupt while waiting.  This is necessary
	     * because interrupting a child also interrupts
	     * the parent.
	     */
	    signal(SIGINT, SIG_IGN);
#endif
	    wait(0);  /* for child process to finish */
#ifdef TP_SIGNAL
	    signal(SIGINT, sig_handler);
#endif

	    printf("\n--- Continue interaction at level %d ---\n\n",
		   fork_level);
	    fprintf(fout, "\n--- Continue interaction at level %d ---\n\n", fork_level);
	    fflush(fout);
	  }
#else  /* TP_FORK not defined */
	  fprintf(fout,"The fork operation is not available, because"
		  " TP_FORK was not defined during compilation.\n");
#endif
	}
	else if (str_ident("stats", sn_to_str(t->sym_num))) {
	  output_stats(fout, Parms[STATS_LEVEL].val);
	  output_stats(stdout, Parms[STATS_LEVEL].val);
	  fflush(stdout);
	}
	else if (str_ident("kill", sn_to_str(t->sym_num))) {
	  printf("\nkilled level %d search during interaction.\n", fork_level);
	  fprintf(fout, "killed level %d search during interaction.\n", fork_level);
	  fprintf(fout, " ok.\n");
	  fclose(fin);
	  fclose(fout);
	  cleanup();
	  exit(INTERACTIVE_EXIT);
	}
	else if (str_ident("continue", sn_to_str(t->sym_num))) {
	  fprintf(fout, " ok.");
	  go_back = 1;
	}
	else if (str_ident("sos", sn_to_str(t->sym_num))) {
	  struct clause *c;
	  for (c = Sos->first_cl; c; c = c->next_cl)
	    print_clause(fout, c);
	}
	else if (str_ident("usable", sn_to_str(t->sym_num))) {
	  struct clause *c;
	  for (c = Usable->first_cl; c; c = c->next_cl)
	    print_clause(fout, c);
	}
	else if (str_ident("demodulators", sn_to_str(t->sym_num))) {
	  struct clause *c;
	  for (c = Demodulators->first_cl; c; c = c->next_cl)
	    print_clause(fout, c);
	}
	else if (str_ident("passive", sn_to_str(t->sym_num))) {
	  struct clause *c;
	  for (c = Passive->first_cl; c; c = c->next_cl)
	    print_clause(fout, c);
	}
	else if (str_ident("options", sn_to_str(t->sym_num))) {
	  print_options(stdout);
	  print_options(fout);
	}
	else if (str_ident("symbols", sn_to_str(t->sym_num))) {
	  print_syms(stdout);
	  print_syms(fout);
	}
	else
	  fprintf(fout, " command not understood.\n");
      }
      else if (str_ident("set", sn_to_str(t->sym_num))) {
	if (change_flag(fout, t, 1)) {
	  print_term(stdout, t); printf(".\n");
	}
      }
      else if (str_ident("clear", sn_to_str(t->sym_num))) {
	if (change_flag(fout, t, 0)) {
	  print_term(stdout, t); printf(".\n");
	}
      }
      else if (str_ident("assign", sn_to_str(t->sym_num))) {
	if (change_parm(fout, t)) {
	  print_term(stdout, t); printf(".\n");
	}
      }
      else
	fprintf(fout, " Command not understood.\n");
	
      if (t)
	zap_term(t);

      if (!go_back) {
	fprintf(fout, " ok.\n> ");
	fflush(fout);
	t = read_term(fin, &rc);
      }
    }
	
    printf("\n--- End interaction, continue search at level %d ---\n\n", fork_level);
    fprintf(fout,"\n--- End interaction, continue search at level %d ---\n", fork_level);
	
    fclose(fin);
    fclose(fout);
    fflush(stdout);
  }

}  /* interact */

/*************
 *
 *   foreach_sos()
 *
 *************/

void foreach_sos(void)
{
  int parent = 1;
  int fork_status;
  struct clause *c;
  int rc;
  int count = 0;

  while(parent) {
    c = read_clause(stdin, &rc);
    count++;

    if (rc == 0)
      Stats[INPUT_ERRORS]++;  /* ok to fork in this case */
    else if (c == NULL ||
	     (c->first_lit && is_symbol(c->first_lit->atom,"end_of_list",0)))
      exit(0);
    else {
      /* set up for child */
      if (Flags[PROCESS_INPUT].val == 0)
	cl_integrate(c);
      append_cl(Sos, c);
      print_clause_bare(stdout, c);
      printf(".  %s Job %d\n", "%", count);
      fflush(stdout);
      Stats[SOS_SIZE]++;
    }

    fork_status = fork();
    if (fork_status < 0)
      abend("Fork failed");
    else if (fork_status == 0)  /* child process */
      parent = 0;
    else {
      /* undo child setup */
      if (rc == 0)
	Stats[INPUT_ERRORS]--;
      else {
	rem_from_list(c);
	Stats[SOS_SIZE]--;
	if (Flags[PROCESS_INPUT].val == 0)
	  cl_del_int(c);
	else
	  cl_del_non(c);
      }
      wait(0);
    }
  }
}  /* foreach_sos */

/*************
 *
 *   init_log_for_x_show()
 *
 *************/

FILE *init_log_for_x_show(void)
{
  char s[20];
  int ppid = 0;
#ifdef TP_NAMES
  ppid = getppid();  /* parent PID */
#endif
  sprintf(s, "Xlog_%d", ppid);
  return(fopen(s, "w"));
}  /* init_log_for_x_show */

/*************
 *
 *   my_process_id()
 *
 *************/

int my_process_id(void)
{
#ifdef TP_FORK
  return getpid();
#else
  return 0;
#endif
}  /* my_process_id */

