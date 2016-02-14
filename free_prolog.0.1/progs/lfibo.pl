:-write('Program: lfibo.pl'),nl.
:-write('Author: Paul Tarau'),nl.
:-write('fibonacci(40) program with constant time lemmas'),nl.
:-write('executed 10000 times'),nl.

go:-
        I=10000,N=40,
        statistics(runtime,_),
        statistics(global_stack,[H1,_]),
        statistics(trail,[TR1,_]),
        f_iter(I,N,R),
        statistics(runtime,[_,T]),
        statistics(global_stack,[H2,_]),
        statistics(trail,[TR2,_]),
        H is H2-H1,TR is TR2-TR1,
        bb,
        write([time=T,heap=H,trail=TR,fibo(N,R)]),nl.

range(Min,Min,Max):-Min=<Max.
range(I,Min,Max):-
        Min<Max,
        Min1 is Min+1,
        range(I,Min1,Max).
        
f_iter(Max,N,R):-range(_,1,Max),fibo(N,R),fail.
f_iter(_,N,R):-fibo(N,R).

fibo(N,1):-N<2,!.
fibo(N,Y):-
	N1 is N-1, N2 is N-2,
	lemma2(fibo,N1,Y1),
	lemma2(fibo,N2,Y2),
	Y is Y1+Y2.

/* somewhat slower version:

xfibo(N,1):-N<2,!.
xfibo(N,Y):-N1 is N-1, N2 is N-2,
	xlemma(xfibo(N1,Y1)),
	xlemma(xfibo(N2,Y2)),
	Y is Y1+Y2.

*/

