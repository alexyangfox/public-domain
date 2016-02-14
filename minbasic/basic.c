/*************************************
**                                  **
**       MINBASIC Interpreter       **
**                                  **
**            C Version             **
**                                  **
**          NMH, 1991-2011          **
**                                  **
**   Free! Neither mine nor yours.  **
**                                  **
*************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <time.h>
#ifdef unix
 #include <unistd.h> /* isatty() */
#endif

#define byte	unsigned char

#define _BPW_	2

struct _aframe {
	byte	*lp;
	byte	*ip;
};

struct _fframe {
	byte	*lp;
	byte	*ip;
	short	vp;
	short	lim;
	short	stp;
};

struct _pframe {
	short	addr;
	short	val;
};

#define aframe	struct _aframe
#define fframe	struct _fframe
#define pframe	struct _pframe

#define NFIL	10
#define NLIN	2048
#define NVAR	260
#define NSTR	125	/* MAX = 125 */
#define NFUN	26
#define BUFL	125
#define TOKL	8
#define WSPL	32766	/* MAX = 32766 */
#define STKL	4048
#define PADL	5
#define LABL	2048
#define SFLG	0x8000

enum {
	EOK  = 0,
	ELLO, ETLO, EOVF, ESTR, ESYN, EOPR, ELIN, EDIV, ESBO,
	ESUB, EMEM, ENYI, ESTK, ENST, ETYP, EEOD, EARG, EDEV,
	EIOX, EINT, ERNG, ESTO, EBRK, ELAB, EDUP, EFUN, EXXX
};

enum {
	_HASH = 1,
	_QUOTE, _LPAR, _RPAR, _MUL, _PLUS, _COMMA, _MINUS, _SLASH,
	_COLON, _SEMI, _LESS, _EQU, _GRTR, _LSEQ, _NEQ, _GTEQ, _ED, _IF,
	_ON, _OR, _TO, _AND, _DEF, _DEL, _DIM, _END, _FOR, _INS, _LET,
	_NEW, _NOT, _REM, _RUN, _ABS, _ASC, _COPY, _DATA, _ELSE, _EOF,
	_EXIT, _FRE, _GOTO, _LEN, _LIST, _LOAD, _MOVE, _NEXT, _OPEN,
	_POKE, _POS, _READ, _RND, _SAVE, _STEP, _STOP, _THEN, _TRON,
	_VAL, _WEND, _WITH, _CALL, _CHRS, _CLEAR, _CLOSE, _ENDIF, _ERASE,
	_FILES, _GOSUB, _INPUT, _IOCTL, _LABEL, _LOCAL, _MIDS, _PEEK,
	_PRINT, _RENUM, _STRS, _TROFF, _UNTIL, _WHILE, _ENDLC, _RPEAT,
	_RETRN, _SEARC, _SYSTM, _RSTOR
};

#define _INT	254
#define _VAR	253
#define _STR	252
#define _TXT	251
#define _FUN	250
#define _LAB	249

jmp_buf	restart;
int	quiet;

byte	line[BUFL+10];
byte	scratch[BUFL];
int	scp;
byte	strpad[PADL*BUFL];
int	spp;
byte	*tokens, *msgs;
byte	space[WSPL], *memtop;
byte	*progp, *ptop;
byte	*ivars, *svars, *fvars;
byte	*sbufs;
byte	*intp, *linep;
int	tracing;
FILE	*files[NFIL];
char	*fnames[NFIL];
int	idev, odev, iod;
byte	istack[STKL];
int	isp;
int	advance;
byte	*datal, *datap;
int	lpos, epos;
int	retval, retflg;
int	errval;
int	old[NLIN], new[NLIN];
byte	labels[LABL];
int	lbp;
int	runflg;

void putstr(char *s) {
	fprintf(files[odev], "%s", s);
}

void putch(int c) {
	putc(c, files[odev]);
}

void putd(int n, int pad) {
	fprintf(files[odev], "%*d", pad, n);
}

int error(int e);

byte *has(byte *p, int tok) {
	for (p = p; *p; p++) {
		if (*p == _INT || *p == _VAR || *p == _STR || *p == _FUN)
			p += 2;
		else if (*p == _TXT || *p == _LAB)
			while (*++p)
				;
		else if (*p == tok)
			return p;
		else if (*p == _REM)
			return 0;
	}
	return 0;
}

int sputc(char *buf, int lim, int bp, int c) {
	if (bp >= lim-1)
		error(ESBO);
	buf[bp] = c;
	return bp + 1;
}

int sputs(char *buf, int lim, int bp, char *s) {
	int	k;

	k = strlen(s);
	if (bp + k >= lim-1)
		error(ESBO);
	memcpy(buf+bp, s, k);
	return bp + k;
}

int sputd(char *buf, int lim, int bp, int val, int pad) {
	char	s[10];

	sprintf(s, "%*d", pad, val);
	return sputs(buf, lim, bp, s);
}

int slist(byte *buf, int len, byte *p, int ind) {
	int	t, v;
	byte	*tp;
	int	spc = 0, i, factor = 0, bp = 0;

	if (*p == _INT) {
		bp = sputd(buf, len, bp, p[1] | p[2]<<8, 5);
		bp = sputs(buf, len, bp, "  ");
		p += 3;
	}
	if (	*p == _NEXT || *p == _WEND || *p == _UNTIL ||
		*p == _ENDIF || *p == _ELSE
	) {
		ind -= 2;
	}
	if (ind < 0)
		ind = 0;
	for (i=0; i<ind; i++)
		bp = sputc(buf, len, bp, ' ');
	if (*p == _ELSE)
		ind += 2;
	while (*p) {
		if (spc)
			bp = sputc(buf, len, bp, ' ');
		spc = 1;
		if (*p == _INT) {
			p += 2;
			bp = sputd(buf, len, bp, p[-1] | *p<<8, 0);
			factor = _INT;
		}
		else if (*p == _VAR) {
			p += 2;
			v = (p[-1] | *p<<8);
			bp = sputc(buf, len, bp, v/(10*_BPW_)+'A');
			if (v%(10*_BPW_))
				bp = sputc(buf, len, bp,
					v%(10*_BPW_)/_BPW_+'0');
			if (p[1] == _LPAR)
				spc = 0;
			factor = 1;
		}
		else if (*p == _STR) {
			p += 2;
			v = ((p[-1] | *p<<8)) / _BPW_;
			bp = sputc(buf, len, bp, v+'A');
			bp = sputc(buf, len, bp, '$');
			if (p[1] == _LPAR)
				spc = 0;
			factor = 0;
		}
		else if (*p == _TXT) {
			bp = sputc(buf, len, bp, '\'');
			while (*++p) {
				bp = sputc(buf, len, bp, *p);
				if (*p == '\'')
					bp = sputc(buf, len, bp, '\'');
			}
			bp = sputc(buf, len, bp, '\'');
			factor = 0;
		}
		else if (*p == _LAB) {
			bp = sputc(buf, len, bp, '&');
			while (*++p)
				bp = sputc(buf, len, bp, *p);
			factor = 0;
		}
		else if (*p == _FUN) {
			p += 2;
			v = ((p[-1] | *p<<8)) / _BPW_;
			bp = sputc(buf, len, bp, v+'A');
			bp = sputc(buf, len, bp, '%');
			spc = 0;
			factor = 0;
		}
		else if (*p == _REM) {
			bp = sputs(buf, len, bp, "REM");
			while (*++p)
				bp = sputc(buf, len, bp, *p);
			factor = 0;
		}
		else {
			tp = tokens;
			t = 1;
			while (*tp) {
				if (*p == t)
					break;
				t++;
				tp += *tp;
			}
			if (!*tp)
				error(EXXX);
			for (v=*tp, t=1; t<v; t++)
				bp = sputc(buf, len, bp, tp[t]);
			if (tp[t-1] == '(')
				spc = 0;
			if (	*p == _FOR && !has(p, _NEXT) ||
				*p == _RPEAT && !has(p, _UNTIL) ||
				*p == _WHILE
			)
				ind += 2;
			if (*p == _IF && has(p, _THEN))
				ind += 2;
			if (*p == _LPAR || *p == _HASH)
				spc = 0;
			if (*p == _MINUS && !factor)
				spc = 0;
			factor = (*p == _RPAR);
		}
		p++;
		if (*p == _RPAR || *p == _COMMA || *p == _SEMI)
			spc = 0;
	}
	buf[bp] = 0;
	return ind;
}

