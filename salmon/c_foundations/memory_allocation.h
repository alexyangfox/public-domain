/* file "memory_allocation.h" */

/*
 *  This file contains the declarations and macros to assist in memory
 *  allocation.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef MEMORY_ALLOCATION_H
#define MEMORY_ALLOCATION_H


/*
 *          C INTERFACE
 *
 *  The interface provided by this file is the two macros MALLOC_ONE_OBJECT()
 *  and MALLOC_ARRAY().  The malloc_and_check() function also appears here, but
 *  that is only so that it may be used in the macro definitions.  It is not
 *  intended that the malloc_and_check() function be used directly by client
 *  code.
 *
 *  The MALLOC_ONE_OBJECT() macro dynamically allocates a single object of the
 *  specified type and returns a pointer to that object.  The MALLOC_ARRAY()
 *  macro dynamically allocates an array of objects of the specified type and
 *  returns a pointer to the array.  In both cases, the macro takes care of
 *  casting the result to the appropriate pointer type.  The allocated objects
 *  can be de-allocated with the free() function.  If there isn't enough memory
 *  to allocate the object or array, an error message is printed to standard
 *  error and NULL is returned.
 *
 *  These two macros are wrappers for malloc() that add some functionality.
 *  There are three reasons for using these macros instead of calling malloc()
 *  directly: to get the return pointer type cast automatically; to get the
 *  size calculation done automatically; and to print an error message in the
 *  case that memory runs out.
 *
 *  In addition, this file has some code to control the substitution of test
 *  functions for malloc() and free() when the macro CHECK_MEMORY_ALLOCATION is
 *  turned on.  Users of this module shouldn't have to worry about the details
 *  of how this works, they should just know that if the #include this file,
 *  then -DCHECK_MEMORY_ALLOCATION will substitute the alternate versions of
 *  malloc() and free().  If CHECK_MEMORY_ALLOCATION is not defined, then the
 *  usual malloc() and free() will be used by code that includes this header
 *  file.
 */

#include <stdlib.h>
#include <stddef.h>


extern void *malloc_and_check(size_t size);
extern void *malloc_and_check_with_file_and_line(size_t size,
        const char *file_name, size_t line_number);

#define MALLOC_ONE_OBJECT(object_type) \
        ((object_type *)(malloc_and_check(sizeof(object_type))))
#define MALLOC_ARRAY(element_type, element_count) \
        ((element_type *)(malloc_and_check(sizeof(element_type) * \
                                           (element_count))))

#ifdef CHECK_MEMORY_ALLOCATION

#define malloc test_malloc
#define free test_free
#define malloc_and_check(size) \
        malloc_and_check_with_file_and_line(size, __FILE__, __LINE__)

#include "memory_allocation_test.h"

#else

#define assert_is_malloced_block(pointer)
#define assert_is_malloced_block_with_minimum_size(pointer, size)
#define assert_is_malloced_block_with_exact_size(pointer, size)

#endif


#endif /*  MEMORY_ALLOCATION_H */
