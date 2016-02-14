#include "PRE"

void c2048(register complex *a)
{
  cpassbig(a,d2048,256);
  c512(a + 1536);
  c512(a + 1024);
  c1024(a);
}
