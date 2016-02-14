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
  register real t1, t2, t3, t4, t5, t6;

  t3 = a[0] + a[1];
  t4 = a[2] + a[3];
  t1 = a[0] - a[1];
  t2 = a[2] - a[3];
  t6 = t3 - t4;
  t3 += t4;
  a[2] = t1;
  a[3] = t2;
  a[0] = t3;
  a[1] = t6;
}

void r8(register real *a)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;

  t2 = a[0] + a[1];
  t8 = a[4] + a[5];
  t3 = a[2] - a[3];
  t6 = t2 - t8;
  t2 += t8;
  t1 = a[2] + a[3];
  t7 = a[6] + a[7];
  a[2] = t6;
  t5 = t1 - t7;
  t1 += t7;
  t4 = a[0] - a[1];
  a[3] = t5;
  t8 = t2 - t1;
  t2 += t1;
  t7 = a[6] - a[7];
  a[1] = t8;
  t6 = t3 - t7;
  t3 += t7;
  a[0] = t2;
  t6 *= sqrthalf;
  t8 = a[4] - a[5];
  t3 *= sqrthalf;
  t1 = t4 - t6;
  t4 += t6;
  t2 = t8 - t3;
  t8 += t3;
  a[6] = t1;
  a[4] = t4;
  a[7] = t2;
  a[5] = t8;
}

/* a[0...8n-1], w[0...2n-1]; n even, n >= 4 */
void rpass(register real *a,register const complex *w,register unsigned int n)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;
  register real *b;
  register unsigned int k;

  b = a + 4 * n;
  k = n - 2;

  RZERO(a[0],a[1],b[0],b[1]);
  R(a[2],a[3],b[2],b[3],w[0].re,w[0].im);
  R(a[4],a[5],b[4],b[5],w[1].re,w[1].im);
  R(a[6],a[7],b[6],b[7],w[2].re,w[2].im);

  for (;;) {
    R(a[8],a[9],b[8],b[9],w[3].re,w[3].im);
    R(a[10],a[11],b[10],b[11],w[4].re,w[4].im);
    R(a[12],a[13],b[12],b[13],w[5].re,w[5].im);
    R(a[14],a[15],b[14],b[15],w[6].re,w[6].im);
    if (!(k -= 2)) break;
    a += 8;
    b += 8;
    w += 4;
  }
}

void r16(register real *a)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;

  RZERO(a[0],a[1],a[8],a[9]);
  R(a[2],a[3],a[10],a[11],d16[0].re,d16[0].im);
  R(a[4],a[5],a[12],a[13],d16[1].re,d16[1].im);
  R(a[6],a[7],a[14],a[15],d16[2].re,d16[2].im);
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
