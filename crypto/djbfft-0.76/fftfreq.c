#include "fftfreq.h"

unsigned int fftfreq_c(unsigned int i,unsigned int n)
{
  unsigned int m;

  if (n <= 2) return i;

  m = n >> 1;
  if (i < m) return fftfreq_c(i,m) << 1;

  i -= m;
  m >>= 1;
  if (i < m) return (fftfreq_c(i,m) << 2) + 1;
  i -= m;
  return ((fftfreq_c(i,m) << 2) - 1) & (n - 1);
}

static void doit(unsigned int *f,unsigned int n,unsigned int scale,unsigned int offset)
{
  if (n <= 1) {
    f[0] = offset;
    return;
  }
  if (n == 2) {
    f[0] = offset;
    f[1] = scale + offset;
    return;
  }
  n >>= 1;
  doit(f,n,scale << 1,offset);

  f += n;
  n >>= 1;
  doit(f,n,scale << 2,offset + scale);
  doit(f + n,n,scale << 2,offset - scale);

}

void fftfreq_ctable(unsigned int *f,unsigned int n)
{
  int i;

  doit(f,n,1,0);
  for (i = 0;i < n;++i)
    f[i] &= n - 1;
}

unsigned int fftfreq_r(unsigned int i,unsigned int n)
{
  unsigned int m;

  if (n <= 2) return i;

  m = n >> 1;
  if (i < m) return fftfreq_r(i,m) << 1;

  i -= m;
  i >>= 1;
  m >>= 1;
  return (fftfreq_c(i,m) << 2) + 1;
}

void fftfreq_rtable(unsigned int *f,unsigned int n)
{
  int i;

  for (i = 0;i < n;++i)
    f[i] = fftfreq_r(i,n);
}
