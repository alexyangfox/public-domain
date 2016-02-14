all_permutations([],[[]]).
all_permutations([X|Xs],Perms2):-
       	all_permutations(Xs,Perms1),
        extend_permutations(Perms1,X,Perms2).

extend_permutations([],_,[]).
extend_permutations([Perm|Perms1],X,[[X|Perm]|Perms3]):-
	extend_permutations(Perms1,X,Perms2),
	insert_item(Perm,X,[],Perms2,Perms3).

insert_item([],_,_,Perms,Perms).
insert_item([Y|Ys],X,Acc,Perms1,[Zs|Perms2]):-
       	reverse_and_append(Acc,[Y,X|Ys],Zs),
        insert_item(Ys,X,[Y|Acc],Perms1,Perms2).

reverse_and_append([],Acc,Acc).
reverse_and_append([X|Xs],Acc,Zs):-
       reverse_and_append(Xs,[X|Acc],Zs).

nats(Max,Max,[Max]):-!.
nats(Curr,Max,[Curr|Ns]):-
	Curr<Max,
	Curr1 is Curr+1,
	nats(Curr1,Max,Ns).

perm([],[]).
perm([X|Xs],Zs):-
	perm(Xs,Ys),
	insert(X,Ys,Zs).

insert(X,Ys,[X|Ys]).
insert(X,[Y|Ys],[Y|Zs]):-
	insert(X,Ys,Zs).

g0(N):-nats(1,N,Ns),perm(Ns,_),fail.
g0(_).

g1(N,Ps):-nats(1,N,Ns),all_permutations(Ns,Ps).
g2(N,Ps):-nats(1,N,Ns),findall(P,perm(Ns,P),Ps).

test(Mes,X):-
	X,
	statistics(runtime,[_,T]),
	write('====>'),write(Mes),write(' time='),write(T),nl,
	statistics.

t0:-test('perms:',g0(8)).

t1:-test('allperms:',g1(8,_)).

t2:-test('allperms with findall:',g2(8,_)).

go:-write('execute with -h20000 option'),nl,t0,fail;t1,fail;t2,fail.

