/* file "merge_dense_integer_arrays.h" */

/*
 *  This file contains the interface to code to do an efficient merge of a
 *  number of arrays of integers while maintaining the ordering of the arrays
 *  as much as possible, where the integers in the arrays are in the range 0 to
 *  N inclusive for some N and every n in the range appears in at least one of
 *  the arrays and no n appears more than once in the same array.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef MERGE_DENSE_INTEGER_ARRAYS_H
#define MERGE_DENSE_INTEGER_ARRAYS_H

#include "basic.h"
#include <stddef.h>


/*
 *      Usage
 *
 *  This interface consists of a single function, merge_dense_integer_arrays().
 *  The first argument to this function is num_elements.  This is one more than
 *  N, the maximum for all the integers elements in all the arrays.  Every
 *  element is in the range zero to num_elements-1 inclusive.  The value of
 *  num_elements must be positive.
 *
 *  This second argument to merge_dense_integer_arrays() is num_arrays.  This
 *  specifies how many arrays are to be merged.  It can be any finite integer
 *  greater than zero (though a value of one is a trivial case).
 *
 *  The third argument to merge_dense_integer_arrays() is array_sizes.  This
 *  parameter is a pointer to an array of integers specifying the sizes of the
 *  arrays to be merged.  The arrays may all have different sizes.  There is
 *  one element of array_sizes for each array to be merged, so array_sizes has
 *  num_arrays elements.  Each array is required to have at least one element,
 *  so every element of array_sizes is required to be one or greater.
 *
 *  The fourth and final argument to merge_dense_integer_arrays() is
 *  ``arrays''.  This is a pointer to an array of pointers where each of the
 *  elements points to one of the arrays of integers to be merged.  Each
 *  element of the arrays pointed to by the elements of ``arrays'' must be in
 *  the range from zero to num_elements-1 inclusive.  Any given integer can
 *  appear at most once in any given array, but may appear in arbitrarily many
 *  different arrays.  In fact, if no element appears in more than one array,
 *  this function is trivial, so it's only in the case of elements appearing in
 *  more than one array that this function becomes interesting.
 *
 *  Note that since every integer from zero to num_elements is required to
 *  appear in at least one of the arrays (this is what we mean by ``dense''),
 *  num_elements is less than or equal to the sum of the number of entries in
 *  all the arrays.  So num_elements can be given type size_t without loss of
 *  generality since it's bounded by a count of memory locations, and since all
 *  the elements in all the arrays are bounded by zero and num_elements, every
 *  element can likewise be given the type size_t without loss of generality.
 *
 *  This function returns a pointer to the merged array.  The number of
 *  elements in the resulting array will always be num_elements because every
 *  element of all the arrays to be merged will appear once in the result.  The
 *  resulting array is malloc()'ed, so it's up to the caller to free the array
 *  when the caller is done with it.
 *
 *  This function does not change anything pointed to by any of its parameters,
 *  directly or indirectly, nor does it free any memory pointed to by any of
 *  its parameters, directly or indirectly.
 *
 *
 *      Error Handling
 *
 *  There are three types of errors this code can encounter:
 *
 *   1) Errors in the parameters that can be detected by this code.
 *   2) Errors in the parameters that cannot be detected by this code.
 *   3) A failure to be able to allocate memory.
 *
 *  The first type of error -- an error in a parameter that this code can
 *  detect -- is handled by failing an assertion.  If this code is compiled
 *  with assert() as a no-op, as it often is in production code, the code will
 *  not check for these errors and it may fail in strange ways because of them.
 *  This code can detect and assertion-fail the following errors in its
 *  parameters: num_elements is less than one; num_arrays is less than one;
 *  array_sizes is NULL; ``arrays'' is NULL; one of the elements of ``arrays''
 *  is NULL; one of the elements of array_sizes is less than one; one of the
 *  elements of one of the arrays to merge is not in the range from zero to
 *  num_elements-1 (inclusive); and not every integer in the range zero to
 *  num_elements-1 (inclusive) appears in one of the arrays.
 *
 *  The second type of error -- an error in a parameter that is not detected by
 *  this code -- may cause any sort of strange behavior or crash, or it may
 *  cause no noticible effect.  Any error in the parameters that is not covered
 *  by the first case is included here.  Examples include having ``arrays''
 *  point to garbage, having one of the elements of ``arrays'' point to
 *  garbage, and having fewer elements in ``arrays'' than specified by
 *  num_arrays.  Behavior in most of these cases is system dependent and may
 *  vary from one run to the next on the same system.
 *
 *  The third type of error is a failure by this code to allocate memory,
 *  either for the result array or for some internal data structure.  This
 *  happens when malloc() returns NULL.  In this case, this function prints an
 *  error message to standard error and returns NULL.
 *
 *  Of course, the foregoing doesn't apply if there are bugs in this module, if
 *  there are bugs in the compiler compiling this module that cause incorrect
 *  code to be generated for it, or if external memory corruption bugs corrupt
 *  the data structures or code used by this module.  In those cases, all bets
 *  are off and there's no way to guarantee what might happen.
 *
 *
 *      Requirements
 *
 *  This requires only an ANSI C compiler and the "basic.h" header file.  The
 *  "basic.h" header file contains some basic definitions I use in all my C
 *  code, such as a boolean type.  You should be able to find "basic.h" in the
 *  same place you found this file.
 *
 *
 *      Design Goals
 *
 *  This module is designed to reconcile possibly conflicting ordered lists of
 *  elements in a way that minimizes the conflicts between the result and all
 *  the original lists.  For example, suppose there are several different
 *  manuscripts that represent different variations of a given epic poem.  Most
 *  of the text of all the versions coincides, but there are some verses that
 *  appear in only some of the manuscripts, and some that are ordered
 *  differently in different manuscripts.  One could assign each verse a unique
 *  number and call this function with each list representing one of the
 *  manuscripts.  The result would be a version that includes all the verses
 *  that appear in any of the manuscripts and where there are variant orderings
 *  it would choose an ordering that conflicts with as few of the manuscripts
 *  as possible.  This is a somewhat artificial example, because in reality for
 *  a poem semantic and historical clues, rather than just numbers of
 *  manuscripts with each ordering, would be used; some manuscripts would be
 *  given greater weight than others; and not all verses appearing in only some
 *  manuscripts would be preferred (some might be undesirable later
 *  accretions).  The example is only used to clearly illustrate the point.  In
 *  fact, this function is intended more for use in parsing and compiling or
 *  otherwise handling computer languages, when different declarations of some
 *  sort of code or datastructure are to be reconciled as a union and it is
 *  desirable, but not necessary, to preserve the original ordering in as many
 *  of the sources as possible.  There are probably other optimization
 *  applications in which it would be useful, for it is fundamentally an
 *  optimization function.
 *
 *  The details of the implementation are hidden so that a different
 *  implementation can be substituted if there is a reason to do so in the
 *  future and that substitution will not require changes to the client code.
 *
 *  Note that the elements of all the arrays are specified as integers not
 *  because the elements of the ultimate problem to be solved are expected to
 *  be in this form, but because this is a convenient, generic way to specify
 *  different elements of a finite set of arbitrary elements.  It is up to the
 *  client code to convert from arbitrary integers, strings, floating-point
 *  values, pointers to objects, or whatever the real elements are into a
 *  format where each element is represented by a unique integer and the
 *  integers are dense.  There's no practical way I can think of in C to write
 *  a generic interface that can deal with all the possible underlying types
 *  and methods for deciding which to equate, and anyway a conversion to an
 *  enumeration of all the elements would be done somewhere along the line in
 *  any case to get the data into a more easily handled form.
 *
 *  The space and time taken by this function are linear in the number of
 *  elements in all the arrays to be merged.  Note that this is the total
 *  number of elements in these arrays, not the number of unique elements, so
 *  it can be significantly larger than num_elements.  It is, rather, the sum
 *  of the elements of the array pointed to by array_sizes.
 *
 *  The choice of C as the implementation language was dictated by the fact
 *  that I have a project that I've chosen to do in C that could use this
 *  functionality.  The reason for using C in that project is that it is so
 *  widely used and available, not that C itself is intrinsically particularly
 *  suited to the task.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, in 2004 and 2005 and placed in
 *  the public domain at that time.  I came up with the idea for it just before
 *  writing it and it isn't based on any other code or anything else from
 *  anyone else.  I intend it first for one particular project I'm working on,
 *  the Snapper program for Linux kernel configuration, and that was the
 *  motivation for creating it.  But I'll also make it available by itself, in
 *  case anyone else finds it useful to incorporate into their own code.
 *
 *
 *      Legal Issues
 *
 *  I've written this code from scratch, without using or refering to any other
 *  code.  I've written it on my own equipment and not for hire for anyone
 *  else, so I have full legal rights to place it in the public domain.
 *
 *  I've chosen to put this software in the public domain rather than
 *  copyrighting it and using the FSF's GPL or a Berkeley-style ``vanity''
 *  license because my personal opinion is that making it public domain
 *  maximizes its potential usefulness to others.  Anyone can feel free to use
 *  it for any purpose, including with their own proprietary code or with GPL
 *  code, without fear of intellectual property issues.  I have no desire to
 *  stop anyone else from making money on this code or getting any other
 *  advantages they can from it.
 *
 *  I do request that anyone who finds this software useful let me know about
 *  it.  You can drop me e-mail at "Chris Wilson" <chris@chriswilson.info> to
 *  let me know how you are using it and what is good and bad about it for you.
 *  Bug reports are also appreciated.  Also, if you release a product or
 *  software package of some sort that includes this software, I would like you
 *  to give me credit in the documentation as appropriate for the importance of
 *  my code in your product.  These are requests, not requirements, so you are
 *  not legally bound to do them, they're just a nice way to show appreciation.
 *
 *  Note that even though this software is public domain and there are no
 *  copyright laws that limit what you can do with it, other laws may apply.
 *  For example, if you lie and claim that you wrote this code when you did
 *  not, or you claim that I endorse a product of yours when I do not, that
 *  could be fraud and you could be legally liable.
 *
 *  There is absolutely no warranty for this software!  I am warning you now
 *  that it may or may not work.  It may have bugs that cause you a lot of
 *  problems.  I disclaim any implied warranties for merchantability or fitness
 *  for a particular purpose.  The fact that I have written some documentation
 *  on what I intended this software for should not be taken as any kind of
 *  warranty that it will actually behave that way.  I am providing this
 *  software as-is in the hope that it will be useful.
 *
 *          Chris Wilson, 2004-2005
 */

extern size_t *merge_dense_integer_arrays(size_t num_elements,
        size_t num_arrays, const size_t *array_sizes,
        const size_t * const *arrays);


#endif /* MERGE_DENSE_INTEGER_ARRAYS_H */
