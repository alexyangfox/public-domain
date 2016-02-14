/* Copyright (C) 2002, 2004 Christopher Clark <firstname.lastname@cl.cam.ac.uk> */
/* Copyright (C) 2008, 2009 Stephan Beal (http://wanderinghorse.net/home/stephan/) */
/* Code originally taken from: http://www.cl.cam.ac.uk/~cwc22/hashtable/ */
 /*
   On 17 June 2009, i (Stephan Beal) got permission from the original author
   (Christopher Clark) to dual-license this code under the following terms:

   - If the code is used in a jurisdiction where Public Domain
   property is regonized, then this code may be considered to be
   in the Public Domain. Its author expressly disclaims copyright
   in jurisdictions where such a disclaimer is allowed.

   - If the code is used in a jurisdiction which does not recognize
   Public Domain, the code must be used in terms with the MIT license,
   as described clearly and concisely at:

   http://en.wikipedia.org/wiki/MIT_License

   and reproduced in full below.

   - If the code is used in a jurisdiction which recognizes Public
   Domain, the user may use the code without limits, as for Public
   Domain property, or may instead opt to use the code under the terms
   of the MIT license.

   The MIT licensing terms follow:
   ========================================================================
   Copyright (C) 2002, 2004 Christopher Clark <firstname.lastname@cl.cam.ac.uk>
   Copyright (c) 2008, 2009 Stephan Beal (http://wanderinghorse.net/home/stephan/)

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
   ========================================================================
   (END LICENSE TEXT)

   The MIT license is compatible with both the GPL and commercial
   software, affording one all of the rights of Public Domain with the
   minor nuisance of being required to keep the above copyright notice
   and license text in the source code. Note also that by accepting the
   Public Domain "license" you can re-license your copy using whatever
   license you like.
 */
