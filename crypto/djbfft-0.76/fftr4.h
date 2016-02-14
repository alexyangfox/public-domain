#ifndef FFTR4_H
#define FFTR4_H

#include "real4.h"

extern const char fftr4_opt[];

extern void fftr4_2(real4 *);
extern void fftr4_4(real4 *);
extern void fftr4_8(real4 *);
extern void fftr4_16(real4 *);
extern void fftr4_32(real4 *);
extern void fftr4_64(real4 *);
extern void fftr4_128(real4 *);
extern void fftr4_256(real4 *);
extern void fftr4_512(real4 *);
extern void fftr4_1024(real4 *);
extern void fftr4_2048(real4 *);
extern void fftr4_4096(real4 *);
extern void fftr4_8192(real4 *);

#define fftr4_un2 fftr4_2
extern void fftr4_un4(real4 *);
extern void fftr4_un8(real4 *);
extern void fftr4_un16(real4 *);
extern void fftr4_un32(real4 *);
extern void fftr4_un64(real4 *);
extern void fftr4_un128(real4 *);
extern void fftr4_un256(real4 *);
extern void fftr4_un512(real4 *);
extern void fftr4_un1024(real4 *);
extern void fftr4_un2048(real4 *);
extern void fftr4_un4096(real4 *);
extern void fftr4_un8192(real4 *);

extern void fftr4_mul2(real4 *,real4 *);
extern void fftr4_mul4(real4 *,real4 *);
extern void fftr4_mul8(real4 *,real4 *);
extern void fftr4_mul16(real4 *,real4 *);
extern void fftr4_mul32(real4 *,real4 *);
extern void fftr4_mul64(real4 *,real4 *);
extern void fftr4_mul128(real4 *,real4 *);
extern void fftr4_mul256(real4 *,real4 *);
extern void fftr4_mul512(real4 *,real4 *);
extern void fftr4_mul1024(real4 *,real4 *);
extern void fftr4_mul2048(real4 *,real4 *);
extern void fftr4_mul4096(real4 *,real4 *);
extern void fftr4_mul8192(real4 *,real4 *);

extern void fftr4_scale2(real4 *);
extern void fftr4_scale4(real4 *);
extern void fftr4_scale8(real4 *);
extern void fftr4_scale16(real4 *);
extern void fftr4_scale32(real4 *);
extern void fftr4_scale64(real4 *);
extern void fftr4_scale128(real4 *);
extern void fftr4_scale256(real4 *);
extern void fftr4_scale512(real4 *);
extern void fftr4_scale1024(real4 *);
extern void fftr4_scale2048(real4 *);
extern void fftr4_scale4096(real4 *);
extern void fftr4_scale8192(real4 *);

#endif
