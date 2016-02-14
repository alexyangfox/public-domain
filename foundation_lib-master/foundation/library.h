/* library.h  -  Foundation library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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

/*! \file library.h
    Dynamic library loader */

#include <foundation/platform.h>
#include <foundation/types.h>


//! Load library
FOUNDATION_API object_t      library_load( const char* name );

//! Unload library
FOUNDATION_API void          library_unload( object_t library );

//! Symbol lookup
FOUNDATION_API void*         library_symbol( object_t library, const char* name );

//! Library name
FOUNDATION_API const char*   library_name( object_t library );

//! Query if valid library
FOUNDATION_API bool          library_valid( object_t library );
