#define SUPER

typedef char *string;
typedef unsigned char byte;
typedef unsigned int no;
typedef no cell;
typedef cell *term,*instr;

/* SIZEs of various DATA AREAS: can be replaced with
variables as they will be dynamically allocated */

#define HMAX (1<<13)
#define MAXATOM (HMAX>>1)
#define MAXLEX 8*MAXATOM

#define VARTAG PUTTAG(0,0)
#define FUNTAG PUTTAG(0,2)
#define INTTAG PUTTAG(0,1)

#define TRUE 1
#define FALSE 0

typedef struct limit
{
  int HEAP,CHOICE,TRAIL,CODE,BOARD,SBUF;
}
*limit;

/* RESTRICTED SET OF WAM INSTRUCTIONS
an easy optimisation is to specialize them for lists*/

#define MAXOP 256

#define END 0

#define UNIFY_VARIABLE 1
#define WRITE_VARIABLE 2

#define UNIFY_VALUE 3
#define WRITE_VALUE 4

#define UNIFY_CONSTANT 5
#define WRITE_CONSTANT 6

#define GET_CONSTANT 7
#define GET_STRUCTURE 8

#define PUT_CONSTANT 9
#define PUT_STRUCTURE 10

#define MOVE_REG 11

#define PUT_VARIABLE 12
#define GET_VALUE 13

#define PUSH_CUT 14
#define PUT_CUT 15
#define GET_CUT 16

#define EXECUTE 17
#define PROCEED 18

#define CLAUSE 254
#define FIRSTARG 255

#define TRY_ME_ELSE 19
#define RETRY_ME_ELSE 20
#define TRUST_ME_ELSE 21

#define TRY_ME_ONLY 22
#define NONDET 23

#define EXEC_SWITCH 24
#define SWITCH 25

#define EXEC_JUMP_IF 26 
#define JUMP_IF 27

#define LOAD_CONSTANT 28
#define LOAD_VALUE 29

#define GET_UNIFY_VAR_VAR 30
#define UNIFY_VAR_VAR 31
#define PUT_WRITE_VAR_VAR 32
#define WRITE_VAR_VAR 33

#define GET_UNIFY_VAL_VAL 34
#define UNIFY_VAL_VAL 35
#define PUT_WRITE_VAL_VAL 36
#define WRITE_VAL_VAL 37

#define GET_UNIFY_VAR_VAL 38
#define UNIFY_VAR_VAL 39
#define PUT_WRITE_VAR_VAL 40
#define WRITE_VAR_VAL 41

#define GET_UNIFY_VAL_VAR 42
#define UNIFY_VAL_VAR 43
#define PUT_WRITE_VAL_VAR 44
#define WRITE_VAL_VAR 45
 
#define MOVE_REGx2 46
#define LOAD_VALUEx2 47
#define LOAD_VAL_SHORT 48

#define EXEC_TRY 49
#define NOP LAST_BUILTIN+1

#define CodeStk 0
#define HeapStk 1
#define TrailStk 2
#define ChoiceStk 3
#define BoardStk 4
#define MaxStk 5

typedef struct stack {
    int size;
    term *top;
    term *oldtop;
    term *base;
    term *margin;
    term *end;
    term *maxused;
    string name;
} *stack;

#define cbase ((instr)wam[CodeStk].base)
#define ctop ((instr)wam[CodeStk].top)
#define bak_ctop ((instr)wam[CodeStk].oldtop)
#define codend ((instr)wam[CodeStk].end)

/* GLOBAL REGISTERS */

#define SAVED_H (*(A-2))
#define SAVED_TR ((term *)*(A-1))
#define SAVED_P ((instr)*A)

#define TR_TOP wam[TrailStk].top

/* HASING TABLE ENTRY */

typedef struct hentry
{
  no pred;
  no fun;
  no val;
}
*hentry;

/* DATA AREA FORMATS & INSTRUCTION FORMATS */

