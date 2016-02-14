#ifndef ICI_CONF_H
#define ICI_CONF_H

/*
 * The following macros control the inclusion of various ICI intrinsic
 * functions. If the macro is defined the functions are NOT include
 * (hence the NO at the start of the name). By undef'ing the macro you
 * are stating that the functions should be included and compile (and
 * possibly even work) for the particular port of ICI.
 */
#define NOMATH          /* Trig and etc. */
#define NOTRACE         /* For debugging. */
#define NOWAITFOR       /* Requires select() or similar system primitive. */
#define NOSYSTEM        /* Command interpreter (shell) escape. */
#define NOPIPES         /* Requires popen(). */
#define NODIR           /* Directory reading function, dir(). */
#define NODLOAD         /* Dynamic loading of native machine code modules. */
#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#define NOPROFILE       /* Profiler, see profile.c. */
#undef  NOSIGNALS       /* ICI level signal handling */

/*
 * This string gets compiled into the executable.
 */
#define CONFIG_STR      "This string gets compiled into the ICI executable"

#endif /*ICI_CONF_H*/
