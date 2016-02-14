#ifndef KERNEL_I386_INIT_H
#define KERNEL_I386_INIT_H

#include <kernel/types.h>







void Init (void);
void InitDebug (void);
void InitI386 (void);
void InitDummyProc (void);
void InitVM (void);
void InitKMalloc (void);
void InitProc (void);
void InitFS (void);
void InitConfigOptions (void);
void InitACPI(void);


void FiniACPI(void);


int32 InitMain (void *arg);
int32 IdleMain (void *arg);



#endif