int list(byte *p, int ind) {
	char	buf[BUFL+1];

	ind = slist(buf, BUFL+1, p, ind);
	putstr(buf);
	putch('\n');
	return ind;
}

int error(int e) {
	errval = e;
	idev = 0;
	odev = 1;
	if (runflg && !e)
		exit(0);
	if (!quiet) {
		if (e)
			putstr("**");
		putstr(msgs + (e * 4));
		if (e && e != EXXX) {
			putstr(" : ");
			if (linep)
				list(linep+1, 0);
			else
				putstr(line);
		}
		putstr("\n");
	}
	iod = -1;
	linep = 0;
	longjmp(restart, 1);
	return -1;
}

int readline(byte *s) {
	int	k;

	*s = 0;
	if (fgets(s, BUFL+1, files[idev]) == NULL || s == NULL)
		return -1;
	if ((k = strlen(s)) < 1)
		return -1;
	if (k >= BUFL)
		error(ELLO);
	s[k-1] = 0;
	return k-1;
}
 
void upcase(char *s) {
	while (*s) {
		*s = toupper(*s);
		s++;
	}
}

void gen(byte *out, int t) {
	out[scp++] = t;
}

void genp(byte *out, int t, int v) {
	gen(out, t);
	gen(out, v&0xff);
	gen(out, v>>8);
}

int kwfind(byte *k, int err) {
	byte	*p = tokens;
	int	t = 1;

	while (*p) {
		if (!memcmp(p, k, *p))
			return t;
		p += *p;
		t++;
	}
	error(err);
	return -1;
}

byte *kw(byte *in, byte *out, int *what) {
	byte	kwb[TOKL+1];
	int	n = 1, t;

	while(isalpha(*in) || *in == '$' || *in == '(') {
		if (n >= TOKL+1)
			error(ETLO);
		kwb[n++] = *in;
		in++;
		if (in[-1] == '(')
			break;
	}
	kwb[0] = n;
	kwb[n] = 0;
	t = kwfind(kwb, ESYN);
	gen(out, t);
	*what = t;
	return in;
}

byte *oper(byte *in, byte *out) {
	byte	kwb[4];

	kwb[0] = 2;
	kwb[1] = *in;
	kwb[2] = kwb[3] = 0;
	if (*in == '<' || *in == '>') {
		if (in[1] == '=' || in[1] == '>' && kwb[1] == '<') {
			kwb[2] = *++in;
			kwb[0]++;
		}
	}
	gen(out, kwfind(kwb, EOPR));
	return in+1;
}

byte *svar(byte *in, byte *out) {
	genp(out, _STR, (*in - 'A') * _BPW_);
	return in+2;
}

byte *fvar(byte *in, byte *out) {
	genp(out, _FUN, (*in - 'A')*_BPW_);
	return in+2;
}

byte *var(byte *in, byte *out) {
	int	addr;

	if (in[1] == '$')
		return svar(in, out);
	if (in[1] == '%')
		return fvar(in, out);
	addr = (*in - 'A') * 10 * _BPW_;
	if ('0' <= in[1] && in[1] <= '9') {
		in++;
		addr += (*in - '0') * _BPW_;
	}
	genp(out, _VAR, addr);
	return in+1;
}

byte *num(byte *in, byte *out) {
	long	v = 0;

	while (isdigit(*in)) {
		v = v * 10 +  (*in - '0');
		if ((v & 0x7fff) != v)
			error(EOVF);
		in++;
	}
	genp(out, _INT, (int) v);
	return in;
}

byte *text(byte *in, byte *out) {
	gen(out, _TXT);
	in++;
	for (;;) {
		if (!*in)
			error(ESTR);
		if (*in == '\'') {
			if (*++in == '\'')
				gen(out, '\'');
			else
				break;
		}
		else
			gen(out, *in);
		in++;
	}
	gen(out, 0);
	return in;
}

byte *label(byte *in, byte *out) {
	gen(out, _LAB);
	in++;
	while(isalpha(*in) || isdigit(*in) || *in == '.')
		gen(out, *in++);
	gen(out, 0);
	return in;
}

byte *comment(byte *in, byte *out) {
	while (*in)
		gen(out, *in++);
	gen(out, 0);
	return in;
}

int tokenize(byte *in, byte *out) {
	int	t;

	scp = 0;
	upcase(in);
	while (*in) {
		while (*in == ' ' || *in == '\t')
			in++;
		if (!*in)
			break;
		else if (*in == '?') {
			gen(out, _PRINT);
			in++;
		}
		else if (isalpha(*in)) {
			t = 0;
			if (isalpha(in[1]))
				in = kw(in, out, &t);
			else
				in = var(in, out);
			if (t == _REM)
				in = comment(in, out);
		}
		else if (isdigit(*in))
			in = num(in, out);
		else if (*in == '&')
			in = label(in, out);
		else if (*in == '\'')
			in = text(in, out);
		else
			in = oper(in, out);
	}
	gen(out, 0);
	return scp;
}

int qtokenize(byte *in, byte *out) {
	jmp_buf	outer;
	int	x;

	memcpy(outer, restart, sizeof(jmp_buf));
	if (setjmp(restart)) {
		memcpy(restart, outer, sizeof(jmp_buf));
		return -1;
	}
	quiet = 1;
	x = tokenize(in, out);
	quiet = 0;
	return x;
}

void moveprog(byte *new) {
	int	i, len, diff;

	if (progp == new)
		return;
	len = ptop - progp + 1;
	diff = new - progp;
	if (new < progp)
		for (i=0; i<len; i++)
			new[i] = progp[i];
	else
		for (i=len-1; i>=0; i--)
			new[i] = progp[i];
	ptop += diff;
	progp = new;
	if (linep) {
		intp += diff;
		linep += diff;
	}
}

#define lineno(addr) \
	((addr)[2] | (addr)[3]<<8)

byte *findln(int lno, int err) {
	byte	*p;
	int	here;

	p = progp;
	while (*p) {
		if ((here = lineno(p)) >= lno) {
			if (err && here != lno)
				error(ELIN);
			return p;
		}
		p += *p;
	}
	if (err)
		error(ELIN);
	return ptop;
}

void match(int t) {
	if (*intp == t)
		intp++;
	else
		error(ESYN);
}

int expr(void);

int variable() {
	int	v;

	intp += 3;
	v = (intp[-2] | intp[-1]<<8);
	if (*intp == _LPAR) {
		intp++;
		v += expr() * _BPW_;
		match(_RPAR);
	}
	if (ivars + v >= progp)
		error(ESUB);
	return v;
}

int f_abs(void) {
	int	v;

	v = expr();
	match(_RPAR);
	return abs(v);
}

byte *strexpr(void);

int f_asc(void) {
	byte	*sp;

	sp = strexpr();
	match(_RPAR);
	return *sp;
}

