/*
 * print
 */
#include <stdio.h>

#include "global.h"

extern struct specsyms g;
extern struct stack wam[];
extern struct limit max;
extern string sbuf, stop;
extern string *atomtable;

#define COUT(Char) (*stop++ = (Char))

#define SOUT(Obj) {string s0=(Obj); while(*stop++ = *s0++);} stop--

#define IOUT(Obj) sprintf(ibuf,"%d",(Obj)); SOUT(ibuf)

#define MAX1 32

/* prints a term given as a heap reference */

void
out(cell xval, term * A)
{
    term xref;
    static char ibuf[MAX1];

    FDEREF(xval);
    if (stop - sbuf > max.SBUF - MAX1) {
	warnmes("string buffer (-i option) exceeded or infinite term");
	return;
    }
    if (VAR(xval)) {
	COUT('_');
	IOUT((term) xval - (term) wam[BoardStk].base);
    }
    else {
	if (INTEGER(xval)) {
	    IOUT(OUTPUT_INT(xval));
	}
	else {
	    if (!GETARITY(xval)) {
		SOUT(NAME(xval));
	    }
#ifdef NOTDEF
	    else if (g.DIF == xval) {
		out(xref + 1, A),
		    SOUT(NAME(s));
		out(xref + 2, A);
	    }
#endif
	    else if IS_LIST
		(xval) {
		COUT('[');
		out(++xref, A);
		FDEREF(++xref);
		while (IS_LIST(xval)) {
		    COUT(',');
		    out(++xref, A);
		    FDEREF(++xref);
		}
		if (g.NIL != xval) {
		    COUT('|');
		    out(xval, A);
		}
		COUT(']');
		}
	    else {
		no i;
		SOUT(NAME(xval));
		COUT('(');
		for (i = 1; i < GETARITY(xval); i++) {
		    out(xref[i], A);
		    COUT(',');
		}
		out(xref[i], A);
		COUT(')');
	    }
	}
    }
}

cell
sout(term xref, int A)
{
    stop = sbuf;
    out(xref, A);
    COUT('\0');
    return new_func(sbuf, 0);
}

void
fout(int xval, int A)
{
    stop = sbuf;
    out(&xval, A);
    COUT('\0');
    fprintf(g.tellfile, "%s", sbuf);
}
