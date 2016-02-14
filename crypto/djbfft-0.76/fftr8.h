#ifndef FFTR8_H
#define FFTR8_H

#include "real8.h"

extern const char fftr8_opt[];

extern void fftr8_2(real8 *);
extern void fftr8_4(real8 *);
extern void fftr8_8(real8 *);
extern void fftr8_16(real8 *);
extern void fftr8_32(real8 *);
extern void fftr8_64(real8 *);
extern void fftr8_128(real8 *);
extern void fftr8_256(real8 *);
extern void fftr8_512(real8 *);
extern void fftr8_1024(real8 *);
extern void fftr8_2048(real8 *);
extern void fftr8_4096(real8 *);
extern void fftr8_8192(real8 *);

#define fftr8_un2 fftr8_2
extern void fftr8_un4(real8 *);
extern void fftr8_un8(real8 *);
extern void fftr8_un16(real8 *);
extern void fftr8_un32(real8 *);
extern void fftr8_un64(real8 *);
extern void fftr8_un128(real8 *);
extern void fftr8_un256(real8 *);
extern void fftr8_un512(real8 *);
extern void fftr8_un1024(real8 *);
extern void fftr8_un2048(real8 *);
extern void fftr8_un4096(real8 *);
extern void fftr8_un8192(real8 *);

extern void fftr8_mul2(real8 *,real8 *);
extern void fftr8_mul4(real8 *,real8 *);
extern void fftr8_mul8(real8 *,real8 *);
extern void fftr8_mul16(real8 *,real8 *);
extern void fftr8_mul32(real8 *,real8 *);
extern void fftr8_mul64(real8 *,real8 *);
extern void fftr8_mul128(real8 *,real8 *);
extern void fftr8_mul256(real8 *,real8 *);
extern void fftr8_mul512(real8 *,real8 *);
extern void fftr8_mul1024(real8 *,real8 *);
extern void fftr8_mul2048(real8 *,real8 *);
extern void fftr8_mul4096(real8 *,real8 *);
extern void fftr8_mul8192(real8 *,real8 *);

extern void fftr8_scale2(real8 *);
extern void fftr8_scale4(real8 *);
extern void fftr8_scale8(real8 *);
extern void fftr8_scale16(real8 *);
extern void fftr8_scale32(real8 *);
extern void fftr8_scale64(real8 *);
extern void fftr8_scale128(real8 *);
extern void fftr8_scale256(real8 *);
extern void fftr8_scale512(real8 *);
extern void fftr8_scale1024(real8 *);
extern void fftr8_scale2048(real8 *);
extern void fftr8_scale4096(real8 *);
extern void fftr8_scale8192(real8 *);

#endif
