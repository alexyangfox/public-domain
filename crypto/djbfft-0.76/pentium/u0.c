#include "PRE"

inline void u4(register complex *a)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;

  t1 = a[0].re + a[1].re;
  t2 = a[0].im + a[1].im;
  t6 = a[3].re + a[2].re;
  t4 = t1;
  t5 = t2;
  t4 += t6;
  t1 -= t6;
  t3 = VOL a[0].re;
  t6 = a[2].im + a[3].im;
  a[0].re = t4;
  t4 = VOL a[0].im;
  t5 += t6;
  t3 -= a[1].re;
  t2 -= t6;
  t4 -= a[1].im;
  a[0].im = t5;
  t7 = a[2].im - a[3].im;
  t8 = a[3].re - a[2].re;
  t5 = t3;
  t6 = t4;
  t3 -= t7;
  t4 -= t8;
  t5 += t7;
  t6 += t8;
  a[2].re = t1;
  a[1].re = t5;
  a[3].re = t3;
  a[1].im = t6;
  a[3].im = t4;
  a[2].im = t2;
}

void u8(register complex *a)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;

  u4(a);

  t7 = a[4].re - a[5].re;
  t8 = a[4].im - a[5].im;
  t1 = a[4].re + a[5].re;
  a[5].re = t7;
  t2 = a[4].im + a[5].im;
  a[5].im = t8;

  t7 = a[6].re - a[7].re;
  t8 = a[6].im - a[7].im;
  t3 = a[6].re + a[7].re;
  a[7].re = t7;
  t4 = a[6].im + a[7].im;
  a[7].im = t8;

  t8 = t3;
  t7 = t2;
  t7 -= t4;
  t8 -= t1;
  t1 += t3;
  t2 += t4;
  t3 = a[2].re - t7;
  t7 += a[2].re;
  t4 = a[2].im - t8;
  t8 += a[2].im;
  a[6].re = t3;
  t5 = a[0].re - t1;
  t1 += a[0].re;
  a[2].re = t7;
  t6 = a[0].im - t2;
  t2 += a[0].im;
  a[4].re = t5;
  a[0].re = t1;
  a[4].im = t6;
  a[0].im = t2;
  a[2].im = t8;
  a[6].im = t4;

  t1 = a[5].re + a[5].im;
  t2 = a[5].im - a[5].re;
  t1 *= sqrthalf;
  t3 = a[7].re - a[7].im;
  t2 *= sqrthalf;
  t4 = a[7].im + a[7].re;
  t3 *= sqrthalf;
  t7 = t2;
  t4 *= sqrthalf;
  t8 = t3;
  t8 -= t1;
  t7 -= t4;
  t1 += t3;
  t2 += t4;
  t4 = a[3].im - t8;
  t8 += a[3].im;
  t3 = a[3].re - t7;
  t7 += a[3].re;
  a[7].im = t4;
  t5 = a[1].re - t1;
  t1 += a[1].re;
  a[3].im = t8;
  t6 = a[1].im - t2;
  t2 += a[1].im;
  a[5].re = t5;
  a[1].re = t1;
  a[5].im = t6;
  a[3].re = t7;
  a[7].re = t3;
  a[1].im = t2;
}

void u16(register complex *a)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;

  u8(a);
  u4(a + 8);
  u4(a + 12);

  UNTRANSFORMZERO(a[0],a[4],a[8],a[12]);
  UNTRANSFORMHALF(a[2],a[6],a[10],a[14]);
  UNTRANSFORM(a[1],a[5],a[9],a[13],d16[0].re,d16[0].im);
  UNTRANSFORM(a[3],a[7],a[11],a[15],d16[0].im,d16[0].re);
}

/* a[0...8n-1], w[0...2n-2] */
void upass(register complex *a,register const complex *w,register unsigned int n)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;
  register unsigned int k;
  register complex *b;

  b = a + 4 * n;
  k = n - 1;
  n <<= 3;

  UNTRANSFORMZERO(A0[0],A1[0],A2[0],A3[0]);
  UNTRANSFORM(A0[1],A1[1],A2[1],A3[1],w[0].re,w[0].im);

  for (;;) {
    UNTRANSFORM(A0[2],A1[2],A2[2],A3[2],w[1].re,w[1].im);
    UNTRANSFORM(A0[3],A1[3],A2[3],A3[3],w[2].re,w[2].im);
    if (!--k) break;
    a += 2;
    b += 2;
    w += 2;
  }
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
