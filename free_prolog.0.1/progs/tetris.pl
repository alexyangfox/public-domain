% Tetris en Prolog (C) Paul Tarau 1989

% version avec evaluation complete mais non-optimisee
% on minimise "l'energie" du relief = la somme des
% hauteurs des briques, y compris les espaces vides
% (apres une chutte hypothetique)

% tetris9.pro
% 
% FAITES: d.

max(20,10). % L,C

% interface ALS

scr_clear:-for(_,1,60),nl,fail; true.

scr_send(p(L0,C0),Char):-
  L is L0+1, C is C0+1,
  put(27),
  cwrite('['),cwrite(L),
  cwrite(';'),cwrite(C),
  cwrite('H'),
  put(Char).

scr_rec(_):-fail.

rtest:-for(I,1,50),random(20,R),cwrite(R),nl,fail.

random(Max,R):-
	random(N),
	R is N mod Max.  

cputime(X):-statistics(runtime,[X,_]).

dir_depl(0,p( 0, 1)).   % right -77
dir_depl(1,p( 1, 0)).   % down -80
dir_depl(2,p( 0,-1)).   % left -75
dir_depl(3,p(-1, 0)).   % up -72

usr_dir(-77,0).
usr_dir(-80,1).
usr_dir(-75,2).
usr_dir(-72,3).
usr_dir(13,-1).
usr_dir(27,0):-fin.


% mouvement

next(Dir,p(L1,C1),p(L2,C2)):-
        max(MaxL,MaxC),
        dir_depl(Dir,p(DL,DC)),
        L2 is L1+DL,C2 is C1+DC,
        L2>=0,L2<MaxL,C2>=0,C2<MaxC.

select(-1,bloc(T,O1,P),bloc(T,O2,P)):-!,
        O2 is (O1+1) mod 4.
select(Dir,bloc(T,O,P1),bloc(T,O,P2)):-
        next(Dir,P1,P2).

/* lecture d'une direction: chutte par defaut */
scr_dir(1).
/*
scr_dir(D):-
        cputime(T0),
        repeat,
        ( scr_rec(C)->usr_dir(C,D)
        ; cputime(T1), DeltaT is T1-T0,DeltaT>0.20,D is 1
        ).
*/

% objets

/* image(Type,[Dir|Ds]) */
image(0,[0,1,0]).       % z.
image(1,[1,0,1]).       % -z.
image(2,[0,1,1]).       % l.
image(3,[0,0,1]).       % -l.
image(4,[1,1,1]).       % i.
image(5,[0,1,2,3]).     % carre.
image(6,[0,0,2,1]).     % a.

bloc2briques(B,Qs):-
        B=bloc(Type,_,_),
        image(Type,Dirs),
        bloc2briques(Dirs,B,Ps),
        sort(Ps,Qs).

bloc2briques([],bloc(_,_,P),[P]):-!.
bloc2briques([D|Ds],bloc(T,O,P1),[P1|Ps]):-
        Dir is (O+D) mod 4,
        next(Dir,P1,P2),
        !,
        bloc2briques(Ds,bloc(T,O,P2),Ps).

afficher_briques([],_):-!.
afficher_briques([P|Ps],Image):-
        scr_send(P,Image),
        !,
        afficher_briques(Ps,Image).

deplacer_briques(Old,New):- 
				"*"=[Brique],
        afficher_briques(Old,32),
        afficher_briques(New,Brique).     

tester_briques(Briques,Relief):-
        member(X,Briques),member(X,Relief)->fail
;       true.

deplacer_bloc(B1,B2,R):-
        bloc2briques(B2,New),
        tester_briques(New,R),
        bloc2briques(B1,Old),
        deplacer_briques(Old,New).

comprimer_relief(N1:R1,N2:R2):-
        max(MaxL,MaxC),
        bagof(Plein:L-Cs,
        Len^(
          bagof(C,member(p(L,C),R1),Cs),
          length(Cs,Len),
          ( Len=MaxC->Plein=1
          ; Plein=0
          )
        ),
        BLCs),
        !,
        eliminer(BLCs,LCs,N),
        !,
        N>0,N2 is N1+N,
        elements(LCs,R2).

score(N):-
        max(MaxL,MaxC),
        Score is N*MaxC,MesL is MaxL+1,
        scr_send(p(MesL,0),7),cwrite('Score:'),cwrite(Score).

element(LCs,p(L,C)):-member(L-Cs,LCs),member(C,Cs).

elements(LCs,Ps):-findall(P,element(LCs,P),Ps).

eliminer([],[],0):-!.
eliminer([L|Ls],Rs2,N2):-
        eliminer(Ls,Rs1,N1),
        enlever(L,Rs1,Rs2,N1,N2).

enlever(1:_,Rs,Rs,N1,N2):-N2 is N1+1,!.
enlever(0:L-Cs,Rs1,[L1-Cs|Rs1],N,N):-L1 is L+N.

touche(p(L,_),_):-max(M,_),L>=M,!. % no free space down
touche(P,Ps):-member(P,Ps),!.


% essayer

essayer_animer_bloc(B,N:R1,N:R2):-
	bloc2briques(B,Ps),
	essayer_descendre_bloc(Ps,R1,R2).

