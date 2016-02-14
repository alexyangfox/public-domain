/* file "platform_dependent.h" */

/*
 *  This file contains the interface to the platform_dependent module.  This
 *  module contains code that is compiled differently for different systems
 *  because it needs functionality beyond the ANSI C 1990 standard.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef PLATFORM_DEPENDENT_H
#define PLATFORM_DEPENDENT_H


#include <stdio.h>
#include <time.h>
#include "c_foundations/basic.h"
#include "o_integer.h"


typedef struct dynamic_library_handle dynamic_library_handle;


extern int redirected_system(const char *command,
        char **result_standard_output, char **result_standard_error,
        boolean *error);
extern o_integer oi_tell(FILE *fp,
        void (*error_handler)(void *data, const char *format, ...),
        void *data);
extern void oi_seek(FILE *fp, o_integer position,
        void (*error_handler)(void *data, const char *format, ...),
        void *data);
extern void seek_end(FILE *fp,
        void (*error_handler)(void *data, const char *format, ...),
        void *data);
extern time_t get_time_with_microseconds(unsigned long *microseconds);
extern const char *time_zone_name_from_tm(struct tm *the_tm,
        boolean *know_if_daylight_savings, boolean *is_daylight_savings,
        boolean *know_seconds_ahead_of_utc, long *seconds_ahead_of_utc);
extern char **directory_contents(const char *directory_name,
        void (*error_handler)(void *data, const char *format, ...),
        void *data);
extern boolean file_exists(const char *name);
extern boolean directory_exists(const char *name);
extern boolean path_is_absolute(const char *path);
extern char *file_name_directory(const char *name, boolean *error);
extern boolean is_dynamic_library_name(const char *name);
extern dynamic_library_handle *open_dynamic_library(const char *name,
        boolean *file_not_found, char **error_message);
extern void *find_dynamic_library_symbol(dynamic_library_handle *handle,
                                         const char *symbol_name);
extern void close_dynamic_library(dynamic_library_handle *handle);
extern char **alternate_dynamic_library_names(const char *original_name);
extern boolean current_process_stack_size_limit_known(void);
extern size_t current_process_stack_size_limit(void);

extern void cleanup_platform_dependent_module(void);


#ifdef __CYGWIN__
#define PATH_SEPARATOR ";"
#define DLL_SUFFIX ".dll"
#define EXECUTABLE_SUFFIX ".exe"
#else
#define PATH_SEPARATOR ":"
#define DLL_SUFFIX ".so"
#define EXECUTABLE_SUFFIX ""
#endif


#ifndef NEVER_USE_SALMON_DLL
#ifndef ALWAYS_USE_SALMON_DLL
#if (defined(__CYGWIN__))
#define ALWAYS_USE_SALMON_DLL
#endif /* (defined(__CYGWIN__)) */
#endif /* !ALWAYS_USE_SALMON_DLL */
#endif /* !NEVER_USE_SALMON_DLL */


/*
 *  Eventually, we might want to put something here to automatically set
 *  USE_CLOCK_GETTIME_REALTIME or USE_UCLOCK on some systems.  The default
 *  right now is to use clock() on all systems for profiling.  That's fine
 *  because it's very portable, but clock() doesn't have very good resolution
 *  on some systems.  On Linux, clock_gettime() gives better resolution, but it
 *  requires linking with -lrt.  On non-Linux systems, there is often no rt
 *  library, so -lrt causes a link failure.  So, to avoid some ugly makefile
 *  complexity, we don't turn on USE_CLOCK_GETTIME_REALTIME by default for
 *  Linux.  And there are at least some versions of Linux don't support
 *  uclock(), which would be another alternative for higher resolution.
 */


