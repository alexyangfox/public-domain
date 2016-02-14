
#include <stdio.h>
#include <string.h>
#include <sys/ksyscalls.h>




/*
 *
 */

int create_assign (char *alias, char *path);
int list_assigns (void);
int print_help (void);

char alias_buf [MAX_PATHNAME_SZ];
char path_buf [MAX_PATHNAME_SZ];



/*
 *
 */

int main (int argc, char *argv[])
{
	int rc;
	
	if (argc <= 1)
		rc = list_assigns();
	else if (argc == 2)
	{
		if (strcmp("--help", argv[1]) == 0)
			rc = print_help();
		else
			rc = create_assign (argv[1], "");
	}
	else if (argc == 3)
		rc = create_assign (argv[1], argv[2]);
	else
	{
		printf ("assign: invalid arguments\n");
		rc = 0;
	}
	return rc;
}




/*
 *
 */

int create_assign (char *alias, char *path)
{
	return kos_set_assign (alias, path);
}




/*
 *
 */

int list_assigns (void)
{
	int i = 0;
	
	while (kos_get_assign (i, alias_buf, path_buf) == 0)
	{
		printf ("%s = %s\n", alias_buf, path_buf);
		i++;
	}
	
	return 0;
}




/*
 *
 */

int print_help (void)
{
	printf ("Usage: assign [ALIAS] [PATH]\n"
			"Create a new pathname alias or list existing aliases\n\n"
			"With no arguments the current aliases are listed.\n"
			"With no PATH, the ALIAS is removed\n");

	return 0;
}




