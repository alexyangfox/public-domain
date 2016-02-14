#define ICI_CORE
#include "exec.h"
#include "str.h"
#include "struct.h"
#include "file.h"
#include "buf.h"
#include "func.h"

static int push_path_elements(ici_array_t *a, char *path); /* Forward. */

/*
 * If we're supporting loading of native code modules we need Unix-style
 * dlopen/dlsym/dlclose routines.  Pick a version according to the platform.
 */

# if defined(_WIN32)
#  include "load-w32.h"
#  include <io.h>
# elif defined(__BEOS__)
#  include "load-beos.h"
# else /* Unix */
#  include <sys/types.h>
#  include <dlfcn.h>
#  include <unistd.h>

#ifndef NODLOAD
typedef void    *dll_t;
#define valid_dll(dll)  ((dll) != NULL)
#endif /* NODLOAD */

# endif /* _WIN32, __BEOS__ */

#ifndef NODLOAD
# ifndef RTLD_NOW
#  define RTLD_NOW 1
# endif
# ifndef RTLD_GLOBAL
#  define RTLD_GLOBAL 0
# endif
#endif

static const char   ici_prefix[] = "ici4";
static const char   old_prefix[] = "ici3";

/*
 * Find and return the outer-most writeable struct in the current scope.
 * This is typically the struct that contains all the ICI core language
 * functions. This is the place we lodge new dynamically loaded modules
 * and is also the typical parent for top level classes made by dynamically
 * loaded modules. Usual error conventions.
 */
ici_objwsup_t *
ici_outermost_writeable_struct(void)
{
    ici_objwsup_t       *outer;
    ici_objwsup_t       *ows;

    outer = NULL;
    for (ows = objwsupof(ici_vs.a_top[-1]); ows != NULL; ows = ows->o_super)
    {
        if (objof(ows)->o_flags & O_ATOM)
            continue;
        if (!isstruct(ows))
            continue;
        outer = ows;
    }
    if (outer == NULL)
        ici_error = "no writeable structs in current scope";
    return outer;
}

/*
 * any = load(string)
 *
 * Function form of auto-loading. String is the name of a file to be
 * loaded stripped of its ``ici'' prefix and ``.ici'' suffix, i.e., it is
 * the same name that would be used in the ici source to refer to the
 * module. The result is the object that would have been assigned to that
 * name in the scope if the module had been auto-loaded, i.e., for an
 * ici module it is the reference to the extern scope of the module, for
 * a native code module it the object returned by that module's init.
 * function.
 */
