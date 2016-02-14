
extern void cpass(complex *,const complex *,unsigned int);
extern void cpassbig(complex *,const complex *,unsigned int);
extern void upass(complex *,const complex *,unsigned int);
extern void upassbig(complex *,const complex *,unsigned int);
extern void rpass(real *,const complex *,unsigned int);
extern void rpassbig(real *,const complex *,unsigned int);
extern void vpass(real *,const complex *,unsigned int);
extern void vpassbig(real *,const complex *,unsigned int);

extern const complex d16[];
extern const complex d32[];
extern const complex d64[];
extern const complex d128[];
extern const complex d256[];
extern const complex d512[];
extern const complex d1024[];
extern const complex d2048[];
extern const complex d4096[];
extern const complex d8192[];
extern const complex d16384[];
extern const complex d32768[];
extern const complex d65536[];

#define sqrthalf (d16[1].re)

#define VOL *(volatile real *)&

/* assumes sizeof(complex) is a multiple of 4 */
#define A0 (a)
#define A1 ((complex *) (n * (sizeof(complex) / 4) + (char *) a))
#define A2 (b)
#define A3 ((complex *) (n * (sizeof(complex) / 4) + (char *) b))
#define X0 (x)
#define X1 ((complex *) (n * (sizeof(complex) / 4) + (char *) x))
#define X2 (y)
#define X3 ((complex *) (n * (sizeof(complex) / 4) + (char *) y))

#define TRANSFORM(a0,a1,a2,a3,wre,wim) { \
  t1 = a0.re + a2.re; \
  t2 = a0.im + a2.im; \
  t3 = a1.re + a3.re; \
  t5 = VOL a0.re; \
  a0.re = t1; \
  t6 = VOL a0.im; \
  t5 -= a2.re; \
  t4 = VOL a1.im; \
  t4 += a3.im; \
  a0.im = t2; \
  t8 = VOL a1.im; \
  a1.im = t4; \
  t7 = VOL a1.re; \
  t8 -= a3.im; \
  t6 -= a2.im; \
  t1 = t5 - t8; \
  t5 += t8; \
  a1.re = t3; \
  t3 = t1 * wim; \
  t7 -= a3.re; \
  t1 *= wre; \
  t8 = t5; \
  t5 *= wre; \
  t2 = t6; \
  t2 += t7; \
  t8 *= wim; \
  t6 -= t7; \
  t4 = t2; \
  t4 *= wim; \
  t7 = t6; \
  t2 *= wre; \
  t1 -= t4; \
  t6 *= wre; \
  t2 += t3; \
  t7 *= wim; \
  t6 -= t8; \
  a2.re = t1; \
  t5 += t7; \
  a3.im = t6; \
  a2.im = t2; \
  a3.re = t5; \
  }

#define TRANSFORMHALF(a0,a1,a2,a3) { \
  t1 = a0.re + a2.re; \
  t2 = a0.im + a2.im; \
  t3 = a1.re + a3.re; \
  t5 = VOL a0.re; \
  a0.re = t1; \
  t6 = VOL a0.im; \
  t5 -= a2.re; \
  t4 = VOL a1.im; \
  t4 += a3.im; \
  a0.im = t2; \
  t8 = VOL a1.im; \
  a1.im = t4; \
  t7 = a1.re - a3.re; \
  t6 -= a2.im; \
  t1 = t5; \
  t8 -= a3.im; \
  a1.re = t3; \
  t2 = t6; \
  t6 -= t7; \
  t5 += t8; \
  t2 += t7; \
  t4 = t6; \
  t1 -= t8; \
  t4 += t5; \
  t6 -= t5; \
  t7 = t1; \
  t4 *= sqrthalf; \
  t7 -= t2; \
  t6 *= sqrthalf; \
  t2 += t1; \
  t7 *= sqrthalf; \
  a3.re = t4; \
  t2 *= sqrthalf; \
  a3.im = t6; \
  a2.re = t7; \
  a2.im = t2; \
  }

#define TRANSFORMZERO(a0,a1,a2,a3) { \
  t5 = a0.re + a2.re; \
  t6 = a0.im + a2.im; \
  t7 = a1.re + a3.re; \
  t8 = a1.im + a3.im; \
  t1 = a0.re - a2.re; \
  a0.re = t5; \
  t2 = a0.im - a2.im; \
  a0.im = t6; \
  t3 = a1.re - a3.re; \
  a1.re = t7; \
  t4 = a1.im - a3.im; \
  a1.im = t8; \
  t6 = t1 - t4; \
  t7 = t2 + t3; \
  t1 += t4; \
  t2 -= t3; \
  a2.re = t6; \
  a3.im = t2; \
  a3.re = t1; \
  a2.im = t7; \
  }

