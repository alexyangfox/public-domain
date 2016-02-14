#include <stdio.h>
#include <math.h>
#include "fftc4.h"
#include "fftc8.h"
#include "fftr4.h"
#include "fftr8.h"
#include "fftfreq.h"

unsigned int freq[8192];
complex8 testroots[8192];
complex4 x4[8192];
complex8 x8[8192];
real4 y4[8192];
real8 y8[8192];

void doitc4(unsigned int m,void (*fft)(complex4 *),void (*unfft)(complex4 *))
{
  unsigned int j;
  unsigned int t;
  double dr;
  double di;
  double err;

  fftfreq_ctable(freq,m);
  for (j = 0;j < m;++j) {
    testroots[j].re = cos(2 * M_PI * j / m);
    testroots[j].im = sin(2 * M_PI * j / m);
  }

  err = 0;
  for (t = 0;t < m;++t) {
    for (j = 0;j < m;++j) x4[j].re = x4[j].im = 0;
    x4[t].re = 1;
    fft(x4);
    for (j = 0;j < m;++j) {
      dr = x4[j].re - testroots[(freq[j] * t) % m].re;
      di = x4[j].im - testroots[(freq[j] * t) % m].im;
      err += (dr * dr + di * di);
    }
    for (j = 0;j < m;++j) x4[j].re = x4[j].im = 0;
    x4[t].im = 1;
    fft(x4);
    for (j = 0;j < m;++j) {
      dr = x4[j].re + testroots[(freq[j] * t) % m].im;
      di = x4[j].im - testroots[(freq[j] * t) % m].re;
      err += (dr * dr + di * di);
    }
  }
  printf("%6d c4+ %.30f\n",m,sqrt((err / m) / m));
  fflush(stdout);

  err = 0;
  for (t = 0;t < m;++t) {
    for (j = 0;j < m;++j) x4[j].re = x4[j].im = 0;
    x4[t].re = 1;
    unfft(x4);
    for (j = 0;j < m;++j) {
      dr = x4[j].re - testroots[(j * freq[t]) % m].re;
      di = x4[j].im + testroots[(j * freq[t]) % m].im;
      err += (dr * dr + di * di);
    }
    for (j = 0;j < m;++j) x4[j].re = x4[j].im = 0;
    x4[t].im = 1;
    unfft(x4);
    for (j = 0;j < m;++j) {
      dr = x4[j].re - testroots[(j * freq[t]) % m].im;
      di = x4[j].im - testroots[(j * freq[t]) % m].re;
      err += (dr * dr + di * di);
    }
  }
  printf("%6d c4- %.30f\n",m,sqrt((err / m) / m));
  fflush(stdout);
}

void doitc8(unsigned int m,void (*fft)(complex8 *),void (*unfft)(complex8 *))
{
  unsigned int j;
  unsigned int t;
  double dr;
  double di;
  double err;

  fftfreq_ctable(freq,m);
  for (j = 0;j < m;++j) {
    testroots[j].re = cos(2 * M_PI * j / m);
    testroots[j].im = sin(2 * M_PI * j / m);
  }

  err = 0;
  for (t = 0;t < m;++t) {
    for (j = 0;j < m;++j) x8[j].re = x8[j].im = 0;
    x8[t].re = 1;
    fft(x8);
    for (j = 0;j < m;++j) {
      dr = x8[j].re - testroots[(freq[j] * t) % m].re;
      di = x8[j].im - testroots[(freq[j] * t) % m].im;
      err += (dr * dr + di * di);
    }
    for (j = 0;j < m;++j) x8[j].re = x8[j].im = 0;
    x8[t].im = 1;
    fft(x8);
    for (j = 0;j < m;++j) {
      dr = x8[j].re + testroots[(freq[j] * t) % m].im;
      di = x8[j].im - testroots[(freq[j] * t) % m].re;
      err += (dr * dr + di * di);
    }
  }
  printf("%6d c8+ %.30f\n",m,sqrt((err / m) / m));
  fflush(stdout);

  err = 0;
  for (t = 0;t < m;++t) {
    for (j = 0;j < m;++j) x8[j].re = x8[j].im = 0;
    x8[t].re = 1;
    unfft(x8);
    for (j = 0;j < m;++j) {
      dr = x8[j].re - testroots[(j * freq[t]) % m].re;
      di = x8[j].im + testroots[(j * freq[t]) % m].im;
      err += (dr * dr + di * di);
    }
    for (j = 0;j < m;++j) x8[j].re = x8[j].im = 0;
    x8[t].im = 1;
    unfft(x8);
    for (j = 0;j < m;++j) {
      dr = x8[j].re - testroots[(j * freq[t]) % m].im;
      di = x8[j].im - testroots[(j * freq[t]) % m].re;
      err += (dr * dr + di * di);
    }
  }
  printf("%6d c8- %.30f\n",m,sqrt((err / m) / m));
  fflush(stdout);
}

