#ifndef WANDERINGHORSE_NET_WHGC_H_INCLUDED
#define WANDERINGHORSE_NET_WHGC_H_INCLUDED

#include <stdarg.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#  ifndef WHGC_HAVE_STDBOOL
#    define WHGC_HAVE_STDBOOL 0
#  endif

#  if defined(WHGC_HAVE_STDBOOL) && !(WHGC_HAVE_STDBOOL)
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
#  endif /* WHGC_HAVE_STDBOOL */
#endif /* __cplusplus */

    /*!
      @page whgc_page_main whgc: Garbage Collection lib for C

      @section whgc_sec_about_whgc About whgc

      whgc (the WanderingHorse.net Garbage Collector) is a simple garbage
      collector (GC) for C.

       Author: Stephan Beal (http://wanderinghorse.net/home/stephan)

       License: the core library is Public Domain, but some of the
       borrowed utility code is released under a BSD license (see
       whhash.{c,h} for details).

       Home page: whgc is part of the pegc project:
       http://fossil.wanderinghorse.net/repos/pegc/

       whgc is a small garbage collection library for C. The library
       provides context objects to which one can attach arbitrary
       data (via void pointers) and arbitrary destructor functions.
       When a GC context is destroyed, the destructors are called
       for each attached item.

       The original use case for whgc was a parser toolkit which
       needed to allocate dynamic resources while generating a
       grammar. By attaching the dynamic data to the parser's GC, we
       eliminated a number of headaches involved with ownership of the
       dynamic data. It also simplified many lookup operations when we
       needed to find shared data.

       @section set_features Features and Misfeatures

       The main feature of whgc are:

       - It allows one to bind key/value pairs to a context
       object. When that object is destroyed, client-defined
       destructor functions will be called on for all of the keys and
       values. This greatly simplifies management and ownership of
       dynamically allocated memory for some use cases.

       - It supports event listeners, to assist in debugging the
       lifetimes of GC'd objects.

       - Aside from GC, it's sometimes useful as a general-purposes lookup
       table (a hashtable of (key=void *,value=void *)).

       @section whgc_sec_example Example

       An exceedingly simple example of using it:

       @code
       whgc_context * cx = whgc_create_context( 0 );
       if( ! cx ) { ... error, probably OOM ... }
       
       int * i = (int*)malloc(sizeof(int));
       *i = 42;
       whgc_add( i, free );

       struct mystruct * my = (struct mystruct*)malloc(sizeof(struct mystruct));
       my->foo = "hi";
       my->bar = 42.42;
       // Assume this function exists:
       //   void mystruct_dtor(void*);
       // and that it deallocates mystruct objects.
       whgc_add( cx, my, mystruct_dtor );

       ...

       whgc_destroy_context( cx );
       // now all items added to the context are destroyed using the
       // given destructor callbacks. They are destroyed in the reverse
       // order they were added (LIFO).
       @endcode

       Aside from whgc_add(), there is the more flexible
       whgc_register(), which allows one to register key/value pairs,
       and separate destructors for each key and value. The lookup key
       can be used with whgc_search() to find the mapped data.

       Assigning a lookup key to data is often useful when we have
       some common handle type we're passing around but need to
       associated dynamically-allocated objects to them. The GC approach
       allows us to transfer ownership to the GC context, so that we can
       map arbitrary data to an arbitrary object and not have to worry
       about whether or not that memory will be deallocated later.

       @section whgc_sec_threadsafety Thread safety

       By default it is never legal to use the same context from
       multiple threads at the same time. That said, the client may
       use their own locking to serialize access. All API functions
       which take a (whgc_context const *) argument require only a
       read lock, whereas those taking a (whgc_context*) argument
       require a exclusive (read/write) access. whgc_create_context()
       is reentrant and does not need to be locked.
    */

    /** @struct whgc_context

       whgc_context is an opaque handle for use with the whgc_xxx()
       routines. They are created using whgc_create_context() and
       destroyed using whgc_destroy_context().
    */
    struct whgc_context;
    typedef struct whgc_context whgc_context;

    /** @typedef void (*whgc_dtor_f)(void*)

       Typedef for deallocation functions symantically compatible with
       free().
    */
    typedef void (*whgc_dtor_f)(void*);
    
    /**
       If cx is null this function works just like malloc() except:

       - It calls memset() to zero out the memory.

       - If cx is not null then ownership of the memory transfers is
       transfered to cx calling whgc_add(cx,theMemory,dtor). cx's memory
       memory allocation telemetry (see whgc_get_stats()) is also updated.

       - If the cx is null then the caller owns the returned memory and must free
       it by passing it to free().

       It returns 0 on an alloc error or if size is 0.
    */
    void * whgc_alloc( whgc_context * cx, size_t size, whgc_dtor_f dtor );

    /**
       Creates a gc context. The clientContext point may internally be
       used as a lookup key or some such but is otherwise unused by
       this API.
    */
    whgc_context * whgc_create_context( void const * clientContext );

    /**
       Returns the client-supplied pointer which was passed to
       whgc_create_context() for the given context. It is sometimes
       useful as a lookup key.
    */
    void const * whgc_get_context_client(whgc_context const *);


    /**
       Registers an arbitrary key and value with the given garbage
       collector, such that whgc_destroy_context(st) will clean up the
       resources using the given destructor functions (which may be 0,
       meaning "do not destroy").

       A custom hash function is supplied which hashes the address of
       the key (that is, a hash value of the numeric value of the
       pointer address). Thus for two keys to be equal they must have
       the same address (or must have found a collision in the hash
       algorithm).

       If keyDtor is not 0 then during cleanup keyDtor(key) is
       called. Likewise, if valDtor is not 0 then valDtor(value) is
       called at cleanup time.

       It is perfectly legal for the key and value to be the same
       object, but only if at least one of the destructor functions is
       0 or a no-op function (otherwise a double-free will happen).

       It is legal for both keyDtor and valDtor to be 0, in which case
       this API simply holds a reference to the data but will not destroy
       it.

       Returns true if the item is now registered, or false on error
       (!st, !key, key was already mapped, or a memory allocation error).

       It is illegal to register the same key more than once with the
       same context. Doing so will cause false to be returned.

       Note that the destruction order of items cleaned up using this
       mechanism is in reverse order of their registration.
    */
    bool whgc_register( whgc_context * cx,
			void * key, whgc_dtor_f keyDtor,
			void * value, whgc_dtor_f valDtor );

    /**
       Convenience form of whgc_register(cx,key,keyDtor,key,0).
    */
    bool whgc_add( whgc_context *cx, void * key, whgc_dtor_f keyDtor );

    /**
       Removes the given key from the given context, transfering ownership
       of the key and the associated value to the caller.
    */
    void * whgc_unregister( whgc_context * cx, void * key );

    /**
       Frees all resources associated with the given context.
       All entries which have been added via whgc_add() are passed to
       the dtor function which was assigned to them via whgc_add().

       Note that the destruction order is in reverse order of the
       registration (FIFO).
    */
    void whgc_destroy_context( whgc_context * );

    /**
       Clears all gc entries, calling their associated destructors.

       This does not destroy cx, only its entries.
    */
    void whgc_clear_context( whgc_context * cx );

    /**
       A destructor for use with functions taking a whgc_dtor_f parameter.
       It requires that its argument be a whgc_context pointer, on which
       it calls whgc_destroy_context().

       This can be useful if you want one gc context to manage multiple
       other contexts. Simply use, e.g.:

       @code
       whgc_gc_add( mainCtx, subCtx, whgc_context_dtor );
       @endcode

       where mainCtx is the primary context and subCtx is a context which
       belongs to mainCtx.
    */
    void whgc_context_dtor( void * );

    /**
       Searches the given context for the given key. Returns 0 if the
       key is not found. Ownership of the returned objet is not changed.
    */
    void * whgc_search( whgc_context const * cx, void const * key );

    /**
       A type for storing some telemetry for a whgc_context.
       Use whgc_get_stats() to collect the current stats of
       a parser.
    */
    struct whgc_stats
    {
	/**
	   Number of entries in the context.
	*/
	size_t entry_count;
	/**
	   Number of registrations made in the context.
	*/
	size_t reg_count;
	/**
	   Number of items unregistered from the context.
	*/
	size_t unreg_count;
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
    };
    typedef struct whgc_stats whgc_stats;

    /**
       Returns the current stats for the given context.
    */
    whgc_stats whgc_get_stats( whgc_context const * );

    /**
       The type is not intended to be instantiated directly by
       clients, but instead used via the whgc_events shared object.

       A helper type for simulating scoped named enums in C. The
       values in this type correspond to those used by whgc_event,
       which allows event listeners to distinguish between various
       event types in one handler.

       All members of this type must have unique values, but the
       values used by the whgc_events object are unspecified (other
       than that they are each unique).
    */
    struct whgc_events_t {
	/**
	   Signal that a GC item has been registered.
	*/
	short registered;

	/**
	   Signal that a GC item has been unregistered.
	*/
	short unregistered;

	/**
	   Signal that a key/val pair is about to e passed through the
	   item's dtor functions. This event is triggered even if the dtor
	   functions won't actually be called (e.g. they are 0). Thus this
	   signals a "virtual" destruction.
	*/
	short destructing_item;

	/**
	   Signal that a context is about to be destroyed.
	*/
	short destructing_context;

	/**
	   This marks the highest reserved event ID. In theory, values
	   above that can be used by client code, but i can personally
	   see no use for doing so. Note that the IDs in this class
	   are not guaranteed to be sequential, so don't use this as
	   an end condition for a loop.
	*/
	short last_event_id;
    };
    typedef struct whgc_events_t whgc_events_t;
    /** @var const whgc_events_t whgc_events

       This is a shared instance of whgc_events_t which holds event
       type IDs. It is used instead of an enum because i find it
       prettier. Sample usage, from a whgc_listener_f implementation:

       @code
       void my_gc_listener( whgc_event const * event )
       {
           if( whgc_events.destructing_context == event->type )
           {
               whgc_stats const st = whgc_get_stats( event->cx );
	       printf("Approx memory allocated by gc context: %u\n", st.alloced);
	   }
       }
       @endcode
    */
    extern const whgc_events_t whgc_events;

    /**
       whgc_event is a type for sending information regarding certain GC
       context state changes to registered listeners.
    */
    struct whgc_event
    {
	/**
	   The context from which this event originated. This parameter
	   is set for all event types.
	 */
	whgc_context const * cx;

	/**
	   Specifies the logical type of the event, which also determines
	   the semantics of the event.

	   The type IDs are defined in the whgc_events shared object
	   and alert the user about which members of the event are set
	   and. e.g. whgc_events.destructing_context does not set the
	   key/value members because they are meaningless for that
	   event. The semantics of the various event types are:

	   - whgc_events.registered signals that the key/value members have
	   just been registered with the context.

	   - whgc_events.unregistered signals that the key/value members
	   have just been unregistered from the context.

	   - whgc_events.destructing_item signals that the key/value
	   members are about to be passed to their registered dtor
	   functions (if any).

	   - whgc_events.destructing_context signals that the cx
	   member is about to be destroyed (key/value will be 0).

	   These semantics should be respected by whgc_listener_f
	   implementations.
	 */
	short type;

	/**
	   The key associated with the event. Only set for events of type:

	   whgc_events.registered, whgc_events.unregistered, whgc_events.destructing_item

	 */
	void const * key;
	/**
	   The value associated with the event. Only set for events of type:

	   whgc_events.registered, whgc_events.unregistered, whgc_events.destructing_item

	 */
	void const * value;
    };
    typedef struct whgc_event whgc_event;
    /** @typedef void (*whgc_listener_f)( whgc_event const * ev )

       A typedef for whgc context event listers. The primary use case
       of a listener is to help debug the lifetimes of GC'd items. The
       listener is guaranteed to not get a null pointer, so long as it
       is triggered from inside of this API.
    */
    typedef void (*whgc_listener_f)( whgc_event const * ev );

    /**
       Adds an event listener to the context. The listener is called
       when certain events happen within the given context. The call
       order of multiple listeners is undefined. There is no way to
       remove a listener.

       Listeners are best reserved for debugging purposes only, as
       they apply some overhead to all add/remove operations. The
       exact amount of overhead is largely a function of what the
       listeners do, as the context is effectively blocked until the
       listeners return.
    */
    bool whgc_add_listener( whgc_context *, whgc_listener_f f );

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* WANDERINGHORSE_NET_WHGC_H_INCLUDED */
