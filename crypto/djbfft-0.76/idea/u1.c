#include "PRE"

/* a[0...8n-1], w[0...n-2] */
void upassbig(register complex *a,register const complex *w,register unsigned int n)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;
  register unsigned int k;

  UNTRANSFORMZERO(a[0],a[2 * n],a[4 * n],a[6 * n]);

  k = n - 1;
  do {
    UNTRANSFORM(a[1],a[2 * n + 1],a[4 * n + 1],a[6 * n + 1],w[0].re,w[0].im);
    ++w;
    ++a;
  } while (--k);

  UNTRANSFORMHALF(a[1],a[2 * n + 1],a[4 * n + 1],a[6 * n + 1]);
  --w;
  ++a;

  k = n - 1;
  do {
    UNTRANSFORM(a[1],a[2 * n + 1],a[4 * n + 1],a[6 * n + 1],w[0].im,w[0].re);
    --w;
    ++a;
  } while (--k);
}
