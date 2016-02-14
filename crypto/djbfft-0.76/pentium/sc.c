#include "PRE"

/* n even, n > 0 */
void scalec(register complex *a,register unsigned int n,register real u)
{
  register real t1, t2, t3, t4;

  do {
    t1 = a[0].re * u;
    t2 = a[0].im * u;
    t3 = a[1].re * u;
    t4 = a[1].im * u;
    a[0].re = t1;
    a[1].re = t3;
    a[0].im = t2;
    a[1].im = t4;
    a += 2;
  } while (n -= 2);
}

void scalec2(complex *a) { scalec(a,2,0.5); }
void scalec4(complex *a) { scalec(a,4,0.25); }
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
