#include "PRE"

void u8192(register complex *a)
{
  u4096(a);
  u2048(a + 4096);
  u2048(a + 6144);
  upassbig(a,d8192,1024);
}
