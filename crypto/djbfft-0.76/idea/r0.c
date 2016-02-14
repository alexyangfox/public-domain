#include "PRE"

void r2(register real *a)
{
  register real t1, t2;

  t1 = a[0] + a[1];
  t2 = a[0] - a[1];
  a[0] = t1;
  a[1] = t2;
}

void r4(register real *a)
{
  register real t1, t2, t3, t4;

  RZERO(a[0],a[1],a[2],a[3]);
  r2(a);
}

/* a[0...8n-1], w[0...2n-1] */
void rpass(register real *a,register const complex *w,register unsigned int n)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;
  register unsigned int k;

  RZERO(a[0],a[1],a[4 * n],a[4 * n + 1]);
  a += 2;

  k = 2 * n - 1;
  do {
    R(a[0],a[1],a[4 * n],a[4 * n + 1],w[0].re,w[0].im);
    a += 2;
    ++w;
  } while (--k);
}

void r8(register real *a)
{
  rpass(a,d16 + 1,1);
  r4(a);
  c2((complex *)(a + 4));
}

void r16(register real *a)
{
  rpass(a,d16,2);
  r8(a);
  c4((complex *)(a + 8));
}

void r32(register real *a)
{
  rpass(a,d32,4);
  r16(a);
  c8((complex *)(a + 16));
}

void r64(register real *a)
{
  rpass(a,d64,8);
  r32(a);
  c16((complex *)(a + 32));
}

void r128(register real *a)
{
  rpass(a,d128,16);
  r64(a);
  c32((complex *)(a + 64));
}

void r256(register real *a)
{
  rpass(a,d256,32);
  r128(a);
  c64((complex *)(a + 128));
}

void r512(register real *a)
{
  rpass(a,d512,64);
  r256(a);
  c128((complex *)(a + 256));
}
