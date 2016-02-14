#ifndef TP_STATS_H
#define TP_STATS_H

/*************
 *
 *    Statistics.  To install a new statistic, append a new name and
 *    index to this list, then insert the code to output it in the
 *    routine `MACE_print_stats'.
 *    Example access:  MACE_Stats[INPUT_ERRORS]++;
 *
 *************/

#define MACE_MAX_STATS        40  /* increase if necessary */

#define FAILED_PATHS              3
#define UNIT_ASSIGNS              4
#define SPLITS                    5
#define PREPROCESS_UNIT_ASSIGNS   6
#define CLAUSES_AFTER_SUB         7
#define INPUT_CLAUSES             8
#define GREATEST_ATOM             9
#define LIT_OCC_INPUT            10
#define LIT_OCC_AFTER_SUB        11
#define MEM_MALLOCED             12
#define SELECTABLE_CLAUSES       13

/* end of MACE_Stats */

#define MAX_INTERNAL_FLAGS  10

extern long MACE_Stats[MACE_MAX_STATS];
extern int Internal_flags[MAX_INTERNAL_FLAGS];

/* function prototypes from stats.c */

void init_stats();
void MACE_print_mem(FILE *fp);
void p_mem(void);
void MACE_print_stats(FILE *fp);
void MACE_p_stats(void);
void MACE_print_times(FILE *fp);
void MACE_p_times(void);
void MACE_output_stats(FILE *fp);

#endif  /* ! TP_STATS_H */
