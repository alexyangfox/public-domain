#include "PRE"

/* n even, n >= 4 */
void mulr(register real *a,register real *b,register unsigned int n)
{
  a[0] *= b[0];
  a[1] *= b[1];
  mulc((complex *)(a + 2),(complex *)(b + 2),(n - 2) / 2);
}

void mulr2(real *a,real *b) { a[0] *= b[0]; a[1] *= b[1]; }
void mulr4(real *a,real *b) { mulr(a,b,4); }
void mulr8(real *a,real *b) { mulr(a,b,8); }
void mulr16(real *a,real *b) { mulr(a,b,16); }
void mulr32(real *a,real *b) { mulr(a,b,32); }
void mulr64(real *a,real *b) { mulr(a,b,64); }
void mulr128(real *a,real *b) { mulr(a,b,128); }
void mulr256(real *a,real *b) { mulr(a,b,256); }
void mulr512(real *a,real *b) { mulr(a,b,512); }
void mulr1024(real *a,real *b) { mulr(a,b,1024); }
void mulr2048(real *a,real *b) { mulr(a,b,2048); }
void mulr4096(real *a,real *b) { mulr(a,b,4096); }
void mulr8192(real *a,real *b) { mulr(a,b,8192); }
