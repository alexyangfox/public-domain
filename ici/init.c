#define ICI_CORE
#include "func.h"
#include "buf.h"
#include "struct.h"
#include "exec.h"
#include "str.h"
#include "pcre/pcre.h"

extern ici_cfunc_t  *funcs[];

/*
 * Perform basic interpreter setup. Return non-zero on failure, usual
 * conventions.
 *
 * After calling this the scope stack has a struct for autos on it, and
 * the super of that is for statics. That struct for statics is where
 * global definitions that are likely to be visible to all code tend
 * to get set. All the intrinsic functions for example. It forms the
 * extern scope of any files parsed at the top level.
 *
 * In systems supporting threads, on exit, the global ICI mutex has been
 * acquired (with ici_enter()). 
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_init(void)
{
    ici_cfunc_t         **cfp;
    ici_struct_t        *scope;
    ici_objwsup_t       *externs;
    ici_exec_t          *x;
    int                 i;
    double              pi = 3.1415926535897932;

    /*
     * Just make sure our universal headers are really the size we
     * hope they are. Nothing actually assumes this. But it would
     * represent a significant inefficiency if they were padded.
     */
    assert(sizeof(ici_obj_t) == 4);
    assert(offsetof(ici_objwsup_t, o_super) == 4);

#   ifndef NDEBUG
    {
        char            v[80];
        /*
         * Check that the #defines of version number are in sync with our version
         * string from conf.c.
         */
        sprintf(v, "@(#)ICI %d.%d.%d", ICI_VER_MAJOR, ICI_VER_MINOR, ICI_VER_RELEASE);
        assert(strncmp(v, ici_version_string, strlen(v)) == 0);
    }
#   endif


    if (ici_chkbuf(120))
        return 1;
    if ((atoms = (ici_obj_t **)ici_nalloc(64 * sizeof(ici_obj_t *))) == NULL)
        return 1;
    atomsz = 64; /* Must be power of two. */
    memset((char *)atoms, 0, atomsz * sizeof(ici_obj_t *));
    if ((objs = (ici_obj_t **)ici_nalloc(256 * sizeof(ici_obj_t *))) == NULL)
        return 1;
    memset((char *)objs, 0, 256 * sizeof(ici_obj_t *));
    objs_limit = objs + 256;
    objs_top = objs;
    for (i = 0; i < nels(ici_small_ints); ++i)
    {
        if ((ici_small_ints[i] = ici_int_new(i)) == NULL)
            return -1;
    }
    ici_zero = ici_small_ints[0];
    ici_one = ici_small_ints[1];
    if (ici_init_sstrings())
        return 1;
    pcre_free = ici_free;
    pcre_malloc = (void *(*)(size_t))ici_alloc;
    if (ici_init_thread_stuff())
        return 1;
    if ((scope = ici_struct_new()) == NULL)
        return 1;
    if ((scope->o_head.o_super = externs = objwsupof(ici_struct_new())) == NULL)
        return 1;
    ici_decref(externs);
    if ((x = ici_new_exec()) == NULL)
        return 1;
    ici_enter(x);
    ici_rego(&ici_os);
    ici_rego(&ici_xs);
    ici_rego(&ici_vs);
    if (ici_engine_stack_check())
        return 1;
    *ici_vs.a_top++ = objof(scope);
    ici_decref(scope);
    if (ici_init_path(externs))
        return 1;
    for (cfp = funcs; *cfp != NULL; ++cfp)
    {
        if (ici_assign_cfuncs(scope->o_head.o_super, *cfp))
            return 1;
    }

#ifndef NOSIGNALS
    ici_signals_init();
#endif

    if
    (
        ici_set_val(externs, SS(_stdin),  'u', stdin)
        ||
        ici_set_val(externs, SS(_stdout), 'u', stdout)
        ||
        ici_set_val(externs, SS(_stderr), 'u', stderr)
        ||
        ici_set_val(externs, SS(pi), 'f', &pi)
    )
        return 1;

#ifndef NOSTARTUPFILE
    if (ici_call(SS(load), "o", SSO(core)))
        return 1;
#endif

    return 0;
}

/*
 * Check that the seperately compiled module that calls this function has been
 * compiled against a compatible versions of the ICI core that is now trying
 * to load it. An external module should call this like:
 *
 *     if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "myname"))
 *         return NULL;
 *
 * As soon as it can on load.  ICI_VER and ICI_BACK_COMPAT_VER come from ici.h
 * at the time that module was compiled.  This functions compares the values
 * passed from the external modules with the values the core was compiled
 * with, and fails (usual conventions) if there is any incompatibility.
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_interface_check(unsigned long mver, unsigned long bver, char const *name)
{
    if (ICI_VER < mver)
    {
        /*
         * Ooh, I'm an old ICI being called by a newer module. Does the module
         * think I'm recent enought?
         */
        if (ICI_VER >= bver)
            return 0;
    }
    else
    {
        /*
         * I'm a relatively up-to-date ICI, but is that module new enough
         * for me?
         */
        if (mver >= ICI_BACK_COMPAT_VER)
            return 0;
    }
    sprintf(ici_buf,
        "%s module was built for ICI %d.%d.%d, which is incompatible with this version %d.%d.%d",
        name,
        (int)(mver >> 24),
        (int)(mver >> 16) & 0xFF,
        (int)(mver & 0xFFFF),
        ICI_VER_MAJOR,
        ICI_VER_MINOR,
        ICI_VER_RELEASE);
    ici_error = ici_buf;
    return 1;

}
