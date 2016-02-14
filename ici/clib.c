#define ICI_CORE
#include "file.h"
#include "exec.h"
#include "func.h"
#include "op.h"
#include "int.h"
#include "float.h"
#include "str.h"
#include "buf.h"
#include "null.h"
#include "re.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#if defined(sun) || defined(BSD4_4) || defined(linux)
#include <sys/wait.h>
#endif
#ifdef NeXT
#include <libc.h>
#endif

#ifndef NODIR
#ifndef _WIN32
#include <sys/param.h>
#include <dirent.h>
#endif   /* _WIN32 */
#endif   /* NODIR */

#if defined(linux) && !defined(MAXPATHLEN)
#include <sys/param.h>
#endif

#ifdef _WIN32
#include <io.h>
#include <process.h>
#include <direct.h>
#endif


/*
 * C library and others.  We don't want to go overboard here.  Would
 * like to keep the executable's size under control.
 */

#ifndef _WIN32
extern int      fgetc();
extern int      fputc();
extern int      fflush();
extern int      ungetc();
extern int      fclose();
extern int      pclose();
#endif
extern int      f_sprintf();
static long     xfseek(); /* Below. */
static int      xfeof(); /* Below. */
static int      xfwrite();
#if !defined(NOSYSTEM) && !defined(_WIN32)
extern int      system();
#endif

ici_ftype_t ici_stdio_ftype =
{
    fgetc,
    ungetc,
    fputc,
    fflush,
    fclose,
    xfseek,
    xfeof,
    xfwrite
};

#ifndef NOPIPES
static int
xpclose(FILE *f)
{
    int rc = pclose(f);

    if (rc != 0)
    {
        sprintf(buf, "popen command exit status %d", rc);
        ici_error = buf;
    }
    return rc;
}

ici_ftype_t  ici_popen_ftype =
{
    fgetc,
    ungetc,
    fputc,
    fflush,
    xpclose,
    xfseek,
    xfeof,
    xfwrite
};
#endif

static int
f_getchar()
{
    ici_file_t          *f;
    int                 c;
    ici_exec_t          *x;

    if (NARGS() != 0)
    {
        if (ici_typecheck("u", &f))
            return 1;
    }
    else
    {
        if ((f = ici_need_stdin()) == NULL)
            return 1;
    }
    ici_signals_blocking_syscall(1);
    x = ici_leave();
    c = (*f->f_type->ft_getch)(f->f_file);
    ici_enter(x);
    ici_signals_blocking_syscall(0);
    if (c == EOF)
    {
        if ((FILE *)f->f_file == stdin)
            clearerr(stdin);
        return ici_null_ret();
    }
    buf[0] = c;
    return ici_ret_with_decref(objof(ici_str_new(buf, 1)));
}

static int
f_getline()
{
    register int        i;
    register int        c;
    register char       *file;
    ici_file_t          *f;
    int                 (*get)();
    ici_exec_t          *x;
    char                *b;
    int                 buf_size;
    ici_str_t           *str;

    x = NULL;
    if (NARGS() != 0)
    {
        if (ici_typecheck("u", &f))
            return 1;
    }
    else
    {
        if ((f = ici_need_stdin()) == NULL)
            return 1;
    }
    get = f->f_type->ft_getch;
    file = f->f_file;
    if ((b = malloc(buf_size = 128)) == NULL)
        goto nomem;
    ici_signals_blocking_syscall(1);
    x = ici_leave();
    for (i = 0; (c = (*get)(file)) != '\n' && c != EOF; ++i)
    {
        if (i == buf_size && (b = realloc(b, buf_size *= 2)) == NULL)
            break;
        b[i] = c;
    }
    ici_enter(x);
    ici_signals_blocking_syscall(0);
    if (b == NULL)
        goto nomem;
    if (i == 0 && c == EOF)
    {
        free(b);
        if ((FILE *)f->f_file == stdin)
            clearerr(stdin);
        return ici_null_ret();
    }
    str = ici_str_new(b, i);
    free(b);
    if (str == NULL)
        return 1;
    return ici_ret_with_decref(objof(str));

nomem:
    ici_error = "ran out of memory";
    return 1;
}

