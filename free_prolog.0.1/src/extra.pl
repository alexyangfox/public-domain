% Continuation manipulations

pop_cc:-
	pop('$cont',Cont),
	bcall(Cont),
	fail.
pop_cc.

call_cc:-
	stack('$cont',[Cont|_]),
	bcall(Cont),
	fail.
call_cc.

show_cc:-
	stack('$cont',Cs),
	member(Cont,Cs),
	nl,
	cc_list(Cont,L),
	member(G,L),
	write(G),nl,
	fail.
show_cc:-nl.

strip_cont(TC,T,C):-
	functor(TC,F,N1),N is N1-1,
	functor(T,F,N),
	arg(N1,TC,C),
	copy_args(1,N,T,TC).

cc_list(Cont,Cont):-var(Cont),!.
cc_list(true,[true]):-!.
cc_list(fail(_),[fail]):-!.
cc_list(T,[G|Gs]):-
	strip_cont(T,G,Cont),
	cc_list(Cont,Gs).

% Blackboard related utilities

% bboard visualisation

bb_element(A+B=C,[A,B,C|_]).
bb_element(D,[_,_,_|L]):-bb_element(D,L).

bb:-
	statistics(bboard,X),write(bboard-X),nl,
	bb_list(L),
		bb_element(B,L),
		write(B),nl,
	fail
; nl.

% BinProlog extensions: NAMING primitives

% naming of pointers to heap objects and copies of constants
% this is very fast, and can be used to save constant objects that
% survive backtracking

% naming of pointers to copies of general heap objects
% can be used as permanent <global variables> that survive backtracking

object(New):-var(New),!,val('$object','$object',New).
object(New):-atomic(New),let('$object','$object',New).

message(New):-var(New),!,object(O),val(O,'$message',New).
message(New):-atomic(New),object(O),let(O,'$message',New).

/* vectors */

% new_object(Name,Type):-def(Name,'$type',Type),!.
% new_object(Name,Type):-errmes('name clash',Name).

vector_def(Name,Dim,Zero):-
%	new_object(Name,vector),
	Max is Dim-1,
	saved(Zero,BBVal),
	for(I,0,Max),
	let(Name,I,BBVal),
	fail.
vector_def(_,_,_).

vector_set(Name,I,Val):-saved(Val,BBVal),set(Name,I,BBVal).

vector_val(Name,I,Val):-val(Name,I,Val).

/* bounded buffers */

bbuf_def(Name,Limit):-Limit<1,!,errmes('bad limit for bbuf',Limit).
bbuf_def(Name,Limit):-Max is Limit+1,
%	new_object(Name,bounded_buffer),
	let(Name,limit,Max),
	let(Name,begin,0),
	let(Name,end,0).

bbuf_add(Name,Element):-
	val(Name,limit,Max),
	val(Name,begin,B),
	val(Name,end,E),
	E1 is (E+1) mod Max,
	B=\=E1,
	saved(Element,NewElement),
	let(Name,E1,NewElement),
	set(Name,end,E1).

bbuf_remove(Name,Element):-
	val(Name,limit,Max),
	val(Name,begin,B),
	val(Name,end,E),
	B=\=E,
	B1 is (B+1) mod Max,
	val(Name,B1,Element),
	set(Name,begin,B1).

/* lemmas */

% H is true fact memoized by xlemma (not necessarily ground)
xlemma(H):-
	nonvar(H),
	functor(H,P,_),
	arg(1,H,A),
	nonvar(A),
	functor(A,F,_),
	xlemma_put(P,F,H),
	!.
xlemma(H):-errmes('unable to make or use lemma',H).

% optimized lemma: <P,I> --> X (P,I,O must be atomic)
lemma2(P,I,O):-val(P,I,X),!,X=O.
lemma2(P,I,O):-functor(G,P,2),arg(1,G,I),G,!,
	arg(2,G,O),
	def(P,I,O).

%optimized lemma <P,I,G> --> O (instantiated executing G)
lemma3(P,I,_,O):-val(P,I,X),!,X=O.
lemma3(P,I,G,O):-G,!,def(P,I,O).

