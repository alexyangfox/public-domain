#include "PRE"

#define TRANSFORMHALF(a0,a1,a2,a3) { \
  t0 = a0.re; \
  t1 = a2.re; \
  t2 = a1.im; \
  t3 = t0 - t1; \
  t4 = a3.im; \
  t0 = t0 + t1; \
  t1 = a0.im; \
  t5 = t2 - t4; \
  t6 = a2.im; \
  t2 = t2 + t4; \
  t4 = a1.re; \
  t7 = t1 - t6; \
  t8 = a3.re; \
  t9 = t3 - t5; \
  t10 = sqrthalf; \
  t11 = t4 - t8; \
  a0.re = t0; \
  t0 = t3 + t5; \
  a1.im = t2; \
  t1 = t1 + t6; \
  a0.im = t1; \
  t1 = t7 + t11; \
  t2 = t4 + t8; \
  a1.re = t2; \
  t2 = t7 - t11; \
  t3 = t9 - t1; \
  t1 = t1 + t9; \
  t4 = t0 + t2; \
  t0 = t2 - t0; \
  t3 = t3 * t10; \
  a2.re = t3; \
  t1 = t1 * t10; \
  a2.im = t1; \
  t4 = t4 * t10; \
  a3.re = t4; \
  t0 = t0 * t10; \
  a3.im = t0; \
  }

#define TRANSFORMZERO(a0,a1,a2,a3) { \
  t0 = a0.re; \
  t1 = a2.re; \
  t2 = a1.im; \
  t3 = a3.im; \
  t4 = t0 - t1; \
  t5 = a0.im; \
  t6 = t0 + t1; \
  t7 = a2.im; \
  t8 = t2 - t3; \
  t9 = a1.re; \
  t10 = t2 + t3; \
  t11 = a3.re; \
  t12 = t5 - t7; \
  a0.re = t6; \
  t13 = t5 + t7; \
  a0.im = t13; \
  t14 = t9 - t11; \
  a1.im = t10; \
  t15 = t9 + t11; \
  a1.re = t15; \
  t0 = t4 - t8; \
  a2.re = t0; \
  t1 = t4 + t8; \
  a3.re = t1; \
  t2 = t12 + t14; \
  a2.im = t2; \
  t3 = t12 - t14; \
  a3.im = t3; \
  }