static int
f_getfile()
{
    register int        i;
    register int        c;
    ici_file_t          *f;
    int                 (*get)();
    char                *file;
    ici_exec_t          *x;
    char                *b;
    int                 buf_size;
    ici_str_t           *str;
    int                 must_close;
    
    must_close = 0;
    str = NULL; /* Pessimistic. */
    if (NARGS() != 0)
    {
        if (isstring(ARG(0)))
        {
            if (ici_call(SS(fopen), "o=o", &f, ARG(0)))
                goto finish;
            must_close = 1;
        }
        else
            f = fileof(ARG(0));
        if (!isfile(f))
        {
            char    n1[ICI_OBJNAMEZ];

            sprintf(ici_buf, "getfile() given %s instead of a file",
                ici_objname(n1, objof(f)));
            ici_error = ici_buf;
            goto finish;
        }
    }
    else
    {
        if ((f = ici_need_stdin()) == NULL)
            goto finish;
    }
    get = f->f_type->ft_getch;
    file = f->f_file;
    if ((b = malloc(buf_size = 128)) == NULL)
        goto nomem;
    ici_signals_blocking_syscall(1);
    x = ici_leave();
    for (i = 0; (c = (*get)(file)) != EOF; ++i)
    {
        if (i == buf_size && (b = realloc(b, buf_size *= 2)) == NULL)
            break;
        b[i] = c;
    }
    ici_enter(x);
    ici_signals_blocking_syscall(0);
    if (b == NULL)
        goto nomem;
    str = ici_str_new(b, i);
    free(b);
    goto finish;

nomem:
    ici_error = "ran out of memory";
finish:
    if (must_close)
    {
        ici_call(SS(close), "o", f);
        ici_decref(f);
    }
    return ici_ret_with_decref(objof(str));
}

static int
f_tmpname()
{
    return ici_str_ret(tmpnam(NULL));
}

static int
f_put()
{
    ici_str_t  *s;
    ici_file_t *f;
    ici_exec_t *x;

    if (NARGS() > 1)
    {
        if (ici_typecheck("ou", &s, &f))
            return 1;
    }
    else
    {
        if (ici_typecheck("o", &s))
            return 1;
        if ((f = ici_need_stdout()) == NULL)
            return 1;
    }
    if (!isstring(objof(s)))
        return ici_argerror(0);
    x = ici_leave();
    if
    (
        (*f->f_type->ft_write)(s->s_chars, s->s_nchars, f->f_file)
        !=
        s->s_nchars
    )
    {
        ici_enter(x);
        ici_error = "write failed";
        return 1;
    }
    ici_enter(x);
    return ici_null_ret();
}

static int
f_fflush()
{
    ici_file_t          *f;
    ici_exec_t          *x;

    if (NARGS() > 0)
    {
        if (ici_typecheck("u", &f))
            return 1;
    }
    else
    {
        if ((f = ici_need_stdout()) == NULL)
            return 1;
    }
    x = ici_leave();
    if ((*f->f_type->ft_flush)(f->f_file) == -1)
    {
        ici_enter(x);
        ici_error = "flush failed";
        return 1;
    }
    ici_enter(x);
    return ici_null_ret();
}

static long
xfseek(FILE *stream, long offset, int whence)
{
    if (fseek(stream, offset, whence) == -1)
    {
        ici_error = "seek failed";
        return -1;
    }
    return ftell(stream);
}

static int
xfeof(FILE *stream)
{
    return feof(stream);
}

static int
xfwrite(char *s, long n, FILE *stream)
{
    return fwrite(s, 1, (size_t)n, stream);
}

