% -----------------------------------------------------------------
% LIBRARY of basic predicates

true.

X=X.

A->B :- A,!,B.

A->B ; C :- !,if(A,B,C).
X ; _ :-X.
_ ; Y :-Y.

or(A,_):-A.
or(_,B):-B.

if(A,B,_):-A,!,B.
if(_,_,C):-C.

(X,Y):-X,Y.

call(X):-X.

\+(X):-X,!,fail.
\+(_).

repeat.
repeat:-repeat.

findall_workhorse(X,G,_):-
	lift_heap,
	G,
	findall_store_heap(X).
findall_workhorse(_,_,Xs):-
	findall_load_heap(Xs).	

findall(X,G,Xs,End):-findall_workhorse(X,G,[End|Xs]).

findall(X,G,Xs):-findall_workhorse(X,G,[[]|Xs]).

gc_read(R):-findall(X,portable_read(X),[R]).

gc_call(G):-findall(G,G,Gs),member(G,Gs).

for(Min,Min,Max):-Min=<Max.
for(I,Min,Max):-
        Min<Max,
        Min1 is Min+1,
        for(I,Min1,Max).

numbervars('$VAR'(N0), N0, N) :- !, N is N0+1.
numbervars(X, N0, N) :- atomic(X), !, N0=N.
numbervars([X|Xs], N0, N) :- !,
        numbervars(X, N0, N1),
        numbervars(Xs, N1, N).
numbervars(X, N0, N) :-
        functor(X, _, A),
        numbervars(0, A, X, N0, N).

numbervars(A, A, _, N0, N) :- !, N0=N.
numbervars(A0, A, X, N0, N) :-
        A1 is A0+1,
        arg(A1, X, X1),
        numbervars(X1, N0, N1),
        numbervars(A1, A, X, N1, N).

see(File):-see_or_fail(File),!.
see(File):-user_error(unable_to_see,File).

tell(File):-tell_or_fail(File),!.
tell(File):-user_error(unable_to_tell,File).

system([]):-!.
system([X|Xs]):-!,name(Command,[X|Xs]),shell(Command).
system(Command):-atomic(Command),shell(Command).

statistics(runtime,[Last,Now]):-runtime(Last,Now).
statistics(global_stack,[Used,Free]):-global_stack(Used,Free).
statistics(local_stack,[Used,Free]):-local_stack(Used,Free).
statistics(trail,[Used,Free]):-trail(Used,Free).
statistics(code,[Used,Free]):-code(Used,Free).
statistics(strings,[Used,Free]):-strings(Used,Free).
statistics(symbols,[Used,Free]):-symbols(Used,Free).
statistics(htable,[Used,Free]):-htable(Used,Free).
statistics(bboard,[Used,Free]):-bboard(Used,Free).

statistics:-
	statistics(Name,Data),
	cwrite(Name),cwrite(=),cwrite(Data),nl,
	fail.
statistics.	

atom(X):-integer(X),!,fail.
atom(X):-atomic(X).

number(X):-integer(X).

compound(X):-nonvar(X), \+(atomic(X)).

=..(T,[F|Xs]):-nonvar(T),!,functor(T,F,N),term2list(1,N,T,Xs).
=..(T,[F|Xs]):-get_length(Xs,0,N),!,functor(T,F,N),term2list(1,N,T,Xs).

term2list(I,N,_,[]):-I>N,!.
term2list(I,N,T,[X|Xs]):-I1 is I+1,arg(I,T,X),term2list(I1,N,T,Xs).

length(L,N):-var(N),!,get_length(L,0,N).
length(L,N):-make_length(L,0,N).

get_length([],I,I).
get_length([_|L],I0,I):-I1 is I0+1,get_length(L,I1,I).

make_length([],I,I):-!.
make_length([_|L],I0,I):-I0<I,I1 is I0+1,make_length(L,I1,I).

tab(N):-for(_,1,N),put(32),fail.
tab(_).

get(R):-repeat,get0(X),(X>32;X<0),!,R=X.

compare(R,X,Y):-compare0(X,Y,R).

A==B :- compare0(A,B,=).
A\==B :- compare0(A,B,R),'$noteq'(R).

A @< B :- compare0(A,B,<).
A @> B :- compare0(A,B,>).
A @=< B :- compare0(A,B,R),'$lesseq'(R).
A @>= B :- compare0(A,B,R),'$gteq'(R).

'$lesseq'(<).
'$lesseq'(=).

'$gteq'(>).
'$gteq'(=).

'$noteq'(<).
'$noteq'(>).

user_error(Mes,Obj):-
	telling(F),
	tell(user),
	cwrite('>>> '),cwrite(Mes),cwrite(': '),
	cwrite(Obj),nl,
	tell(F),
	fail.

% BinProlog extension: NAMING primitives

% naming of pointers to heap objects and copies of constants
% this is very fast, and can be used to save constant objects that
% survive backtracking

def(X,A):-saved(A,S),def('$noname',X,S).
set(X,A):-saved(A,S),set('$noname',X,S).
val(X,A):-val('$noname',X,A).

let(X,Y,V):-def(X,Y,V),!.
let(X,Y,V):-set(X,Y,V).

default_val(X,Y,V,_):-val(X,Y,V),!.
default_val(X,Y,D,D):-def(X,Y,D).

/* stacks */

push(Type,S,X):-default_val(Type,S,Xs,[]),saved([X|Xs],T),set(Type,S,T).

pop(Type,S,X):-default_val(Type,S,V,[]),V=[X|Xs],set(Type,S,Xs).

stack(Type,S,Xs):-val(Type,S,Xs).

push(S,X):-push('$stack',S,X).

pop(S,X):-pop('$stack',S,X).

stack(S,X):-stack('$stack',S,X).

/* bboard garbage collector */

bb_gc:-
	bb_list(B),
	copy_term(B,NewB),
	bb_reset,
	bb_put_back(NewB),
	fail.
bb_gc.

bb_put_back([]).
bb_put_back([O,M,V|Xs]):-
	bb_set(O,M,V),
	bb_put_back(Xs).

saved(X,S):-save_term(X,NewX),!,S=NewX.
saved(X,S):-bb_gc,save_term(X,S),!.
saved(X,_):-
	statistics(bboard,[_,Z]),
	errmes('blackboard overflow, left only',Z).

bb_set(_,_,V):-atomic(V),!.
bb_set(O,M,V):-saved(V,NewV),set(O,M,NewV),NewV=V.

% used in co.pl: cont. passing trick
push_cc0(Cont):-
	push('$cont',Cont),
	fail.

is_dynamic(P):-functor(P,F,N),val(F,'$arity',N).
