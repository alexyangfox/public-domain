#include "PRE"

void c8192(register complex *a)
{
  cpassbig(a,d8192,1024);
  c2048(a + 6144);
  c2048(a + 4096);
  c4096(a);
}
