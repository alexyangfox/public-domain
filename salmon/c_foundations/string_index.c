/* file "string_index.c" */

/*
 *  This file contains the implementation of code to do an efficient index with
 *  ASCII strings as keys and ``void *'' pointers as the indexed data.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include "string_index.h"
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include "memory_allocation.h"
#include "code_point.h"


/*
 *      Data Structures
 *
 *  The primary data structure here is the string_index structure.  The basic
 *  idea is to have a tree of objects representing all the keys.  As you follow
 *  along the characters of a key string, you walk down through the tree.  Once
 *  you reach the end of the key string, you take the value hanging off that
 *  object as the value associated with that key.
 *
 *  The implementation here is optimized a bit from the most obvious tree
 *  structure to represent key strings.  First I'll describe the simpler model
 *  and then I'll describe how this implementation is optimized from that
 *  simpler model.  After I've described both the simpler and the optimized
 *  model, I'll discuss how the optimized model maps to the C string_index
 *  structure.  I won't relate it directly to the string_index structure until
 *  I'm done discussing both of the models, so trying to look at the structure
 *  while reading about the models will probably not be helpful.
 *
 *  In the simpler model, if you are looking up a key, you move forward through
 *  the key one character at a time and each time you move over a character,
 *  you move on to a different object.  So each object can potentially have as
 *  many successor objects as there are characters.  If you have the standard
 *  8-bit characters, then each object can have up to 255 successor object
 *  pointers, one for each of the possible characters (it's 255 and not 256
 *  because the value zero for a character signals the end of the string).  The
 *  object can also potentially have a single ``void *'' for the value
 *  associated with the key that ends there.  And that's all each object would
 *  need to have -- a set of up to 255 (or however many valid characters there
 *  are) pointers associated with the different characters and a single
 *  ``void *''.
 *
 *  For example, if you had a string index with the key/value pairs
 *  ("", "empty"), ("aaa", "3a"), ("abb", "a2b"), and ("aa", "2a") a picture of
 *  how it might be represented in the simpler model might be:
 *
 *
 *                        +---------------+
 *                        | value="empty" |
 *                        +---------------+
 *                                |
 *                                | 'a'
 *                                |
 *                        +---------------+
 *                        |  value=NULL   |
 *                        +---------------+
 *                   'a'     |         |     'b'
 *              +------------+         +------------+
 *              |                                   |
 *      +---------------+                   +---------------+
 *      |  value="2a"   |                   |  value=NULL   |
 *      +---------------+                   +---------------+
 *              |                                   |
 *              | 'a'                               | 'b'
 *              |                                   |
 *      +---------------+                   +---------------+
 *      |  value="3a"   |                   |  value="a2b"  |
 *      +---------------+                   +---------------+
 *
 *
 *  The first optimization that I made from this simpler model to the
 *  implementation here is based on the observation that there will often be
 *  objects in the simpler model with exactly one successor.  These correspond
 *  to substrings such that any of the keys with part of that substring have
 *  that whole substring.  For example, if you look at all the identifiers in
 *  this file that start with the prefix "fol", they are all "follow".  So if
 *  we can collapse strings of objects that have a single successor each and no
 *  value associated with any of them, we can save a lot of space in many cases
 *  and still represent everything.  So into each object I added a ``prefix''
 *  field.  This field contains a string of characters representing a series of
 *  objects that in the simpler model would preceed this object.  When you're
 *  doing a lookup and you get to such an object, you then check to see that
 *  the prefix matches the next characters in the key string.  If not, your key
 *  isn't in the index and you're done.  If they do match, you skip those
 *  characters in the key and go on from there.
 *
 *  The next optimization has to do with how to represent the successor
 *  pointers.  In fact, the simpler model didn't even specify how to represent
 *  them, it just said that there could be up to one for each possible
 *  character and each character was associated with at most one of them.  One
 *  way to represent that would be to build into each object an array of
 *  successor object pointers with the array size equal to the number of
 *  characters and use the character as the index into that array.  That is
 *  effecient in time because if you have a character and you want to get to
 *  the successor object for that character (or find out if there isn't one),
 *  you just do an array lookup.  If the result is NULL it means there is no
 *  successor and if it's non-NULL it is the pointer to that successor.
 *  However, it's not very efficient in terms of space if the tree is sparse --
 *  that is, if the average number of successors per object is small compared
 *  to the number of characters in the character set.
 *
 *  Another way to represent the successor pointers would be as a list of
 *  character/successor pairs.  This would be space-efficient for the case of
 *  sparse trees but it would be time inefficient for the case of objects with
 *  many successors because lookup (and any of the other operations, in fact)
 *  would require on average searching through half the keys for the object to
 *  find the successor.  Keeping the list ordered by key and keeping it in an
 *  array instead of a linked list could cut the search time to the log of the
 *  number of characters, but that's still a penalty from the array-indexed-by-
 *  character alternative and it makes inserting and deleting elements more
 *  expensive.
 *
 *  What I chose to implement here was a hybrid of the two schemes I've talked
 *  about for keeping track of successors.  For small numbers of successors,
 *  this code keeps the successor information in the form of a small array of
 *  characters and a small array of matching successors.  I call this the
 *  ``short array'' method.  For larger numbers of successors, this code keeps
 *  an array of successors indexed by the character.  I call this the ``table''
 *  method.  The cut-off number of successors to switch from the first method
 *  to the second method is determined by the SHORT_ARRAY_MAX_ENTRIES macro.
 *  If the number of successors is less than or equal to this value, the short
 *  array method is used.  Once the number of successors exceeds this value,
 *  the table method is used.  Note that once an object has been converted to
 *  using the table method, it never goes back, even if the number of
 *  successors later drops because of ``remove'' operations.  I chose the value
 *  4 for SHORT_ARRAY_MAX_ENTRIES simply because it seemed like a reasonable
 *  choice to me.  That's a purely subjective choice and I haven't done any
 *  real tests to base it on.  It just seems to me that in most cases 4 will be
 *  enough, that having to search through a list of at most 4 successors isn't
 *  too time consuming (on average you have to search through 2 items in a list
 *  of 4), and that if there are 5 successors anyway, we can just take the
 *  memory hit and allocate a table.  I believe I've used 4 as the cut-off in a
 *  previous implementation of similar functionality, possibly for SUIF, but I
 *  never made any measurements to see how it was working.  At some point it
 *  might be useful to do some experiments to measure the time-space trade-offs
 *  for real-world applications of this code with different values for this
 *  cut-off.  That will give more information on which to base a cut-off value
 *  decision, though the relative importance of time versus space will be
 *  subjective and vary from application to application (as will the data set,
 *  which might affect the trade-off).  For now, though, I think that this is
 *  good enough for most applications.
 *
 *  The next optimization I made had to do with the implementation of the table
 *  method.  This is made complicated by the fact that the C standard doesn't
 *  define the size of the ``char'' type and this is the type I used for the
 *  characters in my keys.  In the vast majority of implementations, including
 *  all the mainstream implementations of C I've seen, such as all C compilers
 *  for x86, MIPS, SPARC, etc., characters are 8 bits, and there's a lot of
 *  code that assumes characters are 8 bits.  Yet the standard only says
 *  characters must be at least 8 bits and specifies a macro to tell a program
 *  how many bits in the ``char'' type of a particular implementation.  And in
 *  fact I have worked with a compiler that used a 16-bit type for ``char'' (at
 *  Silicon Spice, for a now-defunct architecture that had only 16-bit
 *  operations natively).  If I know that characters are only 8 bits, I am
 *  happy to just allocate a table with 256 entries for the table
 *  implementation and never have to worry about re-allocating and copying data
 *  to resize the table.  But I'd really like this code not to rely on anything
 *  ANSI C doesn't guarantee, and if ``char'' is 16 bits (or even 32 bits), it
 *  would be a disaster to allocate a table that covers the whole range of the
 *  ``char'' type.
 *
 *  So I decided to put in two different table implementations, one for the
 *  8-bit ``char'' case and the other for the general case.  In the 8-bit case
 *  I just allocate a 256-element array of pointers (yes, it's one too many
 *  because the zero element will never be used, but that extra element saves a
 *  subtraction of one in the indexing -- on some architectures that would be
 *  free, but on some the extra element might make the code slightly faster
 *  when ``char'' is unsigned (for the signed case I still have to add 128 to
 *  allow for negative characters) and for less than half a percent increase in
 *  size).  In the general case it's more complicated.  When ``char'' is a
 *  large type, chances are the characters used in key strings map into a
 *  small, contiguous range of values within the large range of values for the
 *  ``char'' type.  So I have a starting offset and index field in the object.
 *  When I create the table, I have SHORT_ARRAY_MAX_ENTRIES + 1 (5 right now)
 *  characters so I choose a starting offset and index to cover that range and
 *  somewhat more.  Then I resize the array if more is needed at the start or
 *  end when new keys are added.  This still won't work well if the characters
 *  that are actually used are very widely separated in the ``char'' type, but
 *  that should be such a rare case that if it uses a lot of memory in that
 *  case I'm OK with it.  It is important to note that this is a rare case of C
 *  implementations, not data values -- on the vast majority of C
 *  implementations, this will always be reasonably efficient.
 *
 *  It is important to note also that large ``char'' types are a function of
 *  the architecture, not any internationalization issues.  C is designed to
 *  use ``wchar_t'' for languages that have more characters than will
 *  conveniently fit in an 8-bit type rather than changing the size of
 *  ``char''.  This string index implementation doesn't have explicit support
 *  for wide characters.  It would be best to map them to strings of 8-bit
 *  characters that don't include 0 and use that as the key for this module.
 *  Just substituting ``wchar_t'' for ``char'' in this code would probably not
 *  lead to the best implementation for Chinese, for example, because it would
 *  produce quite large tables in some cases.  It is better to break up Chinese
 *  characters into 8-bit quantities for the purposes of this kind of tree
 *  structure.  If using Unicode, for example, it would be best to use the
 *  UTF-8 format instead of the UTF-16 or UTF-32 formats with this code.  The
 *  Unicode UTF-8 format will work seamlessly with this module.
 *
 *  There is one final optimization here.  I put a field in each object
 *  containing a count of the number of non-NULL successors.  This is redundant
 *  information, but it make the code more efficient when removing keys because
 *  it saves having to search through a large table to know whether a given
 *  node should be deleted.
 *
 *  One last detail before we get to the actual code is the distinction between
 *  having a value that is NULL and having no value.  The models we've talked
 *  about so far only talked about the value field.  That value field can be
 *  set to NULL when there is no value associated with the key that would lead
 *  to that object, but we need additional information to tell whether that's a
 *  value of NULL associated with the key or no value (i.e. whether the
 *  ``exists'' operation returns true or false).  To take care of that, we add
 *  a boolean field to each object that says whether or not it has a value
 *  associated with it.
 *
 *  So now we get to the actual C code for the data structures.  We've
 *  discussed the SHORT_ARRAY_MAX_ENTRIES macro already.  The next bit of code
 *  after that is a ``#if'' block testing for the DISABLE_CHAR8_OPTIMIZATION
 *  and ENABLE_CHAR8_OPTIMIZATION macros.  This section relates to the
 *  optimization for the ``char'' type being 8 characters.  The user can
 *  explicitly enable this option by using -DENABLE_CHAR8_OPTIMIZATION and can
 *  explicitly disable it by useing -DDISABLE_CHAR8_OPTIMIZATION.  If both are
 *  used, enabling takes priority.  If the user doesn't use one of these
 *  overrides, the code uses the CHAR_BIT macro that ANSI C provides to get the
 *  number of bits in the ``char'' type and sets ENABLE_CHAR8_OPTIMIZATION
 *  based on that.  This block of code implements that autodetection which can
 *  be overridden by the user.  It is the ENABLE_CHAR8_OPTIMIZATION macro that
 *  is used by all subsequent C code to determine which case to implement.
 *
 *  One reason for having a way to force the optimization to be enabled or
 *  disabled is for testing, to be able to test both versions of the code on an
 *  architecture with 8-bit characters.  The 8-bit version can't really be
 *  tested safely on a non-8-bit C implementation, though.  Another reason for
 *  having a way to force enabling or disabling of this optimization is for the
 *  case that ``char'' is actually a bigger type but the person compiling the
 *  code knows it will only be used for ASCII characters or some other set of
 *  characters within the first 256 values of the ``char'' type.
 *
 *  Next, there is one final block of pre-processing directives before the data
 *  structures are introduced.  This section begins with
 *  ``#ifdef ASSERT_MALLOCED_SIZES''.  If the user explicitly defines the
 *  ASSERT_MALLOCED_SIZES macro, it includes "memory_allocation_test.h".
 *
 *  Now we get to the first real code, the definition of the string_index_tag
 *  type.  This type is used to distinguish between the two cases discussed
 *  above for representing successor nodes, the ``short array'' and ``table''
 *  cases.
 *
 *  Next is the definition of the string_index type itself.  This is the type
 *  of the objects in the optimized model discussed above.  It is also the type
 *  of the pointer passed back to the user as a handle for the whole string
 *  index -- the user's handle is the pointer to the root node of the tree.
 *
 *  The ``prefix'' field points to the ASCI string for the prefix optimization
 *  described above.  The space for the prefix is allocated by a separate call
 *  to malloc().  The prefix can be of any size and is zero-terminated.  If the
 *  ``prefix'' field is set to NULL, that represents an empty prefix.  Note,
 *  however, that in some cases an empty prefix can be represented by a pointer
 *  to an empty string (i.e. a pointer to a zero character).  This happens when
 *  a string_index object with a non-zero-length prefix is modified to have a
 *  zero-length prefix by an ``enter'' operation.
 *
 *  The ``has_value'' and ``value'' fields are exactly as discussed in the
 *  optimized model above.  The ``children'' field implements the optimization
 *  discussed above about keeping a count of sucessor nodes.  It's worth
 *  commenting on the use of the ``size_t'' type for the ``children'' field.
 *  There is no type in C which the ANSI standard guarantees will hold a count
 *  of all unique objects or memory locations.  The closest thing is the
 *  ``size_t'' type, which will contain the size of any single object.  But the
 *  ``children'' field counts successor nodes which are all stored in an array
 *  of one sort or another (either of the ``short array'' or ``table''
 *  variety), so the count can never exceed the size of the array, so using
 *  size_t as its type means it can never overflow.
 *
 *  The ``tag'' field specifies which of the two methods of representing
 *  successor nodes is being used for this node, the ``short array'' or the
 *  ``table'' method.  The ``tag'' field specifies how the rest of the
 *  string_index structure is to be interpreted.  The rest of the string_index
 *  is the ``sub'' field which is a union with two members, the ``short_array''
 *  and ``table'' fields.
 *
 *  In the ``short array'' case, the sub.short_array field of the union is
 *  used.  This field is a structure with two arrays as components, the
 *  ``indexes'' and ``keys'' arrays.  Each of these arrays has
 *  SHORT_ARRAY_MAX_ENTRIES elements.  They contain the pointers to successors
 *  and the corresponding characters respectively.  If there are four
 *  successors, all four elements of the ``keys'' array will be non-zero.
 *  Otherwise, if there are n successors where n < 4, then the first n elements
 *  of ``keys'' will be non-zero and the next element will be zero.  After the
 *  first zero element of ``keys'', it's undefined what's in the ``keys''
 *  array, and it's undefined what's in the ``indexes'' array for the element
 *  corresponding to the zero element of ``keys'' and for subsequent elements,
 *  if any.  All the ``indexes'' elements that correspond to elements of
 *  ``keys'' before the first element that is zero must contain valid non-NULL
 *  pointers to successor nodes.
 *
 *  In the ``table'' case, the sub.table field of the union is used.  This code
 *  can be compiled two different ways depending on whether or not the
 *  ENABLE_CHAR8_OPTIMIZATION macro is defined.  If this macro is defined, then
 *  the sub.table field contains only a single element, a field named
 *  ``indexes'' containing a pointer to an array of successor pointers.  In
 *  this case, the array being pointed to will always contain exactly 256
 *  entries.  For elements indexed by characters corresponding to successors,
 *  the array element will contain a pointer to the successor node; and for
 *  elements indexed by characters not corresponding to any successor, the
 *  array will contain NULL pointers.  Note that the array is indexed with
 *  CHAR_MIN as the first element, CHAR_MIN+1 as the next, etc.
 *
 *  If the ENABLE_CHAR8_OPTIMIZATION macro is not defined, two additional
 *  fields are added and the ``indexes'' field is used slightly differently.
 *  The ``indexes'' field still points to an array of pointers to successors,
 *  but in this case it doesn't necessarily have 256 elements -- instead the
 *  number of elements is determined by the value of the ``size'' field.  And
 *  rather than starting with CHAR_MIN as the character corresponding to the
 *  first array index, the ``low_index'' field's value is the character
 *  corresponding to the first array index.  For example, if ``low_index'' is
 *  the character '2' and ``size'' is 5 then the ``indexes'' field must point
 *  to an array of 5 elements and those elements will correspond to the
 *  characters '2', '3', '4', '5', and '6' respectively.  That example can't
 *  actually occur in this code, though, because the code that sets up the
 *  ``indexes'' array always chooses a size of at least 256.
 *
 *  And that completes the detailed explanation of the data structures used
 *  here.
 *
 *
 *      Functions
 *
 *  If I've been successful, I've marked every basic block with one use of the
 *  code_point() macro and each one uses a different integer value as the
 *  argument.  In addition to one per basic block, I've added ``else'' blocks
 *  to ``if'' statements where they would not otherwise be necessary just for
 *  the code_point() macros.  Also, where different code is compiled depending
 *  on a ``#if*'', I've put a code_point() in each version.  This is to make it
 *  easier to check that the tests cover all the code in this file.  (Actually,
 *  the tests fall a little short of this goal since I can't test the table
 *  resizing code on an 8-bit ``char'' machine, but other than that I have full
 *  coverage by the code_point() macro.)
 *
 *      create_string_index()
 *
 *  The first function is create_string_index().  Nothing very complicated is
 *  done here.  A single object of type string_index is created, its fields are
 *  set up, and a pointer to it is returned to the user as the handle for this
 *  string index.  This object will continue to be the root of the string index
 *  tree from then on.  If the attempt to allocate this object fails, the
 *  routine simply returns NULL.  The ``prefix'' field is set to NULL to
 *  indicate no prefix -- it would also be legal, but pointless, to allocate an
 *  array and fill it in with any prefix string.  The ``has_value'' field is
 *  set to FALSE -- if it weren't, that would be saying that the empty string
 *  as a key has a value associated with it, which it should not unless and
 *  until the user adds such a key/value pair.  The ``value'' field then has to
 *  be set to NULL -- whenever ``has_value'' is FALSE, we require that
 *  ``value'' be NULL.  The ``children'' field is set to zero as this node will
 *  have no successors initially.  Note that only the root node is allowed to
 *  have zero successors and no value -- other nodes will only be created if
 *  they have a value associated with them or if they have at least one
 *  successor, and if an existing node ever has its value removed with no
 *  successor or its last successor removed while it has no value, the node
 *  itself will be removed.  But the root node itself is never removed.
 *
 *  The ``tag'' field is set to indicate the ``short array'' method of
 *  specifying successors.  Finally, the sub.short_array field is set up to
 *  indicate no successors.  All that's necessary for that is to set keys[0] to
 *  zero -- when keys[0] is zero, all the other elements of ``keys'' are
 *  ignored and all the elements from zero on -- i.e. all elements -- of
 *  ``indexes'' are ignored.
 *
 *      destroy_string_index()
 *
 *  The next function is destroy_string_index().  This function is somewhat
 *  more complicated.  It calls itself recursively to walk through a string
 *  index tree and deallocate everything associated with it.
 *
 *  After a sanity-check assertion that a non-NULL index pointer has been
 *  provided, the first thing this code does is checks to see if the prefix
 *  string is non-NULL, and if so deallocates it.  Then, at code point 8, this
 *  code does one of two things depending on the tag.
 *
 *  If the tag indicates this node uses the short arrray method (code point 9),
 *  it loops through the elements in the short array and calls itself
 *  recursively for each successor (code point 13).  The code between code
 *  points 10 and 13 terminates the loop when it finds the first zero value in
 *  the keys array.
 *
 *  If the tag indicates this node uses the table method (code point 15), it
 *  goes through every element of the table and for each non-NULL successor it
 *  finds, it calls itself recursively.  Note that the way it finds the number
 *  of elements in the the table depends on the ENABLE_CHAR8_OPTIMIZATION
 *  macro, but it doesn't matter which characters correspond to the successors
 *  at this point, so the low_index field is never used.  After the code has
 *  found and deleted all the successor nodes, it frees the memory for the
 *  table itself (this isn't necessary in the short array case since in that
 *  case the two arrays are part of the string_index structure itself, not
 *  separately allocated).
 *
 *  Note that every switch on the ``tag'' field in this module has a default
 *  case with an assert() of FALSE.  This default case can only happen if there
 *  is data corruption of some sort.
 *
 *  Finally (code point 23), whether the tag indicated the short array or table
 *  method, the memory for the string_index itself is deallocated.
 *
 *      enter_into_string_index()
 *
 *  The enter_into_string_index() function is probably the most complex in this
 *  module.  There are a number of different ways that it may have to change
 *  the data structures under different circumstances.
 *
 *  The code starts with the usual sanity-checking assertion that a non-NULL
 *  string_index has been provided.  Then, it checks to see if the string index
 *  is completely empty -- i.e. if it has no successors (the ``children'' field
 *  is zero) and has_value is FALSE.  In this case (code point 25),  The
 *  ``prefix'' field is set to match the key and the ``value'' field is set to
 *  the new value.  The first ``if'' statement here (right after code point 25)
 *  checks to see if the key is the empty string.  If so (code point 26), the
 *  prefix needs to be set to the empty string.  If the prefix is already NULL,
 *  we just leave it like that.  Otherwise, we set the first character of the
 *  prefix string to zero, to make it an empty string (changing the ``prefix''
 *  field to NULL and deallocing the string would also have been fine in this
 *  case).  Otherwise, if the key isn't empty (code point 30), there is a check
 *  to see if we can copy into the memory already allocated for the prefix.  If
 *  the prefix is non-NULL and the length of the old prefix is at least that of
 *  the new prefix, that will work.  Otherwise, we'll allocate a new chunk of
 *  memory for the prefix (code point 32) (we don't know for sure that it won't
 *  fit in the existing prefix because a large block of memory might have been
 *  allocated for an earlier prefix that was later shortened, but since we
 *  don't separately keep track of the size of the prefix block, we have to be
 *  conservative and assume we need a new block).  If the allocation fails
 *  (code point 33), we just immediately return MISSION_FAILED (the data
 *  structures haven't been changed yet, so everything is still in a consistent
 *  state).  Then (code point 35) we copy from the key into the new prefix
 *  block, free the old prefix block if the pointer was non-NULL, and set the
 *  ``prefix'' field to point to the new block.  At this point (code point 40),
 *  we've completed setting up the prefix field, so all that's left is to set
 *  the ``value'' and ``has_value'' fields and return.
 *
 *  If the string index wasn't completely empty before, we reach code point 42.
 *  This is where we start dealing with the prefix, if any.  The ``follow''
 *  local is used to walk through the characters in the key argument to this
 *  function.  Note the ``follow'' local is set before checking to see if there
 *  is a non-NULL prefix.  That's because the code after this block (code point
 *  85), if it is reached, will be dealing with the key minus the prefix, if
 *  any.  So we'll make sure that ``follow'' points to the key after the
 *  prefix, which is just the key in the case of an empty prefix.
 *
 *  If the ``prefix'' pointer is non-NULL (code point 43), we follow through
 *  both the prefix and the key, looking for mismatches.  The follow_prefix
 *  local is used to follow the prefix while the ``follow'' local follows the
 *  key.  The ``while'' loop two lines after code point 43 is what follows
 *  through the prefix and key, one character per iteration, looking for
 *  mismatches.  There are two ways that this loop can terminate: when the
 *  entire prefix has been handled and found to match (in which case the
 *  condition of the ``while'' statement itself will test FALSE); or when a
 *  mismatch is found (by the ``if'' statement following code point 44).  The
 *  only code in the loop other than the handling of the mismatch case is at
 *  code point 82, where ``follow'' and follow_prefix are both incremented.
 *
 *  If a mismatch is found, then control will pass to code point 45.  Once
 *  control reaches here, it won't leave this ``if'' statement until the
 *  function returns.  Note that when we've reached code point 45 we've found a
 *  mismatch before we've reached the end of the prefix.  That means we'll have
 *  to break the prefix.  We're going to have to insert at least one new node
 *  in the tree.  Since other things (including outside client code) might
 *  point to the existing string_index node, the existing node will have to
 *  become the one that contains the shortened first part of the prefix (the
 *  first part of the prefix, which matched, might be empty or non-empty).  A
 *  new node will have to be created to represent the second part of the
 *  original prefix.  This second part of the prefix, again, might be empty (if
 *  the mismatch was in the last character of the original prefix) or
 *  non-empty, but either way the new node created for this second part of the
 *  prefix will have to be given all the successors and the ``value'' and
 *  has_value fields of the original string_index.  If the mismatch is because
 *  the key ends, then the ``value'' of the original string_index will be set
 *  to the new value, but otherwise a second new string_index will have to be
 *  created that will have the value and whatever prefix is necessary to take
 *  into account the rest of the key.  The code from code point 45 to code
 *  point 80 does exactly these things, as described in more detail below.
 *
 *  The ``index2'' local is used to point to the first of the new string_index
 *  objects, the one that will take the successor nodes and the value of the
 *  original string_index.  The ``index3'' local is used to point to the second
 *  new string_index object, if it is needed, the one that will contain the new
 *  value.  The code following code point 45 allocates the first new
 *  string_index object.  If the allocation fails, the function simply returns
 *  MISSION_FAILED -- no changes have been made and nothing has been allocated,
 *  so returning here leaves everything in a consistent state.
 *
 *  Next, at code point 48, we set up the ``prefix'' field of ``index2''.  The
 *  prefix of index2 will be the rest of the original prefix starting with
 *  follow_prefix[1] -- the ``1'' is because the character pointed to be
 *  follow_prefix will be encoded in the transition from the old string_index
 *  to index2, as the character corresponding to that successor.  So we test to
 *  see if follow_prefix[1] is non-zero -- if it is, index2 will need a
 *  non-empty prefix, otherwise index2 will have an empty prefix.  Note that
 *  it's safe to test follow_prefix[1] without checking to see if
 *  follow_prefix[0] is zero because if follow_prefix[0] were zero, the outer
 *  loop (starting two lines after code point 43) would have terminated before
 *  we reached this point.  Note that if follow_prefix[1] is zero, there's no
 *  work to do because index2->prefix was already set to NULL by
 *  create_string_index().
 *
 *  If follow_prefix[1] is non-zero, we reach code point 49.  In this case,
 *  we'll have to allocate a new block of memory for the remainder of the
 *  prefix (well, strictly speaking we might be able to re-use the memory from
 *  the prefix of the original string_index in the case that the mismatch was
 *  in the first character of the original prefix, but that's an optimization
 *  we don't bother to do here just because it further complicates already
 *  complex code -- we'll always leave the original prefix memory block with
 *  the original string_index).  The ``prefix2'' local is where we put the
 *  pointer to the newly allocated block for index2's new prefix.  If the
 *  allocation fails (code point 50), we de-allocate index2 and return
 *  MISSION_FAILED -- we haven't changed anything in the original string index
 *  or allocated anything else yet, so everything is left in a consistent
 *  state.  Then (code point 52), we copy the data into the new prefix memory
 *  block and set the ``prefix'' field of index2 to point to it.  Note that at
 *  this point index2 is still a completely self-consistent string_index, so we
 *  can call destroy_string_index() on it later if something goes wrong and it
 *  will de-allocate both index2 itself and the prefix memory.
 *
 *  So we've reached code point 54.  Now it's time to wee if we need to create
 *  a second new string_index to contain the new value.  As mentioned above, we
 *  have to create the second new string index (index3) if and only if the
 *  mismatch isn't because the key ended.  That's what the ``if'' statement
 *  immediately following code point 54 tests for.  If we do have to create
 *  index3, we reach code point 55 and allocate the second new string_index.
 *  If the allocation fails, we destroy index2 and return MISSION_FAILED, which
 *  leaves everything consistent because destroying index2 de-allocates
 *  everything we've allocated so far and we haven't changed the original
 *  string_index at all yet.  If the creation of index3 succeeds, we reach code
 *  point 58.  At this point, we have to set up the prefix of index3.  The
 *  prefix of index3 takes into account whatever remains of the original key.
 *  But follow[0] is encoded as the character corresponding to index3 as a
 *  successor to the original string_index (this hasn't been set yet, but we'll
 *  get there), so it is only the string from follow[1] on that needs to be
 *  used as the prefix for index3.  The ``if'' line immediately after code
 *  point 58 tests to see if index3's prefix needs to be non-NULL -- if not,
 *  there's no work to be done because create_string_index() already set up
 *  index3 with an empty prefix.  If a non-empty prefix is needed, though, we
 *  reach code point 59 and allocate memory for the new prefix, putting the
 *  pointer to it in the local ``prefix3''.  Then we check to see if the
 *  allocation failed, and if so delete both index2 and index3 (both are
 *  well-formed string_index objects, so we can call destroy_string_index() on
 *  each) and return MISSION_FAILED.  We still haven't touched the original
 *  string index, so everything is left in a consistent state.  If the
 *  allocation succeeds, though, we reach code point 62 where we copy the data
 *  into the new prefix memory block and set index3's ``prefix'' field to point
 *  to the new block.
 *
 *  Now we reach code point 66.  At this point we've successfully done all
 *  allocation of memory necessary, so we can safely start modifying the
 *  original string_index without having to worry about handling allocation
 *  errors.  First, we transfer the value and children from the original
 *  string_index to index2.  The has_value, ``value'', ``children'', and
 *  ``tag'' fields can just be copied over.  Copying over the successor
 *  information, though, depends on the tag, so we have to switch on it.  In
 *  the short array case, we have to loop through the array elements, copying
 *  the ``keys'' and ``indexes'' elements until we either find a ``key''
 *  element that is zero or run out of elements in the array.  Note that when
 *  we find a zero key element, we have to set the corresponding element in
 *  index2's ``key'' array to zero, but we don't have to worry about the
 *  corresponding ``indexes'' element -- that's why the check for a zero key
 *  element is after the copying of the ``key'' element but before the copying
 *  of the ``indexes'' element.  In the table case (code point 73), it's even
 *  simpler because instead of looping through the array and copying elements,
 *  we can simply switch the ownership of the existing table array to index2.
 *  We also have to copy the low_index and ``size'' fields, if the
 *  ENABLE_CHAR8_OPTIMIZATION macro isn't set.
 *
 *  At code point 77, we change the original string_index to use the short
 *  array method with index2 as its first successor.  The character
 *  corresponding to this successor is *follow_prefix -- i.e. the character in
 *  the original prefix that mismatched the new key.  Then, if the mismatch was
 *  because the new key was a prefix of the prefix of the original string_index
 *  (i.e. if *follow is zero), the ``value'' field of the original string_index
 *  is set to the new value and keys[1] is set to zero and ``children'' to one
 *  to indicate that the original string_index has only one successor, index2
 *  (code point 78).  Otherwise, (code point 79), the original string_index is
 *  set to have no value (has_value FALSE and ``value'' NULL) and index3 is set
 *  up as the second successor of the original string index, with *follow as
 *  the corresponding character, and index3 is set up with the new value.
 *
 *  Finally, at code point 80, *follow_prefix is set to zero.  This serves to
 *  truncate the prefix of the original string index to just the part that
 *  matched the new key.  Then the function returns, the new key/value pair
 *  having successfully been inserted.
 *
 *  This brings us up to code point 85.  If we get here, we've established that
 *  the prefix matches the first part of the new key exactly, and the
 *  ``follow'' variable points to the first character of the key after the part
 *  that matched the prefix.  Now if the remainder of the key is empty (i.e.
 *  *follow is zero), then the value should be placed directly in the original
 *  string_index and we're done.  If not, we reach code point 88.
 *
 *  At code point 88, we know we're going to have to put the value in a
 *  successor node of the original node, because we know that the new key is
 *  longer than the prefix and the fact that there is already at least one
 *  successor or the original node has a value means we can't extend the prefix
 *  of the original string_index.  So the rest of this function breaks down
 *  into two case, the short array case (code point 89) and the ``table'' case
 *  (code point 137) of the original string_index node.  The character *follow
 *  must correspond to the successor node that has the new value, so if a
 *  successor already exists for that character, it must be added to that node,
 *  and otherwise a new successor node must be created.
 *
 *  At code point 89, we're in the short array case and we have to figure out
 *  whether we need to create a new successor node.  So we loop through the
 *  elements of ``keys'' looking for a match with *follow.  If we find a match
 *  (code point 94), then we recursively call enter_into_string_index() with
 *  the corresponding successor node and the remainder of the key (follow + 1)
 *  and we're done.  Otherwise, we reach code point 97.
 *
 *  At code point 97 we know we have to create a new successor node.  So we
 *  call create_string_index().  If the allocation failed, we simply return
 *  MISSION_FAILED -- it's safe because we haven't changed anything in the
 *  original string_index yet and we haven't allocated anything else.  If the
 *  allocation succeeded, we reach code point 100 and it's time to set the
 *  prefix of the new node to the string starting with follow[1].  If follow[1]
 *  is zero, there's nothing to do because create_string_index() left the new
 *  node with an empty prefix already.  If follow[1] is non-zero, though, we
 *  reach code point 101 and have to allocate space for the new prefix.  If
 *  this allocation fails, we destroy the newly created node and return
 *  MISSION_FAILED -- again, we haven't allocated anything else or touched the
 *  original string_index, so it's safe to do this.  If the allocation succeeds
 *  (code point 104), we copy the data into the new prefix buffer and set the
 *  ``prefix'' field of the new node to the new memory block.  Note that from
 *  here on we can call destroy_string_index() on the newly allocated node, if
 *  neccessary, to clean up both the node and the prefix memory for it, if any.
 *  That brings us to code point 106, where we set the ``value'' and has_value
 *  fields of the newly-allocated node.
 *
 *  Now we just have to add the link from the original string_index to the new
 *  node.  But this is a bit complicated.  If there's still room in the arrays
 *  for the short array method, we can just put it in there, but if not, we
 *  have to switch the original string_index node from the short array to the
 *  table method.  The ``if'' statement three lines below code point 106
 *  figures out which of these it will be.  The ``item_num'' local contains the
 *  necessary information -- it was used in the loop that looked for an
 *  existing successor with a matching character and didn't find one, so it
 *  must give the index that is one greater than the index of the last existing
 *  successor, or zero if there were no existing successors.  So if item_num is
 *  less than SHORT_ARRAY_MAX_ENTRIES, there's still room left in the short
 *  arrays (code point 107) but otherwise we'll have to switch to the ``table''
 *  method (code point 111).
 *
 *  The first case (code point 107) is relatively straight-forward.  The
 *  appropriate entries in the ``keys'' and ``indexes'' fields are set (the
 *  item_num variable already tells us which elements they go into).  Then, the
 *  following element of the ``keys'' array is set to zero to mark the end of
 *  the valid data, unless the array is completely full.
 *
 *  The other case, at code point 111, is more complex.  Here, we have to
 *  convert from the short array to the table method.
 *
 *  The first thing we do is figure out the number of entries in the new table
 *  (which we'll put in the ``size'' local variable) and the character
 *  corresponding to element zero in the table (which we'll put in the min_char
 *  local variable).  If we're using the 8-bit character optimization (code
 *  point 112), this is trivial -- the size is always 256 and the min_char is
 *  always CHAR_MIN (from <limits.h>, which ANSI C guarantees gives us the
 *  minimum character value).  If we're not using this optimization (code point
 *  113), things are more difficult.  First, we find the maximum and minimum
 *  values of characters we know will be associated with successors -- i.e.
 *  those in the old short array ``keys'' array plus *follow, which is the
 *  character for the new successor we are adding.  The code from code point
 *  113 to code point 121 does this.  We use the minimum as min_char and as the
 *  size, we choose the maximum of 256 or the minimum size to cover both the
 *  minimum and maximum characters (i.e. the difference between them plus one).
 *
 *  This brings us to code point 125, where we actually allocate the new table.
 *  We've been very careful up to this point not to modify anything in the
 *  original string_index tree, so if the allocation fails, all we have to do
 *  is use destroy_string_index() on the newly-allocated string_index and
 *  return MISSION_FAILED; everything will be left in a consistent state.  If
 *  the allocation succeeded (code point 128), we just have to fill in the
 *  table.  We start by going through every element and setting it to NULL and
 *  then (code point 130) add our newly-allocated successor and all the old
 *  successors in the short array fields.  Note that we have to finish reading
 *  all the old short array data before setting any of the ``table'' method
 *  fields because they share the same memory; before code point 132, we don't
 *  use the ``table'' fields and after code point 132 we don't again use the
 *  short array fields.  At code point 132, we update the tag of the original
 *  string_index to indicate that it now uses the ``table'' method.  Then, if
 *  the 8-bit character optimization isn't being used (code point 134), we set
 *  the low_index and ``size'' fields (they're not used in the 8-bit character
 *  optimization case).  Finally, at code point 135, we set the ``indexes''
 *  field to point to the newly-allocated table.
 *
 *  This brings us to code point 136, which is reached whether we converted to
 *  the ``table'' method or not, as long as we started with the short array
 *  method and had to add a new node.  At this point, we increment the
 *  ``children'' field and continue.
 *
 *  Which brings us to the remaining case, at code point 137: the case that the
 *  original string_index used the table method.  The biggest complexity here
 *  is that we might need to re-size the existing table (which never happens in
 *  the 8-bit character optimization case).  It's harder to read this code
 *  because of the #ifdef lines, which interact with the ``if'' and ``else''
 *  blocks in a non-standard way.
 *
 *  The easiest way to understand this code is to first consider the
 *  non-optimized code by itself, ignoring the ENABLE_CHAR8_OPTIMIZATION case,
 *  and then after that is fully understood, look at how the
 *  ENABLE_CHAR8_OPTIMIZATION case is a simplification of that.
 *
 *  The non-optimized case starts with code point 139.  Here, the low_index
 *  local variable is set from the low_index field.  Then tests are done to see
 *  if re-sizing is needed.  If the new character is below the range of the
 *  existing table, code point 140 is reached.  If it's above the range, code
 *  point 148 is reached.  Otherwise, code point 156 is reached.
 *
 *  The first case is code point 140, where the new character is below the
 *  existing range.  In this case, size_diff is set to the number of new
 *  elements that must be added to the start of the table and the sizes of the
 *  old and new tables are calculated.  Then the new table is allocated.  If
 *  the allocation fails (code point 141) the function simply returns
 *  MISSION_FAILED.  Since nothing in the original tree was modified and
 *  nothing else was allocated, this is safe and leaves everything in a
 *  consistent state.  If the allocation succeeds (code point 143), we fill the
 *  first size_diff elements of the new table with NULL pointers and the rest
 *  with the elements from the original table.  Then (code point 147), we
 *  de-allocate the old table, and set the fields of the original string_index
 *  to use the new table and its new size and first elements.  We also update
 *  the ``low_index'' variable to the new first element character because we'll
 *  be using it later on.
 *
 *  The second case is code point 148, where the new character is above the
 *  existing range.  This is handled similarly to the case that the new
 *  character is below the existing range, but with the new NULL elements being
 *  added at the end of the new table instead of at its start.  Also, in this
 *  case, the low_index variable doesn't have to be updated.
 *
 *  The final case is code point 156.  In this case, the new character lies
 *  within the range of the existing table.  So we check to see if there is
 *  already an entry in the table for that character.  If so (code point 158),
 *  we do a recursive call to enter_into_string_index() using that successor
 *  node and the remainder of the key (follow + 1).  And that's it for this
 *  case; we've now discussed everything before code point 164.
 *
 *  Now it's time to revisit everything from code point 137 through 164 and
 *  consider the 8-bit character optimization.  We'll start with code point
 *  139.  The low_index is always CHAR_MIN in the 8-bit optimization case, so
 *  that's what the line at code point 138 does.  Now consider the test two
 *  lines below code point 139, to see if the character is below the range of
 *  the table.  In the 8-bit optimization case, this can never happen, so the
 *  code in the ``if'' block (starting with code point 140) will never be
 *  reached.  Similarly, the test for the character being past the end of the
 *  range can't evaluate to TRUE either, so the ``else if'' block (starting
 *  with code point 148) is also unreachable and the ``else'' part leading to
 *  code point 157 has to be reached.  That's exactly what the code at code
 *  point 138 does -- it skips the tests and the ``if'' and ``else if'' cases
 *  to go right to the ``else'' case of code point 157 -- in fact, it doesn't
 *  even include the new block (the curly brace) before code point 156 because
 *  this code isn't conditional in this case, it's unconditionally executed.
 *  And that's the reason for the #ifdef lines between code points 160 and 164
 *  -- in the 8-bit optimization case there is no right curly brace to close
 *  the block but in the non-optimized case there is; that's the only
 *  difference there.
 *
 *  That brings us to code point 164 where we're beyond the differences between
 *  the 8-bit character optimized and un-optimized versions.  At this point the
 *  table has been expanded if necessary to include the new character, and
 *  there isn't already a successor for that character or we wouldn't have
 *  reached here.  So we next create a new string_index.  If the allocation
 *  failed (code point 165), we return MISSION_FAILED.  Note that even though
 *  we might have modified the original string_index by this point, the only
 *  modification might have been to expand the table.  So it's still in a
 *  consistent state and there's nothing we've allocated that isn't part of
 *  that tree now, so we can safely just exit if the allocation failed.
 *
 *  Now we're at code point 167 and we need to set up the prefix for the newly
 *  created node.  This is the same thing we had to do for the short array
 *  method.  In fact, the code from code point 167 through code point 173 is
 *  identical to that from code point 100 through code point 106, and very
 *  similar to that from code point 58 through code point 64.  Why, you might
 *  wonder, isn't this code broken out as a separate function?  Maybe that
 *  would be better, but it's a bit inconvenient in C because you'd have to
 *  pass several things as parameters and have some way to clean up and return
 *  from the outer function if the allocation failed.  And then when reading
 *  the code you'd have to detour to a different place to see what exactly the
 *  function does.  Most people would probably still factor it out as a
 *  separate function (as they would with several other bits of similar code in
 *  here) and perhaps I would too if I were to do it again, but since this code
 *  is done and tested and debugged and there are reasons for and against doing
 *  it, I'm just leaving it like this -- there isn't any horrible problem with
 *  doing it this way, either.
 *
 *  That brings us to code point 173, and nearly to the end of the function.
 *  All that is left is to set the ``value'' and has_value fields of the new
 *  string_index node, add the new node to the table, and increment the
 *  ``children'' count.
 *
 *      remove_from_string_index()
 *
 *  The most convenient way to implement the functionality of the
 *  remove_from_string_index() function was recursively using a slightly
 *  generalized form of this function.  The slightly generalized form is the
 *  remove_and_return_true_if_object_gone() function, which is the one private
 *  function used by this module.  The remove_and_return_true_if_object_gone()
 *  function differs from this one in that it takes an extra argument, a
 *  boolean, and if that argument has the value TRUE, it deletes the
 *  string_index if it is empty after the remove operation finishes.  The
 *  top-level string_index should never be deleted by a remove operation, but
 *  other string_index objects in the tree should be, so this extra argument is
 *  TRUE only for the top-level object.  Also, the function returns a value
 *  indicating whether or not the string_index was deleted instead of having a
 *  void return value.  Since the top-level string_index is never deleted by a
 *  remove operation, the return value of the top-level call isn't necessary,
 *  but it is necessary for recursive calls because if an object elsewhere in
 *  the tree is removed, other changes will have to be made in the former
 *  parent of that node.
 *
 *      exists_in_string_index()
 *
 *  This function uses the find_existing_node_for_key() private function to
 *  find the node corresponding exactly to the key, if any.  If such a
 *  corresponding node is found (code point 178), the has_value field of that
 *  node is returned.  Otherwise (code point 177), FALSE is returned.
 *
 *      lookup_in_string_index()
 *
 *  The lookup_in_string_index() function is nearly identical to the
 *  exists_in_string_index() function except that if no matching node is found,
 *  NULL is returned instead of FALSE and if a matching node is found, the
 *  ``value'' field is returned instead of the has_value field.
 *
 *      check_string_index_integrity()
 *
 *  The final public function in this module is the
 *  check_string_index_integrity() function.  This function walks the entire
 *  tree looking for any kind of inconsistency it can find.  Note, however,
 *  that one kind of problem that it does not explicitly check for is cycles in
 *  the graph.  But there really isn't a need for it to do so.  If there is a
 *  cycle, this function will just recurse endlessly, which should either crash
 *  or hang the program.  Since this function is intended for testing and
 *  debugging situations only, not for production code, that's OK becuase
 *  either crashing or hanging will show that there is a problem.  It's not as
 *  convenient to debug such a problem as it would be if this code explicitly
 *  checked for cycles and printed a nice message about them if found, but it's
 *  not terribly hard to debug it either, using either a debugger or printf()
 *  calls inserted into the code.  Such debugging methods are likely to be
 *  needed to figure out how the cycle got there in the first place anyway, and
 *  that's likely to be a lot harder that figuring out that it was stuck here
 *  because of a cycle.  If this check was being done in a production context
 *  (such as a check on data read off a disk, where corruption is an error mode
 *  that we might reasonably expect this code to handle gracefully), we would
 *  want to explicitly detect cycles here, and in such contexts I do generally
 *  put in explicit tests for cycles along with tests for other corruption, but
 *  such is not the case here.
 *
 *  This function starts, at code point 182, by asserting that the index
 *  pointer is non-NULL.  This is important not only to make sure the user
 *  didn't pass a NULL pointer in but also because this function is called
 *  recursively on sub-nodes of the tree and an important failure mode is a
 *  NULL pointer where it shouldn't be.  Next, it uses the
 *  assert_is_malloced_block_with_exact_size() function to claim that the index
 *  pointer points to a malloced block of the right size.  In production
 *  builds, that function call reduces to nothing, but if this code is built in
 *  debug form and linked with the appropriate memory allocation checking
 *  module, this can actually do a check at runtime to make sure the pointer is
 *  really pointing to a block of the specified size.
 *
 *  Next, this code checks the prefix.  The prefix pointer is allowed to be
 *  NULL, but if it isn't (code point 183) it must point to an allocated
 *  zero-terminated ASCII string.  The assertion call uses strlen() to figure
 *  out where the terminating zero is and check that ``prefix'' points to an
 *  allocated block at least big enough to hold the string and terminating
 *  zero.  Note that it is allowed to be larger, not required to be exactly the
 *  size needed to hold the string, because in some cases a string will be
 *  allocated and later shorted.  The shortening is done by putting the zero
 *  sooner in the string, avoiding having to reallocate to get a smaller memory
 *  block.
 *
 *  Next, at code point 185, this code checks that the ``value'' and has_value
 *  fields are mutually consistent.  That is, if has_value is FALSE, ``value''
 *  must be NULL.  Note that the converse is not required -- if has_value is
 *  TRUE ``value'' can be NULL or non-NULL.
 *
 *  After that, starting at code point 188, the rest of this function checks
 *  the integrity of the data structures encoding the successor node
 *  information and calls this function recursively on each successor node.
 *  This code is broken by the switch statement into two cases, the short array
 *  case and the table case of methods for representing the successor nodes.
 *  In either case, though, the ``children'' local will be set to the number of
 *  successors, so that at the end of the function the ``children'' field of
 *  the node can be checked.  It does this by first unconditionally setting the
 *  ``children'' variable to zero and then incrementing it each time a
 *  successor is found.
 *
 *  In the short array case (code point 189), there is a loop through the
 *  elements of the ``keys'' and ``indexes'' arrays until either a zero in the
 *  ``keys'' field is found or there are no more elements in the arrays.  For
 *  each element until then (code point 193), the ``children'' local is
 *  incremented and the function calls itself recursively on the successor node
 *  specified by the element in the ``indexes'' array.  Note that when a zero
 *  is found in the ``keys'' element, the corresponding element of the
 *  ``indexes'' array does not contain a valid successor pointer, so the loop
 *  is terminated before trying to handle it as a successor.
 *
 *  In the table case (code point 195), the code loops through all the elements
 *  of the table.  Whether this number of elements is fixed at 256 (code point
 *  196) or read from the ``size'' field (code point 197) is determined by
 *  whether or not the 8-bit character optimization is enabled.  For each
 *  element of the table that is non-NULL (code point 199), the function calls
 *  itself recursively for that successor pointer and increments the local
 *  ``children'' count.  If the element is NULL, that simply indicates there is
 *  no successor for the corresponding character, and this function does
 *  nothing more for that element.  After all the elements in the table have
 *  been dealt with (code point 202), this code checks that the size of the
 *  table itself matches exactly the size it should be for that number of
 *  elements (the item_num local contains the number of elements after the loop
 *  is finished).
 *
 *  Finally, as promised, the ``children'' local is checked against the
 *  ``children'' field of the node at code point 203.
 *
 *      remove_and_return_true_if_object_gone()
 *
 *  The remove_and_return_true_if_object_gone() function takes a pointer to a
 *  node and a key and removes the key from that node, if it exists in the tree
 *  of that node.  In addition, if the ok_to_delete flag is TRUE, it deletes
 *  the node it was passed if after removing the key the node has no value and
 *  no successors.  If the flag is not set it will never delete the node it was
 *  passed.  However, whether or not the ok_to_delete flag is set, this
 *  function will delete sub-nodes if they become empty because of this removal
 *  operation.  The idea is that the ok_to_delete flag should be set to FALSE
 *  for the root node of the tree because a pointer to this node is what the
 *  user uses as a handle for the whole string index, but it should be set to
 *  TRUE for all sub-nodes.  As the name implies, this function returns TRUE if
 *  the node it was passed is deleted and FALSE if that node was not deleted.
 *  Note that if the node it was passed was not deleted but a sub-node was
 *  deleted, it still returns FALSE.  That's so when the function is called
 *  recursively if the node is deleted, the pointer from its predecessor can be
 *  removed.  If the key is discovered not to have a value in the given
 *  string_index, this function return FALSE with no effect.
 *
 *  The code starts, at code point 204, with a sanity-check assertion that the
 *  node pointer is non-NULL.  The recursive calls must be made only when
 *  non-NULL successors are found.  Then the ``follow'' local is set to point
 *  to the start of the key; as with the enter_into_string_index() function,
 *  the ``follow'' local will be used to match against the node's prefix and
 *  left pointing to the next character in the key after the matched prefix, if
 *  any.  If a mis-match is found with the prefix, this function immediately
 *  returns because that means that the key is not in this string_index.
 *
 *  If the prefix is NULL, ``follow'' is just left pointing to the start of the
 *  key and the matching is done, but if it's non-NULL (code point 205), the
 *  follow_prefix local is used to follow the prefix string while the
 *  ``follow'' local follows the key.  The loop that starts two lines below
 *  code point 205 does the matching, moving each of ``follow'' and
 *  follow_prefix forward by one on each iteration.  If a zero is found in the
 *  prefix string, the loop exits and the match is done.  If mis-matching
 *  characters are found, though (code point 207), this code immediately
 *  returns FALSE.
 *
 *  At this point we reach code point 212 and it's time to check to see if this
 *  node itself is the one that corresponds to the key.  This is true if and
 *  only if the part of the key following the prefix match (i.e. that pointed
 *  to by ``follow'') is empty.  In that case (code point 213), we know it's
 *  time to clear th value for this node, if any.  And if that will leave the
 *  node with no successors (i.e. the ``children'' field is zero), and the
 *  ok_to_delete flag is set, it's time to delete this node and return TRUE to
 *  indicate that this has been done.  Otherwise, we simply set the
 *  ``has_value'' field to FALSE and the ``value'' field to NULL and we're
 *  done.
 *
 *  That brings us to code point 218 where it's time to move on to a successor
 *  node.  At this point we know that there's at least one more character in
 *  the key after that which matched the prefix and the first of these extra
 *  characters is pointed to by ``follow''.  So we have to find the successor
 *  that corresponds to the character pointed to by ``follow''.  There are two
 *  cases: the short array case and the table case.
 *
 *  In the short array case (code point 219), we have to iterate through the
 *  array of keys looking for a match with *follow.  If we run out of array
 *  elements (code point 247) or we find a zero in the ``keys'' array (code
 *  point 221), we can immediately return FALSE knowing that this key doesn't
 *  exist in this string_index.  If we find a match (code point 224), we've
 *  found the right successor node to consider.  So at this point we
 *  recursively call this function on the corresponding successor node with the
 *  rest of the key (and the ok_to_delete flag set to TRUE since it's always OK
 *  to delete sub-nodes).
 *
 *  After the recursive call, if the function returned FALSE (code point 225),
 *  there's nothing more to do -- the key has been removed and the successor
 *  node is still there.  But if the successor node has been deleted (code
 *  point 227), we're left with a potential hole in the ``keys'' and
 *  ``indexes'' arrays.  The following loop moves the rest of both arrays up to
 *  fill in the hole.  The loop runs forward through the arrays from the hole
 *  to the end of the array or the first zero in the ``keys'' array.  If no
 *  zero is found but instead the end of the arrays is reached (code point
 *  229), a zero must be inserted as the final element in the ``keys'' array
 *  just before exiting the loop.  Otherwise, a zero in the ``keys'' array will
 *  eventually be found (code point 232) and the loop will exit at that point.
 *  By making sure that we copy the element from the ``keys'' array back one
 *  before checking to see if it is zero, there is no reason to explicitly put
 *  in a zero after one is found -- by that time, the zero has already been
 *  copied to the correct place.  The copying of the ``indexes'' element
 *  happens after checking to see if the ``keys'' element is zero because if it
 *  was, there's no point in copying the ``indexes'' element, since it's
 *  garbage.
 *
 *  Now that the ``keys'' and ``indexes'' arrays have been fixed up (code point
 *  235), we fix the ``children'' field.  Note that we already know it has to
 *  be greater that zero because we just removed a successor, so it had to be
 *  at least 1 (and this was asserted at code point 224, which has to be
 *  reached before code point 235 can be).  If the updated value of the
 *  ``children'' field is zero, we have to consider deleting this node, but if
 *  it's non-zero there's nothing more to do (code point 236).  So we check to
 *  see if this node has a value.  If so (code point 239), the node can't be
 *  deleted so there's nothing else to do.  Also, if the ok_to_delete flag is
 *  FALSE, this node can't be deleted so again there's nothing to do (code
 *  point 242).  Otherwise (code point 244), we de-allocate this node and
 *  return TRUE to indicate we have done so.
 *
 *  The other case (code point 248) is that the table method is being used to
 *  represent successor nodes of this node.  In that case, we need to index
 *  into the table and for that we need to know the index of the first element
 *  in the table.  In the 8-bit character optimization case (code point 249),
 *  this is simply CHAR_MIN and we put that value in the low_index local
 *  variable.  In the non-optimized case (code point 250), the low_index is set
 *  to the value of the low_index  field.  In the non-optimization case, we
 *  also check to see if the character in question (*follow) is below the index
 *  of the first element of the array and return FALSE immediately if so (code
 *  point 251).  In this case we also (code point 253) check to see if our
 *  character is beyond the end of the array (code point 254), which again
 *  means there's nothing to do.  Neither of these tests is necessary in the
 *  optimized case because the range of the ``char'' type is exactly equal to
 *  the range of elements in the table.
 *
 *  So we reach code point 257 where we examine the pointer in the table.  If
 *  it's NULL (code point 258), the key is not in this index and there's
 *  nothing more to do.  Otherwise (code point 260), we've found the successor
 *  so we assert that the ``children'' count is at least 1 (it's good to assert
 *  it's greater than zero since we may be decrementing it soon) and
 *  recursively call this function on the successor node with the remainder of
 *  the key and TRUE as the ok_to_delete flag.  If the recursive call returned
 *  FALSE (code point 261), the successor is still there and there's nothing
 *  more to do.  Otherwise (code point 263), the table element is set to NULL
 *  and the ``children'' field is decremented.  All that's left is to delete
 *  this node if necessary.  It's not necessary (or legal) to delete this node
 *  if there are other successors to this node (code point 264), this node has
 *  a value (code point 267), or the ok_to_delete flag is not set (code point
 *  270).  Otherwise (code point 272), we de-allocate this node and return TRUE
 *  to indicate we have done so.
 *
 *      find_existing_node_for_key()
 *
 *  This function recursively walks the tree to find the node corresponding to
 *  the key.  If there is no such node, it returns NULL, and if there is such a
 *  node, it returns a pointer to that node.
 *
 *  The code starts (at code point 273) by asserting that the ``index''
 *  parameter is a non-NULL pointer.  Then, it begins matching the key against
 *  the prefix field of the node, if necessary.  As with the code following
 *  code point 42 in the enter_into_string_index() function, the ``follow''
 *  local is used to walk through the key while matching with the prefix.  In
 *  fact, most of the code in this function is a simplified version of the code
 *  in the enter_into_string_index() function.  Both functions walk through the
 *  tree matching the key; the difference is that the present function simply
 *  returns NULL as soon as it finds a mismatch while the other function had to
 *  make various complex changes in these cases.
 *
 *  As at code point 42, at code point 273 ``follow'' is set to the start of
 *  the key before checking to see if there is a prefix because after the block
 *  of code handling the prefix, when we reach code point 281, the ``follow''
 *  variable will be assumed to point to whatever part of the key wasn't
 *  matched against the prefix (if there was a mismatch, we wouldn't have
 *  reached code point 281).  Then we check to see if the prefix is non-NULL
 *  and if so we reach code point 274 and start matching against the prefix.
 *
 *  The loop starting two lines below code point 274 walks through the prefix
 *  and the key, one character of each per iteration, until either there is a
 *  mismatch or the end of the prefix is reached.  If there is a mismatch (code
 *  point 276) the code returns NULL immediately.  Otherwise, the ``while''
 *  condition triggers to end the loop after having matched the whole prefix
 *  and we reach code point 279 with ``follow'' pointing to the next character
 *  in the key.
 *
 *  So when we reach code point 281, we've matched the prefix, if any, and
 *  follow points to the rest of the key.  If (and only if) there is no more to
 *  the key, then this node corresponds to the entire key (code point 282) and
 *  we return a pointer to the node.
 *
 *  If we reach code point 284, we've matched the prefix, if any, and there is
 *  still at least one more character in the key.  So we need to find the
 *  successor node corresponding to that character, if any.  At this point, we
 *  have to switch based on whether this node uses the short array or table
 *  method for representing its successors.
 *
 *  If this node uses the short array method of representing successors, we
 *  reach code point 285.  In this case, we iterate through the short array of
 *  successors until we do one of three things: run out of elements in the
 *  table (and reach code point 293 where we return NULL because there was no
 *  matching successor); reach a zero entry in the ``keys'' field (and reach
 *  code point 287 where we return NULL because again this means there was no
 *  matching successor); or we find a match (code point 290) and return the
 *  result of recursively calling this function using the matching successor
 *  node and the rest of the key.
 *
 *  If, on the other hand, this node uses the table method of representing
 *  successors, we reach code point 294.  In this case, what we do depends on
 *  whether we are using the 8-bit character optimization.  If this
 *  optimization is not being used (code point 296), we put the character
 *  corresponding to the first element in the table in the low_index local and
 *  check to see if the character in our key is less than this.  If so (code
 *  point 297), we return NULL.  Then we check to see if our character is
 *  beyond the end of the array and if so (code point 300), again we return
 *  NULL because there is no matching successor.  Note that both of these
 *  checks are unnessary for the 8-bit optimization case because in that case
 *  the range of the table is exactly the legal values of the ``char'' type, so
 *  in the 8-bit optimization case we just set low_index to the lowest value
 *  representable by the ``char'' type and we're ready to continue to code
 *  point 303.
 *
 *  When we reach code point 303, we know that our character is in the range of
 *  the table and that the low_index variable contains the value corresponding
 *  to the first element of the table, so *follow - low_index is the index of
 *  the table that will have our successor node.  If this table entry is NULL
 *  (code point 304) we return NULL.  Otherwise (code point 306), we
 *  recursively call this function on the successor node with the remainder of
 *  the key.
 *
 *  And that's the last of the code in this module.
 *
 *
 *      History
 *
 *  See the history discussion in string_index.h for the history of this code.
 *
 *
 *      Legal Issues
 *
 *  I've written this code from scratch, without using or refering to any other
 *  code, not even previous incarnations of similar functionality that I wrote
 *  myself (I don't even have any of the old versions easily acessible to me at
 *  the moment).  I've written it on my own equipment and not for hire for
 *  anyone else, so I have full legal rights to place it in the public domain.
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
 *          Chris Wilson, 2003-2004
 */


