#include <stdio.h>
#include <stdlib.h>
#include "Clock.h"

 struct MACE_clock MACE_Clocks[MACE_MAX_CLOCKS];

/*************
 *
 *    init_clocks() - Initialize all clocks.
 *
 *************/

void init_clocks(void)
{
    int i;
    for (i = 0; i < MACE_MAX_CLOCKS; i++)
	MACE_clock_reset(i);
}  /* init_clocks */

/*************
 *
 *    long MACE_clock_val(clock_num) - Returns accumulated time in milliseconds.
 *
 *    Clock need not be stopped.
 *
 *************/

long MACE_clock_val(int c)
{
    long msec, i, j;

    i = MACE_Clocks[c].accum_msec;
    if (MACE_Clocks[c].level == 0)
	return(i);
    else {
	MACE_CPU_TIME(msec)
	j = msec - MACE_Clocks[c].curr_msec;
	return(i+j);
	}
}  /* MACE_clock_val */

/*************
 *
 *    MACE_clock_reset(clock_num) - MACE_Clocks must be reset before being used.
 *
 *************/

void MACE_clock_reset(int c)
{
    MACE_Clocks[c].accum_msec = 0;
    MACE_Clocks[c].level = 0;
}  /* MACE_clock_reset */

