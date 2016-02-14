#include "PRE"

void mulc2(complex *a,complex *b)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;

  t0 = a[0].re;
  t1 = b[0].re;
  t2 = b[0].im;
  t3 = a[0].im;
  t4 = t0 * t1;
  t5 = a[1].re;
  t6 = t0 * t2;
  t7 = b[1].re;
  t8 = t3 * t2;
  t9 = b[1].im;
  t10 = t3 * t1;
  t11 = a[1].im;
  t12 = t5 * t7;
  t13 = t5 * t9;
  t14 = t4 - t8;
  a[0].re = t14;
  t15 = t11 * t9;
  t0 = t6 + t10;
  a[0].im = t0;
  t2 = t11 * t7;
  
  t1 = t12 - t15;
  a[1].re = t1;
  t3 = t13 + t2;
  a[1].im = t3;
}

/* n multiple of 4, n > 0 */
void mulc(register complex *a,register complex *b,register unsigned int n)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;

  do {
    t0 = a[0].re;
    t1 = b[0].re;
    t2 = b[0].im;
    t3 = a[0].im;
    t4 = t0 * t1;
    t5 = a[1].re;
    t6 = t0 * t2;
    t7 = b[1].re;
    t8 = t3 * t2;
    t9 = b[1].im;
    t10 = t3 * t1;
    t11 = a[1].im;
    t12 = t5 * t7;
    t13 = a[2].re;
    t14 = t5 * t9;
    t15 = t4 - t8;
    t0 = b[2].re;
    t2 = t11 * t9;
    t1 = t6 + t10;
    t3 = b[2].im;
    t5 = t11 * t7;
    t4 = a[2].im;
    t8 = t13 * t0;
    a[0].re = t15;
    t9 = t13 * t3;
    t6 = t12 - t2;
    a[0].im = t1;
    t10 = t4 * t3;
    t7 = t14 + t5;
    t11 = a[3].re;
    t15 = t4 * t0;
    t13 = b[3].re;
    t2 = b[3].im;
    t12 = t8 - t10;
    t1 = a[3].im;
    t3 = t11 * t13;
    t5 = t9 + t15;
    a[1].re = t6; b += 4;
    t14 = t11 * t2;
    a[1].im = t7;
    t0 = t1 * t2;
    a[2].re = t12;
    t4 = t1 * t13;
    a[2].im = t5; a += 4;
    t8 = t3 - t0;
    a[-1].re = t8;
    t10 = t14 + t4;
    a[-1].im = t10;
  } while (n -= 4);
}

void mulc4(complex *a,complex *b) { mulc(a,b,4); }
void mulc8(complex *a,complex *b) { mulc(a,b,8); }
void mulc16(complex *a,complex *b) { mulc(a,b,16); }
void mulc32(complex *a,complex *b) { mulc(a,b,32); }
void mulc64(complex *a,complex *b) { mulc(a,b,64); }
void mulc128(complex *a,complex *b) { mulc(a,b,128); }
void mulc256(complex *a,complex *b) { mulc(a,b,256); }
void mulc512(complex *a,complex *b) { mulc(a,b,512); }
void mulc1024(complex *a,complex *b) { mulc(a,b,1024); }
void mulc2048(complex *a,complex *b) { mulc(a,b,2048); }
void mulc4096(complex *a,complex *b) { mulc(a,b,4096); }
void mulc8192(complex *a,complex *b) { mulc(a,b,8192); }