xlemma_put(P,F,H):-
	val(P,F,G),
	!,
	copy_term(G,H).
xlemma_put(P,F,H):-
	call(H),
	!,
	saved(H,Lemma),
	def(P,F,Lemma).	

/* global counters */

ctr_dec(F,X,V1):-val(F,X,V),V1 is V-1,set(F,X,V1).
ctr_inc(F,X,V):-val(F,X,V),V1 is V+1,set(F,X,V1).

ctr_def(C,Init):-def('$ctr',C,Init).
ctr_inc(C,Val):-ctr_inc('$ctr',C,Val).
ctr_dec(C,Val):-ctr_dec('$ctr',C,Val).

/* dynamic/1, assert/1 & retract/1 predicates

This is an approximation of other Prologs assert & retract predicates.
For efficiency and programming style reasons we strongly suggest
not to use them too much.

If you simply want lemmas use xlemma or the highly optmized
versions lemma2 & lemma3.

If you want maximal efficiency use def/3 set/3 val/3 combined
with save_term and copy_term. They give you acces to a very fast
hashing table <key,key>--> value, the same that BinProlog
uses internally for indexing by predicate and first argument.

Beware that assert & retract are not optimized for large databases
or frequent use of an asserted predicate.

To use dynamic predicates you have to declare them with dynamic/1
otherwise asserts will simply fail.

To activate an asserted predicate you must use

	?-metacall(Goal).

instead of 

	?- Goal.

To call a dynamic predicate from compiled code, use also
metacall/1. 

*******************************************************/

dynamic(Ps):-make_dynamic(Ps),fail.
dynamic(_).
 
make_dynamic((P1,P2)):-!,make_dynamic(P1,_),make_dynamic(P2,_),fail.
make_dynamic(Pred):-make_dynamic(Pred,_).

make_dynamic(P/N,_):-
	functor(T,P,N),
	( is_compiled(T)->errmes('a compiled predicate cannot be dynamic',P/N)
	  ; def(P,'$first',0),def(P,'$last',0),def(P,'$arity',N),!
  ).
make_dynamic(Pred,Doit):-Doit==force,!.
make_dynamic(Pred,_):-
	errmes('unable to define dynamic predicate',Pred).

% moved to lib.pl:
% is_dynamic(P):-functor(P,F,N),val(F,'$arity',N).

asserta(C):-
	fact2rule(C,(H:-B)),
	functor(H,P,_),
	ctr_dec(P,'$first',No),
	save_term((H:-B),New),
	let(P,No,New).
	
assert(C):-
	fact2rule(C,(H:-B)),
	functor(H,P,_),
	ctr_inc(P,'$last',No),
	save_term((H:-B),New),
	let(P,No,New).

assertz(C):-assert(C).

retract(C):-
	fact2rule(C,(H:-B)),
	functor(H,P,_),
	clause(H,B,I),
	rm(P,I).

clause(H,B):-clause(H,B,_).

clause(G,NewB,I):-
	functor(G,P,N),
	val(P,'$first',Min),
	val(P,'$last',Max),
	for(I,Min,Max),
		val(P,I,(H:-B)),
%		\+ \+ G=H,
		copy_term((H:-B),(G:-NewB)).

consult(File):-
	find_file(File,F),
	statistics(runtime,_),
	write(consulting(F)),nl,
	see(F),
	repeat,
		read(C),expand_term(C,E),
		assert_it(E),
	!,
	seen,
	statistics(runtime,[_,T]),
	write(consulted(F,time=T)),nl.

abolish(P,N):-
	val(P,'$first',Min),val(P,'$last',Last),val(P,'$arity',N),
	Max is Last-1,
	for(I,Min,Max),
		rm(P,I),
	fail.
abolish(P,N):-
	set(P,'$first',0),
	set(P,'$last',0).

assert_it(end_of_file):-!.
assert_it(C):-assert(C),!,fail.
assert_it(C):-
	fact2rule(C,(H:-_)),
	functor(H,P,N),
	make_dynamic(P/N,force),
	assert(C),
	fail.


/*******************************************************************/
% determinism related stuff

det_call(G):-findall(G,G,Gs),is_found_det(Gs,G).

