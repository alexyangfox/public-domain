#define ICI_CORE
#include "fwd.h"
#include "buf.h"
#include <errno.h>


/*------------THIS SHOULD NOW BE ANSI COMPATIBLE, SO WE SHOULDN'T
              NEED THIS STUFF. BUT I'LL LEAVE IT HERE TILL THAT IS
              PROVED.-----------------------------------------------*/
#ifdef  NOTDEF
#ifndef BSD
#include <errno.h>
# ifdef hpux
/*
 * HPUX only defines them for C++
 */
extern int      errno;
extern int      sys_nerr;
extern char     *sys_errlist[];
# endif /* hpux */
#else /* BSD */

# ifdef sun
/*
 * Suns don't define these in any include files...
 */
extern int      errno;
extern int      sys_nerr;
extern char     *sys_errlist[];
# endif

# if defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__linux__)
#  include <errno.h>
# endif

#endif

#endif /* NOTDEF--------------------------------------------------*/


/*
 * Convert the current errno (that is, the standard C global error code) into
 * an ICI error message based on the standard C 'strerror' function.  Returns
 * 1 so it can be use directly in a return from an ICI instrinsic function or
 * similar.  If 'dothis' and/or 'tothis' are non-NULL, they are included in
 * the error message.  'dothis' should be a short name like "'open'".
 * 'tothis' is typically a file name.  The messages it sets are, depending on
 * which of 'dothis' and 'tothis' are NULL, the message will be one of:
 *
 *  strerror
 *  failed to dothis: strerror
 *  failed to dothis tothis: strerror
 *  tothis: strerror
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_get_last_errno(char *dothis, char *tothis)
{
    char                *e;
    size_t              z;

    z = 20;
    if (dothis != NULL)
        z += strlen(dothis);
    if (tothis != NULL)
        z += strlen(tothis);
    z += strlen(e = strerror(errno));
    if (ici_chkbuf(z))
        return 1;
    if (dothis == NULL && tothis == NULL)
        strcpy(buf, e);
    else if (dothis != NULL && tothis == NULL)
        sprintf(buf, "failed to %s: %s", dothis, e);
    else if (dothis != NULL && tothis != NULL)
        sprintf(buf, "failed to %s %s: %s", dothis, tothis, e);
    else
        sprintf(buf, "%s: %s", tothis, e);
    ici_error = buf;
    return 1;
}
