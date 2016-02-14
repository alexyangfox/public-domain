/*
 * binprolog builtins
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "global.h"

extern struct specsyms g;
extern struct stack wam[];
extern string *atomtable,sbuf,stop;

/* BUILTINS */

term
local_error(cell Sym, string Msg)
{
    if (VAR(Sym)) {
	int a = (term *) Sym - wam[HeapStk].base;
	if (a >= 0)
	    fprintf(stderr,
		    "_%d ??? ", (term *) Sym - wam[HeapStk].base);
	else
	    fprintf(stderr, "mem[%u] ??? ", Sym);
    }
    else if (INTEGER(Sym))
	fprintf(stderr, "%d ??? ", OUTPUT_INT(Sym));
    else
	fprintf(stderr, "%s/%d ??? ", NAME(Sym), GETARITY(Sym));
    warnmes(Msg);
    return NULL;
}

int
ocompare(cell vl, cell vr)
{
    int ires;
    ires = (!VAR(vl) << 1) | (!INTEGER(vl));
    ires -= (!VAR(vr) << 1) | (!INTEGER(vr));
    if (0 == ires) {
	if (VAR(vl))
	    ires = vl - vr;
	else if (INTEGER(vl))
	    ires = OUTPUT_INT(vl) - OUTPUT_INT(vr);
	else {
	    ires = strcmp(NAME(vl), NAME(vr));
	    if (0 == ires)
		ires = GETARITY(vl) - GETARITY(vr);
	}
    }
    return (ires > 0) ? !!ires : -!!ires;
}

int
compare(term l, term r)
{
    int ires;
    cell vl, vr;
    DEREF2(l, vl);
    DEREF2(r, vr);
    ires = ocompare(vl, vr);
    if (0 == ires && COMPOUND(vl))
	for (vr = GETARITY(vl), vl = 1; 0 == ires && vl <= vr; vl++)
	    ires = compare(l + vl, r + vl);
    return ires;
}

void
pp(term p, term * A)
{
    fprintf(g.tellfile, "|==>");
    fout(p, A);
    NEWLN();
}

void
answer(term * A)
{
    pp(GETREF((term) wam[HeapStk].base), A);
}

no
name2buf(cell vt)
{
    if (INTEGER(vt))
	sprintf(sbuf, "%d", OUTPUT_INT(vt));
    else {
	if (!GETARITY(vt))
	    sprintf(sbuf, "%s", NAME(vt));
	else
	    return FALSE;
    }
    return TRUE;
}

no
list2buf(term l, cell vl)
{
    term car;
    string name = sbuf;
    while (IS_LIST(vl)) {
	car = ++l;
	DEREF2(car, vl);

	if (INTEGER(vl))
	    *name++ = (char) OUTPUT_INT(vl);
	else
	    return FALSE;

	++l;
	DEREF2(l, vl);
    }
    if (g.NIL != vl)
	return FALSE;

    *name = '\0';
    return TRUE;
}

cell
symcat(cell x1, cell x2)
{
    if (!SYMCONST(x1))
	ERREXIT("symcat/3: bad arguments")
	    if (SYMCONST(x2))
	    sprintf(sbuf, "%s_%s", NAME(x1), NAME(x2));
	else if (VAR(x2))
	    sprintf(sbuf, "%s__%d", NAME(x1),
		    (cell) ((term) x2 - (term) wam[HeapStk].base));
	else
	    sprintf(sbuf, "%s_%d", NAME(x1), OUTPUT_INT(x2));
    return new_func(sbuf, 0);
}

no
see(cell xval)
{
    FILE *f;
    string fname;
    if (VAR(xval) || INTEGER(xval))
	return (no) local_error(xval, "bad file name in TELL");
    fname = NAME(xval);
    f = (FILE *) hget(g.seemark, xval);
    if ((cell) f == g.closed_file) {
	f = fopen(fname, "rb");
	if (!f)
	    ERREXIT("unable to reopen to see file")
	if (!hset(g.seemark, xval, f))
	    ERREXIT("hset in see")
    }
    else if (!f) {
	f = fopen(fname, "rb");
	if (!f)
	    return FALSE;
	if (!hdef(g.seemark, xval, f))
	    ERREXIT("hdef in see")
    }
 /* already opened and ok */
    g.seefile = f;
    g.seefunc = xval;
    return TRUE;
}

void
seen(void)
{
#ifdef NOTDEF
    clearerr(g.seefile);
#endif
    if (g.seefile != stdin) {
	cell f = hget(g.seemark, g.seefunc);
	if (f != g.closed_file) {
	    if (f)
		hset(g.seemark, g.seefunc, g.closed_file);
	    else
		hdef(g.seemark, g.seefunc, g.closed_file);
	    fclose(g.seefile);
	}
    }
    g.seefile = stdin;
    g.seefunc = g.user;
}

