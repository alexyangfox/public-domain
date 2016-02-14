#include "PRE"

void c1024(register complex *a)
{
  cpass(a,d1024,d512,128);
  c256(a + 768);
  c256(a + 512);
  c128(a + 384);
  c128(a + 256);
  c256(a);
}