void doitr4(unsigned int m,void (*fft)(real4 *),void (*unfft)(real4 *))
{
  unsigned int j;
  unsigned int t;
  double dr;
  double di;
  double err;

  fftfreq_rtable(freq,m);
  for (j = 0;j < m;++j) {
    testroots[j].re = cos(2 * M_PI * j / m);
    testroots[j].im = sin(2 * M_PI * j / m);
  }

  err = 0;
  for (t = 0;t < m;++t) {
    for (j = 0;j < m;++j) y4[j] = 0;
    y4[((t * 2) % m) + (t / (m / 2))] = 1;
    fft(y4);
    dr = y4[0] - testroots[0].re;
    err += dr * dr;
    dr = y4[1] - testroots[((m/2) * t) % m].re;
    err += dr * dr;
    for (j = 2;j < m;j += 2) {
      dr = y4[j] - testroots[(freq[j] * t) % m].re;
      di = y4[j + 1] - testroots[(freq[j] * t) % m].im;
      err += dr * dr + di * di;
    }
  }
  printf("%6d r4+ %.30f\n",m,sqrt((err / m) / m));
  fflush(stdout);

  err = 0;
  for (t = 0;t < m;++t) {
    for (j = 0;j < m;++j) y4[j] = 0;
    y4[t] = 1;
    unfft(y4);
    for (j = 0;j < m;++j) {
      dr = y4[((j * 2) % m) + (j / (m / 2))];
      if ((t > 1) && (t & 1))
        dr -= testroots[(j * freq[t]) % m].im;
      else
        dr -= testroots[(j * freq[t]) % m].re;
      err += dr * dr;
    }
  }
  printf("%6d r4- %.30f\n",m,sqrt((err / m) / m));
  fflush(stdout);
}

void doitr8(unsigned int m,void (*fft)(real8 *),void (*unfft)(real8 *))
{
  unsigned int j;
  unsigned int t;
  double dr;
  double di;
  double err;

  fftfreq_rtable(freq,m);
  for (j = 0;j < m;++j) {
    testroots[j].re = cos(2 * M_PI * j / m);
    testroots[j].im = sin(2 * M_PI * j / m);
  }

  err = 0;
  for (t = 0;t < m;++t) {
    for (j = 0;j < m;++j) y8[j] = 0;
    y8[((t * 2) % m) + (t / (m / 2))] = 1;
    fft(y8);
    dr = y8[0] - testroots[0].re;
    err += dr * dr;
    dr = y8[1] - testroots[((m/2) * t) % m].re;
    err += dr * dr;
    for (j = 2;j < m;j += 2) {
      dr = y8[j] - testroots[(freq[j] * t) % m].re;
      di = y8[j + 1] - testroots[(freq[j] * t) % m].im;
      err += dr * dr + di * di;
    }
  }
  printf("%6d r8+ %.30f\n",m,sqrt((err / m) / m));
  fflush(stdout);

  err = 0;
  for (t = 0;t < m;++t) {
    for (j = 0;j < m;++j) y8[j] = 0;
    y8[t] = 1;
    unfft(y8);
    for (j = 0;j < m;++j) {
      dr = y8[((j * 2) % m) + (j / (m / 2))];
      if ((t > 1) && (t & 1))
        dr -= testroots[(j * freq[t]) % m].im;
      else
        dr -= testroots[(j * freq[t]) % m].re;
      err += dr * dr;
    }
  }
  printf("%6d r8- %.30f\n",m,sqrt((err / m) / m));
  fflush(stdout);
}