int f_eof(void) {
	expr();
	match(_RPAR);
	return feof(files[idev]);
}

int f_fre(void) {
	expr();
	match(_RPAR);
	return sbufs-ptop;
}

int f_len(void) {
	byte	*sp;

	sp = strexpr();
	match(_RPAR);
	return strlen(sp);
}

int f_val(void) {
	byte	*sp;
	int	n = 0;

	sp = strexpr();
	match(_RPAR);
	sscanf(sp, "%d", &n);
	return n;
}

int f_pos(void) {
	byte	*s1, *s2;
	int	l1, l2;

	l1 = strlen(s1 = strexpr());
	match(_COMMA);
	l2 = strlen(s2 = strexpr());
	match(_RPAR);
	s1 = strstr(s2, s1);
	return s1 == NULL? 0: s1 - s2 + 1;
}

int f_rnd(void) {
	int	v;

	if ((v = expr()) == 0)
		error(EDIV);
	match(_RPAR);
	return rand() % v;
}

int f_peek(void) {
	int	v;

	v = expr();
	match(_RPAR);
	return space[v];
}

#define getw(p) \
	(space[p] | space[(p)+1]<<8)

#define putw(p, v) \
	(space[p] = (v), space[(p)+1] = (v)>>8)

int funapp(void) {
	int	f = (intp[1] | intp[2]<<8);
	byte	*fn, *iintp;
	int	v, outer, inner, r;

	intp += 3;
	fn = progp + getw(fvars-space+f);
	if (fn == progp)
		error(EFUN);
	v = fn[1] | fn[2]<<8;
	outer = getw(ivars-space+v);
	match(_LPAR);
	inner = expr();
	match(_RPAR);
	putw(ivars-space+v, inner);
	iintp = intp;
	intp = fn+5;
	r = expr();
	intp = iintp;
	putw(ivars-space+v, outer);
	return r;
}

void c_gosub(int how);
void cont(int sub);

int sbrcall(void) {
	int	r;

	c_gosub(_CALL);
	cont(1);
	r = retval;
	retval = retflg = 0;
	match(_RPAR);
	return r;
}

int fac(void) {
	int	v, p;

	switch(*intp) {
		case _INT:	intp += 3;
				v = intp[-2] | intp[-1]<<8;
				return v;
		case _VAR:	p = variable();
				v = ivars[p] | ivars[p+1]<<8;
				if (v > 0x7fff)
					v = -(0x10000-v);
				return v;
		case _LPAR:	intp++;
				v = expr();
				match(_RPAR);
				return v;
		case _FUN:	return funapp();
		case _CALL:	intp++; return sbrcall();
		case _MINUS:	intp++; return -fac();
		case _ABS:	intp++; return f_abs();
		case _ASC:	intp++; return f_asc();
		case _EOF:	intp++; return f_eof();
		case _FRE:	intp++; return f_fre();
		case _VAL:	intp++; return f_val();
		case _LEN:	intp++; return f_len();
		case _PEEK:	intp++; return f_peek();
		case _POS:	intp++; return f_pos();
		case _RND:	intp++; return f_rnd();
		default:	return error(ESYN);
	}
}

int term(void) {
	long	v = fac();
	int	v2;

	for (;;) {
		switch (*intp) {
		case _MUL:	intp++; v *= fac(); break;
		case _SLASH:	intp++;
				if ((v2 = fac()) == 0)
					error(EDIV);
				v /= v2; break;
		default:	return (int) v;
		}
		if ((abs(v) & 0x7fff) != abs(v))
			error(EOVF);
	}
}

int sum(void) {
	long	v = term();

	for (;;) {
		switch (*intp) {
		case _PLUS:	intp++; v += term(); break;
		case _MINUS:	intp++; v -= term(); break;
		default:	return (int) v;
		}
		if ((abs(v) & 0x7fff) != abs(v))
			error(EOVF);
	}
}

int relop(void) {
	int	v;
	byte	*s;

	switch (*intp) {
	case _STR: case _TXT: case _CHRS: case _MIDS: case _STRS:
		s = strexpr();
		switch (*intp) {
		case _EQU:	intp++; return strcmp(s, strexpr()) == 0;
		case _NEQ:	intp++; return strcmp(s, strexpr()) != 0;
		case _LESS:	intp++; return strcmp(s, strexpr()) < 0;
		case _GRTR:	intp++; return strcmp(s, strexpr()) > 0;
		case _LSEQ:	intp++; return strcmp(s, strexpr()) <= 0;
		case _GTEQ:	intp++; return strcmp(s, strexpr()) >= 0;
		default:	error(ESYN);
		}
	default:
 		v = sum();
		switch (*intp) {
		case _EQU:	intp++; return v == sum();
		case _NEQ:	intp++; return v != sum();
		case _LESS:	intp++; return v < sum();
		case _GRTR:	intp++; return v > sum();
		case _LSEQ:	intp++; return v <= sum();
		case _GTEQ:	intp++; return v >= sum();
		default:	return v;
		}
	}
}

int neg(void) {
	if (*intp == _NOT) {
		intp++;
		return !relop();
	}
	else {
		return relop();
	}
}

int logand(void) {
	int	v = neg();

	for (;;) {
		if (*intp == _AND) {
			intp++;
			v = (v != 0 & neg() != 0);
		}
		else {
			return v & 0xffff;
		}
	}
}

int expr(void) {
	int	v = logand();

	for (;;) {
		if (*intp == _OR) {
			intp++;
			v = (v != 0 | logand() != 0);
		}
		else {
			v &= 0xffff;
			if (v > 0x7fff)
				v = -(0x10000-v);
			return v;
		}
	}
}

int strvar(void) {
	int	sv;

	sv = intp[1] | intp[2]<<8;
	intp += 3;
	if (*intp == _LPAR) {
		intp++;
		sv += expr() * _BPW_;
		match(_RPAR);
	}
	if (sv >= NSTR*_BPW_)
		error(ESUB);
	return sv;
}

byte *newpad(int k) {
	byte	*n = strpad + spp;

	if (spp + k >= PADL*BUFL)
		error(ESBO);
	spp += k;
	return n;
}

byte *sf_chr(void) {
	int	v;
	byte	*s;

	v = expr();
	match(_RPAR);
	s = newpad(2);
	s[0] = v;
	s[1] = 0;
	return s;
}

byte *sf_mid(void) {
	byte	*str, *s;
	int	s1, len = BUFL;

	str = strexpr();
	match(_COMMA);
	s1 = expr();
	if (*intp == _COMMA) {
		match(_COMMA);
		len = expr();
	}
	match(_RPAR);
	if (--s1 < 0)
		s1 = 0;
	if (strlen(str) < s1)
		return "";
	if (strlen(str + s1) < len)
		return str + s1;
	s = newpad(len+1);
	memcpy(s, str + s1, len);
	s[len] = 0;
	return s;
}

byte *sf_str(void) {
	int	v;
	byte	*s;

	v = expr();
	if (v > 0x7fff)
		v = -(0x10000-v);
	match(_RPAR);
	s = newpad(7);
	sprintf(s, "%d", v);
	return s;
}

byte *strfac(void) {
	byte	*sv;
	int	sp;

	switch (*intp) {
		case _TXT:	sv = ++intp;
				while (*intp)
					intp++;
				intp++;
				return sv;
		case _STR:	sp = strvar();
				sp = getw(sp);
				return sp? space + sp: (byte *)"";
		case _CHRS:	intp++; return sf_chr();
		case _MIDS:	intp++; return sf_mid();
		case _STRS:	intp++; return sf_str();
		default:	error(ESYN);
	}
}

