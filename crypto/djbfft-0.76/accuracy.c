#include <stdio.h>
#include <math.h>
#include "fftc4.h"
#include "fftc8.h"
#include "fftr4.h"
#include "fftr8.h"

#define ROTATE(x,b) x = (((x) << (b)) | ((x) >> (32 - (b))))
real8 myrand()
{
  static int pos = 0;
  static unsigned long in[4] = {0,0,0,0};
  static unsigned long t[16];
  register unsigned long y;
  register unsigned long x;
  register unsigned long sum;
  int rounds;

  if (!pos) {
    x = 0;
    sum = 0;
    rounds = 3;
    t[1] =t[2] =t[3] =0; t[0] =in[0];
    t[5] =t[6] =t[7] =0; t[4] =in[1];
    t[9] =t[10]=t[11]=0; t[8] =in[2];
    t[13]=t[14]=t[15]=0; t[12]=in[3];
    do {
      sum += 0x9e3779b9; y=x; y+=sum;
      ROTATE(x,5);  x^=y; y=t[0];  x+=y; y=sum; t[0] =x; y+=x;
      ROTATE(x,7);  x^=y; y=t[1];  x+=y; y=sum; t[1] =x; y+=x;
      ROTATE(x,9);  x^=y; y=t[2];  x+=y; y=sum; t[2] =x; y+=x;
      ROTATE(x,13); x^=y; y=t[3];  x+=y; y=sum; t[3] =x; y+=x;
      ROTATE(x,5);  x^=y; y=t[4];  x+=y; y=sum; t[4] =x; y+=x;
      ROTATE(x,7);  x^=y; y=t[5];  x+=y; y=sum; t[5] =x; y+=x;
      ROTATE(x,9);  x^=y; y=t[6];  x+=y; y=sum; t[6] =x; y+=x;
      ROTATE(x,13); x^=y; y=t[7];  x+=y; y=sum; t[7] =x; y+=x;
      ROTATE(x,5);  x^=y; y=t[8];  x+=y; y=sum; t[8] =x; y+=x;
      ROTATE(x,7);  x^=y; y=t[9];  x+=y; y=sum; t[9] =x; y+=x;
      ROTATE(x,9);  x^=y; y=t[10]; x+=y; y=sum; t[10]=x; y+=x;
      ROTATE(x,13); x^=y; y=t[11]; x+=y; y=sum; t[11]=x; y+=x;
      ROTATE(x,5);  x^=y; y=t[12]; x+=y; y=sum; t[12]=x; y+=x;
      ROTATE(x,7);  x^=y; y=t[13]; x+=y; y=sum; t[13]=x; y+=x;
      ROTATE(x,9);  x^=y; y=t[14]; x+=y; y=sum; t[14]=x; y+=x;
      ROTATE(x,13); x^=y; y=t[15]; x+=y;
      t[15] = x;
    } while (--rounds);
    if (!++in[0]) if (!++in[1]) if (!++in[2]) ++in[3];
    pos = 16;
  }

  return 0.0000000004656612873077392578125 * (long) (t[--pos] & 0x7fffffff);
}

complex4 x4[8192];
complex4 y4[8192];
complex4 z4[8192];

complex8 x8[8192];
complex8 y8[8192];
complex8 z8[8192];

void fill4(int n)
{
  int i;

  for (i = 0;i < n;++i) x4[i].re = myrand();
  for (i = 0;i < n;++i) x4[i].im = myrand();
  for (i = 0;i < n;++i) y4[i].re = myrand();
  for (i = 0;i < n;++i) y4[i].im = myrand();
  for (i = 0;i < n;++i) z4[i].re = 0;
  for (i = 0;i < n;++i) z4[i].im = 0;
}

void fill8(int n)
{
  int i;

  for (i = 0;i < n;++i) x8[i].re = myrand();
  for (i = 0;i < n;++i) x8[i].im = myrand();
  for (i = 0;i < n;++i) y8[i].re = myrand();
  for (i = 0;i < n;++i) y8[i].im = myrand();
  for (i = 0;i < n;++i) z8[i].re = 0;
  for (i = 0;i < n;++i) z8[i].im = 0;
}

real8 err4(int n)
{
  int i;
  real8 diff;
  real8 error = 0;

  for (i = 0;i < n;++i) {
    diff = x4[i].re - z4[i].re; error += diff * diff;
    diff = x4[i].im - z4[i].im; error += diff * diff;
  }

  return sqrt(error / n) / n;
}

real8 err8(int n)
{
  int i;
  real8 diff;
  real8 error = 0;

  for (i = 0;i < n;++i) {
    diff = x8[i].re - z8[i].re; error += diff * diff;
    diff = x8[i].im - z8[i].im; error += diff * diff;
  }

  return sqrt(error / n) / n;
}