static int
f_fopen()
{
    char        *name;
    char        *mode;
    ici_file_t  *f;
    FILE        *stream;
    ici_exec_t  *x;
    int         i;

    mode = "r";
    if (ici_typecheck(NARGS() > 1 ? "ss" : "s", &name, &mode))
        return 1;
    x = ici_leave();
    if ((stream = fopen(name, mode)) == NULL)
    {
        i = errno;
        ici_enter(x);
        errno = i;
        return ici_get_last_errno("open", name);
    }
    ici_enter(x);
    if ((f = ici_file_new((char *)stream, &ici_stdio_ftype, stringof(ARG(0)), NULL)) == NULL)
    {
        fclose(stream);
        return 1;
    }
    return ici_ret_with_decref(objof(f));
}

static int
f_fseek()
{
    ici_file_t  *f;
    long        offset;
    long        whence;

    if (ici_typecheck("uii", &f, &offset, &whence))
        return 1;
    switch (whence)
    {
    case 0:
    case 1:
    case 2:
        break;
    default:
        ici_error = "invalid whence value in seek()";
        return 1;
    }
    if ((offset = (*f->f_type->ft_seek)(f->f_file, offset, (int)whence)) == -1)
        return 1;
    return ici_int_ret(offset);
}

#ifndef NOPIPES
static int
f_popen()
{
    char        *name;
    char        *mode;
    ici_file_t  *f;
    FILE        *stream;
    ici_exec_t  *x;
    int         i;
    extern int  pclose();

    mode = "r";
    if (ici_typecheck(NARGS() > 1 ? "ss" : "s", &name, &mode))
        return 1;
    x = ici_leave();
    if ((stream = popen(name, mode)) == NULL)
    {
        i = errno;
        ici_enter(x);
        errno = i;
        return ici_get_last_errno("popen", name);
    }
    ici_enter(x);
    if ((f = ici_file_new((char *)stream, &ici_popen_ftype, stringof(ARG(0)), NULL)) == NULL)
    {
        pclose(stream);
        return 1;
    }
    return ici_ret_with_decref(objof(f));
}
#endif

#ifndef NOSYSTEM
static int
f_system()
{
    char        *cmd;
    long        result;
    ici_exec_t  *x;

    if (ici_typecheck("s", &cmd))
        return 1;
    x = ici_leave();
    result = system(cmd);
    ici_enter(x);
    return ici_int_ret(result);
}
#endif

static int
f_fclose()
{
    ici_obj_t   *o;
    ici_exec_t  *x;
    int         r;

    if (NARGS() != 1)
        return ici_argcount(1);
    o = ARG(0);
    if (!isfile(o))
        return ici_argerror(0);
    x = ici_leave();
    r = ici_file_close(fileof(o));
    ici_enter(x);
    if (r)
        return 1;
    return ici_null_ret();
}

static int
f_eof()
{
    ici_file_t          *f;
    ici_exec_t          *x;
    int                 r;

    if (NARGS() != 0)
    {
        if (ici_typecheck("u", &f))
            return 1;
    }
    else
    {
        if ((f = ici_need_stdin()) == NULL)
            return 1;
    }
    x = ici_leave();
    r = (*f->f_type->ft_eof)(f->f_file);
    ici_enter(x);
    return ici_int_ret((long)r);
}

static int
f_remove(void)
{
    char        *s;

    if (ici_typecheck("s", &s))
        return 1;
    if (remove(s) != 0)
        return ici_get_last_errno("remove", s);
    return ici_null_ret();
}

#ifndef NODIR

#ifdef _WIN32
/*
 * Emulate opendir/readdir/et al under WIN32 environments via findfirst/
 * findnext. Only what f_dir() needs has been emulated (which is to say,
 * not much).
 */

#define MAXPATHLEN      _MAX_PATH

struct dirent
{
    char        *d_name;
};

typedef struct DIR
{
    long                handle;
    struct _finddata_t  finddata;
    int                 needfindnext;
    struct dirent       dirent;
}
DIR;

static DIR *
opendir(const char *path)
{
    DIR         *dir;
    char        fspec[_MAX_PATH+1];

    if (strlen(path) > (_MAX_PATH - 4))
        return NULL;
    sprintf(fspec, "%s/*.*", path);
    if ((dir = ici_talloc(DIR)) != NULL)
    {
        if ((dir->handle = _findfirst(fspec, &dir->finddata)) == -1)
        {
            ici_tfree(dir, DIR);
            return NULL;
        }
        dir->needfindnext = 0;
    }
    return dir;
}

