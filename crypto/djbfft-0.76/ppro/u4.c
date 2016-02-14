#include "PRE"

void u4096(register complex *a)
{
  u2048(a);
  u1024(a + 2048);
  u1024(a + 3072);
  upassbig(a,d4096,512);
}
