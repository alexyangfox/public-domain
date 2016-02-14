subset([],[]).
subset([_|Xs],Ys):-subset(Xs,Ys).
subset([X|Xs],[X|Ys]):-subset(Xs,Ys).

gen_list(0,[]).
gen_list(N,[_|L]):-
        N>0, M is N-1,
        gen_list(M,L).

test(S):-gen_list(12,L),subset(L,S).

go:-write('use bp -h3000'),nl,
	statistics(runtime,_),
	findall(Q,test(Q),Qs),
	statistics(runtime,[_,T]),
	length(Qs,L),
	write([subsets(L),time(T)]),nl,
	statistics.


all:-gen_list(16,L),subset(L,_),fail.

t:-
	statistics(runtime,_),all,fail
;	statistics(runtime,[_,T]),write(time=T),nl.


pow(S,Xs):-findall(X,subset(S,X),Xs).

pow2(S,XXs):-findall(Xs,(pow(S,P),member(X,P),pow(X,Xs)),XXs).

g(N):-gen_list(N,Xs),pow2(Xs,S),length(S,L),write(L),nl.
