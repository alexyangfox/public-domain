#include "auto_qmail.h"

void hier()
{
  h(auto_qmail,-1,-1,0755);

  d(auto_qmail,"bin",-1,-1,0755);
  d(auto_qmail,"man",-1,-1,0755);
  d(auto_qmail,"man/man1",-1,-1,0755);
  d(auto_qmail,"man/cat1",-1,-1,0755);

  c(auto_qmail,"bin","dot-forward",-1,-1,0755);

  c(auto_qmail,"man/man1","dot-forward.1",-1,-1,0644);
  c(auto_qmail,"man/cat1","dot-forward.0",-1,-1,0644);
}
