#ifndef __TL__SIZE_T_H
#define	__TL__SIZE_T_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Hmmm, this needs some more thought...
size_t is "unsigned int" for built-in GCC 2.x functions,
"unsigned long" for GCC 3.x. For 16-bit code, use "unsigned long"
for HUGE memory model, "unsigned int" for others */
//typedef unsigned size_t;
typedef unsigned long size_t;

#ifdef __cplusplus
}
#endif

#endif
