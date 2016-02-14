%   File   : READ.PL
%   Author : D.H.D.Warren + Richard O'Keefe
%   Updated: 5 July 1984 
%   Purpose: Read Prolog terms in Dec-10 syntax.
/*
    Modified by Alan Mycroft to regularise the functor modes.
    This is both easier to understand (there are no more '?'s),
    and also fixes bugs concerning the curious interaction of cut with
    the state of parameter instantiation.

    Since this file doesn't provide "metaread", it is considerably
    simplified.  The token list format has been changed somewhat, see
    the comments in the RDTOK file.

    I have added the rule X(...) -> apply(X,[...]) for Alan Mycroft.

:- public
	read_clause/2.

:- mode
	after_prefix_op(+, +, +, +, +, -, -),
	ambigop(+, -, -, -, -, -),
	cant_follow_expr(+, -),
	expect(+, +, -),
	exprtl(+, +, +, +, -, -),
	exprtl0(+, +, +, -, -),
	infixop(+, -, -, -),
	postfixop(+, -, -),
	prefixop(+, -, -),
	prefix_is_atom(+, +),
	portable_read(?, ?),
	read(+, +, -, -),
	read(+, +, +, -, -),
	read_args(+, -, -),
	read_list(+, -, -),
	syntax_error(+),
	syntax_error(+, +).
*/

op(Pri,Assoc,Name):-make_default_ops,make_op(Pri,Assoc,Name).

make_op(Pri,Assoc,Name):-
	let(Name,Assoc,Pri),
	push(current_op,op(Pri,Assoc,Name)).

make_default_ops:-stack(current_op,_),!.
make_default_ops:-default_op(Op,Assoc,Pri),make_op(Pri,Assoc,Op),fail.
make_default_ops.

current_op(Pri,Assoc,Name):-val(Name,Assoc,Pri),!.
current_op(Pri,Assoc,Name):-stack(current_op,OPs),!,
	member(op(Pri,Assoc,Name),OPs).
current_op(Pri,Assoc,Name):-
	default_op(Name,Assoc,Pri).

default_op((:-),Assoc,Pri):-multiple_op1(Assoc,Pri).

default_op((-->),xfx,1200).
default_op((?-),fx,1200).

default_op((;),xfy,1100).
default_op((->),xfy,1050).
default_op(( ',' ) , xfy, 1000).

default_op(\+,fy,900).
default_op(not,fy,900).
default_op(=,xfx,700).
default_op(is,xfx,700).
default_op(=..,xfx,700).
default_op(==,xfx,700).
default_op(\==,xfx,700).
default_op(@<,xfx,700).
default_op(@>,xfx,700).
default_op(@=<,xfx,700).
default_op(@>=,xfx,700).
default_op(=:=,xfx,700).
default_op(=\=,xfx,700).
default_op(<,xfx,700).
default_op(=<,xfx,700).
default_op(>,xfx,700).
default_op(>=,xfx,700).

default_op('.',xfy,650).

default_op(+,Assoc,Pri):-multiple_op2(Assoc,Pri).
default_op(-,Assoc,Pri):- multiple_op2(Assoc,Pri).

