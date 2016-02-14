#include "PRE"

/* a[0...8n-1], w[0...n-2]; n even, n >= 4 */
void upassbig(register complex *a,register const complex *w,register unsigned int n)
{
  register real t1, t2, t3, t4, t5, t6, t7, t8;
  register complex *b;
  register unsigned int k;
  real buf[17];
  complex *tmp;

  tmp = (complex *) ((((int) buf) & 4) + (char *) buf);

  b = a + 4 * n;
  k = n - 2;
  n <<= 3;

  UNTRANSFORMZERO(A0[0],A1[0],A2[0],A3[0]);
  UNTRANSFORM(A0[1],A1[1],A2[1],A3[1],w[0].re,w[0].im);
  a += 2;
  b += 2;

  do {
    tmp[0].re=A0[0].re;tmp[0].im=A0[0].im;tmp[1].re=A0[1].re;tmp[1].im=A0[1].im;
    tmp[2].re=A1[0].re;tmp[2].im=A1[0].im;tmp[3].re=A1[1].re;tmp[3].im=A1[1].im;
    tmp[4].re=A2[0].re;tmp[4].im=A2[0].im;tmp[5].re=A2[1].re;tmp[5].im=A2[1].im;
    tmp[6].re=A3[0].re;tmp[6].im=A3[0].im;tmp[7].re=A3[1].re;tmp[7].im=A3[1].im;
    UNTRANSFORM(tmp[0],tmp[2],tmp[4],tmp[6],w[1].re,w[1].im);
    UNTRANSFORM(tmp[1],tmp[3],tmp[5],tmp[7],w[2].re,w[2].im);
    A0[0].re=tmp[0].re;A0[0].im=tmp[0].im;A0[1].re=tmp[1].re;A0[1].im=tmp[1].im;
    A1[0].re=tmp[2].re;A1[0].im=tmp[2].im;A1[1].re=tmp[3].re;A1[1].im=tmp[3].im;
    A2[0].re=tmp[4].re;A2[0].im=tmp[4].im;A2[1].re=tmp[5].re;A2[1].im=tmp[5].im;
    A3[0].re=tmp[6].re;A3[0].im=tmp[6].im;A3[1].re=tmp[7].re;A3[1].im=tmp[7].im;
    a += 2;
    b += 2;
    w += 2;
  } while (k -= 2);

  UNTRANSFORMHALF(A0[0],A1[0],A2[0],A3[0]);
  UNTRANSFORM(A0[1],A1[1],A2[1],A3[1],w[0].im,w[0].re);
  a += 2;
  b += 2;

  k = (n >> 3) - 2;
  do {
    tmp[0].re=A0[0].re;tmp[0].im=A0[0].im;tmp[1].re=A0[1].re;tmp[1].im=A0[1].im;
    tmp[2].re=A1[0].re;tmp[2].im=A1[0].im;tmp[3].re=A1[1].re;tmp[3].im=A1[1].im;
    tmp[4].re=A2[0].re;tmp[4].im=A2[0].im;tmp[5].re=A2[1].re;tmp[5].im=A2[1].im;
    tmp[6].re=A3[0].re;tmp[6].im=A3[0].im;tmp[7].re=A3[1].re;tmp[7].im=A3[1].im;
    UNTRANSFORM(tmp[0],tmp[2],tmp[4],tmp[6],w[-1].im,w[-1].re);
    UNTRANSFORM(tmp[1],tmp[3],tmp[5],tmp[7],w[-2].im,w[-2].re);
    A0[0].re=tmp[0].re;A0[0].im=tmp[0].im;A0[1].re=tmp[1].re;A0[1].im=tmp[1].im;
    A1[0].re=tmp[2].re;A1[0].im=tmp[2].im;A1[1].re=tmp[3].re;A1[1].im=tmp[3].im;
    A2[0].re=tmp[4].re;A2[0].im=tmp[4].im;A2[1].re=tmp[5].re;A2[1].im=tmp[5].im;
    A3[0].re=tmp[6].re;A3[0].im=tmp[6].im;A3[1].re=tmp[7].re;A3[1].im=tmp[7].im;
    a += 2;
    b += 2;
    w -= 2;
  } while (k -= 2);
}
