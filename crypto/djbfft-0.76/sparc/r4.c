#include "PRE"

void r4096(register real *a)
{
  rpass(a,d4096,512);
  r2048(a);
  c1024((complex *)(a + 2048));
}