default_op(/\,yfx,500).
default_op(\/,yfx,500).
default_op(#,yfx,500).

default_op(*,yfx,400).
default_op(/,yfx,400).
default_op(//,yfx,400).
default_op(<<,yfx,400).
default_op(>>,yfx,400).
default_op(mod,xfx,300).

default_op(^,xfy,200).
default_op(~,fy,300).

% default_op(&,xfy,800).
default_op(:,yfx,900).
default_op(:=,xfx,700).
default_op(=:,xfx,700).
default_op(<=,xfx,600).
default_op(=>,xfx,600).

default_op((dynamic),fx,1150).

/*
default_op((mode),fx,1150).
default_op((public),fx,1150).
default_op((multifile),fx,1150).
default_op((wait),fx,1150).
default_op(spy,fy,900).
default_op(nospy,fy,900).
*/

multiple_op1(xfx,1200). % :-
multiple_op1(fx,1200).

multiple_op2(yfx,500). % -
multiple_op2(fx,500).

%   read_term(?Answer, ?Variables)
%   reads a term from the current input stream and unifies it with
%   Answer.  Variables is bound to a list of [Atom=Variable] pairs.

read_term(Answer, Variables) :-
	repeat,
	    read_tokens(Tokens, Variables),
	    read_and_check(Tokens,Term),
	!,
	Answer = Term.

read_and_check(Tokens,Term):-
	read(Tokens, 1200, Term, LeftOver),
	all_read(LeftOver).
read_and_check(Tokens,_):-
	syntax_error(Tokens).

%   all_read(+Tokens)
%   checks that there are no unparsed tokens left over.

all_read([]) :- !.
all_read(S) :-
	syntax_error([operator,expected,after,expression], S).


%   expect(Token, TokensIn, TokensOut)
%   reads the next token, checking that it is the one expected, and
%   giving an error message if it is not.  It is used to look for
%   right brackets of various sorts, as they're all we can be sure of.

expect(Token, [Token|Rest], Rest) :- !.
expect(Token, S0, _) :-
	syntax_error([Token,or,operator,expected], S0).


%   I want to experiment with having the operator information held as
%   ordinary Prolog facts.  For the moment the following predicates
%   remain as interfaces to current_op.
%   prefixop(O -> Self, Rarg)
%   postfixop(O -> Larg, Self)
%   infixop(O -> Larg, Self, Rarg)


prefixop(Op, Prec, Prec) :-
	current_op(Prec, fy, Op), !.
prefixop(Op, Prec, Less) :-
	current_op(Prec, fx, Op), !,
	Less is Prec-1.

postfixop(Op, Prec, Prec) :-
	current_op(Prec, yf, Op), !.
postfixop(Op, Less, Prec) :-
	current_op(Prec, xf, Op), !, Less is Prec-1.

infixop(Op, Less, Prec, Less) :-
	current_op(Prec, xfx, Op), !, Less is Prec-1.
infixop(Op, Less, Prec, Prec) :-
	current_op(Prec, xfy, Op), !, Less is Prec-1.
infixop(Op, Prec, Prec, Less) :-
	current_op(Prec, yfx, Op), !, Less is Prec-1.

ambigop(F, L1, O1, R1, L2, O2) :-
	postfixop(F, L2, O2),
	infixop(F, L1, O1, R1), !.

%   read(+TokenList, +Precedence, -Term, -LeftOver)
%   parses a Token List in a context of given Precedence,
%   returning a Term and the unread Left Over tokens.

read([], _, _, _) :- syntax_error([expression,expected], []).
read([Token|RestTokens], Precedence, Term, LeftOver) :-
	read(Token, RestTokens, Precedence, Term, LeftOver).

%   read(+Token, +RestTokens, +Precedence, -Term, -LeftOver)

read(var(Variable,_), ['('|S1], Precedence, Answer, S) :- !,
	read(S1, 999, Arg1, S2),
	read_args(S2, RestArgs, S3), !,
	exprtl0(S3, apply(Variable,[Arg1|RestArgs]), Precedence, Answer, S).
read(var(Variable,_), S0, Precedence, Answer, S) :- !,
	exprtl0(S0, Variable, Precedence, Answer, S).
read(atom(-), [integer(Integer)|S1], Precedence, Answer, S) :-
	Negative is 0-Integer, !,
	exprtl0(S1, Negative, Precedence, Answer, S).
read(atom(Functor), ['('|S1], Precedence, Answer, S) :- !,
	read(S1, 999, Arg1, S2),
	read_args(S2, RestArgs, S3),
	Term =.. [Functor,Arg1|RestArgs], !,
	exprtl0(S3, Term, Precedence, Answer, S).
read(atom(Functor), S0, Precedence, Answer, S) :-
	prefixop(Functor, Prec, Right), !,
	after_prefix_op(Functor, Prec, Right, S0, Precedence, Answer, S).
read(atom(Atom), S0, Precedence, Answer, S) :- !,
	exprtl0(S0, Atom, Precedence, Answer, S).
read(integer(Integer), S0, Precedence, Answer, S) :- !,
	exprtl0(S0, Integer, Precedence, Answer, S).
read('[', [']'|S1], Precedence, Answer, S) :- !,
	exprtl0(S1, [], Precedence, Answer, S).
read('[', S1, Precedence, Answer, S) :- !,
	read(S1, 999, Arg1, S2),
	read_list(S2, RestArgs, S3), !,
	exprtl0(S3, [Arg1|RestArgs], Precedence, Answer, S).
read('(', S1, Precedence, Answer, S) :- !,
	read(S1, 1200, Term, S2),
	expect(')', S2, S3), !,
	exprtl0(S3, Term, Precedence, Answer, S).
read('((', S1, Precedence, Answer, S) :- !,
	read(S1, 1200, Term, S2),
	expect(')', S2, S3), !,
	exprtl0(S3, Term, Precedence, Answer, S).
read('{', ['}'|S1], Precedence, Answer, S) :- !,
	exprtl0(S1, '{}', Precedence, Answer, S).
read('{', S1, Precedence, Answer, S) :- !,
	read(S1, 1200, Term, S2),
	expect('}', S2, S3), !,
	exprtl0(S3, '{}'(Term), Precedence, Answer, S).
read(string(List), S0, Precedence, Answer, S) :- !,
	exprtl0(S0, List, Precedence, Answer, S).
read(Token, S0, _, _, _) :-
	syntax_error([Token,cannot,start,an,expression], S0).


%   read_args(+Tokens, -TermList, -LeftOver)
%   parses {',' expr(999)} ')' and returns a list of terms.

read_args([','|S1], [Term|Rest], S) :- !,
	read(S1, 999, Term, S2), !,
	read_args(S2, Rest, S).
read_args([')'|S], [], S) :- !.
read_args(S, _, _) :-
	syntax_error([',)',expected,in,arguments], S).


%   read_list(+Tokens, -TermList, -LeftOver)
%   parses {',' expr(999)} ['|' expr(999)] ']' and returns a list 
%   of terms.

read_list([','|S1], [Term|Rest], S) :- !,
	read(S1, 999, Term, S2), !,
	read_list(S2, Rest, S).
read_list(['|'|S1], Rest, S) :- !,
	read(S1, 999, Rest, S2), !,
	expect(']', S2, S).
read_list([']'|S], [], S) :- !.
read_list(S, _, _) :-
	syntax_error(['|]',expected,in,list], S).


%   after_prefix_op(+Op, +Prec, +ArgPrec, +Rest, +Precedence, 
%       -Ans, -LeftOver)

after_prefix_op(Op, Oprec, _, S0, Precedence, _, _) :-
	Precedence < Oprec, !,
	syntax_error([prefix,operator,Op,in,context,with,precedence,Precedence],
	S0).
after_prefix_op(Op, Oprec, _, S0, Precedence, Answer, S) :-
	peepop(S0, S1),
	prefix_is_atom(S1, Oprec), % can't cut but would like to
	exprtl(S1, Oprec, Op, Precedence, Answer, S).
after_prefix_op(Op, Oprec, Aprec, S1, Precedence, Answer, S) :-
	read(S1, Aprec, Arg, S2),
	Term =.. [Op,Arg], !,
	exprtl(S2, Oprec, Term, Precedence, Answer, S).


%   The next clause fixes a bug concerning "mop dop(1,2)" where
%   mop is monadic and dop dyadic with higher Prolog priority.

peepop([atom(F),'('|S1], [atom(F),'('|S1]) :- !.
peepop([atom(F)|S1], [infixop(F,L,P,R)|S1]) :- infixop(F, L, P, R).
peepop([atom(F)|S1], [postfixop(F,L,P)|S1]) :- postfixop(F, L, P).
peepop(S0, S0).


%   prefix_is_atom(+TokenList, +Precedence)
%   is true when the right context TokenList of a prefix operator
%   of result precedence Precedence forces it to be treated as an
%   atom, e.g. (- = X), p(-), [+], and so on.

prefix_is_atom([Token|_], Precedence) :- prefix_is_atom(Token, Precedence).

prefix_is_atom(infixop(_,L,_,_), P) :- L >= P.
prefix_is_atom(postfixop(_,L,_), P) :- L >= P.
prefix_is_atom(')', _).
prefix_is_atom(']', _).
prefix_is_atom('}', _).
prefix_is_atom('|', P) :- 1100 >= P.
prefix_is_atom(',', P) :- 1000 >= P.
prefix_is_atom([],  _).


%   exprtl0(+Tokens, +Term, +Prec, -Answer, -LeftOver)
%   is called by read/4 after it has read a primary (the Term).
%   It checks for following postfix or infix operators.

exprtl0([atom(F)|S1], Term, Precedence, Answer, S) :-
	ambigop(F, L1, O1, R1, L2, O2), !,
	(   exprtl([infixop(F,L1,O1,R1)|S1], 0, Term, Precedence, 
		Answer, S)
	;   exprtl([postfixop(F,L2,O2) |S1], 0, Term, Precedence, 
		Answer, S)
	).
exprtl0([atom(F)|S1], Term, Precedence, Answer, S) :-
	infixop(F, L1, O1, R1), !,
	exprtl([infixop(F,L1,O1,R1)|S1], 0, Term, Precedence, Answer, S).
exprtl0([atom(F)|S1], Term, Precedence, Answer, S) :-
	postfixop(F, L2, O2), !,
	exprtl([postfixop(F,L2,O2) |S1], 0, Term, Precedence, Answer, S).
exprtl0([','|S1], Term, Precedence, Answer, S) :-
	Precedence >= 1000, !,
	read(S1, 1000, Next, S2), !,
	exprtl(S2, 1000, (Term,Next), Precedence, Answer, S).
exprtl0(['|'|S1], Term, Precedence, Answer, S) :-
	Precedence >= 1100, !,
	read(S1, 1100, Next, S2), !,
	exprtl(S2, 1100, (Term;Next), Precedence, Answer, S).
exprtl0([Thing|S1], _, _, _, _) :-
	cant_follow_expr(Thing, Culprit), !,
	syntax_error([Culprit,follows,expression], [Thing|S1]).
exprtl0(S, Term, _, Term, S).

cant_follow_expr(atom(_),	atom).
cant_follow_expr(var(_,_),	variable).
cant_follow_expr(integer(_),	integer).
cant_follow_expr(string(_),	string).
cant_follow_expr('((',		bracket).
cant_follow_expr('(',		bracket).
cant_follow_expr('[',		bracket).
cant_follow_expr('{',		bracket).

exprtl([infixop(F,L,O,R)|S1], C, Term, Precedence, Answer, S) :-
	Precedence >= O, C =< L, !,
	read(S1, R, Other, S2),
	Expr =.. [F,Term,Other], /*!,*/
	exprtl(S2, O, Expr, Precedence, Answer, S).
exprtl([postfixop(F,L,O)|S1], C, Term, Precedence, Answer, S) :-
	Precedence >= O, C =< L, !,
	Expr =.. [F,Term],
	peepop(S1, S2),
	exprtl(S2, O, Expr, Precedence, Answer, S).
exprtl([','|S1], C, Term, Precedence, Answer, S) :-
	Precedence >= 1000, C < 1000, !,
	read(S1, 1000, Next, S2), /*!,*/
	exprtl(S2, 1000, (Term,Next), Precedence, Answer, S).
exprtl(['|'|S1], C, Term, Precedence, Answer, S) :-
	Precedence >= 1100, C < 1100, !,
	read(S1, 1100, Next, S2), /*!,*/
	exprtl(S2, 1100, (Term;Next), Precedence, Answer, S).
exprtl(S, _, Term, _, Term, S).


%   This business of syntax errors is tricky.  When an error is 
%   detected, we have to write out a message.  We also have to note 
%   how far it was to the end of the input, and for this we are 
%   obliged to use the data-base.  Then we fail all the way back to 
%   read(), and that prints the input list with a marker where the 
%   error was noticed.  If subgoal_of were available in compiled code 
%   we could use that to find the input list without hacking the 
%   data base.  The really hairy thing is that the original code 
%   noted a possible error and backtracked on, so that what looked 
%   at first sight like an error sometimes turned out to be a wrong 
%   decision by the parser.  This version of the parser makes
%   fewer wrong decisions, and my goal was to get it to do no
%   backtracking at all.  This goal has not yet been met, and it 
%   will still occasionally report an error message and then decide 
%   that it is happy with the input after all.  Sorry about that.


syntax_error(Message, List) :-
	ttynl, display('** SYNTAX ERROR: '),
	display_list(Message),
	length(List, Length),
	let(syntax_error, length, Length), !,
	fail.

syntax_error(List) :-
	val(syntax_error, length, AfterError), % replacement for recorda & recorded
	length(List, Length),
	BeforeError is Length-AfterError,
	display_list(List, BeforeError), !,
	fail.

display_list([Head|Tail]) :-
	ttyput(32),
	display_token(Head), !,
	display_list(Tail).
display_list([]) :-
	ttynl.

display_list(X, 0) :-
	display(' <HERE=> '), !,
	display_list(X, 99999).
display_list([Head|Tail], BeforeError) :-
	display_token(Head),
	ttyput(32),
	Left is BeforeError-1, !,
	display_list(Tail, Left).
display_list([], _) :-
	ttynl.

display_token(atom(X))	  :- !,	display(X).
display_token(var(_,X))	  :- !,	display(X).
display_token(integer(X)) :- !,	display(X).
display_token(string(X))  :- !,	display(X).
display_token(X)	  :-	display(X).


% --------- rdtok.pl ----------------
%   File   : RDTOK.PL
%   Author : R.A.O'Keefe
%   Updated: 2 July 1984
%   Purpose: Tokeniser in reasonably standard Prolog.

/*  This tokeniser is meant to complement the library READ routine.
    It recognises Dec-10 Prolog with the following exceptions:

	%( is not accepted as an alternative to {

	%) is not accepted as an alternative to )

	NOLC convention is not supported (read_name could be made to 
		do it)

	,.. is not accepted as an alternative to | (hooray!)

	large integers are not read in as xwd(Top18Bits,Bottom18Bits)

	After a comma, "(" is read as '((' rather than '('.  This does 
		the parser no harm at all, and the Dec-10 tokeniser's 
		behaviour here doesn't actually buy you anything.  
		This tokeniser guarantees never to return '(' except 
		immediately after an atom, yielding '((' every
		other where.

    In particular, radix notation is EXACTLY as in Dec-10 Prolog 
    version 3.53.  Some times might be of interest.  Applied to an 
    earlier version of this file:

	this code took		    1.66 seconds
	the Dec-10 tokeniser took   1.28 seconds [DEC-10 assembler -Tim]
	A Pascal version took	    0.96 seconds

    The Dec-10 tokeniser was called via the old RDTOK interface, with
    which this file is compatible.  One reason for the difference in
    speed is the way variables are looked up: this code uses a linear
    list, while the Dec-10 tokeniser uses some sort of tree.  The 
    Pascal version is the program WLIST which lists "words" and their 
    frequencies.  It uses a hash table.  Another difference is the way 
    characters are classified: the Dec-10 tokeniser and WLIST have a 
    table which maps ASCII codes to character classes, and don't do 
    all this comparison and memberchking.  We could do that without 
    leaving standard Prolog, but what do you want from one evening's 
    work?

:- public
	read_tokens/2.

:- mode
	read_after_atom(+, ?, -),
	read_digits(+, -, -),
	read_fullstop(+, ?, -),
	read_integer(+, -, -),
	read_lookup(?, +),
	read_name(+, -, -),
	read_solidus(+, ?, -),
	read_solidus(+, -),
	read_string(-, +, -),
	read_string(+, -, +, -),
	more_string(+, +, -, -),
	read_symbol(+, -, -),
	read_tokens(?, ?),
	read_tokens(+, ?, -).
*/


%   read_tokens(TokenList, Dictionary)
%   returns a list of tokens.  It is needed to "prime" read_tokens/2
%   with the initial blank, and to check for end of file.  The
%   Dictionary is a list of AtomName=Variable pairs in no particular 
%   order.  The way end of file is handled is that everything else 
%   FAILS when it hits character "-1", sometimes printing a warning.  
%   It might have been an idea to return the atom 'end_of_file' 
%   instead of the same token list that you'd have got from reading 
%   "end_of_file. ", but (1) this file is for compatibility, and (b) 
%   there are good practical reasons for wanting this behaviour.

read_tokens(TokenList, Dictionary) :-
	read_tokens(32, Dict, ListOfTokens),
	append(Dict, [], Dict), !,   %  fill in the "hole" at the end
	Dictionary = Dict,	     %  unify explicitly so we'll read and
	TokenList = ListOfTokens.    %  then check even with filled in arguments
read_tokens([atom(end_of_file)], []).	%  Eof is all that can go wrong


read_tokens(-1, _, _) :- !,	     %  -1 is the end-of-file character
	fail.			     %  in every standard Prolog
read_tokens(Ch, Dict, Tokens) :-
	Ch =< 32,	     	     %  ignore layout.  CR, LF, and the
	!,			     %  Dec-10 newline character (31)
	get0(NextCh),		     %  are all skipped here.
	read_tokens(NextCh, Dict, Tokens).
read_tokens(37, Dict, Tokens) :- !,	%  %comment
	repeat,				%  skip characters to a line
	    get0(Ch),
	    is_terminator(Ch),
	!,	%  stop when we find one
	Ch =\= -1,			%  fail on EOF
	get0(NextCh),
	read_tokens(NextCh, Dict, Tokens).
read_tokens(47, Dict, Tokens) :- !,	%  /*comment?
	get0(NextCh),
	read_solidus(NextCh, Dict, Tokens).
read_tokens(33, Dict, [atom(!)|Tokens]) :- !,	%  This is a special case so
	get0(NextCh),			%  that !. reads as two tokens.
	read_after_atom(NextCh, Dict, Tokens).	%  It could be cleverer.
read_tokens(40, Dict, ['(('|Tokens]) :- !,	%  NB!!!
	get0(NextCh),
	read_tokens(NextCh, Dict, Tokens).
read_tokens(41, Dict, [')'|Tokens]) :- !,
	get0(NextCh),
	read_tokens(NextCh, Dict, Tokens).
read_tokens(44, Dict, [','|Tokens]) :- !,
	get0(NextCh),
	read_tokens(NextCh, Dict, Tokens).
read_tokens(59, Dict, [atom((;))|Tokens]) :- !,	%   ; is nearly a punctuation
	get0(NextCh),			%   mark but not quite (e.g.
	read_tokens(NextCh, Dict, Tokens).	%   you can :-op declare it).
read_tokens(91, Dict, ['['|Tokens]) :- !,
	get0(NextCh),
	read_tokens(NextCh, Dict, Tokens).
read_tokens(93, Dict, [']'|Tokens]) :- !,
	get0(NextCh),
	read_tokens(NextCh, Dict, Tokens).
read_tokens(123, Dict, ['{'|Tokens]) :- !,
	get0(NextCh),
	read_tokens(NextCh, Dict, Tokens).
read_tokens(124, Dict, ['|'|Tokens]) :- !,
	get0(NextCh),
	read_tokens(NextCh, Dict, Tokens).
read_tokens(125, Dict, ['}'|Tokens]) :- !,
	get0(NextCh),
	read_tokens(NextCh, Dict, Tokens).
read_tokens(46, Dict, Tokens) :- !,		%  full stop
	get0(NextCh),				%  or possibly .=. &c
	read_fullstop(NextCh, Dict, Tokens).
read_tokens(34, Dict, [string(S)|Tokens]) :- !,	%  "string"
	read_string(S, 34, NextCh),
	read_tokens(NextCh, Dict, Tokens).
read_tokens(39, Dict, [atom(A)|Tokens]) :- !,	%  'atom'
	read_string(S, 39, NextCh),
	name(A, S),				%  BUG: '0' = 0 unlike Dec-10 Prolog
	read_after_atom(NextCh, Dict, Tokens).
read_tokens(Ch, Dict, [var(Var,Name)|Tokens]) :- is_maj(Ch),!,
	%  have to watch out for "_"
	read_name(Ch, S, NextCh),
	(  S = "_", Name = '_'			%  anonymous variable
	;  name(Name, S),			%  construct name
	   read_lookup(Dict, Name=Var)		%  lookup/enter in dictionary
	), !,
	read_tokens(NextCh, Dict, Tokens).
read_tokens(Ch, Dict, [integer(I)|Tokens]) :- is_num(Ch),!,
	read_integer(Ch, I, NextCh),
	read_tokens(NextCh, Dict, Tokens).
read_tokens(Ch, Dict, [atom(A)|Tokens]) :- is_min(Ch),!,
	read_name(Ch, S, NextCh),
	name(A, S),
	read_after_atom(NextCh, Dict, Tokens).
read_tokens(Ch, Dict, [atom(A)|Tokens]) :-	% THIS MUST BE THE LAST CLAUSE
	get0(AnotherCh),
	read_symbol(AnotherCh, Chars, NextCh),	% might read 0 chars
	name(A, [Ch|Chars]),			% so might be [Ch]
	read_after_atom(NextCh, Dict, Tokens).


%   The only difference between read_after_atom(Ch, Dict, Tokens) and
%   read_tokens/3 is what they do when Ch is "(".  read_after_atom
%   finds the token to be '(', while read_tokens finds the token to be
%   '(('.  This is how the parser can tell whether <atom> <paren> must
%   be an operator application or an ordinary function symbol 
%   application.  See the library file READ.PL for details.

read_after_atom(40, Dict, ['('|Tokens]) :- !,
	get0(NextCh),
	read_tokens(NextCh, Dict, Tokens).
read_after_atom(Ch, Dict, Tokens) :-
	read_tokens(Ch, Dict, Tokens).


%   read_string(Chars, Quote, NextCh)
%   reads the body of a string delimited by Quote characters.
%   The result is a list of ASCII codes.  There are two complications.
%   If we hit the end of the file inside the string this predicate 
%   FAILS.  It does not return any special structure.  That is the 
%   only reason it can ever fail.  The other complication is that when
%   we find a Quote we have to look ahead one character in case it is 
%   doubled.  Note that if we find an end-of-file after the quote we 
%   *don't* fail, we return a normal string and the end of file 
%   character is returned as NextCh.  If we were going to accept 
%   C-like escape characters, as I think we should, this would need 
%   changing (as would the code for 0'x).  But the purpose of this 
%   module is not to present my ideal syntax but to present something 
%   which will read present-day Prolog programs.

read_string(Chars, Quote, NextCh) :-
	get0(Ch),
	read_string(Ch, Chars, Quote, NextCh).

read_string(-1, _, Quote, -1) :-
	display('! end of file in: '), ttyput(Quote),
	display(token), ttyput(Quote), ttynl,
	!, fail.
read_string(Quote, Chars, Quote, NextCh) :- !,
	get0(Ch),			% closing or doubled quote
	more_string(Ch, Quote, Chars, NextCh).
read_string(Char, [Char|Chars], Quote, NextCh) :-
	read_string(Chars, Quote, NextCh).	% ordinary character


more_string(Quote, Quote, [Quote|Chars], NextCh) :- !,
	read_string(Chars, Quote, NextCh).	% doubled quote
more_string(NextCh, _, [], NextCh).		% end



% read_solidus(Ch, Dict, Tokens)
%   checks to see whether /Ch is a /* comment or a symbol.  If the
%   former, it skips the comment.  If the latter it just calls 
%   read_symbol.  We have to take great care with /* comments to 
%   handle end of file inside a comment, which is why read_solidus/2 
%   passes back an end of file character or a (forged) blank that we 
%   can give to read_tokens.


read_solidus(42, Dict, Tokens) :- !,
	get0(Ch),
	read_solidus(Ch, NextCh),
	read_tokens(NextCh, Dict, Tokens).
read_solidus(Ch, Dict, [atom(A)|Tokens]) :-
	read_symbol(Ch, Chars, NextCh),		% might read 0 chars
	name(A, [47|Chars]),
	read_tokens(NextCh, Dict, Tokens).

read_solidus(-1, -1) :- !,
	display('! end_of_file in /*.. comment'), ttynl.
read_solidus(42, LastCh) :-
	get0(NextCh),
	NextCh =\= 47, !,	%  might be ^Z or * though
	read_solidus(NextCh, LastCh).
read_solidus(42, 32) :- !.	%  the / was skipped in the previous clause
read_solidus(_, LastCh) :-
	get0(NextCh),
	read_solidus(NextCh, LastCh).


%   read_name(Char, String, LastCh)
%   reads a sequence of letters, digits, and underscores, and returns
%   them as String.  The first character which cannot join this sequence
%   is returned as LastCh.

read_name(Char, [Char|Chars], LastCh) :-
	is_alpha_num(Char),!,
	get0(NextCh),
	read_name(NextCh, Chars, LastCh).
read_name(LastCh, [], LastCh).

%   read_symbol(Ch, String, NextCh)
%   reads the other kind of atom which needs no quoting: one which is
%   a string of "symbol" characters.  Note that it may accept 0
%   characters, this happens when called from read_fullstop.

read_symbol(Char, [Char|Chars], LastCh) :-
	is_spec(Char),
	get0(NextCh),
	read_symbol(NextCh, Chars, LastCh).
read_symbol(LastCh, [], LastCh).


%   read_fullstop(Char, Dict, Tokens)
%   looks at the next character after a full stop.  There are
%   three cases:
%	(a) the next character is an end of file.  We treat this
%	    as an unexpected end of file.  The reason for this is
%	    that we HAVE to handle end of file characters in this
%	    module or they are gone forever; if we failed to check
%	    for end of file here and just accepted .<EOF> like .<NL>
%	    the caller would have no way of detecting an end of file
%	    and the next call would abort.
%	(b) the next character is a layout character.  This is a
%	    clause terminator.
%	(c) the next character is anything else.  This is just an
%	    ordinary symbol and we call read_symbol to process it.

read_fullstop(-1, _, _) :- !,
	display('! end_of_file just after full_stop'), ttynl,
	fail.
read_fullstop(Ch, _, []) :-
	Ch =< 32, !.		% END OF CLAUSE
read_fullstop(Ch, Dict, [atom(A)|Tokens]) :-
	read_symbol(Ch, S, NextCh),
	name(A, [46|S]),
	read_tokens(NextCh, Dict, Tokens).



%   read_integer is complicated by having to understand radix notation.
%   There are three forms of integer:
%	0 ' <any character>	- the ASCII code for that character
%	<digit> ' <digits>	- the digits, read in that base
%	<digits>		- the digits, read in base 10.
%   Note that radix 16 is not understood, because 16 is two digits,
%   and that all the decimal digits are accepted in each base (this
%   is also true of C).  So 2'89 = 25.  I can't say I care for this,
%   but it does no great harm, and that's what Dec-10 Prolog does.
%   The X =\= -1 tests are to make sure we don't miss an end of file
%   character.  The tokeniser really should be in C, not least to
%   make handling end of file characters bearable.  If we hit an end
%   of file inside an integer, read_integer will fail.

read_integer(BaseChar, IntVal, NextCh) :-
	Base is BaseChar - 48,
	get0(Ch),
	Ch =\= -1,
	(   Ch =\= 39, read_digits(Ch, Base, 10, IntVal, NextCh)
	;   Base >= 1, read_digits(0, Base, IntVal, NextCh)
	;   get0(IntVal), IntVal =\= -1, get0(NextCh)
	),  !.

read_digits(SoFar, Base, Value, NextCh) :-
	get0(Ch),
	Ch =\= -1,
	read_digits(Ch, SoFar, Base, Value, NextCh).

read_digits(Digit, SoFar, Base, Value, NextCh) :-
	Digit >= 48, Digit =< 57,
	!,
	Temp is SoFar*Base, Temp1 is Temp-48, Next is Temp1+Digit,
	read_digits(Next, Base, Value, NextCh).
read_digits(LastCh, Value, _, Value, LastCh).

%   read_lookup is identical to memberchk except for argument order and
%   mode declaration.

read_lookup([X|_], X) :- !.
read_lookup([_|T], X) :-
	read_lookup(T, X). 

ttynl:-telling(F),tell(user),nl,tell(F).
ttyput(X):-telling(F),tell(user),put(X),tell(F).

display(X):-telling(F),tell(user),portable_display(X),tell(F).

ttyprint(X):-telling(F),tell(user),cwrite(X),nl,tell(F).

memberchk(C,[C|_]):-!. 
memberchk(C,[_|L]):- memberchk(C,L).

portable_read(X):-read_term(X,_).

% added for speed: Paul Tarau, Jan 92 : to be done in C

is_alpha_num(Char):- Char >= 97, Char =< 122. % a..z
is_alpha_num(Char):- Char >= 65, Char =< 90.  % A..Z
is_alpha_num(Char):- Char >= 48, Char =< 57.   % 0..9
is_alpha_num(95).
is_alpha_num(Char):- is_latin1_min(Char).
is_alpha_num(Char):- is_latin1_maj(Char).

is_maj(Ch):- Ch >= 65, Ch =< 90.
is_maj(95).
is_maj(Ch):-is_latin1_maj(Ch).

is_min(Char):- Char >= 97, Char =< 122.
is_min(Char) :- is_latin1_min(Char).

is_num(Char):- Char >= 48, Char =< 57.

% support for latin1 - thanks to Ulrich Neumerkel

is_latin1_maj(Ch) :- Ch >= 192, Ch =< 214. 
is_latin1_maj(Ch) :- Ch >= 216, Ch =< 222. 

is_latin1_min(Ch) :- Ch >= 223, Ch =< 246.
is_latin1_min(Ch) :- Ch >= 248, Ch =< 255.

is_terminator(10).
is_terminator(13).
is_terminator(-1).

%	Char is in "#$&*+-./:<=>?@\^`~"

is_spec(35).
is_spec(36).
is_spec(38).
is_spec(42).
is_spec(43).
is_spec(45).
is_spec(46).
is_spec(47).
is_spec(58).
is_spec(60).
is_spec(61).
is_spec(62).
is_spec(63).
is_spec(64).
is_spec(92).
is_spec(94).
is_spec(96).
is_spec(126).

% neads also append/3

/*
test_read(ok):-repeat,read_term(R,S),write(R-S),nl,R=end_of_file,!.
*/