/* Performance Tuning Parameters */
#define SHORT_ARRAY_MAX_ENTRIES 4

#if (!defined(DISABLE_CHAR8_OPTIMIZATION)) && \
    (!defined(ENABLE_CHAR8_OPTIMIZATION))

#if CHAR_BIT == 8
#define ENABLE_CHAR8_OPTIMIZATION
#endif

#endif

#ifdef ASSERT_MALLOCED_SIZES
#include "memory_allocation_test.h"
#endif


typedef enum string_index_tag { SIT_SHORT_ARRAY, SIT_TABLE } string_index_tag;

struct string_index
  {
    char *prefix;
    boolean has_value;
    void *value;
    size_t children;
    string_index_tag tag;
    union
      {
        struct
          {
            string_index *indexes[SHORT_ARRAY_MAX_ENTRIES];
            char keys[SHORT_ARRAY_MAX_ENTRIES];
          } short_array;
        struct
          {
#ifndef ENABLE_CHAR8_OPTIMIZATION
            char low_index;
            size_t size;
#endif /* !ENABLE_CHAR8_OPTIMIZATION */
            string_index **indexes;
          } table;
      } sub;
  };


static boolean remove_and_return_true_if_object_gone(string_index *index,
        const char *key, boolean ok_to_delete);
static string_index *find_existing_node_for_key(string_index *index,
                                                const char *key);