void doitr4(unsigned int n,void (*fft)(real4 *),void (*unfft)(real4 *),void (*multiply)(real4 *,real4 *),void (*scale)(real4 *))
{
  int i;
  int j;
  int m;

  m = n / 2;

  fill4(m);

  for (i = 0;i < m;++i)
    for (j = 0;j < m;++j)
      if (i + j < m) {
        z4[i + j].re += x4[i].re * y4[j].re;
        z4[i + j].im += x4[i].re * y4[j].im;
        z4[i + j].im += x4[i].im * y4[j].re;
        z4[i + j].re += x4[i].im * y4[j].im;
      }
      else {
        z4[i + j - m].im += x4[i].re * y4[j].re;
        z4[i + j - m].re += x4[i].re * y4[j].im;
        z4[i + j - m].re += x4[i].im * y4[j].re;
        z4[i + j - m].im += x4[i].im * y4[j].im;
      }

  fft((real4 *) y4);
  scale((real4 *) y4);
  fft((real4 *) x4);
  multiply((real4 *) x4,(real4 *) y4);
  unfft((real4 *) x4);

  printf("%6d r4 %.30f\n",n,err4(m));
}

void doitr8(unsigned int n,void (*fft)(real8 *),void (*unfft)(real8 *),void (*multiply)(real8 *,real8 *),void (*scale)(real8 *))
{
  int i;
  int j;
  int m;

  m = n / 2;

  fill8(m);

  for (i = 0;i < m;++i)
    for (j = 0;j < m;++j)
      if (i + j < m) {
        z8[i + j].re += x8[i].re * y8[j].re;
        z8[i + j].im += x8[i].re * y8[j].im;
        z8[i + j].im += x8[i].im * y8[j].re;
        z8[i + j].re += x8[i].im * y8[j].im;
      }
      else {
        z8[i + j - m].im += x8[i].re * y8[j].re;
        z8[i + j - m].re += x8[i].re * y8[j].im;
        z8[i + j - m].re += x8[i].im * y8[j].re;
        z8[i + j - m].im += x8[i].im * y8[j].im;
      }

  fft((real8 *) y8);
  scale((real8 *) y8);
  fft((real8 *) x8);
  multiply((real8 *) x8,(real8 *) y8);
  unfft((real8 *) x8);

  printf("%6d r8 %.30f\n",n,err8(m));
}

void doitc4(unsigned int n,void (*fft)(complex4 *),void (*unfft)(complex4 *),void (*multiply)(complex4 *,complex4 *),void (*scale)(complex4 *))
{
  int i;
  int j;

  fill4(n);

  for (i = 0;i < n;++i)
    for (j = 0;j < n;++j) {
      z4[(i + j) & (n - 1)].re += x4[i].re * y4[j].re;
      z4[(i + j) & (n - 1)].im += x4[i].re * y4[j].im;
      z4[(i + j) & (n - 1)].im += x4[i].im * y4[j].re;
      z4[(i + j) & (n - 1)].re -= x4[i].im * y4[j].im;
    }

  fft(y4);
  scale(y4);
  fft(x4);
  multiply(x4,y4);
  unfft(x4);

  printf("%6d c4 %.30f\n",n,err4(n));
}

void doitc8(unsigned int n,void (*fft)(complex8 *),void (*unfft)(complex8 *),void (*multiply)(complex8 *,complex8 *),void (*scale)(complex8 *))
{
  int i;
  int j;

  fill8(n);

  for (i = 0;i < n;++i)
    for (j = 0;j < n;++j) {
      z8[(i + j) & (n - 1)].re += x8[i].re * y8[j].re;
      z8[(i + j) & (n - 1)].im += x8[i].re * y8[j].im;
      z8[(i + j) & (n - 1)].im += x8[i].im * y8[j].re;
      z8[(i + j) & (n - 1)].re -= x8[i].im * y8[j].im;
    }

  fft(y8);
  scale(y8);
  fft(x8);
  multiply(x8,y8);
  unfft(x8);

  printf("%6d c8 %.30f\n",n,err8(n));
}

