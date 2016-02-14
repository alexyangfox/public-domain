#define ICI_CORE
#include "file.h"

typedef struct sfile
{
    char        *sf_data;
    char        *sf_ptr;
    int         sf_size;
    int         sf_eof;
    int         sf_foreign_data;
}
    sfile_t;

static int
sgetc(sfile_t *sf)
{
    if (sf->sf_ptr >= sf->sf_data && sf->sf_ptr < sf->sf_data + sf->sf_size)
    {
        sf->sf_eof = 0;
        return *sf->sf_ptr++ & 0xFF;
    }
    sf->sf_eof = 1;
    return EOF;
}

static int
sungetc(int c, sfile_t *sf)
{
    if (c != EOF && sf->sf_ptr >= sf->sf_data && sf->sf_ptr > sf->sf_data)
    {
        *--sf->sf_ptr = c;
        sf->sf_eof = 0;
    }
    return c;
}

static int
sclose(sfile_t *sf)
{
    if (!sf->sf_foreign_data)
        ici_free(sf->sf_data);
    ici_tfree(sf, sfile_t);
    return 0;
}

static long
sseek(sfile_t *sf, long offset, long whence)
{
    switch (whence)
    {
    case 0:
        sf->sf_ptr = sf->sf_data + offset;
        break;

    case 1:
        sf->sf_ptr += offset;
        break;

    case 2:
        sf->sf_ptr = sf->sf_data + sf->sf_size + offset;
        break;
    }
    return (long)(sf->sf_ptr - sf->sf_data);
}

static int
sfail(sfile_t *sf)
{
    return 0;
}

static int
seof(sfile_t *sf)
{
    return sf->sf_eof;
}

ici_ftype_t string_ftype =
{
    sgetc,
    sungetc,
    sfail,      /* putc */
    sfail,      /* flush */
    sclose,
    sseek,
    seof,
    sfail       /* write */
};

/*
 * Create an ICI file object that treats the given 'data' (of length 'size')
 * as a read-only file.  If 'ref' is non-NULL it is assumed to be an object
 * that must hang around for this data to stay valid, and the data is used
 * in-place (this is used when reading an ICI string as a file).  But if 'ref'
 * is NULL, it is assumed that the data must be copied into a private
 * allocation first.
 * The private allocation will be freed when the file is closed.
 *
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_file_t *
ici_sopen(char *data, int size, ici_obj_t *ref)
{
    register ici_file_t *f;
    register sfile_t    *sf;

    if ((sf = ici_talloc(sfile_t)) == NULL)
        return NULL;
    sf->sf_size = size;
    sf->sf_eof = 0;
    if ((sf->sf_foreign_data = (ref != NULL)))
    {
        sf->sf_data = data;
        sf->sf_ptr = data;
    }
    else
    {
        if ((sf->sf_data = ici_alloc(size)) == NULL)
        {
            ici_tfree(sf, sfile_t);
            return NULL;
        }
        memcpy(sf->sf_data, data, size);
        sf->sf_ptr = sf->sf_data;
    }
    if ((f = ici_file_new((char *)sf, &string_ftype, NULL, ref)) == NULL)
    {
        if (ref == NULL)
        {
            ici_free(sf->sf_data);
            ici_tfree(sf, sfile_t);
        }
        return NULL;
    }
    return f;
}