#define TAGBITS 2
#define ARITYBITS 8

#define REGBITS 11
#define ARGBITS 11
#define OPBITS (sizeof(no)*8-REGBITS-ARGBITS)

/* if OPBITS > 10 it will be slow on SPARCs */

#define MAXARITY (1<<ARITYBITS)
#define MAXREG (1<<(ARGBITS)-1)

/**********************LOW-LEVEL TERM OPERATIONS*****************/

#define LBITS ARITYBITS
#define MBITS (sizeof(no)*8-LBITS-RBITS)
#define RBITS TAGBITS

#define RMASK (((no)1<<RBITS)-1)
#define LMASK ((no)~0<<(MBITS+RBITS))
#define MMASK (~LMASK&~RMASK)

#define LGET(W) ((no)(W)>>(MBITS+RBITS)) 
#define LPUT(W,Val) ((no)(W)&~LMASK) | (no)(Val)<<(MBITS+RBITS)
#define LSET(W,Val)     ((no)(W)= LPUT(W,Val))

#define MGET(W) ((no)(W)<<LBITS>>(LBITS+RBITS))
#define MPUT(W,Val) (((no)(W)&~MMASK) |((no)(Val)<<(LBITS+RBITS)>>LBITS))
#define MSET(W,Val) ((no)(W)=MPUT(W,Val))

#define RGET(W) ((no)(W)&RMASK)
#define RPUT(W,Val) (((no)(W) & ~RMASK) | (no)(Val)) 
#define RSET(W,Val) ((no)(W)=RPUT(W,Val)) 


/*******************INTERFACE TERM OPERATIONS ******************/

#define SETTAG(W,Val) RSET(W,Val)
#define PUTTAG(W,Val) RPUT(W,Val)
#define GETTAG(W) RGET(W)

#define SETSYMNO(W,Val) MSET(W,Val)
#define PUTSYMNO(W,Val) MPUT(W,Val)
#define GETSYMNO(W) MGET(W)

/* #define SETARITY(W,Val) LSET(W,Val) */
#define PUTARITY(W,Val) LPUT(W,Val)
#define GETARITY(W) LGET(W)

/*************** HIGH-LEVEL TERM OPERATIONS************************/

#define GETREF(t) (*(t))

#define SETREF(t,Ref) (*(t) = (cell)(Ref))

#define NONVARREF(t) (NONVAR(GETREF(t)))

#define NAME(s) (atomtable[GETSYMNO(s)])

#define INPUT_INT(Extval) ((no)((int)(Extval)<<TAGBITS) | INTTAG)
#define OUTPUT_INT(Intval) ((int)(Intval)>>TAGBITS)

#define NONVAR(ACell) GETTAG(ACell)
#define VAR(ACell) (!GETTAG(ACell))
#define INTEGER(ACell) ((ACell) & INTTAG)
#define IDENTIFIER(ACell) ((ACell) & FUNTAG)
#define SYMCONST(ACell) (IDENTIFIER(ACell) && !GETARITY(ACell))
#define COMPOUND(ACell) (IDENTIFIER(ACell) && GETARITY(ACell))
#define SIMPLE(ACell) (!COMPOUND(ACell))

#define ATOMIC(ACell) \
(NONVAR(ACell) && (INTEGER(ACell) || !GETARITY(ACell)))

/*************** LOW-LEVEL INSTRUCTION OPERATIONS *****************/

#define LCBITS REGBITS
#define MCBITS ARGBITS
#define RCBITS OPBITS

#define RCMASK (((no)1<<RCBITS)-1)
#define LCMASK ((no)~0<<(MCBITS+RCBITS))
#define MCMASK (~LCMASK&~RCMASK)

/* Scale must be 0 or 2 */

#define LCGET(W,Scale) ((no)(W)>>(MCBITS+RCBITS+Scale)) 
#define LCPUT(W,Val) ((no)(W)&~LCMASK) | (no)(Val)<<(MCBITS+RCBITS+2)
#define LCSET(W,Val)    ((no)(W)= LCPUT(W,Val))

