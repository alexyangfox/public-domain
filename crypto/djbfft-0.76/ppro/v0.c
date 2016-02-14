#include "PRE"

void v4(register real *a)
{
  register real t1, t2, t3, t4, t5, t6;

  t5 = a[0] + a[1];
  t6 = a[0] - a[1];
  t1 = t5 + a[2];
  t5 -= a[2];
  t3 = t6 + a[3];
  t6 -= a[3];
  a[0] = t1;
  a[1] = t5;
  a[2] = t3;
  a[3] = t6;
}

void v8(register real *a)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;

  t5 = a[0] + a[1];
  t2 = a[4] + a[6];
  t8 = t5 + a[2];
  t5 -= a[2];
  t1 = a[0] - a[1];
  t7 = t8 + t2;
  t8 -= t2;
  t3 = a[4] - a[6];
  a[0] = t7;
  t6 = a[5] + a[7];
  a[1] = t8;
  t7 = t5 + t6;
  t5 -= t6;
  t4 = a[5] - a[7];
  a[4] = t7;
  t6 = t4 - t3;
  t3 += t4;
  a[5] = t5;
  t3 *= sqrthalf;
  t2 = t1 + a[3];
  t1 -= a[3];
  t6 *= sqrthalf;
  t7 = t2 - t3;
  t3 += t2;
  t8 = t1 - t6;
  t6 += t1;
  a[3] = t7;
  a[7] = t8;
  a[2] = t3;
  a[6] = t6;
}

/* a[0...8n-1], w[0...2n-1]; n even, n >= 4 */
void vpass(register real *a,register const complex *w,register unsigned int n)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;
  register real *b;
  register unsigned int k;

  b = a + 4 * n;
  k = n - 2;

  VZERO(a[0],a[1],b[0],b[1]);
  V(a[2],a[3],b[2],b[3],w[0].re,w[0].im);
  V(a[4],a[5],b[4],b[5],w[1].re,w[1].im);
  V(a[6],a[7],b[6],b[7],w[2].re,w[2].im);

  for (;;) {
    V(a[8],a[9],b[8],b[9],w[3].re,w[3].im);
    V(a[10],a[11],b[10],b[11],w[4].re,w[4].im);
    V(a[12],a[13],b[12],b[13],w[5].re,w[5].im);
    V(a[14],a[15],b[14],b[15],w[6].re,w[6].im);
    if (!(k -= 2)) break;
    a += 8;
    b += 8;
    w += 4;
  }
}

void v16(register real *a)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;

  u4((complex *)(a + 8));
  v8(a);
  VZERO(a[0],a[1],a[8],a[9]);
  V(a[2],a[3],a[10],a[11],d16[0].re,d16[0].im);
  V(a[4],a[5],a[12],a[13],d16[1].re,d16[1].im);
  V(a[6],a[7],a[14],a[15],d16[2].re,d16[2].im);
}

void v32(register real *a)
{
  u8((complex *)(a + 16));
  v16(a);
  vpass(a,d32,4);
}

void v64(register real *a)
{
  u16((complex *)(a + 32));
  v32(a);
  vpass(a,d64,8);
}

void v128(register real *a)
{
  u32((complex *)(a + 64));
  v64(a);
  vpass(a,d128,16);
}

void v256(register real *a)
{
  u64((complex *)(a + 128));
  v128(a);
  vpass(a,d256,32);
}

void v512(register real *a)
{
  u128((complex *)(a + 256));
  v256(a);
  vpass(a,d512,64);
}
