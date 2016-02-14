#include "PRE"

/* a[0...8n-1], w[0...n-1] */
void rpassbig(register real *a,register const complex *w,register unsigned int n)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;
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
