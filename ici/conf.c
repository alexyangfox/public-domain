#define ICI_CORE
#include "fwd.h"
#include "func.h"

extern ici_cfunc_t  std_cfuncs[];
extern ici_cfunc_t  ici_re_funcs[];
extern ici_cfunc_t  ici_oo_funcs[];
extern ici_cfunc_t  ici_apl_funcs[];
extern ici_cfunc_t  clib_cfuncs[];
extern ici_cfunc_t  extra_cfuncs[];
extern ici_cfunc_t  load_cfuncs[];
extern ici_cfunc_t  parse_cfuncs[];

#ifndef NOTRACE
extern ici_cfunc_t  trace_cfuncs[];
#endif
#ifndef NOEVENTS
extern ici_cfunc_t  ici_event_cfuncs[];
#endif
#ifndef NOPROFILE
extern ici_cfunc_t  ici_profile_cfuncs[];
#endif
#ifndef NOSIGNALS
extern ici_cfunc_t  ici_signals_cfuncs[];
#endif
#ifndef NODEBUGGING
extern ici_cfunc_t  ici_debug_cfuncs[];
#endif
extern ici_cfunc_t  ici_thread_cfuncs[];

ici_cfunc_t *funcs[] =
{
    std_cfuncs,
    ici_re_funcs,
    ici_oo_funcs,
    ici_apl_funcs,
    clib_cfuncs,
    extra_cfuncs,
    load_cfuncs,
    parse_cfuncs,
#ifndef NOTRACE
    trace_cfuncs,
#endif
#ifndef NODEBUGGING
    ici_debug_cfuncs,
#endif
#ifndef NOEVENTS
    ici_event_cfuncs,
#endif
#ifndef NOPROFILE
    ici_profile_cfuncs,
#endif
#ifndef NOSIGNALS
    ici_signals_cfuncs,
#endif
    ici_thread_cfuncs,
    NULL
};

/*
 * All this does is define a string to identify the version number
 * of the interpreter. Update for new releases.
 *
 * Note test for defined(_WIN32) - MS C defines __STDC__ only when in
 * strict ANSI mode (/Za option) but this causes other problems.
 *
 */
#if defined(__STDC__) || defined(_WIN32)
/*
 * Eg: @(#)ICI 2.0.1, conf-sco.h, Apr 20 1994 11:42:12, SCO config (math win waitfor pipes )
 *
 * Note that the version number also occurs in some defines in fwd.h (ICI_*_VER)
 * and Makefile.maint.
 */
char ici_version_string[] =
    "@(#)ICI 4.1.0, "
    CONFIG_FILE ", "
    __DATE__ " " __TIME__ ", "
    CONFIG_STR
    " ("

#ifndef NDEBUG
    "DEBUG-BUILD "
#endif
#ifndef NOMATH
    "math "
#endif
#ifndef NOTRACE
    "trace "
#endif
#ifndef NOWAITFOR
    "waitfor "
#endif
#ifndef NOSYSTEM
    "system "
#endif
#ifndef NOPIPES
    "pipes "
#endif
#ifndef NODIR
    "dir "
#endif
#ifndef NODLOAD
    "dload "
#endif
#ifndef NOSTARTUPFILE
    "startupfile "
#endif
#ifndef NODEBUGGING
    "debugging "
#endif
#ifndef NOSIGNALS
    "signals "
#endif

    ")";

#else /* __STDC__ */

char ici_version_string[] = "@(#)ICI 4.1.0";

#endif
