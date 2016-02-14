#include "PRE"

void u2048(register complex *a)
{
  u1024(a);
  u512(a + 1024);
  u512(a + 1536);
  upassbig(a,d2048,256);
}
