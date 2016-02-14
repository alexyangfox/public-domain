///////////////////////////////////////////////////////////////////////////////
//
//	12K BASIC -- A Minimal BASIC Interpreter
//
//		    BASYL-II Version
//
//	  Copyright (C) 1991-1994 Nils M. Holm
//
//	This program is FREE SOFTWARE, and comes
//	          WITHOUT ANY WARRANTY.
//
//	COMPILE: BSL -G300 BASIC.BSL
//
///////////////////////////////////////////////////////////////////////////////

get "stdlib"


$def ARGLEN	256
$def BUFLEN	256

$def DO_GLOB	TRUE


const	S_LEN = 6,
	S.LP = 0,
	S.IP = 1,
	S.VP = 2,
	S.LIM = 3,
	S.STP = 4,
	S.ID = 5;

const	BUFL = 64,
	NTOK = 39,
	TOKL = 6,
	BASE = 10,
	NVAR = 260,
	NSTR = 36,
	WSPC = 8192,
	STKL = 16*S_LEN;

const	EOK  = 0,
	ELLO = 1,
	ETLO = 2,
	EOVF = 3,
	ESTR = 4,
	ESYN = 5,
	EIOP = 6,
	EILN = 7,
	EDIV = 8,
	ESBO = 9,
	ESUB = 10,
	EMEM = 11,
	ENIM = 12,
	ESTK = 13,
	ENST = 14,
	ETYP = 15,
	EEOD = 16,
	EARG = 17,
	EXXX = 18;

const	MIN_SERV = 100,
	MAX_SERV = 101,
	IOC_RWND = 100,
	IOC_APPD = 101;

const	_NOT = 1,
	_QUOTE = 2,
	_LPAR = 3,
	_RPAR = 4,
	_MUL = 5,
	_PLUS = 6,
	_COMMA = 7,
	_MINUS = 8,
	_SLASH = 9,
	_COLON = 10,
	_SEMI = 11,
	_LESS = 12,
	_EQU = 13,
	_GRTR = 14,
	_LSEQ = 15,
	_NEQ = 16,
	_GTEQ = 17,
	_TO = 18,
	_IF = 19,
	_DIM = 20,
	_END = 21,
	_FOR = 22,
	_LET = 23,
	_NEW = 24,
	_REM = 25,
	_RUN = 26,
	_ASC = 27,
	_DATA = 28,
	_GOTO = 29,
	_LEN = 30,
	_CALL = 31,
	_LIST = 32,
	_LOAD = 33,
	_NEXT = 34,
	_POKE = 35,
	_READ = 36,
	_SAVE = 37,
	_STEP = 38,
	_STOP = 39,
	_TRON = 40,
	_VAL = 41,
	_CHRS = 42,
	_CMPS = 43,
	_CLEAR = 44,
	_GOSUB = 45,
	_INPUT = 46,
	_MIDS = 47,
	_PEEK = 48,
	_PRINT = 49,
	_STRS = 50,
	_TROFF = 51,
	_IOCTL = 52,
	_RSTOR = 53,
	_RETRN = 54,
	_SYSTM = 55;

const	_INT = 254,
	_VAR = 253,
	_STR = 252,
	_TXT = 251;


static	line, scratch, scp, spad;
static	tokens, msgs;
static	space, memtop, ptop;
static	progp, ivars, svars, sbufs;
static	intp, linep, tracing;
static	idev, odev, iod = -1;
static	istack, isp;
static	advance;
static	datal, datap;


puts(s) SYS.write(odev, s, length(s));


putc(c) { local s=vec 1; s::0 = c; s::1 = 0; puts(s); }


putd(n) {
	local	nb = vec 5;
	puts(intstr(n, -10, nb));
}


