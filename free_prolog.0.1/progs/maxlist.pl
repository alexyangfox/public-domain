go:-
  integers(Is,1,30000),
  time(_),
  max_of_list(Is,M),
	time(T),
  statistics,
	write([time=T,max=M]),nl.

time(T):-statistics(runtime,[_,T]).

integers([],I,I):-!.
integers([I0|L],I0,I):-I0<I,I1 is I0+1,integers(L,I1,I).

/*
From: jensk@hpbbn.bbn.hp.com (Jens Kilian)
Date: Fri, 29 Jan 1993 19:32:44 GMT
*/

max_of_list( [X|L], Max) :-
        max_of_list1( L, X, Max).

max_of_list1( [], Max, Max).
max_of_list1( [X|L], MaxSoFar, Max) :-
        compare( Rel, X, MaxSoFar),
        max_of_list2( Rel, L, X, MaxSoFar, Max).

max_of_list2( <, L, _, MaxSoFar, Max) :- max_of_list1( L, MaxSoFar, Max).
max_of_list2( =, L, _, MaxSoFar, Max) :- max_of_list1( L, MaxSoFar, Max).
max_of_list2( >, L, X, _,        Max) :- max_of_list1( L, X,        Max).
