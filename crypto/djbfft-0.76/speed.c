#include <stdio.h>
#include "fftr4.h"
#include "fftr8.h"
#include "fftc4.h"
#include "fftc8.h"
#include "timing.h"

void nomem()
{
  fprintf(stderr,"speed: fatal: out of memory\n");
  exit(111);
}

complex4 *x4;
complex8 *x8;
real4 *y4;
real8 *y8;

timing start;
timing_basic startb;
timing finish;
timing_basic finishb;

#define TIMINGS 8
#define SIZES 13

timing t[2][TIMINGS];

#define TIME(result,what) { \
  for (i = 0;i < TIMINGS;++i) { \
    timing_now(&t[0][i]); \
    what; \
    timing_now(&t[1][i]); \
  } \
  for (i = 0;i < TIMINGS;++i) \
    result[i] = timing_diff(&t[1][i],&t[0][i]); \
  }

#define TIMESIZES(result,lib,args) { \
  TIME(result[0],lib##2 args) \
  TIME(result[1],lib##4 args) \
  TIME(result[2],lib##8 args) \
  TIME(result[3],lib##16 args) \
  TIME(result[4],lib##32 args) \
  TIME(result[5],lib##64 args) \
  TIME(result[6],lib##128 args) \
  TIME(result[7],lib##256 args) \
  TIME(result[8],lib##512 args) \
  TIME(result[9],lib##1024 args) \
  TIME(result[10],lib##2048 args) \
  TIME(result[11],lib##4096 args) \
  TIME(result[12],lib##8192 args) \
  }

double tnothing[TIMINGS];
double tplus[2][2][SIZES][TIMINGS];
double tminus[2][2][SIZES][TIMINGS];
double tscale[2][2][SIZES][TIMINGS];
double tmul[2][2][SIZES][TIMINGS];

int size[SIZES] =
{ 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192 };

void doc4()
{
  int i;
  TIMESIZES(tplus[1][0],fftc4_,(x4))
  TIMESIZES(tscale[1][0],fftc4_scale,(x4))
  TIMESIZES(tmul[1][0],fftc4_mul,(x4,x4))
  TIMESIZES(tminus[1][0],fftc4_un,(x4))
}

void doc8()
{
  int i;
  TIMESIZES(tplus[1][1],fftc8_,(x8))
  TIMESIZES(tscale[1][1],fftc8_scale,(x8))
  TIMESIZES(tmul[1][1],fftc8_mul,(x8,x8))
  TIMESIZES(tminus[1][1],fftc8_un,(x8))
}

void dor4()
{
  int i;
  TIMESIZES(tplus[0][0],fftr4_,(y4))
  TIMESIZES(tscale[0][0],fftr4_scale,(y4))
  TIMESIZES(tmul[0][0],fftr4_mul,(y4,y4))
  TIMESIZES(tminus[0][0],fftr4_un,(y4))
}

void dor8()
{
  int i;
  TIMESIZES(tplus[0][1],fftr8_,(y8))
  TIMESIZES(tscale[0][1],fftr8_scale,(y8))
  TIMESIZES(tmul[0][1],fftr8_mul,(y8,y8))
  TIMESIZES(tminus[0][1],fftr8_un,(y8))
}

extern complex4 fftc4_roots16[];
extern complex8 fftc8_roots16[];

main()
{
  int i;
  int j;
  int flag8;
  int flagc;

  x4 = (complex4 *) malloc(16384 + 8192 * sizeof(complex4));
  if (!x4) nomem();
  x8 = (complex8 *) malloc(16384 + 8192 * sizeof(complex8));
  if (!x8) nomem();
  y4 = (real4 *) malloc(16384 + 8192 * sizeof(real4));
  if (!y4) nomem();
  y8 = (real8 *) malloc(16384 + 8192 * sizeof(real8));
  if (!y8) nomem();

  x4 += ((10240 + (char *) fftc4_roots16 - (char *) x4) & 16383) / sizeof(complex4);
  x8 += ((10240 + (char *) fftc8_roots16 - (char *) x8) & 16383) / sizeof(complex8);
  y4 += ((10240 + (char *) fftc4_roots16 - (char *) y4) & 16383) / sizeof(real4);
  y8 += ((10240 + (char *) fftc8_roots16 - (char *) y8) & 16383) / sizeof(real8);

  for (i = 0;i < 8192;++i) x4[i].re = x4[i].im = 0;
  for (i = 0;i < 8192;++i) x8[i].re = x8[i].im = 0;
  for (i = 0;i < 8192;++i) y4[i] = 0;
  for (i = 0;i < 8192;++i) y8[i] = 0;

  timing_basic_now(&startb);
  timing_now(&start);

  TIME(tnothing,)

  doc4();
  doc8();
  dor4();
  dor8();

  timing_basic_now(&finishb);
  timing_now(&finish);

  printf("Using");
#ifdef HASRDTSC
  printf(" RDTSC,");
#else
#ifdef HASGETHRTIME
  printf(" gethrtime(),");
#endif
#endif
  printf(" %s/*.c.\n",fftc8_opt);

  printf("   nothing");
  for (i = 0;i < TIMINGS;++i)
    printf(" %7.0f",tnothing[i]);
  printf("\n");

  for (j = 0;j < SIZES;++j)
    for (flagc = 0;flagc < 2;++flagc)
      for (flag8 = 0;flag8 < 2;++flag8) {
        printf("%6d %c%c+",size[j],"rc"[flagc],"48"[flag8]);
        for (i = 0;i < TIMINGS;++i)
          printf(" %7.0f",tplus[flagc][flag8][j][i]);
        printf("\n");
        printf("%6d %c%c-",size[j],"rc"[flagc],"48"[flag8]);
        for (i = 0;i < TIMINGS;++i)
          printf(" %7.0f",tminus[flagc][flag8][j][i]);
        printf("\n");
      }
  for (j = 0;j < SIZES;++j)
    for (flagc = 0;flagc < 2;++flagc)
      for (flag8 = 0;flag8 < 2;++flag8) {
        printf("%6d %c%cs",size[j],"rc"[flagc],"48"[flag8]);
        for (i = 0;i < TIMINGS;++i)
          printf(" %7.0f",tscale[flagc][flag8][j][i]);
        printf("\n");
      }
  for (j = 0;j < SIZES;++j)
    for (flagc = 0;flagc < 2;++flagc)
      for (flag8 = 0;flag8 < 2;++flag8) {
        printf("%6d %c%cm",size[j],"rc"[flagc],"48"[flag8]);
        for (i = 0;i < TIMINGS;++i)
          printf(" %7.0f",tmul[flagc][flag8][j][i]);
        printf("\n");
      }

  printf("Timings are in ticks. Nanoseconds per tick: approximately %f.\n"
    ,timing_basic_diff(&finishb,&startb) / timing_diff(&finish,&start));
  printf("Timings may be underestimates on systems without hardware tick support.\n");

  exit(0);
}
