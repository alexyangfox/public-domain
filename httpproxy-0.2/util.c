#include <stdlib.h>
#include <ctype.h>
#include "util.h"

void dump_buffer (FILE *fp, char *heading,
		  const unsigned char *buffer, int bufsize)
{
    int i, j;
    fprintf(fp, "%s (%d bytes):", heading, bufsize);
    for ( i = 0; i < bufsize; i += 16 ) {
	fprintf(fp, "\n   %04x:", i );
	for ( j = i; j < i + 16; j++ )
	    if (j < bufsize)
		fprintf(fp, " %02x", 255&buffer[j] );
	    else
		fprintf(fp, "   " );
	fprintf(fp, "  ");
	for ( j = i; j < ( i + 16 < bufsize ? i + 16 : bufsize ); j++ )
	    fprintf(fp, "%c", isprint(buffer[j]) ? buffer[j] : '.' );
    }
    fprintf(fp, "\n");
}