byte *strexpr(void) {
	byte	*sv = strfac(), *sv2, *s;
	int	k, len;

	if (*intp == _PLUS) {
		len = strlen(sv);
		s = newpad(BUFL+1);
		memcpy(s, sv, len);
		for (;;) {
			switch (*intp) {
			case _PLUS:	intp++;
					sv2 = strfac();
					if (len + (k = strlen(sv2)) > BUFL)
						error(ESBO);
					memcpy(s + len, sv2, k+1);
					len += k;
					break;
			default:	return s;
			}
		}
	}
	return sv;
}

int framesize(int type) {
	switch (type) {
	case _WHILE:
	case _RPEAT:
	case _CALL:
	case _GOSUB:	return sizeof(aframe);
	case _FOR:	return sizeof(fframe);
	case _WITH:	return sizeof(pframe);
	case _LOCAL:
	case _ENDLC:	return 0;
	default:	error(EXXX);
	}
}

void push(void *sp, int type) {
	int	k = framesize(type);

	if (isp+k+1 >= STKL)
		error(ESTK);
	memcpy(istack + isp, sp, k);
	istack[isp+k] = type;
	isp += k+1;
}

void pick(void *sp, int type) {
	int	k = framesize(type);

	if (isp-k-1 < 0)
		error(ESTK);
	if (istack[isp-1] != type)
		error(ENST);
	memcpy(sp, &istack[isp-k-1], k);
}

void pop(void) {
	int	k;

	if (isp < 1)
		error(ESTK);
	k = framesize(istack[isp-1]);
	if ((isp -= (k+1)) < 0)
		error(ESTK);
}

void trace(void) {
	if (tracing && linep) {
		putch('[');
		putd(lineno(linep), 0);
		putch(']');
	}
}

void skipto(int to, int alt, int over) {
	int	lev = 1;
	byte	*p;

	if (linep == NULL)
		error(ENST);
	for (p = linep + *linep; *p; p += *p) {
		if (p[4] == to)
			lev--;
		else if (p[4] == over) {
			if (over != _IF || has(p+1, _THEN))
				lev++;
		}
		if (!lev || p[4] == alt && lev == 1) {
			linep = p;
			intp = p + 5;
			if (to == _UNTIL)
				expr();
			return;
		}
	}
	error(ENST);
}

void c_if(void) {
	int	first = linep && linep+5 == intp;

	if (!expr()) {
		if (!linep)
			error(EOK);
		if (*intp == _THEN) {
			if (!first)
				error(ESYN);
			skipto(_ENDIF, _ELSE, _IF);
		}
		linep += *linep;
		if (linep >= ptop)
			error(EOK);
		intp = linep + 4;
		trace();
	}
	if (*intp == _THEN) {
		if (!first)
			error(ESYN);
		intp++;
	}
}

void c_else(void) {
	if (!linep || linep+5 != intp)
		error(ESYN);
	skipto(_ENDIF, 0, _IF);
	trace();
}

void let_var(void) {
	int	x, dest = variable();

	if (*intp != _EQU)
		error(ESYN);
	intp++;
	x = expr();
	ivars[dest] = x;
	ivars[dest+1] = x>>8;
}

int allocstr(void) {
	byte	*p;

	p = sbufs;
	while (*p) {
		if (!(*p & 128)) {
			*p |= 128;
			return p-space+1;
		}
		p += (*p & 127);
	}
	if ((sbufs -= BUFL+2) < ptop) {
		sbufs += BUFL+2;
		error(EMEM);
	}
	*sbufs = (BUFL+2 | 128);
	return sbufs-space+1;
}

void freestr(byte *s) {
	if (s-1 == sbufs) {
		s[-1] &= 127;
		while ((*sbufs & 128) == 0 && sbufs < memtop) {
			sbufs += BUFL+2;
		}
	}
	else
		s[-1] &= 127;
}

void asg_str(int sp, byte *s) {
	int	x;

	if (!*s) {
		x = getw(sp);
		if (x)
			freestr(space + x);
		putw(sp, 0);
		return;
	}
	else {
		x = getw(sp);
		if (x == 0) {
			x = allocstr();
			putw(sp, x);
		}
		memcpy(space + x, s, BUFL);
	}
}

void let_str(void) {
	byte	*val;
	int	sp;

	sp = strvar();
	if (*intp != _EQU)
		error(ESYN);
	intp++;
	val = strexpr();
	asg_str(sp, val);
}

void c_def(void) {
	int	f;

	if (!linep)
		error(ESYN);
	if (*intp != _FUN)
		error(ESYN);
	intp += 3;
	f = (intp[-2] | intp[-1]<<8);
	match(_LPAR);
	if (*intp != _VAR)
		error(ESYN);
	intp += 3;
	match(_RPAR);
	match(_EQU);
	putw(fvars-space+f, intp-progp-5);
}

void c_dim(void) {
	byte	*v;

	for (;;) {
		if (*intp != _VAR)
			error(ESYN);
		intp += 3;
		v = ivars + (intp[-2] | intp[-1]<<8);
		match(_LPAR);
		v += expr() * _BPW_;
		match(_RPAR);
		if (v + (ptop-progp) >= sbufs)
			error(EMEM);
		if (v >= progp) {
			moveprog(v);
		}
		if (*intp != _COMMA)
			break;
		intp++;
	}
}

void c_for(void) {
	fframe	fp;
	int	x;

	fp.vp = variable();
	match(_EQU);
	x = expr();
	ivars[fp.vp] = x;
	ivars[fp.vp+1] = x>>8;
	match(_TO);
	fp.lim = expr();
	if (*intp == _STEP) {
		intp++;
		fp.stp = expr();
	}
	else
		fp.stp = 1;
	fp.lp = linep;
	fp.ip = intp;
	push(&fp, _FOR);
}

void c_next(void) {
	int	v;
	fframe	fp;

	pick(&fp, _FOR);
	pop();
	v = ivars[fp.vp] | ivars[fp.vp+1]<<8;
	if (v > 32767)
		v = -(0x10000-v);
	v += fp.stp;
	ivars[fp.vp] = v;
	ivars[fp.vp+1] = v>>8;
	if (fp.stp >= 0? v > fp.lim: v < fp.lim)
		return;
	push(&fp, _FOR);
	linep = fp.lp;
	intp = fp.ip;
	trace();
}

void c_while(void) {
	aframe	fp;
	byte	*here;

	here = intp-1;
	if (!linep || linep+4 != here)
		error(ESYN);
	if (expr()) {
		fp.lp = linep;
		fp.ip = here;
		push(&fp, _WHILE);
	}
	else {
		skipto(_WEND, 0, _WHILE);
		trace();
	}
}

void c_wend(void) {
	aframe	fp;

	if (!linep || linep+5 != intp)
		error(ESYN);
	pick(&fp, _WHILE);
	linep = fp.lp;
	intp = fp.ip;
	pop();
	trace();
}

void c_repeat(void) {
	aframe	fp;

	fp.lp = linep;
	fp.ip = intp;
	push(&fp, _RPEAT);
}

void c_until(void) {
	aframe	fp;

	pick(&fp, _RPEAT);
	if (expr()) {
		pop();
	}
	else {
		linep = fp.lp;
		intp = fp.ip;
		trace();
	}
}

void c_exit(void) {
	if (!isp)
		error(ESTK);
	switch (istack[isp-1]) {
	case _FOR:
		skipto(_NEXT, 0, _FOR);
		break;
	case _WHILE:
		skipto(_WEND, 0, _WHILE);
		break;
	case _RPEAT:
		skipto(_UNTIL, 0, _RPEAT);
		break;
	default:
		error(ENST);
	}
	pop();
	trace();
}

