#ifndef TP_OPTIONS_H
#define TP_OPTIONS_H

/* This file does not depend on header.h */

#include <stdio.h>
#include <limits.h>  /* for INT_MAX */
#include <string.h>  /* for strcmp() */

/*************
 *
 *    MACE_Flags are boolean valued options.  To install a new flag, append
 *    a new name and index to the end of this list, then insert code to
 *    initialize it in the routine `MACE_init_options'.
 *    Example access:  if (MACE_Flags[PARA_FROM_LEFT].val) { ... }
 *    See routine `MACE_init_options' for defaults.
 *
 *************/

#define MACE_MAX_FLAGS        100  /* increase if necessary */

#define PRINT_MODELS            0
#define SUBSUME                 1
#define PRINT_MODELS_PORTABLE   2
#define DISTINCT_CONSTANTS      3
#define PRINT_MODELS_IVY        4
#define QG_CONSTRAINT           5
#define RECORD_ASSIGNMENTS      6
#define CLAUSE_SPLIT            7
#define ISO_X                   8

/* end of MACE_Flags */

/*************
 *
 *    MACE_Parms are integer valued options.  To install a new parm, append
 *    a new name and index to this list, then insert code to
 *    initialize it in the routine `MACE_init_options'.
 *    Example access:  if (MACE_Parms[FPA_LITERALS].val == 4) { ... }
 *    See routine `MACE_init_options' for defaults.
 *
 *************/

#define MACE_MAX_PARMS   30 /* increase if necessary */

#define MACE_MAX_MEM      0 /* stop search after this many K bytes allocated */
#define MAX_TP_SECONDS    1
#define MAX_MODELS        2
#define ISO_CONSTANTS     3
#define PART_VARS         4

/* end of MACE_Parms */

struct flag {  /* MACE_Flags are boolean valued options */
    char *name;
    int val;
    };

struct parm {  /* MACE_Parms are integer valued options */
    char *name;
    int val;
    int min, max;  /* minimum and maximum permissible values */
    };

extern struct flag MACE_Flags[MACE_MAX_FLAGS];
extern struct parm MACE_Parms[MACE_MAX_PARMS];

/* function prototypes from options.c */

void MACE_init_options(void);
void MACE_print_options(FILE *fp);
void MACE_p_options(void);
void auto_MACE_change_flag(FILE *fp, int index, int val);
void MACE_dependent_flags(FILE *fp, int index);
void auto_MACE_change_parm(FILE *fp, int index, int val);
void MACE_dependent_parms(FILE *fp, int index);
int MACE_change_flag(FILE *fp, char *flag_name, int set);
int MACE_change_parm(FILE *fp, char *parm_name, int val);
void MACE_check_options(FILE *fp);

#endif  /* ! TP_OPTIONS_H */
