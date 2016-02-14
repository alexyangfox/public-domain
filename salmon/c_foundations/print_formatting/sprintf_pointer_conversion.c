/* file "sprintf_pointer_conversion.c" */

/*
 *  This file contains the implementation of one particular algorithm to do
 *  pointer conversion to ASCII.  This algorithm uses the sprintf() function to
 *  do the conversion itself, and provides the conversion through the interface
 *  used by the print formatting module.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include "basic.h"
#include "pointer_plug_in.h"
#include "print_formatting/sprintf_pointer_conversion.h"


/*
 *  @@@
 *      Data Structures
 *
 *  Before we get into the details of the data structures, I'll introduce a bit
 *  of terminology.  The set of integers from zero through num_elements-1
 *  inclusive is important for this module.  It is an ordering of all these
 *  integers that will form the final result.  Each of the lists to be merged
 *  consists of some or all of these integers.  So it's useful in this
 *  discussion to have a term for them.  For the purposes of this discussion,
 *  they will be called the ``core element''.  That makes it easier to
 *  distinguish between the core element, which can occur in several lists, and
 *  a particular instance of that core element as an element of one of the
 *  lists to be merged.
 *
 *  Aside from the input and output arrays specified by the interface, there
 *  are two primary data structures used in this module.
 *
 *  The first data structure is an array of element_global_info objects, with
 *  one for each core element.  This structure is dynamically allocated and a
 *  pointer to it is passed around to various functions, with each pointer to
 *  it named element_info.  This data structure provides a place for any
 *  information for which there is at most one copy per core element.
 *
 *  The second major data structure consists of a set of arrays of objects of
 *  type link_info.  These structures are dynamically allocated so that there
 *  is one such array for each of the lists to be merged and the number of
 *  elements in each array is equal to the number of elements in the
 *  corresponding one of the lists to be merged.  While they are allocated this
 *  way, though, they are used to form linked lists, where each element in each
 *  array as also an element in some linked list.  The linked lists are set up
 *  so that there is one list per core element, and the list for each core
 *  element lists the successors of that element in all the arrays to be merged
 *  of which it is a part.  That is, the arrays to be merged can be thought of
 *  as a way of encoding a graph, where one element coming after another in an
 *  array corresponds to the second element having an edge in the graph coming
 *  from the first element.  The link_info objects form a linked list for each
 *  node where the linked list shows all the successors of that node.  The fact
 *  that the link_info objects are allocated in arrays instead of individually
 *  as in most linked lists is a convenience for allocation and deallocation
 *  that works because of where this graph comes from in the first place, but
 *  aside from allocation and deallocation, the link_info objects are treated
 *  as a linked list, not as array elements.  As the algorithm progresses,
 *  elements can be removed from these linked lists, but elements are not
 *  added, and thus no additional allocation is needed.
 *
 *  So, one of the things that the element_global_info and link_info objects do
 *  is represent the graph structure of the arrays to be merged in a different
 *  format, a format which makes it easier to find all the successors of any
 *  given element.  Note that for purposes of this algorithm, we only care
 *  about this graph structure, not about additional information that is lost
 *  in the conversion to graph form.  For example, if the input lists are
 *  {0, 1, 2} and {2, 3}, they would form the same graph structure as if the
 *  input lists had been {0, 1} and {1, 2, 3}, but for the purposes of this
 *  algorithm those differences don't matter.  So the graph structure we
 *  construct represents the information we do care about while removing some
 *  information we don't care about.  Also, it represents it in a form that
 *  makes it easy to drop any particular edge, which, as we shall see, is going
 *  to be useful in the algorithm.
 *
 *  So now we get to the actual C code for the data structures.  After the
 *  includes and comments, there is a block of pre-processing directives before
 *  the data structures are introduced.  This section begins with
 *  ``#ifndef ASSERT_MALLOCED_SIZES''.  If the user doesn't explicitly define
 *  the ASSERT_MALLOCED_SIZES macro, the
 *  assert_is_malloced_block_with_exact_size() macro evaluates to nothing, but
 *  otherwise it includes "memory_allocation_test.h", which defines this as an
 *  external function.  This is called to assert properties of pointers to
 *  internal data structures.  For production, this call evaluates to nothing,
 *  but when ``-DASSERT_MALLOCED_SIZES'' is used, it instead calls code in an
 *  external memory checking module to do the requested checks.  Note that to
 *  use this feature requires memory_allocation_test.h and
 *  memory_allocation_test.c, which are both written by me and which are both
 *  in the public domain.
 *
 *  Now we get to the first real code, which is the definition of the
 *  element_state enumeration.  Each core element will have one field of this
 *  type, so each core element will at any given time be in one of the three
 *  states specified in this enumeration.  The algorithm will at one point be
 *  walking the tree of core elements in depth-first fashion, so during this
 *  time there will be a stack of core elements.  Any elements not yet visited
 *  will be in the ES_UNVISITED state.  Any elements currently on the stack
 *  (visiting them has begun, but they are still being processed) will be in
 *  the ES_BEING_VISITED state.  And any elements that have already been
 *  visited in the tree walk will be in the ES_FINISHED state.
 *
 *  Next comes the definition of the element_global_info structure.  As
 *  mentioned above, there will be one object of this type for each core
 *  element.
 *
 *  The ``state'' field is of type element_state, and reflects the state of the
 *  core element (as described above) during the tree walk (to be described
 *  soon).  The incoming_link_count field is kept up-to-date with the number of
 *  remaining incoming links to this core element in the graph.  For the
 *  algorithm, we will not need to be able to follow those links back.  All we
 *  really need to know is whether there are any left, so we can do that by
 *  keeping a count and decrementing it when a link to it is removed.
 *
 *  The first_link field of the element_global_info structure contains the head
 *  of the linked list of edges in the graph coming out of that core element.
 *  The first_unsearched_link field always points to a link pointer in the
 *  linked list that is headed by first_link.  That is, it contains either the
 *  location of first_link or the location of the next pointer of one of the
 *  elements of the list pointed to by first_link.  It serves as a bookmark of
 *  how far through the successor list we've searched already in case we need
 *  to come back to it and continue the search, and don't want to have to
 *  re-examine the links that have already been handled.  Initially, it is the
 *  location of first_link, because none of the outgoing edges has been handled
 *  yet.  It is a pointer to the link rather than a pointer to the element that
 *  link points to because the element might have to be removed from the list
 *  if a cycle is found, so the pointer to it might have to be updated.
 *
 *  The final type definition is that of the link_info structure.  As described
 *  above, objects of this type are used to construct the outgoing edge linked
 *  lists of the graph.  The two fields implement the linked list of edges: the
 *  target_element field specifies which core element the edge points to and
 *  the next_link field determines the next object in the linked list.
 *
 *  And that completes the detailed explanation of the data structures used
 *  here.
 *
 *
 *      Algorithm
 *
 *  Here's an outline of how the algorithm works.  First, the graph structure
 *  is put into the form of element_global_info and link_info objects.  Then,
 *  the whole graph is walked looking for cycles.  Any cycles that are found
 *  are removed.  That is, every edge in the cycle is removed.  Any nodes that
 *  are left with no incoming edges are put on the start list.  This is
 *  repeated until no cycles are left.  Then, the remaining graph is walked,
 *  with the edges being removed as they are traversed.  Any node that has
 *  other incoming edges is ignored.  Only when the last edge into a node is
 *  walked is the node put in the output list and the edges from that node
 *  walked.
 *
 *  The key step here is the removal of cycles.  Each cycles represent
 *  conflicts among the different input arrays about the ordering of the
 *  elements in that cycle.  As long as there are cycles, there is no way to
 *  order all the elements in such a way that every edges goes from an earlier
 *  element to a later element in the final ordering.  As we find cycles, we
 *  remove every element in each cycle.  So if an element tends to follow
 *  another in most cases but in a few its the other way around, the removal of
 *  cycles will tend to remove as many edges each direction, making it likely
 *  that the element that usually follows ends up following in the result.
 *
 *      Functions
 *
 *      merge_dense_integer_arrays()
 *
 *  The merge_dense_integer_arrays() function is the one function in this
 *  module that is externally accessable.  It is the top-level function that
 *  carries out the merge operation.
 *
 *  The first thing it does, at code point 1, is assert some checks on its
 *  parameters.  Then it calls create_internal_structures(), which creates both
 *  the array of element_global_info objects and the array of arrays of
 *  link_info objects.  That function returns a pointer to the array of
 *  element_global_info objects and sets the location pointed to by its
 *  link_arrays_location parameter to point to the array of arrays of link_info
 *  objects.  It also fills in all of these data structures with the starting
 *  information based on the arrays to be merged, which includes setting up the
 *  linked lists of successors for each core element.  In addition, it creates
 *  one more linked list, the head list, which is also a linked list of
 *  link_info objects but which specifies all the head elements of all the
 *  lists.  This head list is also composed of elements in the array of arrays
 *  of link_info objects.  The location pointed to by the head_list parameter
 *  is set to point to this list and the head_list_tail is set to point to the
 *  last pointer in the list (the location of the NULL that terminates the
 *  list).  This head_list_tail allows elements to be appended to the list
 *  efficiently.  If the create_internal_structures() function fails (it can
 *  fail if malloc() fails), it returns NULL and sets the link_info array of
 *  arrays pointer to NULL also.  Then the ``if'' statement right after the
 *  call to create_internal_structures() tests for this failure condition.  If
 *  element_info is NULL (code point 2), then the code asserts that link_arrays
 *  is also NULL and returns NULL to indicate a failure.
 *
 *  Once create_internal_structures() has been called successfully (code point
 *  4), some assertions are made about the data structures set up by that
 *  function.  Then it walks through the head element list and for each element
 *  (code point 5) it calls remove_cycles().  The remove_cycles() function is
 *  the heart of the algorithm.  It recursively walks the graph looking for
 *  cycles.  Calling it for each element in the head list insures that it walks
 *  the entire graph.  As the name implies, this function removes all the
 *  cycles it finds, so when the loop is done and all the calls to
 *  remove_cycles() have been made, all cycles in the graph are gone.  It
 *  asserts that the result returned by remove_cycles() is TRUE because a
 *  result of FALSE is used to indicate the incoming edge is part of a cycle,
 *  which can't be because this is being entered from the head list, not from
 *  another node.
 *
 *  Next, at code point 6, the code calls order_elements() to walk the graph
 *  and create the result array.  If order_elements() fails (because malloc()
 *  failed), it returns NULL, so right after the call to order_elements() is a
 *  check to see if the result is NULL.  If it is not NULL (code point 7), the
 *  result is asserted to be of the correct size.  Note that if it is NULL
 *  (code point 8), nothing special is done because this NULL result will
 *  simply be returned by this function anyway, after the internal data
 *  structures have been deallocated.
 *
 *  Finally, at code point 9, delete_internal_structures() is called to
 *  deallocate all the internal data structures that were allocated.  Note that
 *  this does not include the result array since that will be passed back as
 *  the result.  Then, the result array is returned.
 *
 *      create_internal_structures()
 *
 *  Next is the create_internal_structures() function.  This is what allocates
 *  the internal data structures and fills them in, translating from the array
 *  representation passed in to the graph structure used internally.
 *
 *  To start with, at code point 10, this function makes some assertions about
 *  its input parameters.  Then, malloc() is called to allocate element_info,
 *  the array of element_global_info objects.  If the malloc() fails (code
 *  point 11), the appropriate message is printed to standard error and NULL is
 *  both returned and set in the location pointed to by link_arrays_location,
 *  to indicate failure.
 *
 *  Next (code point 13), the array of pointers to link_info objects is
 *  allocated.  There is one element in this array for each of the input arrays
 *  to be merged.  If the allocation fails (code point 14) what element_info
 *  points to is freed and again an error message is printed to standard error
 *  and NULL is both returned and set in the location pointed to by
 *  link_arrays_location.
 *
 *  After that, at code point 16, it's time to set up the initial state of the
 *  elements in the array pointed to by element_info.  For each element (code
 *  point 17), the state is set to ES_UNVISITED, the incoming link count is set
 *  to zero, and the first_link field is set to NULL.  That is, the graph
 *  structure is set to have no edges.  The first_unsearched_link field is set
 *  to point to the first_link field to indicate that none of the links has
 *  been walked looking for cycles yet.  Later in this function we will walk
 *  the input arrays and add edges to the graph as they are discovered.
 *
 *  Next (code point 18), comes the allocation of all the arrays of link_info
 *  objects and the building of the edge arrays.  The head_list is also built
 *  at this time.  The array_num variable is the index for the loop through the
 *  input arrays.  Note that we loop through them backwards, starting from the
 *  highest number and ending with the first array.  This is because edges are
 *  added to the front of the head and successor linked lists, so going through
 *  the arrays in reverse order means these linked lists will end up with the
 *  edges from the lower-numbered arrays before those of the higher numbered
 *  arrays.  This will make the elements of the earlier arrays tend to come
 *  before those of the later arrays all else being equal, which is desirable.
 *
 *  For each of the input arrays (code point 19) we first have the decrement of
 *  the index variable of the loop and then array_size and this_array are set
 *  to grab the elements of array_sizes and ``arrays'' respectively that
 *  correspond to this input array.  Assertions make sure that these have
 *  reasonable values.  Then malloc() is called to allocate the array of
 *  link_info objects for this one of the input arrays to be merged.
 *
 *  If the malloc() call fails (code point 20) an error is printed to standard
 *  error.  Then, all the memory that was allocated so far has to be cleaned
 *  up.  In general, at this point some but not all of the link_info arrays
 *  have been allocated, so only those that have already been allocated need to
 *  be freed.  This is done by first incrementing array_num (because the
 *  allocation that was supposed to be for the old value of array_num failed
 *  and shouldn't be cleaned up) and then deallocating the array for each
 *  element of link_arrays from there up to but not including num_arrays.
 *  Before each array is freed (code point 21) the code asserts that it is
 *  actually an allocated array of the expected size.  Then (code point 22) the
 *  top-level link_arrays array is freed as is the element_info array.  This
 *  completes the cleanup and then the location pointed to by
 *  link_arrays_location is set to NULL and NULL is returned to indicate the
 *  failure of this function.
 *
 *  If the malloc() succeeded (code point 24), the input array is walked and
 *  the array of link_info objects is used to add to all the appropriate linked
 *  lists.  Note that the first element in the input array needs to be added to
 *  the head list while every other element needs to be added to the successor
 *  list of the element that comes before it in the input array.  This is done
 *  by using a single loop but making sure that the source_list variable points
 *  to the head list on the first iteration and to the successor list of the
 *  previous element on every other iteration.  At the start of each iteration
 *  (code point 25), the number of the core element of this position of the
 *  input array is read into the target_element local variable.  Then an
 *  assertion checks that the element is in the proper range.
 *
 *  The next two lines implement a check that no single element appears more
 *  than once in the same input array.  The ``state'' field is used for this
 *  purpose since it's not being otherwise used in the
 *  create_internal_structures() function.  The state is set to
 *  ES_BEING_VISITED for each element as it is seen in the array.  Before it is
 *  set, though, an assertion checks that the value of the state was
 *  ES_UNVISITED.  If the same element appears in the input array more than
 *  once, this assertion will trigger the second time the element is seen in
 *  the list.  Later, after the input array has been walked all the way
 *  through (code point 29), the array will be walked again to set all the
 *  states back to ES_UNVISITED to make sure that if the element is seen in
 *  another list, the assertion isn't triggered.
 *
 *  The next line increments the incoming link count for this core element.
 *  Note that the incoming link count will therefor count both incoming links
 *  that are edges from other core elements and also incoming links from the
 *  head list.  Then the corresponding element of the link_info array has its
 *  fields set.  The target_element field is set to the present core element
 *  and the next_link field is set to whatever used to be the head of what
 *  source_list points to.  Then comes special code for handling the
 *  head_list_tail pointer.  If the head list was empty and this element is
 *  being added to the head list, the head_list_tail pointer must be updated to
 *  point to the next field of this link element.  This case is detected by
 *  checking to see if the source_list is equal to what the head_list_tail
 *  points to.  This can only happen if the source_list is the head list and if
 *  the tail of the head list is the start of the list.  When this is the case
 *  (code point 26), the head_list_tail is set to point to the next pointer of
 *  the link_info object being added to the list.  Since successive objects are
 *  added to the start of the list, once the head_list_tail has been set to the
 *  next pointer of an object, it doesn't need to be changed in this function.
 *
 *  After that, at code point 28, what's pointed to by the source_list pointer
 *  is updated to point to the new element and then the source_list pointer is
 *  set to the location of the successor link array for the present element, so
 *  that on the next iteration, the successor element will be added to the
 *  correct list.
 *
 *  Once every element of the input array has been handled (code point 29),
 *  it's time to set all the state fields back to ES_UNVISITED so we're ready
 *  for the next input array (and so that all the states will be ES_UNVISITED
 *  when this function is done).  Since link_num is already at the end of the
 *  input array, it's convenient to simply step it backwards through all the
 *  elements of the input array.  For each such element (code point 30) a
 *  couple of assertions sanity check the value of the element and the state of
 *  that core element then set the state back to ES_UNVISITED.  When that is
 *  done (code point 31) the appropriate element of link_arrays is set to the
 *  newly allocated and set up array of link_info objects.
 *
 *  When all the input arrays have been walked and the linked lists filled in
 *  (code point 32), all that's left is to assert that each core element's
 *  incoming link count is greater than zero (code point 33).  This is
 *  equivalent to checking that each core element appears in at least one of
 *  the input arrays, because the incoming link count is initially (before
 *  cycles are removed) equal to the number of times that core element appears
 *  in all of the input arrays to be merged.
 *
 *  When that's done (code point 34), all the data structures are set up, so
 *  they are returned to the caller.
 *
 *      remove_cycles()
 *
 *  As its name implies, the remove_cycles() function removes cycles from the
 *  graph until there are no cycles left.  For each cycle that is found, every
 *  edge in the cycle is removed, not just one of them.  It returns TRUE if
 *  this element was not found to be a part of a cycle that includes the edge
 *  going into it and FALSE if this element is part of a cycle that includes
 *  the edge going into it.
 *
 *  This function starts by making some assertions about its inputs.  Then it
 *  examines the start_element.  It sets the element_record local to point to
 *  the element_global_info record for this element.  Then it switches on the
 *  state of the element.
 *
 *  If the state is ES_UNVISITED (code point 36), then this element hasn't yet
 *  been visited in the walk, or has only been visited on walks during which it
 *  was found to be part of a cycle and for which the incoming edge was
 *  deleted.  In this case, the state is set to ES_BEING_VISITED.  Then, each
 *  of the link coming out of this element is walked until a cycle is found or
 *  all the links from this object have been walked.  Note that if a cycle is
 *  found, the link that is part of that cycle will be removed, so the
 *  iteration through the list out outgoing links uses a local variable of type
 *  ``link_info **'', not a variable of type ``link_info *''.  At each
 *  iteration it is set to point to the pointer in the list that points to the
 *  current link_info object, which is the pointer that needs to be updated to
 *  remove that element from the list.  That's why the link_pointer variable is
 *  set initially to the first_unsearched_link field of the core element.
 *  During each iteration, it is either set to the location of the next_link
 *  field of the element, unless the current element was just removed and we're
 *  continuing through the loop, in which case it doesn't have to be changed at
 *  all because the thing that's next after this has changed.
 *
 *  Inside the loop, the this_link local is set to point to the current element
 *  of the linked list.  The target_element local is used to store the number
 *  of the core element pointed to by the current element of the edge linked
 *  list.  Then remove_cycles() is called recursively on the target.  If the
 *  recursive call returns FALSE then a cycle has been found that includes this
 *  edge.  Further, when such a cycle is found, the ``state'' field for the
 *  element that was found to complete the cycle is set to ES_FINISHED.  This
 *  is to indicate how far back up the call chain edges must be removed.
 *
 *  When the code finds that the recursive call indicated this edge is part of
 *  a cycle (code pointe 38), it first stashes away the pointer to the next
 *  element in the linked list in the next_link variable.  Since the next field
 *  may change before the next_link is used, it is critical to store this
 *  before messing with the linked list.  Then the incoming link count for the
 *  target element is read.  Since this link points to that target, the
 *  incoming link count must be at least one, so the code here asserts that.
 *  But whether it is exactly one or not is important to what happens next.  If
 *  it is exactly one (code point 39), then when this link is removed, there
 *  will be no links coming into the target node.  So in this case the target
 *  node must be added to the head list.  Since we are traversing the head list
 *  already, it must be added to the end of the head list to make sure the walk
 *  actually reaches it again.  So the code at code point 39 sets the tail of
 *  the head list to point to the element being removed from the successor
 *  list, updates the pointer to the tail of the head list to point to the next
 *  pointer of this element, and sets that next field to NULL.  Note that in
 *  this case the incoming link count is unchanged because the link from
 *  another node is replaced by a link from the head list.  If the incoming
 *  link count was greater than one (code point 40), then all that needs to be
 *  done is to decrement the incoming link count.  The old link_info element
 *  doesn't have to be put in another list since it will be dropped and not be
 *  part of any list after this.  The code at code point 41 does the actual
 *  removing of the link_info element from the successor list by setting what's
 *  pointed to by the link_pointer to point to what next_link points to.
 *
 *  What happens next depends on the ``state'' field of the start element.
 *  Recall that before beginning the walk through the successors of the start
 *  element the state was set to ES_BEING_VISITED.  When a cycle is found, the
 *  state of the element that closes the cycle is changed to ES_FINISHED to
 *  indicate that this is where the loop is closed.  So the code here checks
 *  the state.  If it is ES_FINISHED (code point 42), then the link we just
 *  removed is the last one that needs to be removed to completely remove the
 *  discovered cycle.  So in this case the state is set back to
 *  ES_BEING_VISITED and the walk through the successor nodes can continue.  If
 *  the state wasn't set to ES_FINISHED (code point 43), then this isn't the
 *  last edge that needs to be removed.  So we set the first_unsearched_link
 *  field to store how far through the list of successors we got, so that if
 *  and when we revisit this node we don't have to go through paths we've
 *  already been through.  Then we set the state to ES_UNVISITED because now if
 *  we reach it again we shouldn't consider it part of a cycle because it's not
 *  on the stack of elements indicating our current path, and it's not finished
 *  being handled.  Then the function returns FALSE to indicate that the edge
 *  coming into this call needs to be removed also.
 *
 *  The next code is at code point 45, which is the ``else'' case for the test
 *  to see if the current link needs to be removed.  In this case, it does not
 *  need to be removed, so we simply update link_pointer to point to the next
 *  link in the list.
 *
 *  After that, the next code comes at code point 47, which follows the
 *  completion of the loop through the successors of this element.  If this
 *  point is reached, then we've made it through all the successors without
 *  finding a cycle that includes the edge coming into the start node for this
 *  call, so we set the state to ES_FINISHED because we know that we've walked
 *  everything reachable from this node so if we ever reach it again in this
 *  walk, we know that we don't have to look, we will find no cycles through
 *  this node.  Then we return TRUE to indicate that the edge that was followed
 *  to reach this node does not have to be removed.
 *
 *  The next case is the case that the state of the element when we enter this
 *  function is ES_BEING_VISITED.  In this case, we've just found the node that
 *  closes a cycle.  So we set the state of that node to ES_FINISHED so that
 *  when we're removing edges as we go back up the call chain we don't go past
 *  this element.  Then we return FALSE to indicate that the edge followed to
 *  reach this node is part of a cycle and needs to be removed.
 *
 *  The final possible case is that the state of the element when we enter this
 *  function is ES_FINISHED.  That means we've already walked everything
 *  reachable from this node and not found a cycle, so we can safely assume we
 *  don't have to walk it again.  So we simply return TRUE to indicate no
 *  cycles including the edge along which this node was reached.
 *
 *  Finally, there is a default case that simply asserts FALSE.  This should
 *  never be reachable unless the data structure is corrupt.  It has to return
 *  something at this point to make the compiler happy, so it returns FALSE (on
 *  the theory that TRUE means everything is fine, though it doesn't matter
 *  much what it returns -- either way, things are messed up and not likely to
 *  end up with the correct answer).
 *
 *      order_elements()
 *
 *  The order_elements() function is used to walk an acyclic graph and produce
 *  an ordering that is consistent with the graph.  Since the graph is acyclic,
 *  there must be such an ordering.
 *
 *  The first thing this function does (code point 50) is some assertions on
 *  its parameters.  Then, it tries to allocate the array for the final result.
 *  The final result always has to have num_elements elements.  If the malloc()
 *  attempt fails (code point 51), an appropriate error message is printed and
 *  NULL is returned to indicate failure.
 *
 *  Next, at code point 53, this code walks the graph and fills in the result
 *  array.  It walks through the head list and for each element in the head
 *  list, recursively walks starting from that element, using the
 *  walk_and_fill() function to do the walking (code point 54).  In this walk,
 *  the last time an element is reached, it is put in the output and all
 *  successor nodes are walked.  Successor nodes aren't walked until the last
 *  time it is reached.  This insures that before a core element goes into the
 *  final output array, all predecessors have been put there.  When the walk is
 *  done (code point 55), all that's left to do is assert that every element
 *  has been filled in in the output array and then return that output array.
 *
 *      walk_and_fill()
 *
 *  As mentioned above, the walk_and_fill() function does the final walk of the
 *  graph and puts the core elements in the result array.  Since it needs to
 *  handle each element only the last time it is reached and nothing else will
 *  use the graph after it's done, it simply decrements the count of incoming
 *  links and ignores the element until that incoming link count reaches zero.
 *
 *  The first thing this function does (code point 56) is some assertions about
 *  its parameters.  Then it sets the local element_record local to point to
 *  the element_global_info record for the core element in question.  For good
 *  measure, it checks to see that the state is set to ES_FINISHED -- there's
 *  no reason it wouldn't be and the state no longer matters, but this
 *  assertion might catch certain kinds of data structure corruption.  Then the
 *  incoming link count is read and decremented.  The original value is
 *  asserted to be at least one.  If it is more than that (code point 57), this
 *  function simply returns -- it will wait to deal with the code until the
 *  link count is one, indicating the last link into this core element has just
 *  been followed.
 *
 *  If we reach code point 59, then it is time to handle this element.  First,
 *  we assert that there is room left in the result array for it.  Then this
 *  core element is placed in the output array and the pointer to the next
 *  location in the output array is incremented.  After that, all the outgoing
 *  edges from this node are followed and this function is called recursively
 *  on the target of each such edge (code point 60).  When this is done, the
 *  function exits (code point 61).
 *
 *      delete_internal_structures()
 *
 *  The final function is the delete_internal_structures() function.  This
 *  function simply deletes the internal data structures that have been
 *  allocated.  It assumes that the complete set of internal data structures
 *  has been set up.
 *
 *  The first thing it does (code point 62) is make some assertions about the
 *  data structures it is about to deallocate.  Then it frees element_info
 *  (this is a simple array).  After that, it deallocates each of the arrays
 *  pointed to by the elements of the array pointed to by the link_arrays
 *  parameter (code point 63).  Finally, it frees the array pointed to by
 *  link_arrays itself (code point 64).
 *
 *  And that's the last of the code in this module.
 *
 *
 *      History
 *
 *  See the history discussion in merge_dense_integer_arrays.h for the history
 *  of this code.
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
 *  @@@
 *
 *          Chris Wilson, 2005, 2007, 2008
 */


extern verdict do_sprintf_pointer_conversion(void *plug_in_data,
        char *output_buffer, void *value_data, size_t *output_byte_count)
  {
    int int_byte_count;

    assert(plug_in_data == NULL);
    assert(output_buffer != NULL);
    assert(output_byte_count != NULL);

    int_byte_count = sprintf(output_buffer, "%p", value_data);
    if (int_byte_count < 0)
      {
        *output_byte_count = 0;
        return MISSION_FAILED;
      }
    else
      {
        *output_byte_count = int_byte_count;
        return MISSION_ACCOMPLISHED;
      }
  }

extern size_t sprintf_pointer_conversion_max_output_characters(void)
  {
    return ((sizeof(void *) * 8) + 25);
  }
