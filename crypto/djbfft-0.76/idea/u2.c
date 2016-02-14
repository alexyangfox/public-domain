#include "PRE"

void u1024(register complex *a)
{
  u512(a);
  u256(a + 512);
  u256(a + 768);
  upassbig(a,d1024,128);
}
