#ifndef S11N_NET_VAPPENDF_H_INCLUDED_
#define S11N_NET_VAPPENDF_H_INCLUDED_ 1
#include <stdarg.h>
#include <ostream>
namespace s11n {

/**
   This code implements a printf-like implementation which supports
   aribtrary data destinations.

   Authors: many, probably. This code supposedly goes back to the
   early 1980's.

   License: Public Domain.
*/

/**
   The vappendf_appender typedef is used to provide vappendf()
   with a flexible output routine, so that it can be easily
   send its output to arbitrary targets.

   The policies which implementations need to follow are:

   - arg is an implementation-specific pointer (may be 0) which is
   passed to vappendf. vappendf() doesn't know what this argument is
   but passes it to its vappendf_appender. Typically it will be an
   object or resource handle to which string data is pushed or output.

   - The 'data' parameter is the data to append. If it contains
   embedded nulls, this function will stop at the first one. Thus
   it is not binary-safe.

   - n is the number of bytes to read from data. If n<0 then
   strlen(data) should be used.

   - Returns, on success, the number of bytes appended (may be 0).

   - Returns, on error, an implementation-specified negative number.
   Returning a negative error code will cause vappendf() to stop the
   processing of that string. Note that 0 is a success value (some
   printf format specifiers do not add anything to the output).
*/
typedef long (*vappendf_appender)( void * arg,
				   char const * data,
				   long n );


/*
  This function works similarly to classical printf implementations,
  but instead of outputing somewhere specific, it uses a callback
  function to push its output somewhere. This allows it to be used for
  arbitrary external representations. It can be used, for example, to
  output to an external string, a UI widget, or file handle (it can
  also emulate printf by outputing to stdout this way).

 INPUTS:

 pfAppend : The is a vappendf_appender function which is responsible
 for accumulating the output. If pfAppend returns a negative integer
 then processing stops immediately.

 pfAppendArg : is ignored by this function but passed as the first
 argument to pfAppend. pfAppend will presumably use it as a data
 store for accumulating its string.

 fmt : This is the format string, as in the usual printf().

 ap : This is a pointer to a list of arguments.  Same as in
 vprintf() and friends.


 OUTPUTS:

 The return value is the total number of characters sent to the
 function "func", or a negative number on a pre-output error. If this
 function returns an integer greater than 1 it is in general
 impossible to know if all of the elements were output. As such
 failure can only happen if the callback function returns an error,
 and this type of error is very rare in a printf-like context, this is
 not considered to be a significant problem. (The same is true for any
 classical printf implementations, as far as i'm aware.)


 CURRENT (documented) PRINTF EXTENSIONS:

 %z works like %s, but takes a non-const (char *) in the va_list, and
 vappendf deletes the string (using free()) after appending it to the
 output.

 %h (HTML) works like %s but converts certain characters (like '<' and '&' to
 their HTML escaped equivalents.

 %t (URL encode) works like %s but converts certain characters into a representation
 suitable for use in an HTTP URL. (e.g. ' ' gets converted to %20)

 %T (URL decode) does the opposite of %t - it decodes URL-encoded
 strings.

 %n requires an (int*) in the va_list, which gets assigned
 the number of bytes this function has appended to cb so far.

 %r requires an int and renders it in "ordinal form". That is,
 the number 1 converts to "1st" and 398 converts to "398th".

 %q quotes a string as required for SQL. That is, '\'' characters get doubled.

 %Q as %q, but includes the outer '\'' characters and null pointers
 replaced by SQL NULL.

 (The %q and %Q specifiers are options inherited from this printf
 implementation's sqlite3 genes.)

*/
long vappendf(
  vappendf_appender pfAppend,          /* Accumulate results here */
  void * pfAppendArg,                /* Passed as first arg to pfAppend. */
  const char *fmt,                   /* Format string */
  va_list ap                         /* arguments */
  );

/**
   Identical to vappendf() but does not take a va_list.
*/
long appendf(vappendf_appender pfAppend,
	     void * pfAppendArg,
	     const char *fmt,
	     ... );

/**
   An overload which sends its output to the given ostream.
*/
long vappendf( std::ostream & target,
	       const char *fmt,
	       va_list ap
	       );

/**
   An overload which sends its output to the given ostream.
*/
long appendf( std::ostream & target,
	      const char *fmt,
	      ... );

} /* extern "C" || namespace */
#endif // S11N_NET_VAPPENDF_H_INCLUDED_
