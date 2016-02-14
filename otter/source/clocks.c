/*
 *  clocks.c -- This file has various timing routines.  (Some of them
 *  have been moved to macros.h.)
 *
 */

#include "header.h"

/*************
 *
 *    clock_init() - Initialize all clocks.
 *
 *************/

void clock_init(void)
{
  int i;

  for (i=0; i<MAX_CLOCKS; i++)
    clock_reset(i);
}  /* clock_init */

/*
 *
 *    CPU_TIME(sec, usec) - It has been sec seconds + usec microseconds
 *    since the start of this process.
 *
 */

/* This routine has been made into a macro. */

/*
 *
 *    CLOCK_START(clock_num) - Start or continue timing.
 *
 *    If the clock is already running, a warning message is printed.
 *
 */

/* This routine has been made into a macro. */

/*
 *
 *    CLOCK_STOP(clock_num) - Stop timing and add to accumulated total.
 *
 *    If the clock not running, a warning message is printed.
 *
 */

/* This routine has been made into a macro. */

/*************
 *
 *    long clock_val(clock_num) - Returns accumulated time in milliseconds.
 *
 *    Clock need not be stopped.
 *
 *************/

long clock_val(int c)
{
  long sec, usec, i, j;

  i = (Clocks[c].accum_sec * 1000) + (Clocks[c].accum_usec / 1000);
  if (Clocks[c].curr_sec == -1)
    return(i);
  else {
    CPU_TIME(sec, usec)
      j = ((sec - Clocks[c].curr_sec) * 1000) +
      ((usec - Clocks[c].curr_usec) / 1000);
    return(i+j);
  }
}  /* clock_val */

/*************
 *
 *    clock_reset(clock_num) - Clocks must be reset before being used.
 *
 *************/

void clock_reset(int c)
{
  Clocks[c].accum_sec = Clocks[c].accum_usec = 0;
  Clocks[c].curr_sec = Clocks[c].curr_usec = -1;
}  /* clock_reset */

/*************
 *
 *   char *get_time() - get a string representation of current date and time
 *
 *************/

char *get_time(void)
{
#ifdef TP_ABSOLUTELY_NO_CLOCKS
  return("(date unknown).\n");
#else
  time_t i;
  i = time((time_t *) NULL);
  return(asctime(localtime(&i)));
#endif
}  /* get_time */

/*************
 *
 *    long system_time() - Return system time in milliseconds.
 *
 *************/

long system_time(void)
{
#ifdef TP_ABSOLUTELY_NO_CLOCKS
  return(0);
#else
#ifdef TP_RUSAGE
  struct rusage r;
  long sec, usec;

  getrusage(RUSAGE_SELF, &r);
  sec = r.ru_stime.tv_sec;
  usec = r.ru_stime.tv_usec;

  return((sec * 1000) + (usec / 1000));
#else
  return(0);
#endif
#endif
}  /* system_time */

/*************
 *
 *    long run_time() - Return run time in milliseconds.
 *
 *************/

long run_time(void)
{
#ifdef TP_ABSOLUTELY_NO_CLOCKS
  return((long) 0);
#else
#ifdef TP_RUSAGE
  struct rusage r;
  long sec, usec;

  getrusage(RUSAGE_SELF, &r);
  sec = r.ru_utime.tv_sec;
  usec = r.ru_utime.tv_usec;

  return((sec * 1000) + (usec / 1000));
#else
  long ticks;
  long sec;

  ticks = clock();
  sec = ((double) ticks / CLOCKS_PER_SEC) * 1000;
  return(sec);
#endif
#endif
}  /* run_time */

/*************
 *
 *     wall_seconds()
 *
 *************/

long wall_seconds(void)
{
#ifdef TP_ABSOLUTELY_NO_CLOCKS
  return((long) 0);
#else
  /* This is ANSI, and it seems to work for many OS. */
  time_t i;

  i = time((time_t *) NULL);
  return((long) i);
#endif
}  /* wall_seconds */
