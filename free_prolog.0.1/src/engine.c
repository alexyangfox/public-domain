/*
 * binprolog binWAM engine
 */
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "global.h"

#define NEWVAR2(R1,R2) SETREF(H,H); (R1)=(R2)=(cell)(H++)
#define NEWVAR(R) SETREF(H,H); (R)=(cell)(H++)

extern struct specsyms g;
extern cell compare_vals[];
extern struct stack wam[];
extern byte timestamp,*hstamp,*atomstamp;
extern struct limit max;

extern string lextable,newlex;
extern no newatom;
extern string sbuf,stop;
extern no hcount;

term *
unwind_trail(term * new, term * old)
{
    while (old < new) {
	new--;
	SETREF(*new, *new);
    }
    return new;
}

/* this is an optional operation saving lot of trail space,
but it can cost a small amount of extra time */

term *
tidy_trail(term * from, term * to, term hb)
{
    while (from < to)
	if (*from < hb)
	    from++;
	else
	    *from = *(--to);
    return to;
}

#define GETARG() S
#define NEXTARG() S+=sizeof(cell)
#define READARG(R) (R)=GETARG();NEXTARG()
#define READVAL(R) UNIFAIL((R),GETARG()); NEXTARG()

/* WAM macros */

#define COPY_CELLS(to,from,Arity) \
{ no k; \
  for(k=(Arity)<<2; k; k-=4) \
    SETREF((term)((no)(to)+k),GETREF((term)((no)(from)+k))); \
}

#define MAKE_CHOICE_POINT(bp,Arity) \
  COPY_CELLS(A,regs,Arity);         \
  A+=Arity+3;                      \
  SAVED_H=H;                       \
  SAVED_TR=TR_TOP;                 \
  SAVED_P=(instr)(bp)

#define RESTORE_STATE(Arity)    \
  TR_TOP=unwind_trail(TR_TOP,SAVED_TR);  \
  H=SAVED_H;  \
  A-=Arity+3; \
  COPY_CELLS(regs,A,Arity); \
  cutB=(cell)A

#define FAILURE() {P=SAVED_P; continue;}
/* fprintf(stderr,
"A=%u: FAILING TO => code[%u]\n",
A-wam[ChoiceStk].base,SAVED_P-cbase); */

#define UNIFAIL(Term1,Term2) if(!unify((Term1),(Term2),wam,A)) FAILURE()

#define WARNING(errname) {warnmes(errname); FAILURE()}

#define WARFUN(Sym,Msg) {local_error(Sym,Msg); FAILURE()}

#define OUT(Expr) \
if(LEFTFIELD) \
  {UNIFAIL((Expr),An)} \
else \
  An=(cell)(Expr); \
NEXT(1)

#define PUT_ARG(I,Val) UNIFAIL(Val,regs[I])
#define PUT_INT_ARG(I,Val) PUT_ARG((I),INPUT_INT(Val))

#define NEXT(Isize) P+=Isize; continue
#define CONT(Cont) regs[1]=regs[Cont]; P++; continue
#define GETFIELDS(Step) (fields=((term)P)[Step])
#define FLOW(Isize) {P+=Isize; GETFIELDS(0);}
#define EMPTY

#define INTS() \
ires=(int)X(1); xval=X(2); \
if(! ( INTEGER((cell)ires) && INTEGER(xval) ) ) WARNING("integer expected")

#define COMPUTE(Op) \
INTS(); \
ires=INPUT_INT(OUTPUT_INT( ires ) Op OUTPUT_INT(  (int)xval )); \
OUT(ires)

#define MUST_BE(Relop) \
INTS(); \
if(! ( ires Relop (int)xval ) ) FAILURE() \
NEXT(1)

#define HEAP_MARK wam[TrailStk].end
#define HEAP_BASE (wam[TrailStk].end+1)

#define PUSH_HEAP_MARKS() HEAP_MARK-=2
#define POP_HEAP_MARKS() HEAP_MARK+=2

#define HEAP_MIDDLE() (*HEAP_BASE+(((term)wam[HeapStk].end-*HEAP_BASE)>>2))

