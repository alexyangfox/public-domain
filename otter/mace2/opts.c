#include "Opts.h"

 struct flag MACE_Flags[MACE_MAX_FLAGS];
 struct parm MACE_Parms[MACE_MAX_PARMS];

/*************
 *
 *    MACE_init_options()
 *
 *************/

void MACE_init_options(void)
{
    int i;
    struct flag *f;
    struct parm *p;

    for (i = 0; i < MACE_MAX_FLAGS; i++)
	MACE_Flags[i].name = "";
    for (i = 0; i < MACE_MAX_PARMS; i++)
	MACE_Parms[i].name = "";

    /* MACE_Flags are Boolean-valued options */

    f = &(MACE_Flags[PRINT_MODELS]);
    f->name = "print_models";
    f->val = 0;

    f = &(MACE_Flags[PRINT_MODELS_PORTABLE]);
    f->name = "print_models_portable";
    f->val = 0;

    f = &(MACE_Flags[PRINT_MODELS_IVY]);
    f->name = "print_models_ivy";
    f->val = 0;

    f = &(MACE_Flags[SUBSUME]);
    f->name = "subsume";
    f->val = 0;

    f = &(MACE_Flags[DISTINCT_CONSTANTS]);
    f->name = "distinct_constants";
    f->val = 0;

    f = &(MACE_Flags[QG_CONSTRAINT]);
    f->name = "qg_constraint";
    f->val = 0;

    f = &(MACE_Flags[RECORD_ASSIGNMENTS]);
    f->name = "record_assignments";
    f->val = 0;

    f = &(MACE_Flags[CLAUSE_SPLIT]);
    f->name = "clause_split";
    f->val = 0;

    f = &(MACE_Flags[ISO_X]);
    f->name = "iso_x";
    f->val = 0;

    /* MACE_Parms are integer-valued options */

    p = &(MACE_Parms[MACE_MAX_MEM]);
    p->name = "max_mem";
    p->min = 0;
    p->max = INT_MAX;
    p->val = 96000;

    p = &(MACE_Parms[MAX_TP_SECONDS]);
    p->name = "max_seconds";
    p->min = 0;
    p->max = INT_MAX;
    p->val = INT_MAX;

    p = &(MACE_Parms[MAX_MODELS]);
    p->name = "max_models";
    p->min = 1;
    p->max = INT_MAX;
    p->val = 1;

    p = &(MACE_Parms[ISO_CONSTANTS]);
    p->name = "iso_constants";
    p->min = 0;
    p->max = INT_MAX;
    p->val = 5;

    p = &(MACE_Parms[PART_VARS]);
    p->name = "part_vars";
    p->min = 0;
    p->max = INT_MAX;
    p->val = 1;

}  /* MACE_init_options */

/*************
 *
 *    MACE_print_options(fp)
 *
 *************/

void MACE_print_options(FILE *fp)
{
    int i, j;

    fprintf(fp, "\n--------------- options ---------------\n");

    j = 0;
    for (i = 0; i < MACE_MAX_FLAGS; i++)  /* print set flags */
	if (MACE_Flags[i].name[0] != '\0') {
            fprintf(fp, "%s", MACE_Flags[i].val ? "set(" : "clear(");
	    fflush(stdout);
	    fprintf(fp, "%s). ", MACE_Flags[i].name);
	    fflush(stdout);
	    j++;
	    if (j % 3 == 0)
	        fprintf(fp, "\n");
	    fflush(stdout);
            }

    fprintf(fp, "\n\n");

    j = 0;
    for (i = 0; i < MACE_MAX_PARMS; i++)  /* print parms */
	if (MACE_Parms[i].name[0] != '\0') {
	    fprintf(fp, "assign(");
	    fprintf(fp, "%s, %d). ", MACE_Parms[i].name, MACE_Parms[i].val);
	    j++;
	    if (j % 3 == 0)
		fprintf(fp, "\n");
	    }
    fprintf(fp, "\n");

}  /* MACE_print_options */

/*************
 *
 *    MACE_p_options()
 *
 *************/

void MACE_p_options(void)
{
    MACE_print_options(stdout);
}  /* MACE_p_options */

/*************
 *
 *   auto_MACE_change_flag()
 *
 *************/

void auto_MACE_change_flag(FILE *fp, int index, int val)
{
    if (MACE_Flags[index].val != val) {
	fprintf(fp, "   dependent: %s(%s).\n",
		val ? "set" : "clear", MACE_Flags[index].name);
	MACE_Flags[index].val = val;
	MACE_dependent_flags(fp, index);
	}
}  /* auto_MACE_change_flag */