extern string_index *create_string_index(void)
  {
    string_index *result;

    code_point(1);
    result = MALLOC_ONE_OBJECT(string_index);
    if (result == NULL)
      {
        code_point(2);
        return result;
      }
    else
      {
        code_point(3);
      }
    code_point(4);
    result->prefix = NULL;
    result->has_value = FALSE;
    result->value = NULL;
    result->children = 0;
    result->tag = SIT_SHORT_ARRAY;
    result->sub.short_array.keys[0] = 0;
    return result;
  }

extern void destroy_string_index(string_index *index)
  {
    code_point(5);
    assert(index != NULL);
    if (index->prefix != NULL)
      {
        code_point(6);
        free(index->prefix);
      }
    else
      {
        code_point(7);
      }
    code_point(8);
    switch (index->tag)
      {
        case SIT_SHORT_ARRAY:
          {
            size_t item_num;

            code_point(9);
            for (item_num = 0; item_num < SHORT_ARRAY_MAX_ENTRIES; ++item_num)
              {
                code_point(10);
                if (index->sub.short_array.keys[item_num] == 0)
                  {
                    code_point(11);
                    break;
                  }
                else
                  {
                    code_point(12);
                  }
                code_point(13);
                destroy_string_index(index->sub.short_array.indexes[item_num]);
              }
            code_point(14);
            break;
          }
        case SIT_TABLE:
          {
            size_t item_num;

            code_point(15);
#ifdef ENABLE_CHAR8_OPTIMIZATION
            code_point(16);
            for (item_num = 0; item_num < 256; ++item_num)
#else /* !ENABLE_CHAR8_OPTIMIZATION */
            code_point(17);
            for (item_num = 0; item_num < index->sub.table.size; ++item_num)
#endif /* !ENABLE_CHAR8_OPTIMIZATION */
              {
                code_point(18);
                if (index->sub.table.indexes[item_num] != NULL)
                  {
                    code_point(19);
                    destroy_string_index(index->sub.table.indexes[item_num]);
                  }
                else
                  {
                    code_point(20);
                  }
                code_point(21);
              }
            code_point(22);
            free(index->sub.table.indexes);
            break;
          }
        default:
          {
            assert(FALSE);
            break;
          }
      }
    code_point(23);
    free(index);
  }