main()
{
  doitr4(2,fftr4_2,fftr4_un2,fftr4_mul2,fftr4_scale2);
  doitc4(2,fftc4_2,fftc4_un2,fftc4_mul2,fftc4_scale2);
  doitr4(4,fftr4_4,fftr4_un4,fftr4_mul4,fftr4_scale4);
  doitc4(4,fftc4_4,fftc4_un4,fftc4_mul4,fftc4_scale4);
  doitr4(8,fftr4_8,fftr4_un8,fftr4_mul8,fftr4_scale8);
  doitc4(8,fftc4_8,fftc4_un8,fftc4_mul8,fftc4_scale8);
  doitr4(16,fftr4_16,fftr4_un16,fftr4_mul16,fftr4_scale16);
  doitc4(16,fftc4_16,fftc4_un16,fftc4_mul16,fftc4_scale16);
  doitr4(32,fftr4_32,fftr4_un32,fftr4_mul32,fftr4_scale32);
  doitc4(32,fftc4_32,fftc4_un32,fftc4_mul32,fftc4_scale32);
  doitr4(64,fftr4_64,fftr4_un64,fftr4_mul64,fftr4_scale64);
  doitc4(64,fftc4_64,fftc4_un64,fftc4_mul64,fftc4_scale64);
  doitr4(128,fftr4_128,fftr4_un128,fftr4_mul128,fftr4_scale128);
  doitc4(128,fftc4_128,fftc4_un128,fftc4_mul128,fftc4_scale128);
  doitr4(256,fftr4_256,fftr4_un256,fftr4_mul256,fftr4_scale256);
  doitc4(256,fftc4_256,fftc4_un256,fftc4_mul256,fftc4_scale256);
  doitr4(512,fftr4_512,fftr4_un512,fftr4_mul512,fftr4_scale512);
  doitc4(512,fftc4_512,fftc4_un512,fftc4_mul512,fftc4_scale512);
  doitr4(1024,fftr4_1024,fftr4_un1024,fftr4_mul1024,fftr4_scale1024);
  doitc4(1024,fftc4_1024,fftc4_un1024,fftc4_mul1024,fftc4_scale1024);
  doitr4(2048,fftr4_2048,fftr4_un2048,fftr4_mul2048,fftr4_scale2048);
  doitc4(2048,fftc4_2048,fftc4_un2048,fftc4_mul2048,fftc4_scale2048);
  doitr4(4096,fftr4_4096,fftr4_un4096,fftr4_mul4096,fftr4_scale4096);
  doitc4(4096,fftc4_4096,fftc4_un4096,fftc4_mul4096,fftc4_scale4096);
  doitr4(8192,fftr4_8192,fftr4_un8192,fftr4_mul8192,fftr4_scale8192);
  doitc4(8192,fftc4_8192,fftc4_un8192,fftc4_mul8192,fftc4_scale8192);

  doitr8(2,fftr8_2,fftr8_un2,fftr8_mul2,fftr8_scale2);
  doitc8(2,fftc8_2,fftc8_un2,fftc8_mul2,fftc8_scale2);
  doitr8(4,fftr8_4,fftr8_un4,fftr8_mul4,fftr8_scale4);
  doitc8(4,fftc8_4,fftc8_un4,fftc8_mul4,fftc8_scale4);
  doitr8(8,fftr8_8,fftr8_un8,fftr8_mul8,fftr8_scale8);
  doitc8(8,fftc8_8,fftc8_un8,fftc8_mul8,fftc8_scale8);
  doitr8(16,fftr8_16,fftr8_un16,fftr8_mul16,fftr8_scale16);
  doitc8(16,fftc8_16,fftc8_un16,fftc8_mul16,fftc8_scale16);
  doitr8(32,fftr8_32,fftr8_un32,fftr8_mul32,fftr8_scale32);
  doitc8(32,fftc8_32,fftc8_un32,fftc8_mul32,fftc8_scale32);
  doitr8(64,fftr8_64,fftr8_un64,fftr8_mul64,fftr8_scale64);
  doitc8(64,fftc8_64,fftc8_un64,fftc8_mul64,fftc8_scale64);
  doitr8(128,fftr8_128,fftr8_un128,fftr8_mul128,fftr8_scale128);
  doitc8(128,fftc8_128,fftc8_un128,fftc8_mul128,fftc8_scale128);
  doitr8(256,fftr8_256,fftr8_un256,fftr8_mul256,fftr8_scale256);
  doitc8(256,fftc8_256,fftc8_un256,fftc8_mul256,fftc8_scale256);
  doitr8(512,fftr8_512,fftr8_un512,fftr8_mul512,fftr8_scale512);
  doitc8(512,fftc8_512,fftc8_un512,fftc8_mul512,fftc8_scale512);
  doitr8(1024,fftr8_1024,fftr8_un1024,fftr8_mul1024,fftr8_scale1024);
  doitc8(1024,fftc8_1024,fftc8_un1024,fftc8_mul1024,fftc8_scale1024);
  doitr8(2048,fftr8_2048,fftr8_un2048,fftr8_mul2048,fftr8_scale2048);
  doitc8(2048,fftc8_2048,fftc8_un2048,fftc8_mul2048,fftc8_scale2048);
  doitr8(4096,fftr8_4096,fftr8_un4096,fftr8_mul4096,fftr8_scale4096);
  doitc8(4096,fftc8_4096,fftc8_un4096,fftc8_mul4096,fftc8_scale4096);
  doitr8(8192,fftr8_8192,fftr8_un8192,fftr8_mul8192,fftr8_scale8192);
  doitc8(8192,fftc8_8192,fftc8_un8192,fftc8_mul8192,fftc8_scale8192);

  exit(0);
}
