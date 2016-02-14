/* file "generate_platform_dependent_salm.c" */

/*
 *  This file is a program to generate "platform_dependent.salm", a Salmon file
 *  containing platform-specific Salmon code.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stdio.h>
#include "platform_dependent.h"


extern int main(int argc, char *argv[])
  {
    printf("/* file \"platform_dependent.salm\" */\n");
    printf("\n");
    printf("/*\n");
    printf(" *  This file contains platform-specific code for Salmon.\n");
    printf(" *\n");
    printf(" *  This file was generated automatically by\n");
    printf(" *  \"generate_platform_dependent_salm.c\".\n");
    printf(" */\n");
    printf("\n");
    printf("\n");

    printf("immutable path_separator := \"%s\";\n", PATH_SEPARATOR);
    printf("immutable shared_library_suffix := \"%s\";\n", DLL_SUFFIX);
    printf("immutable executable_suffix := \"%s\";\n", EXECUTABLE_SUFFIX);
    printf("immutable install_directory := \"%s\";\n", INSTALL_DIRECTORY);
    printf("immutable binary_install_directory := \"%s\";\n",
           BINARY_INSTALL_DIRECTORY);
    printf("immutable library_install_directory := \"%s\";\n",
           LIBRARY_INSTALL_DIRECTORY);
    printf("immutable dll_install_directory := \"%s\";\n",
           DLL_INSTALL_DIRECTORY);
    printf("immutable c_include_install_directory := \"%s\";\n",
           C_INCLUDE_INSTALL_DIRECTORY);
    printf("immutable salmon_library_install_directory := \"%s\";\n",
           SALMON_LIBRARY_INSTALL_DIRECTORY);

    return 0;
  }