static int
f_load(void)
{
    ici_str_t   *name;
    ici_obj_t   *result;
    char        fname[FILENAME_MAX];
    char        entry_symbol[64];
    ici_struct_t    *statics;
    ici_struct_t    *autos;
    ici_struct_t    *externs;
    ici_objwsup_t   *outer;
    ici_file_t  *file;
    FILE        *stream;

    externs = NULL;
    statics = NULL;
    autos = NULL;
    file = NULL;
    result = NULL;

    /*
     * This is here to stop a compiler warning, complaining that entry_symbol
     * is never used (but we know it is)
     */
    entry_symbol[0] = 0;

    if (ici_typecheck("o", &name))
        return 1;
    if (!isstring(objof(name)))
        return ici_argerror(0);
    /*
     * Find the outer-most writeable scope. This is where the new name
     * will be defined should it be loadable as a library.
     */
    if ((outer = ici_outermost_writeable_struct()) == NULL)
        return 0;

    /*
     * Simplify a lot of checks by simply ignoring huge module names.
     */
    if (name->s_nchars > sizeof entry_symbol - 20)
        return 0;

#ifndef NODLOAD
    /*
     * First of all we consider the case of a dynamically loaded native
     * machine code.
     */
    strcpy(fname, ici_prefix);
    strcat(fname, name->s_chars);
/*
#ifndef NDEBUG
    strcat(fname, "-d");
#endif
*/

    if (ici_find_on_path(fname, ICI_DLL_EXT))
    {
        dll_t           lib;
        ici_obj_t       *(*library_init)(void);
        ici_obj_t       *o;

        /*
         * We have a file .../iciX.EXT. Attempt to dynamically load it.
         */
        lib = dlopen(fname, RTLD_NOW|RTLD_GLOBAL);
        if (!valid_dll(lib))
        {
            char        *err;
            char const  fmt[] = "failed to load %s, %s";

            if ((err = (char *)dlerror()) == NULL)
                err = "dynamic link error";
            if (ici_chkbuf(strlen(fmt) + strlen(fname) + strlen(err) + 1))
                strcpy(buf, err);
            else
                sprintf(buf, fmt, fname, err);
            ici_error = buf;
            return -1;
        }
#ifdef NEED_UNDERSCORE_ON_SYMBOLS
        sprintf(entry_symbol, "_ici_%s_library_init", name->s_chars);
#else
        sprintf(entry_symbol, "ici_%s_library_init", name->s_chars);
#endif
        library_init = (ici_obj_t *(*)(void))dlsym(lib, entry_symbol);
        if (library_init == NULL)
        {
#ifndef SUNOS5 /* Doing the dlclose results in a crash under Solaris - why? */
            dlclose(lib);
#endif
            if (ici_chkbuf(strlen(entry_symbol) + strlen(fname)))
                ici_error = "failed to find library entry point";
            else
            {
                sprintf
                (
                    buf,
                    "failed to find the entry symbol %s in %s",
                    entry_symbol,
                    fname
                );
                ici_error = buf;
            }
            return -1;
        }
        if ((o = (*library_init)()) == NULL)
        {
            dlclose(lib);
            return -1;
        }
        result = o;
        if (ici_assign(outer, name, o))
            goto fail;
        if (!isstruct(o))
            return ici_ret_with_decref(o);
        externs = structof(o);
    }
#endif /* NODLOAD */

    strcpy(fname, ici_prefix);
    strcat(fname, name->s_chars);
    if (ici_find_on_path(fname, ".ici"))
        goto got_file;
    strcpy(fname, old_prefix);
    strcat(fname, name->s_chars);
    if (ici_find_on_path(fname, ".ici"))
    {
        ici_str_t       *fn;

    got_file:
        /*
         * We have a file .../iciX.ici, open the file, make new statics
         * and autos, assign the statics into the extern scope under
         * the given name, then parse the new module.
         */
        if ((stream = fopen(fname, "r")) == NULL)
            return ici_get_last_errno("open", fname);
        if ((fn = ici_str_new_nul_term(fname)) == NULL)
        {
            fclose(stream);
            goto fail;
        }
        file = ici_file_new((char *)stream, &ici_stdio_ftype, fn, NULL);
        ici_decref(fn);
        if (file == NULL)
        {
            fclose(stream);
            goto fail;
        }
        if ((autos = ici_struct_new()) == NULL)
            goto fail;
        if ((statics = ici_struct_new()) == NULL)
            goto fail;
        autos->o_head.o_super = objwsupof(statics);
        ici_decref(statics);
        if (externs == NULL)
        {
            if ((externs = ici_struct_new()) == NULL)
                goto fail;
            result = objof(externs);
            if (ici_assign(outer, name, externs))
                goto fail;
        }
        statics->o_head.o_super = objwsupof(externs);
        externs->o_head.o_super = outer;
        if (ici_parse(file, objwsupof(autos)) < 0)
            goto fail;
        ici_file_close(file);
        ici_decref(autos);
        ici_decref(file);
    }
    if (result == NULL)
    {
        if (ici_chkbuf(3 * name->s_nchars + 80))
            return 1;
        sprintf(buf, "\"%s\" undefined and could not find %s%s%s or %s%s.ici ",
            name->s_chars,
            ici_prefix,
            name->s_chars,
            ICI_DLL_EXT,
            ici_prefix,
            name->s_chars);
        ici_error = buf;
        goto fail;
    }
    return ici_ret_with_decref(result);

fail:
    if (file != NULL)
        ici_decref(file);
    if (autos != NULL)
        ici_decref(autos);
    if (result != NULL)
        ici_decref(result);
    return 1;
}

#if defined(__BEOS__)
/*
 * Get library path. Needed even without dynamic loading of binaries, to
 * load ICI files dynamically
 */
#include <FindDirectory.h>

/*
 * Push path elements specific to BeOS onto the array a (which is the ICI
 * path array used for finding dynamically loaded modules and stuff). These
 * are in addition to the ICIPATH environment variable.
 */
static int
push_os_path_elements(ici_array_t *a)
{
    char                path[FILENAME_MAX];

    if (find_directory(B_USER_ADDONS_DIRECTORY, 0, false, path, sizeof(path) - 6) == 0)
    {
        strcat(path, "/ici");
        if (push_path_elements(a, path))
            return 1;
    }
    if (find_directory(B_USER_LIB_DIRECTORY, 0, false, path, sizeof(path) - 6) == 0)
    {
        strcat(path, "/ici");
        if (push_path_elements(a, path))
            return 1;
    }
    return 0;
}
#elif defined(_WIN32)
/*
 * Push path elements specific to Windows onto the array a (which is the ICI
 * path array used for finding dynamically loaded modules and stuff). These
 * are in addition to the ICIPATH environment variable. We try to mimic
 * the search behaviour of LoadLibrary() (that being the Windows thing to
 * do).
 */
