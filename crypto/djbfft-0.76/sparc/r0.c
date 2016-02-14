#include "PRE"

#define R(a0,a1,b0,b1,wre,wim) { \
  t0 = a0; \
  t1 = a1; \
  t2 = b0; \
  t3 = b1; \
  t4 = t0 - t1; \
  t5 = wre; \
  t6 = t0 + t1; \
  t7 = wim; \
  t8 = t2 - t3; \
  t9 = t2 + t3; \
  t10 = t4 * t5; \
  a0 = t6; \
  t11 = t4 * t7; \
  a1 = t9; \
  t12 = t8 * t5; \
  t13 = t8 * t7; \
  t14 = t11 + t12; \
  b1 = t14; \
  t15 = t10 - t13; \
  b0 = t15; \
  }

#define RHALF(a0,a1,b0,b1) { \
  t0 = a0; \
  t1 = a1; \
  t2 = b0; \
  t3 = b1; \
  t4 = sqrthalf; \
  t5 = t0 - t1; \
  t6 = t0 + t1; \
  a0 = t6; \
  t7 = t2 - t3; \
  t8 = t2 + t3; \
  a1 = t8; \
  t9 = t5 - t7; \
  t10 = t5 + t7; \
  t11 = t9 * t4; \
  b0 = t11; \
  t12 = t10 * t4; \
  b1 = t12; \
  }

#define RZERO(a0,a1,b0,b1) { \
  t0 = a0; \
  t1 = a1; \
  t2 = b0; \
  t3 = b1; \
  t4 = t0 - t1; \
  b0 = t4; \
  t5 = t0 + t1; \
  a0 = t5; \
  t6 = t2 - t3; \
  b1 = t6; \
  t7 = t2 + t3; \
  a1 = t7; \
  }

void r2(register real *a)
{
  register real t0, t1, t2, t3;

  t0 = a[0];
  t1 = a[1];
  t2 = t0 + t1;
  t3 = t0 - t1;
  a[0] = t2;
  a[1] = t3;
}

void r4(register real *a)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;

  t0 = a[0];
  t1 = a[1];
  t2 = a[2];
  t3 = a[3];
  t4 = t0 + t1;
  t5 = t0 - t1;
  a[2] = t5;
  t6 = t2 + t3;
  t7 = t2 - t3;
  a[3] = t7;

  t8 = t4 - t6;
  a[1] = t8;
  t9 = t4 + t6;
  a[0] = t9;
}

void r8(register real *a)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;

  t0 = a[0];
  t1 = a[1];
  t2 = a[2];
  t3 = a[3];
  t4 = t0 - t1;
  t5 = a[6];
  t6 = t0 + t1;
  t7 = a[7];
  t8 = t2 - t3;
  t9 = a[4];
  t10 = t2 + t3;
  t11 = a[5];
  t12 = t5 - t7;
  t13 = d16[1].re;
  t14 = t5 + t7;
  t15 = t9 + t11;
  t0 = t9 - t11;
  t1 = t8 - t12;
  t2 = t8 + t12;
  t3 = t6 + t15;
  t5 = t10 + t14;
  t1 = t1 * t13;
  t7 = t10 - t14;
  a[3] = t7;
  t2 = t2 * t13;
  t9 = t6 - t15;
  a[2] = t9;
  t11 = t4 + t1;
  a[4] = t11;
  t8 = t4 - t1;
  a[6] = t8;
  t12 = t0 + t2;
  a[5] = t12;
  t10 = t0 - t2;
  a[7] = t10;
  t14 = t3 - t5;
  a[1] = t14;
  t7 = t3 + t5;
  a[0] = t7;
}

void r16(register real *a)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;

  RZERO(a[0],a[1],a[8],a[9]);
  RHALF(a[4],a[5],a[12],a[13]);
  R(a[2],a[3],a[10],a[11],d16[0].re,d16[0].im);
  R(a[6],a[7],a[14],a[15],d16[0].im,d16[0].re);
  r8(a);
  c4((complex *)(a + 8));
}

/* a[0...8n-1], w[0...n-1] */
void rpass(register real *a,register const complex *w,register unsigned int n)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;
  register unsigned int k;

  RZERO(a[0],a[1],a[4 * n],a[4 * n + 1]);
  a += 2;

  k = n - 1;
  do {
    R(a[0],a[1],a[4 * n],a[4 * n + 1],w[0].re,w[0].im);
    a += 2;
    ++w;
  } while (--k);

  RHALF(a[0],a[1],a[4 * n],a[4 * n + 1]);
  a += 2;
  --w;

  k = n - 1;
  do {
    R(a[0],a[1],a[4 * n],a[4 * n + 1],w[0].im,w[0].re);
    a += 2;
    --w;
  } while (--k);
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
