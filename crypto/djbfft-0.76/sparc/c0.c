#include "PRE"

const char optid[] = "sparc";

void c2(register complex *a)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;

  t0 = a[0].re;
  t1 = a[1].re;
  t2 = a[0].im;
  t3 = a[1].im;
  t4 = t0 + t1;
  a[0].re = t4;
  t5 = t0 - t1;
  a[1].re = t5;
  t6 = t2 + t3;
  a[0].im = t6;
  t7 = t2 - t3;
  a[1].im = t7;
}

inline void c4(register complex *a)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;

  t0 = a[0].re;
  t1 = a[2].re;
  t2 = a[1].im;
  t3 = a[3].im;
  t4 = t0 - t1;
  t5 = a[0].im;
  t6 = t0 + t1;
  t7 = a[2].im;
  t8 = t2 - t3;
  t9 = a[1].re;
  t10 = t2 + t3;
  t11 = a[3].re;
  t12 = t5 - t7;
  t13 = t5 + t7;
  t14 = t9 - t11;
  t15 = t9 + t11;
  t0 = t4 - t8;
  a[2].re = t0;
  t1 = t4 + t8;
  a[3].re = t1;
  t2 = t12 + t14;
  a[2].im = t2;
  t3 = t12 - t14;
  a[3].im = t3;
  t5 = t6 + t15;
  a[0].re = t5;
  t7 = t6 - t15;
  a[1].re = t7;
  t9 = t13 + t10;
  a[0].im = t9;
  t11 = t13 - t10;
  a[1].im = t11;
}

void c8(register complex *a)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;

  t0 = a[1].im;
  t1 = a[5].im;
  t2 = a[3].im;
  t3 = a[7].im;
  t4 = t0 + t1;
  t5 = t0 - t1;
  t6 = a[0].re;
  t7 = t2 + t3;
  t8 = a[4].re;
  t9 = t2 - t3;
  t10 = a[2].re;
  t11 = t6 + t8;
  t12 = a[6].re;
  t13 = t4 - t7;
  t14 = a[1].re;
  t15 = t4 + t7;
  t0 = a[5].re;
  t1 = t10 + t12;
  t2 = a[3].re;
  t3 = t6 - t8;
  t4 = a[7].re;
  t7 = t10 - t12;
  t6 = t11 - t1;
  t8 = t11 + t1;
  t10 = t14 - t0;
  t12 = a[2].im;
  t1 = t14 + t0;
  t11 = a[6].im;
  t0 = t6 - t13;
  a[2].re = t0;
  t14 = t6 + t13;
  a[3].re = t14;
  t0 = t10 - t9;
  t6 = t10 + t9;
  t13 = t2 - t4;
  t14 = a[0].im;
  t9 = t2 + t4;
  t10 = a[4].im;
  t2 = t12 - t11;
  t4 = sqrthalf;
  t11 = t12 + t11;
  t12 = t5 + t13;
  t5 = t5 - t13;
  t13 = t14 - t10;
  t10 = t14 + t10;
  t14 = t0 - t12;
  t0 = t12 + t0;
  t12 = t6 + t5;
  t5 = t5 - t6;
  t14 = t14 * t4;
  t6 = t10 - t11;
  t0 = t0 * t4;
  t10 = t10 + t11;
  t12 = t12 * t4;
  t11 = t1 - t9;
  t5 = t5 * t4;
  t4 = t1 + t9;
  t1 = t3 - t2;
  t9 = t3 + t2;
  t2 = t13 + t7;
  t3 = t13 - t7;
  t7 = t6 + t11;
  a[2].im = t7;
  t13 = t6 - t11;
  a[3].im = t13;
  t7 = t8 + t4;
  a[0].re = t7;
  t6 = t8 - t4;
  a[1].re = t6;
  t11 = t10 + t15;
  a[0].im = t11;
  t13 = t10 - t15;
  a[1].im = t13;
  t7 = t1 + t14;
  a[4].re = t7;
  t6 = t1 - t14;
  a[5].re = t6;
  t11 = t2 + t0;
  a[4].im = t11;
  t13 = t2 - t0;
  a[5].im = t13;
  t4 = t9 + t12;
  a[6].re = t4;
  t8 = t9 - t12;
  a[7].re = t8;
  t10 = t3 + t5;
  a[6].im = t10;
  t15 = t3 - t5;
  a[7].im = t15;
}