extern verdict enter_into_string_index(string_index *index, const char *key,
                                       void *value)
  {
    const char *follow;

    code_point(24);
    assert(index != NULL);

    if ((index->children == 0) && (!(index->has_value)))
      {
        code_point(25);
        if (*key == 0)
          {
            code_point(26);
            if (index->prefix != NULL)
              {
                code_point(27);
                index->prefix[0] = 0;
              }
            else
              {
                code_point(28);
              }
            code_point(29);
          }
        else
          {
            code_point(30);
            if ((index->prefix != NULL) &&
                (strlen(index->prefix) >= strlen(key)))
              {
                code_point(31);
                strcpy(index->prefix, key);
              }
            else
              {
                char *new_prefix;

                code_point(32);
                new_prefix = MALLOC_ARRAY(char, strlen(key) + 1);
                if (new_prefix == NULL)
                  {
                    code_point(33);
                    return MISSION_FAILED;
                  }
                else
                  {
                    code_point(34);
                  }
                code_point(35);
                strcpy(new_prefix, key);
                if (index->prefix != NULL)
                  {
                    code_point(36);
                    free(index->prefix);
                  }
                else
                  {
                    code_point(37);
                  }
                code_point(38);
                index->prefix = new_prefix;
              }
            code_point(39);
          }
        code_point(40);
        index->value = value;
        index->has_value = TRUE;
        return MISSION_ACCOMPLISHED;
      }
    else
      {
        code_point(41);
      }

    code_point(42);
    follow = key;
    if (index->prefix != NULL)
      {
        char *follow_prefix;

        code_point(43);
        follow_prefix = index->prefix;
        while (*follow_prefix != 0)
          {
            code_point(44);
            if (*follow != *follow_prefix)
              {
                string_index *index2;
                string_index *index3;

                code_point(45);
                index2 = create_string_index();
                if (index2 == NULL)
                  {
                    code_point(46);
                    return MISSION_FAILED;
                  }
                else
                  {
                    code_point(47);
                  }
                code_point(48);
                if (follow_prefix[1] != 0)
                  {
                    char *prefix2;

                    code_point(49);
                    prefix2 =
                            MALLOC_ARRAY(char, strlen(follow_prefix + 1) + 1);
                    if (prefix2 == NULL)
                      {
                        code_point(50);
                        destroy_string_index(index2);
                        return MISSION_FAILED;
                      }
                    else
                      {
                        code_point(51);
                      }
                    code_point(52);
                    strcpy(prefix2, follow_prefix + 1);
                    index2->prefix = prefix2;
                  }
                else
                  {
                    code_point(53);
                  }
                code_point(54);
                if (*follow != 0)
                  {
                    code_point(55);
                    index3 = create_string_index();
                    if (index3 == NULL)
                      {
                        code_point(56);
                        destroy_string_index(index2);
                        return MISSION_FAILED;
                      }
                    else
                      {
                        code_point(57);
                      }
                    code_point(58);
                    if (follow[1] != 0)
                      {
                        char *prefix3;

                        code_point(59);
                        prefix3 = MALLOC_ARRAY(char, strlen(follow + 1) + 1);
                        if (prefix3 == NULL)
                          {
                            code_point(60);
                            destroy_string_index(index2);
                            destroy_string_index(index3);
                            return MISSION_FAILED;
                          }
                        else
                          {
                            code_point(61);
                          }
                        code_point(62);
                        strcpy(prefix3, follow + 1);
                        index3->prefix = prefix3;
                      }
                    else
                      {
                        code_point(63);
                      }
                    code_point(64);
                  }
                else
                  {
                    code_point(65);
                  }
                code_point(66);
                index2->has_value = index->has_value;
                index2->value = index->value;
                index2->children = index->children;
                index2->tag = index->tag;
                switch (index->tag)
                  {
                    case SIT_SHORT_ARRAY:
                      {
                        size_t item_num;

                        code_point(67);
                        for (item_num = 0; item_num < SHORT_ARRAY_MAX_ENTRIES;
                             ++item_num)
                          {
                            code_point(68);
                            index2->sub.short_array.keys[item_num] =
                                    index->sub.short_array.keys[item_num];
                            if (index->sub.short_array.keys[item_num] == 0)
                              {
                                code_point(69);
                                break;
                              }
                            else
                              {
                                code_point(70);
                              }
                            code_point(71);
                            index2->sub.short_array.indexes[item_num] =
                                    index->sub.short_array.indexes[item_num];
                          }
                        code_point(72);
                        break;
                      }
                    case SIT_TABLE:
                      {
                        code_point(73);
#ifdef ENABLE_CHAR8_OPTIMIZATION
                        code_point(74);
#else /* !ENABLE_CHAR8_OPTIMIZATION */
                        code_point(75);
                        index2->sub.table.low_index =
                                index->sub.table.low_index;
                        index2->sub.table.size = index->sub.table.size;
#endif /* !ENABLE_CHAR8_OPTIMIZATION */
                        code_point(76);
                        index2->sub.table.indexes = index->sub.table.indexes;
                        break;
                      }
                    default:
                      {
                        assert(FALSE);
                        break;
                      }
                  }
                code_point(77);
                index->tag = SIT_SHORT_ARRAY;
                index->sub.short_array.keys[0] = *follow_prefix;
                index->sub.short_array.indexes[0] = index2;
                if (*follow == 0)
                  {
                    code_point(78);
                    index->has_value = TRUE;
                    index->value = value;
                    index->children = 1;
                    index->sub.short_array.keys[1] = 0;
                  }
                else
                  {
                    code_point(79);
                    index->has_value = FALSE;
                    index->value = NULL;
                    index->children = 2;
                    index->sub.short_array.keys[1] = *follow;
                    index->sub.short_array.indexes[1] = index3;
                    index->sub.short_array.keys[2] = 0;
                    index3->has_value = TRUE;
                    index3->value = value;
                  }
                code_point(80);
                *follow_prefix = 0;
                return MISSION_ACCOMPLISHED;
              }
            else
              {
                code_point(81);
              }
            code_point(82);
            ++follow;
            ++follow_prefix;
          }
        code_point(83);
      }
    else
      {
        code_point(84);
      }

    code_point(85);
    if (*follow == 0)
      {
        code_point(86);
        index->value = value;
        index->has_value = TRUE;
        return MISSION_ACCOMPLISHED;
      }
    else
      {
        code_point(87);
      }

    code_point(88);
    switch (index->tag)
      {
        case SIT_SHORT_ARRAY:
          {
            size_t item_num;
            string_index *new_index;

            code_point(89);
            for (item_num = 0; item_num < SHORT_ARRAY_MAX_ENTRIES; ++item_num)
              {
                code_point(90);
                if (index->sub.short_array.keys[item_num] == 0)
                  {
                    code_point(91);
                    break;
                  }
                else
                  {
                    code_point(92);
                  }
                code_point(93);
                if (index->sub.short_array.keys[item_num] == *follow)
                  {
                    code_point(94);
                    return enter_into_string_index(
                            index->sub.short_array.indexes[item_num],
                            follow + 1, value);
                  }
                else
                  {
                    code_point(95);
                  }
                code_point(96);
              }

            code_point(97);
            new_index = create_string_index();
            if (new_index == NULL)
              {
                code_point(98);
                return MISSION_FAILED;
              }
            else
              {
                code_point(99);
              }
            code_point(100);
            if (follow[1] != 0)
              {
                char *new_prefix;

                code_point(101);
                new_prefix = MALLOC_ARRAY(char, strlen(follow + 1) + 1);
                if (new_prefix == NULL)
                  {
                    code_point(102);
                    destroy_string_index(new_index);
                    return MISSION_FAILED;
                  }
                else
                  {
                    code_point(103);
                  }
                code_point(104);
                strcpy(new_prefix, follow + 1);
                new_index->prefix = new_prefix;
              }
            else
              {
                code_point(105);
              }
            code_point(106);
            new_index->has_value = TRUE;
            new_index->value = value;
            if (item_num < SHORT_ARRAY_MAX_ENTRIES)
              {
                code_point(107);
                index->sub.short_array.keys[item_num] = *follow;
                index->sub.short_array.indexes[item_num] = new_index;
                if (item_num + 1 < SHORT_ARRAY_MAX_ENTRIES)
                  {
                    code_point(108);
                    index->sub.short_array.keys[item_num + 1] = 0;
                  }
                else
                  {
                    code_point(109);
                  }
                code_point(110);
              }
            else
              {
                char min_char;
#ifndef ENABLE_CHAR8_OPTIMIZATION
                char max_char;
#endif /* !ENABLE_CHAR8_OPTIMIZATION */
                size_t size;
                string_index **indexes;

                code_point(111);
#ifdef ENABLE_CHAR8_OPTIMIZATION
                code_point(112);
                size = 256;
                min_char = CHAR_MIN;
#else /* !ENABLE_CHAR8_OPTIMIZATION */
                code_point(113);
                min_char = *follow;
                max_char = *follow;
                for (item_num = 0; item_num < SHORT_ARRAY_MAX_ENTRIES;
                     ++item_num)
                  {
                    char new_char;

                    code_point(114);
                    new_char = index->sub.short_array.keys[item_num];
                    if (new_char < min_char)
                      {
                        code_point(115);
                        min_char = new_char;
                      }
                    else
                      {
                        code_point(116);
                      }
                    code_point(117);
                    if (new_char > max_char)
                      {
                        code_point(118);
                        max_char = new_char;
                      }
                    else
                      {
                        code_point(119);
                      }
                    code_point(120);
                  }
                code_point(121);
                size = (max_char - min_char) + 1;
                if (size < 256)
                  {
                    code_point(122);
                    size = 256;
                  }
                else
                  {
                    code_point(123);
                  }
                code_point(124);
#endif /* !ENABLE_CHAR8_OPTIMIZATION */
                code_point(125);
                indexes = MALLOC_ARRAY(string_index *, size);
                if (indexes == NULL)
                  {
                    code_point(126);
                    destroy_string_index(new_index);
                    return MISSION_FAILED;
                  }
                else
                  {
                    code_point(127);
                  }
                code_point(128);
                for (item_num = 0; item_num < size; ++item_num)
                  {
                    code_point(129);
                    indexes[item_num] = NULL;
                  }
                code_point(130);
                indexes[*follow - min_char] = new_index;
                for (item_num = 0; item_num < SHORT_ARRAY_MAX_ENTRIES;
                     ++item_num)
                  {
                    code_point(131);
                    indexes[index->sub.short_array.keys[item_num] - min_char] =
                            index->sub.short_array.indexes[item_num];
                  }
                code_point(132);
                index->tag = SIT_TABLE;
#ifdef ENABLE_CHAR8_OPTIMIZATION
                code_point(133);
#else /* !ENABLE_CHAR8_OPTIMIZATION */
                code_point(134);
                index->sub.table.low_index = min_char;
                index->sub.table.size = size;
#endif /* !ENABLE_CHAR8_OPTIMIZATION */
                code_point(135);
                index->sub.table.indexes = indexes;
              }
            code_point(136);
            ++(index->children);
            break;
          }
        case SIT_TABLE:
          {
            char low_index;
            string_index *new_index;

            code_point(137);
#ifdef ENABLE_CHAR8_OPTIMIZATION
            code_point(138);
            low_index = CHAR_MIN;
#else /* !ENABLE_CHAR8_OPTIMIZATION */
            code_point(139);
            low_index = index->sub.table.low_index;
            if (*follow < low_index)
              {
                size_t size_diff;
                size_t old_size;
                size_t new_size;
                string_index **new_indexes;
                size_t item_num;

                code_point(140);
                size_diff = (low_index - *follow);
                old_size = index->sub.table.size;
                new_size = old_size + size_diff;
                new_indexes = MALLOC_ARRAY(string_index *, new_size);
                if (new_indexes == NULL)
                  {
                    code_point(141);
                    return MISSION_FAILED;
                  }
                else
                  {
                    code_point(142);
                  }
                code_point(143);
                for (item_num = 0; item_num < size_diff; ++item_num)
                  {
                    code_point(144);
                    new_indexes[item_num] = NULL;
                  }
                code_point(145);
                for (; item_num < new_size; ++item_num)
                  {
                    code_point(146);
                    new_indexes[item_num] =
                            index->sub.table.indexes[item_num - size_diff];
                  }
                code_point(147);
                free(index->sub.table.indexes);
                index->sub.table.indexes = new_indexes;
                index->sub.table.low_index = *follow;
                low_index = *follow;
                index->sub.table.size = new_size;
              }
            else if (*follow >= low_index + index->sub.table.size)
              {
                size_t old_size;
                size_t new_size;
                string_index **new_indexes;
                size_t item_num;

                code_point(148);
                old_size = index->sub.table.size;
                new_size = (*follow - low_index) + 1;
                new_indexes = MALLOC_ARRAY(string_index *, new_size);
                if (new_indexes == NULL)
                  {
                    code_point(149);
                    return MISSION_FAILED;
                  }
                else
                  {
                    code_point(150);
                  }
                code_point(151);
                for (item_num = 0; item_num < old_size; ++item_num)
                  {
                    code_point(152);
                    new_indexes[item_num] = index->sub.table.indexes[item_num];
                  }
                code_point(153);
                for (; item_num < new_size; ++item_num)
                  {
                    code_point(154);
                    new_indexes[item_num] = NULL;
                  }
                code_point(155);
                free(index->sub.table.indexes);
                index->sub.table.indexes = new_indexes;
                index->sub.table.size = new_size;
              }
            else
              {
                code_point(156);
#endif /* !ENABLE_CHAR8_OPTIMIZATION */
                code_point(157);
                if (index->sub.table.indexes[*follow - low_index] != NULL)
                  {
                    code_point(158);
                    return enter_into_string_index(
                            index->sub.table.indexes[*follow - low_index],
                            follow + 1, value);
                  }
                else
                  {
                    code_point(159);
                  }
                code_point(160);
#ifdef ENABLE_CHAR8_OPTIMIZATION
                code_point(161);
#else /* !ENABLE_CHAR8_OPTIMIZATION */
                code_point(162);
              }
            code_point(163);
#endif /* !ENABLE_CHAR8_OPTIMIZATION */

            code_point(164);
            new_index = create_string_index();
            if (new_index == NULL)
              {
                code_point(165);
                return MISSION_FAILED;
              }
            else
              {
                code_point(166);
              }
            code_point(167);
            if (follow[1] != 0)
              {
                char *new_prefix;

                code_point(168);
                new_prefix = MALLOC_ARRAY(char, strlen(follow + 1) + 1);
                if (new_prefix == NULL)
                  {
                    code_point(169);
                    destroy_string_index(new_index);
                    return MISSION_FAILED;
                  }
                else
                  {
                    code_point(170);
                  }
                code_point(171);
                strcpy(new_prefix, follow + 1);
                new_index->prefix = new_prefix;
              }
            else
              {
                code_point(172);
              }

            code_point(173);
            new_index->has_value = TRUE;
            new_index->value = value;
            index->sub.table.indexes[*follow - low_index] = new_index;
            ++(index->children);

            break;
          }
        default:
          {
            assert(FALSE);
            break;
          }
      }

    code_point(174);
    return MISSION_ACCOMPLISHED;
  }

