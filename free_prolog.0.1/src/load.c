/*
 * bytecode loader
 */
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "global.h"

extern struct specsyms g;
extern struct stack wam[];
extern struct limit max;
extern string sbuf,stop;
extern string *atomtable;

byte *instr_len;

void
init_instr_len(void)
{
    int i;

    instr_len = (byte *) malloc(MAXOP);

    for (i = 0; i < MAXOP; i++)
	instr_len[i] = 1;

    instr_len[GET_STRUCTURE] = 2;
    instr_len[PUT_STRUCTURE] = 2;

    instr_len[UNIFY_CONSTANT] = 2;

    instr_len[WRITE_CONSTANT] = 2;

    instr_len[GET_CONSTANT] = 2;
    instr_len[PUT_CONSTANT] = 2;

    instr_len[EXECUTE] = 2;
    instr_len[PROCEED] = 2;

    instr_len[END] = 2;

    instr_len[TRY_ME_ELSE] = 2;
    instr_len[RETRY_ME_ELSE] = 2;
    instr_len[TRUST_ME_ELSE] = 2;
    instr_len[TRY_ME_ONLY] = 2;

    instr_len[NONDET] = 2;
    instr_len[EXEC_TRY] = 2;
    instr_len[EXEC_SWITCH] = 2;
    instr_len[SWITCH] = 2;
    instr_len[JUMP_IF] = 2;
    instr_len[EXEC_JUMP_IF] = 2;
    instr_len[LOAD_CONSTANT] = 2;

    instr_len[GET_UNIFY_VAR_VAR] = 2;
    instr_len[GET_UNIFY_VAL_VAL] = 2;
    instr_len[GET_UNIFY_VAR_VAL] = 2;
    instr_len[GET_UNIFY_VAL_VAR] = 2;

    instr_len[PUT_WRITE_VAR_VAR] = 2;
    instr_len[PUT_WRITE_VAL_VAL] = 2;
    instr_len[PUT_WRITE_VAR_VAL] = 2;
    instr_len[PUT_WRITE_VAL_VAR] = 2;
}

char
get_one_int(char *name, int *ip)
{
    int val = 0;
    char c;
    while ((c = *name++) && c >= '0' && c <= '9')
	val = (val << 3) + (val << 1) + (c - '0');	/* efficient val*10 */
    *ip = val;
    return !c;
}

cell
fun_name(string name, no arity)
{
    int val;
    cell f;
    if (get_one_int(name, &val))
	f = INPUT_INT(val);
    else
	f = new_func(name, arity);
    return f;
}

cell
input_fun(string name, no arity)
{
    int val;
    cell f;
    if (' ' != *name && 1 == sscanf(name, "%d", &val))
	f = INPUT_INT(val);
    else
	f = new_func(name, arity);
    return f;
}

#if defined SUPER
#define MAGIC GET_UNIFY_VAR_VAR
#else
#define MAGIC GET_STRUCTURE
#endif

instr
link_switch(instr p, no doit)
{
    instr jlabel = GETLABEL(p + 2), label = jlabel;
    if (TRUST_ME_ELSE == GETOP(label) &&
	MAGIC == GETOP(label + 2)) {
	if (doit) {
	    SETOP(p, JUMP_IF);
	    label += 2;
	    SETLABEL(p, label);
	}
	return jlabel;
    }
    return NULL;
}

no
link_code(instr p)
{
    instr label;
    cell f;
    int l = 1;
    for (; p < ctop; p += l) {
	l = INSTR_LEN(p);
	switch (GETOP(p)) {
	case EXECUTE:			/* we look to what's next */
	    f = GETFUN(p);
	    if (!(label = (instr) hget(g.predmark, f))) {
		fprintf(stderr, "code[%u..%u] >>%s/%u<<\n",
			p - cbase, ctop - cbase, NAME(f), GETARITY(f));
		ERREXIT("undefined predicate after :-")
	    }
	    else {
		switch (GETOP(label)) {

		case DEMO_0:
		    SETOP(p, DEMO_0);
		    SETREG(p, 1);
		    SETOP(p + instr_len[DEMO_0], NOP);
		    SETREG(p + instr_len[DEMO_0], 0);
		    break;

		case DEMO_1:
		    SETOP(p, DEMO_1);
		    SETREG(p, 1);
		    SETOP(p + instr_len[DEMO_1], NOP);
		    SETREG(p + instr_len[DEMO_1], 0);
		    break;

		case NONDET:
		    SETOP(p, EXEC_TRY);
		    SETLABEL(p, label + instr_len[NONDET]);
		    break;

		case TRY_ME_ELSE:
		    SETOP(p, EXEC_TRY);
		    SETLABEL(p, label);
		    break;

		case JUMP_IF:
		    SETOP(p, EXEC_JUMP_IF);
		    SETLABEL(p, label);
		    break;

		case SWITCH:
		    if (link_switch(label, FALSE)) {
			SETOP(p, EXEC_JUMP_IF);
			SETLABEL(p, label);
		    }
		    else {
			SETOP(p, EXEC_SWITCH);
			SETLABEL(p, label);
		    }
		    break;

		default:
		    SETLABEL(p, label);
		}
	    }
	    break;

	case SWITCH:
	    (void) link_switch(p, TRUE);
	    break;

	case NONDET:
	    label = (instr) hget(g.predmark, GETFUN(p));
	    hset(g.predmark, GETFUN(p), label + instr_len[NONDET]);
	    SETOP(label, TRY_ME_ONLY);
	    break;

	default:
	    if (0 == l) {
		l = 1;
	    }
	}
    }
    return TRUE;
}

