/*

        Definitions for error-checking memory allocator

*/

#ifdef SMARTALLOC

extern void *sm_alloc();
#define alloc(x)       sm_alloc(__FILE__, __LINE__, (x))

#else

extern void *alloc();

#endif
