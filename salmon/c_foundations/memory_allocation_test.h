/* file "memory_allocation_test.h" */

/*
 *  This file contains the interface to code to help test the dynamic memory
 *  allocation and deallocation of other pieces of software.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef MEMORY_ALLOCATION_TEST_H
#define MEMORY_ALLOCATION_TEST_H

#include <stddef.h>


/*
 *      Usage
 *
 *  This module is intended to help test other software that uses malloc() and
 *  free() to dynamically allocate and de-allocate memory.
 *
 *  To use this code to test some target code, you should get all calls to
 *  malloc() in the target code replaced with calls to test_malloc() from this
 *  module and all calls to free() in the target code replaced with calls to
 *  test_free() from this module.  For example, you might compile your target
 *  code with ``-Dmalloc=test_malloc -Dfree=test_free'' on the command line (be
 *  careful, though -- if malloc() and/or free() is a macro in your system
 *  header files, this may not work right).  Note that you must be careful not
 *  to compile this module that way because this module must have access to the
 *  real malloc() and free() functions.
 *
 *  IMPORTANT NOTE: You must not mix test_malloc() and test_free() with the
 *  real malloc() and free() in the sense that if you allocate with
 *  test_malloc() you must not use free() to free that block and if you
 *  allocate with malloc() you must not use test_free() to free that block.
 *
 *  The test_malloc() and test_free() macros are designed to catch certain
 *  kinds of errors related to memory allocation and deallocation.  One thing
 *  they do is put padding before and after the blocks of memory that are
 *  returned by test_malloc(), with special patterns written in the padding.
 *  The test_free() macro tests to make sure that the padding blocks are
 *  unmodified.  Also, this module keeps a list of all allocated blocks and
 *  tests to make sure when one is freed that it had been allocated before and
 *  that it hadn't already been freed.  If this module finds any errors, it
 *  prints a message to stderr and exits with an error code.
 *
 *  This module considers trying to allocate zero bytes as an error and prints
 *  an error message and exits when that happens.  Note that this is more
 *  restrictive than ANSI C, which says allocating zero bytes is legal.  But
 *  ANSI C leaves it implementation-defined whether that returns NULL or a
 *  valid, unique pointer, so it's not very portable.  Also, it's not very
 *  useful to malloc zero bytes, so there's a good chance that there's a bug if
 *  the code being tested tries to do so.  For these reasons, my own personal
 *  coding style is to never call malloc with zero as the argument, so I've
 *  written this module to consider that an error.  Note, however, that it
 *  would be pretty trivial to change this code to make malloc(0) legal and to
 *  either make it return NULL or make it return a valid, unique pointer (with
 *  padding to make sure the user doesn't write to it).
 *
 *  Similarly, if a NULL pointer is freed, this module considers it an error
 *  and prints an error message to stderr and exits.  Calling free(NULL) is
 *  legal in ANSI C, so this is another case of this module being more strict
 *  than ANSI C.  The reason is that I personally prefer a stricter coding
 *  style where free() is never called on a pointer if I think it might be
 *  NULL.  That way, if this module catches a free() of NULL, it's because some
 *  pointer I don't think could be NULL actually is, so there's something going
 *  on in my code that I didn't expect.  As with malloc(0), it's trivial to
 *  change this module to allow free(NULL) and have it have no effect.
 *
 *  Note that this module trades error catching for efficency.  It is not
 *  suitable for production use.  It should only be used for testing purposes
 *  and it will not be able to handle really large tests.  It's inefficient in
 *  both time and space.  For example, it keeps a list of allocated blocks and
 *  searches that list (in time proportional to the number of allocations)
 *  every time something is freed.  It also never deallocates any memory, to
 *  make sure that reallocation of the same memory doesn't obscure any bugs.
 *
 *  Note that test_malloc() and test_free() are implemented as macros that call
 *  the underlying functions test_malloc_implementation() and
 *  test_free_implementation().  This is so the source file name and line
 *  number of the callsites to test_malloc() and test_free() can be captures
 *  and used to give more useful information in the event that a memory error
 *  is caught.
 *
 *  In addition to the test_malloc() and test_free() macros, there are some
 *  other macros and functions provided by this interface that can be useful in
 *  testing memory allocation.
 *
 *  The assert_is_malloced_block(),
 *  assert_is_malloced_block_with_minimum_size(), and
 *  assert_is_malloced_block_with_exact_size() macros can be used to instrument
 *  the code being tested.  Calls to these macros should be put in the code
 *  being tested in a way that they will not be called when the code is in
 *  production use instead of being tested with this library.  These macros
 *  must not be called on blocks allocated with malloc() instead of
 *  test_malloc().  The usual way to do this would be to use macros that
 *  evaluate to nothing when not testing the code.
 *
 *  Each of those three macros tests that a given pointer points to an object
 *  that was allocated and not yet deallocated, and checks that the buffers
 *  before and after the block of allocated memory are undisturbed.  That's all
 *  the assert_is_malloced_block() macro does.  The
 *  assert_is_malloced_block_with_minimum_size() call also takes a size and
 *  makes sure that the given block is at least that size.  Finally, the
 *  assert_is_malloced_block_with_exact_size() tests that the given block is
 *  exactly the given size.
 *
 *  As with test_malloc() and test_free(), the three assert_is_malloced_*()
 *  interfaces are implemented as macros calling underying functions in order
 *  to capture callsite source file and line number information.
 *
 *  The final six functions provided by this module -- unfreed_blocks(),
 *  check_blocks(), describe_unfreed_blocks(), set_memory_allocation_limit(),
 *  clear_memory_allocation_limit(), and get_memory_allocation_count() -- are
 *  intended for use by testing code.  The unfreed_blocks() call returns the
 *  total number of blocks that were allocated by test_malloc() and never
 *  deallocated by test_free().  This is intended for use in finding memory
 *  leaks.  The check_blocks() function checks for bad writes, either to the
 *  padding before and after each block or to the block itself after
 *  test_free() has been called.  Note that check_blocks() isn't guaranteed to
 *  find all such bugs, but it has a good chance of detecting them, since
 *  test_free() sets the whole block to a standard pattern and check_blocks()
 *  will check that the pattern is still there.  The same applies to the
 *  padding before and after allocated blocks.
 *
 *  The describe_unfreed_blocks() function is another function indended for use
 *  in tracking down memory leaks.  It prints out an error message for each
 *  block that is still unfreed, describing its size and where it was
 *  allocated.  The set_memory_allocation_limit() function is used to test the
 *  handling of out-of-memory situations.  It is passed the number of
 *  successful calls to test_malloc() will be allowed before subsequent calls
 *  return NULL.  The clear_memory_allocation_limit() function clears any such
 *  limits to restore the behavior that all calls to test_malloc() will succeed
 *  (assuming that the real malloc() doesn't run out of memory, of course).
 *  And finally the get_memory_allocation_count() tells the testing code how
 *  many allocations have been done.  This is intended to allow the testing
 *  code to choose appropriate values when calling
 *  set_memory_allocation_limit().
 *
 *
 *      Error Handling
 *
 *  These functions are designed to test the way the test_malloc() and
 *  test_free() functions are used.  If they detect any misuse of these
 *  functions, they will print an appropriate error message to stderr and exit
 *  with a non-zero error code.
 *
 *  There is only one of these functions, test_malloc(), that can ever fail to
 *  do what it is asked to do (unless, of course, there are bugs in the
 *  implementation of this module, bugs in the compiler compiling this module,
 *  or memory corruption bugs that allow code outside this module to corrupt
 *  the internal data structures of this module -- which, in fact, could happen
 *  if the code being tested seriously misuses test_malloc() by writing way
 *  beyond an allocated block).  The only time test_malloc() can fail to behave
 *  as requested is if the real malloc() fails because there really isn't
 *  enough memory available.  In that case, test_malloc() will print an error
 *  message to stderr and exit with a non-zero error code.
 *
 *
 *      Requirements
 *
 *  This requires only an ANSI C compiler.
 *
 *
 *      Design Goals
 *
 *  There are many more sophisticated systems in existence for testing and
 *  debugging memory allocation problems.  Many of them are superior to this
 *  one in various ways and you may well be better off using one of those.  So
 *  it's natural to ask why I wrote this one.  Here are the reasons I wrote
 *  this:
 *
 *    * I wanted to be able to simulate out-of-memory conditions.  The other
 *      memory bug tracking schemes I'm aware of don't allow the kind of
 *      simulated out-of-memory control that I put in this module.
 *
 *    * It was faster to write this module than to try to find what I'm looking
 *      for in some existing system and learn how to use that other system.  I
 *      designed and implemented this module all in one day (Saturday, November
 *      1, 2003).
 *
 *    * I wanted something that I could include in packages that I release as
 *      public domain code.  So other sorts of ``free'' or ``open'' software,
 *      including GPL or Berkeley-style licensed copyrighted code don't meet my
 *      requirements.
 *
 *    * I wanted something that would be very portable.  Many of the more
 *      sophisticated (and useful) memory bug tracking schemes require compiler
 *      support or are otherwise limited to particular platforms.
 *
 *    * By using this code that I wrote myself, I can have a better assurance
 *      of its quality.  If I had used code from elsewhere, I couldn't be as
 *      certain of its quality.
 *
 *    * Using code that I wrote myself makes it less likely that I'll have
 *      problems caused by misunderstanding exactly how the code works.
 *
 *    * I enjoy creating this module.
 *
 *    * I like the idea of using software I wrote as much as possible as
 *      opposed to software someone else wrote -- it's an ego boost.
 *
 *  The choice of C as the implementation language was dictated by the fact
 *  that I have a couple of projects that I've chosen to do in C that could use
 *  this functionality.  The reason for using C in those projects is that it is
 *  so widely used and available, not particularly that C itself is
 *  intrinsically particularly suited for the task.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, in 2003 and placed in the public
 *  domain at that time.  As far as I'm aware, it's completely original.  I
 *  haven't written similar code before that I can remember and I'm not basing
 *  this on any similar code.  I am, however, aware of the theory of operation
 *  of some other memory bug finding systems, such as Purify and some systems
 *  whose names I don't know that put wrappers on malloc() and free().  This
 *  system is somewhat different from those, though some of the ideas in this
 *  system may have been influenced by ideas from those systems.  This code is
 *  not based on the code to implement any other such system -- in fact, as far
 *  as I can remember, I've never even looked at the code to implement any
 *  other such system.
 *
 *  The immediate impetus to design and implement this code was to test the
 *  string_index module that I've just written.  I designed it with the
 *  intention that it could be used to test other code that uses malloc() and
 *  free(), though, including the arbitrary-precision integer arithmetic code
 *  I'm partway through implementing for the CWX1 interpreter as well as many
 *  other pieces of software.  I intend to publicly release it as a stand-alone
 *  module for anyone else to use, in case others find it useful too, as well
 *  as to include it in other pieces of public domain software I release.
 *
 *  In 2008, I modified this module to capture source file and line number
 *  information and use that in all diagnostic messages.
 *
 *
 *      Legal Issues
 *
 *  I've written this code from scratch, without using or refering to any other
 *  code, on my own equipment and not for hire for anyone else, so I have full
 *  legal rights to place it in the public domain.
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
 *          Chris Wilson, 2003
 */

