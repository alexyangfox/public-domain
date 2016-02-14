#include "PRE"

/* a[0...8n-1], w[0...n-1]; n even, n >= 8 */
void rpassbig(register real *a,register const complex *w,register unsigned int n)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;
  register real *b;
  register unsigned int k;

  b = a + 4 * n;

  RZERO(a[0],a[1],b[0],b[1]);
  R(a[2],a[3],b[2],b[3],w[0].re,w[0].im);

  k = n - 2;
  do {
    R(a[4],a[5],b[4],b[5],w[1].re,w[1].im);
    R(a[6],a[7],b[6],b[7],w[2].re,w[2].im);
    a += 4;
    b += 4;
    w += 2;
  } while (k -= 2);

  RHALF(a[4],a[5],b[4],b[5]);
  R(a[6],a[7],b[6],b[7],w[0].im,w[0].re);

  k = n - 2;
  do {
    R(a[8],a[9],b[8],b[9],w[-1].im,w[-1].re);
    R(a[10],a[11],b[10],b[11],w[-2].im,w[-2].re);
    a += 4;
    b += 4;
    w -= 2;
  } while (k -= 2);
}
