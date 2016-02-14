#include "PRE"

void c4096(register complex *a)
{
  cpassbig(a,d4096,512);
  c2048(a);
  c1024(a + 2048);
  c1024(a + 3072);
}