static int
push_os_path_elements(ici_array_t *a)
{
    char                fname[MAX_PATH];
    char                *p;

    if (GetModuleFileName(NULL, fname, sizeof fname - 10) > 0)
    {
        if ((p = strrchr(fname, ICI_DIR_SEP)) != NULL)
            *p = '\0';
        if (push_path_elements(a, fname))
            return 1;
        p = fname + strlen(fname);
        *p++ = ICI_DIR_SEP;
        strcpy(p, "ici");
        if (push_path_elements(a, fname))
            return 1;
    }
    if (push_path_elements(a, "."))
        return 1;
    if (GetSystemDirectory(fname, sizeof fname - 10) > 0)
    {
        if (push_path_elements(a, fname))
            return 1;
        p = fname + strlen(fname);
        *p++ = ICI_DIR_SEP;
        strcpy(p, "ici");
        if (push_path_elements(a, fname))
            return 1;
    }
    if (GetWindowsDirectory(fname, sizeof fname - 10) > 0)
    {
        if (push_path_elements(a, fname))
            return 1;
        p = fname + strlen(fname);
        *p++ = ICI_DIR_SEP;
        strcpy(p, "ici");
        if (push_path_elements(a, fname))
            return 1;
    }
    if ((p = getenv("PATH")) != NULL)
    {
        if (push_path_elements(a, p))
            return 1;
    }
    return 0;
}
#else
/*
 * Assume a UNIX-like sytem.
 *
 * Push path elements specific to UNIX-like systems onto the array a (which
 * is the ICI path array used for finding dynamically loaded modules and
 * stuff). These are in addition to the ICIPATH environment variable.
 */
static int
push_os_path_elements(ici_array_t *a)
{
    char                *p;
    char                *q;
    char                *path;
    char                fname[FILENAME_MAX];

    if (push_path_elements(a, "/usr/local/lib/ici4:/opt/lib/ici4:/sw/lib/ici4"))
        return 1;
    if ((path = getenv("PATH")) != NULL)
    {
        for (p = path; *p != '\0'; p = *q == '\0' ? q : q + 1)
        {
            if ((q = strchr(p, ':')) == NULL)
                q = p + strlen(p);
            if (q - 4 < p || memcmp(q - 4, "/bin", 4) != 0 || q - p >= FILENAME_MAX - 10)
                continue;
            memcpy(fname, p, (q - p) - 4);
            strcpy(fname + (q - p) - 4, "/lib/ici4");
            if (push_path_elements(a, fname))
                return 1;
        }
    }
#   ifdef ICI_CONFIG_PREFIX
        /*
         * Put a configuration defined location on, if there is one..
         */
        if (push_path_elements(a, ICI_CONFIG_PREFIX "/lib/ici4"))
            return 1;
#   endif
    return 0;
}
#endif /* End of selection of which push_os_path_elements() to use */

/*
 * Push one or more file names from path, seperated by the local system
 * seperator character (eg. : or ;), onto a. Usual error conventions.
 */
static int
push_path_elements(ici_array_t *a, char *path)
{
    char                *p;
    char                *q;
    ici_str_t           *s;
    ici_obj_t           **e;

    for (p = path; *p != '\0'; p = *q == '\0' ? q : q + 1)
    {
        if ((q = strchr(p, ICI_PATH_SEP)) == NULL)
            q = p + strlen(p);
        if ((s = ici_str_new(p, q - p)) == NULL)
            return 1;
        /*
         * Don't add duplicates...
         */
        for (e = a->a_base; e < a->a_top; ++e)
        {
            if (*e == objof(s))
                goto skip;
        }
        /*
         * Don't add inaccessable dirs...
         */
        if (access(s->s_chars, 0) != 0)
            goto skip;
        if (ici_stk_push_chk(a, 1))
        {
            ici_decref(s);
            return 1;
        }
        *a->a_top++ = objof(s);
    skip:
        ici_decref(s);
    }
    return 0;
}

/*
 * Set the path variable in externs to be an array of all the directories
 * that should be searched in for ICI extension modules and stuff.
 */
int
ici_init_path(ici_objwsup_t *externs)
{
    ici_array_t         *a;
    int                 r;
    char                *path;

    if ((a = ici_array_new(0)) == NULL)
        return 1;
    r = assign_base(externs, SSO(path), a);
    ici_decref(a);
    if (r)
        return 1;
    if ((path = getenv("ICIPATH")) != NULL)
    {
        if (push_path_elements(a, path))
            return 1;
    }
    push_os_path_elements(a);
    return 0;
}

ici_cfunc_t load_cfuncs[] =
{
    {CF_OBJ, (char *)SS(load), f_load},
    {CF_OBJ}
};
