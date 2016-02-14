/*************************************************************************
|                                                                        |
|   MTIME.C                                                     30.09.89 |
|   PAMAKE Utility:  compiler-dependent time functions                   |
|                                                                        |
*************************************************************************/

/*************************************************************************
|                                                                        |
|   Important note:  PAMAKE stores date/time values in two different     |
|   ways depending on which compiler is in use:                          |
|                                                                        |
|   LATTICE, TURBOC   :   Date/time held in DOS file format              |
|                         (PAMAKE_FTIME is #defined)                     |
|                                                                        |
|   MSC, VMS          :   Date/time held in 'Unix' format                |
|                                                                        |
*************************************************************************/

#ifdef VMS
#include stdio
#include "h.h"
#include errno
#include ctype
#include string
#include stdlib
#include types
#include stat
#endif

#ifdef LATTICE
#define PAMAKE_FTIME 1
#include <stdio.h>
#include <fcntl.h>
#include "h.h"
#include <dos.h>
#include <error.h>
#include <time.h>
#endif

#ifdef __TURBOC__
#define PAMAKE_FTIME 1
#include <stdio.h>
#include <fcntl.h>
#include "h.h"
#include <dos.h>
#include <io.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#endif

#ifdef MSC
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <sys\utime.h>
#include <dos.h>
#include "h.h"
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#endif 

/*************************************************************************
|                                                                        |
|   This module implements the following functions:                      |
|                                                                        |
|   modtime    :  get the modification time of a file, 0 if not found    |
|   curtime    :  return current time in suitable format                 |
|   touch      :  update the time of a file to now                       |
|   dodisp     :  display a file time                                    |
|                                                                        |
*************************************************************************/

/*************************************************************************
|                                                                        |
|   MODTIME                                                              |
|                                                                        |
*************************************************************************/

#ifdef LATTICE
#define MODTIME_DONE 1

void
modtime(np)
struct name *           np;
{
    long                    ret;
    int                     fd;

    if ((fd = open(dollar(np->n_name), 0)) < 0)
    {
        if (errno != ENOENT) 
            fatal("Unable to open file %s: error code %02x", dollar(np->n_name), errno);
        np->n_time = 0L;
    }
    else
    {
        if ((ret = getft(fd)) == -1L)
            fatal("Unable to get modification time for file %s; error code %02x", dollar(np->n_name), errno);
        np->n_time = ret;
    }
    close(fd);
}

#endif


#ifdef __TURBOC__
#define MODTIME_DONE 1

void
modtime(np)
struct name *           np;
{
    long                    ret;
    int                     fd;

    if ((fd = open(dollar(np->n_name), 0)) < 0)
    {
        if (errno != ENOENT) 
            fatal("Unable to open file %s: error code %02x", dollar(np->n_name), errno);
        np->n_time = 0L;
    }
    else
    {
        if (getftime(fd,(struct ftime *)&ret))
            fatal("Unable to get modification time for file %s; error code %02x", dollar(np->n_name), errno);
        np->n_time = ret;
    }
    close(fd);
}

#endif


#ifndef MODTIME_DONE

void
modtime(np)
struct name *           np;
{
    struct stat             info;
    int                     fd;

    if ((fd = open(dollar(np->n_name), 0)) < 0)
    {
        if (errno != ENOENT) 
            fatal("Unable to open file %s: error code %02x", dollar(np->n_name), errno);
        np->n_time = 0L;
    }
    else if (fstat(fd, &info) < 0)
        fatal("Unable to get modification time for file %s; error code %02x", dollar(np->n_name), errno);
    else
        np->n_time = info.st_mtime;

    close(fd);
}

#endif


/*************************************************************************
|                                                                        |
|   CURTIME                                                              |
|                                                                        |
*************************************************************************/

#ifdef PAMAKE_FTIME

