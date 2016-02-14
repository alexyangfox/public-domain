/*

		    Extended dump and load utility

			    by John Walker
		       http://www.fourmilab.ch/

		This program is in the public domain.

*/

#define Version \
                      "1.3  --  September 2000"


#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#define FALSE	0
#define TRUE	1

#define EOS     '\0'

static char addrformat[80] = "%6X";
static char scanaddr[80] = "%lx%c";

static char dataformat1[80] = "%02X";
static char scandata[80] = "%x%n%c";

static int bytesperline = 16, doublechar = FALSE,
	   dflen = 2, loading = FALSE, streaming = FALSE;
static unsigned long fileaddr;
static unsigned char lineecho[32];

/*  OUTLINE  --  Edit a line of binary data into the selected output
		 format.  */

static void outline(out, dat, len)
  FILE *out;
  unsigned char *dat;
  int len;
{
    char oline[132];
    int i;

    sprintf(oline, addrformat, fileaddr);
    strcat(oline, ":");
    for (i = 0; i < len; i++) {
	char outedit[80];

	sprintf(outedit, dataformat1, dat[i]);
        strcat(oline, (i == (bytesperline / 2)) ? "  " : " ");
	strcat(oline, outedit);
    }

    if (doublechar) {
	char oc[2];
	int shortfall = ((bytesperline - len) * (dflen + 1)) +
			(len <= (bytesperline / 2) ? 1 : 0);

	while (shortfall-- > 0) {
            strcat(oline, " ");
	}
	oc[1] = EOS;
        strcat(oline, " | ");
	for (i = 0; i < len; i++) {
	    int b = dat[i];

            /* Map non-printing characters to "." according to the
	       definitions for ISO 8859/1 Latin-1. */

            if ((b < ' ') || (b > '~' && b < 160)) {
                b = '.';
	    }

	    /* Many existing systems which support Latin-1 lack
	       a definition for character 160, the non-breaking
	       space.  Translate this to a space to avoid
	       confusion. */

	    if (b == 160) {
                b = ' ';
	    }
	    oc[0] = (char) b;
	    strcat(oline, oc);
	}
    }
    strcat(oline, "\n");
    fputs(oline, out);
}

/*  INTERPLINE	--  Interpret a line of input.	*/

static int interpline(line, lineno, out)
  char *line;
  int lineno;
  FILE *out;
{
    char *cp = line;
    char c;
    unsigned long lfaddr;
    int gfaddr = FALSE;

    /* Scan the line for a possible alternative format information
       following a vertical bar and delete it. */

    while ((c = *cp++) != EOS) {
        if (c == '|') {
	    cp[-1] = EOS;
	    break;
	}
    }

    /* Scan the line for a file address terminated by a colon.	Save
       the file address. */

    cp = line;

    while ((c = *cp++) != EOS) {
        if (c == ':') {
	    int sa;
	    char tchar;

	    cp[-1] = EOS;
	    sa = sscanf(line, scanaddr, &lfaddr, &tchar);
	    if (sa == 0 || (sa > 1 && tchar != EOS)) {
		fprintf(stderr,
                    "Bad file address \"%s\" on line %d:\n",
		    line, lineno);
		return FALSE;
	    }
	    gfaddr = TRUE;
            cp[-1] = ':';
	    break;
	}
    }
    if (!gfaddr) {
	cp = line;
    }
    if (!streaming) {
	if (!gfaddr) {
            fprintf(stderr, "File address missing on line %d:\n", lineno);
            fprintf(stderr, "%s\n", line);
	    return FALSE;
	}
	if (lfaddr != fileaddr) {
            fprintf(stderr, "File address sequence error on line %d.\n",
		lineno);
            fprintf(stderr, "  Expected ");
	    fprintf(stderr, addrformat, fileaddr);
            fprintf(stderr, ", received ");
	    fprintf(stderr, addrformat, lfaddr);
            fprintf(stderr, ".\n");
            fprintf(stderr, "%s\n", line);
	    return FALSE;
	}
    }

    while ((c = *cp++) != EOS) {
	if (!isspace(c)) {
	    int scanl, nscan, dvalue;
	    char termchar;

	    if (((scanl = sscanf(cp - 1, scandata, &dvalue, &nscan, &termchar)) == 0) ||
		(dvalue < 0) || (dvalue > 255) || 
		(scanl == 2 && !isspace(termchar))) {
                fprintf(stderr, "Improper value, \"%s\" on line %d:\n",
		    cp - 1, lineno);
                fprintf(stderr, "%s\n", line);
                fprintf(stderr, "Bytes must be specified as digits separated by white space.\n");
		return FALSE;
	    }
	    putc((char) dvalue, out);
	    fileaddr++;
	    cp += nscan;
	}
    }
    return TRUE;
}

/*  Main program  */