essayer_descendre_bloc(Ps1,R1,R2):-
        essayer_changer_bloc(Ps1,Ps2,R1),
        !,
        essayer_descendre_bloc(Ps2,R1,R2).
essayer_descendre_bloc(Ps,R1,R3):-       
        append(Ps,R1,R2),
        !,
        essayer_reduire(R2,R3).

essayer_reduire(R1,R2):-
        comprimer_relief(0:R1,_:R2),
        !.
essayer_reduire(R,R).

descendre([],[],MaxL):-!.
descendre([p(L1,C)|Ps1],[p(L2,C)|Ps2],MaxL):-
	L2 is L1+1,
	L2<MaxL,
	!,
	descendre(Ps1,Ps2,MaxL).

essayer_changer_bloc(Bs1,Bs2,R):-
	max(MaxL,_),
	descendre(Bs1,Bs2,MaxL),
  ( member(X,Bs2),member(X,R)->fail
  ; true
	),
  !.


% evaluer

minimiser_energie_relief(B0,_,N:R1):-
        B0=bloc(Type,_,_),
        set_best(B0,32000),
        generer_bloc(Type,B),
        essayer_animer_bloc(B,N:R1,_:R2),
        energie(R2,Val),
        the_best(OldB,OldVal),
        Val<OldVal,
        set_best(B,Val),
        deplacer_bloc(OldB,B,R1),
        max(MaxL,_),L is MaxL+2,
				[Prompt]=">",
        scr_send(p(L,0),Prompt),
        statistics(global_stack,Stat),
        write('Energie':Val),write(' Heap':Stat),write('    '),
        fail.
minimiser_energie_relief(_,B,_):-
        the_best(B,_).

generer_bloc(Type,bloc(Type,Orientation,p(3,C))):-
        max(_,MaxC),
        MaxC1 is MaxC-1,
        for(C,0,MaxC1),
        for(Orientation,0,3).

surface(R1,S):-
        findall(C-Ls,
              setof(L,member(p(L,C),R1),Ls),
        CLs),
        findall(p(L1,C0),
           (member(C0-[L0|_],CLs),L1 is L0-1),
        S).

energie(Briques,G):-
        surface(Briques,S),
        energie(S,0,G).

energie([],G,G):-!.
energie([p(L,_)|Ps],G1,G3):-
        max(MaxL,_),
        G2 is G1+((MaxL-L)*(MaxL-L+1) // 2),
        !,
        energie(Ps,G2,G3).

init_best:-def(v,32000),!,def(id,3),def(dir,0),def(l,1),def(c,1).
init_best:-set(v,32000).

set_best(bloc(Id,Dir,p(L,C)),V):-
	set(id,Id),set(dir,Dir),set(l,L),set(c,C),set(v,V).

the_best(bloc(Id,Dir,p(L,C)),V):-
	val(id,Id),val(dir,Dir),val(l,L),val(c,C),val(v,V). 

% animer

impact(B,N:R1,N:R2):-
        bloc2briques(B,Ps),       
        member(p(L,C),Ps),L1 is L+1,
        touche(p(L1,C),R1),
        append(Ps,R1,R2),
        !.

reduire(N1:R1,N2:R2):-
        comprimer_relief(N1:R1,N2:R2),
        deplacer_briques(R1,R2),
        score(N2),
        !.
reduire(R,R).


changer_bloc(B1,B2,_:R):-
        scr_dir(Dir),
        select(Dir,B1,B2),
        deplacer_bloc(B1,B2,R),
        !.
changer_bloc(B,B,_).

animer_bloc(B,R1,R3):-impact(B,R1,R2),!,
        reduire(R2,R3).
animer_bloc(B1,R1,R2):-
        changer_bloc(B1,B2,R1),
        !,
        animer_bloc(B2,R1,R2).


% jouer

plein(Relief):-member(p(L,_),Relief), L=<5. % no free space up

creer_bloc(bloc(Type,0,p(5,MidC))):-!,
        max(_,MaxC),MidC is MaxC // 2,
        random(7,Type).

meilleur_bloc(jeu,B0,B0,_):-!.
meilleur_bloc(demo,B0,B,NR):-
        minimiser_energie_relief(B0,B,NR).

jouer(_,_:Relief):-plein(Relief),!.
jouer(Mode,Relief1):-
        creer_bloc(B0),
        meilleur_bloc(Mode,B0,B,Relief1),
        !,
        gc_call(animer_bloc(B,Relief1,Relief2)),
        !,
        jouer(Mode,Relief2).

init(N:[]):-
        scr_clear,
        max(MaxL,MaxC),[Board]="#",
        init_best,
        (for(L,6,MaxL),scr_send(p(L,MaxC),Board),fail; true),
        (for(C,0,MaxC),scr_send(p(MaxL,C),Board),fail; true),
        N=0,score(N),
        !.

fin :- 
        max(L,_),L1 is L+3,
        scr_send(p(L1,0),32),nl,
        abort.

go(Mode):-
        init(State),
        jouer(Mode,State),
        fin.

g:-go(jeu).

go:-go(demo).

d:-go.
