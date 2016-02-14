#include "PRE"

void mulr2(real *a,real *b)
{
  register real t1, t2;

  t1 = a[0] * b[0];
  t2 = a[1] * b[1];
  a[0] = t1;
  a[1] = t2;
}

void mulr4(real *a,real *b)
{
  register real t1, t2, t3, t4, t5, t6;

  t1 = a[2] * b[2];
  t2 = a[3] * b[3];
  t3 = a[3] * b[2];
  t4 = a[2] * b[3];
  t5 = a[0] * b[0];
  t6 = a[1] * b[1];
  t1 -= t2;
  t3 += t4;
  a[0] = t5;
  a[1] = t6;
  a[2] = t1;
  a[3] = t3;
}

/* n multiple of 4, n >= 8 */
void mulr(register real *a,register real *b,register unsigned int n)
{
  mulr4(a,b);
  mulc((complex *)(a + 4),(complex *)(b + 4),(n - 4) / 2);
}

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
