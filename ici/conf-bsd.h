#ifndef ICI_CONF_H
#define ICI_CONF_H

#include <sys/param.h>
#undef isset

#include <math.h>

#ifndef BSD4_4
/*
 * Prior to 4.4BSD sprintf didn't return a count of the characters
 * written to the buffer.
 */
#define BAD_PRINTF_RETVAL
#endif

#undef  NOMATH          /* Trig and etc. */
#define NOTRACE         /* For debugging. */
#undef  NOWAITFOR       /* Requires select() or similar system primitive. */
#undef  NOSYSTEM        /* Command interpreter (shell) escape. */
#undef  NOPIPES         /* Requires popen(). */
#undef  NODIR           /* Directory reading function, dir(). */
#undef  NODLOAD         /* Dynamic loading of native machine code modules. */
#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#undef  NOPROFILE       /* Profiler, see profile.c. */
#undef  NOSIGNALS       /* ICI level signal handling */

#ifdef _THREAD_SAFE
#define ICI_USE_POSIX_THREADS
#endif

#if defined(__FreeBSD__) && (__FreeBSD__ < 3)
/*
 * Pre-3.0 FreeBSD uses a.out format objects that appends a leading "_"
 * to symbol names. 3.0 and later are ELF that doesn't require this.
 */
#define NEED_UNDERSCORE_ON_SYMBOLS
#define CONFIG_STR      "FreeBSD 2.x configuration"
#else
#define CONFIG_STR      "FreeBSD 3.x,4.x,5.x configuration"
#endif

#endif /*ICI_CONF_H*/
