p:-compile(co).

p(Term):-write(Term).
pp(Term):-portray_clause(Term).

r(X):-read(X).
std_expand_term(C,D):-expand_term(C,D).

cwrite(X):-write(X).
ttyprint(X):-telling(F),tell(user),write(X),nl,tell(F).

symcat(Op,Type,OpType):-	
	[Link]="_",
	name(Op,Pref),
	name(Type,Suf),
	append(Pref,[Link|Suf],Chars),
	!,
	name(OpType,Chars).
