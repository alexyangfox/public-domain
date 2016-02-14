#ifndef ICI_CONF_H
#define ICI_CONF_H

#define BSD     43
/*#undef        BSD */

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

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

/*
 * End of ici.h export. --ici.h-end--
 */

#define ICI_USE_POSIX_THREADS
#define pthread_mutexattr_settype pthread_mutexattr_setkind_np
#define PTHREAD_MUTEX_RECURSIVE PTHREAD_MUTEX_RECURSIVE_NP

/*
 * Mentioned in the version string.
 */
#define CONFIG_STR      "Linux 2.x, libc 5 or 6"

#endif /*ICI_CONF_H*/
