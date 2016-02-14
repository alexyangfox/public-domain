#define ICI_CORE
#include "ptr.h"
#include "exec.h"
#include "file.h"
#include "str.h"
#include "struct.h"
#include "buf.h"
#include "wrap.h"
#include "func.h"
#ifndef NOPROFILE
#include "profile.h"
#endif

/*
 * An optional main entry point to the ICI interpreter.  'ici_main' handles a
 * complete interpreter life-cycle based on the given arguments.  A command
 * line ICI interpreter is expected to simply pass its given 'argc' and 'argv'
 * on to 'ici_main' then return its return value.
 *
 * If ici_main2 fails (that is, returns non-zero) it will also set ici_error in
 * the usual ICI manner.  However it will have already printed an error
 * message on standard error, so no further action need be taken.
 *
 * 'ici_main' handles all calls to 'ici_init()' and 'ici_uninit()' within its
 * scope.  A program calling 'ici_main' should *not* call 'ici_init()'.
 *
 * 'argc' and 'argv' are as standard for C 'main' functions.  For details on
 * the interpretation of the arguments, see documentation on normal command
 * line invocation of the ICI interpreter.
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_main(int argc, char *argv[])
{
    int                 i;
    int                 j;
    char                *s;
    char                *fmt;
    char                *arg0;
    ici_array_t         *av;
    FILE                *stream;
    ici_file_t          *f;
    int                 help = 0;

#   ifndef NDEBUG
        /*
         * There is an alarming tendancy to forget the NDEBUG define for
         * release builds of ICI.  This printf is an attempt to stop it.
         * Debug builds are *VERY* inefficient due to some very expensive
         * asserts in the main interpreter loop.
         */
        fprintf(stderr, "%s: Warning - this is a debug build.\n", argv[0]);
#   endif

    if (ici_init())
        goto fail;

    /*
     * Process arguments.  Two pass, first gather "unused" arguments,
     * ie, arguments which are passed into the ICI code.  Stash these in
     * the array av.  NB: must be in sync with the second pass below.
     */
    if ((av = ici_array_new(1)) == NULL)
        goto fail;
    *av->a_top++ = objof(&o_null); /* Leave room for argv[0]. */
    arg0 = NULL;
    if (argc <= 1)
        goto usage;
    if
    (
        argc > 1
        &&
        argv[1][0] != '-'
#       ifdef WIN32
            && argv[1][0] != '/'
#       endif
    )
    {
        /*
         * Usage1: ici file [args...]
         */
        arg0 = argv[1];
        for (i = 2; i < argc; ++i)
        {
            if (ici_stk_push_chk(av, 1))
                goto fail;
            if ((*av->a_top = objof(ici_str_get_nul_term(argv[i]))) == NULL)
                goto fail;
            ++av->a_top;
        }
    }
    else
    {
        /*
         * Usage2: ici [-f file] [-p prog] etc... [--] [args...]
         */
        for (i = 1; i < argc; ++i)
        {
            if
            (
                argv[i][0] == '-'
#               ifdef WIN32
                    || argv[i][0] == '/'
#               endif
            )
            {
                for (j = 1; argv[i][j] != '\0'; ++j)
                {
                    switch (argv[i][j])
                    {
                    case 'v':
                        fprintf(stderr, "%s\n", ici_version_string);
                        return 0;

                    case 'm':
                        if (argv[i][++j] != '\0')
                            s = &argv[i][j];
                        else if (++i >= argc)
                            goto usage;
                        else
                            s = argv[i];
                        if ((av->a_base[0] = objof(ici_str_get_nul_term(s))) == NULL)
                            goto fail;
                        break;

                    case '-':
                        while (++i < argc)
                        {
                            if (ici_stk_push_chk(av, 1))
                                goto fail;
                            if ((*av->a_top = objof(ici_str_get_nul_term(argv[i])))==NULL)
                                goto fail;
                            ++av->a_top;
                        }
                        break;

                    case 'f':
                    case 'l':
                    case 'e':
                        if (argv[i][++j] != '\0')
                            arg0 = &argv[i][j];
                        else if (++i >= argc)
                            goto usage;
                        else
                            arg0 = argv[i];
                        break;

                    case '0': case '1': case '2': case '3': case '4':
                    case '5': case '6': case '7': case '8': case '9':
                        continue;

                    case 'h':
                    case '?':
                        help = 1;
                    default:
                        goto usage;
                    }
                    break;
                }
            }
            else
            {
                if (ici_stk_push_chk(av, 1))
                    goto fail;
                if ((*av->a_top = objof(ici_str_get_nul_term(argv[i]))) == NULL)
                    goto fail;
                ++av->a_top;
            }
        }
    }
    if (av->a_base[0] == objof(&o_null))
    {
        if (arg0 == NULL)
            arg0 = argv[0];
        if ((av->a_base[0] = objof(ici_str_get_nul_term(arg0))) == NULL)
            goto fail;
    }
    else
        arg0 = stringof(av->a_base[0])->s_chars;
    if (av->a_top - av->a_base == 2 && stringof(av->a_base[1])->s_nchars == 0)
        --av->a_top; /* Patch around Bourne shell "$@" with no args bug. */

    {
        long            l;

        l = av->a_top - av->a_base;
        if
        (
            ici_set_val(objwsupof(ici_vs.a_top[-1])->o_super, SS(argv), 'o', objof(av))
            ||
            ici_set_val(objwsupof(ici_vs.a_top[-1])->o_super, SS(argc), 'i', &l)
        )
            goto fail;
        ici_decref(av);
     }

    /*
     * Pass two over the arguments; actually parse the modules.
     */
    if
    (
        argc > 1
        &&
        argv[1][0] != '-'
#       ifdef WIN32
            && argv[1][0] != '/'
#       endif
    )
    {
        if ((stream = fopen(argv[1], "r")) == NULL)
        {
            sprintf(buf, "%s: Could not open %s.", argv[0], argv[1]);
            ici_error = buf;
            goto fail;
        }
        if (ici_parse_file(argv[1], (char *)stream, &ici_stdio_ftype))
            goto fail;
    }
    else
    {
        for (i = 1; i < argc; ++i)
        {
            if
            (
                argv[i][0] != '-'
#               ifdef WIN32
                    && argv[i][0] != '/'
#               endif
            )
                continue;
            if (argv[i][1] == '\0')
            {
                if (ici_parse_file("stdin", (char *)stdin, &ici_stdio_ftype))
                    goto fail;
                continue;
            }
            for (j = 1; argv[i][j] != '\0'; ++j)
            {
                switch (argv[i][j])
                {
                case '-':
                    i = argc;
                    break;

                case 'e':
                    if (argv[i][++j] != '\0')
                        s = &argv[i][j];
                    else if (++i >= argc)
                        goto usage;
                    else
                        s = argv[i];
                    if ((f = ici_sopen(s, strlen(s), NULL)) == NULL)
                        goto fail;
                    f->f_name = SS(empty_string);
                    if (ici_parse(f, objwsupof(ici_vs.a_top[-1])) < 0)
                        goto fail;
                    ici_decref(f);
                    break;

                case 'l':
                    if (argv[i][++j] != '\0')
                        s = &argv[i][j];
                    else if (++i >= argc)
                        goto usage;
                    else
                        s = argv[i];
                    if (ici_call(SS(load), "s", s))
                        goto fail;
                    break;

                case 'f':
                    fmt = "%s";
                    if (argv[i][++j] != '\0')
                        s = &argv[i][j];
                    else if (++i >= argc)
                        goto usage;
                    else
                        s = argv[i];
                    if (ici_chkbuf(strlen(s) + strlen(fmt)))
                        goto fail;
                    sprintf(buf, fmt, s);
                    if ((stream = fopen(buf, "r")) == NULL)
                    {
                        sprintf(buf, "%s: Could not open %s.", argv[0], s);
                        ici_error = buf;
                        goto fail;
                    }
                    if (ici_parse_file(buf, (char *)stream, &ici_stdio_ftype))
                        goto fail;
                    break;

                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                    if ((stream = fdopen(argv[i][j] - '0', "r")) == NULL)
                    {
                        sprintf(buf, "%s: Could not access file descriptor %d.",
                            argv[0], argv[i][j] - '0');
                        ici_error = buf;
                        goto fail;
                    }
                    if (ici_parse_file(arg0, (char *)stream, &ici_stdio_ftype))
                        goto fail;
                    continue;
                }
                break;
            }
        }
    }

