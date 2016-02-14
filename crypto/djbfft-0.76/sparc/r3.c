#include "PRE"

void r2048(register real *a)
{
  rpass(a,d2048,256);
  r1024(a);
  c512((complex *)(a + 1024));
}