#define UNTRANSFORM(a0,a1,a2,a3,wre,wim) { \
  t1 = a2.re * wre; \
  t3 = a2.im * wim; \
  t5 = a3.re * wre; \
  t7 = a3.im * wim; \
  t2 = a2.im * wre; \
  t4 = a2.re * wim; \
  t6 = a3.im * wre; \
  t8 = a3.re * wim; \
  t1 += t3; \
  t5 -= t7; \
  t2 -= t4; \
  t6 += t8; \
  t3 = t5 + t1; \
  t5 -= t1; \
  t4 = t2 - t6; \
  t6 += t2; \
  t1 = a0.re - t3; \
  t3 += a0.re; \
  t2 = a0.im - t6; \
  t6 += a0.im; \
  t8 = a1.re - t4; \
  t4 += a1.re; \
  t7 = a1.im - t5; \
  t5 += a1.im; \
  a2.re = t1; \
  a1.re = t4; \
  a0.im = t6; \
  a0.re = t3; \
  a1.im = t5; \
  a2.im = t2; \
  a3.im = t7; \
  a3.re = t8; \
  }

#define UNTRANSFORMHALF(a0,a1,a2,a3) { \
  t1 = a2.re + a2.im; \
  t2 = a2.im - a2.re; \
  t1 *= sqrthalf; \
  t3 = a3.re - a3.im; \
  t2 *= sqrthalf; \
  t4 = a3.im + a3.re; \
  t3 *= sqrthalf; \
  t7 = t2; \
  t4 *= sqrthalf; \
  t8 = t3; \
  t8 -= t1; \
  t7 -= t4; \
  t1 += t3; \
  t2 += t4; \
  t4 = a1.im - t8; \
  t8 += a1.im; \
  t3 = a1.re - t7; \
  t7 += a1.re; \
  a3.im = t4; \
  t5 = a0.re - t1; \
  t1 += a0.re; \
  a1.im = t8; \
  t6 = a0.im - t2; \
  t2 += a0.im; \
  a2.re = t5; \
  a0.re = t1; \
  a2.im = t6; \
  a1.re = t7; \
  a3.re = t3; \
  a0.im = t2; \
  }

#define UNTRANSFORMZERO(a0,a1,a2,a3) { \
  t1 = a2.re + a3.re; \
  t2 = a2.im + a3.im; \
  t3 = a2.im - a3.im; \
  t4 = a3.re - a2.re; \
  t5 = a0.re - t1; \
  t6 = a0.im - t2; \
  t7 = a1.re - t3; \
  t8 = a1.im - t4; \
  t1 += a0.re; \
  t2 += a0.im; \
  t3 += a1.re; \
  t4 += a1.im; \
  a2.re = t5; \
  a3.re = t7; \
  a2.im = t6; \
  a1.im = t4; \
  a1.re = t3; \
  a0.im = t2; \
  a0.re = t1; \
  a3.im = t8; \
  }

#define R(a0,a1,b0,b1,wre,wim) { \
  t1 = a0 - a1; \
  t2 = b0 - b1; \
  t5 = t1 * wim; \
  t6 = t2 * wim; \
  t3 = VOL a0; \
  t1 *= wre; \
  t3 += a1; \
  t2 *= wre; \
  t1 -= t6; \
  t4 = VOL b0; \
  t2 += t5; \
  t4 += b1; \
  a0 = t3; \
  b1 = t2; \
  a1 = t4; \
  b0 = t1; \
  }

#define RHALF(a0,a1,b0,b1) { \
  t1 = a0 - a1; \
  t2 = b0 - b1; \
  t3 = a0 + a1; \
  t5 = t1 - t2; \
  t1 += t2; \
  t4 = VOL b0; \
  t5 *= sqrthalf; \
  t4 += b1; \
  t1 *= sqrthalf; \
  a0 = t3; \
  b1 = t1; \
  a1 = t4; \
  b0 = t5; \
  }

#define RZERO(a0,a1,b0,b1) { \
  t1 = a0 - a1; \
  t2 = b0 - b1; \
  t3 = a0 + a1; \
  t4 = b0 + b1; \
  b0 = t1; \
  a0 = t3; \
  b1 = t2; \
  a1 = t4; \
  }

#define V(a0,a1,b0,b1,wre,wim) { \
  t5 = b0 * wre; \
  t1 = b1 * wim; \
  t6 = b1 * wre; \
  t5 += t1; \
  t3 = b0 * wim; \
  t2 = a0 - t5; \
  t6 -= t3; \
  t5 += a0; \
  t4 = a1 - t6; \
  t6 += a1; \
  a1 = t2; \
  a0 = t5; \
  b1 = t4; \
  b0 = t6; \
  }

#define VHALF(a0,a1,b0,b1) { \
  t5 = b0 + b1; \
  t6 = b1 - b0; \
  t5 *= sqrthalf; \
  t2 = VOL a0; \
  t6 *= sqrthalf; \
  t2 -= t5; \
  t5 += a0; \
  t4 = a1 - t6; \
  t6 += a1; \
  a1 = t2; \
  a0 = t5; \
  b0 = t6; \
  b1 = t4; \
  }

#define VZERO(a0,a1,b0,b1) { \
  t1 = a0 + b0; \
  t2 = a0 - b0; \
  t3 = a1 + b1; \
  t4 = a1 - b1; \
  a0 = t1; \
  b0 = t3; \
  a1 = t2; \
  b1 = t4; \
  }
