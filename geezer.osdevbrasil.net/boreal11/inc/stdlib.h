#ifndef __TL_STDLIB_H
#define	__TL_STDLIB_H

#ifdef __cplusplus
extern "C"
{
#endif

unsigned rand(void);
void srand(unsigned new_seed);
void exit(int exit_code);

#ifdef __cplusplus
}
#endif

#endif
