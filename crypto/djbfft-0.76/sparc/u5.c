#include "PRE"

void u8192(register complex *a)
{
  u2048(a);
  u1024(a + 2048);
  u1024(a + 3072);
  u2048(a + 4096);
  u2048(a + 6144);
  upass(a,d8192,d4096,1024);
}
