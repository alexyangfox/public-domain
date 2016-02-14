/* uuid.h  -  Foundation library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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

/*! \file uuid.h
    UUID utility functions */

#include <foundation/platform.h>
#include <foundation/types.h>


//! UUID namespace "dns"
FOUNDATION_API const uuid_t           UUID_DNS;

/*! Generate UUID based on random numbers
    \return                           Random-based UUID */
FOUNDATION_API uuid_t                 uuid_generate_random( void );

/*! Generate UUID based on time (and host id)
    \return                           Time-based UUID */
FOUNDATION_API uuid_t                 uuid_generate_time( void );

/*! Generate UUID based on namespace and name
    \param namespace                  Namespace
	\param name                       Name
    \return                           Name-based UUID */
FOUNDATION_API uuid_t                 uuid_generate_name( const uuid_t namespace, const char* name );

/*! Check if UUIDs are equal
    \param u0                         First UUID
	\param u1                         Second UUID
	\return                           true if equal, false if not */
static FORCEINLINE CONSTCALL bool     uuid_equal( const uuid_t u0, const uuid_t u1 ) { return uint128_equal( u0, u1 ); }

/*! Make null UUID
	\return                           Null UUID */
static FORCEINLINE CONSTCALL uuid_t   uuid_null( void ) { return uint128_make( 0, 0 ); }

/*! Check if UUID is null
    \param uuid                       UUID
	\return                           true if null, false if not */
static FORCEINLINE CONSTCALL bool     uuid_is_null( const uuid_t uuid ) { return uint128_is_null( uuid ); }