is_found_det([G],G0):-!,G0=G.
is_found_det(Gs,G):-errmes(must_be_deterministic(G),Gs).

% bestof/3: a specification

/*
bestof(Winner,Game,Generator):-
	findall(Winner,Generator,[X|Xs]),
	let_them_play(Xs,Game,X,Winner).
	
let_them_play([],_,Winner,Winner).
let_them_play([Challenger|Xs],Game,Incumbent,Winner) :-
	is_better(NewIncumbent,Game,Incumbent,Challenger),
	let_them_play(Xs,Game,NewIncumbent,Winner).

is_better(Winner,Game,Incumbent,Challenger):-
	\+ \+ 	%  ensures that the winner does not steal bindings
	apply(Game,[Challenger,Incumbent]),
	!,
	Winner=Challenger.
is_better(Incumbent,_Game,Incumbent,_Challenger).
*/

% a practical implementation

bestof(X,Rel,Generator):-
	inc_level(bestof,Level),
	Generator,
	update_bestof(Level,X,Rel),
	fail.
bestof(X,Rel,Generator):-
	dec_level(bestof,Level),
	in0(bestof,Level,X).

update_bestof(Level,New,Rel):-
	rd0(bestof,Level,Old),
	!,
	apply(Rel,[New,Old]),
	saved(New,S),
	set(bestof,Level,S).
update_bestof(Level,New,_):-
	saved(New,S),
	out0(bestof,Level,S).

inc_level(Obj,X1):-val(Obj,Obj,X),!,X1 is X+1,set(Obj,Obj,X1).
inc_level(Obj,1):-def(Obj,Obj,1).

dec_level(Obj,X):-val(Obj,Obj,X),X>0,X1 is X-1,set(Obj,Obj,X1).

% to be optimized:
% apply or (better) call/N must be replaced with a WAM-level operation
% however, if =.. is moved to the WAM performances look fast enough

apply(Closure,Args):-
	Closure=..L1,
	det_append(L1,Args,L2),
	Goal=..L2,!,
	Goal.

call(Closure,X1):-apply(Closure,[X1]).
call(Closure,X1,X2):-apply(Closure,[X1,X2]).
call(Closure,X1,X2,X3):-apply(Closure,[X1,X2,X3]).
call(Closure,X1,X2,X3,X4):-apply(Closure,[X1,X2,X3,X4]).

% alternative findall/3 without WAM-support : still quite fast
% uses the blackboard instead of the more efficient but somewhat risky
% heap-lifting technique

find_all(X,Generator,_):-
	inc_level(find_all,Level),
	Generator,
	push(find_all,Level,X),
	fail.
find_all(X,_,Result):-
	dec_level(find_all,Level),
	stack(find_all,Level,Sols),
	reverse(Sols,Result),
	let(find_all,Level,[]).

reverse(Xs,Rs):-rev(Xs,[],Rs).

rev([],Acc,Acc).
rev([X|Xs],Acc,Zs):-rev(Xs,[X|Acc],Zs).

ints([],I,I):-!.
ints([I0|L],I0,I):-I0<I,I1 is I0+1,ints(L,I1,I).

% sort, adapted from public domain code written by R.A. O'Keefe
% use merge_sort(<,_,_) if you do not want duplications eliminated
% use merge_sort(>,_,_) for descending order

sort(L1,L2):-merge_sort(<,L1,DupL),remdup(DupL,L2).

remdup([],[]).
remdup([X,Y|Xs],Ys):-compare(=,X,Y),!,remdup([X|Xs],Ys).
remdup([X|Xs],[X|Ys]):-remdup(Xs,Ys).
      
merge_sort(Rel, L,S ):-
	length(L,N),
	merge_sort1(N, Rel, L,S,[] ).

merge_sort1( 0,_,L,[],L ):-!.
merge_sort1( 1,_,[X|L],[X],L ):-!.
merge_sort1( N,Rel,L,S,R ):-				% N >= 2
	N1 is N >> 1,	N2 is N-N1,
	merge_sort1( N1,Rel,L,S1,R1),	
	merge_sort1( N2,Rel,R1,S2,R),
	merge_2( S2,Rel,S1,S ).

