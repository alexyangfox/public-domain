#include "PRE"

void scaler2(real *a)
{
  register real t0, t1;
  register real u;

  u = 0.5;
  t0 = a[0];
  t1 = a[1];
  t0 *= u; a[0] = t0;
  t1 *= u; a[1] = t1;
}

void scaler4(real *a) { scalec2((complex *) a); scaler2(a); }
void scaler8(real *a) { scalec4((complex *) a); scaler2(a); }
void scaler16(real *a) { scalec8((complex *) a); scaler2(a); }
void scaler32(real *a) { scalec16((complex *) a); scaler2(a); }
void scaler64(real *a) { scalec32((complex *) a); scaler2(a); }
void scaler128(real *a) { scalec64((complex *) a); scaler2(a); }
void scaler256(real *a) { scalec128((complex *) a); scaler2(a); }
void scaler512(real *a) { scalec256((complex *) a); scaler2(a); }
void scaler1024(real *a) { scalec512((complex *) a); scaler2(a); }
void scaler2048(real *a) { scalec1024((complex *) a); scaler2(a); }
void scaler4096(real *a) { scalec2048((complex *) a); scaler2(a); }
void scaler8192(real *a) { scalec4096((complex *) a); scaler2(a); }
