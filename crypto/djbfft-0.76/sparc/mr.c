#include "PRE"

void mulr2(real *a,real *b)
{
  register real t0, t1, t2, t3;

  t0 = a[0];
  t1 = b[0];
  t2 = a[1];
  t3 = b[1];
  t0 *= t1;
  t2 *= t3;
  a[0] = t0;
  a[1] = t2;
}

void mulr4(real *a,real *b)
{
  register real t0, t1, t2, t3, t4, t5, t6, t7, t8, t9;

  mulr2(a,b);
  t0 = a[2];
  t1 = b[2];
  t2 = b[3];
  t3 = a[3];
  t4 = t0 * t1;
  t5 = t0 * t2;
  t6 = t3 * t2;
  t7 = t3 * t1;
  
  t8 = t4 - t6;
  t9 = t5 + t7;
  a[2] = t8;
  a[3] = t9;
}

void mulr8(real *a,real *b)
{
  mulr4(a,b);
  mulc2((complex *)(a + 4),(complex *)(b + 4));
}

/* n multiple of 8, n >= 16 */
void mulr(register real *a,register real *b,register unsigned int n)
{
  mulr8(a,b);
  mulc((complex *)(a + 8),(complex *)(b + 8),(n - 8) / 2);
}

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
