
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

#define TRANSFORM(a0,a1,a2,a3,wre,wim) { \
  t1 = a0.re - a2.re; \
  t2 = a0.im - a2.im; \
  t3 = a1.re - a3.re; \
  t4 = a1.im - a3.im; \
  a0.re += a2.re; \
  a0.im += a2.im; \
  a1.re += a3.re; \
  a1.im += a3.im; \
  t5 = t1 - t4; \
  t6 = t2 + t3; \
  t7 = t1 + t4; \
  t8 = t2 - t3; \
  a2.re = t5 * wre - t6 * wim; \
  a2.im = t6 * wre + t5 * wim; \
  a3.re = t7 * wre + t8 * wim; \
  a3.im = t8 * wre - t7 * wim; \
  }

#define TRANSFORMHALF(a0,a1,a2,a3) { \
  t1 = a0.re - a2.re; \
  t2 = a0.im - a2.im; \
  t3 = a1.re - a3.re; \
  t4 = a1.im - a3.im; \
  a0.re += a2.re; \
  a0.im += a2.im; \
  a1.re += a3.re; \
  a1.im += a3.im; \
  t5 = t1 - t4; \
  t6 = t2 + t3; \
  t7 = t1 + t4; \
  t8 = t2 - t3; \
  a2.re = (t5 - t6) * sqrthalf; \
  a2.im = (t6 + t5) * sqrthalf; \
  a3.re = (t7 + t8) * sqrthalf; \
  a3.im = (t8 - t7) * sqrthalf; \
  }

#define TRANSFORMZERO(a0,a1,a2,a3) { \
  t1 = a0.re - a2.re; \
  t2 = a0.im - a2.im; \
  t3 = a1.re - a3.re; \
  t4 = a1.im - a3.im; \
  a0.re += a2.re; \
  a0.im += a2.im; \
  a1.re += a3.re; \
  a1.im += a3.im; \
  a2.re = t1 - t4; \
  a2.im = t2 + t3; \
  a3.re = t1 + t4; \
  a3.im = t2 - t3; \
  }

#define UNTRANSFORM(a0,a1,a2,a3,wre,wim) { \
  t5 = a2.re * wre + a2.im * wim; \
  t6 = a2.im * wre - a2.re * wim; \
  t7 = a3.re * wre - a3.im * wim; \
  t8 = a3.im * wre + a3.re * wim; \
  t1 = t5 + t7; \
  t2 = t6 + t8; \
  t3 = t6 - t8; \
  t4 = t7 - t5; \
  a2.re = a0.re - t1; \
  a2.im = a0.im - t2; \
  a3.re = a1.re - t3; \
  a3.im = a1.im - t4; \
  a0.re += t1; \
  a0.im += t2; \
  a1.re += t3; \
  a1.im += t4; \
  }

#define UNTRANSFORMHALF(a0,a1,a2,a3) { \
  t5 = (a2.re + a2.im) * sqrthalf; \
  t6 = (a2.im - a2.re) * sqrthalf; \
  t7 = (a3.re - a3.im) * sqrthalf; \
  t8 = (a3.im + a3.re) * sqrthalf; \
  t1 = t5 + t7; \
  t2 = t6 + t8; \
  t3 = t6 - t8; \
  t4 = t7 - t5; \
  a2.re = a0.re - t1; \
  a2.im = a0.im - t2; \
  a3.re = a1.re - t3; \
  a3.im = a1.im - t4; \
  a0.re += t1; \
  a0.im += t2; \
  a1.re += t3; \
  a1.im += t4; \
  }

#define UNTRANSFORMZERO(a0,a1,a2,a3) { \
  t1 = a2.re + a3.re; \
  t2 = a2.im + a3.im; \
  t3 = a2.im - a3.im; \
  t4 = a3.re - a2.re; \
  a2.re = a0.re - t1; \
  a2.im = a0.im - t2; \
  a3.re = a1.re - t3; \
  a3.im = a1.im - t4; \
  a0.re += t1; \
  a0.im += t2; \
  a1.re += t3; \
  a1.im += t4; \
  }

#define R(a0,a1,b0,b1,wre,wim) { \
  t1 = a0 - a1; \
  t2 = b0 - b1; \
  t3 = a0 + a1; \
  t4 = b0 + b1; \
  b0 = t1 * wre - t2 * wim; \
  b1 = t2 * wre + t1 * wim; \
  a0 = t3; \
  a1 = t4; \
  }

#define RHALF(a0,a1,b0,b1) { \
  t1 = a0 - a1; \
  t2 = b0 - b1; \
  t3 = a0 + a1; \
  t4 = b0 + b1; \
  b0 = (t1 - t2) * sqrthalf; \
  b1 = (t2 + t1) * sqrthalf; \
  a0 = t3; \
  a1 = t4; \
  }

#define RZERO(a0,a1,b0,b1) { \
  t1 = a0 - a1; \
  t2 = b0 - b1; \
  t3 = a0 + a1; \
  t4 = b0 + b1; \
  b0 = t1; \
  b1 = t2; \
  a0 = t3; \
  a1 = t4; \
  }

#define V(a0,a1,b0,b1,wre,wim) { \
  t5 = b0 * wre + b1 * wim; \
  t6 = b1 * wre - b0 * wim; \
  t1 = a0 + t5; \
  t2 = a0 - t5; \
  t3 = a1 + t6; \
  t4 = a1 - t6; \
  a0 = t1; \
  a1 = t2; \
  b0 = t3; \
  b1 = t4; \
  }

#define VHALF(a0,a1,b0,b1) { \
  t5 = (b0 + b1) * sqrthalf; \
  t6 = (b1 - b0) * sqrthalf; \
  t1 = a0 + t5; \
  t2 = a0 - t5; \
  t3 = a1 + t6; \
  t4 = a1 - t6; \
  a0 = t1; \
  a1 = t2; \
  b0 = t3; \
  b1 = t4; \
  }

#define VZERO(a0,a1,b0,b1) { \
  t1 = a0 + b0; \
  t2 = a0 - b0; \
  t3 = a1 + b1; \
  t4 = a1 - b1; \
  a0 = t1; \
  a1 = t2; \
  b0 = t3; \
  b1 = t4; \
  }

