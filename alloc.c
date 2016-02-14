/*

        Error checking memory allocator

*/

#include <stdio.h>
#include <stdlib.h>

#ifdef TESTERR
#undef NULL
#define NULL  buf
#endif

/*LINTLIBRARY*/

#define V        (void)

#ifdef SMARTALLOC

extern void *sm_malloc();

/*  SM_ALLOC  --  Allocate buffer and signal on error  */

void *sm_alloc(fname, lineno, nbytes)
  char *fname;
  int lineno;
  unsigned nbytes;
{
        void *buf;

        if ((buf = sm_malloc(fname, lineno, nbytes)) != NULL) {
           return buf;
        }
        V fprintf(stderr, "\nBoom!!!  Memory capacity exceeded.\n");
        V fprintf(stderr, "  Requested %u bytes at line %d of %s.\n",
           nbytes, lineno, fname);
        abort();
        /*NOTREACHED*/
}
#else

/*  ALLOC  --  Allocate buffer and signal on error  */

void *alloc(nbytes)
  unsigned nbytes;
{
        void *buf;

        if ((buf = malloc(nbytes)) != NULL) {
           return buf;
        }
        V fprintf(stderr, "\nBoom!!!  Memory capacity exceeded.\n");
        V fprintf(stderr, "  Requested %u bytes.\n", nbytes);
        abort();
        /*NOTREACHED*/
}
#endif
