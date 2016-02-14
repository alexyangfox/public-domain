#define ICI_CORE
#include "file.h"
#include "str.h"
#include "parse.h"
#include "primes.h"

/*
 * Returns 0 if these objects are eq, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
static int
cmp_file(ici_obj_t *o1, ici_obj_t *o2)
{
    return fileof(o1)->f_file != fileof(o2)->f_file
        || fileof(o1)->f_type != fileof(o2)->f_type;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
static void
free_file(ici_obj_t *o)
{
    if ((o->o_flags & F_CLOSED) == 0)
    {
        if (o->o_flags & F_NOCLOSE)
            (*fileof(o)->f_type->ft_flush)(fileof(o)->f_file);
        else
            ici_file_close(fileof(o));
    }
    ici_tfree(o, ici_file_t);
}

/*
 * Return the object at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 */
static ici_obj_t *
fetch_file(ici_obj_t *o, ici_obj_t *k)
{
    if (k == SSO(name) && fileof(o)->f_name != NULL)
        return objof(fileof(o)->f_name);
    if (fileof(o)->f_type == &ici_parse_ftype && k == SSO(line))
    {
        ici_int_t   *l;

        if ((l = ici_int_new(parseof(fileof(o)->f_file)->p_lineno)) != NULL)
            ici_decref(l);
        return objof(l);
    }
    return ici_fetch_fail(o, k);
}



/*
 * Return a file object with the given 'ftype' and a file type specific
 * pointer 'fp' which is often something like a 'STREAM *' or a file
 * descriptor.  The 'name' is mostly for error messages and stuff.  The
 * returned object has a ref count of 1.  Returns NULL on error.
 *
 * The 'ftype' is a pointer to a struct of stdio-like function pointers that
 * will be used to do I/O operations on the file (see 'ici_ftype_t').  The
 * given structure is assumed to exist as long as necessary.  (It is normally
 * a static srtucture, so this is not a problem.) The core-supplied struct
 * 'ici_stdio_ftype' can be used if 'fp' is a 'STREAM *'.
 *
 * The 'ref' argument is an object reference that the file object will keep in
 * case the 'fp' argument is an implicit reference into some object (for
 * example, this is used for reading an ICI string as a file).  It may be NULL
 * if not required.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_file_t *
ici_file_new(void *fp, ici_ftype_t *ftype, ici_str_t *name, ici_obj_t *ref)
{
    register ici_file_t *f;

    if ((f = ici_talloc(ici_file_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(f, TC_FILE, 0, 1, 0);
    f->f_file = fp;
    f->f_type = ftype;
    f->f_name = name;
    f->f_ref = ref;
    ici_rego(f);
    return f;
}

/*
 * Close the given ICI file 'f' by calling the lower-level close function
 * given in the 'ici_ftype_t' associated with the file.  A guard flag is
 * maintained in the file object to prevent multiple calls to the lower level
 * function (this is really so we can optionally close the file explicitly,
 * and let the garbage collector do it to).  Returns non-zero on error, usual
 * conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_file_close(ici_file_t *f)
{
    if (objof(f)->o_flags & F_CLOSED)
    {
        ici_error = "file already closed";
        return 1;
    }
    objof(f)->o_flags |= F_CLOSED;
    return (*f->f_type->ft_close)(f->f_file);
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_file(ici_obj_t *o)
{
    long        mem;

    o->o_flags |= O_MARK;
    mem = sizeof(ici_file_t);
    if (fileof(o)->f_name != NULL)
        mem += ici_mark(objof(fileof(o)->f_name));
    if (fileof(o)->f_ref != NULL)
        mem += ici_mark(objof(fileof(o)->f_ref));
    return mem;
}

ici_type_t  file_type =
{
    mark_file,
    free_file,
    ici_hash_unique,
    cmp_file,
    ici_copy_simple,
    ici_assign_fail,
    fetch_file,
    "file"
};