#define MCGET(W,Scale) ((no)(W)<<(LCBITS)>>(LCBITS+RCBITS+Scale))
#define MCPUT(W,Val) \
  (((no)(W)&~MCMASK)|((no)(Val)<<(LCBITS+RCBITS+2)>>LCBITS))
#define MCSET(W,Val) ((no)(W)=MCPUT(W,Val))

#define RCGET(W) ((no)(W)&RCMASK)
#define RCPUT(W,Val) (((no)(W) & ~RCMASK) | (no)(Val)) 
#define RCSET(W,Val) ((no)(W)=RCPUT(W,Val)) 

/*******INTERFACE INSTRUCTION OPERATIONS ********************/

#define SETOPCELL(W,Val) RCSET(W,Val)
#define PUTOPCELL(W,Val) RCPUT(W,Val)
#define GETOPCELL(W) RCGET(W)

#define SETLEFTCELL(W,Val) MCSET(W,Val)
#define PUTLEFTCELL(W,Val) MCPUT(W,Val)
#define GETLEFTCELL(W) MCGET(W,2)

#define SETREGCELL(W,Val) LCSET(W,Val)
#define PUTREGCELL(W,Val) LCPUT(W,Val)
#define GETREGCELL(W) LCGET(W,2)

/********* HIGH LEVEL INSTRUCTION OPERATIONS*****************/

#define REGFIELD GETREGCELL(fields)
#define LEFTFIELD GETLEFTCELL(fields)
#define OPFIELD GETOPCELL(fields)

#define An (*(term)((no)regs+ LCGET(fields,0) ))
#define Ai (*(term)((no)regs+ MCGET(fields,0) ))

/* cell instr; An, Ai, Op */

#define GETOP(Ip) GETOPCELL(*(instr)(Ip))
#define SETOP(Ip,Val) SETOPCELL(*(instr)(Ip),(Val))

#define GETREG(Ip) GETREGCELL(*(instr)(Ip))
#define SETREG(Ip,Val) SETREGCELL(*(instr)(Ip),(Val))

#define GETLEFT(Ip) GETLEFTCELL(*(instr)(Ip))
#define SETLEFT(Ip,Val) SETLEFTCELL(*(instr)(Ip),(Val))

#define GETFUN(Ip) (*(term)((Ip)+1))
#define SETFUN(Ip,Val) (*(term)(++(Ip)))=(Val)

#define GETLABEL(Ip) (*(instr *)((Ip)+1))
#define SETLABEL(Ip,Val) (*(instr *)((Ip)+1))=(Val)

/* OTHER MACROS */
#define ERREXIT(Mes) {warnmes(Mes); return 0;}
#define NEWLN() fprintf(g.tellfile,"\n")
#define INSTR_LEN(Instr) instr_len[GETOP(Instr)]
#define ON(stk,Ob) \
  wam[stk].base<=(term*)(Ob) && (term*)(Ob)<wam[stk].end

#define MOD(x,y) ((x) & ((y)-1))

#define LOADTIME 0
#define RUNTIME 1
#define BBOARDTIME 2

typedef struct specsyms {
    cell NIL, DOT, DIF, empty, true, prolog_main, predmark, seemark, tellmark, user,
      closed_file, seefunc, tellfunc;
    FILE *seefile, *tellfile;
} *specsyms;

#define IS_LIST(Term) (g.DOT==(Term))

#define PUSHVAL(Val) SETREF(H++,(Val))
#define MAKE_LIST() PUSHVAL(g.DOT)

#define PUSH_LIST(Elem) {  \
  MAKE_LIST();             \
  PUSHVAL((Elem));         \
}

#define PUSH_NIL() PUSHVAL(g.NIL)

#define CHECK(Top,LimitNo,Mess) \
if(((term *)Top) >= wam[LimitNo].margin) fatal_error(Mess)