int main(argc, argv)
  int argc; char *argv[];
{
    int i, b, bp, cdata = FALSE, f = 0;
    char *cp, *clabel, opt;
    FILE *in = stdin, *out = stdout;

    for (i = 1; i < argc; i++) {
	cp = argv[i];
        if (*cp == '-') {
	    opt = *(++cp);
	    if (islower(opt)) {
		opt = (char) toupper(opt);
	    }
	    switch (opt) {

                case 'A':             /* -Af  --  Set address format */
		    opt = cp[1];
		    if (islower(opt)) {
			opt = (char) toupper(opt);
		    }
		    switch (opt) {
                        case 'D':
                            strcpy(addrformat, "%8d");
                            strcpy(scanaddr, "%ld%c");
			    break;

                        case 'H':
                        case 'X':
                            strcpy(addrformat, "%6X");
                            strcpy(scanaddr, "%lx%c");
			    break;

                        case 'O':
                            strcpy(addrformat, "%8o");
                            strcpy(scanaddr, "%lo%c");
			    break;

			default:
			    fprintf(stderr,
                            "Invalid address format '%c'.  Must be D, H, or O.\n", cp[1]);
			    return 2;
		    }
		    break;

                case 'C':
		    doublechar = TRUE;
		    break;

                case 'D':
		    cdata = TRUE;
		    clabel = cp + 1;
		    break;

                case 'L':
		    loading = TRUE;
		    break;

                case 'N':             /* -Nf  --  Set numeric dump format */
		    opt = cp[1];
		    if (islower(opt)) {
			opt = (char) toupper(opt);
		    }
		    switch (opt) {
                        case 'D':
                            strcpy(dataformat1, "%3d");
                            strcpy(scandata, "%d%n%c");
			    break;

                        case 'H':
                        case 'X':
                            strcpy(dataformat1, "%02X");
                            strcpy(scandata, "%x%n%c");
			    break;

                        case 'O':
                            strcpy(dataformat1, "%03o");
                            strcpy(scandata, "%o%n%c");
			    break;

			default:
			    fprintf(stderr,
                            "Invalid numeric dump format '%c'.  Must be D, H, or O.\n", cp[1]);
			    return 2;
		    }
		    break;

                case 'S':
		    streaming = TRUE;
		    break;

                case '?':
                case 'H':
                case 'U':
    fprintf(stderr, "XD  --  Extended dump.  Call with xd [input [output]]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "        Options:\n");
    fprintf(stderr, "             -af     Print addresses in f = Decimal, Hex, or Octal\n");
    fprintf(stderr, "             -c      Dump as ISO characters\n");
    fprintf(stderr, "             -dlabel Dump as a C data declaration\n");
    fprintf(stderr, "             -l      Load file from hex dump\n");
    fprintf(stderr, "             -nf     Numeric dump in f = Decimal, Hex, or Octal\n");
    fprintf(stderr, "             -s      Stream load (don't check file addresses)\n");
    fprintf(stderr, "             -u      Print this message\n");
    fprintf(stderr, "\nBy John Walker, http://www.fourmilab.ch/\n");
    fprintf(stderr,"Version %s\n", Version);
		    return 0;
	    }
	} else {
	    switch (f) {
		case 0:

		/** Warning!  On systems which distinguish text mode and
		    binary I/O (MS-DOS, Macintosh, etc.) the modes in these
		    open statements will have to be made conditional based
		    upon whether an encode or decode is being done, which
                    will have to be specified earlier.  But it's worse: if
		    input or output is from standard input or output, the 
		    mode will have to be changed on the fly, which is
                    generally system and compiler dependent.  'Twasn't me
                    who couldn't conform to Unix CR/LF convention, so 
                    don't ask me to write the code to work around
                    Apple and Microsoft's incompatible standards.

		    This file contains code, conditional on _WIN32, which
		    sets binary mode using the method prescribed by
                    Microsoft Visual C 1.52 ("Monkey C"); this may
                    require modification if you're using a different
		    compiler or release of Monkey C.  */

                    if ((in = fopen(cp, loading ? "r" : "rb")) == NULL) {
                        fprintf(stderr, "Cannot open input file %s\n", cp);
			return 2;
		    }
		    f++;
		    break;

		case 1:
                    if ((out = fopen(cp, loading ? "wb" : "w")) == NULL) {
                        fprintf(stderr, "Cannot open output file %s\n", cp);
			return 2;
		    }
		    f++;
		    break;

		default:
                    fprintf(stderr, "Too many file names specified.\n");
	    }
	}
    }

#ifdef _WIN32

    /*  If input is from standard input and we aren't loading
	from a dump file, set the input file mode to binary.  */

    if ((in == stdin) && (!loading)) {
	_setmode(_fileno(in), _O_BINARY);
    }

    /*  If output is to standard output and we're loading a
	binary file from a dump, set the output file mode to
	binary.  */

    if ((out == stdout) && (loading)) {
	_setmode(_fileno(out), _O_BINARY);
    }
#endif

    bp = 0;
    fileaddr = 0;

    if (loading) {
	char in_line[256];
	int lineno = 0;

	while (fgets(in_line, (sizeof in_line) - 2, in)) {
	    lineno++;
	    if (!interpline(in_line, lineno, out)) {
		fclose(out);
		return 2;
	    }
	}
    } else {
	if (cdata) {
	    char cout[80];

            fprintf(out, "unsigned char %s[] = {\n",
                clabel[0] == EOS ? "xd_data" : clabel);
            strcpy(cout, "    ");

	    while ((b = getc(in)) != EOF) {
		if (strlen(cout) > 72) {
                    fprintf(out, "%s\n", cout);
                    strcpy(cout, "    ");
		}
                sprintf(cout + strlen(cout), "%d,", b);
	    }
	    if (strlen(cout) > 4) {
		cout[strlen(cout) - 1] = EOS; /* Strip trailing comma */
                fprintf(out, "%s\n", cout);
	    }
            fprintf(out, "};\n");
	} else {
	    while ((b = getc(in)) != EOF) {
		if (bp >= bytesperline) {
		    outline(out, lineecho, bp);
		    bp = 0;
		    fileaddr += bytesperline;
		}
		lineecho[bp++] = (char) b;
	    }

	    if (bp > 0) {
		outline(out, lineecho, bp);
	    }
	}
    }
    return 0;
}
