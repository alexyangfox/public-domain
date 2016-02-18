#ifndef __TL_SYSTEM_H
#define	__TL_SYSTEM_H

#ifdef __cplusplus
extern "C"
{
#endif

unsigned char inportb(unsigned port);
void outportb(unsigned port, unsigned val);
unsigned disable(void);
unsigned enable(void);
void restore_flags(unsigned flags);

#ifdef __cplusplus
}
#endif

#endif