int setdev(int dev) {
	if (dev < 0 || dev >= NFIL || files[dev] == NULL)
		error(EDEV);
	return dev;
}

void c_ioctl(void) {
	int	dev, rc;
	byte	*service;

	match(_HASH);
	dev = setdev(number());
	match(_COMMA);
	service = strexpr();
	switch(*service) {
		case 'A':	rc = fseek(files[dev], 0L, SEEK_END);
				break;
		case 'R':	rc = fseek(files[dev], 0L, SEEK_SET);
				break;
		case 'S':	fflush(files[dev]);
				break;
		case 'T':	fflush(files[dev]);
				rc = ftruncate(fileno(files[dev]),
					ftell(files[dev]));
				break;
		default:	error(EARG);
	}
	if (rc < 0)
		error(EIOX);
}

void c_let(void) {
	for (;;) {
		switch (*intp) {
			case _VAR:	let_var(); break;
			case _STR:	let_str(); break;
			default:	error(ESYN);
		}
		if (*intp != _COMMA)
			break;
		intp++;
	}
}

void c_clear(void) {
	memset(svars, 0, NSTR * _BPW_);
	memset(fvars, 0, NFUN * _BPW_);
	memset(ivars, 0, NVAR * _BPW_);
	*(sbufs = memtop) = 0;
	moveprog(&space[(NVAR+NSTR+NFUN)*_BPW_]);
	isp = 0;
	datal = datap = NULL;
}

void c_new(void) {
	ptop = progp = &space[(NSTR+NVAR+NFUN)*_BPW_];
	c_clear();
	*progp = 0;
	ptop = progp;
	tracing = 0;
	epos = lpos = 0;
}

void c_load() {
	int	dev;

	if (linep)
		error(EINT);
	if (iod >= 0)
		error(ENST);
	match(_HASH);
	dev = setdev(number());
	c_new();
	iod = idev;
	idev = dev;
	rewind(files[dev]);
}

byte *findlab(char *name, int k) {
	byte	*a;

	for (a = labels; *a; a += *a) {
		if (k == *a-3 && !memcmp(name, a+3, k))
			return space + (a[1] | a[2] << 8);
	}
	return NULL;
}

void mklabels(void) {
	byte	*lp, *p, *name;
	int	i, dest;

	labels[lbp = 0] = 0;
	for (lp = progp; *lp; lp += *lp) {
		for (p = lp+1; *p; p++) {
			if (	*p == _INT || *p == _VAR || *p == _STR ||
				*p == _FUN
			)
				p += 2;
			else if (*p == _TXT)
				while (*++p)
					;
			else if (*p == _LABEL && p[1] == _LAB) {
				++p;
				name = p+1;
				for (++p; *p; p++)
					;
				i = p - name;
				if (findlab(name, i)) {
					linep = lp;
					error(EDUP);
				}
				if (lbp + i + 4 >= LABL)
					error(EMEM);
				labels[lbp++] = i+3;
				dest = lp + *lp - space;
				labels[lbp++] = dest;
				labels[lbp++] = dest >> 8;
				memcpy(labels + lbp, name, i);
				lbp += i;
				labels[lbp] = 0;
			}
			else if (*p == _REM)
				break;
		}
	}
}

int number(void) {
	if (*intp != _INT)
		error(ESYN);
	intp += 3;
	return intp[-2] | intp[-1]<<8;
}

byte *destline(void) {
	byte	*addr;
	byte	*name;
	int	i;

	if (*intp == _INT)
		return findln(number(), 1);
	name = intp+1;
	for (++intp; *intp; intp++)
		;
	i = intp - name;
	intp++;
	if ((addr = findlab(name, i)) != NULL)
		return addr;
	mklabels();
	if ((addr = findlab(name, i)) != NULL)
		return addr;
	error(ELAB);
}

void c_run(void) {
	byte	*addr;

	if (linep)
		error(EINT);
	c_clear();
	retflg = 0;
	addr = progp;
	if (*intp && *intp != _COLON)
		addr = destline();
	linep = addr;
}

void c_goto(void) {
	linep = destline();
	advance = 0;
}

void c_ongoto(void) {
	#define MAXDEST 16
	byte	*dests[MAXDEST];
	int	n, x;

	x = expr();
	match(_GOTO);
	dests[0] = destline();
	n = 1;
	while (*intp == _COMMA) {
		intp++;
		if (n >= MAXDEST)
			error(ERNG);
		dests[n++] = destline();
	}
	if (1 <= x && x <= n) {
		linep = dests[x-1];
		advance = 0;
	}
}

void prettylist(int sl, int el, int max) {
	int	ind, first, rem, loc = 0, outd;
	byte	*p;

	p = progp;
	while (*p && lineno(p) < sl)
		p += *p;
	rem = ind = 0;
	first = 1;
	outd = 0;
	while (*p && lineno(p) <= el) {
		if (max-- == 0)
			break;
		if (	(p[4] == _REM || p[4] == _LABEL) &&
			!rem && !ind && !first
		)
			putch('\n');
		rem = p[4] == _REM || p[4] == _LABEL;
		if (has(p+1, _LOCAL) && loc)
			outd = 2;
		else if (has(p+1, _ENDLC) ||
			p[4] == _RETRN && loc && ind == 2
		) {
			ind -= 2;
			loc = 0;
		}
		ind = list(p+1, ind - outd) + outd;
		if (has(p+1, _LOCAL) && !loc) {
			ind += 2;
			loc = 1;
		}
		outd = first = 0;
		p += *p;
	}
}

void c_list(void) {
	int	sl = 0, el = 32767;

	if (*intp && *intp != _COLON) {
		sl = expr();
		if (*intp == _COMMA) {
			intp++;
			el = expr();
		}
	}
	prettylist(sl, el, -1);
}

void c_open(void) {
	int	dev;
	byte	*name;
	FILE	*f;

	match(_HASH);
	dev = number();
	if (dev < 2 || dev >= NFIL)
		error(EDEV);
	match(_COMMA);
	name = strexpr();
	if ((f = fopen(name, "r+")) == NULL)
		if ((f = fopen(name, "w+")) == NULL)
			error(EIOX);
	if (files[dev])
		fclose(files[dev]);
	files[dev] = f;
	if (fnames[dev])
		free(fnames[dev]);
	fnames[dev] = strdup(name);
}

void c_poke(void) {
	int	a;

	a = expr();
	match(_COMMA);
	space[a] = expr();
}

int nextdata(int type) {
	int	v, sign = 1;

	if (!datap || !(*datap)) {
		if (datal)
			datal += *datal;
		else
			datal = progp;
		while (*datal) {
			if (datal[4] == _DATA)
				break;
			datal += *datal;
		}
		if (!(*datal))
			error(EEOD);
		datap = datal + 5;
	}
	if (*datap == _MINUS) {
		sign = -1;
		datap++;
	}
	if (*datap != type)
		error(ETYP);
	if (type == _INT) {
		v = (datap[1] | datap[2]<<8) * sign;
		datap += 3;
	}
	else {
		v = datap-progp+1;
		while (*datap)
			datap++;
		datap++;
	}
	if (*datap == _COMMA)
		datap++;
	return v;
}

void c_read(void) {
	int	v, d;
	byte	*s;

	for (;;) {
		switch(*intp) {
		case _VAR:	v = variable();
				d = nextdata(_INT);
				ivars[v] = d;
				ivars[v+1] = d>>8;
				break;
		case _STR:	v = strvar();
				s = progp + nextdata(_TXT);
				asg_str(v, s);
				break;
		default:	error(ESYN);
				break;
		}
		if (*intp != _COMMA)
			break;
		intp++;
	}
}