#define START_INTERP() \
  A=wam[ChoiceStk].base;          \
  TR_TOP=wam[TrailStk].base;      \
  S=0; H=(term)wam[HeapStk].base; \
  P=start_point();                \
  NEWVAR(regs[1]);                \
  regs[2]=g.true;                 \
  cutB=(cell)cbase;               \
  MAKE_CHOICE_POINT(cutB,2)

static string bak_newlex;
static long rtime;

void
save_orig(void)
{
    timestamp = RUNTIME;
    bak_ctop = ctop;
    bak_newlex = newlex;
    rtime = cputime();
}

void
restart_orig(void)
{
    ctop = bak_ctop;
    newlex = bak_newlex;
    hbak(LOADTIME);
    atombak(LOADTIME);
    wam[BoardStk].top = wam[BoardStk].base;
}

instr
start_point(void)
{
    instr p = (instr) hget(g.predmark, g.prolog_main);
    if (!p)
	fatal_error("no definition for: main/1");
    return p;
}

#define PUSH(top,a) (*++(top)=(cell)(a))
#define POP(top) (*(top)--)
#define NOTEMPTY(top,base) ((top)>(term)(base))
#define BIND(Var,Ref,Val) \
{SETREF(Var,COMPOUND(Val)?(cell)Ref:Val); TRAIL_IF(Var);}

no
unify(cell v1, cell v2, stack wam, term * A)
{
    term U = (term) A;			/* top of push down list */

    PUSH(U, v1);
    PUSH(U, v2);

    while (NOTEMPTY(U, A)) {
	term t1 = U--, t2 = U--;
	DEREF2(t1, v1);
	DEREF2(t2, v2);
	if (t1 != t2) {
	    if (VAR(v1)) {		/* unb. var. v1 */
		if (VAR(v2) && v2 > v1) {	/* unb. var. v2 */
		    SETREF((term) v2, v1);
		    TRAIL_IF((term) v2);
		}
		else
		    BIND((term) v1, t2, v2)
	    }
	    else if (VAR(v2))		/* v1 is NONVAR */
		BIND((term) v2, t1, v1)
	    else if (v1 != v2)		/* both are NONVAR */
		return FALSE;
	    else if (IDENTIFIER(v1) && (v1 = GETARITY(v1))) {	/* they have the same
								 * FUNCTOR, v1==v2 */
		CHECK(U, ChoiceStk, "unification overflow");

		PUSH(U, t1 + v1);
		PUSH(U, t2 + v1);
		while (--v1) {
		    PUSH(U, t1[v1]);
		    PUSH(U, t2[v1]);
		}
	    }
	}
    }
    return TRUE;
}

#define UNITEST(T1,T2) if(!unify(T1,T2,wam,A)) return NULL

term
functor(term H, term t, term f, term n, stack wam, term * A)
{
    cell r, arity;
    DEREF2(t, r);
    if (NONVAR(r)) {
	if (INTEGER(r) || !(arity = GETARITY(r))) {
	    UNITEST(f, r);
	    UNITEST(n, INPUT_INT(0));
	}
	else {
	    UNITEST(f, PUTARITY(r, 0));
	    UNITEST(n, INPUT_INT(arity));
	}
    }
    else {				/* t is a variable */
	cell i;
	DEREF2(f, r);
	DEREF2(n, i);
	if (!INTEGER(i))
	    return NULL;
	arity = OUTPUT_INT(i);
	if (INTEGER(r)) {
	    if (0 != arity)
		return NULL;
	    UNITEST(t, r);
	}
	else if (NONVAR(r)) {
	    term s = H;
	    if (arity >= MAXARITY)
		ERREXIT("functor has bad arity")
		    PUSHVAL(PUTARITY(r, arity));
	    while (arity--) {
		SETREF(H, H);
		H++;
	    }
	    UNITEST(t, s);
	}
	else
	    return NULL;
    }
    return H;
}

term
name2list(term H, term t, term l, stack wam, term * A)
{
    cell vt, vl;
    DEREF2(t, vt);
    DEREF2(l, vl);

    if (NONVAR(vl) && list2buf(l, vl)) {
	UNITEST(t, fun_name(sbuf, 0));
    }
    else if (NONVAR(vt) && name2buf(vt)) {
	term r = H;
	string name = sbuf;
	while (*name)
	    PUSH_LIST(INPUT_INT(*name++))
		PUSH_NIL();
	UNITEST(r, vl);
    }
    else if (VAR(vt) && VAR(vl))
	ERREXIT("both args of name cannot be variables")
    else
	ERREXIT("bad data in args of name/2")
    return H;
}

