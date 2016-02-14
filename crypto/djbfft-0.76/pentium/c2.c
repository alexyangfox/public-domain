#include "PRE"

void c1024(register complex *a)
{
  cpassbig(a,d1024,128);
  c256(a + 768);
  c256(a + 512);
  c512(a);
}
