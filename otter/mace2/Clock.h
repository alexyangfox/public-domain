#ifndef TP_CLOCKS_H
#define TP_CLOCKS_H

#include <time.h>
#ifdef TP_RUSAGE         /* getrusage() */
#  include <sys/time.h>  /* needed for SunOS */
#  include <sys/resource.h>
#endif

/*************
 *
 *    MACE_Clocks.  To install a new clock, append a new name and
 *    index to this list, then insert the code to output it in the
 *    routine `MACE_print_times'.  Example of use: MACE_CLOCK_START(INPUT_TIME),
 *    MACE_CLOCK_STOP(INPUT_TIME),  micro_sec = MACE_clock_val(INPUT_TIME);.
 *
 *************/

#define MACE_MAX_CLOCKS          40
 
#define GENERATE_TIME        0
#define DECIDE_TIME          1

struct MACE_clock {  /* for timing, see cos.h, macros.h, and clocks.c */
    unsigned long accum_msec;   /* accumulated time */
    unsigned long curr_msec;    /* time since clock has been turned on */
    int level;         /* STARTs - STOPs */
    };

extern struct MACE_clock MACE_Clocks[MACE_MAX_CLOCKS];

/*************
 *
 *    MACE_CPU_TIME(msec) - It has been sec milliseconds  (UNIX user time)
 *        since the start of this process.
 *
 *************/

#ifdef TP_RUSAGE
#define MACE_CPU_TIME(msec)  \
{  \
    struct rusage r;  \
    getrusage(RUSAGE_SELF, &r);  \
    msec = r.ru_utime.tv_sec * 1000 + r.ru_utime.tv_usec / 1000;  \
}  /* MACE_CPU_TIME */
#else
#define MACE_CPU_TIME(msec) {msec = 0;}
#endif

/*************
 *
 *    MACE_CLOCK_START(clock_num) - Start or continue timing.
 *
 *************/

#ifdef NO_CLOCK
#define MACE_CLOCK_START(c)   /* empty string */
#else
#define MACE_CLOCK_START(c)  \
{  \
    struct MACE_clock *cp;  \
    cp = &MACE_Clocks[c];  \
    cp->level++; \
    if (cp->level == 1) \
	MACE_CPU_TIME(cp->curr_msec) \
}  /* MACE_CLOCK_START */
#endif

/*************
 *
 *    MACE_CLOCK_STOP(clock_num) - Stop timing and add to accumulated total.
 *
 *    If the clock not running, a warning message is printed.
 *
 *************/

#ifdef NO_CLOCK
#define MACE_CLOCK_STOP(c)   /* empty string */
#else
#define MACE_CLOCK_STOP(c)  \
{  \
    long msec;  \
    struct MACE_clock *cp;  \
    cp = &MACE_Clocks[c];  \
    cp->level--; \
    if (cp->level < 0) {  \
	fprintf(stderr, "\007WARNING, MACE_CLOCK_STOP: clock %d not running.\n", c);  \
	printf("WARNING, MACE_CLOCK_STOP: clock %d not running.\n", c);  \
	cp->level = 0; \
	}  \
    else if (cp->level == 0) {  \
	MACE_CPU_TIME(msec)  \
	cp->accum_msec += msec - cp->curr_msec;  \
	}  \
}  /* MACE_CLOCK_STOP */
#endif

/* function prototypes from clocks.c */

void init_clocks(void);
long MACE_clock_val(int c);
void MACE_clock_reset(int c);

#endif  /* ! TP_CLOCKS_H */
