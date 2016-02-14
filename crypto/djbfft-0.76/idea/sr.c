#include "PRE"

/* n even, n >= 4 */
void scaler(register real *a,register unsigned int n)
{
  register real u = 1.0 / n;

  a[0] *= u;
  a[1] *= u;
  a += 2;
  n -= 2;

  u += u;

  do {
    a[0] *= u;
    a[1] *= u;
    a += 2;
  } while (n -= 2);
}

void scaler2(real *a) { a[0] *= 0.5; a[1] *= 0.5; }
void scaler4(real *a) { scaler(a,4); }
void scaler8(real *a) { scaler(a,8); }
void scaler16(real *a) { scaler(a,16); }
void scaler32(real *a) { scaler(a,32); }
void scaler64(real *a) { scaler(a,64); }
void scaler128(real *a) { scaler(a,128); }
void scaler256(real *a) { scaler(a,256); }
void scaler512(real *a) { scaler(a,512); }
void scaler1024(real *a) { scaler(a,1024); }
void scaler2048(real *a) { scaler(a,2048); }
void scaler4096(real *a) { scaler(a,4096); }
void scaler8192(real *a) { scaler(a,8192); }
