#include "PRE"

void scaler2(real *a)
{
  register real t1, t2;

  t1 = a[0] * 0.5;
  t2 = a[1] * 0.5;
  a[0] = t1;
  a[1] = t2;
}

/* n multiple of 4, n >= 4 */
void scaler(register real *a,register unsigned int n,register real u)
{
  register real t1;

  t1 = VOL a[0];
  t1 *= u;
  a[0] = t1;
  t1 = VOL a[1];
  t1 *= u;
  a[1] = t1;
  u += u;
  t1 = VOL a[2];
  t1 *= u;
  a[2] = t1;
  t1 = VOL a[3];
  t1 *= u;
  a[3] = t1;

  while (n -= 4) {
    t1 = VOL a[4];
    t1 *= u;
    a[4] = t1;
    t1 = VOL a[5];
    t1 *= u;
    a[5] = t1;
    t1 = VOL a[6];
    t1 *= u;
    a[6] = t1;
    t1 = VOL a[7];
    t1 *= u;
    a[7] = t1;
    a += 4;
  }
}

void scaler4(real *a) { scaler(a,4,0.25); }
void scaler8(real *a) { scaler(a,8,0.125); }
void scaler16(real *a) { scaler(a,16,0.0625); }
void scaler32(real *a) { scaler(a,32,0.03125); }
void scaler64(real *a) { scaler(a,64,0.015625); }
void scaler128(real *a) { scaler(a,128,0.0078125); }
void scaler256(real *a) { scaler(a,256,0.00390625); }
void scaler512(real *a) { scaler(a,512,0.001953125); }
void scaler1024(real *a) { scaler(a,1024,0.0009765625); }
void scaler2048(real *a) { scaler(a,2048,0.00048828125); }
void scaler4096(real *a) { scaler(a,4096,0.000244140625); }
void scaler8192(real *a) { scaler(a,8192,0.0001220703125); }
