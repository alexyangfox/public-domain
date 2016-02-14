#include "PRE"

void u4096(register complex *a)
{
  u1024(a);
  u512(a + 1024);
  u512(a + 1536);
  u1024(a + 2048);
  u1024(a + 3072);
  upass(a,d4096,d2048,512);
}