#ifndef WANDERINGHORSE_NET_WHHASH_H_INCLUDED
#define WANDERINGHORSE_NET_WHHASH_H_INCLUDED
#include <stddef.h> /* size_t */
#ifdef __cplusplus
extern "C" {
#endif
/**
   @page whhash_page_main whhash: hashtable API

   The functions and types named whhash* are part of the
   WanderingHorse.net hashtable library. It is a hashtable
   implementation based on code by Christopher Clark,
   adopted, extended, and changed somewhat by yours truly.

   License: Dual: Public Domain in jurisdictions which allow it,
   or optionally the MIT license.

   Maintainer: Stephan Beal (http://wanderinghorse.net/home/stephan)

   The hashtables described here map (void*) to (void*) by using a
   client-supplied hash algorithm on the key pointers. The hashtable
   can optionally take over ownership of its keys or values, via
   whhash_set_key_dtor() and whhash_set_key_dtor(). The ownership
   management option makes this type useful as a simple garbage
   collector.

   @section whhash_sec_example Example

   @code
   whhash_table  *h;
   struct some_key   *k;
   struct some_value *v;

   // Assume the following functions which can hash and compare the
   // some_key and some_val structs:
   static whhash_val_t         hash_some_key( void *k );
   static int                  cmp_key_val ( void *key1, void *key2 );

   h = whhash_create(16, hash_some_key, cmp_key_val);
   k = (struct some_key *)     malloc(sizeof(struct some_key));
   v = (struct some_value *)   malloc(sizeof(struct some_value));

   ...initialise k and v to suitable values...

   if (! whhash_insert(h,k,v) )
   {
         ...error... probably OOM or one of the args was null...;
   }

   if (NULL == (found = whhash_search(h,k) ))
   {    printf("not found!");                  }
   if (NULL == (found = whhash_take(h,k) ))
   {    printf("Not found\n");                 }

   whhash_destroy( h );
   @endcode

   @section whhash_sec_ownership Memory ownership

   By default a hashtable does no memory management of its keys or
   values.  However, whhash_set_key_dtor() and whhash_set_val_dtor()
   can be used to set a cleanup function. If a cleanup function is
   set, it is called (and passed the appropriate key or value) when an
   item is removed from the hashtable (e.g. when the hashtable is
   destroyed). It is possible to transfer ownership back to the caller
   by using whhash_take().

   Example:

   @code
   whhash_table  *h;
   h = whhash_create(16, whhash_hash_cstring_djb2, whhash_cmp_cstring );
   whhash_set_dtors(h, free, free);
   char * str = 0;
   char * key = 0;
   int i = 0;
   for( int i = 0; i < 10; ++i )
   {
       // Assume mnprintf() is a printf-like func which allocates new strings:
       key = mnprintf("key_%d",i);
       str = mnprintf("...%d...",i);
       whhash_insert(h, key, str ); // transfers ownership of key/str to h
   }
   whhash_destroy( h ); // Calls free() on each inserted copy of key and str.
   @endcode


   @section whhash_sec_macros Macros

   Macros may be used to define type-safe(r) whhash_table access functions, with
   methods specialized to take known key and value types as parameters.
 
   Example:

   Insert this at the start of your file:

@code
 DEFINE_WHHASH_INSERT(insert_some, struct some_key, struct some_value);
 DEFINE_WHHASH_SEARCH(search_some, struct some_key, struct some_value);
 DEFINE_WHHASH_TAKE(take_some, struct some_key);
 DEFINE_WHHASH_REMOVE(remove_some, struct some_key);
@endcode

    This defines the functions 'insert_some', 'search_some',
    'take_some', and 'remove_some'.  These operate just like
    whhash_insert() etc., with the same parameters, but their function
    signatures have 'struct some_key *' rather than 'void *', and
    hence can generate compile time errors if your program is
    supplying incorrect data as a key (and similarly for value).

    Note that the hash and key equality functions passed to
    whhash_create still take 'void *' parameters instead of 'some key
    *'. This shouldn't be a difficult issue as they're only defined
    and passed once, and the other functions will ensure that only
    valid keys are supplied to them.

    The cost for this checking is increased code size and runtime
    overhead - if performance is important, it may be worth switching
    back to the unsafe methods once your program has been debugged
    with the safe methods.  This just requires switching to some
    simple alternative defines - eg:

@code
#define insert_some whhash_insert
@endcode

@section whhash_sec_threadsafety Thread safety

    By default it is never legal to use the same hashtable from
    multiple threads at the same time. That said, the client may use
    their own locking to serialize access. All API functions which
    take a (whhash_table const *) argument require only a read lock,
    whereas those taking a (whhash_table*) argument require a
    exclusive (read/write) access. whhash_create() is reentrant and
    does not need to be locked.

    Special consideration is also needed considering locking of stored
    keys/values. If a referenced item is used by multiple threads,
    this code has no way of knowing about it. When in doubt, don't
    store items which are shared across threads in a hashtable unless
    you know that lifetime and ownership issues can be mitigated. If,
    e.g., a hashtable does not own its entries, one needs to make sure
    that if the entry is deleted from somewhere else, that it's
    removed from the hashtable (or ensure that that entry cannot be
    references again, otherwise you'll get a dangling pointer back).


   @section whhash_sec_links Other resources

   Some other resources:

   - The original implementation off of which whhash is based can be found
   at: http://www.cl.cam.ac.uk/~cwc22/hashtable/

   - A good intro to hashtables:
   http://en.wikipedia.org/wiki/Hash_table

   - Brief analysis and implementations of of several well-known hash routines:
   http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx
*/

/** @typedef whhash_val_t

   The hash key value type by the whhash API.
 */
typedef unsigned long whhash_val_t;

/**
 @var const whhash_val_t whhash_hash_val_err

   whhash_hash_val_err is ((whhash_val_t)-1). It is used to report
   hash value errors. Hash routines which want to report
   an error may do so by returning this value.
 */
extern const whhash_val_t whhash_hash_val_err;
/** @struct whhash_table

    whhash_table is an opaque handle to hashtable data. They are
    created using whhash_create() and destroyed using
    whhash_destroy().
*/
struct whhash_table;
typedef struct whhash_table whhash_table;


/**
   whhash_create() allocates a new hashtable with the given minimum
   starting size (the actual size may, and probably will, be
   larger). The given hash function and comparison function are used
   for all hashing and comparisons, respectively.

   The comparison function must return 0 if no match is made, else
   non-zero. The hash function must create a hash value for the given
   object, the more randomly distributed the better.
   
   minsize specifies the minimum initial size of the hashtable. It may
   (and probably will) allocate more than this, however.
*/
whhash_table * whhash_create(whhash_val_t minsize,
			     whhash_val_t (*hashfunction) (void const *),
			     int (*key_eq_fn) (void const *,void const *));


/**
   Sets the destructor function for h's keys, which are cleaned up
   when items are removed or the whhash_table is destroyed. By default
   a whhash_table owns its keys and will call free() to release
   them. If you use keys from managed memory and don't want them
   destroyed by the whhash_table, simply pass 0 as the dtor argument.
 */
void whhash_set_key_dtor( whhash_table * h, void (*dtor)( void * ) );

/**
   This is similar to whhash_set_key_dtor(), but applies to
   whhash_table values instead of keys. By default a whhash_table does
   not own its values and will not delete them under any
   circumstances. To make the whhash_table take ownership, simply pass
   'free' (or suitable function for your type, e.g. many structs may
   need a custom destructor) to this function.
 */
void whhash_set_val_dtor( whhash_table * h, void (*dtor)( void * ) );

/**
   Equivalent to whhash_set_key_dtor(h,keyDtor) and
   whhash_set_val_dtor(h,valDtor).
*/
void whhash_set_dtors( whhash_table * h, void (*keyDtor)( void * ), void (*valDtor)( void * ) );


/**
 whhash_insert() adds new entries to a hashtable.

 Neither of the h or k parameters may be 0. A value of 0 is okay but
 in practice it means a client cannot differentiate between "not
 found" and "found value of 0". The objects pointed to by the
 parameters must outlive this hashtable (or be managed/owned by it by
 assigning destructor functions, e.g. via whhash_set_dtors()).

 When a key is re-inserted (already mapped to something) then results
 are undefined. When in doubt you should use whhash_take() to remove
 the entry before re-adding it (but then freeing the old value becomes
 your responsibility). (Historical note: some versions of this code
 automatically replaced existing items, but profiling shows it to
 nearly double the insertion costs so this feature was removed.)

 This function will cause the table to expand if the insertion would
 take the ratio of entries to table size over the maximum load factor.

 Key/value ownership: By default a hashtable does not own its keys nor
 its values. You may set the ownership policy by using
 whhash_set_key_dtor() and whhash_set_val_dtor(). The destructors are
 called whenever items are removed from the whhash_table or the
 whhash_table is destroyed.
   
 The key object must not change in a way which affects its hash value
 after it is inserted. To do so invokes undefined behaviour.

 That the key is not a const parameter is unfortunate, but it's the
 only way we can properly apply ownership management without violating
 constness. The original intention of this type was a hash which owned its
 keys, which is why the parameter has historically not been const. Current
 usage of this class ranges from general purpose lookup (without memory
 management) to garbage collector, and some of those uses cannot be achieved
 with a const key.
 */
int 
whhash_insert(whhash_table *h, void *k, void *v);

/**
 whhash_replace() changes the value associated with a key, where there
 already exists a value bound to the key in the whhash_table. If (v ==
 existingValue) then this routine has no side effects, otherwise this
 function calls whhash_free_val() for any existing value tied to
 k, so it may (or may not) deallocate an object.

 Source by Holger Schemel. Modified by Stephan Beal to use
 whhash_free_value().

 Returns 0 if no match is found, -1 if a match is made and replaced,
 and 1 if (v == existingValue).

 Neither the h nor k parameters may be 0.
 */
int
whhash_replace(whhash_table *h, void *k, void *v);

#define DEFINE_WHHASH_INSERT(fnname, keytype, valuetype) \
int fnname (whhash_table *h, keytype *k, valuetype *v) \
{ \
    return whhash_insert(h,k,v); \
}

/**
  whhash_search() searches for the given key and returns the
  associated value (if found) or 0 (if not found). Ownership of the
  returned value is unchanged.
 */
void *
whhash_search(whhash_table *h, void const * k);

/**
   Works like whhash_search() but can differentiate between a found
   value of 0 and no-such-element.

   Returns 0 for if wh does not contain the given key and non-zero for
   if wh does contain the key.

   Maintainer's note: the wh parameter should be const, but its not
   for internal reasons.
 */
short whhash_contains(whhash_table *wh, void const * k);


#define DEFINE_WHHASH_SEARCH(fnname, keytype, valuetype) \
valuetype * fnname (whhash_table *h, keytype const *k) \
{ \
    return (valuetype *) (whhash_search(h,k)); \
}

/**
 whhash_take() removes the given key from the whhash_table and
 returns the value to the caller. The key/value destructors (if any)
 set via whhash_set_key_dtor() and whhash_set_val_dtor() WILL
 NOT be called! If you want to remove an item and call the dtors for
 its key and value, use whhash_remove().

 If (!h), (!k), or no match is found, then 0 is returned and ownership
 of k is not changed. If a match is found which is 0, 0 is
 still returned but ownership of k is returned to the caller. There
 is unfortunately no way for a caller to differentiate between these
 two cases.

 */
void * whhash_take(whhash_table *h, void const *k);

/**
   Works like whhash_take(h,k), but also calls the dtors registered
   for the key and value, which may (or may not) deallocate the
   objects. Since the destructor may (or may not) destroy the key, k
   may not (or may) be valid after this function returns.

   Returns 0 if it finds no entry to remove, else non-zero.
*/
short whhash_remove(whhash_table *h, void *k);

#define DEFINE_WHHASH_TAKE(fnname, keytype, valuetype) \
valuetype * fnname (whhash_table *h, keytype const *k) \
{ \
    return (valuetype *) (whhash_take(h,k)); \
}

#define DEFINE_WHHASH_REMOVE(fnname, keytype) \
short fnname (whhash_table *h, keytype const *k) { return whhash_remove(h,k);}


/*!
  whhash_count() returns the number of items in the hashtable.
 This is a constant-time operation.
*/
size_t
whhash_count(whhash_table const * h);


/**
 whhash_destroy() cleans up resources allocated by a whhash_table and calls
 the configured destructors for each key and value. After
 this call, h is invalid.
 */
void whhash_destroy(whhash_table *h);

/**
   Similar to whhash_destroy(), but only cleans up the internal
   contents, and does not actualy delete the h pointer. Thus h
   can be used for further inserts. The next insert will force
   the hashtable to re-allocate internal storage. This routine
   is the only way to get a whhash_table to reduce its size.

   This routine will force any registered dtors to be called on
   each key/value in the hashtable.
*/
void whhash_clear(whhash_table *h);

/**
  A comparison function for use with whhash_create(). If (k1==k2) it
  returns non-0 (true), otherwise it performs strcmp(k1,k2) and
  returns true if they match.

  This function accepts null pointers. Two null pointers will compare
  equal, otherwise a single null pointer in the pair will evaluate
  to false.
 */
int whhash_cmp_cstring( void const * k1, void const * k2 );


/**
  A comparison function for use with whhash_create().
  It essentially performs (*((long*)k1) == (*((long*)k2))).

  Trying to compare numbers of different-sized types (e.g. long and
  short) won't work (well, probably won't). Results are undefined.
 */
int whhash_cmp_long( void const * k1, void const * k2 );

/**
   An int/long hashing function for use with whhash_create().  It
   requires that n point to a long integer, and it simply returns the
   value of n, or whhash_hash_val_err on error (n is NULL).
 */
whhash_val_t whhash_hash_long( void const * n );

/**
   This is a hash routine for generic void pointers. To avoid clustering
   of hashvalues, it performs the following:

   - Casts k to its numeric value (as a long, or some other platform-specific
   numeric type).
   - Performs a Berstein Hash on the first (sizeof(long)/sizeof(char)) bytes
   of that numeric value, treating each byte as a separate input character.

   That greatly improves the distribution range of the hash values over using
   the address of k as the hash value.
*/
whhash_val_t whhash_hash_void_ptr( void const * k );

/**
   A hashtable comparison function which simply returns (k1 == k2).
*/
int whhash_cmp_void_ptr( void const * k1, void const * k2 );

/**
  A C-string hashing function for use with whhash_create().  Uses the
  so-called "djb2" algorithm. Returns whhash_hash_val_err if (!str).

  For notes on the hash algorithm see:

  http://www.cse.yorku.ca/~oz/hash.html

*/
whhash_val_t whhash_hash_cstring_djb2( void const * str );

/**
   Implements the "Modified Bernstein Hash", as described at:

   http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx

*/
whhash_val_t whhash_hash_cstring_djb2m( void const * str );


/**
   Implements the "Shift-Add-XOR" (SAX) hash, as described at:

   http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx
*/
whhash_val_t whhash_hash_cstring_djb2m( void const * str );


/**
   Implements the One-at-a-time hash, as described at:

   http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx
*/
whhash_val_t whhash_hash_cstring_oaat( void const * str );

/**
   The "rotating hash", as described at:

   http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx

   To quote that page:

   "Much of the time, the rotating hash is sufficient, and can be
   considered the minimal acceptable algorithm."
*/
whhash_val_t whhash_hash_cstring_rot( void const * str );

/**
  A C-string hashing function for use with whhash_create().  Uses the
  so-called "sdbm" algorithm. Returns whhash_hash_val_err if (!str).

  For notes on the hash algorithm see:

  http://www.cse.yorku.ca/~oz/hash.html
*/
whhash_val_t whhash_hash_cstring_sdbm( void const * str );


/*! @struct whhash_iter

  Opaque handle for whhash_table interators.
*/
struct whhash_iter;
typedef struct whhash_iter whhash_iter;


/**
   whhash_get_iter() creates a new iterator for the given hashtable
   and returns it. The caller must call free() on the object when he
   is done with it. If (!whhash_count(h)) then this function returns
   0.
*/
whhash_iter * whhash_get_iter(whhash_table *h);

/**
Returns the key of the (key,value) pair at the current position,
or 0 if !i.
*/
void * whhash_iter_key(whhash_iter *i);

/**
 Returns the value of the (key,value) pair at the current position,
   or 0 if !i.
*/
void * whhash_iter_value(whhash_iter *i);

/**
  Advance the iterator to the next element. Returns zero if advanced to
  end of table or if (!itr).
*/
int whhash_iter_advance(whhash_iter *itr);

/**
   Removes current element and advance the iterator to the
   next element the associated destructors to free (or not) the
   pointed-to key and value, so this call may (or may not) deallocate
   those objects.
*/
int whhash_iter_remove(whhash_iter *itr);

 /**
  Searches for the given key in itr's associated hashtable.
  If found, itr is modified to point to the found item and
  a true value is returned. If no item is found or if (!itr||!k)
  then false (0) is returned.
*/
int whhash_iter_search(whhash_iter *itr, void *k);

#define DEFINE_WHHASH_ITERATOR_SEARCH(fnname, keytype) \
int fnname (whhash_iter *i, keytype *k) \
{ \
    return (whhash_iter_search(i,k)); \
}

/**
   The whhash_stats struct is used for telemetry collection for
   whhash_table objects.  These objects should be created by calling
   whhash_get_stats().
*/
struct whhash_stats
{
    /**
       Number of entries in the context.
    */
    size_t entries;

    /**
       Number of registrations made in the context.
    */
    size_t insertions;

    /**
       Number of whhash_take() calls made in the context.
    */
    size_t removals;

    /**
       The number of searches made on the hashtable.
    */
    size_t searches;

    /**
       A rough *approximatation* of amount of memory allocated for
       whgc-specific internal structures used by the context,
       including the size of the underlying hashtable(s). A
       context has no way of knowing how much memory is used by
       registered items.
       
       Don't take this number too seriously. i try to keep the
       telemetry up to date for allocs, but keeping track of
       deallocs is more difficult due to timing and scope issues.
	*/
    size_t alloced;
    /**
       The number of times the size of the hashtable has been
       increased beyond the initial value.
    */
    size_t expansions;

    /**
       This is incremented each time a search operation fails
       to find the proper entry on the first try. If this
       number is large, the hash function may need to be
       changed.
    */
    size_t search_collisions;
};
typedef struct whhash_stats whhash_stats;

/**
   Returns the current statistics for h, or an object
   with all 0 values if (!h).
*/
whhash_stats whhash_get_stats( whhash_table const * h );

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* WANDERINGHORSE_NET_WHHASH_H_INCLUDED */

/*
 Copyright (c) 2002, Christopher Clark
 Copyright (C) 2008 Stephan Beal (http://wanderinghorse.net/home/stephan/)
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 
 * Neither the name of the original author; nor the names of any contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.
  
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