merge_2([],_,S,S ):-!.
merge_2([X|L1],Rel,[Y|L2],[X|L] ):-compare(Rel,X,Y),!,
	merge_2(L1,Rel,[Y|L2],L ).
merge_2(L1,Rel,[Y|L2],[Y|L] ):-
	merge_2(L2,Rel,L1,L ).

%   Keysorting.  Adapted by Mats Carlsson from R.O'Keefe's code, 
%		but uses recursion instead of an auxiliary stack.  
%		Takes care to check validity of arguments.
%   Could be speed up if there were an inline keycompare/3.

ksort(List, Sorted) :-
	keysort(List, -1, S, []), !,
	Sorted = S.
ksort(X, Y):-user_error('illegal_arguments',keysort(X,Y)).

keysort([Head|Tail], Lim, Sorted, Rest) :- !,
	nonvar(Head),
	Head = _-_,
	Qh = [Head|_],
	samkeyrun(Tail, Qh, Qh, Run, Rest0),
	keysort(Rest0, 1, Lim, Run, Sorted, Rest).
keysort(Rest, _, [], Rest).

keysort([Head|Tail], J, Lim, Run0, Sorted, Rest) :-
	J =\= Lim, !,
	nonvar(Head),
	Head = _-_,
	Qh = [Head|_],
	samkeyrun(Tail, Qh, Qh, Run1, Rest0),
	keysort(Rest0, 1, J, Run1, Run2, Rest1),
	keymerge(Run0, Run2, Run),
	K is J+J,
	keysort(Rest1, K, Lim, Run, Sorted, Rest).
keysort(Rest, _, _, Sorted, Sorted, Rest).

samkeyrun([Hd|Tail], QH, QT, Run, Rest) :-
	nonvar(Hd),
	Hd = H-_,
	QT = [Q-_|QT2], 
	Q @=< H, !,
	QT2 = [Hd|_],
	samkeyrun(Tail, QH, QT2, Run, Rest).
samkeyrun([Hd|Tail], QH, QT, Run, Rest) :-
	nonvar(Hd),
	Hd = H-_,
	QH = [Q-_|_],
	H @< Q, !,
	samkeyrun(Tail, [Hd|QH], QT, Run, Rest).
samkeyrun(Rest, Run, [_], Run, Rest).

% keymerge(+List, +List, -List).
keymerge([], L2, Out) :- !,
	Out = L2.
keymerge([H1|T1], L2, Out) :-	
	L2 = [K2-_|_],
	H1 = K1-_,
	K1 @=< K2, !,
	Out = [H1|Out1],
	keymerge(T1, L2, Out1).
keymerge(L1, [H2|L2], Out) :- !,
	Out = [H2|Out1],
	keymerge(L1, L2, Out1).
keymerge(List, _, List).

% -------------------- ---------------------

%   File   : SETOF.PL
%   Author : R.A.O'Keefe
%   Updated: 17 November 1983
%   Purpose: define setof/3, bagof/3, findall/3, and findall/4
%   Needs  : Not.Pl

% Adapted by Paul Tarau for BinProlog: uses a heap based findall/3.
% therefore some database hacking predicates have been removed.
% Updated: 19 July 1992

