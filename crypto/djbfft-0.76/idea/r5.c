#include "PRE"

void r8192(register real *a)
{
  rpassbig(a,d8192,1024);
  r4096(a);
  c2048((complex *)(a + 4096));
}
