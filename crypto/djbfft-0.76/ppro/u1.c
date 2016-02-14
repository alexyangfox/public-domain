#include "PRE"

/* a[0...8n-1], w[0...n-2]; n even, n >= 4 */
void upassbig(register complex *a,register const complex *w,register unsigned int n)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;
  register complex *a1;
  register complex *a2;
  register complex *a3;
  register unsigned int k;

  a2 = a + 4 * n;
  a1 = a + 2 * n;
  a3 = a2 + 2 * n;
  k = n - 2;

  UNTRANSFORMZERO(a[0],a1[0],a2[0],a3[0]);
  UNTRANSFORM(a[1],a1[1],a2[1],a3[1],w[0].re,w[0].im);
  a += 2;
  a1 += 2;
  a2 += 2;
  a3 += 2;

  do {
    UNTRANSFORM(a[0],a1[0],a2[0],a3[0],w[1].re,w[1].im);
    UNTRANSFORM(a[1],a1[1],a2[1],a3[1],w[2].re,w[2].im);
    a += 2;
    a1 += 2;
    a2 += 2;
    a3 += 2;
    w += 2;
  } while (k -= 2);

  UNTRANSFORMHALF(a[0],a1[0],a2[0],a3[0]);
  UNTRANSFORM(a[1],a1[1],a2[1],a3[1],w[0].im,w[0].re);
  a += 2;
  a1 += 2;
  a2 += 2;
  a3 += 2;

  k = n - 2;
  do {
    UNTRANSFORM(a[0],a1[0],a2[0],a3[0],w[-1].im,w[-1].re);
    UNTRANSFORM(a[1],a1[1],a2[1],a3[1],w[-2].im,w[-2].re);
    a += 2;
    a1 += 2;
    a2 += 2;
    a3 += 2;
    w -= 2;
  } while (k -= 2);
}
