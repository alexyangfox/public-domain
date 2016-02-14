/*
 * glob tester
 *
 *	glot [pattern input]
 *
 * pattern assertions:
 *	pattern  ~   string	(matches)
 *	pattern !~   string	(does not match)
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define BIG	1024

int test(char *);
extern int amatch(char *, char *);

void
main(int argc, char **argv)
{
        int len;
        char line[BIG];
	FILE *fp = stdin;
	int err = 0;

	if (argc > 1)
		if ((fp = fopen(argv[1], "r")) == NULL) {
			printf("oops: %s: cannot open.\n", argv[1]);
			exit(1);
		}

        while (fgets(line, sizeof line, fp) != NULL) {
                line[len = strlen(line) - 1] = 0;
                err += test(line);
        }

	if (err == 0)
		printf("ok.\n");
	else
		printf("%d errors.\n", err);

	exit(0);
}

int
test(char *line)
{
	char *pat, *str, *opr;

	if ((pat = strtok(line, " \t")) == NULL)
			return 0;

	if ((opr = strtok(NULL, " \t")) == NULL ||
	    (str = strtok(NULL, " \t")) == NULL) {
		fprintf(stderr, "oops: malformed input: \"%s\"\n", line);
		return 1;
	}

	else {
		int r, n;

		if (strcmp(opr, "~") == 0)
			r = 1;
		else if (strcmp(opr, "!~") == 0)
			r = 0;
		else {
			fprintf(stderr, "oops: malformed op: %s\n", opr);
			return 1;
		}

		n = amatch(str, pat);

		if (r != n) {
			printf("? %s %s %s\n", pat, opr, str);
			return 1;
		}

		return 0;	/* matched */
	}
}
