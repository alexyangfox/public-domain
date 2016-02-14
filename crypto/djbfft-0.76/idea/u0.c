#include "PRE"

void u4(register complex *a)
{
  register real t1, t2, t3, t4;

  u2(a);
  UNTRANSFORMZERO(a[0],a[1],a[2],a[3]);
}

void u8(register complex *a)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;

  u4(a);
  u2(a + 4);
  u2(a + 6);
  UNTRANSFORMZERO(a[0],a[2],a[4],a[6]);
  UNTRANSFORMHALF(a[1],a[3],a[5],a[7]);
}

/* a[0...8n-1], w[0...2n-2] */
void upass(register complex *a,register const complex *w,register unsigned int n)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;
  register unsigned int k;

  UNTRANSFORMZERO(a[0],a[2 * n],a[4 * n],a[6 * n]);

  k = 2 * n - 1;

  do {
    UNTRANSFORM(a[1],a[2 * n + 1],a[4 * n + 1],a[6 * n + 1],w[0].re,w[0].im);
    ++w;
    ++a;
  } while (--k);
}

void u16(register complex *a)
{
  u8(a);
  u4(a + 8);
  u4(a + 12);
  upass(a,d16,2);
}

void u32(register complex *a)
{
  u16(a);
  u8(a + 16);
  u8(a + 24);
  upass(a,d32,4);
}

void u64(register complex *a)
{
  u32(a);
  u16(a + 32);
  u16(a + 48);
  upass(a,d64,8);
}

void u128(register complex *a)
{
  u64(a);
  u32(a + 64);
  u32(a + 96);
  upass(a,d128,16);
}

void u256(register complex *a)
{
  u128(a);
  u64(a + 128);
  u64(a + 192);
  upass(a,d256,32);
}

void u512(register complex *a)
{
  u256(a);
  u128(a + 256);
  u128(a + 384);
  upass(a,d512,64);
}
