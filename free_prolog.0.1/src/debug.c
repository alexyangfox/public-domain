/*
 * debug support routines
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "defs.h"
#include "prof.h"
#include "global.h"

extern struct stack wam[];
extern no hcount;
extern hentry htable;
extern byte *instr_len;
extern struct specsyms g;
extern term local_error();

extern string lextable,newlex,*atomtable;
extern no newatom;

/* loop check limits */

#define MAXITER 8000L

#define CASE_DOUBLE_INSTR \
  case UNIFY_VAR_VAR:   \
  case WRITE_VAR_VAR:   \
  case UNIFY_VAL_VAL:     \
  case WRITE_VAL_VAL:     \
  case UNIFY_VAR_VAL:     \
  case WRITE_VAR_VAL:     \
  case UNIFY_VAL_VAR:     \
  case WRITE_VAL_VAR: \
  case MOVE_REGx2: \
  case LOAD_VALUEx2: \
  case LOAD_VAL_SHORT

#define CASE_MOVE_INSTR \
  case MOVE_REG:        \
  case PUT_VARIABLE:  \
  case GET_VALUE: \
  case LOAD_VALUE

#define CASE_TRY_INSTR  \
  case TRY_ME_ELSE:     \
  case RETRY_ME_ELSE:   \
  case TRUST_ME_ELSE

string
instr_name(no c)
{
    switch (c) {
    case GET_STRUCTURE:
	return "GET_STRUCTURE";
    case PUT_STRUCTURE:
	return "PUT_STRUCTURE";

    case UNIFY_VARIABLE:
	return "UNIFY_VARIABLE";
    case UNIFY_VALUE:
	return "UNIFY_VALUE";
    case UNIFY_CONSTANT:
	return "UNIFY_CONSTANT";

    case WRITE_VARIABLE:
	return "WRITE_VARIABLE";
    case WRITE_VALUE:
	return "WRITE_VALUE";
    case WRITE_CONSTANT:
	return "WRITE_CONSTANT";

    case GET_CONSTANT:
	return "GET_CONSTANT";
    case PUT_CONSTANT:
	return "PUT_CONSTANT";

    case MOVE_REG:
	return "MOVE_REG";
    case PUT_VARIABLE:
	return "PUT_VARIABLE";
    case GET_VALUE:
	return "GET_VALUE";

    case EXECUTE:
	return "EXECUTE";
    case PROCEED:
	return "PROCEED";

    case PUSH_CUT:
	return "PUSH_CUT";
    case PUT_CUT:
	return "PUT_CUT";
    case GET_CUT:
	return "GET_CUT";

    case END:
	return "END";

    case TRY_ME_ELSE:
	return "TRY_ME_ELSE";
    case RETRY_ME_ELSE:
	return "RETRY_ME_ELSE";
    case TRUST_ME_ELSE:
	return "TRUST_ME_ELSE";
    case TRY_ME_ONLY:
	return "TRY_ME_ONLY";

    case NONDET:
	return "NONDET";
    case EXEC_TRY:
	return "EXEC_TRY";
    case CLAUSE:
	return "CLAUSE";
    case SWITCH:
	return "SWITCH";
    case EXEC_SWITCH:
	return "EXEC_SWITCH";
    case JUMP_IF:
	return "JUMP_IF";
    case EXEC_JUMP_IF:
	return "EXEC_JUMP_IF";
    case FIRSTARG:
	return "FIRSTARG";
    case LOAD_CONSTANT:
	return "LOAD_CONSTANT";
    case LOAD_VALUE:
	return "LOAD_VALUE";

    case UNIFY_VAR_VAR:
	return "UNIFY_VAR_VAR @@";
    case WRITE_VAR_VAR:
	return "WRITE_VAR_VAR @@";

    case UNIFY_VAL_VAL:
	return "UNIFY_VAL_VAL @@";
    case WRITE_VAL_VAL:
	return "WRITE_VAL_VAL @@";

    case UNIFY_VAR_VAL:
	return "UNIFY_VAR_VAL @@";
    case WRITE_VAR_VAL:
	return "WRITE_VAR_VAL @@";

    case UNIFY_VAL_VAR:
	return "UNIFY_VAL_VAR @@";
    case WRITE_VAL_VAR:
	return "WRITE_VAL_VAR @@";

    case GET_UNIFY_VAR_VAR:
	return "GET_UNIFY_VAR_VAR @@@";
    case GET_UNIFY_VAL_VAL:
	return "GET_UNIFY_VAL_VAL @@@";
    case GET_UNIFY_VAR_VAL:
	return "GET_UNIFY_VAR_VAL @@@";
    case GET_UNIFY_VAL_VAR:
	return "GET_UNIFY_VAL_VAR @@@";

    case PUT_WRITE_VAR_VAR:
	return "PUT_WRITE_VAR_VAR @@@";
    case PUT_WRITE_VAL_VAL:
	return "PUT_WRITE_VAL_VAL @@@";
    case PUT_WRITE_VAR_VAL:
	return "PUT_WRITE_VAR_VAL @@@";
    case PUT_WRITE_VAL_VAR:
	return "PUT_WRITE_VAL_VAR @@@";

    case MOVE_REGx2:
	return "MOVE_REGx2 @@";
    case LOAD_VALUEx2:
	return "LOAD_VALUEx2 @@";
    case LOAD_VAL_SHORT:
	return "LOAD_VAL_SHORT @@";

    case NOP:
	return "NOP";

    default:

	if (c >= INLINE && c <= NOP)
	    return bu_name[c - INLINE];
	else
	    return "*** BAD INSTRUCTION CODE ***";
    }
}