extern void remove_from_string_index(string_index *index, const char *key)
  {
    code_point(175);
    remove_and_return_true_if_object_gone(index, key, FALSE);
  }

extern boolean exists_in_string_index(string_index *index, const char *key)
  {
    string_index *matching_node;

    code_point(176);
    matching_node = find_existing_node_for_key(index, key);
    if (matching_node == NULL)
      {
        code_point(177);
        return FALSE;
      }
    else
      {
        code_point(178);
        return matching_node->has_value;
      }
  }

extern void *lookup_in_string_index(string_index *index, const char *key)
  {
    string_index *matching_node;

    code_point(179);
    matching_node = find_existing_node_for_key(index, key);
    if (matching_node == NULL)
      {
        code_point(180);
        return NULL;
      }
    else
      {
        code_point(181);
        return matching_node->value;
      }
  }

extern void check_string_index_integrity(string_index *index)
  {
    size_t children;

    code_point(182);
    assert(index != NULL);
    assert_is_malloced_block_with_exact_size(index, sizeof(string_index));

    if (index->prefix != NULL)
      {
        code_point(183);
        assert_is_malloced_block_with_minimum_size(index->prefix,
                                                   strlen(index->prefix) + 1);
      }
    else
      {
        code_point(184);
      }

    code_point(185);
    if (!(index->has_value))
      {
        code_point(186);
        assert(index->value == NULL);
      }
    else
      {
        code_point(187);
      }

    code_point(188);
    children = 0;
    switch (index->tag)
      {
        case SIT_SHORT_ARRAY:
          {
            size_t item_num;

            code_point(189);
            for (item_num = 0; item_num < SHORT_ARRAY_MAX_ENTRIES; ++item_num)
              {
                code_point(190);
                if (index->sub.short_array.keys[item_num] == 0)
                  {
                    code_point(191);
                    break;
                  }
                else
                  {
                    code_point(192);
                  }
                code_point(193);
                ++children;
                check_string_index_integrity(
                        index->sub.short_array.indexes[item_num]);

              }
            code_point(194);
            break;
          }
        case SIT_TABLE:
          {
            size_t item_num;

            code_point(195);
#ifdef ENABLE_CHAR8_OPTIMIZATION
            code_point(196);
            for (item_num = 0; item_num < 256; ++item_num)
#else /* !ENABLE_CHAR8_OPTIMIZATION */
            code_point(197);
            for (item_num = 0; item_num < index->sub.table.size; ++item_num)
#endif /* !ENABLE_CHAR8_OPTIMIZATION */
              {
                code_point(198);
                if (index->sub.table.indexes[item_num] != NULL)
                  {
                    code_point(199);
                    ++children;
                    check_string_index_integrity(
                            index->sub.table.indexes[item_num]);
                  }
                else
                  {
                    code_point(200);
                  }
                code_point(201);
              }
            code_point(202);
            assert_is_malloced_block_with_exact_size(index->sub.table.indexes,
                    sizeof(string_index *) * item_num);
            break;
          }
        default:
          {
            assert(FALSE);
            break;
          }
      }
    code_point(203);
    assert(children == index->children);
  }