cell currpred;
no badcode;

instr last;
static no prev_prev_len,prev_len;

#define SKIPBAD() if(badcode) return FALSE

#define COMPRESS(Simple,Double,First,Triple) \
if(1==prev_len && Simple==GETOP(ctop-1)) \
  {       SETOP(ctop-1,Double); ctop--; SETLEFT(ctop,reg); \
    if(2==prev_prev_len && First==GETOP(ctop-2)) \
          {SETOP(ctop-2,Triple);} \
    break; \
  }

#if defined SUPER
#define OCOMPRESS(Simple,Double,First,Triple) \
  COMPRESS(Simple,Double,First,Triple)
#else
#define OCOMPRESS(Simple,Double,First,Triple) \
if(1==prev_len && Simple==GETOP(ctop-1)) \
  {       SETOP(ctop-1,Double); ctop--; SETLEFT(ctop,reg); \
    break; \
  }
#endif

no
insert_op(no opcode, no reg, string name, no arity)
{
    SETOP(ctop, opcode);
    switch (opcode) {
    case CLAUSE:
	{
	    cell pred = input_fun(name, arity);
	    no ok = hdef(g.predmark, pred, ctop);
	    if (currpred != g.true)
		SETLABEL(last, ctop);
	    reg++;
	    if (ok) {
		badcode = FALSE;
		if (currpred != g.true && GETOP(ctop - 2) != END) {
		    switch (GETOP(last)) {
		    case TRY_ME_ELSE:	/* begin of single cls */
			{
			    instr p;
			    for (p = last - 2; p <= ctop - 4; p++)
				*p = *(p + 4);
			}
			SETOP(ctop - 4, END);
			SETOP(ctop - 2, END);
			break;

		    case RETRY_ME_ELSE:
			SETOP(last, TRUST_ME_ELSE);
			break;

		    default:
			badcode = TRUE;
			ERREXIT("bad code in backpatching")
		    }
		}
		SKIPBAD();
		currpred = pred;
		SETOP(ctop, SWITCH);
		SETREG(ctop, 0);
		SETFUN(ctop, pred);
		ctop++;

		SETOP(ctop, TRY_ME_ELSE);	/* reg = how far is
						 * fun=nextcls */
		SETREG(ctop, arity);	/* arity is here, not on the stack */
	    }
	    else if (currpred == pred) {
		SKIPBAD();
		SETOP(ctop, RETRY_ME_ELSE);
		SETREG(ctop, arity);
	    }
	    else {
		badcode = TRUE;
		fprintf(stderr, "%s/%u <=", name, arity);
		ERREXIT("predicate leads other group of clauses: ignored")
	    }
	    last = ctop++;
	}
	break;

    case FIRSTARG:			/* MaxReg-FunFirstarg/Arity */
	SKIPBAD();
	if (reg >= MAXREG)
	    ERREXIT("not enough registers") {
	    no funval = (no) input_fun(name, arity);
	    if ('_' == *name || !hdef(currpred, funval, ctop)) {
		instr label = (instr) hget(g.predmark, currpred);
		SETOP(label, NONDET);
	    }
	    }
	ctop--;				/* null effect, as we do ctop++ later */
	break;

    case EXECUTE:
	SKIPBAD();
	SETFUN(ctop, input_fun(name, arity));
	break;

    case PUT_VARIABLE:
    case GET_VALUE:
	SKIPBAD();
	SETREG(ctop, reg);
	SETLEFT(ctop, arity);
	break;

    case GET_STRUCTURE:
    case PUT_STRUCTURE:
	SKIPBAD();
	SETREG(ctop, reg);
	SETFUN(ctop, input_fun(name, arity));
	break;

    case UNIFY_VARIABLE:
	SKIPBAD();
	OCOMPRESS(UNIFY_VALUE, UNIFY_VAL_VAR, GET_STRUCTURE,
		  GET_UNIFY_VAL_VAR)
	    OCOMPRESS(UNIFY_VARIABLE, UNIFY_VAR_VAR, GET_STRUCTURE,
		      GET_UNIFY_VAR_VAR)
	    SETREG(ctop, reg);
	break;

    case UNIFY_VALUE:
	SKIPBAD();
	OCOMPRESS(UNIFY_VALUE, UNIFY_VAL_VAL, GET_STRUCTURE,
		  GET_UNIFY_VAL_VAL)
	    OCOMPRESS(UNIFY_VARIABLE, UNIFY_VAR_VAL, GET_STRUCTURE,
		      GET_UNIFY_VAR_VAL)
	    SETREG(ctop, reg);
	break;

    case WRITE_VARIABLE:
	SKIPBAD();
	OCOMPRESS(WRITE_VALUE, WRITE_VAL_VAR, PUT_STRUCTURE,
		  PUT_WRITE_VAL_VAR)
	    OCOMPRESS(WRITE_VARIABLE, WRITE_VAR_VAR, PUT_STRUCTURE,
		      PUT_WRITE_VAR_VAR)
	    SETREG(ctop, reg);
	break;

    case WRITE_VALUE:
	SKIPBAD();
	OCOMPRESS(WRITE_VALUE, WRITE_VAL_VAL, PUT_STRUCTURE,
		  PUT_WRITE_VAL_VAL)
	    OCOMPRESS(WRITE_VARIABLE, WRITE_VAR_VAL, PUT_STRUCTURE,
		      PUT_WRITE_VAR_VAL)
	    SETREG(ctop, reg);
	break;

    case MOVE_REG:
	SKIPBAD();
	if (1 == prev_len && MOVE_REG == GETOP(ctop - 1) &&
	    !(1 == prev_prev_len && MOVE_REGx2 == GETOP(ctop - 2))
	    )
	    SETOP(ctop - 1, MOVE_REGx2);
	SETREG(ctop, reg);
	SETLEFT(ctop, arity);
	break;

    case LOAD_VALUE:
	SKIPBAD();
	if (1 == prev_len && LOAD_VALUE == GETOP(ctop - 1)
	    && 1 == GETREG(ctop - 1) && 2 == reg) {
	    SETOP(ctop - 1, LOAD_VALUEx2);
	    SETREG(ctop - 1, arity);
	    ctop--;
	    break;
	/* An = REG = second,  Ai = LEFT = first */
	}
	SETREG(ctop, reg);
	SETLEFT(ctop, arity);
	break;

    case LOAD_CONSTANT:
	{
	    cell small;
	    if (1 == prev_len && LOAD_VALUE == GETOP(ctop - 1)
		&& 1 == GETREG(ctop - 1) && 2 == reg
		&& ((small = input_fun(name, arity)), INTEGER(small))
		&& (no) OUTPUT_INT(small) < (no) 128) {
		SETOP(ctop - 1, opcode = LOAD_VAL_SHORT);
		SETREG(ctop - 1, small);
		ctop--;
		break;
	    }
	}
	SETREG(ctop, reg);
	SETFUN(ctop, input_fun(name, arity));
	break;

    case GET_CONSTANT:
    case PUT_CONSTANT:
    case UNIFY_CONSTANT:
    case WRITE_CONSTANT:
	SKIPBAD();
	SETREG(ctop, reg);
	SETFUN(ctop, input_fun(name, arity));
	break;

    case END:
	SKIPBAD();
	SETREG(ctop, reg);
	ctop++;
	badcode = TRUE;
	if (!link_code(bak_ctop))
	    return FALSE;
	break;

    case ARITH:
	SKIPBAD();
	SETOP(ctop, (GETOP(ctop) + arity));
	if (reg) {
	    SETREG(ctop, reg);
	    SETLEFT(ctop, *name - '0');
	}
	break;

    case INLINE:
    case BUILTIN:
	SKIPBAD();
	SETREG(ctop, reg);
	SETOP(ctop, (GETOP(ctop) + arity));
	break;

    default:
	SKIPBAD();
	SETREG(ctop, reg);
    }
    CHECK(ctop, CodeStk, "code overflow");
    prev_prev_len = prev_len;
    prev_len = instr_len[opcode];
    ctop++;
    return TRUE;
}