void
show_tags(void)
{
    int i;
    fprintf(stderr, "FUNTAG->%X ~FUNTAG->%X\nINTTAG->%X ~INTTAG->%X\n",
	    FUNTAG, ~FUNTAG, INTTAG, ~INTTAG);
}

#define HDEREF(x) \
while(ON(HeapStk,x) && !NONVARREF(x) && (x)!=(term)GETREF(x)) \
 (x)=(term)GETREF(x)

string
str(no ptr)
{
    int i;
    no done = FALSE;
    static char s[80];
    for (i = 0; i < MaxStk; i++)
	if (ON(i, ptr)) {
	    sprintf(s, "%s[%u]", wam[i].name, (term *) ptr - wam[i].base);
	    done = TRUE;
	}
    if (done)
	return s;
    else if (INTEGER(ptr))
	sprintf(s, "int(%d)", OUTPUT_INT(ptr));
    else {
	if (GETSYMNO(ptr) < MAXATOM && NAME(ptr))
	    sprintf(s, "%s/%u", NAME(ptr), GETARITY(ptr));
	else
	    sprintf(s, "MEM[%u]", (no) ptr);
    }
    return s;
}

term
cleanup(term p)
{
    if (ON(HeapStk, p) && NONVARREF(p))
	p = (term) GETREF(p);
    return p;
}

string
smartref(term ptr)
{
    term temp = ptr;
    HDEREF(temp);
    temp = cleanup(temp);
    if (ON(HeapStk, ptr) && !NONVARREF(ptr) && temp != cleanup(ptr)) {
	static char s[80];
	sprintf(s, "_%u -> %s", ptr - (term) wam[HeapStk].base, str((no) temp));
	return s;
    }
    else
	return str(temp);
}

void
hshow(void)
{
    no i;
    int max = 50;
    fprintf(stderr, "\nHTABLE %u/%u\n", hcount + 1, HMAX);
    for (i = 0; max > 0 && i < HMAX; i++)
	if (htable[i].val) {
	    max--;
	    fprintf(stderr, "[%u] pred->%s", i, smartref(htable[i].pred));
	    fprintf(stderr, " fun->%s", smartref(htable[i].fun));
	    fprintf(stderr, " val->%s\n", smartref(htable[i].val));
	}
    fprintf(stderr, "\n");
}

void
show_instr(i, iptr, outf)
int i;
instr iptr;
FILE *outf;
{
    no op = GETOP(iptr);
    string name = instr_name(op);
    switch (op) {
    case END:
	fprintf(outf, ".");
	break;

CASE_TRY_INSTR:
	fprintf(outf, "\n<%d> [%u]: (%d) %-16s */%u, ->",
		i, iptr - cbase, op, name, GETREG(iptr));
	fprintf(outf, "[%u] ", GETLABEL(iptr) - cbase);
	break;

CASE_MOVE_INSTR:
CASE_DOUBLE_INSTR:
	fprintf(outf,
		"<%d> [%u]: (%d) %-16s X%u,", i, iptr - cbase, op, name, GETREG(iptr));
	fprintf(outf, "A%u", GETLEFT(iptr));
	break;

    case NONDET:
    case SWITCH:
    case EXEC_SWITCH:
    case JUMP_IF:
    case EXEC_JUMP_IF:
    case EXEC_TRY:
	fprintf(outf, "\n");
    default:
	fprintf(outf, "<%d> [%u]: (%d) %-16s", i, iptr - cbase, op, name);
	if (0 != GETREG(iptr))
	    fprintf(outf, " X%u", GETREG(iptr));
	if (2 == INSTR_LEN(iptr))
	    fprintf(outf, " %s", smartref(GETFUN(iptr)));
    }
    fprintf(outf, "\n");
}

