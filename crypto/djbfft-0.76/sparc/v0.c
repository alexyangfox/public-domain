#include "PRE"

#define V(a0,a1,b0,b1,wre,wim) { \
  t0 = b0; \
  t1 = wre; \
  t2 = wim; \
  t3 = b1; \
  t4 = t0 * t1; \
  t5 = a0; \
  t6 = t0 * t2; \
  t7 = a1; \
  t8 = t3 * t1; \
  t9 = t3 * t2; \
  t10 = t8 - t6; \
  t11 = t4 + t9; \
  t12 = t7 + t10; \
  b0 = t12; \
  t13 = t7 - t10; \
  b1 = t13; \
  t14 = t5 + t11; \
  a0 = t14; \
  t15 = t5 - t11; \
  a1 = t15; \
  }

#define VHALF(a0,a1,b0,b1) { \
  t0 = b0; \
  t1 = b1; \
  t2 = sqrthalf; \
  t3 = a0; \
  t4 = t0 + t1; \
  t5 = a1; \
  t6 = t1 - t0; \
  t4 = t4 * t2; \
  t6 = t6 * t2; \
  t7 = t3 + t4; \
  a0 = t7; \
  t8 = t3 - t4; \
  a1 = t8; \
  t9 = t5 + t6; \
  b0 = t9; \
  t10 = t5 - t6; \
  b1 = t10; \
  }

#define VZERO(a0,a1,b0,b1) { \
  t0 = a0; \
  t1 = b0; \
  t2 = a1; \
  t3 = b1; \
  t4 = t0 + t1; \
  a0 = t4; \
  t5 = t0 - t1; \
  a1 = t5; \
  t6 = t2 + t3; \
  b0 = t6; \
  t7 = t2 - t3; \
  b1 = t7; \
  }

void v4(register real *a)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;

  v2(a);
  VZERO(a[0],a[1],a[2],a[3]);
}

void v8(register real *a)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;

  u2((complex *)(a + 4));
  v4(a);
  VZERO(a[0],a[1],a[4],a[5]);
  VHALF(a[2],a[3],a[6],a[7]);
}

void v16(register real *a)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;

  u4((complex *)(a + 8));
  v8(a);
  VZERO(a[0],a[1],a[8],a[9]);
  VHALF(a[4],a[5],a[12],a[13]);
  V(a[2],a[3],a[10],a[11],d16[0].re,d16[0].im);
  V(a[6],a[7],a[14],a[15],d16[0].im,d16[0].re);
}

/* a[0...8n-1], w[0...n-1] */
void vpass(register real *a,register const complex *w,register unsigned int n)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;
  register unsigned int k;

  VZERO(a[0],a[1],a[4 * n],a[4 * n + 1]);
  a += 2;

  k = n - 1;
  do {
    V(a[0],a[1],a[4 * n],a[4 * n + 1],w[0].re,w[0].im);
    a += 2;
    ++w;
  } while (--k);

  VHALF(a[0],a[1],a[4 * n],a[4 * n + 1]);
  a += 2;
  --w;

  k = n - 1;
  do {
    V(a[0],a[1],a[4 * n],a[4 * n + 1],w[0].im,w[0].re);
    a += 2;
    --w;
  } while (--k);
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
