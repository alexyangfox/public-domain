/* file "non_unicode.c" */

/*
 *  This file is part of a test of the Salmon Programming Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


extern int main(int argc, char *argv[])
  {
    char *buffer;

    buffer = malloc(strlen(argv[1]) + 100);
    assert(buffer != NULL);
    sprintf(buffer, "%s abc def%cghi jkl", argv[1], 255);
    system(buffer);
    free(buffer);
  }
