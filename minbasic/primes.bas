  100  LET N = 1000
  110  DIM Z(N)
  120  LET Z(0) = 2, T = 1, P = 1
  130  PRINT 2,
  140  WHILE T < N
  150    LET P = P + 2
  160    FOR I = 0 TO T - 1
  170      LET R = P / Z(I) * Z(I)
  180      IF R = P EXIT
  190    NEXT
  200    IF I < T - 1 GOTO 150
  210    LET Z(T) = P, T = T + 1
  220    PRINT P,
  230  WEND
  240  END