static boolean remove_and_return_true_if_object_gone(string_index *index,
        const char *key, boolean ok_to_delete)
  {
    const char *follow;

    code_point(204);
    assert(index != NULL);

    follow = key;
    if (index->prefix != NULL)
      {
        const char *follow_prefix;

        code_point(205);
        follow_prefix = index->prefix;
        while (*follow_prefix != 0)
          {
            code_point(206);
            if (*follow != *follow_prefix)
              {
                code_point(207);
                return FALSE;
              }
            else
              {
                code_point(208);
              }
            code_point(209);
            ++follow;
            ++follow_prefix;
          }
        code_point(210);
      }
    else
      {
        code_point(211);
      }

    code_point(212);
    if (*follow == 0)
      {
        code_point(213);
        if ((index->children == 0) && ok_to_delete)
          {
            code_point(214);
            destroy_string_index(index);
            return TRUE;
          }
        else
          {
            code_point(215);
          }
        code_point(216);
        index->has_value = FALSE;
        index->value = NULL;
        return FALSE;
      }
    else
      {
        code_point(217);
      }

    code_point(218);
    switch (index->tag)
      {
        case SIT_SHORT_ARRAY:
          {
            size_t item_num;

            code_point(219);
            for (item_num = 0; item_num < SHORT_ARRAY_MAX_ENTRIES; ++item_num)
              {
                code_point(220);
                if (index->sub.short_array.keys[item_num] == 0)
                  {
                    code_point(221);
                    return FALSE;
                  }
                else
                  {
                    code_point(222);
                  }
                code_point(223);
                if (index->sub.short_array.keys[item_num] == *follow)
                  {
                    boolean gone;

                    code_point(224);
                    assert(index->children > 0);
                    gone = remove_and_return_true_if_object_gone(
                            index->sub.short_array.indexes[item_num],
                            follow + 1, TRUE);
                    if (!gone)
                      {
                        code_point(225);
                        return FALSE;
                      }
                    else
                      {
                        code_point(226);
                      }
                    code_point(227);
                    while (TRUE)
                      {
                        code_point(228);
                        if (item_num >= SHORT_ARRAY_MAX_ENTRIES - 1)
                          {
                            code_point(229);
                            index->sub.short_array.keys[item_num] = 0;
                            break;
                          }
                        else
                          {
                            code_point(230);
                          }
                        code_point(231);
                        index->sub.short_array.keys[item_num] =
                                index->sub.short_array.keys[item_num + 1];
                        if (index->sub.short_array.keys[item_num + 1] == 0)
                          {
                            code_point(232);
                            break;
                          }
                        else
                          {
                            code_point(233);
                          }
                        code_point(234);
                        index->sub.short_array.indexes[item_num] =
                                index->sub.short_array.indexes[item_num + 1];
                        ++item_num;
                      }
                    code_point(235);
                    --(index->children);
                    if (index->children > 0)
                      {
                        code_point(236);
                        return FALSE;
                      }
                    else
                      {
                        code_point(237);
                      }
                    code_point(238);
                    if (index->has_value)
                      {
                        code_point(239);
                        return FALSE;
                      }
                    else
                      {
                        code_point(240);
                      }
                    code_point(241);
                    if (!ok_to_delete)
                      {
                        code_point(242);
                        return FALSE;
                      }
                    else
                      {
                        code_point(243);
                      }
                    code_point(244);
                    destroy_string_index(index);
                    return TRUE;
                  }
                else
                  {
                    code_point(245);
                  }
                code_point(246);
              }
            code_point(247);
            return FALSE;
          }
        case SIT_TABLE:
          {
            char low_index;
            boolean gone;

            code_point(248);
#ifdef ENABLE_CHAR8_OPTIMIZATION
            code_point(249);
            low_index = CHAR_MIN;
#else /* !ENABLE_CHAR8_OPTIMIZATION */
            code_point(250);
            low_index = index->sub.table.low_index;
            if (*follow < low_index)
              {
                code_point(251);
                return FALSE;
              }
            else
              {
                code_point(252);
              }
            code_point(253);
            if (*follow >= low_index + index->sub.table.size)
              {
                code_point(254);
                return FALSE;
              }
            else
              {
                code_point(255);
              }
            code_point(256);
#endif /* !ENABLE_CHAR8_OPTIMIZATION */
            code_point(257);
            if (index->sub.table.indexes[*follow - low_index] == NULL)
              {
                code_point(258);
                return FALSE;
              }
            else
              {
                code_point(259);
              }
            code_point(260);

            assert(index->children > 0);
            gone = remove_and_return_true_if_object_gone(
                    index->sub.table.indexes[*follow - low_index], follow + 1,
                    TRUE);
            if (!gone)
              {
                code_point(261);
                return FALSE;
              }
            else
              {
                code_point(262);
              }
            code_point(263);
            index->sub.table.indexes[*follow - low_index] = NULL;
            --(index->children);
            if (index->children > 0)
              {
                code_point(264);
                return FALSE;
              }
            else
              {
                code_point(265);
              }
            code_point(266);
            if (index->has_value)
              {
                code_point(267);
                return FALSE;
              }
            else
              {
                code_point(268);
              }
            code_point(269);
            if (!ok_to_delete)
              {
                code_point(270);
                return FALSE;
              }
            else
              {
                code_point(271);
              }
            code_point(272);
            destroy_string_index(index);
            return TRUE;
          }
        default:
          {
            assert(FALSE);
            return FALSE;
          }
      }
  }

