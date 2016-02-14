#ifndef PRE_C
#define PRE_C

#include "fftc4.h"
#include "fftr4.h"

#define SMALL
#define optid fftc4_opt

#define real real4
#define complex complex4

#define mulc fftc4_mul
#define mulc2 fftc4_mul2
#define mulc4 fftc4_mul4
#define mulc8 fftc4_mul8
#define mulc16 fftc4_mul16
#define mulc32 fftc4_mul32
#define mulc64 fftc4_mul64
#define mulc128 fftc4_mul128
#define mulc256 fftc4_mul256
#define mulc512 fftc4_mul512
#define mulc1024 fftc4_mul1024
#define mulc2048 fftc4_mul2048
#define mulc4096 fftc4_mul4096
#define mulc8192 fftc4_mul8192

#define mulr fftr4_mul
#define mulr2 fftr4_mul2
#define mulr4 fftr4_mul4
#define mulr8 fftr4_mul8
#define mulr16 fftr4_mul16
#define mulr32 fftr4_mul32
#define mulr64 fftr4_mul64
#define mulr128 fftr4_mul128
#define mulr256 fftr4_mul256
#define mulr512 fftr4_mul512
#define mulr1024 fftr4_mul1024
#define mulr2048 fftr4_mul2048
#define mulr4096 fftr4_mul4096
#define mulr8192 fftr4_mul8192

#define scalec fftc4_scale
#define scalec2 fftc4_scale2
#define scalec4 fftc4_scale4
#define scalec8 fftc4_scale8
#define scalec16 fftc4_scale16
#define scalec32 fftc4_scale32
#define scalec64 fftc4_scale64
#define scalec128 fftc4_scale128
#define scalec256 fftc4_scale256
#define scalec512 fftc4_scale512
#define scalec1024 fftc4_scale1024
#define scalec2048 fftc4_scale2048
#define scalec4096 fftc4_scale4096
#define scalec8192 fftc4_scale8192

#define scaler fftr4_scale
#define scaler2 fftr4_scale2
#define scaler4 fftr4_scale4
#define scaler8 fftr4_scale8
#define scaler16 fftr4_scale16
#define scaler32 fftr4_scale32
#define scaler64 fftr4_scale64
#define scaler128 fftr4_scale128
#define scaler256 fftr4_scale256
#define scaler512 fftr4_scale512
#define scaler1024 fftr4_scale1024
#define scaler2048 fftr4_scale2048
#define scaler4096 fftr4_scale4096
#define scaler8192 fftr4_scale8192

#define cpass fftc4_pass
#define cpassbig fftc4_passbig
#define c2 fftc4_2
#define c4 fftc4_4
#define c8 fftc4_8
#define c16 fftc4_16
#define c32 fftc4_32
#define c64 fftc4_64
#define c128 fftc4_128
#define c256 fftc4_256
#define c512 fftc4_512
#define c1024 fftc4_1024
#define c2048 fftc4_2048
#define c4096 fftc4_4096
#define c8192 fftc4_8192

#define upass fftc4_unpass
#define upassbig fftc4_unpassbig
#define u2 fftc4_un2
#define u4 fftc4_un4
#define u8 fftc4_un8
#define u16 fftc4_un16
#define u32 fftc4_un32
#define u64 fftc4_un64
#define u128 fftc4_un128
#define u256 fftc4_un256
#define u512 fftc4_un512
#define u1024 fftc4_un1024
#define u2048 fftc4_un2048
#define u4096 fftc4_un4096
#define u8192 fftc4_un8192

#define rpass fftr4_pass
#define rpassbig fftr4_passbig
#define r2 fftr4_2
#define r4 fftr4_4
#define r8 fftr4_8
#define r16 fftr4_16
#define r32 fftr4_32
#define r64 fftr4_64
#define r128 fftr4_128
#define r256 fftr4_256
#define r512 fftr4_512
#define r1024 fftr4_1024
#define r2048 fftr4_2048
#define r4096 fftr4_4096
#define r8192 fftr4_8192

#define vpass fftr4_unpass
#define vpassbig fftr4_unpassbig
#define v2 fftr4_un2
#define v4 fftr4_un4
#define v8 fftr4_un8
#define v16 fftr4_un16
#define v32 fftr4_un32
#define v64 fftr4_un64
#define v128 fftr4_un128
#define v256 fftr4_un256
#define v512 fftr4_un512
#define v1024 fftr4_un1024
#define v2048 fftr4_un2048
#define v4096 fftr4_un4096
#define v8192 fftr4_un8192

#define d16 fftc4_roots16
#define d32 fftc4_roots32
#define d64 fftc4_roots64
#define d128 fftc4_roots128
#define d256 fftc4_roots256
#define d512 fftc4_roots512
#define d1024 fftc4_roots1024
#define d2048 fftc4_roots2048
#define d4096 fftc4_roots4096
#define d8192 fftc4_roots8192

#include "4i.c"

#endif
