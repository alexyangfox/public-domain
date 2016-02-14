#include "PRE"

void v1024(register real *a)
{
  u256((complex *)(a + 512));
  v512(a);
  vpassbig(a,d1024,128);
}