static string_index *find_existing_node_for_key(string_index *index,
                                                const char *key)
  {
    const char *follow;

    code_point(273);
    assert(index != NULL);

    follow = key;
    if (index->prefix != NULL)
      {
        const char *follow_prefix;

        code_point(274);
        follow_prefix = index->prefix;
        while (*follow_prefix != 0)
          {
            code_point(275);
            if (*follow != *follow_prefix)
              {
                code_point(276);
                return NULL;
              }
            else
              {
                code_point(277);
              }
            code_point(278);
            ++follow;
            ++follow_prefix;
          }
        code_point(279);
      }
    else
      {
        code_point(280);
      }

    code_point(281);
    if (*follow == 0)
      {
        code_point(282);
        return index;
      }
    else
      {
        code_point(283);
      }

    code_point(284);
    switch (index->tag)
      {
        case SIT_SHORT_ARRAY:
          {
            size_t item_num;

            code_point(285);
            for (item_num = 0; item_num < SHORT_ARRAY_MAX_ENTRIES; ++item_num)
              {
                code_point(286);
                if (index->sub.short_array.keys[item_num] == 0)
                  {
                    code_point(287);
                    return NULL;
                  }
                else
                  {
                    code_point(288);
                  }
                code_point(289);
                if (index->sub.short_array.keys[item_num] == *follow)
                  {
                    code_point(290);
                    return find_existing_node_for_key(
                            index->sub.short_array.indexes[item_num],
                            follow + 1);
                  }
                else
                  {
                    code_point(291);
                  }
                code_point(292);
              }
            code_point(293);
            return NULL;
          }
        case SIT_TABLE:
          {
            char low_index;

            code_point(294);
#ifdef ENABLE_CHAR8_OPTIMIZATION
            code_point(295);
            low_index = CHAR_MIN;
#else /* !ENABLE_CHAR8_OPTIMIZATION */
            code_point(296);
            low_index = index->sub.table.low_index;
            if (*follow < low_index)
              {
                code_point(297);
                return NULL;
              }
            else
              {
                code_point(298);
              }
            code_point(299);
            if (*follow >= low_index + index->sub.table.size)
              {
                code_point(300);
                return NULL;
              }
            else
              {
                code_point(301);
              }
            code_point(302);
#endif /* !ENABLE_CHAR8_OPTIMIZATION */
            code_point(303);
            if (index->sub.table.indexes[*follow - low_index] == NULL)
              {
                code_point(304);
                return NULL;
              }
            else
              {
                code_point(305);
              }
            code_point(306);
            return find_existing_node_for_key(
                    index->sub.table.indexes[*follow - low_index], follow + 1);
          }
        default:
          {
            assert(FALSE);
            return NULL;
          }
      }
  }
