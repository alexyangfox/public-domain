#ifndef PRE_C
#define PRE_C

#include "fftc8.h"
#include "fftr8.h"

#define optid fftc8_opt

#define real real8
#define complex complex8

#define mulc fftc8_mul
#define mulc2 fftc8_mul2
#define mulc4 fftc8_mul4
#define mulc8 fftc8_mul8
#define mulc16 fftc8_mul16
#define mulc32 fftc8_mul32
#define mulc64 fftc8_mul64
#define mulc128 fftc8_mul128
#define mulc256 fftc8_mul256
#define mulc512 fftc8_mul512
#define mulc1024 fftc8_mul1024
#define mulc2048 fftc8_mul2048
#define mulc4096 fftc8_mul4096
#define mulc8192 fftc8_mul8192

#define mulr fftr8_mul
#define mulr2 fftr8_mul2
#define mulr4 fftr8_mul4
#define mulr8 fftr8_mul8
#define mulr16 fftr8_mul16
#define mulr32 fftr8_mul32
#define mulr64 fftr8_mul64
#define mulr128 fftr8_mul128
#define mulr256 fftr8_mul256
#define mulr512 fftr8_mul512
#define mulr1024 fftr8_mul1024
#define mulr2048 fftr8_mul2048
#define mulr4096 fftr8_mul4096
#define mulr8192 fftr8_mul8192

#define scalec fftc8_scale
#define scalec2 fftc8_scale2
#define scalec4 fftc8_scale4
#define scalec8 fftc8_scale8
#define scalec16 fftc8_scale16
#define scalec32 fftc8_scale32
#define scalec64 fftc8_scale64
#define scalec128 fftc8_scale128
#define scalec256 fftc8_scale256
#define scalec512 fftc8_scale512
#define scalec1024 fftc8_scale1024
#define scalec2048 fftc8_scale2048
#define scalec4096 fftc8_scale4096
#define scalec8192 fftc8_scale8192

#define scaler fftr8_scale
#define scaler2 fftr8_scale2
#define scaler4 fftr8_scale4
#define scaler8 fftr8_scale8
#define scaler16 fftr8_scale16
#define scaler32 fftr8_scale32
#define scaler64 fftr8_scale64
#define scaler128 fftr8_scale128
#define scaler256 fftr8_scale256
#define scaler512 fftr8_scale512
#define scaler1024 fftr8_scale1024
#define scaler2048 fftr8_scale2048
#define scaler4096 fftr8_scale4096
#define scaler8192 fftr8_scale8192

#define cpass fftc8_pass
#define cpassbig fftc8_passbig
#define c2 fftc8_2
#define c4 fftc8_4
#define c8 fftc8_8
#define c16 fftc8_16
#define c32 fftc8_32
#define c64 fftc8_64
#define c128 fftc8_128
#define c256 fftc8_256
#define c512 fftc8_512
#define c1024 fftc8_1024
#define c2048 fftc8_2048
#define c4096 fftc8_4096
#define c8192 fftc8_8192

#define upass fftc8_unpass
#define upassbig fftc8_unpassbig
#define u2 fftc8_un2
#define u4 fftc8_un4
#define u8 fftc8_un8
#define u16 fftc8_un16
#define u32 fftc8_un32
#define u64 fftc8_un64
#define u128 fftc8_un128
#define u256 fftc8_un256
#define u512 fftc8_un512
#define u1024 fftc8_un1024
#define u2048 fftc8_un2048
#define u4096 fftc8_un4096
#define u8192 fftc8_un8192

#define rpass fftr8_pass
#define rpassbig fftr8_passbig
#define r2 fftr8_2
#define r4 fftr8_4
#define r8 fftr8_8
#define r16 fftr8_16
#define r32 fftr8_32
#define r64 fftr8_64
#define r128 fftr8_128
#define r256 fftr8_256
#define r512 fftr8_512
#define r1024 fftr8_1024
#define r2048 fftr8_2048
#define r4096 fftr8_4096
#define r8192 fftr8_8192

#define vpass fftr8_unpass
#define vpassbig fftr8_unpassbig
#define v2 fftr8_un2
#define v4 fftr8_un4
#define v8 fftr8_un8
#define v16 fftr8_un16
#define v32 fftr8_un32
#define v64 fftr8_un64
#define v128 fftr8_un128
#define v256 fftr8_un256
#define v512 fftr8_un512
#define v1024 fftr8_un1024
#define v2048 fftr8_un2048
#define v4096 fftr8_un4096
#define v8192 fftr8_un8192

#define d16 fftc8_roots16
#define d32 fftc8_roots32
#define d64 fftc8_roots64
#define d128 fftc8_roots128
#define d256 fftc8_roots256
#define d512 fftc8_roots512
#define d1024 fftc8_roots1024
#define d2048 fftc8_roots2048
#define d4096 fftc8_roots4096
#define d8192 fftc8_roots8192

#include "8i.c"

#endif
