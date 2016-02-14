#ifndef FFTC8_H
#define FFTC8_H

#include "complex8.h"

extern const char fftc8_opt[];

extern void fftc8_2(complex8 *);
extern void fftc8_4(complex8 *);
extern void fftc8_8(complex8 *);
extern void fftc8_16(complex8 *);
extern void fftc8_32(complex8 *);
extern void fftc8_64(complex8 *);
extern void fftc8_128(complex8 *);
extern void fftc8_256(complex8 *);
extern void fftc8_512(complex8 *);
extern void fftc8_1024(complex8 *);
extern void fftc8_2048(complex8 *);
extern void fftc8_4096(complex8 *);
extern void fftc8_8192(complex8 *);

#define fftc8_un2 fftc8_2
extern void fftc8_un4(complex8 *);
extern void fftc8_un8(complex8 *);
extern void fftc8_un16(complex8 *);
extern void fftc8_un32(complex8 *);
extern void fftc8_un64(complex8 *);
extern void fftc8_un128(complex8 *);
extern void fftc8_un256(complex8 *);
extern void fftc8_un512(complex8 *);
extern void fftc8_un1024(complex8 *);
extern void fftc8_un2048(complex8 *);
extern void fftc8_un4096(complex8 *);
extern void fftc8_un8192(complex8 *);

extern void fftc8_mul2(complex8 *,complex8 *);
extern void fftc8_mul4(complex8 *,complex8 *);
extern void fftc8_mul8(complex8 *,complex8 *);
extern void fftc8_mul16(complex8 *,complex8 *);
extern void fftc8_mul32(complex8 *,complex8 *);
extern void fftc8_mul64(complex8 *,complex8 *);
extern void fftc8_mul128(complex8 *,complex8 *);
extern void fftc8_mul256(complex8 *,complex8 *);
extern void fftc8_mul512(complex8 *,complex8 *);
extern void fftc8_mul1024(complex8 *,complex8 *);
extern void fftc8_mul2048(complex8 *,complex8 *);
extern void fftc8_mul4096(complex8 *,complex8 *);
extern void fftc8_mul8192(complex8 *,complex8 *);

extern void fftc8_scale2(complex8 *);
extern void fftc8_scale4(complex8 *);
extern void fftc8_scale8(complex8 *);
extern void fftc8_scale16(complex8 *);
extern void fftc8_scale32(complex8 *);
extern void fftc8_scale64(complex8 *);
extern void fftc8_scale128(complex8 *);
extern void fftc8_scale256(complex8 *);
extern void fftc8_scale512(complex8 *);
extern void fftc8_scale1024(complex8 *);
extern void fftc8_scale2048(complex8 *);
extern void fftc8_scale4096(complex8 *);
extern void fftc8_scale8192(complex8 *);

#endif
