#include "PRE"

void scalec2(complex *a)
{
  register real t0, t1, t2, t3;
  register real u;

  u = 0.5;
  t0 = a[0].re;
  t1 = a[0].im;
  t2 = a[1].re;
  t3 = a[1].im;
  t0 *= u; a[0].re = t0;
  t1 *= u; a[0].im = t1;
  t2 *= u; a[1].re = t2;
  t3 *= u; a[1].im = t3;
}

void scalec4(complex *a)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real u;

  u = 0.25;
  t0 = a[0].re;
  t1 = a[0].im;
  t2 = a[1].re;
  t3 = a[1].im;
  t4 = a[2].re;
  t5 = a[2].im;
  t6 = a[3].re;
  t7 = a[3].im;
  t0 *= u; a[0].re = t0;
  t1 *= u; a[0].im = t1;
  t2 *= u; a[1].re = t2;
  t3 *= u; a[1].im = t3;
  t4 *= u; a[2].re = t4;
  t5 *= u; a[2].im = t5;
  t6 *= u; a[3].re = t6;
  t7 *= u; a[3].im = t7;
}

/* n multiple of 8, n > 0 */
void scalec(register complex *a,register unsigned int n,register real u)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13;

  for (;;) {
    t0 = a[0].re;
    t1 = a[0].im;
    t2 = a[1].re;
    t3 = a[1].im;
    t4 = a[2].re;
    t5 = a[2].im;
    t6 = a[3].re;
    t7 = a[3].im;
    t8 = a[4].re;
    t9 = a[4].im;
    t10 = a[5].re;
    t11 = a[5].im; t0 *= u;
    t12 = a[6].re; t1 *= u;
    t13 = a[6].im; t2 *= u;
    a[0].re = t0; t3 *= u;
    a[0].im = t1; t4 *= u;
    t0 = a[7].re; t5 *= u;
    t1 = a[7].im; t6 *= u;
    a[1].re = t2; t7 *= u;
    a[1].im = t3; t8 *= u;
    a[2].re = t4; t9 *= u;
    a[2].im = t5; t10 *= u;
    a[3].re = t6; t11 *= u;
    a[3].im = t7; t12 *= u;
    a[4].re = t8; t13 *= u;
    a[4].im = t9; n -= 8;
    a[5].re = t10; a += 8;
    a[-3].im = t11; t0 *= u;
    a[-2].re = t12; t1 *= u;
    a[-2].im = t13;
    a[-1].re = t0;
    a[-1].im = t1;

    if (!n) break;
  }
}

void scalec8(complex *a) { scalec(a,8,0.125); }
void scalec16(complex *a) { scalec(a,16,0.0625); }
void scalec32(complex *a) { scalec(a,32,0.03125); }
void scalec64(complex *a) { scalec(a,64,0.015625); }
void scalec128(complex *a) { scalec(a,128,0.0078125); }
void scalec256(complex *a) { scalec(a,256,0.00390625); }
void scalec512(complex *a) { scalec(a,512,0.001953125); }
void scalec1024(complex *a) { scalec(a,1024,0.0009765625); }
void scalec2048(complex *a) { scalec(a,2048,0.00048828125); }
void scalec4096(complex *a) { scalec(a,4096,0.000244140625); }
void scalec8192(complex *a) { scalec(a,8192,0.0001220703125); }