void
show_wam_instr(int i, instr iptr, term * A, term H)
{
    show_instr(i, iptr, stderr);

#if defined DEBUG
    if (H > (term) wam[HeapStk].base && A > wam[ChoiceStk].base) {
	fprintf(stderr, "\tst:%u SAVED_H:%u H:%u TR_TOP:%u ",
		A - wam[ChoiceStk].base, SAVED_H - (term) wam[HeapStk].base,
		H - (term) wam[HeapStk].base, TR_TOP - wam[TrailStk].base);
	if (GETREG(iptr) > 0 && GETREG(iptr) < MAXREG)
	    fprintf(stderr, "X%u", GETREG(iptr));
	if (GETLEFT(iptr) > 0 && GETLEFT(iptr) < MAXREG)
	    fprintf(stderr, "A%u", GETLEFT(iptr));
	fprintf(stderr, "\n");
    }
#endif
}

int
show_code(instr from, instr to)
{
    instr p;
    fprintf(stderr, "SHOWING CODE\n");
    for (p = from; p < to && p < ctop; p += INSTR_LEN(p))
	show_wam_instr(p - cbase, p, wam[ChoiceStk].base, (term) wam[HeapStk].base);
    fprintf(stderr, "\n");
}

/* shows heap, stack & trail_stack */

void
show_regions(term * A, term H)
{
    term t, *tp;

    fprintf(stderr, "\nHEAP => H:%u\n", H - (term) wam[HeapStk].base);
    for (t = (term) wam[HeapStk].base; t < H; t++) {
	term x = t;
	cell v;
	fprintf(stderr, "[_%u] ", t - (term) wam[HeapStk].base);
	while (VAR((v) = GETREF(x)) && (x) != (term) (v)) {
	    fprintf(stderr, "_%u->", (term) (v) - (term) wam[HeapStk].base);
	    (x) = (term) (v);
	}
	fprintf(stderr, ":: %s\n", smartref(v));
    }

    fprintf(stderr, "\nTRAIL => TR_TOP:%u\n", TR_TOP - wam[TrailStk].base);
    for (tp = wam[TrailStk].base; tp < TR_TOP; tp++)
	fprintf(stderr, "[%u]->[%u] %s\n", tp - wam[TrailStk].base,
		*tp - (term) wam[HeapStk].base, smartref(*tp));

    fprintf(stderr, "\nCHOICE => %u\n", A - wam[ChoiceStk].base);
    for (tp = wam[ChoiceStk].base; tp <= A; tp++)
	fprintf(stderr, "[%u] %s\n", tp - wam[ChoiceStk].base, smartref(*tp));

    fprintf(stderr, "\nBlackBOARD => %u\n", wam[BoardStk].top - wam[BoardStk].base);
    for (tp = wam[BoardStk].base; tp < wam[BoardStk].top; tp++)
	fprintf(stderr, "[%u] %s\n", tp - wam[BoardStk].base, smartref(*tp));
}

#define heapused (term)wam[HeapStk].maxused
#define stackused wam[ChoiceStk].maxused
#define trailused wam[TrailStk].maxused

