go:-
	tell('defs.h'),
	make_defs,
  told,
  tell('prof.h'),
  make_names,
	told.

make_defs:-
  n_inline(First),n_arith(Arith),n_builtin(Bu),
  number_bu(First,Last,N-X),
  write('#define '),write(X),write(' '),write(N),nl,
  N=:=Last,
  !,
  nl,write('#define INLINE '),write(First),nl,nl,
  nl,write('#define ARITH '),write(Arith),nl,nl,
  nl,write('#define BUILTIN '),write(Bu),nl,nl,
  nl,write('#define LAST_BUILTIN '),write(Last),nl,nl.

make_names:-
  n_inline(Max),
  write('#ifdef PROF'),nl,
  write('char *bu_name[]={'),nl,
  number_bu(Max,Last,N-X),
  write('"'),write(X),write('",'),nl,
  N=:=Last,
  !,
  write('"LAST_BUILTIN"};'),nl,write('#endif'),nl,nl.


number_bu(First,Last,N-X):-
  findall(B,bname(B),Bs),
  length(Bs,L),Last is First+L-1,
  member_i(X,Bs,First,N).

to_upper1(X,Y):-[A]="a",[Z]="z",X>=A,X=<Z,[AA]="A",!,
  Y is AA+(X-A).  
to_upper1(X,X).

to_upper([],[]).
to_upper([X|Xs],[Y|Ys]):-
  to_upper1(X,Y),
  to_upper(Xs,Ys).

b_idiom(+,"PLUS").
b_idiom(-,"SUB").
b_idiom(*,"MUL").
b_idiom(//,"DIV").
b_idiom(put,"PUT0").
b_idiom(<,"LESS").
b_idiom(>,"GREATER").
b_idiom(=<,"LESS_EQ").
b_idiom(>=,"GREATER_EQ").
b_idiom(=:=,"ARITH_EQ").
b_idiom(=\=,"ARITH_DIF").
b_idiom(<<,"LSHIFT").
b_idiom(>>,"RSHIFT").
b_idiom(/\,"L_AND").
b_idiom(\/,"L_OR").
b_idiom(#,"L_XOR").
b_idiom(\,"L_NEG").
b_idiom('$demo',"DEMO").

to_bname(F,U):-b_idiom(F,U),!.
to_bname(F,U):-name(F,L),to_upper(L,U).

bname(Name):-
  [X]="_",
  is_builtin(F/N),name(N,Arity),
  to_bname(F,U),
  append(U,[X|Arity],List),
  name(Name,List).

nth_member(X,Xs,N):-member_i(X,Xs,1,N).

member_i(X,[X|_],N,N).
member_i(X,[_|Xs],N1,N3):-
  N2 is N1+1,
  member_i(X,Xs,N2,N3).

