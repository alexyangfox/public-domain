#ifndef ICI_CONF_H
#define ICI_CONF_H

#define BSD     43

#undef  NOMATH          /* Trig and etc. */
#define NOTRACE         /* For debugging. */
#undef  NOWAITFOR       /* Requires select() or similar system primitive. */
#undef  NOSYSTEM        /* Command interpreter (shell) escape. */
#undef  NOPIPES         /* Requires popen(). */
#define NODIR           /* Directory reading function */
#define NODLOAD         /* Dynamic loading of native machine code modules. */
#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#define NOPROFILE       /* Profiler, see profile.c. */
#undef  NOSIGNALS       /* ICI level signal handling */

/*
 * Mentioned in the version string.
 */
#define CONFIG_STR      "NeXTSTEP 2.x, 3.x, 4.x"

#endif /*ICI_CONF_H*/
