#include "PRE"

void c8192(register complex *a)
{
  cpassbig(a,d8192,1024);
  c4096(a);
  c2048(a + 4096);
  c2048(a + 6144);
}
