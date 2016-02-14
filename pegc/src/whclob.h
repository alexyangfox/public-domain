#ifndef WANDERINGHORSE_NET_WHCLOB_H_INCLUDED_
#define WANDERINGHORSE_NET_WHCLOB_H_INCLUDED_ 1
#include <stdarg.h>
#include <stddef.h>

/*! @page whclob_page_main whclob: dynamic char array utilities

@section whclob_sec_about About whclob

The whclob API encapsulates behaviours related to creating, appending,
reading, writing, and freeing C-style blobs (stored in memory as char
arrays).

It is primarily intended to act as an output buffer for binary or
string data. It provides, for example, printf-like functionality which
makes use of the dynamic resize features blobs to simplify the
creation and ownership management of dynamic C-style strings.

whclob is heavily inspired and influenced by code in the Fossil source
tree (http://www.fossil-scm.org) written by D. Richard Hipp. In that
tree, blobs are used to store files in memory and to provide output
buffering support for the built-in web/cgi servers.

Rather than being a fork of Dr. Hipp's code, this is a
reimplementation done solely for licensing reasons - his code is GPL
and this is Public Domain.

Much of the heavy lifting of this API is done by the vappendf API
(see: \ref vappendf_page_main).

Author: Stephan Beal (http://wanderinghorse.net/home/stephan/)

@section whclob_sec_usage Usage

A whclob object must be initialized via whclob_init() (or equivalent)
and finalized via whclob_finalize(). Between those two calls, any
number of other clob-API functions may be used (unless documented
otherwise). As an example:

@code
whclob * c;
whclob_init( &c, 0, 0 );
whclob_appendf( c, "Hello, %s!", "world");
...
printf("%s\n",whclob_bufferc(c));
whclob_finalize( c );
@endcode

You can dump a whclob to stdout with the whclob_export() API:

whclob_export( c, stdout, whclob_exporter_FILE );

But doing so with binary data is not recommended.

Some example uses for clobs are:

- Creating and copying strings.
- Buffering input or output.
- Easily reading a whole file (or other input source) into memory.

@section whclob_sec_todo TODOs

- The read/write API is not complete. (Can't quite remember what's
missing, though.)

- We really need to move from using 'long' to 'size_t' in many places.
In some places that's not practical because we need to have room for
negative error codes.

************************************************************************/


/** @def WHCLOB_USE_FILE

    If whclob is built with WHCLOB_USE_FILE set to a true value then
    an import/export API is included for importing/export whclob
    objects from/to FILE handles.
*/
#if !defined(WHCLOB_USE_FILE)
#define WHCLOB_USE_FILE 1
#endif
#if WHCLOB_USE_FILE
#include <stdio.h>
#endif

/** @def WHCLOB_USE_ZLIB

    If whclob is built with WHCLOB_USE_ZLIB set to a true value then
    some routines are added which use zlib to de/compress whclob
    objects in memory. Enabling this requires that the client link
    with zlib (e.g. with the -lz linker argument).
*/
#if !defined(WHCLOB_USE_ZLIB)
#define WHCLOB_USE_ZLIB 0
#endif

/** @def WHCLOB_USE_BASE64

    If whclob is built with WHCLOB_USE_BASE64 set to a true value then
    an import/export API is included for importing/export whclob
    objects from/to base64 encoding. This required additional 3rd-party code
    (sold separately).
*/
#if !defined(WHCLOB_USE_BASE64)
#define WHCLOB_USE_BASE64 0
#endif



