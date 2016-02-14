#include "PRE"

void c4096(register complex *a)
{
  cpass(a,d4096,d2048,512);
  c1024(a);
  c512(a + 1024);
  c512(a + 1536);
  c1024(a + 2048);
  c1024(a + 3072);
}
