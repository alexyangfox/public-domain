/* file "platform_dependent_libs.c" */

/*
 *  This file contains code to print out the names of any libraries needed to
 *  link against for things used by "platform_dependent.h".
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stdio.h>


#ifndef DYNAMIC_LIBRARY_STANDARD
#ifndef DYNAMIC_LIBRARY_DLOPEN
#ifdef _POSIX_C_SOURCE
#define DYNAMIC_LIBRARY_DLOPEN
#endif /* _POSIX_C_SOURCE */
#endif /* DYNAMIC_LIBRARY_DLOPEN */
#endif /* DYNAMIC_LIBRARY_STANDARD */


#ifdef DYNAMIC_LIBRARY_DLOPEN
static const char *result = " -ldl";
#else /* !DYNAMIC_LIBRARY_DLOPEN */
static const char *result = "";
#endif /* !DYNAMIC_LIBRARY_DLOPEN */


extern int main(int argc, char *argv[])
  {
    printf("%s", result);
    return 0;
  }
