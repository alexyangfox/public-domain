/* file "string_index.h" */

/*
 *  This file contains the interface to code to do an efficient index with
 *  ASCII strings as keys and ``void *'' pointers as the indexed data.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef STRING_INDEX_H
#define STRING_INDEX_H

#include "basic.h"


/*
 *      Usage
 *
 *  The interface is fairly self-explanatory.  The string_index object
 *  logically consists of a finite set of key/value pairs.  The keys are ASCII
 *  strings and the values are arbitrary pointers.
 *
 *  Externally, the data fields of the string_index are invisible.  All
 *  external accesses are through the seven functions declared in this header
 *  file.  Two of the functions allow the creation and destruction of
 *  string_index objects.  When a string_index object is created, it is empty
 *  -- in the conceptual model of key/value pairs, the list of those pairs for
 *  that object is empty.
 *
 *  The enter_into_string_index() function adds or changes key/value pairs and
 *  the remove_from_string_index() function removes key/value pairs.  These are
 *  the only two functions that modify string_index objects.  The
 *  enter_into_string_index() function takes the string_index and the new key
 *  and new value for that key.  If the key doesn't already exist in that
 *  string_index, the key/value pair is added.  If the key already exists in
 *  that string_index, the key/value pair that has that key is replaced by the
 *  new key/value pair.  So there can never be more than one key/value pair in
 *  a given string_index with a particular key.  If the operation succeeds,
 *  MISSION_ACCOMPLISHED is returned; otherwise MISSION_FAILED is returned and
 *  the string index is left unchanged.  The remove_from_string_index()
 *  function removes the key/value pair with the specified key, if it exists in
 *  that string_index.  If the specified key does not exist, there is no
 *  effect.
 *
 *  The next two functions, exists_in_string_index() and
 *  lookup_in_string_index(), don't change the string_index but rather query
 *  information from it.  The exists_in_string_index() function checks to see
 *  whether or not a given key is in the string_index.  The
 *  lookup_in_string_index() function returns either the value associated with
 *  a given key, if that key exists in the string_index, or NULL if the key
 *  does not exist in the string_index.
 *
 *  Note that NULL is a valid value and there is a difference between a key
 *  that is associated with the value NULL and a key that is not present at all
 *  in the string_index.  These two cases aren't distinguishable with the
 *  lookup_in_string_index() function, which will return NULL in either case,
 *  but they are distinguishable with the exists_in_string_index() function.
 *  Of course, whether or not the client code treats these two cases as
 *  logically equivalent is up to the client code.
 *
 *  For example, if you have:
 *
 *      string_index *myindex = create_string_index();
 *      enter_into_string_index(myindex, "a", "Alpha");
 *      enter_into_string_index(myindex, "b", NULL);
 *
 *  then the following calls will return these values:
 *
 *      exists_in_string_index(myindex, "a") --> TRUE
 *      lookup_in_string_index(myindex, "a") --> "Alpha"
 *      exists_in_string_index(myindex, "b") --> TRUE
 *      lookup_in_string_index(myindex, "b") --> NULL
 *      exists_in_string_index(myindex, "c") --> FALSE
 *      lookup_in_string_index(myindex, "c") --> NULL
 *
 *  The final function, check_string_index_integrity(), checks to make sure
 *  that nothing is messed up internally in the string_index.  Unless the data
 *  structures are corrupted or there is a bug in the string_index code, this
 *  should never find any problems.  In production code it probably isn't
 *  necessary to use it, especially since it takes some time to do its
 *  checking, but it can be useful for debugging, much as an assert() is useful
 *  for debugging.  This function uses assert(), in fact, to do its checks, so
 *  compiling a release version of this code that makes assert() have no effect
 *  will mean this function doesn't do much.  If this function finds any
 *  problem, the result will be an assertion failure.
 *
 *
 *      Error Handling
 *
 *  The only way that any of the operations in this module can fail is if they
 *  are unable to allocate memory.  This can only happen to the
 *  create_string_index() and enter_into_string_index() functions.  If
 *  create_string_index() fails trying to allocate memory, it will return NULL.
 *  If enter_into_string_index() fails, it will return MISSION_FAILED with no
 *  change to the string_index.
 *
 *  Of course, the previous paragraph doesn't apply if there are bugs in this
 *  module, if there are bugs in the compiler compiling this module that cause
 *  incorrect code to be generated for it, or if external memory corruption
 *  bugs corrupt the data structures or code used by this module.  In those
 *  cases, all bets are off and there's no way to guarantee what might happen.
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
 *  This module is designed to provide a relatively simple, efficient,
 *  general-purpose way of looking up values by string for C code.  The details
 *  of the implementation are hidden so that a different implementation can be
 *  substituted if there is a reason to do so in the future and that
 *  substitution will not require changes to the client code.
 *
 *  ``void *'' was chosen as the value type because it is anticipated that that
 *  will be fine for most of the places functionality like this will be
 *  desired.  I've found that usually it's a pointer of some sort that I want
 *  as the value type, though not in all cases.  In some cases, a level of
 *  indirection will have to be introduced to get to the values that are really
 *  of interest.  Such are the limitations of C.  If I were writing this for
 *  ML, that language's polymorphism would naturally allow any value type, and
 *  if I were writing it for C++, a template could do the same thing (though
 *  with templates it's hard to hide the implementation).
 *
 *  This is intended for the kinds of places where many programmers would use
 *  hash tables.  Instead of using hash tables, though, I've implemented this
 *  module with a tree structure.  Hash table proponents like the
 *  ultra-efficiency of hash tables.  I, however, am not comfortable with the
 *  unpredictability of hash tables -- in the worst case, you get everything in
 *  one bin and then it is inefficient.  If you consider the hash value
 *  generation as part of the cost of each operation, and your hash value is
 *  based on the entire string, not some prefix of it (using just a prefix is a
 *  bad way to do it because in some applications there will be a bunch of
 *  values with the same prefix), then this code is as efficient as the
 *  best-case of the hash table implementation.  They are both linear in the
 *  number of characters in the key string, for each operation
 *  (enter_into_string_index(), remove_from_string_index(),
 *  exists_in_string_index(), and lookup_in_string_index()).  It is true that
 *  if you use hash tables you might compute the hash value once and use it
 *  multiple times, but if it's just a constant number of times then the
 *  complexity is the same (linear) and it's unlikely that you'll be computing
 *  a hash value and re-using that same hash value in more than a constant
 *  number of ways.  If you really are computing a hash value and re-using it a
 *  lot, you might as well just be keeping a pointer to your value instead of
 *  the hash value -- the whole point of an index like this is when you've got
 *  a new string that you want to look up, not when you have a string you
 *  already have data associated with.
 *
 *  The tree structure used here takes up space that is linear or less in the
 *  number of characters in all the keys currently in the string_index.  This
 *  is true even if there were a bunch of other keys that were added and then
 *  removed -- i.e. the fact that there were other keys added and removed can
 *  only increase the space taken by at most a constant factor.
 *
 *  Actually, the limit on the space taken up is a little better, but it's more
 *  complicated to explain the real limit.  The real limit on the space is
 *  linear in a count of all the characters in all the keys minus the
 *  characters in common prefixes.  That is, if several keys share a prefix,
 *  the prefix characters only count once, not for every one of the keys they
 *  are in.
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
 *  domain at that time.  I've written similar code more than once before, such
 *  as a C++ implementation for the SUIF compiler system.  But now I want a
 *  version that is my own, free of anyone else's copyright interest, and I
 *  want to be able to make it freely available to everyone.  I intend it first
 *  for two particular projects I'm working on, the CWX1 C interpreter and the
 *  Snapper program for Linux kernel configuration.  But I'll also make it
 *  available by itself, in case anyone else finds it useful to incorporate
 *  into their own code.
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
 *          Chris Wilson, 2003
 */

typedef struct string_index string_index;

extern string_index *create_string_index(void);
extern void destroy_string_index(string_index *index);
extern verdict enter_into_string_index(string_index *index, const char *key,
                                       void *value);
extern void remove_from_string_index(string_index *index, const char *key);
extern boolean exists_in_string_index(string_index *index, const char *key);
extern void *lookup_in_string_index(string_index *index, const char *key);
extern void check_string_index_integrity(string_index *index);


#endif /* STRING_INDEX_H */
