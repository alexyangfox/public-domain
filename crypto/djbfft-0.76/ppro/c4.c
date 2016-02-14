#include "PRE"

void c4096(register complex *a)
{
  cpassbig(a,d4096,512);
  c1024(a + 3072);
  c1024(a + 2048);
  c2048(a);
}
