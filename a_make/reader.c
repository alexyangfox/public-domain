/*************************************************************************
|                                                                        |
|   READER.C                                                    31.08.89 |
|   PAMAKE Utility:  read in the makefile                                |
|                                                                        |
*************************************************************************/

#ifdef VMS
#include stdio
#include string
#include stdlib
#include ctype
#include "h.h"
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "h.h"
#endif

int                     lineno;

/*****  syntax error handler - print message, with line number, and exit */

#ifndef __TSC__
void
error(msg, a1, a2, a3)
char *          msg;
{
    fprintf(stderr, "%s: ", myname);
    fprintf(stderr, msg, a1, a2, a3);
    if (lineno)
        fprintf(stderr, " in file %s on line %d", fname[nestlvl], lineno);
    fputc('\n', stderr);
    exit(PXERROR);
}
#endif

/*****  read a line */

/*************************************************************************
|                                                                        |
|   Read a line into the supplied string of length LZ.  Remove           |
|   comments, ignore blank lines. Deal with quoted (\) #, and            |
|   quoted newlines.  If EOF return TRUE.                                |
|                                                                        |
*************************************************************************/

bool
getline(str)
char *          str;
{
    register char *         p;
    char *                  q;
    char *                  a;
    int                     pos = 0;
    FILE *                  fd;
    FILE *                  incf;
    int                     specialhash;

    if (nestlvl < 0) return TRUE;           /* EOF */

    for (;;)
    {
        fd = ifile[nestlvl];
        if (fgets(str+pos, LZ-pos, fd) == (char *)0)
        {
            fclose(fd);
            if (nestlvl == 0) 
            {
                nestlvl--;
                ifeof();
                return TRUE;
            }
            fln[nestlvl] = 0;
            nestlvl--;
            continue;
        }
        lineno = ++fln[nestlvl];

        if ((p = strchr(str+pos, '\n')) == (char *)0)
            error("Input line is too long");

        if (p[-1] == '\\')
        {
            p[-1] = '\n';
            pos = (int)(p - str);
            continue;
        }

        if (!strncmp(str,"#ifn",4)) specialhash = 2;
        else if (!strncmp(str,"#if",3)) specialhash = 1;
        else if (!strncmp(str,"#else",5)) specialhash = 3;
        else if (!strncmp(str,"#endif",6)) specialhash = 4;
        else if (!strncmp(str,"#include",8)) specialhash = 5;
        else specialhash = 0;

        p = str + (specialhash != 0);
        while (((q = strchr(p, '#')) != (char *)0) &&
            (p != q) && (q[-1] == '\\'))
        {
            a = q - 1;      /*  Del \ chr; move rest back  */
            p = q;
            while ((*a++ = *q++) != '\0')
                ;
        }

        if (q != (char *)0) 
        {
            while ( (q != str) && (pspace(q[-1])) ) q--;
            q[0] = '\n';
            q[1] = '\0';
        }

        if (ifproc(str,specialhash)) 
        {
            pos = 0;
            continue;
        }

        if (specialhash == 5)
        {
            q = str + 8;
            while (pspace(q[0])) q++;
            if (nestlvl >= 3)
                fatal("Include files nested too deeply");
            a = q + strlen(q) - 1;
            if (*a == '\n') *a = '\0';
            expand(q);
            incf = fopen(dollar(q),"r");
            if (incf == (FILE *)0)
                fatal("Unable to open include file %s", dollar(q));
            ifile[++nestlvl] = incf;
            strncpy(fname[nestlvl],q,80);
            continue;
        }

        p = str;
        while (pspace(*p))     /*  Checking for blank  */
            p++;

        if (*p != '\0')  
            return FALSE;
        pos = 0;
    }
}

/*****  get token */

/*************************************************************************
|                                                                        |
|   Get a word from the current line, surounded by white space.          |
|   return a pointer to it. String returned has no white spaces          |
|   in it.                                                               |
|                                                                        |
*************************************************************************/

char *
gettok(ptr)
char * *        ptr;
{
    register char *         p;

    while (pspace(**ptr)) (*ptr)++;         /* skip spaces  */
    if (**ptr == '\0') return NULL;         /* nothing after spaces  */
    p = *ptr;                               /* word starts here  */
    while ((**ptr != '\0') && (!pspace(**ptr))) (*ptr)++;  /* find word end */
    *(*ptr)++ = '\0';                       /* terminate it  */
    return(p);
}

/*************************************************************************
|                                                                        |
|   Check that the character is whitespace.  Note that isspace gives an  |
|   undefined result if the character is not ASCII.                      |
|                                                                        |
*************************************************************************/

int pspace (c)
int c;

{
    return (isascii(c & 0xff) && isspace(c & 0xff));
}