/*************
 *
 *   void MACE_dependent_flags(FILE *fp, int index)
 *
 *   Flag[index] has just been changed.  Change any flags or parms that
 *   depend on it.  Write actions to *fp.
 *
 *   Mutually recursive with auto_MACE_change_flag and auto_MACE_change_parm.
 *
 *************/

void MACE_dependent_flags(FILE *fp, int index)
{
    /* This part handles flags that have just been set. */

    if (MACE_Flags[index].val) {

	switch (index) {
	    }
	}

    /* This part handles flags that have just been cleared. */

    if (MACE_Flags[index].val) {
	switch (index) {
	    }
	}
}  /* MACE_dependent_flags */

/*************
 *
 *   auto_MACE_change_parm()
 *
 *************/

void auto_MACE_change_parm(FILE *fp, int index, int val)
{
    if (MACE_Parms[index].val != val) {
	fprintf(fp, "   dependent: assign(%s, %d).\n",
		MACE_Parms[index].name, val);
		
	MACE_Parms[index].val = val;
	MACE_dependent_parms(fp, index);
	}
}  /* auto_MACE_change_parm */

/*************
 *
 *   void MACE_dependent_parms(FILE *fp, int index)
 *
 *   MACE_Parms[index] has just been changed.  Change any flags or parms that
 *   depend on it.  Write actions to *fp.
 *
 *   Mutually recursive with auto_MACE_change_flag and auto_MACE_change_parm.
 *
 *************/

void MACE_dependent_parms(FILE *fp, int index)
{
    switch (index) {
	}
}  /* MACE_dependent_parms */

/*************
 *
 *    int MACE_change_flag(fp, flag_name, set)
 *
 *    If success, return index of flag, if fail, return -1.
 *    Warning and error messages go to file fp.
 *
 *************/

int MACE_change_flag(FILE *fp, char *flag_name, int set)
{
    int index, found;

    found = 0;
    index = 0;
    while (index < MACE_MAX_FLAGS && !found)
	if (strcmp(flag_name, MACE_Flags[index].name) == 0)
	    found = 1;
	else
	    index++;
    if (!found) {
	fprintf(fp, "ERROR: flag `%s' not found.\n", flag_name);
	return(-1);
	}
    else if (MACE_Flags[index].val == set) {
	fprintf(fp, "WARNING: ");
	if (set)
	    fprintf(fp, " flag `%s' already set.\n", flag_name);
	else
	    fprintf(fp, " flag `%s' already clear.\n", flag_name);
	return(index);
	}
    else {
	MACE_Flags[index].val = set;
	return(index);
	}
}  /* MACE_change_flag */

/*************
 *
 *    int MACE_change_parm(fp, parm_name, val)
 *
 *    If success, return index of parm, if fail, return -1.
 *    Warning and error messages go to file fp.
 *
 *************/

int MACE_change_parm(FILE *fp, char *parm_name, int val)
{
    int index, found;

    found = 0;
    index = 0;
    while (index < MACE_MAX_PARMS && !found)
	if (strcmp(parm_name, MACE_Parms[index].name) == 0)
	    found = 1;
	else
	    index++;
    if (!found) {
	fprintf(fp, "ERROR: parameter `%s' not found.\n", parm_name);
	return(-1);
	}
    else if (val < MACE_Parms[index].min || val > MACE_Parms[index].max) {
	fprintf(fp, "ERROR: assign(%s, %d),", parm_name, val);
	fprintf(fp, " integer must be in range [%d,%d].\n",
		MACE_Parms[index].min, MACE_Parms[index].max);
	return(-1);
	}
    else if (val == MACE_Parms[index].val) {
	fprintf(fp, "WARNING: assign(%s, %d),", parm_name, val);
	fprintf(fp, " already has that value.\n");
	return(index);
	}
    else {
	MACE_Parms[index].val = val;
	return(index);
	}
}  /* MACE_change_parm */

/*************
 *
 *    MACE_check_options(fp)  --  check for inconsistent or odd settings
 *
 *    If a bad combination of settings is found, either a warning
 *    message is printed, or an ABEND occurs.
 *
 *************/

void MACE_check_options(FILE *fp)
{
    fprintf(fp, "Called MACE_check_options.\n");
}  /* MACE_check_options */

