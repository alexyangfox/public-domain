#include "PRE"

void c1024(register complex *a)
{
  cpassbig(a,d1024,128);
  c512(a);
  c256(a + 512);
  c256(a + 768);
}