void c16(register complex *a)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;

  t0 = a[0].re;
  t1 = a[8].re;
  t2 = a[4].im;
  t3 = a[12].im;
  t4 = t0 - t1;
  t5 = a[0].im;
  t6 = t0 + t1;
  t7 = a[8].im;
  t8 = t2 - t3;
  t9 = a[4].re;
  t10 = t2 + t3;
  t11 = a[12].re;
  t12 = t5 - t7;
  a[0].re = t6;
  t13 = t5 + t7;
  a[0].im = t13;
  t14 = t9 - t11;
  a[4].im = t10;
  t15 = t9 + t11;
  a[4].re = t15;
  t0 = t4 - t8;
  a[8].re = t0;
  t1 = t4 + t8;
  a[12].re = t1;
  t2 = t12 + t14;
  a[8].im = t2;
  t3 = t12 - t14;
  a[12].im = t3;
  t6 = a[2].re;
  t5 = a[10].re;
  t7 = a[6].im;
  t13 = a[14].im;
  t10 = t6 - t5;
  t9 = a[2].im;
  t11 = t6 + t5;
  t15 = a[10].im;
  t0 = t7 - t13;
  t4 = a[6].re;
  t8 = t7 + t13;
  t1 = a[14].re;
  t2 = t9 - t15;
  t12 = sqrthalf;
  t14 = t9 + t15;
  a[2].re = t11;
  t3 = t4 - t1;
  a[2].im = t14;
  t5 = t4 + t1;
  a[6].re = t5;
  t6 = t10 - t0;
  a[6].im = t8;
  t7 = t10 + t0;
  t13 = t2 + t3;
  t9 = a[1].re;
  t15 = t2 - t3;
  t11 = a[9].re;
  t14 = a[5].im;
  t1 = a[13].im;
  t4 = t6 - t13;
  t5 = t13 + t6;
  t8 = a[1].im;
  t0 = t7 + t15;
  t10 = a[9].im;
  t2 = t15 - t7;
  t3 = a[5].re;
  t4 = t4 * t12;
  a[10].re = t4;
  t5 = t5 * t12;
  t6 = t9 - t11;
  a[10].im = t5;
  t0 = t0 * t12;
  t13 = t9 + t11;
  a[14].re = t0;
  t2 = t2 * t12;
  t7 = t14 - t1;
  a[14].im = t2;
  t15 = t14 + t1;
  t4 = a[13].re;
  t5 = t8 - t10;
  t9 = d16[0].re;
  t11 = t6 - t7;
  t0 = d16[0].im;
  t12 = t3 - t4;
  t2 = t6 + t7;
  a[1].re = t13;
  t1 = t8 + t10;
  t14 = t11 * t9;
  a[5].im = t15;
  t6 = t5 + t12;
  t7 = t11 * t0;
  a[1].im = t1;
  t13 = t5 - t12;
  t8 = a[3].re;
  t10 = t2 * t9;
  t15 = a[11].re;
  t11 = t3 + t4;
  t1 = t2 * t0;
  a[5].re = t11;
  t5 = t6 * t0;
  t12 = a[7].im;
  t3 = t8 - t15;
  t4 = t6 * t9;
  t2 = t8 + t15;
  t11 = a[15].im;
  t6 = t13 * t0;
  t8 = t13 * t9;
  t15 = t14 - t5;
  a[9].re = t15;
  t0 = a[3].im;
  t9 = t12 - t11;
  t13 = t4 + t7;
  t5 = a[11].im;
  t14 = t12 + t11;
  t15 = a[7].re;
  t4 = t3 - t9;
  t7 = a[15].re;
  t11 = t0 - t5;
  a[9].im = t13;
  t12 = d16[0].im;
  t13 = t15 - t7;
  t3 = t3 + t9;
  t9 = d16[0].re;
  t6 = t10 + t6;
  a[13].re = t6;
  t10 = t4 * t12;
  a[3].re = t2;
  t6 = t11 + t13;
  t2 = t4 * t9;
  t4 = t8 - t1;
  a[13].im = t4;
  t1 = t3 * t12;
  t8 = t11 - t13;
  t4 = t3 * t9;
  t11 = t0 + t5;
  t13 = t6 * t9;
  a[3].im = t11;
  t3 = t15 + t7;
  t0 = t6 * t12;
  a[7].re = t3;
  t5 = t8 * t9;
  a[7].im = t14;
  t11 = t8 * t12;
  t7 = t10 - t13;
  a[11].re = t7;
  t15 = t0 + t2;
  a[11].im = t15;
  t6 = t1 + t5;
  a[15].re = t6;
  t3 = t11 - t4;
  a[15].im = t3;

  c8(a);
  c4(a + 8);
  c4(a + 12);
}

void c32(register complex *a)
{
  cpass(a,d32,d16,4);
  c8(a);
  c4(a + 8);
  c4(a + 12);
  c8(a + 16);
  c8(a + 24);
}

void c64(register complex *a)
{
  cpass(a,d64,d32,8);
  c16(a);
  c8(a + 16);
  c8(a + 24);
  c16(a + 32);
  c16(a + 48);
}

void c128(register complex *a)
{
  cpass(a,d128,d64,16);
  c32(a);
  c16(a + 32);
  c16(a + 48);
  c32(a + 64);
  c32(a + 96);
}

void c256(register complex *a)
{
  cpass(a,d256,d128,32);
  c64(a);
  c32(a + 64);
  c32(a + 96);
  c64(a + 128);
  c64(a + 192);
}

void c512(register complex *a)
{
  cpass(a,d512,d256,64);
  c128(a);
  c64(a + 128);
  c64(a + 192);
  c128(a + 256);
  c128(a + 384);
}
