#include "PRE"

/* n > 0 */
void scalec(register complex *a,register unsigned int n)
{
  register real u = 1.0 / n;

  do {
    a[0].re *= u;
    a[0].im *= u;
    ++a;
  } while (--n);
}

void scalec2(complex *a) { scalec(a,2); }
void scalec4(complex *a) { scalec(a,4); }
void scalec8(complex *a) { scalec(a,8); }
void scalec16(complex *a) { scalec(a,16); }
void scalec32(complex *a) { scalec(a,32); }
void scalec64(complex *a) { scalec(a,64); }
void scalec128(complex *a) { scalec(a,128); }
void scalec256(complex *a) { scalec(a,256); }
void scalec512(complex *a) { scalec(a,512); }
void scalec1024(complex *a) { scalec(a,1024); }
void scalec2048(complex *a) { scalec(a,2048); }
void scalec4096(complex *a) { scalec(a,4096); }
void scalec8192(complex *a) { scalec(a,8192); }
