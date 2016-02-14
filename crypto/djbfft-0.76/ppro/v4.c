#include "PRE"

void v4096(register real *a)
{
  u1024((complex *)(a + 2048));
  v2048(a);
  vpassbig(a,d4096,512);
}