unsigned long curtime(void)
{
    unsigned long           ct;
    struct tm *             p;   
    unsigned int            lo,hi;

    time((time_t *)&ct);
    p = localtime((time_t *)&ct);
    hi = (p->tm_year - 80) << 9;
    hi += ((p->tm_mon + 1) << 5);
    hi += p->tm_mday;
    lo = (p->tm_hour << 11);
    lo += (p->tm_min << 5);
    lo += p->tm_sec;
    return (lo + ((unsigned long)hi << 16));
}

#else

unsigned long curtime(void)
{
    unsigned long           ct;

    time(&ct);
    return ct;
}

#endif


/*************************************************************************
|                                                                        |
|   TOUCH                                                                |
|                                                                        |
*************************************************************************/

#ifdef VMS
#define TOUCH_DONE 1

void
touch(np)
struct name *           np;
{
    printf("    file %s not touched:  touch not implemented for VMS\n",
        dollar(np->n_name));
}

#endif

#ifdef LATTICE
#define TOUCH_DONE 1

void
touch(np)
struct name *           np;
{
    int                     fd;

    if (!domake || !silent) printf("    touch %s\n", dollar(np->n_name));
    if (domake)
    {
        if ((fd = open(dollar(np->n_name), 0)) < 0)
        {
            if (errno != ENOENT) 
                fatal("Unable to open file %s: error code %02x", dollar(np->n_name), errno);
            np->n_time = 0L;
            return;
        }
        if (chgft(fd,curtime()))
        {
            fatal("Unable to update modification time for file %s; error code %02x",
                dollar(np->n_name),errno);
        }
        close(fd);
    }
}

#endif

#ifdef __TURBOC__
#define TOUCH_DONE 1

void
touch(np)
struct name *           np;
{
    int                     fd;
    long                    ret;

    if (!domake || !silent) printf("    touch %s\n", dollar(np->n_name));
    if (domake)
    {
        if ((fd = open(dollar(np->n_name), 0)) < 0)
        {
            if (errno != ENOENT) 
                fatal("Unable to open file %s: error code %02x", dollar(np->n_name), errno);
            np->n_time = 0L;
            return;
        }
        ret = curtime();
        if (setftime(fd,(struct ftime *)&ret))
        {
            fatal("Unable to update modification time for file %s; error code %02x",
                dollar(np->n_name),errno);
        }
        close(fd);
    }
}

#endif


#ifndef TOUCH_DONE

void
touch(np)
struct name *           np;
{
    if (!domake || !silent) printf("    touch %s\n", dollar(np->n_name));
    if (domake)
    {
        if (utime(dollar(np->n_name),(void *)0))
        {
            if (errno != ENOENT) 
                fatal("Unable to update modification time for file %s; error code %02x",
                    dollar(np->n_name),errno);
            else
                fprintf(stderr,"%s: '%s' modification time not updated, file not found\n",
                    myname, dollar(np->n_name));
        }
    }
}

#endif


/*************************************************************************
|                                                                        |
|   DODISP                                                               |
|                                                                        |
*************************************************************************/

#ifdef PAMAKE_FTIME

void
dodisp(name,t)
char *          name;
long            t;
{
    unsigned long u = t;

    static char * mn[12] = {"Jan","Feb","Mar","Apr","May","Jun",
                            "Jul","Aug","Sep","Oct","Nov","Dec"};
    if (t < 10)
        printf("... %s Timestamp %ld\n",name,t);
    else 
    {
        printf("... %s Timestamp ",name);
        printf("%s %02d %02d:%02d:%02d %4d\n",
            mn[(int)((u >> 21) & 0xf) - 1],
            (int)((u >> 16) & 0x1f),
            (int)((u >> 11) & 0x1f),
            (int)((u >> 5) & 0x3f),
            (int)(u & 0x1f),
            (int)((u >> 25) + 1980));
    }
}

#else

void
dodisp(name,t)
char *          name;
long            t;
{
    if (t < 10)
        printf("... %s Timestamp %ld\n",name,t);
    else
        printf("... %s Timestamp %s\n",name,ctime(&t));
}

#endif