#define AUTO_FIND_CLOCK_RESOLUTION_SECONDS(x) \
  { \
    CLOCK_T start; \
 \
    GET_CLOCK(start); \
    while (TRUE) \
      { \
        CLOCK_T end; \
        CLOCK_T diff; \
        double result; \
 \
        GET_CLOCK(end); \
        CLOCK_DIFF(diff, start, end); \
        result = CLOCK_TO_DOUBLE_SECONDS(diff); \
        if (result != 0) \
          { \
            x = result; \
            break; \
          } \
      } \
  }

#ifdef USE_CLOCK_GETTIME_REALTIME

#define BILLION 1000000000
#define CLOCK_T struct timespec
#include <errno.h>
#define GET_CLOCK(x) clock_gettime(CLOCK_REALTIME, &(x))
#define CLOCK_DIFF(x, start, end) \
    x.tv_sec = (end).tv_sec - (start).tv_sec; \
    if ((end).tv_nsec >= (start).tv_nsec) \
      { \
        x.tv_nsec = (end).tv_nsec - (start).tv_nsec; \
      } \
    else \
      { \
        --(x.tv_sec); \
        x.tv_nsec = ((end).tv_nsec + BILLION) - (start).tv_nsec; \
      }
#define CLOCK_ADD(x, left, right) \
    x.tv_sec = (left).tv_sec + (right).tv_sec; \
    x.tv_nsec = (left).tv_nsec + (right).tv_nsec; \
    if (x.tv_nsec >= BILLION) \
      { \
        ++(x.tv_sec); \
        x.tv_nsec -= BILLION; \
      }
#define CLOCK_TO_DOUBLE_SECONDS(x) \
    (((double)((x).tv_sec)) + (((double)((x).tv_nsec)) / BILLION))
#define CLOCK_ZERO(x) (x).tv_sec = 0; (x).tv_nsec = 0
#define PRINT_CLOCK_SOURCE(fp) \
    fprintf(fp, "real time from clock_gettime(CLOCK_REALTIME, ...)")

/*
 *  We could use clock clock_getres(CLOCK_REALTIME, ...) to get the resolution,
 *  but it turns out that on at least some Linux systems this returns a value
 *  that is much greater than the actual resolution that clock_gettime() will
 *  give us.  In fact, it seems to be the resolution of clock().
 *
 *  So, instead we just figure out the answer by experimentation.
 */
#define CLOCK_RESOLUTION_SECONDS(x) AUTO_FIND_CLOCK_RESOLUTION_SECONDS(x)

#elif defined(USE_UCLOCK)

#define CLOCK_T uclock_t
#define GET_CLOCK(x) x = uclock()
#define CLOCK_DIFF(x, start, end) x = (end) - (start)
#define CLOCK_ADD(x, start, end) x = (end) + (start)
#define CLOCK_TO_DOUBLE_SECONDS(x) (((double)(x)) / ((double)UCLOCKS_PER_SEC))
#define CLOCK_ZERO(x) x = 0
#define PRINT_CLOCK_SOURCE(fp) fprintf(fp, "processor time from uclock()")
#define CLOCK_RESOLUTION_SECONDS(x) AUTO_FIND_CLOCK_RESOLUTION_SECONDS(x)

#else /* use clock() */

#define CLOCK_T clock_t
#define GET_CLOCK(x) x = clock()
#define CLOCK_DIFF(x, start, end) x = (end) - (start)
#define CLOCK_ADD(x, start, end) x = (end) + (start)
#define CLOCK_TO_DOUBLE_SECONDS(x) (((double)(x)) / ((double)CLOCKS_PER_SEC))
#define CLOCK_ZERO(x) x = 0
#define PRINT_CLOCK_SOURCE(fp) fprintf(fp, "processor time from clock()")
#define CLOCK_RESOLUTION_SECONDS(x) AUTO_FIND_CLOCK_RESOLUTION_SECONDS(x)

#endif /* use clock() */


#ifndef NEVER_CLEAR_MEM_BEFORE_PTHREAD_MUTEX_INIT
#ifndef ALWAYS_CLEAR_MEM_BEFORE_PTHREAD_MUTEX_INIT
#if (defined(__CYGWIN__))