list(p) {
	local	t, tp, v;
	global	error:297;

	while (::p) {
		putc('*S');
		if (::p == _INT) {
			p += 2;
			putd(::(p-1) \/ ::p<<8, 0);
		}
		else if (::p == _VAR) {
			p += 2;
			v = (::(p-1) \/ ::p<<8) - ivars;
			putc(v/(10*_BPW_)+'A');
			putc(v%(10*_BPW_)/_BPW_+'0');
		}
		else if (::p == _STR) {
			p += 2;
			v = ((::(p-1) \/ ::p<<8)-svars)/_BPW_;
			putc(v>=26-> v-26+'0', v+'A');
			putc('$');
		}
		else if (::p == _TXT) {
			putc('\'');
			while (::(p+=1)) {
				putc(::p); if (::p=='\'') putc('\'');
			}
			putc('\'');
		}
		else {
			tp = tokens; t = 1;
			while (::tp) {
				if (::p == t) break;
				t += 1;
				tp += ::tp;
			}
			if (\(::tp)) error(EXXX);
			for (t=1, ::tp) putc(tp::t);
		}
		p += 1;
	}
	puts("*R*N");
}


error(e) {
	static	rframe, rlimit;
	forward label	restart;

	if (e) puts("\*\*");
	puts(msgs+(e<<2));
	if (e /\ e \= EXXX) {
		puts(" : ");
		if (linep) list(linep+1); else puts(line);
	}
	puts("*R*N");
	iod = -1;
	idev = SYSIN;
	longjmp(restart, rframe, rlimit);
}


readline(s) {
	local	l;

	(IOS.buf+IOS.input*IOS.LEN)!IOS.FILENO = idev;
	if ((l = reads(s, BUFL+1)) < 1) return -1;
	if (l >= BUFL) error(ELLO);
	s::(l-1) = 0;
	return l-1;
}

 
toupr(c) return ('a' <= c /\ c <= 'z') -> c-32, c;


upcase(s) while (::s) {
		::s = toupr(::s);
		s += 1;
	}


isalpha(c) {
	return ('A' <= c /\ c <= 'Z');
}


isdig(c) {
	return ('0' <= c /\ c <= '9');
}


gen(t) {
	scratch::scp = t;
	scp += 1;
}


