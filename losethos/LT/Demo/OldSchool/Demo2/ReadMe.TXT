This demonstrates that linking can be done
at $FG,2$LOAD$FG$ time.  This is the LoseThos answer
to $FG,2$OBJ$FG$ files.  

Right-Click $FG,2$MakeFile.CPZ$FG$ and click $FG,2$INCLUDE$FG$ or
do the following:

1) Do a $LK,"Dir","MN:Dir"$() command

2) Right-Click on OD1.SPZ and click $FG,2$COMPILE$FG$.
   Right-Click on OD2.SPZ and click $FG,2$COMPILE$FG$.
   Right-Click on ODMain.SPZ and click $FG,2$COMPILE$FG$.

3) In any order, do the following and ignore warnings.
   Right-Click on OD1.BIZ and click $FG,2$LOAD$FG$.
   Right-Click on ODMain.BIZ and click $FG,2$LOAD$FG$.
   Right-Click on OD2.BIZ and click $FG,2$LOAD$FG$.

4) Right-Click on ObjDemo.HPZ and click $FG,2$INCLUDE$FG$.

5) Type at the command-line:
$ID,2$$FG,2$>ObjDemo("Param1","Param2");
$ID,-2$


$FG,2$#include$FG$ the file $LK,"::/LT/Adam/BinRep.APZ","FI:::/LT/Adam/BinRep.APZ"$ and
call $LK,"BinRep","MN:BinRep"$() to analyze the $FG,2$.BIZ$FG$ files.  
