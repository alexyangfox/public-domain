$WW,1$If you insist on doing statically compiled, old school, programs instead of  just-in-time compiled programs, this directory tree has some examples.The main reason to do static programs is to make closed-source applications.Why not do it like the programs in $FG,2$/LT/Apps$FG$ instead?

See $LK,"Scoping and Linkages","FI:::/LT/Doc/ImportExports.TXZ"$ for details on $FG,2$extern$FG$, $FG,2$import$FG$, $FG,2$LTextern$FG$, $FG,2$LTimport$FG$, etc.

$FG,5$Demo1$FG$ shows a multiple file static module.
$FG,4$

$FG,5$Demo2$FG$ shows a multiple module project.If you $FG,2$#include $FG$the $FG,4$MakeFile$FG$ you'll see a clear example of how syms are exported and imported by modules and bound to syms in task's hash tables.

 
$FG,4$