void c_restore(void) {
	byte	*addr;

	if (*intp && *intp != _COLON) {
		addr = destline();
		datal = addr;
		datap = addr+5;
	}
	else {
		datap = datal = NULL;
	}
}

void c_save(void) {
	int	ood = odev;

	match(_HASH);
	ood = odev;
	odev = setdev(number());
	rewind(files[odev]);
	fflush(files[odev]);
	ftruncate(fileno(files[odev]), 0);
	prettylist(0, 32767, -1);
	fflush(files[odev]);
	odev = ood;
}

void c_close(void) {
	int	dev;

	match(_HASH);
	dev = number();
	if (dev < 2 || dev >= NFIL)
		error(EDEV);
	if (dev == odev)
		odev = 1;
	if (dev == idev)
		idev = 0;
	if (files[dev] == NULL)
		return;
	fclose(files[dev]);
	if (fnames[dev])
		free(fnames[dev]);
	fnames[dev] = NULL;
	files[dev] = NULL;
}

void c_erase(void) {
	if (remove(strexpr()))
		error(EIOX);
}

void c_files(void) {
	int	i;

	for (i=0; i<NFIL; i++) {
		if (files[i] && fnames[i]) {
			fprintf(files[odev], "#%d = %s\n", i, fnames[i]);
		}
	}
}

void c_label(void) {
	if (*intp == _LAB)
		while (*intp++)
			;
	else
		error(ESYN);
}

void c_local(int check) {
	int	a, v, pa;
	byte	*sp;
	byte	x;
	pframe	pp;

	for (;;) {
		switch (*intp) {
		case _VAR:
			a = getw(intp-space+1);
			a = ivars-space+a;
			v = getw(a);
			intp += 3;
			if (*intp == _EQU) {
				intp++;
				pa = expr();
			}
			else
				pa = v;
			putw(a, pa);
			break;
		case _STR:
			a = getw(intp-space+1);
			v = getw(a);
			intp += 3;
			pa = allocstr();
			if (*intp == _EQU) {
				intp++;
				sp = strexpr();
			}
			else
				sp = space + v;
			memcpy(space + pa, sp, BUFL);
			putw(a, pa);
			a = a | SFLG;
			break;
		default:
			error(ESYN);
			break;
		}
		pp.addr = a;
		pp.val = v;
		push(&pp, _WITH);
		if (*intp != _COMMA)
			break;
		intp++;
	}
	if (check)
		push(&x, _LOCAL);
}

void c_gosub(int how) {
	byte	*addr;
	aframe	fp;

	push(&fp, _ENDLC);
	addr = destline();
	if (*intp == _WITH) {
		intp++;
		c_local(0);
	}
	fp.lp = linep;
	fp.ip = intp;
	push(&fp, how);
	linep = addr;
	advance = 0;
}

void c_input(void) {
	int	i = idev, vp, sp, x;
	byte	*buf = newpad(BUFL+1);

	if (*intp == _HASH) {
		intp++;
		idev = setdev(number());
		if (!*intp || *intp == _COLON)
			return;
		if (*intp == _COMMA)
			intp++;
	}
	for (;;) {
		switch(*intp) {
		case _VAR:	vp = variable();
				readline(buf);
				sscanf(buf, "%d", &x);
				ivars[vp] = x;
				ivars[vp+1] = x>>8;
				break;
		case _STR:	sp = strvar();
				readline(buf);
				upcase(buf);
				asg_str(sp, buf);
				break;
		default:	error(ESYN);
				break;
		}
		if (*intp != _COMMA)
			break;
		intp++;
	}
	if (idev == 0 && feof(stdin))
		clearerr(stdin);
	idev = i;
}

void c_print(void) {
	int	o = odev, doNL = 1;

	if (*intp == _HASH) {
		intp++;
		odev = setdev(number());
		if (!*intp || *intp == _COLON)
			return;
		if (*intp == _COMMA)
			intp++;
	}
	while (*intp && *intp != _COLON) {
		switch (*intp) {
			case _CHRS: case _MIDS: case _STRS:
			case _TXT:	
			case _STR:	putstr(strexpr());
					break;
			default:	putd(expr(), 0);
					break;
		}
		if (*intp == _COMMA) {
			intp++;
			putch('\t');
			doNL = 0;
		}
		else if (*intp == _SEMI) {
			intp++;
			doNL = 0;
		}
		else {
			doNL = 1;
			break;
		}
	}
	if (doNL)
		putch('\n');
	else
		fflush(files[odev]);
	odev = o;
}

int adjust(byte *p, int pass, int *n, int *o, int t, int *missp) {
	int	here, i;

	if (p[1] == _LAB) {
		for (i=0; *p; i++)
			p++;
		return i;
	}
	here = lineno(p);
	for (i = 0; i < t; i++)
		if (o[i] == here)
			break;
	if (i >= t)
		(*missp)++;
	else if (pass == 1) {
		p[2] = n[i];
		p[3] = n[i] >> 8;
	}
	return 3;
}

void renum(int from, int to, int start, int delta) {
	byte	*p, *lp;
	int	t = 0;
	int	here;
	long	ln;
	int	pass, missed = 0;

	ln = start;
	for (p = progp; p < ptop; p += *p) {
		if (t > NLIN)
			error(EMEM);
		here = old[t] = lineno(p);
		if (here < from || here > to) {
			new[t++] = here;
		}
		else {
			if (ln != (ln & 0x7fff))
				error(EOVF);
			new[t++] = ln;
			ln += delta;
		}
	}
	for (pass = 0; pass < 2; pass++) {
		ln = start;
		for (lp = progp; lp < ptop; lp += *lp) {
			here = lineno(lp);
			if (pass == 1 && from <= here && here <= to) {
				lp[2] = ln;
				lp[3] = ln>>8;
				ln += delta;
			}
			for (p = lp+1; *p; p++) {
				if (*p == _INT || *p == _VAR ||
				    *p == _STR || *p == _FUN
				) {
					p += 2;
				}
				else if (*p == _TXT || *p == _LAB) {
					while (*++p)
						;
				}
				else if (*p == _REM) {
					break;
				}
				else if (*p == _GOSUB || *p == _GOTO) {
					if (p[1] != _INT && p[1] != _LAB)
						error(ELIN);
					do {
						p += adjust(p, pass, new, old,
							t, &missed);
						p++;
					} while (*p == _COMMA);
					p--;
				}
				else if (*p == _RSTOR && p[1] == _INT) {
					p += adjust(p, pass, new, old, t,
						&missed);
				}
				else if (*p == _CALL) {
					if (p[1] != _INT && p[1] != _LAB)
						error(ELIN);
					p += adjust(p, pass, new, old, t,
						&missed);
				}
			}
		}
		if (missed) {
			error(ELIN);
		}
	}
}

void c_renum(void) {
	int	start = 100, delta = 10;

	if (linep)
		error(EINT);
	if (*intp && *intp != _COLON)
		start = delta = expr();
	if (*intp == _COMMA) {
		intp++;
		delta = expr();
	}
	renum(0, 32767, start, delta);
}

void store(byte *src, int len);

void c_ins(void) {
	int	where, inc = 10, k;
	byte	prg[BUFL];

	if (linep)
		error(EINT);
	where = expr();
	if (*intp == _COMMA) {
		intp++;
		inc = expr();
	}
	for (;;) {
		k = sputd(line, BUFL, 0, where, 0);
		line[k++] = ' ';
		putd(where, 5);
		putch(' ');
		if (readline(line+k) <= 0)
			break;
		k = tokenize(line, prg);
		renum(where, 32767, where+inc, inc);
		where += inc;
		store(prg, k);
	}
	error(EOK);
}

