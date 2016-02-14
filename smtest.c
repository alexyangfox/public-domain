/*

	SMARTALLOC test program.

	This  may  be  built  with  SMARTALLOC	defined or not defined
	(usually on the compiler call line).  It  should  compile  and
	link  without errors in either mode.  If built with SMARTALLOC
	defined it should produce the following output:

	   Orphaned buffer:	120 bytes allocated at line 53 of smtest.c
	   Orphaned buffer:	200 bytes allocated at line 54 of smtest.c
	   Orphaned buffer:	 40 bytes allocated at line 55 of smtest.c
	   Orphaned buffer:	 55 bytes allocated at line 62 of smtest.c
	   No orphaned buffer messages should follow this line.

	To dump the orphaned buffers, define DUMPBUF at compile time.

	If built without SMARTALLOC, no output should be generated.

	Designed and implemented by John Walker in September 1989.

*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "smartall.h"
#include "alloc.h"

static char *bchain = NULL;

#define ec(x) cp = (x); assert(cp != NULL); \
	*((char **) cp) = bchain; bchain = cp
#define dc()  cp = bchain; bchain = *((char **) cp);  free(cp)

#ifdef DUMPBUF
#define Dumparam 1
#else
#define Dumparam 0
#endif

int main()
{
	char *cp, *ra;

#ifdef OLD_unix
	malloc_debug(2);
#endif

        /* Allocate and chain together storage that's subject to
	   the orphaned buffer check. */

	ec(malloc(120));
	ec(alloc(200));
	ec(calloc(10, 4));

	ra = alloc(60);
        strcpy(ra + 8, "Hello, there.  This is data.");
	ra = realloc(ra, 100);
	ra = realloc(ra, 100);
	ra = realloc(ra, 2048);
	ec(realloc(ra, 55));

	/* Allocate and chain some storage for which checking is
	   disabled by the sm_static mechanism. */

	sm_static(1);
	ra = malloc(10);
	ra = alloc(20);
	ra = calloc(30, sizeof(short));
	ra = realloc(ra, 40);
	sm_static(0);

        /* Test the "actually" variants. */

	ra = actuallymalloc(100);
	ra = actuallycalloc(10, sizeof(double));
	ra = actuallyrealloc(ra, 15 * sizeof(double));
	actuallyfree(ra);

	/* Produce orphan buffer listing. */

	sm_dump(Dumparam);

	/* Release chained buffers. */

	while (bchain != NULL) {
	   dc();
	}

	/* Now verify that all have been released. */

#ifdef SMARTALLOC
	fprintf(stderr,
           "No orphaned buffer messages should follow this line.\n");
#endif
	sm_dump(1);
	return 0;
}