/*  This file defines two predicates which act like setof/3 and bagof/3.
    I have seen the code for these routines in Dec-10 and in C-Prolog,
    but I no longer recall it, and this code was independently derived
    in 1982 by me and me alone.

    Most of the complication comes from trying to cope with free variables
    in the Filter; these definitions actually enumerate all the solutions,
    then group together those with the same bindings for the free variables.
    There must be a better way of doing this.  I do not claim any virtue for
    this code other than the virtue of working.  In fact there is a subtle
    bug: if setof/bagof occurs as a data structure in the Generator it will
    be mistaken for a call, and free variables treated wrongly.  Given the
    current nature of Prolog, there is no way of telling a call from a data
    structure, and since nested calls are FAR more likely than use as a
    data structure, we just put up with the latter being wrong.  The same
    applies to negation.

    Would anyone incorporating this in their Prolog system please credit
    both me and David Warren;  he thought up the definitions, and my
    implementation may owe more to subconscious memory of his than I like
    to think.  At least this ought to put a stop to fraudulent claims to
    having bagof, by replacing them with genuine claims.

    Thanks to Dave Bowen for pointing out an amazingly obscure bug: if
    the Template was a variable and the Generator never bound it at all
    you got a very strange answer!  Now fixed, at a price.

	bagof/3,		%   Like bagof (Dec-10 manual p52)
	setof/3.		%   Like setof (Dec-10 manual p51)

:- mode
	bagof(+,+,?),
	concordant_subset(+,+,-),
	concordant_subset(+,+,-,-),
	concordant_subset(+,+,+,+,-),
	replace_key_variables(+,+,+),
	setof(+,+,?).

%   setof(Template, Generator, Set)
%   finds the Set of instances of the Template satisfying the Generator..
%   The set is in ascending order (see compare/3 for a definition of
%   this order) without duplicates, and is non-empty.  If there are
%   no solutions, setof fails.  setof may succeed more than one way,
%   binding free variables in the Generator to different values.  This
%   predicate is defined on p51 of the Dec-10 Prolog manual.
*/

setof(Template, Filter, Set) :-
	bagof(Template, Filter, Bag),
	sort(Bag, Set).


%   bagof(Template, Generator, Bag)
%   finds all the instances of the Template produced by the Generator,
%   and returns them in the Bag in they order in which they were found.
%   If the Generator contains free variables which are not bound in the
%   Template, it assumes that this is like any other Prolog question
%   and that you want bindings for those variables.  (You can tell it
%   not to bother by using existential quantifiers.)
%   bagof records three things under the key '.':
%	the end-of-bag marker	       -
%	terms with no free variables   -Term
%	terms with free variables   Key-Term
%   The key '.' was chosen on the grounds that most people are unlikely
%   to realise that you can use it at all, another good key might be ''.
%   The original data base is restored after this call, so that setof
%   and bagof can be nested.  If the Generator smashes the data base
%   you are asking for trouble and will probably get it.
%   The second clause is basically just findall, which of course works in
%   the common case when there are no free variables.

bagof(Template, Generator, Bag) :-
	free_variables(Generator, Template, [], Vars),
	Vars \== [],
	!,
	Key =.. [.|Vars],
	functor(Key, ., N),
	findall(Key-Template,Generator,Recorded),
	replace_instance(Recorded, Key, N, [], OmniumGatherum),
	keysort(OmniumGatherum, Gamut), !,
	concordant_subset(Gamut, Key, Answer),
	Bag = Answer.
bagof(Template, Generator, [B|Bag]) :-
	findall(Template,Generator,[B|Bag]).

_^Goal:-Goal.

replace_instance([], _, _, AnsBag, AnsBag) :- !.
replace_instance([NewKey-Term|Xs], Key, NVars, OldBag, NewBag) :-
		replace_key_variables(NVars, Key, NewKey), !,
		replace_instance(Xs,Key, NVars, [NewKey-Term|OldBag], NewBag).

%   There is a bug in the compiled version of arg in Dec-10 Prolog,
%   hence the rather strange code.  Only two calls on arg are needed
%   in Dec-10 interpreted Prolog or C-Prolog.

replace_key_variables(0, _, _) :- !.
replace_key_variables(N, OldKey, NewKey) :-
	arg(N, NewKey, Arg),
	nonvar(Arg), !,
	M is N-1,
	replace_key_variables(M, OldKey, NewKey).
replace_key_variables(N, OldKey, NewKey) :-
	arg(N, OldKey, OldVar),
	arg(N, NewKey, OldVar),
	M is N-1,
	replace_key_variables(M, OldKey, NewKey).

%   concordant_subset([Key-Val list], Key, [Val list]).
%   takes a list of Key-Val pairs which has been keysorted to bring
%   all the identical keys together, and enumerates each different
%   Key and the corresponding lists of values.

concordant_subset([Key-Val|Rest], Clavis, Answer) :-
	concordant_subset(Rest, Key, List, More),
	concordant_subset(More, Key, [Val|List], Clavis, Answer).