main()
{
  doitr4(2,fftr4_2,fftr4_un2);
  doitc4(2,fftc4_2,fftc4_un2);
  doitr4(4,fftr4_4,fftr4_un4);
  doitc4(4,fftc4_4,fftc4_un4);
  doitr4(8,fftr4_8,fftr4_un8);
  doitc4(8,fftc4_8,fftc4_un8);
  doitr4(16,fftr4_16,fftr4_un16);
  doitc4(16,fftc4_16,fftc4_un16);
  doitr4(32,fftr4_32,fftr4_un32);
  doitc4(32,fftc4_32,fftc4_un32);
  doitr4(64,fftr4_64,fftr4_un64);
  doitc4(64,fftc4_64,fftc4_un64);
  doitr4(128,fftr4_128,fftr4_un128);
  doitc4(128,fftc4_128,fftc4_un128);
  doitr4(256,fftr4_256,fftr4_un256);
  doitc4(256,fftc4_256,fftc4_un256);
  doitr4(512,fftr4_512,fftr4_un512);
  doitc4(512,fftc4_512,fftc4_un512);
  doitr4(1024,fftr4_1024,fftr4_un1024);
  doitc4(1024,fftc4_1024,fftc4_un1024);
  doitr4(2048,fftr4_2048,fftr4_un2048);
  doitc4(2048,fftc4_2048,fftc4_un2048);
  doitr4(4096,fftr4_4096,fftr4_un4096);
  doitc4(4096,fftc4_4096,fftc4_un4096);
  doitr4(8192,fftr4_8192,fftr4_un8192);
  doitc4(8192,fftc4_8192,fftc4_un8192);
  
  doitr8(2,fftr8_2,fftr8_un2);
  doitc8(2,fftc8_2,fftc8_un2);
  doitr8(4,fftr8_4,fftr8_un4);
  doitc8(4,fftc8_4,fftc8_un4);
  doitr8(8,fftr8_8,fftr8_un8);
  doitc8(8,fftc8_8,fftc8_un8);
  doitr8(16,fftr8_16,fftr8_un16);
  doitc8(16,fftc8_16,fftc8_un16);
  doitr8(32,fftr8_32,fftr8_un32);
  doitc8(32,fftc8_32,fftc8_un32);
  doitr8(64,fftr8_64,fftr8_un64);
  doitc8(64,fftc8_64,fftc8_un64);
  doitr8(128,fftr8_128,fftr8_un128);
  doitc8(128,fftc8_128,fftc8_un128);
  doitr8(256,fftr8_256,fftr8_un256);
  doitc8(256,fftc8_256,fftc8_un256);
  doitr8(512,fftr8_512,fftr8_un512);
  doitc8(512,fftc8_512,fftc8_un512);
  doitr8(1024,fftr8_1024,fftr8_un1024);
  doitc8(1024,fftc8_1024,fftc8_un1024);
  doitr8(2048,fftr8_2048,fftr8_un2048);
  doitc8(2048,fftc8_2048,fftc8_un2048);
  doitr8(4096,fftr8_4096,fftr8_un4096);
  doitc8(4096,fftc8_4096,fftc8_un4096);
  doitr8(8192,fftr8_8192,fftr8_un8192);
  doitc8(8192,fftc8_8192,fftc8_un8192);

  exit(0);
}