#define test_malloc(size)  test_malloc_implementation(size, __FILE__, __LINE__)
extern void *test_malloc_implementation(size_t size, const char *file_name,
                                        size_t line_number);

#define test_free(ptr)  test_free_implementation(ptr, __FILE__, __LINE__)
extern void test_free_implementation(void *ptr, const char *file_name,
                                     size_t line_number);

#define assert_is_malloced_block(pointer) \
        assert_is_malloced_block_implementation(pointer, __FILE__, __LINE__)
extern void assert_is_malloced_block_implementation(void *pointer,
        const char *file_name, size_t line_number);

#define assert_is_malloced_block_with_minimum_size(pointer, size) \
        assert_is_malloced_block_with_minimum_size_implementation(pointer, \
                size, __FILE__, __LINE__)
extern void assert_is_malloced_block_with_minimum_size_implementation(
        void *pointer, size_t size, const char *file_name, size_t line_number);

#define assert_is_malloced_block_with_exact_size(pointer, size) \
        assert_is_malloced_block_with_exact_size_implementation(pointer, \
                size, __FILE__, __LINE__)
extern void assert_is_malloced_block_with_exact_size_implementation(
        void *pointer, size_t size, const char *file_name, size_t line_number);

extern size_t unfreed_blocks(void);
extern void check_blocks(void);
extern void describe_unfreed_blocks(void);
extern void set_memory_allocation_limit(size_t allocation_count);
extern void clear_memory_allocation_limit(void);
extern size_t get_memory_allocation_count(void);


#endif /* MEMORY_ALLOCATION_TEST_H */