%   concordant_subset(Rest, Key, List, More)
%   strips off all the Key-Val pairs from the from of Rest,
%   putting the Val elements into List, and returning the
%   left-over pairs, if any, as More.

concordant_subset([Key-Val|Rest], Clavis, [Val|List], More) :-
	Key == Clavis,
	!,
	concordant_subset(Rest, Clavis, List, More).
concordant_subset(More, _, [], More).


%   concordant_subset/5 tries the current subset, and if that
%   doesn't work if backs up and tries the next subset.  The
%   first clause is there to save a choice point when this is
%   the last possible subset.

concordant_subset([],   Key, Subset, Key, Subset) :- !.
concordant_subset(_,    Key, Subset, Key, Subset).
concordant_subset(More, _,   _,   Clavis, Answer) :-
	concordant_subset(More, Clavis, Answer).


% ---extracted from: not.pl --------------------%   File   : NOT.PL

%   Author : R.A.O'Keefe
%   Updated: 17 November 1983
%   Purpose: "suspicious" negation 

%   In order to handle variables properly, we have to find all the 
%   universally quantified variables in the Generator.  All variables
%   as yet unbound are universally quantified, unless
%	a)  they occur in the template
%	b)  they are bound by X^P, setof, or bagof
%   free_variables(Generator, Template, OldList, NewList)
%   finds this set, using OldList as an accumulator.

free_variables(Term, Bound, VarList, [Term|VarList]) :-
	var(Term),
	term_is_free_of(Bound, Term),
	list_is_free_of(VarList, Term),
	!.
free_variables(Term, _, VarList, VarList) :-
	var(Term),
	!.
free_variables(Term, Bound, OldList, NewList) :-
	explicit_binding(Term, Bound, NewTerm, NewBound),
	!,
	free_variables(NewTerm, NewBound, OldList, NewList).
free_variables(Term, Bound, OldList, NewList) :-
	functor(Term, _, N),
	free_variables(N, Term, Bound, OldList, NewList).

free_variables(0,    _,     _, VarList, VarList) :- !.
free_variables(N, Term, Bound, OldList, NewList) :-
	arg(N, Term, Argument),
	free_variables(Argument, Bound, OldList, MidList),
	M is N-1, !,
	free_variables(M, Term, Bound, MidList, NewList).

%   explicit_binding checks for goals known to existentially quantify
%   one or more variables.  In particular \+ is quite common.

explicit_binding(\+ _,	       Bound, fail,	Bound      ).
explicit_binding(not(_),	     Bound, fail,	Bound	   ).
explicit_binding(Var^Goal,	   Bound, Goal,	Bound+Var).
explicit_binding(setof(Var,Goal,Set),  Bound, Goal-Set, Bound+Var).
explicit_binding(bagof(Var,Goal,Bag),  Bound, Goal-Bag, Bound+Var).

term_is_free_of(Term, Var) :-
	var(Term), !,
	Term \== Var.
term_is_free_of(Term, Var) :-
	functor(Term, _, N),
	term_is_free_of(N, Term, Var).

term_is_free_of(0, _, _) :- !.
term_is_free_of(N, Term, Var) :-
	arg(N, Term, Argument),
	term_is_free_of(Argument, Var),
	M is N-1, !,
	term_is_free_of(M, Term, Var).

list_is_free_of([], _).
list_is_free_of([Head|Tail], Var) :-
	Head \== Var,
	list_is_free_of(Tail, Var).


% ------------ interface ------------------------

std_keysort(L,S):-ksort(L,S).
keysort(L,S):-ksort(L,S).

% -------------------------------------------------------------------------
%   File   : INTERP
%   Author : R.A.O'Keefe
%   Updated: 2 March 84
%   Purpose: Meta-circular interpreter for Prolog

/*  This is a genuinely meta-circular interpreter for a subset of Prolog
    containing cuts.  It relies on the fact that disjunction is transparent
    to cut just like conjunction.  If it doesn't work in your Prolog, and
    if you paid more than $100 for it, take your Prolog back to the shop
    and insist that they fix it, there are at least four different ways of
    implementing disjunction so that it works.
*/

