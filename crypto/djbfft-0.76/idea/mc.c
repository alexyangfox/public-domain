#include "PRE"

/* n > 0 */
void mulc(register complex *a,register complex *b,register unsigned int n)
{
  register real t1, t2, t3, t4;

  do {
    t1 = a[0].re;
    t2 = a[0].im;
    t3 = b[0].re;
    t4 = b[0].im;
    a[0].re = t1 * t3 - t2 * t4;
    a[0].im = t1 * t4 + t2 * t3;
    ++a;
    ++b;
  } while (--n);
}

void mulc2(complex *a,complex *b) { mulc(a,b,2); }
void mulc4(complex *a,complex *b) { mulc(a,b,4); }
void mulc8(complex *a,complex *b) { mulc(a,b,8); }
void mulc16(complex *a,complex *b) { mulc(a,b,16); }
void mulc32(complex *a,complex *b) { mulc(a,b,32); }
void mulc64(complex *a,complex *b) { mulc(a,b,64); }
void mulc128(complex *a,complex *b) { mulc(a,b,128); }
void mulc256(complex *a,complex *b) { mulc(a,b,256); }
void mulc512(complex *a,complex *b) { mulc(a,b,512); }
void mulc1024(complex *a,complex *b) { mulc(a,b,1024); }
void mulc2048(complex *a,complex *b) { mulc(a,b,2048); }
void mulc4096(complex *a,complex *b) { mulc(a,b,4096); }
void mulc8192(complex *a,complex *b) { mulc(a,b,8192); }
