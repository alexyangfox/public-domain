#ifndef __TL__NULL_H
#define	__TL__NULL_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Someone else's kernel (which will not be named here :)
used "#define NULL (void *)0", which is WRONG!!!
(Hint: try compiling C++ code that uses that definition of NULL.) */
#define	NULL	0

#ifdef __cplusplus
}
#endif

#endif