no
tell_to(cell xval)
{
    FILE *f;
    string fname;
    if (VAR(xval) || INTEGER(xval))
	return (no) local_error(xval, "bad file name in TELL");
    fname = NAME(xval);
    f = (FILE *) hget(g.tellmark, xval);
    if ((cell) f == g.closed_file) {
	f = fopen(fname, "wb");
	if (!f)
	    return FALSE;
	if (!hset(g.tellmark, xval, f))
	    ERREXIT("hset in tell")
    }
    else if (!f) {
	f = fopen(fname, "wb");
	if (!f)
	    return FALSE;
	if (!hdef(g.tellmark, xval, f))
	    ERREXIT("hdef in tell")
    }
 /* already opened and ok */
    g.tellfile = f;
    g.tellfunc = xval;
    return TRUE;
}

void
told(void)
{
#ifdef NOTDEF
    clearerr(g.tellfile);
#endif
    if (g.tellfile != stdout) {
	hset(g.tellmark, g.tellfunc, g.closed_file);
	fclose(g.tellfile);
    }
    g.tellfile = stdout;
    g.tellfunc = g.user;
}

int
stat_used(term * top, int s)
{
    return (top - wam[s].base) * sizeof(*top);
}

int
stat_left(term * top, int s)
{
    return (wam[s].end - top) * sizeof(*top);
}

void
shell_1(term regs)
{
    term xref;
    cell xval;
    FDEREF(regs[1]);
    strcpy(sbuf, NAME(xval));
    (void) system(sbuf);
}

int
add_instr(term regs)
{
    string name;
    if (INTEGER(X(3))) {
	name = sbuf;
	sprintf(name, "%d", OUTPUT_INT(X(3)));
    }
    else
	name = NAME(X(3));
    if (!insert_op(
		   (no) OUTPUT_INT(X(1)),
		   (no) OUTPUT_INT(X(2)),
		   name,
		   (no) OUTPUT_INT(X(4))
		   ))
	return FALSE;
    else
	return TRUE;
}

/* TODO: member_chk -> builtin */

term
det_append(term H, term regs)
{
    term xref;
    cell xval;
    xref = RX(1);
    if (VAR((cell) xref)) {
	xval = GETREF(xref);
	while (IS_LIST(xval)) {
	    CHECK(H, HeapStk, "heap owerflow in det_append/3");
	    PUSH_LIST(xref + 1);
	    FDEREF(xref + 2);
	}
    }
    else
	xval = (cell) xref;
    if (g.NIL != xval)
	return local_error(xval, "[] expected in 1st arg of det_append/3");
    else {
	PUSHVAL(RX(2));
    }
    return H;
}


term
recursive_save_term(term h, term t, term ct, term * A, term from, term to, term hb)
{
    cell val_t;
    DEREF2(t, val_t);
    if (t >= from && t < to)
	SETREF(ct, t);
    else if (VAR(val_t)) {
	SETREF(ct, ct);
	SETREF(t, ct);
	if (t < hb)
	    TRAIL_IT(t);
    }
    else if (INTEGER(val_t) || !GETARITY(val_t))
	SETREF(ct, val_t);
    else {
	SETREF(ct, h);
	SETREF(h, val_t);
	ct = h++;
	h += (val_t = GETARITY(val_t));
	if (h > to)
	    return NULL;
	while (val_t-- && h)
	    h = recursive_save_term(h, ++t, ++ct, A, from, to, hb);
    }
    return h;
}

/* b is the top of the bboard */
term
save_term(term b, term t, term * A)
{
    term *bakTR = TR_TOP;
    SETREF(b, b);
    b = recursive_save_term(b + 1, t, b, A,
        (term) wam[BoardStk].base,	/* replace base->top to avoid sharing */
	(term) wam[BoardStk].margin,
	(term) wam[HeapStk].margin);
    TR_TOP = unwind_trail(TR_TOP, bakTR);
    return b;
}

#ifdef NOTDEF
term recursive_copy_term(h,t,ct,A, hb)
  term h,t,ct,*A,hb;
{ cell val_t;
  DEREF2(t,val_t);
  if(VAR(val_t))
    { SETREF(ct,ct);
      SETREF(t,ct);
      if(t<hb) TRAIL_IT(t);
    }
  else if(INTEGER(val_t) || !GETARITY(val_t))
    SETREF(ct,val_t);
  else
    {
      SETREF(ct,h); SETREF(h,val_t);
      ct=h++; h+=(val_t=GETARITY(val_t));
      CHECK(h,HeapStk,"heap overflow in copy_term");
      while(val_t--)
        h=recursive_copy_term(h,++t,++ct,A, hb);
    }
  return h;
}
#endif

term
copy_term(term h, term t, term * A)
{
    term *bakTR = TR_TOP;
    SETREF(h, h);
 /* h=recursive_copy_term(h+1,t,h,A, h); */

    h = recursive_save_term(h + 1, t, h, A,
			    h,
			    (term) wam[HeapStk].margin,
			    h);

    if (!h)
	fatal_error("heap overflow in copy_term");

    TR_TOP = unwind_trail(TR_TOP, bakTR);
    return h;
}



