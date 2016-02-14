/* file "test_thread_link.c" */

/*
 *  This file is used to test what linking options are needed for the thread
 *  module since they can differ from system to system.  This program is not
 *  meant to actually run, just to be compiled as a test.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stdio.h>
#include "../c_foundations/basic.h"


extern verdict salmoneye_plugin_initialize(void);


extern int main(int argc, char *argv[])
  {
    salmoneye_plugin_initialize();
    printf("The thread module was linked properly.\n");
    return 0;
  }