% the metainterpreter moved to top.pl => Paul Tarau

trace(Goal) :-
	tr_goal(Goal, 0).


tr_goal(call(Goal), Depth) :- !,
	nonvar(Goal),
	tr_body(Goal, Depth).
tr_goal(\+(Goal), Depth) :-
	tr_body(Goal, Depth),
	!, fail.
tr_goal(\+(_), _) :- !.
tr_goal(Goal, Depth) :-
	(   tab(Depth), write('Call: '), print(Goal), nl, fail
	;   Depth1 is 1+Depth,
	    tr_call(Goal, Depth1),
	    (   tab(Depth), write('Exit: '), print(Goal), nl, fail
	    ;	true
	    ;   tab(Depth), write('Redo: '), print(Goal), nl, fail
	    )
	;   tab(Depth), write('Fail: '), print(Goal), nl, fail
	).

tr_call(bagof(X,Y,Z), Depth) :- !,		% include these 4 lines if you
	bagof(X, tr_body(Y,Depth), Z).		% really want them, but they do
tr_call(setof(X,Y,Z), Depth) :- !,		% slow things down a bit.
	setof(X, tr_body(Y,Depth), Z).
tr_call(findall(X,Y,Z), Depth) :- !,
	findall(X, tr_body(Y,Depth), Z).
tr_call(Goal, _) :-
	is_compiled(Goal),		% <--- check for a compiled predicate
	!,
	call(Goal).
tr_call(Goal, Depth) :-
	call(debug_clause(Goal, Body)),
	tr_body(Body, Depth, AfterCut, HadCut),
	(   HadCut = yes,
		!,
		tab(Depth), write('CUT'), nl,
		tr_body(AfterCut, Depth)
	;   HadCut = no
	).


tr_body(Body, Depth) :-
	tr_body(Body, Depth, AfterCut, HadCut),
	(   HadCut = yes,
		!,
		tab(Depth), write('CUT'), nl,
		tr_body(AfterCut, Depth)
	;   HadCut = no
	).


tr_body((Conj1,Conj2), Depth, AfterCut, HadCut) :- !,
	tr_body(Conj1, Conj2, Depth, AfterCut, HadCut).
tr_body(!, _, true, yes) :- !.
tr_body((Disj1;_), Depth, AfterCut, HadCut) :-
	tr_body(Disj1, Depth, AfterCut, HadCut).
tr_body((_;Disj2), Depth, AfterCut, HadCut) :- !,
	tr_body(Disj2, Depth, AfterCut, HadCut).
tr_body(true, _, true, no) :- !.
tr_body(Goal, Depth, true, no) :-
	tr_goal(Goal, Depth).

tr_body(!, AfterCut, _, AfterCut, yes) :- !.
tr_body((A,B), Conj, Depth, AfterCut, HadCut) :- !,
	tr_body(A, (B,Conj), Depth, AfterCut, HadCut).
tr_body((Disj1;_), Conj, Depth, AfterCut, HadCut) :-
	tr_body(Disj1, Conj, Depth, AfterCut, HadCut).
tr_body((_;Disj2), Conj, Depth, AfterCut, HadCut) :- !,
	tr_body(Disj2, Conj, Depth, AfterCut, HadCut).
tr_body(true, Body, Depth, AfterCut, HadCut) :- !,
	tr_body(Body, Depth, AfterCut, HadCut).
tr_body(Goal, Body, Depth, AfterCut, HadCut) :-
	tr_goal(Goal, Depth),
	tr_body(Body, Depth, AfterCut, HadCut).

% for compatibility with various prologs

not(X):-X,!,fail.
not(_).

term_chars(T,Cs):-nonvar(T),!,swrite(T,S),name(S,Cs).
term_chars(T,Cs):-name(S,Cs),sread(S,T).

gensym(Root,Symbol):-
        val(gensym,Root,N),!,
        N1 is N+1,
        symcat(Root,N1,Symbol),
        set(gensym,Root,N1).
gensym(Root,Symbol):-
        def(gensym,Root,1),
        symcat(Root,1,Symbol).

