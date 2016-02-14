#include "PRE"

void c2048(register complex *a)
{
  cpass(a,d2048,d1024,256);
  c512(a + 1536);
  c512(a + 1024);
  c256(a + 768);
  c256(a + 512);
  c512(a);
}