void delete(byte *p, int len);
void insert(byte *p, int len);

void c_cpmov(int move) {
	int	from, to, dest, lines, bytes, diff;
	byte	*l0, *l1, *ld;

	if (linep)
		error(EINT);
	from = expr();
	match(_COMMA);
	to = expr();
	if (*intp == _COMMA) {
		*intp++;
		dest = expr();
	}
	else {
		dest = to;
		to = from;
	}
	if (dest >= from && dest <= to)
		error(ELIN);
	lines = to - from + 1;
	l0 = findln(from, 1);
	findln(to, 1);
	l1 = findln(to+1, 0);
	bytes = l1 - l0;
	if (bytes == 0)
		return;
	renum(dest, 32767, dest+lines+9, 10);
	ld = findln(dest+lines+9, 0);
	diff = ld < l0? bytes: 0;
	insert(ld, bytes);
	memcpy(ld, l0+diff, bytes);
	if (move)
		delete(l0+diff, bytes);
	renum(0, 32767, 100, 10);
}

void c_del(void) {
	int	from, to;
	byte	*l0, *l1;

	if (linep)
		error(EINT);
	from = to = expr();
	if (*intp == _COMMA) {
		intp++;
		to = expr();
	}
	l0 = findln(from, 1);
	findln(to, 1);
	l1 = findln(to+1, 0);
	if (l0 != l1)
		delete(l0, l1-l0);
}

void c_endloc(int check) {
	pframe	pp;
	int	sp;
	byte	x;

	if (check) {
		pick(&x, _LOCAL);
		isp--;
	}
	while (isp > 0 && istack[isp-1] == _WITH) {
		pick(&pp, _WITH);
		if (pp.addr & SFLG) {
			sp = getw(pp.addr & (SFLG-1));
			freestr(space + sp);
			putw(pp.addr & (SFLG-1), pp.val);
		}
		else {
			putw(pp.addr, pp.val);
		}
		pop();
		if (check && istack[isp-1] == _LOCAL)
			isp--;
	}
	if (isp > 0 && istack[isp-1] == _ENDLC)
		pop();
}

void c_return(void) {
	aframe	fp;

	if (*intp && *intp != _COLON)
		retval = expr();
	if (isp && istack[isp-1] == _LOCAL)
		c_endloc(1);
	if (isp && istack[isp-1] == _CALL) {
		pick(&fp, _CALL);
		retflg = 1;
	}
	else {
		pick(&fp, _GOSUB);
	}
	linep = fp.lp;
	intp = fp.ip;
	pop();
	c_endloc(0);
	trace();
}

void c_search(void) {
	byte	*p, *q, *what, *w2, *s;
	byte	buf[BUFL+1];
	int	sl = 0, el = 32767, ln;

	if (linep)
		error(EINT);
	what = strexpr();
	for (p = q = what; *p; p++, q++)
		if (*p == '%') {
			if (p[1] == '%') {
				*q = '%';
				p++;
			}
			else {
				*q = '\1';
			}
		}
		else {
			*q = *p;
		}
	*q = 0;
	w2 = strchr(what, '\1');
	if (w2)
		*w2++ = 0;
	if (*intp == _COMMA) {
		intp++;
		sl = expr();
		if (*intp == _COMMA) {
			intp++;
			el = expr();
		}
	}
	for (p = progp; *p; p += *p) {
		ln = lineno(p);
		if (sl <= ln && ln <= el) {
			slist(buf, BUFL, p+1, 0);
			if (	(s = strstr(buf, what)) &&
				(!w2 || strstr(s+1, w2))
			)
				list(p+1, 0);
		}
	}
}

byte *edit(byte *addr) {
	byte	new[BUFL+1], cmd[BUFL+1], prg[BUFL];
	int	i, j, kb, kc, k;

	epos = addr[2] | addr[3] << 8;
	for (;;) {
		slist(line, BUFL+1, addr+4, 0);
		kb = strlen(line);
		putstr("> ");
		putd(epos, 0);
		putch(' ');
		putstr(line);
		putstr("\n> ");
		putd(epos, 0);
		putch(' ');
		if ((kc = readline(cmd)) < 1)
			error(EOK);
		upcase(cmd);
		j = sputd(new, BUFL+1, 0, epos, 0);
		new[j++] = ' ';
		k = j;
		for (i = 0; i < kc; i++) {
			switch (cmd[i]) {
			case ' ':
				if (i < kb)
					new[j++] = line[i];
				break;
			case 'X':
				break;
			case 'D':
				line[kb = i] = 0;
				break;
			case 'C':
				line[kb = i] = 0;
			case 'A':
				if (cmd[i] == 'A') {
					strcpy(new+k, line);
					j = strlen(new);
				}
			case 'I':
				if (strlen(line) + kc-i >= BUFL)
					error(ESBO);
				memcpy(new+j, cmd+i+1, kc-i-1);
				j += kc-i-1;
				break;
			case 'Q':
				break;
			default:
				error(ESYN);
			}
			if (strchr("ACIQ", cmd[i]))
				break;
		}
		if (i < kb && cmd[i] != 'A') {
			strcpy(new+j, line+i);
			j += strlen(line+i);
		}
		new[j] = 0;
		strcpy(line, new);
		kb = tokenize(line, prg);
		store(prg, kb);
		if (cmd[i] == 'Q')
			break;
	}
}

void c_ed(void) {
	if (linep)
		error(EINT);
	if (*intp && *intp != _COLON)
		epos = expr();
	edit(findln(epos, 1));
	error(EOK);
}

void listq(int pos) {
	if (linep)
		error(EINT);
	prettylist(pos, 32767, 20);
}

void c_listq(void) {
	if (*intp && *intp != _COLON)
		lpos = expr();
	listq(lpos);
}

void c_nextq(void) {
	byte	*p;
	int	i;

	p = findln(lpos, 0);
	for (i = 0; i < 20 && *p; i++)
		p += *p;
	if (*p)
		lpos = lineno(p);
	listq(lpos);
}

void c_prevq(void) {
	byte	*p;
	int	q[21];

	memset(q, 0, sizeof(q));
	for (p = progp; *p; p += *p) {
		memcpy(q+1, q, sizeof(int)*20);
		q[0] = lineno(p);
		if (lineno(p) >= lpos)
			break;
	}
	listq(lpos = q[20]);
}