#ifndef NOPROFILE
    /* Make sure any profiling that started while parsing has finished. */
    if (ici_profile_active)
        ici_profile_return();
#endif

    ici_uninit();
    return 0;

usage:
    fprintf(stderr, "usage1: %s file args...\n", argv[0]);
    fprintf(stderr, "usage2: %s [-f file] [-] [-e prog] [-#] [-l mod] [-m name] [--] args...\n", argv[0]);
    fprintf(stderr, "usage3: %s [-h | -? | -v]\n", argv[0]);
    if (!help)
    {
        fprintf(stderr, "The -h switch will show details.\n");
    }
    else
    {
        fprintf(stderr, "\n");
        fprintf(stderr, "usage1:\n");
        fprintf(stderr, " Makes the ICI argv from file and args, then parses (i.e. executes) the file.\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "usage2:\n");
        fprintf(stderr, " Makes the ICI argv from the otherwise unused arguments, then processes the\n");
        fprintf(stderr, " following options in order. Repeats are allowed.\n");
        fprintf(stderr, " -f file  Parses the ICI code in file.\n");
        fprintf(stderr, " -        Parses ICI code from standard input.\n");
        fprintf(stderr, " -e prog  Parses the text 'prog' directly.\n");
        fprintf(stderr, " -#       Parses from the file descriptor #.\n");
        fprintf(stderr, " -l mod   Loads the module 'mod' as if by load().\n");
        fprintf(stderr, " -m name  Sets the ICI argv[0] to name.\n");
        fprintf(stderr, " --       Place following arguments in the ICI argv without interpretation.\n");
        fprintf(stderr, " args...  Otherwise unused arguments are placed in the ICI argv.\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "usage3:\n");
        fprintf(stderr, " -h | -?  Prints this help message then exits.\n");
        fprintf(stderr, " -v       Prints the version string then exits.\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "See 'The ICI Programming Language' (ici.pdf from ici.sf.net).\n");
    }
    ici_uninit();
    ici_error = "invalid command line arguments";
    return !help;

fail:
    fflush(stdout);
    fprintf(stderr, "%s\n", ici_error);
    ici_uninit();
    return 1;
}