static struct dirent *
readdir(DIR *dir)
{
    if (dir->needfindnext && _findnext(dir->handle, &dir->finddata) != 0)
            return NULL;
    dir->dirent.d_name = dir->finddata.name;
    dir->needfindnext = 1;
    return &dir->dirent;
}

static void
closedir(DIR *dir)
{
    _findclose(dir->handle);
    ici_tfree(dir, DIR);
}

#define S_ISREG(m)      (((m) & _S_IFMT) == _S_IFREG)
#define S_ISDIR(m)      (((m) & _S_IFMT) == _S_IFDIR)

#endif

/*
 * array = dir([path] [, regexp] [, format])
 *
 * Read directory named in path (a string, defaulting to ".", the current
 * working directory) and return the entries that match the pattern (or
 * all names if no pattern passed). The format string identifies what
 * sort of entries should be returned. If the format string is passed
 * then a path MUST be passed (to avoid any ambiguity) but path may be
 * NULL meaning the current working directory (same as "."). The format
 * string uses the following characters,
 *
 *      f       return file names
 *      d       return directory names
 *      a       return all names (which includes things other than
 *              files and directories, e.g., hidden or special files
 *
 * The default format specifier is "f".
 */
static int
f_dir(void)
{
    char                *path   = ".";
    char                *format = "f";
    ici_regexp_t        *regexp = NULL;
    ici_obj_t           *o;
    ici_array_t         *a;
    DIR                 *dir;
    struct dirent       *dirent;
    int                 fmt;
    ici_str_t           *s;

    switch (NARGS())
    {
    case 0:
        break;

    case 1:
        o = ARG(0);
        if (isstring(o))
            path = stringof(o)->s_chars;
        else if (isnull(o))
            ;   /* leave path as is */
        else if (isregexp(o))
            regexp = regexpof(o);
        else
            return ici_argerror(0);
        break;

    case 2:
        o = ARG(0);
        if (isstring(o))
            path = stringof(o)->s_chars;
        else if (isnull(o))
            ;   /* leave path as is */
        else if (isregexp(o))
            regexp = regexpof(o);
        else
            return ici_argerror(0);
        o = ARG(1);
        if (isregexp(o))
        {
            if (regexp != NULL)
                return ici_argerror(1);
            regexp = regexpof(o);
        }
        else if (isstring(o))
            format = stringof(o)->s_chars;
        else
            return ici_argerror(1);
        break;

    case 3:
        o = ARG(0);
        if (isstring(o))
            path = stringof(o)->s_chars;
        else if (isnull(o))
            ;   /* leave path as is */
        else
            return ici_argerror(0);
        o = ARG(1);
        if (!isregexp(o))
            return ici_argerror(1);
        regexp = regexpof(o);
        o = ARG(2);
        if (!isstring(o))
            return ici_argerror(2);
        format = stringof(o)->s_chars;
        break;

    default:
        return ici_argcount(3);
    }

    if (*path == '\0')
        path = ".";

#define FILES   1
#define DIRS    2
#define OTHERS  4

    for (fmt = 0; *format != '\0'; ++format)
    {
        switch (*format)
        {
        case 'f':
            fmt |= FILES;
            break;

        case 'd':
            fmt |= DIRS;
            break;

        case 'a':
            fmt |= OTHERS | DIRS | FILES;
            break;

        default:
            ici_error = "bad directory format specifier";
            return 1;
        }
    }
    if ((a = ici_array_new(0)) == NULL)
        return 1;
    if ((dir = opendir(path)) == NULL)
    {
        ici_get_last_errno("open directory", path);
        goto fail;
    }
    while ((dirent = readdir(dir)) != NULL)
    {
        struct stat     statbuf;
        char            abspath[MAXPATHLEN+1];

        if
        (
            regexp != NULL
            &&
            pcre_exec
            (
                regexp->r_re,
                regexp->r_rex,
                dirent->d_name,
                strlen(dirent->d_name),
                0,
                0,
                re_bra,
                nels(re_bra)
            )
            < 0
        )
            continue;
        sprintf(abspath, "%s/%s", path, dirent->d_name);
#ifndef _WIN32
        if (lstat(abspath, &statbuf) == -1)
        {
            ici_get_last_errno("get stats on", abspath);
            closedir(dir);
            goto fail;
        }
        if (S_ISLNK(statbuf.st_mode) && stat(abspath, &statbuf) == -1)
            continue;
#else
        if (stat(abspath, &statbuf) == -1)
            continue;
#endif
        if
        (
            (S_ISREG(statbuf.st_mode) && fmt & FILES)
            ||
            (S_ISDIR(statbuf.st_mode) && fmt & DIRS)
            ||
            fmt & OTHERS
        )
        {
            if
            (
                (s = ici_str_new_nul_term(dirent->d_name)) == NULL
                ||
                ici_stk_push_chk(a, 1)
            )
            {
                if (s != NULL)
                    ici_decref(s);
                closedir(dir);
                goto fail;
            }
            *a->a_top++ = objof(s);
            ici_decref(s);
        }
    }
    closedir(dir);
    return ici_ret_with_decref(objof(a));

#undef  FILES
#undef  DIRS
#undef  OTHERS

fail:
    ici_decref(a);
    return 1;
}
#endif  /* NODIR */