#define STAT(Top,StackNo) \
  PUT_INT_ARG(1,stat_used(Top,StackNo)) \
  PUT_INT_ARG(2,stat_left(Top,StackNo)) \
CONT(3)

#define CUT_AND_CHECK_HEAP() cutB=(cell)A; CHECK(H,HeapStk,"heap overflow")

#define EXEC() P=GETLABEL(P); CUT_AND_CHECK_HEAP()

#define GSTR(Advance,Rm1,Rm2,Wm1,Wm2,Steps) \
      FDEREF(An); \
      if(VAR(xval)) \
        { S=0; \
          SETREF(H,GETFUN(P)); \
          SETREF(xref,H++); \
          TRAIL_IF(xref); \
          Advance;Wm1;Wm2; \
          NEXT(Steps); \
        } \
      if(xval!=GETFUN(P)) FAILURE() \
      S=(cell)(xref+1);Advance;Rm1;Rm2;NEXT(Steps)

#define PSTR(Action) \
      SETREF(H,GETFUN(P)); \
      An=(cell)(H++); \
      Action

/* WAM-code interpreter */

  /********************
  CHOICE POINT:

   /\ <-----------* A register
   || P
   || TR
   || H
   || regs(1..arity)
   .....
  *********************/

/*****************************************************************
  initially P points to the first put_structure in the last clause
  that is INTERPRETED AS A GOAL. The clause must be of the form:

  answer:-pred(_).

  where 'pred' is a unary predicate defined previously
******************************************************************/