void
profiler(int mes, instr P, term * A, term H)
{
    no i;
    static long all, optable[MAXOP];

    switch (mes) {
    case 0:
	all = 0;
	for (i = 0; i < MAXOP; i++)
	    optable[i] = 0L;
	heapused = (term) wam[HeapStk].base;
	stackused = wam[ChoiceStk].base;
	trailused = wam[TrailStk].base;
	break;

    case 1:
	all++;
	optable[GETOP(P)]++;
	if (H > heapused)
	    heapused = H;
	if (A > stackused)
	    stackused = A;
	if (TR_TOP > trailused)
	    trailused = TR_TOP;

#if defined DEBUG && !defined TRACE
	show_wam_instr(all, P, A, H);
	if (MAXITER < all)
	    fatal_error("MAXITER instructions: INFINITE LOOP?");
	if ((term *) A <= wam[ChoiceStk].base)
	    fatal_error("stack underflow");
#endif

#if defined TRACE
	switch (GETOP(P)) {
	/*
	 * case EXEC_SWITCH: case EXEC_JUMP_IF:
	 * case DEMO_0: case DEMO_1:
	 * CASE_DOUBLE_INSTR:
	 *
	 * case EXEC_TRY: case EXECUTE:
	 * show_wam_instr(all,P,A,H); break;
	 */
	}
#endif
	break;

    case 2:
	{
	    no sz = sizeof(term);
	    fprintf(stderr, "\nPROFILE and STATISTICS\n");
	    for (i = 0; i < MAXOP; i++) {
		long count = optable[i];
		if (count) {
		    long dd = 100 * count / all, cc = 100 * (100 * count - dd * all) / all;
		    fprintf(stderr, "[%2d] %-16s --> \t %8lu\t(%3lu.%2lu %c)\n",
			    i, instr_name(i), count,
			    dd, cc, '%');
		}
	    }
	    fprintf(stderr, "\nTOTAL:%8lu\n\n", all);
	    fprintf(stderr,
		    "@=> instr_len:%x lex:%u code:%u trail:%u\nchoice:%u heap:%u end:%u\n",
		    (no) instr_len,
		    (no) lextable,
		    (no) cbase,
		    (no) wam[TrailStk].base,
		    (no) wam[ChoiceStk].base,
		    (no) wam[HeapStk].base,
		    (no) wam[HeapStk].end
		);
	    fprintf(stderr,
		    "USED: code:%ux%u=%u/%u\natoms:%ux(%u/%u) chars:%u/%u\n",
		    sizeof(cell), ctop - cbase,
		    sizeof(cell) * (ctop - cbase),
		    sizeof(cell) * wam[CodeStk].size,
		    sizeof(string), newatom, MAXATOM,
		    newlex - lextable, MAXLEX);
	    fprintf(stderr,
	    "heap:%u/%u stack:%u/%u trail:%u/%u\nhtable:%ux(%u/%u) bytes\n",
		    sz * (heapused - (term) wam[HeapStk].base), sz * wam[HeapStk].size,
	    sz * (stackused - wam[ChoiceStk].base), sz * wam[ChoiceStk].size,
	     sz * (trailused - wam[TrailStk].base), sz * wam[TrailStk].size,
		    sizeof(struct hentry), hcount + 1, HMAX);
	    fprintf(stderr, "H=%u A=%u TR=%u bytes\n\n",
		    sz * (H - (term) wam[HeapStk].base),
		    sz * (A - wam[ChoiceStk].base), sz * (TR_TOP - wam[TrailStk].base));
	}
	break;
    }
}

void
show_atoms(void)
{
    no i;
    fprintf(stderr, "ATOMTABLE: %u atoms\n\n", newatom);
    for (i = 0; i < MAXATOM; i++)
	if (atomtable[i])
	    fprintf(stderr, "[%u] %s->%u\n", i, atomtable[i], skey(atomtable[i]));
    fprintf(stderr, "\n");
}

cell
list_asm(cell atom, cell arity, cell max)
{
    cell fun;
    instr p;
    term xref;
    cell xval;
    FDEREF(atom);
    atom = xval;
    if (!SYMCONST(xval))
	return (cell) local_error(atom, "list_asm: 1st arg must be a symbol");
    FDEREF(arity);
    arity = xval;
    if (!INTEGER(arity))
	return (cell) local_error(arity, "list_asm: arg 2 must an arity");
    FDEREF(max);
    max = xval;
    if (!INTEGER(max))
	return (cell) local_error(max, "list_asm: arg 3 must be an integer");
    arity = OUTPUT_INT(arity) + 1;
    max = OUTPUT_INT(max);
    fun = PUTARITY(atom, arity);
    p = (instr) hget(g.predmark, fun);
    if (!p)
	return FALSE;
    fprintf(g.tellfile, "BINARY PREDICATE: %s/%d @code[%u]\n",
	    NAME(atom), arity, p - cbase);
    {
	int i;
	for (i = 0; i < max; i++, p += INSTR_LEN(p))
	    show_instr(i, p, g.tellfile);
    }
    return TRUE;
}
