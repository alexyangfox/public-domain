/* +++Date last modified: 05-Jul-1997 */

/*
**  WIN 32 & OS/2 support functions for DIRPORT.H
**
**  Public domain by Jerry Coffin
**
**  15-May-96 David Nugent    Moved OS/2 functions from
**                            dirport.h to here
**                            Fixed for 32-bit OS/2
**                            Allowed compilation under
**                            DOS for easier makefiles
*/

#if defined(__MSDOS__) || defined(MSDOS)
/* #error "DirPort.c is NOT intended for DOS programs ..." */
#else

#include "dirport.h"

#if defined(OS2)

int FIND_FIRST (char * spec, unsigned attr, DOSFileData *ff)
{
  _SYSINT cnt = 1;
  ff->dh = (HDIR) -1;
  return (int) DosFindFirst ((PSZ)spec, &ff->dh, (_SYSINT)attr, &ff->f,
                             (_SYSINT)sizeof(struct _FILEFINDBUF), &cnt, FINDLVL);
}

int FIND_NEXT (DOSFileData *ff)
{
  _SYSINT cnt = 1;
  return (int) DosFindNext (ff->dh, &ff->f, sizeof(struct _FILEFINDBUF), &cnt);
}

int FIND_END (DOSFileData *ff)
{
  return (int) DosFindClose (ff->dh);
}

#else   /* WIN32 & WINNT */

WORD ff_date(DOSFileData *f)
{
      WORD DOS_date;
      WORD DOS_time;

      FileTimeToDosDateTime(&(f->file.ftLastWriteTime), &DOS_date, &DOS_time);

      return DOS_date;
}

WORD ff_time(DOSFileData *f)
{
      WORD DOS_date;
      WORD DOS_time;

      FileTimeToDosDateTime(&(f->file.ftLastWriteTime), &DOS_date, &DOS_time);

      return DOS_time;
}

WORD ff_yr(DOSFileData *f)
{
      SYSTEMTIME t;

      FileTimeToSystemTime(&(f->file.ftLastWriteTime), &t);

      return t.wYear;
}

WORD ff_mo(DOSFileData *f)
{
      SYSTEMTIME t;

      FileTimeToSystemTime(&(f->file.ftLastWriteTime), &t);

      return t.wMonth;
}


WORD ff_day(DOSFileData *f)
{
      SYSTEMTIME t;

      FileTimeToSystemTime(&(f->file.ftLastWriteTime), &t);

      return t.wDay;
}

WORD ff_hr(DOSFileData*f)
{
      SYSTEMTIME t;

      FileTimeToSystemTime(&(f->file.ftLastWriteTime), &t);

      return t.wHour;
}

WORD ff_min(DOSFileData *f)
{
      SYSTEMTIME t;

      FileTimeToSystemTime(&(f->file.ftLastWriteTime), &t);

      return t.wMinute;
}

WORD ff_tsec(DOSFileData *f)
{
      SYSTEMTIME t;

      FileTimeToSystemTime(&(f->file.ftLastWriteTime), &t);

      return t.wMilliseconds / 100;
}

int FIND_FIRST(char *spec, unsigned attrib, DOSFileData *ff)
{
      ff_attr(ff) = attrib | _A_ARCH;

      ff->handle = FindFirstFile(spec, &(ff->file));

      if ( INVALID_HANDLE_VALUE == ff->handle ) {
            return 1;
      }

      while ( 0 != (ff->file.dwFileAttributes & ~ff_attr(ff)))
      {
            if ( FALSE == FindNextFile(ff->handle, &(ff->file))) {
                  return 1;
            }
      }
      return 0;
}

int FIND_NEXT(DOSFileData *ff)
{
      do {
            if ( FALSE == FindNextFile(ff->handle, &(ff->file))) {
                  return 1;
            }

      } while ( 0 != (ff->file.dwFileAttributes & ~ff_attr(ff)));
      return 0;
}

int FIND_END(DOSFileData *ff)
{
    return (int)FindClose(ff->handle);
}

#endif /* OS/2   */

#endif /* MS-DOS */
