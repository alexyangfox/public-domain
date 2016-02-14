#include "PRE"

void v4(register real *a)
{
  register real t1, t2, t3, t4;

  v2(a);
  VZERO(a[0],a[1],a[2],a[3]);
}

/* a[0...8n-1], w[0...2n-1] */
void vpass(register real *a,register const complex *w,register unsigned int n)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;
  register unsigned int k;

  VZERO(a[0],a[1],a[4 * n],a[4 * n + 1]);
  a += 2;

  k = 2 * n - 1;
  do {
    V(a[0],a[1],a[4 * n],a[4 * n + 1],w[0].re,w[0].im);
    a += 2;
    ++w;
  } while (--k);
}

void v8(register real *a)
{
  u2((complex *)(a + 4));
  v4(a);
  vpass(a,d16 + 1,1);
}

void v16(register real *a)
{
  u4((complex *)(a + 8));
  v8(a);
  vpass(a,d16,2);
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