void interpret(byte *pp) {
	intp = pp;
	advance = 1;
	for (;;) {
		spp = 0;
		switch (*intp++) {
			case _SLASH:	c_listq(); break;
			case _LESS:	c_prevq(); break;
			case _GRTR:	c_nextq(); break;
			case _ED:	c_ed(); break;
			case _IF:	c_if(); break;
			case _ON:	c_ongoto(); break;
			case _DEF:	c_def(); return;
			case _DEL:	c_del(); break;
			case _DIM:	c_dim(); break;
			case _END:	error(isp? ENST: EOK); break;
			case _FOR:	c_for(); break;
			case _INS:	c_ins(); break;
			case _LET:	c_let(); break;
			case _NEW:	c_new(); error(EOK); break;
			case _REM:	return;
			case _RUN:	c_run(); return;
			case _COPY:	c_cpmov(0); break;
			case _DATA:	return;
			case _ELSE:	c_else(); break;
			case _EXIT:	c_exit(); break;
			case _GOTO:	c_goto(); return;
			case _LIST:	c_list(); break;
			case _LOAD:	c_load(); break;
			case _MOVE:	c_cpmov(1); break;
			case _NEXT:	c_next(); break;
			case _OPEN:	c_open(); break;
			case _POKE:	c_poke(); break;
			case _READ:	c_read(); break;
			case _SAVE:	c_save(); break;
			case _STOP:	error(ESTO); break;
			case _TRON:	tracing = 1; break;
			case _WEND:	c_wend(); break;
			case _CLEAR:	c_clear(); break;
			case _CLOSE:	c_close(); break;
			case _ENDIF:	if (!linep || linep+5 != intp)
						error(ESYN);
					break;
			case _ERASE:	c_erase(); break;
			case _FILES:	c_files(); break;
			case _GOSUB:	c_gosub(_GOSUB); return;
			case _INPUT:	c_input(); break;
			case _IOCTL:	c_ioctl(); break;
			case _LABEL:	c_label(); break;
			case _LOCAL:	c_local(1); break;
			case _PRINT:	c_print(); break;
			case _RENUM:	c_renum(); break;
			case _TROFF:	tracing = 0; break;
			case _UNTIL:	c_until(); break;
			case _WHILE:	c_while(); break;
			case _ENDLC:	c_endloc(1); break;
			case _RETRN:	c_return();
					if (retflg)
						return;
					break;
			case _TXT:	intp--;
			case _SEARC:	c_search(); break;
			case _SYSTM:	exit(0); break;
			case _RPEAT:	c_repeat(); break;
			case _RSTOR:	c_restore(); break;
			case 0:		return;
			default:	error(ESYN);
		}
		if (*intp == _COLON)
			intp++;
	}
}

void cont(int sub) {
	while (*linep) {
		trace();
		interpret(linep+4);
		if (retflg || !linep)
			return;
		if (advance)
			linep += *linep;
	}
}

void run(byte *pp) {
	isp = 0;
	interpret(pp);
	if (linep)
		cont(0);
	if (isp)
		error(ENST);
}

void delete(byte *addr, int len) {
	byte	*p;

	for (p=addr; p<ptop; p++)
		*p = p[len];
	ptop -= len;
	*ptop = 0;
}

void insert(byte *addr, int len) {
	byte	*p;

	if (ptop + len >= sbufs)
		error(EMEM);
	for (p=ptop; p>=addr; p--)
		p[len] = *p;
	ptop += len;
}

void store(byte *src, int len) {
	byte	*addr;
	int	lno;

	lno = src[1] | src[2]<<8;
	if (lno != (lno & 0x7fff))
		error(ELIN);
	addr = findln(lno, 0);
	if (len < 5) {
		edit(addr);
		return;
	}
	if (lno == lineno(addr))
		delete(addr, *addr);
	insert(addr, len+1);
	*addr = len+1;
	memcpy(++addr, src, len);
}

void process(byte *ln) {
	linep = 0;
	labels[lbp = 0] = 0;
	scp = tokenize(ln, scratch);
	if (*scratch == _INT)
		store(scratch, scp);
	else if (*scratch)
		run(scratch);
}

void kbint(int foo) {
	signal(SIGINT, kbint);
	error(EBRK);
}

void init(void) {
	int	i;

	tokens=	"\2#" "\2'" "\2(" "\2)" "\2*" "\2+" "\2," "\2-"
		"\2/" "\2:" "\2;" "\2<" "\2=" "\2>"
		"\3<=" "\3<>" "\3>=" "\3ED" "\3IF" "\3ON" "\3OR"
		"\3TO"
		"\4AND" "\4DEF" "\4DEL" "\4DIM" "\4END"
		"\4FOR" "\4INS" "\4LET" "\4NEW" "\4NOT"
		"\4REM" "\4RUN"
		"\5ABS(" "\5ASC(" "\5COPY" "\5DATA" "\5ELSE"
		"\5EOF(" "\5EXIT" "\5FRE(" "\5GOTO" "\5LEN("
		"\5LIST" "\5LOAD" "\5MOVE" "\5NEXT" "\5OPEN"
		"\5POKE" "\5POS(" "\5READ" "\5RND(" "\5SAVE"
		"\5STEP" "\5STOP" "\5THEN" "\5TRON" "\5VAL("
		"\5WEND" "\5WITH"
		"\6CALL(" "\6CHR$(" "\6CLEAR" "\6CLOSE"
		"\6ENDIF" "\6ERASE" "\6FILES" "\6GOSUB"
		"\6INPUT" "\6IOCTL" "\6LABEL" "\6LOCAL"
		"\6MID$(" "\6PEEK(" "\6PRINT" "\6RENUM"
		"\6STR$(" "\6TROFF" "\6UNTIL" "\6WHILE"
		"\7ENDLOC" "\7REPEAT" "\7RETURN" "\7SEARCH"
		"\7SYSTEM"
		"\10RESTORE";
	msgs =	" OK\0LLO\0TLO\0OVF\0STR\0SYN\0OPR\0LIN\0"
		"DIV\0SBO\0SUB\0MEM\0NYI\0STK\0NST\0TYP\0"
		"EOD\0ARG\0DEV\0IOX\0INT\0RNG\0STO\0BRK\0"
		"LAB\0DUP\0FUN\0XXX";
	for (i=0; i<NFIL; i++) {
		files[i] = NULL;
		fnames[i] = NULL;
	}
	files[idev = 0] = stdin;
	files[odev = 1] = stdout;
	iod = -1;
	quiet = 0;
	c_new();
	signal(SIGINT, kbint);
	srand(time(NULL));
}

void banner(void) {
	putstr(" __  __ _ __  _ ____   __   ____ _  ____ \n");
	putstr("|  \\/  | |  \\| |    ) /  \\ / ___) |/  __)\n");
	putstr("|      | |     | __ \\/    \\\\___ \\ |  |__ \n");
	putstr("|_|\\/|_|_|_|\\__|____/__/\\__\\____/_|\\____)\n");
	putstr("[]-------------------------------------[]\n");
	putstr(" |  MINBASIC BY NMH  1991-2011  FREE!  |\n");
	putstr("[]-------------------------------------[]\n");
}

int main(int argc, char **argv) {
	int	fn = 2, c, prog = 0;

	runflg = 0;
	svars = space;
	fvars = &space[NSTR*_BPW_];
	ivars = &space[(NSTR+NFUN)*_BPW_];
	memtop = &space[WSPL-1];
	init();
	if (argc > 1 && !strcmp(argv[1], "@")) {
		prog = runflg = 1;
		argv++;
		argc--;
	}
	if (!runflg)
		banner();
	if (argc > 1 && !runflg)
		putch('\n');
	while (--argc) {
		c = 0;
		if ((files[fn] = fopen(*++argv, "r+")) == NULL) {
			if (!prog)
				files[fn] = fopen(*argv, "w+");
			c = 1;
		}
		if (files[fn] == NULL) {
			if (!runflg)
				fprintf(files[odev], "   ! %s\n", *argv);
		}
		else {
			fnames[fn] = strdup(*argv);
			if (!runflg) {
				fprintf(files[odev], "#%d = %s",
					fn, *argv);
				if (c)
					putstr(" (created)");
				putch('\n');
			}
		}
		fn++;
		prog = 0;
	}
	if (!runflg)
		putch('\n');
	if (setjmp(restart))
		exit(errval? 1: 0);
	if (runflg) {
		process(strcpy(line, "LOAD#2"));
		while (readline(line) >= 0)
			process(line);
		rewind(files[2]);
		idev = 0;
		process(strcpy(line, "RUN"));
	}
	setjmp(restart);
	while (readline(line) >= 0) {
		process(line);
		if (idev == 0)
			error(EOK);
	}
	if (iod >= 0) {
		rewind(files[idev]);
		idev = iod;
		iod = -1;
		error(EOK);
	}
	clearerr(stdin);
#ifdef unix
	if (isatty(0))
#endif
		error(EOK);
	return 0;
}
