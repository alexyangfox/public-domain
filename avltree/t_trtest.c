/* t_trtest - test the tree functions
 * vix 24jul87 [documented, added savestr for net distribution]
 */

#define MAIN

#include <stdio.h>
#include "vixie.h"
#include "tree.h"

main()
{
	tree	*t;
	char	line[100];

	tree_init(&t);
	while (printf("line (or .):  "), gets(line), line[0] != '.')
	{
		if (strncmp(line, "~r ", 3)) {
			trtest(&t, line, 1);
		}
		else {
			FILE *f;

			if (!(f = fopen(&line[3], "r")))
				perror(&line[3]);
			else {
				while (fgets(line, 100, f)) {
					line[strlen(line)-1] = '\0';
					printf("(%s)\n", line);
					trtest(&t, line, 0);
				}
				fclose(f);
			}
		}
	}
}

trtest(tt, line, inter)
tree	**tt;
char	*line;
{
	char	opts[100], *tree_srch(), *pc, *n;
	int	uar_print(), duar(), compar(), opt, status;

	pc = tree_srch(tt, compar, line);
	printf("tree_srch=%08lx\n", pc);
	if (pc)
	{
		printf("     <%s>\n", pc);

		if (inter) {
			printf("delete? "); gets(opts); opt = (opts[0]=='y');
		}
		else
			opt = 1;

		if (opt) {
			status = tree_delete(tt, compar, line, duar);
			printf("delete=%d\n", status);
		}
	}
	else
	{
		if (inter) {
			printf("add? "); gets(opts); opt = (opts[0]=='y');
		}
		else
			opt = 1;

		if (opt) {
			char	*savestr();

			n = savestr(line);
			tree_add(tt, compar, n, duar);
		}
	}
	tree_trav1(*tt, 0);
}

duar(pc)
char *pc;
{
	printf("duar called, pc=%08X: <%s>\n", pc, pc?pc:"");
	free(pc);
}

tree_trav1(t, l)
tree	*t;
{
	int	i;

	if (!t) return;
	tree_trav1(t->tree_l, l+1);
	for (i=0;  i<l;  i++) printf("  ");
	printf("%08lx (%s)\n", t->tree_p, t->tree_p);
	tree_trav1(t->tree_r, l+1);
}	
	
uar_print(pc)
char	*pc;
{
	printf("uar_print(%08lx)", pc);
	if (pc)
		printf(" '%s'", pc);
	putchar('\n');
	return 1;
}

compar(l, r)
	char *l, *r;
{
	printf("compar(%s,%s)=%d\n", l, r, strcmp(l, r));
	return strcmp(l, r);
}

char *
savestr(str)
	char	*str;
{
	char	*save;

	save = malloc(strlen(str) + 1);
	strcpy(save, str);
	return save;
}
