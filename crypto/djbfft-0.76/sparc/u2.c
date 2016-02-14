#include "PRE"

void u1024(register complex *a)
{
  u256(a);
  u128(a + 256);
  u128(a + 384);
  u256(a + 512);
  u256(a + 768);
  upass(a,d1024,d512,128);
}