/*
#define SHORTEN_IT(x) \
if((x)>SAVED_H) \
{       term u,v=(term)GETREF(x); \
  if(VAR(v)&&VAR((u=(term)GETREF(v)))) SETREF((x),u); \
}
*/

#define DEREF2(x,v) \
{ /* SHORTEN_IT(x); */ \
while(VAR((v)=GETREF(x)) && (x)!=(term)(v)) (x)=(term)(v); \
}

#define FDEREF(Expr) \
if(NONVAR(xref=(term)(Expr))) xval=(cell)xref; else DEREF2(xref,xval)

#define TEMPARGS 8
#define X(I) regs[-I]
#define RX(I) ((term) (X(I)))

#define IN(I,Expr) \
if(NONVAR(xref=(term)(Expr))) X(I)=(cell)xref; \
else \
{ DEREF2(xref,xval); \
  X(I)=(COMPOUND(xval))?(cell)xref:xval; \
}

#define TRAIL_IT(V) \
{CHECK(TR_TOP,TrailStk,"trail overflow"); *TR_TOP++=(V);}

#define TRAIL_IF(V) if((V)<SAVED_H) TRAIL_IT(V)

/*
 * protos
 */
/* builtins.c */
term local_error(cell, string);
int ocompare(cell, cell);
int compare(term, term);
void pp(term, term *);
void answer(term *);
no name2buf(cell);
no list2buf(term, cell);
cell symcat(cell, cell);
no see(cell);
void seen(void);
no tell_to(cell);
void told(void);
int stat_used(term *, int);
int stat_left(term *, int);
void shell_1(term);
int add_instr(term);
term det_append(term, term);
term recursive_save_term(term, term, term, term *, term, term, term);
term save_term(term, term, term *);
term copy_term(term, term, term *);
/* debug.c */
string instr_name(no);
void show_tags(void);
string str(no);
term cleanup(term);
string smartref(term);
void hshow(void);
void show_instr(int, instr, FILE *);
void show_wam_instr(int, instr, term *, term);
int show_code(instr, instr);
void show_regions(term *, term);
void profiler(int, instr, term *, term);
void show_atoms(void);
cell list_asm(cell, cell, cell);
/* dict.c */
int hinit(void);
no hdef(no, no, no);
no hset(no, no, no);
no hget(no, no);
void hbak(int);
term hlist(term, int);
int hash_args2(term);
int hash_args3(term);
cell hash_op(cell (*)(no, no, no), term);
/* engine.c */
term *unwind_trail(term *, term *);
term *tidy_trail(term *, term *, term);
void save_orig(void);
void restart_orig(void);
instr start_point(void);
no unify(cell, cell, stack, term *);
term functor(term, term, term, term, stack, term *);
term name2list(term, term, term, stack, term *);
no interp(void);
/* load.c */
void init_instr_len(void);
char get_one_int(char *, int *);
cell fun_name(string, no);
cell input_fun(string, no);
instr link_switch(instr, no);
no link_code(instr);
no insert_op(no, no, string, no);
no init_code(void);
void skip_header(FILE *);
no load(string);
/* read.c */
cell make_funcell(string, string, int);
term make_term(string, string, int);
term cons(term, term);
int match_sym(string *, string *);
int match_var(string *, string *);
int match_int(string *, string *);
int match_char(int);
int match_args(int *, term []);
int match_list_elements(term);
int match_term(term);
cell sread(term, cell);
/* ru.c */
void warnmes(string);
void fatal_error(string);
void make_constants(void);
long cputime(void);
no make_stack(stack, int, int, string);
void init(void);
int toplevel(char *);
char *get_cmd_line_options(int, char *[]);
int main(int, char *[]);
/* sym.c */
no make_symtable(void);
string insert_lex(string);
no skey(string);
no atomno(string);
void atombak(int);
cell new_func(string, no);
/* write.c */
void out(cell, term *);
cell sout(term, int);
void fout(int, int);