/*
 * Used as a common error return for system calls that fail. Sets the
 * global error string if the call fails otherwise returns the integer
 * result of the system call.
 */
static int
sys_ret(int ret)
{
    if (ret < 0)
        return ici_get_last_errno(NULL, NULL);
    return ici_int_ret((long)ret);
}

/*
 * rename(oldpath, newpath)
 */
static int
f_rename()
{
    char                *o;
    char                *n;

    if (ici_typecheck("ss", &o, &n))
        return 1;
    return sys_ret(rename(o, n));
}

/*
 * chdir(newdir)
 */
static int
f_chdir()
{
    char                *n;

    if (ici_typecheck("s", &n))
        return 1;
    return sys_ret(chdir(n));
}

/*
 * string = getcwd()
 */
static int
f_getcwd(void)
{
    char        buf[MAXPATHLEN+1];

    if (getcwd(buf, sizeof buf) == NULL)
        return sys_ret(-1);
    return ici_str_ret(buf);
}

ici_cfunc_t clib_cfuncs[] =
{
    {CF_OBJ,    (char *)SS(printf),       f_sprintf,      (void *)1},
    {CF_OBJ,    (char *)SS(getchar),      f_getchar},
    {CF_OBJ,    (char *)SS(getfile),      f_getfile},
    {CF_OBJ,    (char *)SS(getline),      f_getline},
    {CF_OBJ,    (char *)SS(fopen),        f_fopen},
#ifndef NOPIPES
    {CF_OBJ,    (char *)SS(_popen),        f_popen},
#endif
    {CF_OBJ,    (char *)SS(tmpname),      f_tmpname},
    {CF_OBJ,    (char *)SS(put),          f_put},
    {CF_OBJ,    (char *)SS(flush),        f_fflush},
    {CF_OBJ,    (char *)SS(close),        f_fclose},
    {CF_OBJ,    (char *)SS(seek),         f_fseek},
#ifndef NOSYSTEM
    {CF_OBJ,    (char *)SS(system),       f_system},
#endif
    {CF_OBJ,    (char *)SS(eof),          f_eof},
    {CF_OBJ,    (char *)SS(remove),       f_remove},
#ifndef NODIR
    {CF_OBJ,    (char *)SS(dir),          f_dir},
#endif
    {CF_OBJ,    (char *)SS(getcwd),       f_getcwd},
    {CF_OBJ,    (char *)SS(chdir),        f_chdir},
    {CF_OBJ,    (char *)SS(rename),       f_rename},

    {CF_OBJ}
};