#ifdef __cplusplus
extern "C" {
#endif

/**
   whclob_rc_t holds status codes for the clob API.
   Their values must be unique negative integers, with
   the exception of OK, which must be 0.

   Many functions in the clob API return 0 or a positive
   number on success and one of these codes on error.

   The are accessed like: whclob_rc.AllocError. This is
   unconventional, but the intention is so that debuggers can get an
   actual symbol name. It also provides a namespace for
   our enum-like values.

   TODO: change these codes to int instead of long.
*/
struct whclob_rc_t
{
	/** The success status code. Must have the value 0. */
	const long OK;
	/** Generic error code. */
	const long Err;
	/** Signals that an allocation or reallocation failed. */
	const long AllocError;
	/** Signals that a null was passed or received where
	   one was not expected. */
	const long UnexpectedNull;
	/** Signals that some value was out of range. */
	const long RangeError;
	/** Signals an I/O error of some sort. */
	const long IOError;
	/** Signals some sort of argument-type error (e.g. (void *) args). */
	const long ArgError;
};
typedef struct whclob_rc_t whclob_rc_t;

/**
   The official way to get the clob status code values
   is via the whclob_rc object.
*/
extern const whclob_rc_t whclob_rc;

/**
   @struct whclob

   The whclob type (an opaque struct) holds data relating to in-memory
   blobs allocated by malloc(). A whclob holds (and owns) a
   varying-length character array for storing arbitrary binary data.

   Note that because client code cannot see the whclob implementation,
   they cannot create whclob objects on the stack. The proper way
   to create, initialize, and free whclob objects is:

\code
   whclob * c;
   if( whclob_rc.OK != whclob_init(&c,0,0) ) { ... error ... }
   ...
   whclob_finalize( c );
\endcode

Or to use the whclob_new() convenience function:

\code
   whclob * c = whclob_new();
   if( !c ) { ... OOM ... }
   ...
   whclob_finalize( c );
\endcode

*/
struct whclob;
typedef struct whclob whclob;


/**
   Allocates a new whclob object and assigns (*cb) to it. It initializes
   the object as described below. On error it assigns (*cb) to 0.

   It is only legal to call this on fresh, uninitialized whclob
   pointer. A clob is considered to be "uninitialized" just after
   declaration (before it is assigned to) and after whclob_finalize() has
   been called on it.

   To avoid memory leaks, for each successful call to whclob_init() you
   must have a matching call to whclob_finalize(). If this function
   fails then (*cb) is set to 0, so there is no need to call
   whclob_finalize() (but it is still safe (but a no-op) to do so, for
   the cases where doing so may simplify error handling).

   Interpretation of the 'data' and 'n' arguments:

   - If data is null and n is less than 1 then cb is initialized
   to a zero-length string.

   - If data is non-null and n is less than 0 then strlen(data) is used
   to calculate n.

   - If data is non-null and n is greater than 0 then the first n
   bytes of data are copied and data is assumed to be at least that
   long.

   - If data is 0 and n is positive then n bytes are reserved and
   zeroed.

   RETURNS:

   If this function returns whclob_rc.OK then (*cb) points to a new whclob
   object. On error it returns one of the negative integers defined in
   whclob_rc:

   - whclob_rc.UnexpectedNull if (!cb).

   - whclob_rc.AllocError if allocation of the new whclob fails.

   USAGE NOTES:

   Calling whclob_finalize(0) has no ill effects (other than an
   ignorable error code), so algorithms may be organized a call
   whclob_finalize() regardless of whether whclob_init() succeeds or
   fails. It is anticipated that this may simplify some error-handling
   cases.
*/
long whclob_init( whclob ** cb, char const * data, long n );

/**
   A simplified form of whclob_init() which allocates a new, empty whclob
   of the given size and returns it. The caller owns it and must finalize it with
   whclob_finalize(). On error (out of memory) it returns 0.
*/
whclob * whclob_new_n( size_t reserved );

/**
   Equivalent to whclob_new_n(0).
*/
whclob * whclob_new();

/**
  See whclob_set_alloc_policy() and
  See whclob_set_default_alloc_policy().
*/
typedef long (*whclob_alloc_policy_t)( long );

/**
   Sets the current allocation size determination policy and returns
   the previous one (so you may politely reset it when yours goes out
   of scope).

   When the framework wants to (re)allocate memory it will call the
   current policy function and pass it the suggested amount of memory
   it wants to reallocate. The policy may return that number as-is or
   adjust it upwards (e.g. *= 1.5) (never downwards) and return that
   number, which the reallocation will then use. If a smaller number
   is returned it will be ignored.

   Note that the policy only specifies a requested size, not where the
   memory will come from. The memory will always be pulled from
   malloc(). Likewise, the cleanup routines always use free() to
   delete the memory.

   The intention is to allow clients to who know they will append
   blobs a lot to increase the size of the allocs to reduce the number
   of potential reallocations.

   Be aware that the reallocation internals may allocate slightly more
   memory than requested, e.g. to unsure that the blob always has a
   trailing null character.

   If passed 0 it will use a default policy, which simply returns the
   value passed to it. You can fetch the current policy, if you like,
   by passing 0 to this function, then passing that return value
   back to this function.
*/
whclob_alloc_policy_t whclob_set_default_alloc_policy( whclob_alloc_policy_t );

/**
   Works just like whclob_set_default_alloc_policy() except that it
   applies only to cb and takes precedence over
   whclob_set_default_alloc_policy().
*/
whclob_alloc_policy_t whclob_set_alloc_policy( whclob * cb, whclob_alloc_policy_t );


/**
   Free the resources, if any, associated with cb and zeroes out the
   freed memory. After calling this, whclob_buffer(cb) will be an empty
   string and both whclob_size(cb) and whclob_capacity(cb) will be 0.

   It is illegal to pass a whclob which was not initialized by
   whclob_init() (or equivalent). Doing so results in undefined
   behaviour.

   Note that this function does not actually free the cb pointer
   itself. After this function returns, cb can be re-used by any clob
   API which is append-friendly (e.g. not whclob_char_filln()).

   On success it returns whclob_rc.OK. On error it returns:

   - whclob_rc.UnexpectedNull if (!cb).
*/
long whclob_reset( whclob * cb );

/**
   whclob_finalize() works just like whclob_reset(), plus it
   deallocates cb by calling free(). After calling this, cb is invalid
   until/unless it is passed to whclob_init() again.

   Returns whclob_rc.OK on success and whclob_rc.UnexpectedNull if
   (!cb).
*/
long whclob_finalize( whclob * cb );

/**
   Force's cb's cursor and used counter to be in bounds of the
   actual allocated memory. They are moved forwards or backwards,
   as necessary, to bring them into bounds.
*/
void whclob_force_in_bounds( whclob * cb );

/**
   Tries to reserve at least sz bytes of memory for the native blob
   associated with cb. It will not shrink the blob, only grow it or
   leave it alone, with one exception: if sz is 0 then the effect is
   the same as whclob_reset().

   This function may invalidate any old pointers obtained via
   whclob_buffer().

   Note that shrinking a clob may cause the internal position
   cursor (used by the read/write API) to move backwards to the
   new logical EOF.

   If cb is "const" (points to a blob but has an allocated size of 0)
   then calling this with a non-zero sz will cause sz bytes of the
   referenced blob data to be deeply copied to cb.

   On success it returns the number of bytes allocated to cb. The
   number may be larger than sz, due to internal details of this API
   and the implementation of the current allocation policy (see
   whclob_set_default_alloc_policy()).

   On error it returns one of the negative numbers specified
   by whclob_rc:

   - whclob_rc.AllocError = (re)allocation failed.

   - whclob_rc.UnexpectedNull = cb parameter was 0.

*/
long whclob_reserve( whclob * cb, unsigned int sz );

/**
   Works like whclob_reserve(), but marks all memory
   in the clob as used. This means that appending
   will start from sz, rather than from whatever
   the end previously was. It returns the same values
   as whclob_reserve().
*/
long whclob_resize( whclob * cb, unsigned int sz );

/**
   Returns the number of "used" bytes in cb. Appending to a clob will
   start after the last "used" byte. Note that clobs may (and normally do)
   have more memory allocated to them than is "used".

   Returns 0 or greater (the size of cb) on success and
   whclob_rc.UnexpectedNull if (!cb).
*/
long whclob_size( whclob const * cb );

/**
   Returns the current allocated capacity of cb, or
   whclob_rc.UnexpectedNull if (!cb).

   The capacity will always be equal to or greater than whclob_size(cb).
*/
long whclob_capacity( whclob const * cb );

/**
   Returns a pointer to cb's buffer (may be 0). The buffer is owned by
   cb.

   Do not keep ahold of the returned pointer for longer than
   necessary, as any operations which change cb may invalidate that
   pointer.

   Do not generically depend on the returned array being a
   null-terminated string, as cb can hold arbitrary data. Use
   whclob_size(sb) to get the length of the data.
*/
char * whclob_buffer( whclob * cb );

/**
   A variant of whclob_buffer() for places where a const is needed.
*/
char const * whclob_bufferc( whclob const * cb );

/**
   Gives up cb's control of any allocated memory and returns it to the
   caller, who takes over ownership.

   If a blob is storing binary data then there is no way to know the
   size of the returned buffer. Thus if you need its length, be sure
   to call whclob_size(cb) before calling this.

   This function effectively does whclob_reset(cb) but does not finalize
   cb. Thus after calling this function cb is still a valid object for
   purposes of the other whclob_xxx() functions.
 */
char * whclob_take_buffer( whclob * cb );

/**
   Flags for whclob_seek(), they are identical in nature to SEEK_SET,
   SEEK_CUR, and SEEK_END used by the conventional fseek() C function.

   WHCLOB_SEEK_SET: moves the cursor to absolute position (offset).

   WHCLOB_SEEK_CUR: moves the cursor to (cursor + offset).

   WHCLOB_SEEK_END: moves the cursor to relative postition (end -
   offset), where 'end' is the index 1 after the last character of the
   USED portion of the blob. e.g. the end of "abcd" is at position 4.
*/
enum WHClobSeekWhence { WHCLOB_SEEK_SET = 0,
			WHCLOB_SEEK_CUR = 1,
			WHCLOB_SEEK_END = -1
};

/**
   Similar to the standard fseek(), whclob_seek() moves the blob's
   cursor to its (current pos + offset), relative to whence, which
   must be one of of the ClobSeekFlags values. Once the seek is
   complete, the cursor is bumped back into bounds (or at EOF). The
   return value is the new cursor position within the blob.

   Note that when using WHCLOB_SEEK_END, only a negative offset makes
   sense, as a positive offset is out of bounds.

   The return value is a positive integer on success, indicting the
   new position within the blob. On error a negative number is
   returned:

   - whclob_rc.RangeError = the 'whence' value is unknown.

*/
long whclob_seek( whclob * cb, long offset, int whence );

/**
  Moves cb's cursor back to the begining of the blob.
*/
void whclob_rewind( whclob * cb );

/** Returns whclob_rc.OK if pos is within the used bounds of cb,
    else returns whclob_rc.RangeError.
*/
long whclob_pos_in_bounds( whclob * cb, long pos );


/**
   Fills the pointed-to memory with char specified by c, starting at
   blob offset specified by startPos and copying the next n bytes. It ignores
   the "used" space boundary and fills any allocated space. It does
   not change the size of the allocated memory block or its used-space
   count.

   On success it returns the number of zeroes which it fills
   in. Normally this is n, but if (startPos + n) would overflow, the
   blob is filled to the end and that (smaller) length is returned.

   On failure it returns one of the values defined in whclob_rc:

   - whclob_rc.Err if cb does not contain a blob, or points
   to a blob but does not own it.

   - whclob_rc.RangeError if startPos is out of bounds or n <= 0.
*/
long whclob_char_filln( whclob * cb, char c, long startPos, long n );

/**
   Same as whclob_char_filln( cb, '\\0', 0, whclob_size( cb ) );
*/
long whclob_zero_fill( whclob * cb );


/**
   Appends the first dsize bytes of data to cb's blob, exanding the
   blob as needed. If dsize is -1 then strlen(data) is used to
   calculate the length. If dsize is 0 an error code is returned (see
   below). This does not modify cb's internal position cursor.

   If cb does not currently own the blob it points to then this
   function may cause it to create a copy of that object, and append
   to that copy. This may or may not be expensive, depending on the
   size of the copy.

   Unlike whclob_appendf(), this function is safe for use with binary
   data, but only if dsize is explicitly set. Otherwise it will only
   append up to the first null character in data.

   On success a positive integer is returned, the number of bytes by
   which the blob extended. On failure, one of the negative values
   defined in whclob_rc is returned:

   - whclob_rc.AllocError if a memory (re)allocation fails.

   - whclob_rc.RangeError if dsize is 0.
*/
long whclob_append( whclob * cb, char const * data, long dsize );

/**
   Appends n copies of ch to cb and returns the number added. If n is less
   than 1 then whclob_rc.RangeError is returned and cb is not modified.
*/
long whclob_append_char_n( whclob * cb, char ch, const long n );

/**
  Makes a deep copy of src, placing it into dest.

  Neither src nor dest may be 0. dest is deallocated before the copy
  happens.

  Returns one of the values defined in whclob_rc:

  - whclob_rc.RangeError if (src==dest).

  - whclob_rc.AllocError if a memory (re)allocation fails.

  - whclob_rc.OK on success.
*/
long whclob_copy( whclob * src, whclob * dest );



/**
   Like printf, but output is appended to the end of cb, expanding it
   as necessary. It also imlements a couple extension conversions, to
   be documented in the docs for vappendf().

   Return value: if the number is >= 0 then it is the size by which
   the target blob grew as a result of the printf (i.e. the number of
   bytes printf added to it). If the return value is negative then it
   is an error code from whclob_rc.

   This function is not safe for binary data - if fmt contains
   any null characters the processing will stop.

   For the list of non-standard format specifiers, see
   the documentation for vappendf().
*/
long whclob_vappendf( whclob * cb, char const * fmt, va_list vargs );


/**
   See whclob_vappendf(whclob*,char const *,va_list). This is identical
   except that it takes an elipses list instead of va_list.
*/
long whclob_appendf( whclob * cb, const char * fmt, ... );



/**
   Reads up to n bytes from src, starting at startPos, and copies them
   to dest. This does not advance cb's internal cursor.

   On success the return value is the number of bytes read, which may
   be less than n (or 0 on EOF).

   On error it returns one of the negative values defined
   in whclob_rc:

   - whclob_rc.UnexpectedNull if src or dest are null

   - whclob_rc.RangeError if n is less than 1 or if startPos is out of
   src's bounds.
*/
long whclob_copy_slice( whclob * src, whclob * dest, long startPos, long n );

/**
   Reads up to n bytes from src, starting at the current
   cursor position (whclob_tell()). Reading advances
   the cursor.

   The return value is the number of bytes read, which
   may be less than n (or 0 on EOF).
*/
long whclob_read( whclob * src, whclob * dest, long n );

/**
   Returns the current position of cb's internal cursor, analogous to
   the standard ftell() function.
*/
long whclob_tell( whclob const * cb );

/**
   This function is just like whclob_append() but it starts writing
   at the position returned by whclob_tell(cb). Writing advances
   the internal cursor. It returns the number of bytes written.

   If dsize is -1 then strlen(data) is used for dsize.

   On success it returns the number of bytes written. On failure it
   returns a negative integer error code defined in whclob_rc:

   - whclob_rc.AllocationError if memory could not be allocated.

   - whclob_rc.RangeError if dsize is 0.

   - whclob_rc.UnexpectedNull if data is 0.
*/
long whclob_write( whclob * cb, char const * data, long dsize );



/**
   This function ensures that the one-past-the-last item in the blob
   to 0. The "used" size of cb does not change.

   On success it returns whclob_rc.OK. On failure it returns one of the
   negitive integer values defined in whclob_rc:

   - whclob_rc.Err if cb does not own a pointer to an underlying blob.

   - whclob_rc.AllocError if a memory (re)allocation fails.
*/
long whclob_null_terminate( whclob * cb );

/**
   "Chops" cb off at a given length. If the current used space is
   smaller than pos then it will be padded with NULLs to make it fit.
   It is is longer than pos then the clob will be reallocated (if
   necessary) to fit the new space and any old data which is now
   outside the used range will be zeroed out (this is necessary for
   sane behaviour in some use cases). In any case, on success the new
   position of the clob will be the given position.

   If allocPolicy is 0 then the amount of memory allocated by cb is
   not adjusted (unless (pos>whclob_size(cb)), in which case we have
   to expand the clob). If it is >0, whclob_reserve() will be called
   to try to reserve at least pos bytes. If (allocPolicy<0) then the
   allocated memory is shrunk to fit (if possible). If pos is past the
   current end-of-data position then allocPolicy is forced to 1 so
   that the buffer can be expanded (if needed).

   Returns whclob_rc.OK or a positive value on success or a negative
   value from whclob_rc on error.

   The two most common uses for this function:

   - "Shrink-wrapping" the value (eliminating extra allocated space)

   - Re-using a clob's buffer as a target for, e.g. fread(), by
   reserving a certain amount of space then truncating it at position
   0 but with an allocPolicy of >=0. This can save on allocations compared
   to using, e.g. whclob_reset().

*/
long whclob_truncate( whclob * cb, long pos, int allocPolicy );

/* TODO???: whclob_trim() */


/**
   whclob_exporter is a generic interface to exporting whclob objects
   (e.g. to arbitrary streams).

   Policy for implementations:

   - Take a (char const *) data pointer and "export" n bytes of it,
   where the meaning of "export" is implementation-specified.

   - The 'arg' argument is an implementation-specific pointer. This API
   does not use it, but passes it on so that client code can use it, e.g.
   to accumulate or export data (e.g. by passing an output stream handle).

   - On success it must return 0 or greater. Ideally it should return the
   exact number of bytes processed, but that might not be feasible for
   some implementations. In those cases, returning n is the best approach.

   - On failure it must return a negative number, prefferably one of those
   defined by whclob_rc.
*/
typedef long (*whclob_exporter)( void * arg, char const * data, long n );

/**
   whclob_export() is a generic interface for exporting blobs to
   "external representations".

   It calls pf( arg, ... ) one time to export cb's data using whatever
   approach pf implements. The return value is that of calling pf.
*/
long whclob_export( whclob const * cb, void * arg, whclob_exporter pf );

/**
   whclob_importer is the import counterpart of whclob_exporter. It
   defines an interface for callbacks which can provide a blob with
   data.

   Note that for input it is not generically possible to
   get the length of the data before starting. Thus this
   API takes a whclob object, which it can append to, whereas
   the whclob_exporter functions take raw data pointers as
   sources.

   target must be an initialized whclob. It is appended to,
   not overwritten. It is expanded as necessary.

   Return value is the number of bytes appended to the target, or a
   negative number on error (preferably one of the whclob_rc error
   codes).
*/
typedef long (*whclob_importer)( whclob * target, void * arg );

/**
   Returns the same as pf( arg, dest ).
*/
long whclob_import( whclob * dest, void * arg, whclob_importer pf );

/**
   Moves a block of memory within cb, starting at start1, moving n
   bytes to the position starting at start2. All points and ranges
   must be in the whclob_size() range. start2 may be less than start1
   but they may not be equal. A value of less than 1 for n is not
   currently supported, though it should be to support backwards
   movement of memory.

   Memory which is "moved" gets filled with the specified filler char
   after its contents are copied to its new location.

   This function does not change the size of cb.

   On success it returns the number of bytes moved (n)
   and on failure it returns:

   - whclob_rc.RangeError if any values or ranges are out of bounds

   - whclob_rc.UnexpectedNull if cb is null.
*/
long whclob_memmove_fill( whclob * cb, char const filler, int start1, int n, int start2 );

/**
   A convenience form of whclob_memmove_fill(cb,0,start1,n,start2).
*/
long whclob_memmove( whclob * cb, int start1, int n, int start2 );

/**
   Swaps the contents of n bytes between two blobs. start1 is relative
   to cb1 and start2 is relative to cb2.

   On success it returns the number of bytes swapped. On error it returns
   one of:

   - whclob_rc.UnexpectedNull if (!cb1) or (!cb2).

   - whclob_rc.RangeError if start1 and n are not within cb1's bounds
   or start2 and n are not within cb2's bounds.

   This is a linear operation (based on the value of n). If you want
   to swap the entire contents of two clobs, you can do so in constant
   time by using whclob_swap() instead of this routine.
*/
long whclob_memswap( whclob * cb1, int start1, int n, whclob * cb2, int start2 );


/**
   Efficiently swaps the contents of blobs c1 and c2, which must
   both be initialized clobs.

   Returns whclob_rc.OK on success or whclob_rc.UnexpectedNull
   if either c1 or c2 are null.
*/
long whclob_swap( whclob * c1, whclob * c2 );

/**
   Copies the contents of src to the dest, which
   must be an UNINITIALIZED whclob object.

   Returns whclob_rc.OK on success or, on error:

   - whclob_rc.AllocError

   - whclob_rc.UnexpectedNull if src is null.

Example usage:

\code

whclob * orig;
whclob_init( &orig );
whclob_appendf( orig, "Hi, world." );
whclob * clone;
whclob_clone( orig, &clone );

\endcode

Note the pointer-to-pointer parameter for whclob_init()
and the second parameter to whclob_clone().

*/
long whclob_clone( whclob * src, whclob ** dest );

/**
   Works more or less like sprintf(), but supports the printf
   specifiers accepted by whclob_appendf() and can automatically
   extends the string as necessary. The caller owns the returned
   null-terminated string and must free it using free().
*/
char * whclob_vmprintf( char const * fmt, va_list vargs );

/**
   Functionally identical to whclob_vmprintf() but takes
   a (...) ellipses list instead of a va_list.
*/
char * whclob_mprintf( char const * fmt, ... );


#if WHCLOB_USE_FILE
/** @implements whclob_exporter

   This is a sample whclob_exporter for use with whclob_export.
   It is used like this:

   whclob_export( clob, an_open_FILE_handle, whclob_exporter_FILE );

   The second argument to whclob_export must be an open (FILE*).

   Returns n on success and a negative number on error.

   The file handle is not closed by this routine.
*/
long whclob_exporter_FILE( void * fh, char const * data, long n );

/**
   Like whclob_exporter_FILE(), but expects arg to be a (char const *)
   filename. Returns the same as the equivalent whclob_exporter_FILE()
   unless arg cannot be opened as a file, in which case
   whclob_rc.IOError is returned.
*/
long whclob_exporter_filename( void * arg, char const * data, long n );

/**
   A whclob_importer implementation for whclob_import. It expects
   arg to be an open (FILE*). The target is appended with the
   contents of the file. On error, a negative number is returned.
   On success, the number of bytes added to the target.

   If arg is not a (FILE*) then whclob_rc.ArgError is returned.

   The file handle is not closed by this routine.
*/
long whclob_importer_FILE( whclob * target, void * arg );

/**
   A whclob_importer implementation similar to whclob_importer_FILE(), but
   expects arg to be a (char const *) filename. Returns the same as
   the equivalent whclob_importer_FILE() call unless arg cannot be
   opened as a file, in which case whclob_rc.IOError is returned.
*/
long whclob_importer_filename( whclob * target, void * arg );

/**
   Sends cb's blob to the given file handle, which must have been
   previously opened. Returns the number of bytes written on success
   or a negative number (probably whclob_rc.IOError) on error.

   The file handle is not closed by this routine.
 */
long whclob_export_FILE( whclob const * cb, FILE * dest );

/**
   Identical to whclob_export_to_FILE() but takes a file
   name.
*/
long whclob_export_filename( whclob const * cb, char const * dest );

/**
   The read counterpart of whclob_export_FILE(), appends all data
   from src to cb. Return value is as for whclob_import().
*/
long whclob_import_FILE( whclob * cb, FILE * src );

/**
   Identical to whclob_import_FILE() but takes a file
   name.
*/
long whclob_import_filename( whclob * dest, char const * src );
#endif /* WHCLOB_USE_FILE */

#if WHCLOB_USE_ZLIB
/**
   Compresses the contents of src to dest using the zlib compress()
   function. dest may be either the same as src or an initialized
   clob. If dest is not src then this function will clear dest's
   contents whether or not this routine succeeds. If (dest==src)
   then src is only modified if this routine succeeds.

   Returns whclob_rc.OK on success, some other value on error:

   - whclob_rc.IOError: compression failed

   - whclob_rc.AllocError = a (re)allocation failed.

   Note that the compressed data is compressed using zlib but contains
   its own header, so it will not be usable by tools like gunzip.
*/
int whclob_compress( whclob * src, whclob * dest );

/**
   The converse of whclob_compress(), src is expected to be clob containing
   data compressed with whclob_compress(). dest must be an initialized clob.
   If dest is not the same as src then dest will be reset whether not
   this routine succeeds. If (dest==src) then it is only modified
   if this routine succeeds.

   Returns whclob_rc.OK on success, some other value on error:

   - whclob_rc.RangeError: source does not appear to contain any
   compressed data or data was written by a different version of this
   API.

   - whclob_rc.ArgError: input is too small to contain the compression
   header.

   - whclob_rc.IOError: decompression failed

   - whclob_rc.AllocError = a (re)allocation failed. 

*/
int whclob_uncompress( whclob * src, whclob * dest );


/**
   Works equivalently to whclob_compress(), but uses the zlib deflate()
   algorithm instead of the compress() algorithm.
*/
int whclob_deflate( whclob *cIn, whclob *cOut );

/**
   The converse of whclob_deflate(), this works equivalently to
   whclob_uncompress(), but uses the zlib inflate() algorithm instead of
   the uncompress() algorithm.
*/
int whclob_inflate( whclob *cIn, whclob *cOut );
#endif /* WHCLOB_USE_ZLIB */

#if WHCLOB_USE_BASE64
/**
   Encodes cIn's contents in base64 and sends it to cOut.
   cIn may be the same as cOut. If cOut is not cIn then
   it is cleared before encoding begins. On success, cOut is populated with
   the encoded data and whclob_rc.OK is returned. On error:

   - If (!cIn, !cOut, or !whclob_size(cIn)) then whclob_rc.ArgError
   is returned and cOut is unmodified.

   - On any other error, if (cIn!=cOut) then cOut will contain no
   data, otherwise cIn/cOut will be unmodified. In these cases some
   other error code from whclob_rc will be returned.
*/
long whclob_base64_enc( whclob const *cIn, whclob *cOut );

/**
   The converse of whclob_base64_enc(), with the same
   conventions.
*/
long whclob_base64_dec( whclob const *cIn, whclob *cOut );
#endif /* WHCLOB_USE_BASE64 */


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* WANDERINGHORSE_NET_WHCLOB_H_INCLUDED_ */