no
init_code(void)
{
    no ok;
    SETOP(ctop, END);
    ctop += instr_len[END];
    currpred = g.true;
    badcode = TRUE;
    prev_prev_len = prev_len = 2;
    SETOP(ctop, PROCEED);
    ok = hdef(g.predmark, g.true, ctop);
    SETFUN(ctop, g.true);
    ctop++;
    return ok;
}

#define GET_BYTE(a_byte) if(EOF==(a_byte=getc(f))) break
#define GET_INSTR() \
  int i;          \
      GET_BYTE(opcode); GET_BYTE(reg); GET_BYTE(arity); \
      for(i=0; i<max.SBUF; i++) \
        { GET_BYTE(sbuf[i]);  \
          if(!sbuf[i]) break; \
        }                     \
      sbuf[i]='\0'

void
skip_header(FILE *f)
{
    no b, c = getc(f);
    if ('#' != c) {
	ungetc(c, f);
	return;
    }
    else
	b = c;
    while (TRUE) {
	b = c;
	GET_BYTE(c);
	if ('$' == b && '0' == c) {
	    GET_BYTE(c);
	    break;
	}
    }
}

no
load(string fname)
{
    FILE *f;
    no opcode, reg, arity;

    f = fopen(fname, "rb");
    if (!f)
	ERREXIT("file not found")
	    skip_header(f);

    while (TRUE) {
	GET_INSTR();
	if (!insert_op(opcode, reg, sbuf, arity))
	    return FALSE;
    }
    fclose(f);
    return TRUE;
}
