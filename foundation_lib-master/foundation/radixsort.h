/* radixsort.h  -  Foundation library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
 * 
 * This library provides a cross-platform foundation library in C11 providing basic support data types and
 * functions to write applications and games in a platform-independent fashion. The latest source code is
 * always available at
 * 
 * https://github.com/rampantpixels/foundation_lib
 * 
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#pragma once

/*! \file radixsort.h
    Radix sorter for 32/64-bit and floating point values */

#include <foundation/platform.h>
#include <foundation/types.h>


FOUNDATION_API radixsort_t*               radixsort_allocate( radixsort_data_t type, radixsort_index_t num );
FOUNDATION_API void                       radixsort_deallocate( radixsort_t* sort );

FOUNDATION_API const radixsort_index_t*   radixsort( radixsort_t* sort, const void* input, radixsort_index_t num );

