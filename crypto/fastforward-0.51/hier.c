#include "auto_qmail.h"

void hier()
{
  h(auto_qmail,-1,-1,0755);

  d(auto_qmail,"bin",-1,-1,0755);
  d(auto_qmail,"doc",-1,-1,0755);
  d(auto_qmail,"doc/fastforward",-1,-1,0755);
  d(auto_qmail,"man",-1,-1,0755);
  d(auto_qmail,"man/man1",-1,-1,0755);
  d(auto_qmail,"man/cat1",-1,-1,0755);

  c(auto_qmail,"bin","fastforward",-1,-1,0755);
  c(auto_qmail,"bin","printforward",-1,-1,0755);
  c(auto_qmail,"bin","setforward",-1,-1,0755);
  c(auto_qmail,"bin","newaliases",-1,-1,0755);
  c(auto_qmail,"bin","printmaillist",-1,-1,0755);
  c(auto_qmail,"bin","setmaillist",-1,-1,0755);
  c(auto_qmail,"bin","newinclude",-1,-1,0755);

  c(auto_qmail,"doc/fastforward","ALIASES",-1,-1,0644);

  c(auto_qmail,"man/man1","fastforward.1",-1,-1,0644);
  c(auto_qmail,"man/man1","printforward.1",-1,-1,0644);
  c(auto_qmail,"man/man1","setforward.1",-1,-1,0644);
  c(auto_qmail,"man/man1","newaliases.1",-1,-1,0644);
  c(auto_qmail,"man/man1","printmaillist.1",-1,-1,0644);
  c(auto_qmail,"man/man1","setmaillist.1",-1,-1,0644);
  c(auto_qmail,"man/man1","newinclude.1",-1,-1,0644);

  c(auto_qmail,"man/cat1","fastforward.0",-1,-1,0644);
  c(auto_qmail,"man/cat1","printforward.0",-1,-1,0644);
  c(auto_qmail,"man/cat1","setforward.0",-1,-1,0644);
  c(auto_qmail,"man/cat1","newaliases.0",-1,-1,0644);
  c(auto_qmail,"man/cat1","printmaillist.0",-1,-1,0644);
  c(auto_qmail,"man/cat1","setmaillist.0",-1,-1,0644);
  c(auto_qmail,"man/cat1","newinclude.0",-1,-1,0644);
}
