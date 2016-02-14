/* file "memory_allocation.c" */

/*
 *  This file contains the implementation of a utility function for memory
 *  allocation.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "basic.h"
#include "memory_allocation.h"
#include "diagnostic.h"


#ifndef CHECK_MEMORY_ALLOCATION

extern void *malloc_and_check(size_t size)
  {
    void *result;

    result = malloc(size);
    if (result == NULL)
      {
        basic_error("Out of memory while trying to allocate %lu byte%s.",
                    (unsigned long)size, ((size == 1) ? "" : "s"));
      }
    return result;
  }

#endif /* !CHECK_MEMORY_ALLOCATION */

extern void *malloc_and_check_with_file_and_line(size_t size,
        const char *file_name, size_t line_number)
  {
    void *result;

#ifdef CHECK_MEMORY_ALLOCATION
    result = test_malloc_implementation(size, file_name, line_number);
#else
    result = malloc(size);
#endif

    if (result == NULL)
      {
        basic_error("Out of memory while trying to allocate %lu byte%s.",
                    (unsigned long)size, ((size == 1) ? "" : "s"));
      }

    return result;
  }