no
interp (void) /*wam)
  stack wam; */
{
  cell RegisterBuffer[TEMPARGS+MAXREG];
  term regs=RegisterBuffer+TEMPARGS;
  cell S;
  term H;
  instr P;
  term *A;
  cell cutB;

  save_orig();
  START_INTERP();

  for(;;)
  {
      term xref; cell xval,fields;
      int ires;

#ifdef PROF
      profiler(1,P,A,H);
#endif

  switch(GETOPCELL(GETFIELDS(0)))
  {
    case END:
    return TRUE;

    case UNIFY_VARIABLE:
      if(S)
    {
      READARG(An);
      NEXT(1);
    }
    case WRITE_VARIABLE:
      NEWVAR(An);
    NEXT(1);

    case UNIFY_VALUE:
      if(S)
        {
          READVAL(An);
          NEXT(1);
        }
    case WRITE_VALUE:
      PUSHVAL(An);
    NEXT(1);

    case GET_STRUCTURE: GSTR(EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,2);

    case PUT_STRUCTURE: PSTR(NEXT(2));

    case MOVE_REG:
      An=Ai;
    NEXT(1);

    case MOVE_REGx2:
      An=Ai;
      GETFIELDS(1);
      An=Ai;
    NEXT(2);

    case PUT_VARIABLE:
      NEWVAR2(An,Ai);
    NEXT(1);

    case GET_VALUE:
      UNIFAIL(An,Ai)
    NEXT(1);

    case UNIFY_VAR_VAR:
    if(S)
    {
      READARG(An);
      READARG(Ai);
      NEXT(1);
    }
    case WRITE_VAR_VAR:
      NEWVAR(An);
      NEWVAR(Ai);
    NEXT(1);

    case UNIFY_VAL_VAR:
    if(S)
      {
        READVAL(An);
        READARG(Ai);
        NEXT(1);
      }
    case WRITE_VAL_VAR:
      PUSHVAL(An);
      NEWVAR(Ai);
    NEXT(1);

    case UNIFY_VAL_VAL:
    if(S)
      {
        READVAL(An);
        READVAL(Ai);
        NEXT(1);
      }
    case WRITE_VAL_VAL:
      PUSHVAL(An);
      PUSHVAL(Ai);
    NEXT(1);

    case UNIFY_VAR_VAL:
      if(S)
    {
      READARG(An);
      READVAL(Ai);
      NEXT(1);
    }
    case WRITE_VAR_VAL:
      NEWVAR(An);
      PUSHVAL(Ai);
    NEXT(1);

    case UNIFY_CONSTANT:
    if(S)
    { xval=GETREF((term)GETARG()); NEXTARG();
      FDEREF(xval);
      if(VAR(xval))
        {
          SETREF(xref,GETFUN(P));
          TRAIL_IF(xref);
        }
      else if(xval!=GETFUN(P))FAILURE()
      NEXT(2);
    }
    case WRITE_CONSTANT:
       PUSHVAL(GETFUN(P));
    NEXT(2);

    case GET_CONSTANT:
      FDEREF(An); /* now xref is a register address */
      if(VAR(xval))
        {
          SETREF(xref, GETFUN(P)); /*fill the unb. VAR with a const*/
          TRAIL_IF(xref);
        }
      else if(xval!=GETFUN(P))
        FAILURE()
    NEXT(2);

    case PUT_CONSTANT:
    An=GETFUN(P);
    NEXT(2);

    case PUSH_CUT:
      PUSHVAL(INPUT_INT(cutB-(cell)wam[ChoiceStk].base));
    NEXT(1);

    case PUT_CUT:
      A=(term *)cutB;
      TR_TOP=tidy_trail(SAVED_TR,TR_TOP,SAVED_H);
    NEXT(1);

    case GET_CUT:
      FDEREF(regs[1]);
      A=(term *)(OUTPUT_INT(xval)+(cell)wam[ChoiceStk].base);
      TR_TOP=tidy_trail(SAVED_TR,TR_TOP,SAVED_H);
    NEXT(1);

    case EXECUTE:
      EXEC();
    continue;

    case PROCEED:
      answer(A);
#if defined DEBUG
      show_regions(A,H);
#endif
    FAILURE()

    case EXEC_JUMP_IF: EXEC();
    case JUMP_IF:
      FDEREF(regs[1]);
      if(VAR(xval)) {NEXT(2);}
        { instr label=GETLABEL(P);
          if(xval!=GETFUN(label)) {NEXT(4);}
          S=(cell)(xref+1);
#if defined SUPER
          fields=*(term)(label+2);
          READARG(An); READARG(Ai);
          P=label+3;
#else
          P=label+2;
#endif
          continue;
        }

#if defined SUPER
    case GET_UNIFY_VAL_VAR:
     GSTR(GETFIELDS(2),READVAL(An),READARG(Ai),PUSHVAL(An),NEWVAR(Ai),3);

    case GET_UNIFY_VAL_VAL:
      GSTR(GETFIELDS(2),READVAL(An),READVAL(Ai),PUSHVAL(An),PUSHVAL(Ai),3);

    case GET_UNIFY_VAR_VAR:
      GSTR(GETFIELDS(2),READARG(An),READARG(Ai),NEWVAR(An),NEWVAR(Ai),3);

    case GET_UNIFY_VAR_VAL:
      GSTR(GETFIELDS(2),READARG(An),READVAL(Ai),NEWVAR(An),PUSHVAL(Ai),3);

    case PUT_WRITE_VAR_VAR: PSTR(FLOW(2));
      NEWVAR(An);
      NEWVAR(Ai);
    NEXT(1);

    case PUT_WRITE_VAL_VAR: PSTR(FLOW(2));
      PUSHVAL(An);
      NEWVAR(Ai);
    NEXT(1);

    case PUT_WRITE_VAL_VAL: PSTR(FLOW(2));
      PUSHVAL(An);
      PUSHVAL(Ai);
    NEXT(1);

    case PUT_WRITE_VAR_VAL: PSTR(FLOW(2));
      NEWVAR(An);
      PUSHVAL(Ai);
    NEXT(1);
#endif

    case EXEC_TRY: EXEC(); GETFIELDS(0);
    case TRY_ME_ELSE:
      xval=REGFIELD;
      CHECK(A,ChoiceStk,"local stack overflow");
      MAKE_CHOICE_POINT(GETLABEL(P),xval);
    NEXT(2);

    case RETRY_ME_ELSE:
      xval=REGFIELD;
      SAVED_P=GETLABEL(P);  /* also: SAVED_P=GETLABEL(SAVED_P); */
      RESTORE_STATE(xval);
      A+=xval+3;
    NEXT(2);

    case TRUST_ME_ELSE:
    xval=REGFIELD;
      RESTORE_STATE(xval);
    NEXT(2);

          case TRY_ME_ONLY: /* nop */
          case NONDET:
          NEXT(2);

          case EXEC_SWITCH: EXEC();
          case SWITCH:
            FDEREF(regs[1]);
            if(VAR(xval)) {NEXT(2);}
            if(!(P=(instr)hget(GETFUN(P),xval))) FAILURE()
            S=(cell)(xref+1);
          NEXT(2);

          case LOAD_CONSTANT:
            xval=REGFIELD; X(xval)=GETFUN(P);
          NEXT(2);

          case LOAD_VAL_SHORT:
            xval=REGFIELD; X(2)=xval;
            IN(1,Ai);
          NEXT(1);

          case LOAD_VALUEx2:
            IN(1,Ai);
            IN(2,An);
          NEXT(1);

          case LOAD_VALUE:
            ires=REGFIELD;
            IN(ires,Ai);
          NEXT(1);

   /* INLINE_PREDS */

          case FAIL_0:
            FAILURE()

          case CWRITE_1:
            /* don't DEREF !!! ; regs may also be An */
            fout(regs[0],A);
          NEXT(1);

          case NL_0:
            NEWLN();
          NEXT(1);

          case VAR_1:
            FDEREF(regs[0]);
            if(NONVAR(xval)) FAILURE()
          NEXT(1);

          case NONVAR_1:
            FDEREF(regs[0]);
            if(VAR(xval)) FAILURE()
          NEXT(1);

          case INTEGER_1:
            FDEREF(regs[0]);
            if(INTEGER(xval))
              {NEXT(1);}
            else
              FAILURE()

          case ATOMIC_1:
            FDEREF(regs[0]);
            if(ATOMIC(xval))
              {NEXT(1);}
            else
              FAILURE()

          case IS_COMPILED_1:
            FDEREF(regs[0]);
            xval=PUTARITY(xval,1+GETARITY(xval));
            if(hget(g.predmark,xval))
              {NEXT(1);}
            else
              FAILURE()

          case LIFT_HEAP_0:
            PUSH_HEAP_MARKS();
            *HEAP_BASE = *HEAP_MARK = H; /*loH*/
            H=HEAP_MIDDLE();
          NEXT(1);

                  case SEEN_0:
                    seen();
                  NEXT(1);

                  case TOLD_0:
                    told();
                  NEXT(1);


          /* INLINE ARITH */

          case PLUS_3: COMPUTE(+);
          case SUB_3:  COMPUTE(-);
          case MUL_3:  COMPUTE(*);
          case DIV_3:
            if(0==OUTPUT_INT(X(2))) WARFUN(X(1),"divised by 0");
            COMPUTE(/);
          case MOD_3: COMPUTE(%);
          case RANDOM_1: OUT(INPUT_INT(((int)((no)rand()>>2))));

          case GET0_1:
            if(feof(g.seefile))
            {  clearerr(g.seefile);
               /* warnmes("Read past EOF"); */ ires=0;
            }
            else
              ires=getc(g.seefile);
            OUT(INPUT_INT(ires));

          case PUT0_1:
            xval=X(1);
            if(!INTEGER(xval)) WARFUN(xval,"integer expected in put/1");
            putc(OUTPUT_INT(xval),g.tellfile);
          NEXT(1);

                  case LESS_2: MUST_BE(<);
                  case GREATER_2: MUST_BE(>);
                  case LESS_EQ_2: MUST_BE(<=);
                  case GREATER_EQ_2: MUST_BE(>=);
                  case ARITH_EQ_2: MUST_BE(==);
                  case ARITH_DIF_2: MUST_BE(!=);

                  case LSHIFT_3:COMPUTE(<<);
                  case RSHIFT_3:COMPUTE(>>);
                  case L_AND_3:COMPUTE(&);
                  case L_OR_3:COMPUTE(|);
                  case L_XOR_3:COMPUTE(^);
                  case L_NEG_3:OUT(INPUT_INT(~OUTPUT_INT(X(2))));

                  case COMPARE0_3:
                    ires=compare(&X(1),&X(2));
                    xval=compare_vals[ires+1];
                  OUT(xval);

                  case ARG_3:
                      xval=X(1);
                      if(!INTEGER(xval))
                        WARFUN(xval,"arg/3's 1st arg must be integer");
                      ires=OUTPUT_INT(xval);

                      xref=RX(2);
                      if(ATOMIC((cell)xref)) FAILURE()
                      xval=GETREF(xref);
                      if(VAR(xval))
                        WARFUN(xval,"arg/3's 2nd arg must be nonvar");
                      if(ires<=0 || ires>GETARITY(xval))
                        WARFUN(xval,"arg/3's 1st arg must be in 1..arity");
                      xref+=ires;
                    OUT(xref);

                  case DEF_3:
                    if(!hash_args3(regs) || !hash_op(hdef,regs)) FAILURE()
                  NEXT(1);

                  case SET_3:
                    if(!hash_args3(regs) || !hash_op(hset,regs)) FAILURE()
                  NEXT(1);

                  case VAL_3:
                    xval=hget(X(1),X(2)); if(!xval)FAILURE()
                  OUT(xval);

                  case RM_2:
                    if(!hash_args2(regs)) FAILURE()
                    X(3)=g.empty;
                    if(!hash_op(hset,regs)) FAILURE()
                  NEXT(1);

                  case EVAL0_3:
                  case OUT0_3:
                    if(!hash_args3(regs)) FAILURE()
                    xval=hget(X(1),X(2));
                    if(xval && g.empty!=xval) FAILURE()
                    else if(!xval) xval=hash_op(hdef,regs);
                    else xval=hash_op(hset,regs);
                    if(!xval) FAILURE()
                  NEXT(1);
                  case RD0_3:
                    xval=hget(X(1),X(2));
                    if(!xval || g.empty==xval) FAILURE()
                  OUT(xval);

                  case IN0_3:
                    if(!hash_args2(regs)) FAILURE()
                    X(4)=hget(X(1),X(2));
                    if(!X(4) || g.empty==X(4)) FAILURE()
                    X(3)=g.empty;
                    if(!hash_op(hset,regs)) FAILURE()
                  OUT(X(4));

                  case SYMCAT_3:
                    xval=symcat(X(1),X(2)); if(!xval)FAILURE()
                  OUT(xval);

                  case COPY_TERM_2: xval=X(1);
                    if(!ATOMIC(xval))
                      {
                        xref=copy_term(H,(term)xval,A);
                        xval=(cell)H;
                        H=xref;
                      }
                  OUT(xval);

                  case SAVE_TERM_2: xval=X(1);
                  if(!ATOMIC(xval))
                    {
                      xval=(cell)wam[BoardStk].top;
                      if(!(xref=save_term((term)xval,RX(1),A))) FAILURE()
                      wam[BoardStk].top=(term*)xref;
                    }
                  OUT(xval);

                  case SREAD_2:
                    xval=X(1);
                    if(!SYMCONST(xval)) FAILURE()
                    xval=sread(H,xval);
                    if(!xval) FAILURE()
                    H=(term)(xval)+1;
                  OUT(xval);

                  case SWRITE_2:
                    xval=sout(RX(1),A);
                  OUT(xval);

                  case SEEING_1:
                    OUT(g.seefunc);

                  case TELLING_1:
                    OUT(g.tellfunc);

                  case SEE_OR_FAIL_1:
                    if(!see(X(1))) FAILURE()
                  NEXT(1);

                  case TELL_OR_FAIL_1:
                    if(!tell_to(X(1))) FAILURE()
                  NEXT(1);

                  case ADD_INSTR_4:
                    if(!add_instr(regs)) FAILURE()
                  NEXT(1);

                  case DET_APPEND_3:
                    xref=H;
                    if(!(H=det_append(H,regs))) FAILURE()
                  OUT(xref);


              /* OLD SPECIAL BUILTINS */

                  case DEMO_0:
                    CUT_AND_CHECK_HEAP();
                    FDEREF(regs[1]);
                    P=(instr) hget(g.predmark,xval);
                    if(!P)
                      WARFUN(xval,"undefined predicate")
                    xval=GETARITY(xval);
                    COPY_CELLS(regs,xref,xval);
                  continue;

                  case DEMO_1:
                    CUT_AND_CHECK_HEAP();
                    FDEREF(regs[1]);
                    fields=GETARITY(xval);
                    P=(instr) hget(g.predmark,PUTARITY(xval,++fields));
                    if(!P)
                      WARFUN(xval,"metacall to unknown predicate")
                    SETREF(regs+fields--,regs[2]);
                    COPY_CELLS(regs,xref,fields);
                  continue;

                  /*
                    The heap will look like this:
                    |DOTSYM|->1|->2|1:answer...|2:newH|
                  */
                  case FINDALL_STORE_HEAP_1:
                    H=*HEAP_MARK; /* loH */
                    CHECK(H,HeapStk,
                      "heap overflow in findall_store_heap");
                    MAKE_LIST();
                    xref=H++; SETREF(xref,xref+2); xref=H++;
                    (term)wam[HeapStk].margin=HEAP_MIDDLE()-8;
                    H=copy_term(H,regs+1,A);
                    wam[HeapStk].margin=wam[HeapStk].end;
                    SETREF(xref,H);
                    *HEAP_MARK=H; /* new loH */
                  FAILURE()

                 case FINDALL_LOAD_HEAP_1:
                   H=xref=*HEAP_MARK;
                   PUSHVAL(xref);
                   MAKE_LIST();
                   PUSHVAL(xref++);
                   PUSHVAL( *HEAP_BASE);
                   POP_HEAP_MARKS();
                   UNIFAIL(regs[1],xref)
                 CONT(2);

                  case FUNCTOR_3:
                    xref=functor(H,regs+1,regs+2,regs+3,wam,A);
                    if(!xref) FAILURE()
                    H=xref;
                  CONT(4);

                  case NAME_2:
                    xref=name2list(H,regs+1,regs+2,wam,A);
                    if(!xref) FAILURE()
                    H=xref;
                  CONT(3);

                  case ABORT_0:
                    START_INTERP();
                  continue;

                  case RESTART_0: /* can be used only from system zone*/
                    restart_orig();
                  NEXT(1);

                  case SHELL_1: shell_1(regs);
                  CONT(2);

                  case RUNTIME_2:
                  { long t=cputime();
                    PUT_INT_ARG(1,t)
                    PUT_INT_ARG(2,(t-rtime))
                    rtime=t;
                  }
                  CONT(3);

                  case GLOBAL_STACK_2:
                    STAT(H,HeapStk);

                  case LOCAL_STACK_2:
                    STAT(A,ChoiceStk);

                  case TRAIL_2:
                    STAT(TR_TOP,TrailStk);

                  case CODE_2:
                    STAT(ctop,CodeStk);

                  case STRINGS_2:
                    PUT_INT_ARG(1,(newlex-lextable))
                    PUT_INT_ARG(2,(lextable+MAXLEX)-newlex)
                  CONT(3);

                  case SYMBOLS_2:
                    PUT_INT_ARG(1,newatom*sizeof(string))
                    PUT_INT_ARG(2,(MAXATOM-newatom)*sizeof(string))
                  CONT(3);

                  case HTABLE_2:
                    PUT_INT_ARG(1,(hcount+1)*sizeof(struct hentry))
                    PUT_INT_ARG(2,(HMAX-(hcount+1))*sizeof(struct hentry))
                  CONT(3);

                  case LIST_ASM_3:
#ifdef PROF
                    if(!list_asm(regs[1],regs[2],regs[3])) FAILURE()
#endif
                  CONT(4);

                  case BBOARD_2:
                    STAT(wam[BoardStk].top,BoardStk);

                  case BB_LIST_1:
                    H=hlist((xref=H),BBOARDTIME);
                    PUT_ARG(1,xref);
                  CONT(2);

                  case BB_RESET_0:
                    wam[BoardStk].top=wam[BoardStk].base;
                  CONT(1);

                  case PROFILE_0:
#ifdef PROF
                    profiler(2,P,A,H);
#endif
                  CONT(1);

    default:
      fprintf(stderr,"*** bad instruction: [%u] ***\n",GETOP(P));
    return FALSE;
  }
}
}
