#include "PRE"

const char optid[] = "ppro";

void c2(register complex *a)
{
  register real t1;

  t1 = a[1].re;
  a[1].re = a[0].re - t1;
  a[0].re += t1;

  t1 = a[1].im;
  a[1].im = a[0].im - t1;
  a[0].im += t1;
}

inline void c4(register complex *a)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;

  t5 = a[2].re;
  t1 = a[0].re - t5;
  t7 = a[3].re;
  t5 += a[0].re;
  t3 = a[1].re - t7;
  t7 += a[1].re;
  t8 = t5 + t7;
  a[0].re = t8;
  t5 -= t7;
  a[1].re = t5;
  t6 = a[2].im;
  t2 = a[0].im - t6;
  t6 += a[0].im;
  t5 = a[3].im;
  a[2].im = t2 + t3;
  t2 -= t3;
  a[3].im = t2;
  t4 = a[1].im - t5;
  a[3].re = t1 + t4;
  t1 -= t4;
  a[2].re = t1;
  t5 += a[1].im;
  a[0].im = t6 + t5;
  t6 -= t5;
  a[1].im = t6;
}

void c8(register complex *a)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;

  t7 = a[4].im;
  t4 = a[0].im - t7;
  t7 += a[0].im;
  a[0].im = t7;

  t8 = a[6].re;
  t5 = a[2].re - t8;
  t8 += a[2].re;
  a[2].re = t8;

  t7 = a[6].im;
  a[6].im = t4 - t5;
  t4 += t5;
  a[4].im = t4;

  t6 = a[2].im - t7;
  t7 += a[2].im;
  a[2].im = t7;

  t8 = a[4].re;
  t3 = a[0].re - t8;
  t8 += a[0].re;
  a[0].re = t8;

  a[4].re = t3 - t6;
  t3 += t6;
  a[6].re = t3;

  t7 = a[5].re;
  t3 = a[1].re - t7;
  t7 += a[1].re;
  a[1].re = t7;

  t8 = a[7].im;
  t6 = a[3].im - t8;
  t8 += a[3].im;
  a[3].im = t8;
  t1 = t3 - t6;
  t3 += t6;

  t7 = a[5].im;
  t4 = a[1].im - t7;
  t7 += a[1].im;
  a[1].im = t7;

  t8 = a[7].re;
  t5 = a[3].re - t8;
  t8 += a[3].re;
  a[3].re = t8;

  t2 = t4 - t5;
  t4 += t5;

  t6 = t1 - t4;
  t8 = sqrthalf;
  t6 *= t8;
  a[5].re = a[4].re - t6;
  t1 += t4;
  t1 *= t8;
  a[5].im = a[4].im - t1;
  t6 += a[4].re;
  a[4].re = t6;
  t1 += a[4].im;
  a[4].im = t1;

  t5 = t2 - t3;
  t5 *= t8;
  a[7].im = a[6].im - t5;
  t2 += t3;
  t2 *= t8;
  a[7].re = a[6].re - t2;
  t2 += a[6].re;
  a[6].re = t2;
  t5 += a[6].im;
  a[6].im = t5;

  c4(a);
}

void c16(register complex *a)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;

  TRANSFORMZERO(a[0],a[4],a[8],a[12]);
  TRANSFORM(a[1],a[5],a[9],a[13],d16[0].re,d16[0].im);
  TRANSFORMHALF(a[2],a[6],a[10],a[14]);
  TRANSFORM(a[3],a[7],a[11],a[15],d16[0].im,d16[0].re);
  c4(a + 8);
  c4(a + 12);

  c8(a);
}

/* a[0...8n-1], w[0...2n-2]; n >= 2 */
void cpass(register complex *a,register const complex *w,register unsigned int n)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;
  register complex *a1;
  register complex *a2;
  register complex *a3;

  a2 = a + 4 * n;
  a1 = a + 2 * n;
  a3 = a2 + 2 * n;
  --n;

  TRANSFORMZERO(a[0],a1[0],a2[0],a3[0]);
  TRANSFORM(a[1],a1[1],a2[1],a3[1],w[0].re,w[0].im);

  for (;;) {
    TRANSFORM(a[2],a1[2],a2[2],a3[2],w[1].re,w[1].im);
    TRANSFORM(a[3],a1[3],a2[3],a3[3],w[2].re,w[2].im);
    if (!--n) break;
    a += 2;
    a1 += 2;
    a2 += 2;
    a3 += 2;
    w += 2;
  }
}

void c32(register complex *a)
{
  cpass(a,d32,4);
  c8(a + 16);
  c8(a + 24);
  c16(a);
}

void c64(register complex *a)
{
  cpass(a,d64,8);
  c16(a + 32);
  c16(a + 48);
  c32(a);
}

void c128(register complex *a)
{
  cpass(a,d128,16);
  c32(a + 64);
  c32(a + 96);
  c64(a);
}

void c256(register complex *a)
{
  cpass(a,d256,32);
  c64(a + 128);
  c64(a + 192);
  c128(a);
}

void c512(register complex *a)
{
  cpass(a,d512,64);
  c128(a + 384);
  c128(a + 256);
  c256(a);
}
