#include "PRE"

void c8192(register complex *a)
{
  cpass(a,d8192,d4096,1024);
  c2048(a + 6144);
  c2048(a + 4096);
  c1024(a + 3072);
  c1024(a + 2048);
  c2048(a);
}
