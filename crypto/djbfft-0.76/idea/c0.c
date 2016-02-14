#include "PRE"

const char optid[] = "idea";

void c2(register complex *a)
{
  register real t1, t2, t3, t4;

  t1 = a[0].re + a[1].re;
  t2 = a[0].im + a[1].im;
  t3 = a[0].re - a[1].re;
  t4 = a[0].im - a[1].im;

  a[0].re = t1;
  a[0].im = t2;
  a[1].re = t3;
  a[1].im = t4;
}

void c4(register complex *a)
{
  register real t1, t2, t3, t4;

  TRANSFORMZERO(a[0],a[1],a[2],a[3]);
  c2(a);
}

void c8(register complex *a)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;

  TRANSFORMZERO(a[0],a[2],a[4],a[6]);
  TRANSFORMHALF(a[1],a[3],a[5],a[7]);
  c4(a);
  c2(a + 4);
  c2(a + 6);
}

/* a[0...8n-1], w[0...2n-2] */
void cpass(register complex *a,register const complex *w,register unsigned int n)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;
  register unsigned int k;

  TRANSFORMZERO(a[0],a[2 * n],a[4 * n],a[6 * n]);

  k = 2 * n - 1;

  do {
    TRANSFORM(a[1],a[2 * n + 1],a[4 * n + 1],a[6 * n + 1],w[0].re,w[0].im);
    ++w;
    ++a;
  } while (--k);
}

void c16(register complex *a)
{
  cpass(a,d16,2);
  c8(a);
  c4(a + 8);
  c4(a + 12);
}

void c32(register complex *a)
{
  cpass(a,d32,4);
  c16(a);
  c8(a + 16);
  c8(a + 24);
}

void c64(register complex *a)
{
  cpass(a,d64,8);
  c32(a);
  c16(a + 32);
  c16(a + 48);
}

void c128(register complex *a)
{
  cpass(a,d128,16);
  c64(a);
  c32(a + 64);
  c32(a + 96);
}

void c256(register complex *a)
{
  cpass(a,d256,32);
  c128(a);
  c64(a + 128);
  c64(a + 192);
}

void c512(register complex *a)
{
  cpass(a,d512,64);
  c256(a);
  c128(a + 256);
  c128(a + 384);
}