/*
 *  On Cygwin, I've found that if pthread_mutex_init() is called on
 *  uninitialized memory, sometimes it fails with a return code of EBUSY, which
 *  indicates the system thinks the lock has already been initialized but not
 *  destroyed.  Clearing the memory to zero first fixes the problem.
 */
#define ALWAYS_CLEAR_MEM_BEFORE_PTHREAD_MUTEX_INIT

#endif /* (defined(__CYGWIN__)) */
#endif /* !ALWAYS_CLEAR_MEM_BEFORE_PTHREAD_MUTEX_INIT */
#endif /* !NEVER_CLEAR_MEM_BEFORE_PTHREAD_MUTEX_INIT */

#ifdef ALWAYS_CLEAR_MEM_BEFORE_PTHREAD_MUTEX_INIT
#include <string.h>
#define PRE_PTHREAD_MUTEX_INIT(lock) \
        memset(&(lock), 0, sizeof(pthread_mutex_t))
#else /* !ALWAYS_CLEAR_MEM_BEFORE_PTHREAD_MUTEX_INIT */
#define PRE_PTHREAD_MUTEX_INIT(lock)
#endif /* !ALWAYS_CLEAR_MEM_BEFORE_PTHREAD_MUTEX_INIT */

#ifndef MULTI_THREADED

#define DECLARE_SYSTEM_LOCK(name)
#define INITIALIZE_SYSTEM_LOCK(lock, fail)
#define DESTROY_SYSTEM_LOCK(lock)
#define GRAB_SYSTEM_LOCK(lock)
#define RELEASE_SYSTEM_LOCK(lock)

#else /* MULTI_THREADED */

#include <pthread.h>

#define DECLARE_SYSTEM_LOCK(name) pthread_mutex_t name

#define DO_SYSTEM_LOCK_OPERATION(lock, operation) \
  { \
    int retcode = pthread_mutex_ ## operation lock; \
    assert(retcode == 0); \
  }

#define INITIALIZE_SYSTEM_LOCK(lock, fail) \
  { \
    PRE_PTHREAD_MUTEX_INIT(lock); \
    int retcode = pthread_mutex_init(&(lock), NULL); \
    if (retcode != 0) \
      { \
        fail; \
      } \
  }

#define DESTROY_SYSTEM_LOCK(lock) DO_SYSTEM_LOCK_OPERATION((&(lock)), destroy)
#define GRAB_SYSTEM_LOCK(LOCK) DO_SYSTEM_LOCK_OPERATION((&(LOCK)), lock)
#define RELEASE_SYSTEM_LOCK(lock) DO_SYSTEM_LOCK_OPERATION((&(lock)), unlock)

#ifndef NDEBUG

typedef struct thread_lock_info thread_lock_info;

typedef struct
  {
    unsigned long magic;
    pthread_mutex_t mutex;
    thread_lock_info *owner;
    const char *locked_file;
    unsigned long locked_line_num;
    const char *name;
    const char *initialization_file;
    unsigned long initialization_line_num;
  } lock_plus;

struct thread_lock_info
  {
    lock_plus *waiting_for;
    const char *waiting_file;
    unsigned long waiting_line_num;
    pthread_t handle;
    thread_lock_info *next;
  };

extern pthread_mutex_t meta_lock;

extern thread_lock_info *current_thread_lock_info(void);

#define GOOD_LOCK_MAGIC 0x878de780
#define BAD_LOCK_MAGIC 0x878ed780

#undef DECLARE_SYSTEM_LOCK
#define DECLARE_SYSTEM_LOCK(name) lock_plus name

