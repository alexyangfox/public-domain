#ifndef ICI_CONF_H
#define ICI_CONF_H

#define BSD     43
#define BAD_PRINTF_RETVAL

#undef  NOMATH          /* Trig and etc. */
#define NOTRACE         /* For debugging. */
#undef  NOWAITFOR       /* Requires select() or similar system primitive. */
#undef  NOSYSTEM        /* Command interpreter (shell) escape. */
#undef  NOPIPES         /* Requires popen(). */
#undef  NODIR           /* Directory reading function, dir(). */
#define NODLOAD         /* Dynamic loading of native machine code modules. */
#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#define NOPROFILE       /* Profiler, see profile.c. */
#undef  NOSIGNALS       /* ICI level signal handling */

/*
 * Mentioned in the version string.
 */
#define CONFIG_STR      "SunOS 4.x"

#include <sys/param.h>
#undef isset

#ifndef FILENAME_MAX
#define FILENAME_MAX    MAXPATHLEN
#endif

#endif /*ICI_CONF_H*/
