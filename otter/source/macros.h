/*
 *  macros.h -- This file contains some #define preprocessor macros
 *
 */

/*************
 *
 *    CPU_TIME(sec, usec) - It has been sec seconds + usec microseconds
 *        since the start of this process.
 *
 *************/

#if defined(TP_ABSOLUTELY_NO_CLOCKS)
#define CPU_TIME(sec, usec) {sec = usec = 0;}
#else
#ifdef TP_RUSAGE
#define CPU_TIME(sec, usec) \
{ \
    struct rusage r; \
    getrusage(RUSAGE_SELF, &r); \
    sec = r.ru_utime.tv_sec; \
    usec = r.ru_utime.tv_usec; \
}  /* CPU_TIME */

#else
#define CPU_TIME(sec, usec) {sec = usec = 0;}
#endif
#endif

/*************
 *
 *    CLOCK_START(clock_num) - Start or continue timing.
 *
 *        If the clock is already running, a warning message is printed.
 *
 *************/

#if defined(TP_ABSOLUTELY_NO_CLOCKS)
#define CLOCK_START(c)   /* empty string */
#else
#define CLOCK_START(c) \
{ \
  if (Flags[CLOCKS].val) { \
    struct clock *cp; \
 \
    cp = &Clocks[c]; \
    if (cp->curr_sec != -1) { \
	fprintf(stderr, "WARNING, CLOCK_START: clock %d already on.\n", c); \
	printf("WARNING, CLOCK_START: clock %d already on.\n", c); \
	} \
    else \
	CPU_TIME(cp->curr_sec, cp->curr_usec) \
  }\
}  /* CLOCK_START */
#endif

/*************
 *
 *    CLOCK_STOP(clock_num) - Stop timing and add to accumulated total.
 *
 *        If the clock not running, a warning message is printed.
 *
 *************/

#if defined(TP_ABSOLUTELY_NO_CLOCKS)
#define CLOCK_STOP(c)   /* empty string */
#else
#define CLOCK_STOP(c) \
{ \
  if (Flags[CLOCKS].val) { \
    long sec, usec; \
    struct clock *cp; \
 \
    cp = &Clocks[c]; \
    if (cp->curr_sec == -1) { \
	fprintf(stderr, "WARNING, CLOCK_STOP: clock %d already off.\n", c); \
	printf("WARNING, CLOCK_STOP: clock %d already off.\n", c); \
	} \
    else { \
	CPU_TIME(sec, usec) \
	cp->accum_sec += sec - cp->curr_sec; \
	cp->accum_usec += usec - cp->curr_usec; \
	cp->curr_sec = -1; \
	cp->curr_usec = -1; \
	} \
  }\
}  /* CLOCK_STOP */
#endif

/*************
 *
 *    SET_BIT, CLEAR_BIT, BIT.
 *
 *************/

/* SCRATCH_BIT is by several operations to temporarily mark terms.
 * When using it, make sure that no other operation is using it, and
 * make sure to clear it when done. */


#define SET_BIT(bits, flag)    (bits = bits | flag)
#define CLEAR_BIT(bits, flag)  (bits = bits & ~flag)
#define TP_BIT(bits, flag)        (bits & flag)

/* for terms: */

#define SCRATCH_BIT       01
#define ORIENTED_EQ_BIT   02

/* for clauses: */

#define SCRATCH_BIT       01
