#include "PRE"

void u2048(register complex *a)
{
  u512(a);
  u256(a + 512);
  u256(a + 768);
  u512(a + 1024);
  u512(a + 1536);
  upass(a,d2048,d1024,256);
}
