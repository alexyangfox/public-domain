/*****************************************************************************
C code to recursively traverse a file hierarchy
Output is similar to that of "dir /s /b" under MS-DOS

Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: ?
This code is public domain (no copyright).
You can do whatever you want with it.
*****************************************************************************/
#include <sys/stat.h> /* S_IFDIR, struct stat, stat() */
#include <stdlib.h> /* malloc(), realloc(), free() */
#include <string.h> /* NULL, strlen(), strcpy(), strcat() */
#include <dirent.h> /* DIR, struct dirent, opendir(), readdir(), closedir() */
#include <setjmp.h> /* jmp_buf, setjmp(), longjmp() */
#include <stdio.h> /* printf() */

#define	ERR_MEM	-2

static jmp_buf g_oops;
/*****************************************************************************
Improved string functions that use dynamic memory.

Declare a string variable like this (it MUST be initialized to NULL):
	char *foo = NULL;

Set a string to a given value:
	my_strcpy(&foo, "All I want for Christmas is: ");

Append to a string:
	my_strcpy(&foo, "peace on Earth, ");
	my_strcpy(&foo, "goodwill toward men, ");
	my_strcpy(&foo, "my two front teeth.");

Before returning from the function do this:
	free(foo);
*****************************************************************************/
static void my_strcpy(char **dst_p, char *src)
{
	char *dst, *temp;
	unsigned len;

	dst = (*dst_p);
	len = strlen(src) + 1;
	temp = realloc(dst, len);
	if(temp == NULL)
		longjmp(g_oops, ERR_MEM);
	dst = temp;
	strcpy(dst, src);
	(*dst_p) = dst;
}
/*****************************************************************************
*****************************************************************************/
static void my_strcat(char **dst_p, char *src)
{
	char *dst, *temp;
	unsigned len;

	dst = (*dst_p);
	if(dst == NULL)
	{
		dst = malloc(strlen(src) + 1);
		if(dst == NULL)
			longjmp(g_oops, ERR_MEM);
		strcpy(dst, src);
	}
	else
	{
		len = strlen(dst) + strlen(src) + 1;
		temp = realloc(dst, len);
		if(temp == NULL)
			longjmp(g_oops, ERR_MEM);
		dst = temp;
		strcat(dst, src);
	}
	(*dst_p) = dst;
}
/*****************************************************************************
*****************************************************************************/
static int recurse(char *dir_name)
{
	char *file_name = NULL;
	struct dirent *entry;
	struct stat statbuf;
	DIR *dir;

/* open directory */
	dir = opendir(dir_name);
	if(dir == NULL)
	{
		printf("Can't open directory '%s'\n", dir_name);
		return -1;
	}
/* read entries */
	while(1)
	{
		entry = readdir(dir);
		if(entry == NULL)
			break;
/* ignore . and .. directories */
		if(entry->d_name[0] == '.')
		{
			if(entry->d_name[1] == '\0')
				continue;
			if(entry->d_name[1] == '.')
				if(entry->d_name[2] == '\0')
					continue;
		}
/* form full path to entry */
		my_strcpy(&file_name, dir_name);
		if(dir_name[strlen(dir_name) - 1] != '/')
			my_strcat(&file_name, "/");
		my_strcat(&file_name, entry->d_name);
/* file or subdir? stat it to find out */
		if(stat(file_name, &statbuf) != 0)
			longjmp(g_oops, -1); /* shouldn't happen */
/* if directory, recurse */
		if(statbuf.st_mode & S_IFDIR)
			recurse(file_name);
/* if file, display path name
You could put a hook here to do something more interesting or useful... */
		else
			printf("%s\n", file_name);
	}
	free(file_name);
	closedir(dir);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	int i, err;

	if(arg_c < 2)
	{
		printf("Specify a directory on the command line\n");
		return 1;
	}
	err = setjmp(g_oops);
	switch(err)
	{
	case 0:
		break;
	case ERR_MEM:
		printf("Out of memory\n");
		return 2;
	case -1:
	default:
		printf("Unknown error %d\n", err);
		return 3;
	}
	for(i = 1; i < arg_c; i++)
	{
		err = recurse(arg_v[i]);
	}
	return 0;
}
