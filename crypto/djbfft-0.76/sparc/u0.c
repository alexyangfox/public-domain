#include "PRE"

inline void u4(register complex *a)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;

  t0 = a[0].re;
  t1 = a[1].re;
  t2 = a[0].im;
  t3 = a[1].im;
  t4 = t0 + t1;
  t5 = a[2].re;
  t6 = t0 - t1;
  t7 = a[3].re;
  t8 = t2 + t3;
  t9 = a[2].im;
  t10 = t2 - t3;
  t11 = a[3].im;
  t12 = t5 + t7;
  t13 = t7 - t5;
  t14 = t9 + t11;
  t15 = t9 - t11;
  t0 = t4 + t12;
  a[0].re = t0;
  t1 = t4 - t12;
  a[2].re = t1;
  t2 = t10 + t13;
  a[1].im = t2;
  t3 = t10 - t13;
  a[3].im = t3;
  t5 = t8 + t14;
  a[0].im = t5;
  t7 = t8 - t14;
  a[2].im = t7;
  t9 = t6 + t15;
  a[1].re = t9;
  t11 = t6 - t15;
  a[3].re = t11;
}

void u8(register complex *a)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;

  t0 = a[0].re;
  t1 = a[1].re;
  t2 = a[0].im;
  t3 = a[1].im;
  t4 = t0 + t1;
  t5 = a[2].re;
  t6 = t0 - t1;
  t7 = a[3].re;
  t8 = t2 + t3;
  t9 = a[2].im;
  t10 = t2 - t3;
  t11 = a[3].im;
  t12 = t5 + t7;
  t13 = t7 - t5;
  t14 = t9 + t11;
  t15 = t9 - t11;
  t0 = a[4].re;
  t1 = t4 + t12;
  t2 = a[5].re;
  t3 = t8 - t14;
  t5 = a[6].re;
  t7 = t6 + t15;
  t9 = a[7].re;
  t11 = t6 - t15;
  t6 = t0 + t2;
  t15 = t0 - t2;
  t0 = t5 + t9;
  t2 = t5 - t9;
  t5 = t10 + t13;
  t9 = t6 + t0;
  t0 = t0 - t6;
  t6 = t10 - t13;
  t10 = a[6].im;
  t13 = t1 + t9;
  a[0].re = t13;
  t13 = t1 - t9;
  a[4].re = t13;
  t1 = t3 + t0;
  a[2].im = t1;
  t9 = t3 - t0;
  a[6].im = t9;
  t13 = a[7].im;
  t1 = t4 - t12;
  t0 = a[4].im;
  t3 = t8 + t14;
  t9 = a[5].im;
  t4 = t10 + t13;
  t12 = t10 - t13;
  t8 = t0 + t9;
  t14 = t0 - t9;
  t10 = sqrthalf;
  t13 = t2 - t12;
  t0 = t8 + t4;
  t9 = t8 - t4;
  t4 = t12 + t2;
  t8 = t15 + t14;
  t2 = t14 - t15;
  t12 = t3 + t0;
  a[0].im = t12;
  t13 = t13 * t10;
  t14 = t1 + t9;
  a[2].re = t14;
  t8 = t8 * t10;
  t15 = t3 - t0;
  a[4].im = t15;
  t4 = t4 * t10;
  t12 = t1 - t9;
  a[6].re = t12;
  t2 = t2 * t10;
  t14 = t8 + t13;
  t0 = t13 - t8;
  t3 = t2 + t4;
  t15 = t2 - t4;
  t1 = t7 + t14;
  a[1].re = t1;
  t9 = t5 + t3;
  a[1].im = t9;
  t12 = t11 + t15;
  a[3].re = t12;
  t10 = t6 + t0;
  a[3].im = t10;
  t8 = t7 - t14;
  a[5].re = t8;
  t13 = t5 - t3;
  a[5].im = t13;
  t2 = t11 - t15;
  a[7].re = t2;
  t4 = t6 - t0;
  a[7].im = t4;
}

