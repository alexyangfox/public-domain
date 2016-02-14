  100  LET S = 20 : REM MAX. SIZE
  110  LET S1 = S * S : REM ARRAY SIZE
  120  DIM Z(S1 * 3)
  130  LET N = 20 : REM SIZE
  140  LET M = N * 3 / 2 : REM MINES
  150  LET X = 0, Y = 0 : REM COORDINATES
  160  LET U = S1 - M : REM UNCHECKED FIELDS
  170  LET F1 = ASC('+') : REM UNCHECKED CHAR
  180  LET F2 = ASC('.') : REM CHECKED CHAR
  190  LET F3 = ASC('@') : REM MINE CHAR
  200  LET F4 = 127 : REM INTERNAL MINE CHAR

  210  REM *** MAIN ***
  220  GOSUB &CLEAR
  230  GOSUB &MINES
  240  GOSUB &PRINT
  250  LET U = CALL(&COUNT WITH W = F1)
  260  IF NOT U GOTO 320
  270  ON CALL(&INPUT) GOTO 270, 220, 410, 240
  280  IF CALL(&GET) <> F4 THEN
  290    GOSUB &SCAN
  300    GOTO 240
  310  ENDIF
  320  GOSUB &MARK
  330  GOSUB &PRINT
  340  IF U THEN
  350    PRINT U; ' FIELDS LEFT UNCHECKED.';
  360  ELSE
  370    PRINT 'PERFECT GAME!';
  380  ENDIF
  390  PRINT '   AGAIN? (Y/N) '; : INPUT A$
  400  IF MID$(A$, 1, 1) = 'Y' GOTO 220
  410  END

  420  LABEL &GET : REM X,Y = COORDINATES
  430  IF X < 1 OR Y < 1 OR X > N OR Y > N RETURN ASC('X')
  440  RETURN Z((Y - 1) * N + X - 1)

  450  LABEL &PUT : REM X,Y = COORDS, W = WHAT
  460  LET Z((Y - 1) * N + X - 1) = W
  470  RETURN

  480  LABEL &CLEAR
  490  LOCAL I, J
  500    FOR J = 1 TO N
  510      FOR I = 1 TO N
  520        GOSUB &PUT WITH X = I, Y = J, W = F1
  530      NEXT
  540    NEXT
  550  RETURN

  560  LABEL &MARK
  570  LOCAL I, J
  580    FOR J = 1 TO N
  590      FOR I = 1 TO N
  600        IF CALL(&GET WITH X = I, Y = J) = F4 THEN
  610          GOSUB &PUT WITH X = I, Y = J, W = F3
  620        ELSE
  630          IF NOT U AND CALL(&GET WITH X = I, Y = J) = F2 THEN
  640            GOSUB &PUT WITH X = I, Y = J, W = ASC(' ')
  650          ENDIF
  660        ENDIF
  670      NEXT
  680    NEXT
  690  RETURN

  700  LABEL &COUNT : REM W = WHAT
  710  LOCAL I, J, C = 0
  720    FOR J = 1 TO N
  730      FOR I = 1 TO N
  740        IF CALL(&GET WITH X = I, Y = J) = W LET C = C + 1
  750      NEXT
  760    NEXT
  770  RETURN C

  780  LABEL &MINES
  790  LOCAL I, C1, C2
  800    FOR I = 1 TO M
  810      REPEAT
  820        LET C1 = RND(N) + 1, C2 = RND(N) + 1
  830      UNTIL CALL(&GET WITH X = C1, Y = C2) <> F4
  840      GOSUB &PUT WITH X = C1, Y = C2, W = F4
  850    NEXT
  860  RETURN

  870  LABEL &INPUT
  880  LOCAL R = 0
  890    REPEAT
  900      PRINT U; '> '; : INPUT A$
  910      LET X = ASC(A$) - ASC('A') + 1
  920      LET Y = VAL(MID$(A$, 2))
  930      IF Y = 0 LET R = CALL(&COMMAND) : EXIT
  940    UNTIL X > 0 AND X <= N AND Y > 0 AND Y <= N
  950  RETURN R

  960  LABEL &COMMAND
  970  LOCAL I = POS(' ', A$), B$ = ''
  980    IF I LET B$ = MID$(A$, I + 1), A$ = MID$(A$, 1, I - 1)
  990    IF A$ = 'CHARS' GOSUB &SET.CHARS : RETURN 4
 1000    IF A$ = 'QUIT' RETURN 3
 1010    IF A$ = 'MINES' GOSUB &SET.MINES : RETURN 2
 1020    IF A$ = 'NEW' RETURN 2
 1030    IF A$ = 'SIZE' GOSUB &SET.SIZE : RETURN 2
 1040    IF A$ = '' RETURN 4
 1050    IF A$ = 'HELP' GOSUB &HELP : RETURN 1
 1060  RETURN 1

 1070  LABEL &SET.CHARS
 1080  LOCAL G1 = F1, G2 = F2, G3 = F3
 1090    IF MID$(B$, 1, 1) <> '' LET G1 = ASC(MID$(B$, 1, 1))
 1100    IF MID$(B$, 2, 1) <> '' LET G2 = ASC(MID$(B$, 2, 1))
 1110    IF MID$(B$, 3, 1) <> '' LET G3 = ASC(MID$(B$, 3, 1))
 1120    GOSUB &REPLACE WITH C1 = F1, C2 = 0
 1130    GOSUB &REPLACE WITH C1 = F2, C2 = G2
 1140    GOSUB &REPLACE WITH C1 = 0, C2 = G1
 1150    LET F1 = G1, F2 = G2, F3 = G3
 1160  RETURN 4

 1170  LABEL &REPLACE : REM C1 = OLD, C2 = NEW
 1180  LOCAL I, J
 1190    FOR J = 1 TO N
 1200      FOR I = 1 TO N
 1210        IF CALL(&GET WITH X = I, Y = J) = C1 THEN
 1220          GOSUB &PUT WITH X = I, Y = J, W = C2
 1230        ENDIF
 1240      NEXT
 1250    NEXT
 1260  RETURN

 1270  LABEL &SET.MINES
 1280  LET M = VAL(B$)
 1290  IF M > N * 2 PRINT 'TOO MANY!' : LET M = N * 2
 1300  RETURN 2

 1310  LABEL &SET.SIZE
 1320  LET N = VAL(B$)
 1330  IF N < 10 PRINT 'TOO SMALL!' : LET N = 10
 1340  IF N > 20 PRINT 'TOO BIG!' : LET N = 20
 1350  LET M = N * 3 / 2
 1360  RETURN 2

 1370  LABEL &HELP
 1380  PRINT 'A1...T20   CHECK COORDINATES'
 1390  PRINT 'CHARS UCM  CHANGE (UNCHECKED, CHECKED, MINE) CHARS'
 1400  PRINT 'MINES #    CHANGE NUMBER OF MINES'
 1410  PRINT 'NEW        NEW GAME'
 1420  PRINT 'QUIT       QUIT'
 1430  PRINT 'SIZE #     CHANGE FIELD SIZE'
 1440  PRINT '(EMPTY)    REDRAW FIELD'
 1450  RETURN

 1460  LABEL &PRINT
 1470  LOCAL I, J
 1480    GOSUB &CLRSCR
 1490    GOSUB &XLABS
 1500    FOR J = 1 TO N
 1510      IF J < 10 PRINT ' ';
 1520      PRINT J;
 1530      FOR I = 1 TO N
 1540        LET C = CALL(&GET WITH X = I, Y = J)
 1550        IF C = F4 LET C = F1
 1560        PRINT ' '; CHR$(C);
 1570      NEXT
 1580      PRINT ' '; J
 1590    NEXT
 1600    GOSUB &XLABS
 1610  RETURN

 1620  LABEL &CLRSCR
 1630  LOCAL I
 1640    FOR I = 1 TO 25
 1650      PRINT
 1660    NEXT
 1670  RETURN

 1680  LABEL &XLABS
 1690  LOCAL I
 1700    PRINT '  ';
 1710    FOR I = 1 TO N
 1720      PRINT ' '; CHR$(I + ASC('A') - 1);
 1730    NEXT
 1740    PRINT
 1750  RETURN

 1760  LABEL &SCAN : REM X,Y = COORDINATES
 1770  LET P = 0, L = F2
 1780  REPEAT
 1790    LET K = CALL(&CHECK)
 1800    LET C = CALL(&GET)
 1810    IF P AND (K OR C <> F1) THEN
 1820      IF K AND C = F1 GOSUB &PUT WITH W = K + ASC('0')
 1830      LET Y = Z(S1 - 1 + P), X = Z(S1 - 2 + P), D = CALL(&GET)
 1840      ON D GOTO 1860, 1870, 1880
 1850      GOSUB &PUT WITH W = L : LET P = P - 2 : GOTO 1940
 1860      GOSUB &PUT WITH W = 2 : LET X = X + 1 : GOTO 1940
 1870      GOSUB &PUT WITH W = 3 : LET Y = Y - 1 : GOTO 1940
 1880      GOSUB &PUT WITH W = 4 : LET Y = Y + 1 : GOTO 1940
 1890    ELSE
 1900      GOSUB &PUT WITH W = 1
 1910      LET Z(S1 + P) = X, Z(S1 + 1 + P) = Y, P = P + 2
 1920      LET X = X - 1
 1930    ENDIF
 1940  UNTIL P = 0
 1950  RETURN

 1960  LABEL &CHECK
 1970  LOCAL I, J, C = 0
 1980    FOR I = X - 1 TO X + 1
 1990      FOR J = Y - 1 TO Y + 1
 2000        IF CALL(&GET WITH X = I, Y = J) = F4 LET C = C + 1
 2010      NEXT
 2020    NEXT
 2030  RETURN C
