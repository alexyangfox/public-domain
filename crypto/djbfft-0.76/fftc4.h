#ifndef FFTC4_H
#define FFTC4_H

#include "complex4.h"

extern const char fftc4_opt[];

extern void fftc4_2(complex4 *);
extern void fftc4_4(complex4 *);
extern void fftc4_8(complex4 *);
extern void fftc4_16(complex4 *);
extern void fftc4_32(complex4 *);
extern void fftc4_64(complex4 *);
extern void fftc4_128(complex4 *);
extern void fftc4_256(complex4 *);
extern void fftc4_512(complex4 *);
extern void fftc4_1024(complex4 *);
extern void fftc4_2048(complex4 *);
extern void fftc4_4096(complex4 *);
extern void fftc4_8192(complex4 *);

#define fftc4_un2 fftc4_2
extern void fftc4_un4(complex4 *);
extern void fftc4_un8(complex4 *);
extern void fftc4_un16(complex4 *);
extern void fftc4_un32(complex4 *);
extern void fftc4_un64(complex4 *);
extern void fftc4_un128(complex4 *);
extern void fftc4_un256(complex4 *);
extern void fftc4_un512(complex4 *);
extern void fftc4_un1024(complex4 *);
extern void fftc4_un2048(complex4 *);
extern void fftc4_un4096(complex4 *);
extern void fftc4_un8192(complex4 *);

extern void fftc4_mul2(complex4 *,complex4 *);
extern void fftc4_mul4(complex4 *,complex4 *);
extern void fftc4_mul8(complex4 *,complex4 *);
extern void fftc4_mul16(complex4 *,complex4 *);
extern void fftc4_mul32(complex4 *,complex4 *);
extern void fftc4_mul64(complex4 *,complex4 *);
extern void fftc4_mul128(complex4 *,complex4 *);
extern void fftc4_mul256(complex4 *,complex4 *);
extern void fftc4_mul512(complex4 *,complex4 *);
extern void fftc4_mul1024(complex4 *,complex4 *);
extern void fftc4_mul2048(complex4 *,complex4 *);
extern void fftc4_mul4096(complex4 *,complex4 *);
extern void fftc4_mul8192(complex4 *,complex4 *);

extern void fftc4_scale2(complex4 *);
extern void fftc4_scale4(complex4 *);
extern void fftc4_scale8(complex4 *);
extern void fftc4_scale16(complex4 *);
extern void fftc4_scale32(complex4 *);
extern void fftc4_scale64(complex4 *);
extern void fftc4_scale128(complex4 *);
extern void fftc4_scale256(complex4 *);
extern void fftc4_scale512(complex4 *);
extern void fftc4_scale1024(complex4 *);
extern void fftc4_scale2048(complex4 *);
extern void fftc4_scale4096(complex4 *);
extern void fftc4_scale8192(complex4 *);

#endif
