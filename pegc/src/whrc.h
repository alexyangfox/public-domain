#ifndef WANDERINGHORSE_NET_WHRC_H_INCLUDED
#define WANDERINGHORSE_NET_WHRC_H_INCLUDED 1
/** @page whrc_page_main whrc: reference counting library for C

whrc (the WanderingHorse.net Reference Counter) is a C library for
adding reference counts to arbitrary void pointers.  It allows one to
register a destructor function which will be called when the reference
count goes to 0. It is similar to the whgc garbage collector
(see: @ref whgc_page_main)
but is lighter-weight and does not manage key/value
pairs as that library does.

It internally uses a hashtable which uses a hash code based on the
(void*) address of an item. In theory the hashtable can average 0(1)
speeds, so there is very little performance hit.

Example:
@code
whrc_context * cx = whrc_create_context();
char * str = (char *)malloc(...);
whrc_register( cx, str, free );
size_t rc = whrc_refcount(cx,str); // rc == 1
rc = whrc_addref(cx,str); //rc == 2
rc = whrc_rmref(cx,str); // rc == 1
rc = whrc_rmref(cx,str); // rc == 0, free(str) is called
whrc_destroy_context(cx,true);
@endcode

(The bool argument to whrc_destroy_context() says whether or not to
free up any "dangling" items (those with references) in the context.)

That demonstrates the majority of the API. There are only a handful of
functions to learn.

@section whrc_sec_threadsafety Thread safety

By default it is never legal to use the same @ref whrc_context from
    multiple threads at the same time. That said, the client may use
    their own locking to serialize access. All API functions which
    take a (@ref whrc_context const *) argument require only a read lock,
    whereas those taking a (@ref whrc_context *) argument require a
    exclusive (read/write) access. whrc_create_context() is reentrant
    and does not need to be locked.

    Special consideration is also needed considering locking of stored
    items. If a referenced item is used by multiple threads, this code
    has no way of knowing about it. When in doubt, don't store items
    which are shared across threads unless you know that lifetime and
    ownership issues can be mitigated.
*/

#include <stddef.h> /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#  ifndef WHGC_HAVE_STDBOOL
#    define WHGC_HAVE_STDBOOL 0
#  endif
#  if defined(WHRC_HAVE_STDBOOL) && !(WHRC_HAVE_STDBOOL)
#    if !defined(bool)
#      define bool char
#    endif
#    if !defined(true)
#      define true 1
#    endif
#    if !defined(false)
#      define false 0
#    endif
#  else /* aha! stdbool.h! C99. */
#    include <stdbool.h>
#  endif /* WHRC_HAVE_STDBOOL */
#endif /* __cplusplus */

/** @typedef void (*whrc_dtor_f)(void*)

    A destructor function semantically compatible with free().
*/
typedef void (*whrc_dtor_f)(void*);

/**
   Defined to ((size_t)-1), and returned from some functions
   which need to report an error.
*/
extern const size_t whrc_ref_err_val;

/**@typedef struct whrc_context whrc_context

   Opaque handle type for reference count operations.
*/
struct whrc_context;
typedef struct whrc_context whrc_context;

/**
   Creates a new whrc_context object and transfers ownership to the
   caller. The caller must free it with whrc_destroy_context().
*/
whrc_context * whrc_create_context();

/**
   Clears all items in a context, unconditionally passing them to
   their registered destructors.
*/
void whrc_clear_context( whrc_context * );

/**
   Frees all resources allocated by the context. If freeItems is true
   then whrc_clear_context() is called, otherwise any remaining items
   in the cache are left untouched (and will leak unless ownership of
   the items has been explicitly transfered).
*/
void whrc_destroy_context( whrc_context * cx, bool freeItems );

/**
   Registers item with cx and sets its reference count to 1.
   It is not legal to register the same item more than once.

   Returns true if the item is now registered (and therefore
   owned by this object) or false if !cx, !item, or if an entry
   is already found for the given key.

   Note that it is legal to set a 0 dtor, in which case this
   object does not own the item but will still maintain a reference
   count. In any case, item must outlive the context or be destroyed
   by the context.
*/
bool whrc_register( whrc_context * cx, void * item, whrc_dtor_f );

/**
   Returns true only if item is registered in the given context.
*/
bool whrc_is_registered( whrc_context * cx, void const * item );


/**
   Adds one to the reference count of item and returns the new
   reference count, or whrc_ref_err_val if (!cx), (!item), or
   if no entry is found.
*/
size_t whrc_addref( whrc_context * cx, void * item );

/**
   Subtracts one to the reference count of item and returns the new
   reference count, or whrc_ref_err_val if (!cx) or (!item), or
   if no entry is found. If 0 is returned then the dtor registered
   via whrc_register() is called and passed item.
*/
size_t whrc_rmref( whrc_context * cx, void * item );

/**
   Returns the current refcount for the given item, or whrc_ref_err_val
   if no such item is found or if cx or item are 0.
*/
size_t whrc_refcount( whrc_context * cx, void const * item );


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* WANDERINGHORSE_NET_WHRC_H_INCLUDED */
