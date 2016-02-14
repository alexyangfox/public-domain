#ifdef vms
#include stdio
#else
#include <stdio.h>
#endif

/*
 * Rudimentary grep to test regex routines.
 *
 * DEBUG is only applicable to oz version of regex. Make sure to 
 * compile regex.c with -DDEBUG as well.
 *
 */
extern char *re_comp();
extern re_exec();

main(argc,argv)
char *argv[];
{
	char str[512];
	FILE *f;
	register int n;
	register char *p;

	if (argc < 2)
		error("usage: grep pat [file]");

	if ((p = re_comp(argv[1])) != 0) {
		printf("\t%s: %s\n", p, argv[1]);
		exit(1);
	}
#ifdef DEBUG
	symbolic(argv[1]);
#endif
	if (p = argv[2]) {
		if ((f = fopen(p, "r")) == NULL) {
			printf("cannot open %s\n", argv[2]);
			exit(1);
		}
		while ((n = load(str, f)) != EOF)
			if (re_exec(str))
				printf("%s\n",str);

	}
	exit(0);
}
load (s, f)
char *s;
FILE *f;
{
	register int c;
	static int lineno = 0;

	while ((c = getc(f)) != '\n' && c != EOF)
		*s++ = c;
	if (c == EOF)
		return (EOF);
	*s = (char) 0;
	return (++lineno);
}

error(s) char *s ; 
{ 
	fprintf(stderr,"%s\n",s); 
	exit(1); 
}
