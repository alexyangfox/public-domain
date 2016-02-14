% TOPLEVEL
toplevel(bye):-
	repeat,
		telling(O),tell(user),cwrite('?- '),
		seeing(I),see(user),
		read_term(Body,Vs),
		topinterp(Body,Vs,I,O),
	!.
		
topinterp(end_of_file,_,_,_):-!.
topinterp(Goal,Vs,I,O):-
	see(I),tell(O),
%	let_top_answer((Vs:-Goal)),
	report_answers(Vs,Goal,Ok),
	telling(F),tell(user),cwrite(Ok),nl,tell(F),
	fail.

% let_top_answer(X):-	let(top,answer,X).
% get_top_answer(X):-val(top,answer,X).

[File]:-compile(File).

compile(File):-
	find_file(File,F),
  compile_mem(F),fail.

find_file(File,NewFile):-
	seeing(CF),
	find_file1(CF,File,NewFile).

find_file1(CF,File,NewFile):-
	see_a_file(
		["","progs/","myprogs/"],File,
		[".pl",".pro",""],NewFile),
	!,
	see(CF).
find_file1(CF,File,_):-
	see(CF),
	errmes('file * *.pl *.pro not found in . progs myprogs:',File).

see_a_file(Prefs,File,Sufs,NewFile):-
	member(Pref,Prefs),
	member(Suf,Sufs),	
	name(File,L),
	det_append(L,Suf,R),
	det_append(Pref,R,Fname),
	name(NewFile,Fname),
	see_or_fail(NewFile),
	seen.

report_answers([],Goal,yes):-
	metacall(Goal),!.
report_answers([V|Vs],Goal,_):-
	metacall(Goal),
		telling(F),
		tell(user),
		report_top_vars([V|Vs]),
		tell(F),
	fail.
report_answers(_,_,no).

report_top_vars(Eqs):-
	member(V=E,Eqs),
	cwrite(V),cwrite(=),writeq(E),nl,
	fail.
report_top_vars(_):-nl.

X is E:-meta_is(E,R),X=R,integer(X),!.
X is E:-errmes(error_in_is,X is E).

meta_is(E,R):-atomic(E),!,R=E.
meta_is(E,_Error):-var(E),!,fail.
meta_is(+(X),R):-!,meta_is(X,R).
meta_is(-(X),R):-!,meta_is(X,N),-(0,N,R).
meta_is(\(X),R):-!,meta_is(X,N),\(0,N,R).
meta_is(E,R):-E=..[Op,E1,E2],!,
	meta_is(E1,X1),
	meta_is(E2,X2),
	G=..[Op,X1,X2,R],
	G.

write(X):-portable_write(X).
print(X):-portable_print(X).
writeq(X):-portable_writeq(X).

p(X):-portable_write(X).

% read(X):-gc_read(X).
% r(X):-gc_read(X).

read(X):-portable_read(X).
r(X):-portable_read(X).

expand_term(C,E):-portable_expand_term(C,E).
std_expand_term(C,D):-portable_expand_term(C,D).

pp(Term):-nonvar(Term),portable_portray_clause(Term)->fail;true.

portray_clause(Term):-portable_portray_clause(Term).

metacall(Body):-do_body(Body).

/* R.A. O'Keefe's meta-circular interpreter (see also extra.pl) */

do_body(Body) :-
	do_body(Body, AfterCut, HadCut),
	( HadCut = yes,
		!,
		do_body(AfterCut)
	;   HadCut = no
	).

do_body((!,AfterCut), AfterCut, yes) :- !.
do_body((Goal,Body), AfterCut, HadCut) :- !,
	do_goal(Goal),
	do_body(Body, AfterCut, HadCut).
do_body(!, true, yes).
do_body((Disj1;_), AfterCut, HadCut) :-
	do_body(Disj1, AfterCut, HadCut).
do_body((_;Disj2), AfterCut, HadCut) :- !,
	do_body(Disj2, AfterCut, HadCut).
do_body(Goal, true, no) :-
	do_goal(Goal).

do_goal(Goal) :-
	is_compiled(Goal), % <--- check for a compiled predicate
	!,
	Goal.
do_goal(Goal) :-
	is_dynamic(Goal),
  !,
	clause(Goal, Body),	% <--- assume anything else is interpreted
	do_body(Body, AfterCut, HadCut),
	(	HadCut = yes,
		!,
		do_body(AfterCut)
	;	HadCut = no
	).
do_goal(Undef):-
	errmes('undefined predicate',Undef).
