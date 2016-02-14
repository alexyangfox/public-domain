#include "Mace2.h"

 long MACE_Stats[MACE_MAX_STATS];
 struct MACE_clock MACE_Clocks[MACE_MAX_CLOCKS];
 int Internal_flags[MAX_INTERNAL_FLAGS];

extern int Domain_size;       /* owned by mace.c */
extern int Init_wall_seconds; /* owned by mace.c */

/*************
 *
 *   init_stats()
 *
 *************/

void init_stats()
{
    int i;

    init_clocks();
    for (i = 0; i < MAX_INTERNAL_FLAGS; i++)
	Internal_flags[i] = 0;
    for (i = 0; i < MACE_MAX_STATS; i++)
	MACE_Stats[i] = 0;
}  /* init_stats */

/*************
 *
 *    MACE_print_mem()
 *
 *************/

void MACE_print_mem(FILE *fp)
{
    fprintf(fp, "\n------------- memory usage ------------\n");

    fprintf(fp, "Memory dynamically allocated (MACE_tp_alloc): %d.\n", MACE_total_mem());

}  /* print_mem */

/*************
 *
 *   p_mem
 *
 *************/

void p_mem(void)
{
    MACE_print_mem(stdout);
}  /* p_mem */

/*************
 *
 *    MACE_print_stats(fp)
 *
 *************/

void MACE_print_stats(FILE *fp)
{
    fprintf(fp, "\n----- statistics for domain size %d ----\n", Domain_size);
    
    fprintf(fp, "Input:\n");
    fprintf(fp, "    Clauses input               %7ld\n", MACE_Stats[INPUT_CLAUSES]);
    fprintf(fp, "    Literal occurrences input   %7ld\n", MACE_Stats[LIT_OCC_INPUT]);
    fprintf(fp, "    Greatest atom               %7ld\n", MACE_Stats[GREATEST_ATOM]);
    fprintf(fp, "Unit preprocess:\n");
    fprintf(fp, "    Preprocess unit assignments %7ld\n", MACE_Stats[PREPROCESS_UNIT_ASSIGNS]);
    fprintf(fp, "    Clauses after subsumption   %7ld\n", MACE_Stats[CLAUSES_AFTER_SUB]);
    fprintf(fp, "    Literal occ. after subsump. %7ld\n", MACE_Stats[LIT_OCC_AFTER_SUB]);
    fprintf(fp, "    Selectable clauses          %7ld\n", MACE_Stats[SELECTABLE_CLAUSES]);
    fprintf(fp, "Decide:\n");
    fprintf(fp, "    Splits                      %7ld\n", MACE_Stats[SPLITS]);
    fprintf(fp, "    Unit assignments            %7ld\n", MACE_Stats[UNIT_ASSIGNS]);
    fprintf(fp, "    Failed paths                %7ld\n", MACE_Stats[FAILED_PATHS]);
    fprintf(fp, "Memory:\n");
    fprintf(fp, "    Memory malloced           %7ld K\n", MACE_Stats[MEM_MALLOCED] / 1024);
    fprintf(fp, "    Memory MACE_tp_alloced    %7d K\n", MACE_total_mem());
    fprintf(fp, "Time (seconds):\n");
    fprintf(fp, "    Generate ground clauses  %10.2f\n", MACE_clock_val(GENERATE_TIME) / 1000.);
    fprintf(fp, "    DPLL                     %10.2f\n", MACE_clock_val(DECIDE_TIME) / 1000.);
}  /* MACE_print_stats */

/*************
 *
 *    MACE_p_stats()
 *
 *************/

void MACE_p_stats(void)
{
    MACE_print_stats(stdout);
}  /* MACE_p_stats */

/*************
 *
 *    MACE_print_times(fp)
 *
 *************/

void MACE_print_times(FILE *fp)
{
    long t, min, hr;

    fprintf(fp, "\n=======================================\n");
    fprintf(fp, "Total times for run (seconds):\n");

    t = run_time();
    fprintf(fp, "    user CPU time            %10.2f  ", t / 1000.);
    t = t / 1000; hr = t / 3600; t = t % 3600; min = t / 60; t = t % 60;
    fprintf(fp, " (%ld hr, %ld min, %ld sec)\n", hr, min, t); 

    t = system_time();
    fprintf(fp, "    system CPU time          %10.2f  ", t/ 1000.);
    t = t / 1000; hr = t / 3600; t = t % 3600; min = t / 60; t = t % 60;
    fprintf(fp, " (%ld hr, %ld min, %ld sec)\n", hr, min, t); 

    t = wall_seconds() - Init_wall_seconds;
    fprintf(fp, "    wall-clock time          %7ld     ", t);
    hr = t / 3600; t = t % 3600; min = t / 60; t = t % 60;
    fprintf(fp, " (%ld hr, %ld min, %ld sec)\n", hr, min, t); 

}  /* MACE_print_times */

/*************
 *
 *    MACE_output_stats
 *
 *************/

void MACE_output_stats(FILE *fp)
{
    MACE_print_stats(fp);
    MACE_print_times(fp);
}  /* MACE_output_stats */