/*
  TRANSFORM(a[1],a2[1],a4[1],a6[1],wre,wim)
  TRANSFORM(a1[1],a3[1],a5[1],a7[1],wnre,wnim)
  TRANSFORM(a[1],a1[1],a2[1],a3[1],vre,vim)
  increase a, a1, a2, a3, a4, a5, a6, a7
  increase w, decrease wn
*/
#define DOIT(wre,wim,wnre,wnim,vre,vim) { \
  t0 = a[1].re; \
  t1 = a4[1].re; \
  t2 = a2[1].re; \
  t3 = a6[1].re; \
  t4 = t0 + t1; \
  t5 = a2[1].im; \
  t6 = t0 - t1; \
  t7 = a6[1].im; \
  t8 = t2 + t3; \
  t9 = a[1].im; \
  t10 = t2 - t3; \
  t11 = a4[1].im; \
  t12 = t5 + t7; \
  t13 = t5 - t7; \
  t14 = wre; \
  t15 = t9 + t11; \
  t0 = t9 - t11; \
  t1 = wim; \
  t2 = t6 - t13; \
  w += 1; \
  t3 = t6 + t13; \
  t5 = t4 + t8; \
  a[1].re = t5; \
  t7 = t4 - t8; \
  t9 = t2 * t14; \
  t11 = t0 + t10; \
  t6 = t2 * t1; \
  t13 = a1[1].re; \
  t5 = t15 + t12; \
  a[1].im = t5; \
  a += 1; \
  t4 = t3 * t14; \
  t8 = t0 - t10; \
  t2 = t3 * t1; \
  t5 = a5[1].re; \
  t0 = t15 - t12; \
  t10 = t11 * t1; \
  t3 = a3[1].re; \
  t12 = t11 * t14; \
  t15 = t13 + t5; \
  t11 = a7[1].re; \
  t1 = t8 * t1; \
  t5 = t13 - t5; \
  t13 = a3[1].im; \
  t8 = t8 * t14; \
  t14 = t9 - t10; \
  a4[1].re = t14; \
  t9 = t12 + t6; \
  a4[1].im = t9; \
  a4 += 1; \
  t10 = t4 + t1; \
  a6[1].re = t10; \
  t14 = t8 - t2; \
  a6[1].im = t14; \
  a6 += 1; \
  t6 = a7[1].im; \
  t12 = t3 + t11; \
  t9 = a1[1].im; \
  t1 = t3 - t11; \
  t4 = a5[1].im; \
  t10 = t13 - t6; \
  t2 = t13 + t6; \
  t8 = t9 + t4; \
  t14 = t5 - t10; \
  t3 = wnre; \
  t11 = t9 - t4; \
  t6 = wnim; \
  t13 = t15 + t12; \
  a1[1].re = t13; \
  t4 = t5 + t10; \
  wn -= 1; \
  t9 = t14 * t3; \
  t13 = t8 - t2; \
  t5 = t14 * t6; \
  t10 = t11 + t1; \
  t14 = vre; \
  t1 = t11 - t1; \
  t11 = t4 * t3; \
  t12 = t15 - t12; \
  t15 = t4 * t6; \
  t4 = t8 + t2; \
  a1[1].im = t4; \
  t2 = t10 * t6; \
  a1 += 1; \
  t8 = t10 * t3; \
  t4 = t7 - t13; \
  t10 = t1 * t6; \
  t6 = vim; \
  t1 = t1 * t3; \
  a7 += 1; \
  t3 = t7 + t13; \
  t7 = t4 * t14; \
  a5 += 1; \
  t13 = t4 * t6; \
  t4 = t0 + t12; \
  t2 = t9 - t2; \
  a5[0].re = t2; \
  t9 = t3 * t14; \
  a3 += 1; \
  t2 = t3 * t6; \
  t3 = t0 - t12; \
  t0 = t4 * t6; \
  t12 = t8 + t5; \
  a5[0].im = t12; \
  t5 = t4 * t14; \
  t8 = t11 + t10; \
  a7[0].re = t8; \
  t12 = t3 * t6; \
  t4 = t1 - t15; \
  a7[0].im = t4; \
  t10 = t3 * t14; \
  t11 = t7 - t0; \
  a2[1].re = t11; \
  t8 = t5 + t13; \
  a2[1].im = t8; \
  a2 += 1; \
  t6 = t9 + t12; \
  a3[0].re = t6; \
  t1 = t10 - t2; \
  a3[0].im = t1; \
  }

/* a[0...8n-1], w[0...n-2], v[0...n/2-2]; n even, n >= 4 */
void cpass(register complex *a,register const complex *w,register const complex *v,register unsigned int n)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7;
  register real t8, t9, t10, t11, t12, t13, t14, t15;
  register complex *a1;
  register complex *a2;
  register complex *a3;
  register complex *a4;
  register complex *a5;
  register complex *a6;
  register complex *a7;
  register unsigned int k;
  register const complex *wn;

  a4 = a + 4 * n;
  a2 = a + 2 * n;
  a6 = a4 + 2 * n;
  a1 = a + n;
  a3 = a2 + n;
  a5 = a4 + n;
  a7 = a6 + n;
  wn = w + n;

  TRANSFORMZERO(a[0],a2[0],a4[0],a6[0])
  TRANSFORMHALF(a1[0],a3[0],a5[0],a7[0])
  TRANSFORMZERO(a[0],a1[0],a2[0],a3[0])

  k = (n >> 1) - 1;

  do {
    DOIT(w[0].re,w[0].im,wn[-2].im,wn[-2].re,v[0].re,v[0].im)
    v += 1;
  } while (--k);

  DOIT(w[0].re,w[0].im,wn[-2].im,wn[-2].re,sqrthalf,sqrthalf)

  k = (n >> 1) - 1;

  do {
    DOIT(w[0].re,w[0].im,wn[-2].im,wn[-2].re,v[-1].im,v[-1].re)
    v -= 1;
  } while (--k);
}
