#include "auto_home.h"

void hier()
{
  h(auto_home,-1,-1,02755);

  d(auto_home,"lib",-1,-1,02755);
  c(auto_home,"lib","djbfft.a",-1,-1,0644);

  d(auto_home,"include",-1,-1,02755);
  c(auto_home,"include","real4.h",-1,-1,0644);
  c(auto_home,"include","real8.h",-1,-1,0644);
  c(auto_home,"include","complex4.h",-1,-1,0644);
  c(auto_home,"include","complex8.h",-1,-1,0644);
  c(auto_home,"include","fftc4.h",-1,-1,0644);
  c(auto_home,"include","fftc8.h",-1,-1,0644);
  c(auto_home,"include","fftr4.h",-1,-1,0644);
  c(auto_home,"include","fftr8.h",-1,-1,0644);
  c(auto_home,"include","fftfreq.h",-1,-1,0644);
}