genp(p, v) {
	gen(p); gen(v&#XFF); gen(v>>8);
}


kwfind(k, err) {
	local	p = tokens, t=1;

	while (::p) {
		if (\memcomp(p, k, ::p)) {
			gen(t);
			return;
		}
		p += ::p;
		t += 1;
	}
	error(err);
}


kw(in) {
	local	kwb = vec TOKL/_BPW_+1, p = 1;

	while(isalpha(::in) \/ ::in == '$' \/ ::in == '(') {
		kwb::p = ::in;
		if ((p += 1) > TOKL+1) error(ETLO);
		in += 1;
		if (::(in-1) == '(') break;
	}
	kwb::0 = p;
	kwb::p = 0;
	kwfind(kwb, ESYN);
	return in;
}


oper(in) {
	local	kwb = vec 4/_BPW_;

	kwb::0 = 2;
	kwb::1 = ::in;
	kwb::2 = kwb::3 = 0;
	if (::in == '<' \/ ::in == '>')
		if (in::1 == '=' \/ in::1 == '>' /\ kwb::1 == '<') {
			kwb::2 = ::(in+=1);
			::kwb += 1;
		}
	kwfind(kwb, EIOP);
	return in+1;
}


svar(in) {
	if ('0' <= ::in /\ ::in <= '9')
		genp(_STR, (::in-'0'+26)*_BPW_+svars);
	else
		genp(_STR, (::in-'A')*_BPW_+svars);
	return in+2;
}


var(in) {
	local	addr;

	if (in::1 == '$') return svar(in);

	addr = ivars + (::in-'A')*10*_BPW_;
	if ('0' <= in::1 /\ in::1 <= '9') {
		in += 1;
		addr += (::in-'0')*_BPW_;
	}
	genp(_VAR, addr);
	return in+1;
}


num(in) {
	local	v, o;

	if (in::1 == '$') return svar(in);

	while (isdig(::in)) {
		o = v;
		v *= BASE;
		if (v < o) error(EOVF);
		if (v/BASE \= o) error(EOVF);
 		v += (::in-'0');
		if (v < o) error(EOVF);
		in += 1;
	}
	genp(_INT, v);
	return in;
}


text(in) {
	gen(_TXT);
	in += 1;
	while (TRUE) {
		if (\(::in)) error(ESTR);
		if (::in == '\'') {
			if (::(in+=1) == '\'')
				gen('\'');
			else
				break;
		}
		else
			gen(::in);
		in += 1;
	}
	gen(0);
	return in;
}


tokenize(in) {
	scp = 0;
	upcase(in);
	while (::in) {
		while (::in == '*S' \/ ::in == '*T') in += 1;
		if (\(::in)) break;
		if (isalpha(::in)) {
			if (isalpha(::(in+1))) in = kw(in);
			else in = var(in);
		}
		else if (isdig(::in)) in = num(in);
		else if (::in == '\'') in = text(in);
		else in = oper(in);
	}
	gen(0);
}


moveprog(new) {
	local	i, len;

	if (progp == new) return;
	len = ptop-progp+1;
	if (new < progp)
		for (i=0, len) new!i = progp!i;
	else
		for (i=len-1, -1, -1) new!i = progp!i;
	ptop += (new-progp);
	progp = new;
}


findln(lno) {
	local	cl, p;

	p = progp;
	while (::p) {
		cl = p::2 \/ p::3<<8;
		if (cl >= lno) return p;
		p += ::p;
	}
	return ptop;
}


lineno(addr) return(addr::2 \/ addr::3<<8);


variable() {
	global	expr:299;
	local	v;

	intp += 3; v = (::(intp-2) \/ ::(intp-1)<<8);
	if (::intp == _LPAR) {
		intp += 1;
		v += (expr() * _BPW_);
		if (::intp \= _RPAR) error(ESYN);
		intp += 1;
	}
	if (v |>= progp) error(ESUB);
	return v;
}


match(t) ::intp == t-> intp+=1, error(ESYN);


f.asc() {
	global	strexpr:298;
	local	sp;

	sp = strexpr();
	match(_RPAR);
	return ::sp;
}


f.cmps() {
	global	strfac:295;
	local	s1, s2, l1, l2;

	l1 = length(s1 = strfac());
	match(_COMMA);
	l2 = length(s2 = strfac());
	match(_RPAR);
	return memcomp(s1, s2, l1>l2 -> l1, l2);
}


f.len() {
	local	sp;

	sp = strexpr();
	match(_RPAR);
	return length(sp);
}


f.val() {
	local	sp;

	sp = strexpr();
	match(_RPAR);
	return ascint(sp);
}


f.peek() {
	local	vp;

	vp = expr();
	match(_RPAR);
	return space::vp;
}


f.ioctl() {
	local	dev, service, rc;

	dev = expr();
	match(_COMMA);
	service = expr();
	match(_RPAR);
	switch(service) {
		case IOC_RWND:	rc = SYS.rewind(dev); break;
		case IOC_APPD:	rc = SYS.append(dev); break;
		default:	error(EARG);
	}

	return rc;
}


fac() {
	local	v;

	switch(::intp) {
		case _INT:	intp += 3; v = (::(intp-2) \/ ::(intp-1)<<8);
				return v;
		case _VAR:	return !variable();
		case _LPAR:	intp += 1; v = expr();
				if (::intp \= _RPAR) error(ESYN);
				intp += 1; return v;
		case _MINUS:	intp += 1; return -fac();
		case _NOT:	intp += 1; return \fac();
		case _ASC:	intp += 1; return f.asc();
		case _CMPS:	intp += 1; return f.cmps();
		case _VAL:	intp += 1; return f.val();
		case _LEN:	intp += 1; return f.len();
		case _PEEK:	intp += 1; return f.peek();
		case _IOCTL:	intp += 1; return f.ioctl();
		default:	error(ESYN);
	}
}


term() {
	local	v = fac(), v2;

	while (TRUE) switch (::intp) {
		case _MUL:	intp += 1; v *= fac(); break;
		case _SLASH:	intp += 1;
				if ((v2 = fac()) == 0) error(EDIV);
				v /= v2; break;
		default:	return v;
	}
}


sum() {
	local	v = term();

	while (TRUE) switch (::intp) {
		case _PLUS:	intp += 1; v += term(); break;
		case _MINUS:	intp += 1; v -= term(); break;
		default:	return v;
	}
}


expr() {
	local	v = sum();

	while (TRUE) switch (::intp) {
		case _EQU:	intp += 1; v = (v == sum()); break;
		case _NEQ:	intp += 1; v = (v \= sum()); break;
		case _LESS:	intp += 1; v = (v < sum()); break;
		case _GRTR:	intp += 1; v = (v > sum()); break;
		case _LSEQ:	intp += 1; v = (v <= sum()); break;
		case _GTEQ:	intp += 1; v = (v >= sum()); break;
		default:	return v;
	}
}


strvar() {
	local	sv;

	sv = ::(intp+1)\/::(intp+2)<<8; intp += 3;
	if (::intp == _LPAR) {
		intp += 1;
		sv += (expr() * _BPW_);
		if (::intp \= _RPAR) error(ESYN);
		intp += 1;
	}
	if (sv |>= ivars) error(ESUB);
	return sv;
}


sf.chr() {
	local	v;

	v = expr();
	match(_RPAR);
	spad::0 = v;
	spad::1 = 0;
	return spad;
}


sf.mid() {
	local	s1, ln=BUFL, str;

	str = strexpr();
	match(_COMMA);
	s1 = expr();
	if (::intp == _COMMA) {
		match(_COMMA);
		ln = expr();
	}
	match(_RPAR);
	if ((s1 -= 1) < 0) s1 = 0;
	if (length(str) < s1) return "";
	if (length(@(str::s1)) < ln) return @(str::s1);
	memcopy(spad, @(str::s1), BUFL/_BPW_);
	spad::ln = 0;
	return spad;
}


sf.str() {
	local	v;

	v = expr();
	match(_RPAR);
	intstr(v, -10, spad);
	return spad;
}


strfac() {
	local	sv;

	switch (::intp) {
		case _TXT:	sv = (intp+=1); while (::intp) intp += 1;
				intp += 1; return sv;
		case _STR:	sv = !strvar(); return \sv-> "", sv;
		case _CHRS:	intp += 1; return sf.chr();
		case _MIDS:	intp += 1; return sf.mid();
		case _STRS:	intp += 1; return sf.str();
		default:	error(ESYN);
	}
}


strexpr() {
	local	sv = strfac(), sv2, l1, l2;

	while (TRUE) switch (::intp) {
		case _PLUS:	sv = memcopy(spad, sv, BUFL/_BPW_);
				intp += 1; sv2 = strfac();
				if ((l1=length(sv)) + (l2=length(sv2)) > BUFL)
					error(ESBO);
				memcopy(@(sv::l1), sv2, (l2+_BPW_)/_BPW_);
				break;
		default:	return sv;
	}
}


push(sp) {
	if (isp >= STKL-S_LEN) error(ESTK);
	memcopy(@(istack!isp), sp, S_LEN);
	isp += S_LEN;
}


pick(sp) {
	if (isp < S_LEN) error(ESTK);
	memcopy(sp, @(istack!isp-S_LEN), S_LEN);
	return sp!S.ID;
}


pop() if ((isp -= S_LEN) < 0) error(ESTK);


trace() if (tracing /\ linep) {
		puts(" ["); putd(lineno(linep)); puts("] ");
	}


c_if() {
	intp += 1;
	while (TRUE) {
		if (\expr()) {
			if (\linep) error(EOK);
			linep += ::linep;
			intp = @(linep::4);
			return;
		}
		if (::intp \= _COMMA) break;
		intp += 1;
	}
}


let.var() {
	local	dest = variable();

	::intp == _EQU-> intp+=1, error(ESYN);
	!dest = expr();
}


allocstr() {
	local	p;

	p = sbufs;
	while (::p) {
		if (\(::p & 128)) {
			::p |= 128;
			return p+1;
		}
		p += (::p & 127);
	}
	if ((sbufs -= BUFL+2) |< ptop) {
		sbufs += BUFL+2;
		error(EMEM);
	}
	::sbufs = (BUFL+2 | 128);
	return sbufs+1;
}


freestr(s) {
	if (s-1 == sbufs)
		sbufs += BUFL+2;
	else
		::(s-1) &= 127;
}


asg.str(dp, s) {
	if (!dp == NULL) {
		if (\(::s)) return;
		!dp = allocstr();
	}
	else if (\(::s)) {
		freestr(!dp);
		!dp= 0;
		return;
	}
	memcopy(!dp, s, BUFL/_BPW_);
}


let.str() {
	local	destp, val;

	destp = strvar();
	::intp == _EQU-> intp+=1, error(ESYN);
	val = strexpr();
	asg.str(destp, val);
}


c_dim() {
	local	v, diff;

	intp += 1;
	while (TRUE) {
		if (::intp \= _VAR) error(ESYN);
		intp += 3; v = (::(intp-2) \/ ::(intp-1)<<8);
		match(_LPAR);
		v += expr()*_BPW_;
		match(_RPAR);
		if (v + (ptop-progp) |>= sbufs \/ v |< space) error(EMEM);
		if (v |>= progp) {
			moveprog(v);
			diff = v-progp;
			intp += diff;
			linep += diff;
		}
		if (::intp \= _COMMA) break;
		intp += 1;
	}
}


c_for() {
	local	sp = vec S_LEN;

	intp += 1;
	sp!S.VP = variable();
	match(_EQU);
	!(sp!S.VP) = expr();
	match(_TO);
	sp!S.LIM = expr();
	if (::intp == _STEP) {
		intp += 1;
		sp!S.STP = expr();
	}
	else
		sp!S.STP = 1;
	sp!S.LP = linep;
	sp!S.IP = intp;
	sp!S.ID = _FOR;
	push(sp);
}


c_next() {
	local st, vp, sp = vec S_LEN;

	intp += 1;
	if (pick(sp) \= _FOR) error(ENST);
	pop();
	!(vp = sp!S.VP) += (st = sp!S.STP);
	if (st>=0-> !vp>sp!S.LIM, !vp<sp!S.LIM) return;
	push(sp);
	linep = sp!S.LP;
	intp = sp!S.IP;
	trace();
}


c_let() {
	switch (::(intp+=1)) {
		case _VAR:	let.var(); break;
		case _STR:	let.str(); break;
		default:	error(ESYN);
	}
}


c_clear() {
	intp += 1;
	memfill(ivars, 0, NVAR);
	memfill(svars, 0, NSTR);
	::(sbufs = memtop) = 0;
	moveprog(@(space!NVAR+NSTR));
	isp = 0;
	datal = datap = 0;
}


c_new() {
	ptop = progp = @(space!NSTR+NVAR);
	c_clear();
	::progp = 0;
	ptop = progp;
	tracing = FALSE;
}


c_load() {
	if (iod \= -1) error(ENST);
	match(_NOT);
	c_new();
	iod = idev;
	idev = expr();
}


c_run() {
	c_clear();
	linep = progp;
}


c_goto() {
	local	lno, addr;

	intp += 1;
	addr = findln(lno = expr());
	if (lineno(addr) \= lno) error(EILN);
	linep = addr; advance = FALSE;
}


c_list() {
	local	sl, el = 32767, p;

	if (::(intp += 1)) {
		sl = expr();
		if (::intp == _COMMA) {
			intp += 1; el = expr();
		}
	}
	p = progp;
	while (::p /\ lineno(p) < sl) p += ::p;
	while (::p /\ lineno(p) <= el) {
		list(p+1);
		p += ::p;
	}
}


c_poke() {
	local	a;

	intp += 1;
	a = expr();
	match(_COMMA);
	space::a = expr();
}


nextdata(type) {
	local	v;

	if (\datap \/ \(::datap)) {
		if (datal) datal += ::datal; else datal = progp;
		while (::datal) {
			if (datal::4 == _DATA) break;
			datal += ::datal;
		}
		if (\(::datal)) error(EEOD);
		datap = @(datal::5);
	}
	if (::datap \= type) error(ETYP);
	if (type == _INT) {
		v = ::(datap+1)\/::(datap+2)<<8;
		datap += 3;
	}
	else {
		v = datap+1;
		while (::datap) datap += 1;
		datap += 1;
	}
	return v;
}


c_read() {
	intp += 1;
	while (TRUE) {
		switch(::intp) {
			case _VAR:	!variable() = nextdata(_INT); break;
			case _STR:	asg.str(strvar(), nextdata(_TXT));
					break;
			default:	error(ESYN);
		}
		if (::intp \= _COMMA) break;
		intp += 1;
	}
}


c_save() {
	local	p = progp, ood = odev;

	intp += 1;
	match(_NOT);
	odev = expr();
	while (::p) {
		list(p+1);
		p += ::p;
	}
	odev = ood;
}


c_gosub() {
	local	lno, addr, sp = vec S_LEN;

	intp += 1;
	addr = findln(lno = expr());
	if (lineno(addr) \= lno) error(EILN);
	sp!S.LP = linep;
	sp!S.IP = intp;
	sp!S.ID = _GOSUB;
	push(sp);
	linep = addr; advance = FALSE;
}


c_input() {
	local	vp;

	if (::(intp+=1) == _NOT) {
		intp += 1;
		idev = expr();
		IOS.sync(IOS.input);
		return;
	}

	while (TRUE) {
		switch(::intp) {
			case _VAR:	vp = variable();
					readline(spad);
					!vp = ascint(spad);
					break;
			case _STR:	vp = strvar();
					readline(spad);
					asg.str(vp, spad);
					break;
			default:	error(ESYN);
		}
		if (::intp \= _COMMA) break;
		intp += 1;
	}
}


c_print() {
	local	doNL = TRUE;

	if (::(intp+=1) == _NOT) {
		intp += 1;
		odev = expr();
		return;
	}

	while (::intp /\ ::intp \= _COLON) {
		switch (::intp) {
			case _MINUS: case _NOT: case _LPAR:
			case _ASC: case _LEN: case _VAL: case _CMPS:
			case _PEEK: case _IOCTL:
			case _INT:
			case _VAR:	putd(expr(), 0); break;

			case _CHRS: case _MIDS: case _STRS:
			case _TXT:	
			case _STR:	puts(strexpr()); break;

			default:	error(ESYN);
		}
		if (::intp == _COMMA) {
			intp += 1; putc('*T'); doNL = FALSE;
		}
		else if (::intp == _SEMI) {
			intp += 1; doNL = FALSE;
		}
		else {
			doNL = TRUE; break;
		}
	}
	if (doNL) puts("*R*N");
}


c_return() {
	local	x, sp = vec S_LEN;

	if (pick(sp) \= _GOSUB) error(ENST);
	linep = sp!S.LP;
	intp = sp!S.IP;
	pop();
	trace();
}


c_system() {
	finish(1);
}


execute(pp) {
	local	oldpp = intp;

	intp = pp;
	advance = TRUE;
	while (TRUE) {
label next;
		switch (::intp) {
			case _IF:	c_if(); goto next;
			case _DIM:	c_dim(); break;
			case _END:	error(isp-> ENST, EOK);
			case _FOR:	c_for(); break;
			case _LET:	c_let(); break;
			case _NEW:	c_new(); break;
			case _REM:	return;
			case _RUN:	c_run(); return;
			case _CALL:	error(ENIM);
			case _DATA:	return;
			case _GOTO:	c_goto(); return;
			case _LIST:	c_list(); break;
			case _LOAD:	c_load(); break;
			case _NEXT:	c_next(); break;
			case _POKE:	c_poke(); break;
			case _READ:	c_read(); break;
			case _SAVE:	c_save(); break;
			case _STOP:	putd(linep); puts(" STOP*R*N");
					error(EOK);
			case _TRON:	intp += 1; tracing = TRUE; break;
			case _CLEAR:	c_clear(); break;
			case _GOSUB:	c_gosub(); return;
			case _INPUT:	c_input(); break;
			case _PRINT:	c_print(); break;
			case _RSTOR:	intp+=1; datap = datal = 0; break;
			case _RETRN:	c_return(); break;
			case _SYSTM:	c_system(); break;
			case _TROFF:	intp += 1; tracing = FALSE; break;
			default:	error(ESYN);
		}
		switch (::intp) {
			case _COLON:	intp += 1; break;
			case 0:		return;
			default:	error(ESYN);
		}
	}
}


run(pp) {
	isp = 0;
	execute(pp);
	if (linep) {
		while (::linep) {
			trace();
			execute(@(linep::4));
			if (\linep) break;
			if (advance) linep += ::linep;
		}
	}
	if (isp) error(ENST);
}


delete(addr) {
	local	i, k = ::addr;

	ufor (i=addr, ptop) ::i = ::(i+k);
	ptop -= k;
	::ptop = 0;
}


insert(addr, len) {
	local	i;

	if (ptop + len >= sbufs) error(EMEM);
	ufor (i=ptop, addr-1, -1) ::(i+len) = ::i;
	ptop += len;
}


store() {
	local	addr, lno, i;

	if ((lno = scratch::1 \/ scratch::2<<8) < 0) error(EILN);
	addr = findln(lno);
	if (lno == lineno(addr)) delete(addr);

	if (scp < 5) return;

	insert(addr, scp+1);
	::addr = scp+1;
	addr += 1;
	for (i=0, scp) addr::i = scratch::i;
}


process(ln) {
	linep = 0;
	tokenize(ln);
	if (::scratch == _INT)
		store();
	else if (::scratch)
		run(scratch);
}


init() {
	local	p;

	tokens=	"\002#" "\002'" "\002(" "\002)" "\002**" "\002+"
		"\002," "\002-" "\002/" "\002:" "\002;" "\002<"
		"\002=" "\002>"
		"\003<=" "\003<>" "\003>=" "\003TO" "\003IF"
		"\004DIM" "\004END" "\004FOR" "\004LET" "\004NEW"
		"\004REM" "\004RUN"
		"\005ASC(" "\005DATA" "\005GOTO" "\005LEN(" "\005CALL"
		"\005LIST" "\005LOAD" "\005NEXT" "\005POKE" "\005READ"
		"\005SAVE" "\005STEP" "\005STOP" "\005TRON" "\005VAL("
		"\006CHR$(" "\006CMPS(" "\006CLEAR" "\006GOSUB"
		"\006INPUT" "\006MID$(" "\006PEEK(" "\006PRINT"
		"\006STR$(" "\006TROFF"
		"\007IOCTL(" "\007RESTOR" "\007RETURN" "\007SYSTEM";

	msgs =	" OK.LLO.TLO.OVF.STR.SYN.IOP.ILN.DIV.SBO.SUB.MEM.NIM."
		"STK.NST.TYP.EOD.ARG.XXX";
	p = msgs;
	while (::p) {
		if (::p == '.') ::p = 0;
		p += 1;
	}

	idev = SYSIN;
	odev = SYSOUT;

	c_new();
}


start() {
	local	p=0, i, arg = vec 128, dev;
	local	line_v = vec BUFL/_BPW_,
		scratch_v = vec BUFL/_BPW_,
		space_v = vec WSPC/_BPW_,
		spad_v = vec BUFL/_BPW_,
		istack_v = vec STKL;

	puts("12K-BASIC 1.0 Copyright 1994 Nils M. Holm*R*N");
	while (findarg(p+=1, arg)) {
		if ((dev = SYS.open(arg, S.RDWR)) < 0) {
			puts("! "); puts(arg);
		}
		else {
			puts("#"); putd(dev); puts(" = "); puts(arg);
		}
		puts("*R*N");
	}

	scratch = scratch_v;
	line = line_v;
	space = space_v;
	spad = spad_v;
	istack = istack_v;

	svars = space;
	ivars = @(space!NSTR);
	memtop = @(space::WSPC-1);

	init();

label restart;
	rframe = frame();
	rlimit = limit();
	while (readline(line) >= 0) {
		process(line);
		if (idev == SYSIN) error(EOK);
	}
	if (iod \= -1) {
		idev = iod; iod = -1; error(EOK);
	}
}
