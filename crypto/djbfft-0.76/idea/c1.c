#include "PRE"

/* a[0...8n-1], w[0...n-2] */
void cpassbig(register complex *a,register const complex *w,register unsigned int n)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;
  register unsigned int k;

  TRANSFORMZERO(a[0],a[2 * n],a[4 * n],a[6 * n]);

  k = n - 1;
  do {
    TRANSFORM(a[1],a[2 * n + 1],a[4 * n + 1],a[6 * n + 1],w[0].re,w[0].im);
    ++a;
    ++w;
  } while (--k);

  TRANSFORMHALF(a[1],a[2 * n + 1],a[4 * n + 1],a[6 * n + 1]);
  ++a;
  --w;

  k = n - 1;
  do {
    TRANSFORM(a[1],a[2 * n + 1],a[4 * n + 1],a[6 * n + 1],w[0].im,w[0].re);
    ++a;
    --w;
  } while (--k);
}