void u16(register complex *a)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;

  u4(a + 8);
  u4(a + 12);
  u8(a);

  t0 = a[8].re;
  t1 = a[12].re;
  t2 = a[8].im;
  t3 = a[12].im;
  t4 = t0 + t1;
  t5 = a[0].re;
  t6 = t1 - t0;
  t7 = a[4].im;
  t8 = t2 + t3;
  t9 = a[0].im;
  t10 = t2 - t3;
  t11 = a[4].re;
  t12 = t5 + t4;
  a[0].re = t12;
  t13 = t5 - t4;
  a[8].re = t13;
  t14 = t7 - t6;
  a[12].im = t14;
  t15 = t7 + t6;
  a[4].im = t15;
  t0 = a[10].re;
  t1 = a[10].im;
  t2 = a[14].re;
  t3 = a[14].im;
  t12 = t0 + t1;
  t4 = sqrthalf;
  t5 = t1 - t0;
  t13 = a[2].re;
  t14 = t2 - t3;
  t6 = a[2].im;
  t7 = t3 + t2;
  t12 = t12 * t4;
  t15 = t9 + t8;
  a[0].im = t15;
  t5 = t5 * t4;
  t0 = t11 + t10;
  a[4].re = t0;
  t14 = t14 * t4;
  t1 = t9 - t8;
  a[8].im = t1;
  t7 = t7 * t4;
  t2 = t11 - t10;
  a[12].re = t2;
  t3 = a[6].re;
  t15 = t12 + t14;
  t0 = a[6].im;
  t8 = t14 - t12;
  t9 = t5 + t7;
  t1 = a[9].re;
  t4 = t5 - t7;
  t10 = a[9].im;
  t11 = d16[0].re;
  t2 = d16[0].im;
  t12 = t13 + t15;
  a[2].re = t12;
  t14 = t1 * t11;
  t5 = t13 - t15;
  a[10].re = t5;
  t7 = t10 * t2;
  t12 = a[13].re;
  t13 = t1 * t2;
  t15 = a[13].im;
  t5 = t10 * t11;
  t1 = t0 + t8;
  a[6].im = t1;
  t10 = t12 * t11;
  t1 = t0 - t8;
  a[14].im = t1;
  t0 = t15 * t2;
  t8 = t6 + t9;
  a[2].im = t8;
  t1 = t12 * t2;
  t8 = t14 + t7;
  t2 = a[1].re;
  t12 = t6 - t9;
  t7 = a[1].im;
  t14 = t10 - t0;
  a[10].im = t12;
  t6 = t15 * t11;
  t9 = t3 + t4;
  a[6].re = t9;
  t0 = t5 - t13;
  t10 = a[5].re;
  t12 = t8 + t14;
  t11 = a[5].im;
  t15 = t14 - t8;
  t9 = t3 - t4;
  a[14].re = t9;
  t5 = t6 + t1;
  t13 = a[11].re;
  t8 = t2 + t12;
  t14 = a[11].im;
  t3 = t2 - t12;
  t4 = d16[0].im;
  t9 = t11 + t15;
  t1 = d16[0].re;
  a[1].re = t8;
  t6 = t0 + t5;
  t2 = t13 * t4;
  a[9].re = t3;
  t12 = t0 - t5;
  t8 = t14 * t1;
  t3 = t11 - t15;
  t0 = a[15].re;
  t5 = t13 * t1;
  t11 = t7 + t6;
  t15 = a[15].im;
  t13 = t14 * t4;
  a[5].im = t9;
  t14 = t10 + t12;
  t9 = t0 * t4;
  t6 = t7 - t6;
  t7 = t15 * t1;
  a[13].im = t3;
  t3 = t10 - t12;
  t10 = t0 * t1;
  a[1].im = t11;
  t12 = t2 + t8;
  t0 = t15 * t4;
  a[5].re = t14;
  t1 = t9 - t7;
  a[9].im = t6;
  t11 = t13 - t5;
  a[13].re = t3;
  t2 = t0 + t10;
  t8 = a[3].re;
  t4 = t12 + t1;
  t15 = a[7].im;
  t14 = t1 - t12;
  t7 = a[3].im;
  t9 = t11 + t2;
  t6 = a[7].re;
  t5 = t11 - t2;
  t13 = t8 + t4;
  a[3].re = t13;
  t3 = t8 - t4;
  a[11].re = t3;
  t0 = t15 + t14;
  a[7].im = t0;
  t10 = t15 - t14;
  a[15].im = t10;
  t1 = t7 + t9;
  a[3].im = t1;
  t12 = t6 + t5;
  a[7].re = t12;
  t2 = t7 - t9;
  a[11].im = t2;
  t11 = t6 - t5;
  a[15].re = t11;
}

void u32(register complex *a)
{
  u8(a);
  u4(a + 8);
  u4(a + 12);
  u8(a + 16);
  u8(a + 24);
  upass(a,d32,d16,4);
}

void u64(register complex *a)
{
  u16(a);
  u8(a + 16);
  u8(a + 24);
  u16(a + 32);
  u16(a + 48);
  upass(a,d64,d32,8);
}

void u128(register complex *a)
{
  u32(a);
  u16(a + 32);
  u16(a + 48);
  u32(a + 64);
  u32(a + 96);
  upass(a,d128,d64,16);
}

void u256(register complex *a)
{
  u64(a);
  u32(a + 64);
  u32(a + 96);
  u64(a + 128);
  u64(a + 192);
  upass(a,d256,d128,32);
}

void u512(register complex *a)
{
  u128(a);
  u64(a + 128);
  u64(a + 192);
  u128(a + 256);
  u128(a + 384);
  upass(a,d512,d256,64);
}