#undef INITIALIZE_SYSTEM_LOCK
#define INITIALIZE_SYSTEM_LOCK(lock, fail) \
  { \
    lock.magic = GOOD_LOCK_MAGIC; \
    PRE_PTHREAD_MUTEX_INIT(lock.mutex); \
    int retcode = pthread_mutex_init(&(lock.mutex), NULL); \
    if (retcode != 0) \
      { \
        fail; \
      } \
    lock.owner = NULL; \
    lock.locked_file = NULL; \
    lock.locked_line_num = 0; \
    lock.name = #lock; \
    lock.initialization_file = __FILE__; \
    lock.initialization_line_num = __LINE__; \
  }

#undef DESTROY_SYSTEM_LOCK
#define DESTROY_SYSTEM_LOCK(lock) \
  { \
    pthread_mutex_lock(&meta_lock); \
    assert(lock.magic == GOOD_LOCK_MAGIC); \
    assert(lock.owner == NULL); \
    int retcode = pthread_mutex_destroy(&(lock.mutex)); \
    assert(retcode == 0); \
    lock.magic = BAD_LOCK_MAGIC; \
    pthread_mutex_unlock(&meta_lock); \
  }

#undef GRAB_SYSTEM_LOCK
#define GRAB_SYSTEM_LOCK(lock) \
  { \
    thread_lock_info *me; \
    thread_lock_info *follow; \
 \
    me = current_thread_lock_info(); \
    pthread_mutex_lock(&meta_lock); \
    assert(lock.magic == GOOD_LOCK_MAGIC); \
    assert(me->waiting_for == NULL); \
    me->waiting_for = &lock; \
    me->waiting_file = __FILE__; \
    me->waiting_line_num = __LINE__; \
    follow = lock.owner; \
    while (follow != NULL) \
      { \
        if (follow == me) \
          { \
            fprintf(stderr, "Internal Error: deadlock.\n"); \
            follow = me; \
            while (TRUE) \
              { \
                lock_plus *waiting_for; \
                thread_lock_info *next; \
 \
                waiting_for = follow->waiting_for; \
                next = waiting_for->owner; \
                fprintf(stderr, \
                        "Internal Error:   process %p is waiting at %s:%lu " \
                        "for ", (void *)(follow->handle), \
                        follow->waiting_file, follow->waiting_line_num); \
                fprintf(stderr, "lock `%s'[%s:%lu]", waiting_for->name, \
                        waiting_for->initialization_file, \
                        waiting_for->initialization_line_num); \
                fprintf(stderr, ", which is held by %p as of %s:%lu.\n", \
                        (void *)(next->handle), waiting_for->locked_file, \
                        waiting_for->locked_line_num); \
                follow = next; \
                if (follow == me) \
                    break; \
              } \
            break; \
          } \
        if (follow->waiting_for == NULL) \
            break; \
        follow = follow->waiting_for->owner; \
      } \
    pthread_mutex_unlock(&meta_lock); \
    int retcode = pthread_mutex_lock(&(lock.mutex)); \
    assert(retcode == 0); \
    pthread_mutex_lock(&meta_lock); \
    assert(me->waiting_for == &lock); \
    me->waiting_for = NULL; \
    assert(lock.owner == NULL); \
    lock.owner = me; \
    lock.locked_file = __FILE__; \
    lock.locked_line_num = __LINE__; \
    pthread_mutex_unlock(&meta_lock); \
  }

#undef RELEASE_SYSTEM_LOCK
#define RELEASE_SYSTEM_LOCK(lock) \
  { \
    thread_lock_info *me; \
 \
    me = current_thread_lock_info(); \
    pthread_mutex_lock(&meta_lock); \
    assert(lock.magic == GOOD_LOCK_MAGIC); \
    assert(lock.owner == me); \
    assert(me->waiting_for == NULL); \
    lock.owner = NULL; \
    pthread_mutex_unlock(&meta_lock); \
    int retcode = pthread_mutex_unlock(&(lock.mutex)); \
    assert(retcode == 0); \
  }

#endif /* !NDEBUG */

#endif /* MULTI_THREADED */


#endif /* PLATFORM_DEPENDENT_H */
