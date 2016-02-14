#include <stdlib.h>
#include <string.h> /* memset() */
#include "whgc.h"
#include "whhash.h"

/**
   Partial list of changes changes by Stephan Beal:

   - from unsigned int to unsigned long for hash keys.

   - created typedef whhash_val_t.

   - Added whhash_hash_cstring_{djb2,sdbm}() algorithms,
   taken from: http://www.cse.yorku.ca/~oz/hash.html

   - whhash_iter() now returns 0 if the whhash_table
   is empty, simplifying iteration error checking a bit.

   - The API is now const-safe, insofar as feasible. That is, funcs
   which can get away with (void const *) instead of (void*) now use
   const parameters.

   - Added ability to set a custom key/value dtors for each
   whhash_table.

   - Slightly changed semantics of some routines to accommodate the
   dtor handling.

   - Default dtor for keys is now null instead of free.
*/



#include <stdio.h> /* only for debuggering. */

#if defined(__cplusplus)
extern "C" {
#  include <cassert>
#  define ARG_UNUSED(X)
#else
#  include <assert.h>
#  define ARG_UNUSED(X) X
#endif /* __cplusplus */

#if 1
#define MARKER printf("**** MARKER: %s:%d:%s():\n",__FILE__,__LINE__,__func__);
#else
#define MARKER printf("**** MARKER: %s:%d:\n",__FILE__,__LINE__);
#endif

/**
   Define the constants for the whgc_events object...
*/
const whgc_events_t whgc_events = {
1, /* registered */
2, /* unregistered */
3, /* destructing_item */
4, /* destructing_context */
10 /* last_event_id */
};

/**
   Holder for generic gc data.
*/
struct whgc_gc_entry
{
    void * key;
    void * value;
    whgc_dtor_f keyDtor;
    whgc_dtor_f valueDtor;
    struct whgc_gc_entry * left;
    struct whgc_gc_entry * right;
};
typedef struct whgc_gc_entry whgc_gc_entry;
#define WHGC_GC_ENTRY_INIT {0,0,0,0,0,0}
static const whgc_gc_entry whgc_gc_entry_init = WHGC_GC_ENTRY_INIT;

#define WHGC_STATS_INIT {\
	0, /* entry_count */			\
	0, /* reg_count */		\
	0, /* unreg_count */		\
	0 /* alloced */		\
    }
static const whgc_stats whgc_stats_init = WHGC_STATS_INIT;

struct whgc_listener
{
    struct whgc_listener * next;
    whgc_listener_f func;
};
typedef struct whgc_listener whgc_listener;
#define WHGC_LISTENER_INIT {0,0}
static const whgc_listener whgc_listener_init = WHGC_LISTENER_INIT;

struct whgc_context
{
    void const * client;
    /**
       Hashtable of (void*) to (whgc_gc_entry*)
    */
    whhash_table * ht;
    /**
       Holds the right-most (most recently added) entry. A cleanup,
       the list is walked leftwards to free the entries in reverse
       order.
    */
    whgc_gc_entry * current;
    /**
       Event listeners.
    */
    whgc_listener * listeners;
    /**
       Informal stats.
    */
    whgc_stats stats;
};

#define WHGC_CONTEXT_INIT {\
	0,/*client*/			   \
	    0,/*ht*/			   \
	    0,/*current*/		   \
	    0,/*listeners*/		   \
	WHGC_STATS_INIT}
static const whgc_context whgc_context_init = WHGC_CONTEXT_INIT;

/**
   A destructor for use with the hashtable API. Calls
   whhash_destroy((whhash_table*)k).
static void whgc_free_hashtable( void * k )
{
    //MARKER; printf("Freeing HASHTABLE @%p\n",k);
    whhash_destroy( (whhash_table*)k );
}
*/

/**
   A logging version of free().
 */
static void whgc_free( void * k )
{
    //MARKER; printf("Freeing GENERIC (void*) @%p\n",k);
    free(k);
}

void * whgc_alloc( whgc_context * cx, size_t size, whgc_dtor_f dtor )
{
    void * ret = size ? malloc( size ) : 0;
    if( ! ret ) return 0;
    memset( ret, 0, size );
    if( !cx ) return ret;
    cx->stats.alloced += size;
    whgc_add( cx, ret, dtor );
    return ret;
}

void const * whgc_get_context_client(whgc_context const *cx)
{
    return cx ? cx->client : 0;
}
/**
   Calls all registered listeners with the given
   parameters.
*/
static void whgc_fire_event( whgc_context const *cx,
			     short ev,
			     void const * key,
			     void const * val )
{
    //MARKER;printf("Firing event %d for cx @%p\n",ev,cx);
    whgc_listener * l = cx ? cx->listeners : 0;
    if( l )
    {
	whgc_event E;
	E.cx = cx;
	E.type = ev;
	E.key = key;
	E.value = val;
	while( l )
	{
	    if( l->func )
	    {
		//MARKER;printf("Firing @%p(cx=@%p,event=%d,key=@%p,val=@%p)\n",l->func,cx,ev,key,val);
		l->func( &E );
	    }
	    l = l->next;
	}
    }
}

/**
   Destructor for use with the hashtable API. Frees
   whgc_gc_entry objects by calling the assigned
   dtors and then calling free(e).

   The caller is expected to relink e's neighbors himself if needed
   before calling this function.

   The cx parameter is only used for firing a
   whgc_event_destructing_item event.
*/
static void whgc_free_gc_entry( whgc_context * cx,
				whgc_gc_entry * e )
{
    if( ! e ) return;
    if( cx ) whgc_fire_event( cx, whgc_events.destructing_item, e->key, e->value );
    //MARKER;printf("Freeing GC item e[@%p]: key=[%p(@%p)] val[%p(@%p)]]\n",e,e->keyDtor,e->key,e->valueDtor,e->value);
    if( e->valueDtor )
    {
	//MARKER;printf("dtor'ing GC value %p( @%p )\n",e->valueDtor, e->value);
	e->valueDtor(e->value);
    }
    if( e->keyDtor )
    {
	//MARKER;printf("dtor'ing GC key %p( @%p )\n",e->keyDtor, e->key);
	e->keyDtor(e->key);
    }
    if( cx ) cx->stats.alloced -= sizeof(whgc_gc_entry);
    whgc_free(e);
}

/**
   A no-op "destructor" to assist in tracking down destructions.
*/
static void whgc_free_noop( void * ARG_UNUSED(v) )
{
    //MARKER;printf("dtor no-op @%p\n",v);
}

static whhash_table * whgc_hashtable( whgc_context * cx )
{
    if( ! cx ) return 0;
    if( cx->ht ) return cx->ht;
    if( (cx->ht = whhash_create( 10, whhash_hash_void_ptr, whhash_cmp_void_ptr ) ) )
    {
	whhash_set_dtors( cx->ht, whgc_free_noop, whgc_free_noop );
	/**
	   We use no-op dtors so we can log the destruction process, but cx->ht
	   does not own anything. Because we need predictable destruction order,
	   we manage a list of entries and destroy them in reverse order.
	*/
	/* Reminder: we don't update cx->stats.alloced here. We accommodate the
	   Hashtable size in whgc_get_stats().
	*/
    }
    return cx->ht;
}

whgc_context * whgc_create_context( void const * clientContext )
{
    whgc_context * cx = (whgc_context *)malloc(sizeof(whgc_context));
    if( ! cx ) return 0;
    *cx = whgc_context_init;
    cx->stats.alloced += sizeof(whgc_context);
    cx->client = clientContext;
    return cx;
}

bool whgc_add_listener( whgc_context *cx, whgc_listener_f f )
{
    if( ! cx || !f ) return false;
    //MARKER;printf("Adding listener @%p() to cx @%p\n",f,cx);
    whgc_listener * l = (whgc_listener *)
	whgc_alloc( cx, sizeof(whgc_listener), whgc_free_noop );
	//malloc(sizeof(whgc_listener));
    if( ! l ) return 0;
    *l = whgc_listener_init;
    l->func = f;
    //whgc_add( cx, l, whgc_free_noop );
    whgc_listener * L = cx->listeners;
    if( L )
    {
	while( L->next ) L = L->next;
	L->next = l;
    }
    else
    {
	cx->listeners = l;
    }
    return true;
}

bool whgc_register( whgc_context * cx,
		    void * key, whgc_dtor_f keyDtor,
		    void * value, whgc_dtor_f valDtor )
{
    if( !key || !whgc_hashtable(cx) || (0 != whhash_search(cx->ht, key)) )
    {
	return false;
    }
    whgc_gc_entry * e = (whgc_gc_entry*)whgc_alloc( 0, sizeof(whgc_gc_entry), 0 );
    if( ! e ) return false;
    cx->stats.alloced += sizeof(whgc_gc_entry);
    *e = whgc_gc_entry_init;
    e->key = key;
    e->keyDtor = keyDtor;
    e->value = value;
    e->valueDtor = valDtor;
    //MARKER;printf("Registering GC item e[@%p]: key[@%p]/%p() = val[@%p]/%p()\n",e,e->key,e->keyDtor,e->value,e->valueDtor);
    whhash_insert( cx->ht, key, e );
    ++(cx->stats.reg_count);
    cx->stats.entry_count = whhash_count(cx->ht);
    if( cx->current )
    {
	e->left = cx->current;
	cx->current->right = e;
    }
    cx->current = e;
    whgc_fire_event( cx, whgc_events.registered, e->key, e->value );
    return true;
}

bool whgc_add( whgc_context * cx, void * key, whgc_dtor_f keyDtor )
{
    return whgc_register( cx, key, keyDtor, key, 0 );
}

void * whgc_unregister( whgc_context * cx, void * key )
{
    if( ! cx || !cx->ht || !key ) return 0;
    whgc_gc_entry * e = (whgc_gc_entry*)whhash_take( cx->ht, key );
    void * ret = e ? e->value : 0;
    if( e )
    {
	cx->stats.entry_count = whhash_count(cx->ht);
	++(cx->stats.unreg_count);
	if( e->left ) e->left->right = e->right;
	if( e->right ) e->right->left = e->left;
	if( cx->current == e ) cx->current = (e->right ? e->right : e->left);
	/**
	   ^^^ this is pedantic. In theory cx->current must always be
	   the right-most entry, so we could do: cx->current=e->left;
	 */
	void * k = e->key;
	void * v = e->value;
	whgc_free(e);
	cx->stats.alloced -= sizeof(whgc_gc_entry);
	whgc_fire_event( cx, whgc_events.unregistered, k, v );
    }
    return ret;
}

void * whgc_search( whgc_context const * cx, void const * key )
{
    if( ! cx || !key || !cx->ht ) return 0;
    whgc_gc_entry * e = (whgc_gc_entry*)whhash_search( cx->ht, key );
    return e ? e->value : 0;
}

void whgc_clear_context( whgc_context * cx )
{
    if( ! cx ) return;
    //MARKER;printf("Cleaning up %u GC entries...\n",cx->stats.entry_count);
    if( cx->ht )
    {
	//MARKER;printf("Cleaning up %u GC entries...\n",whhash_count(cx->ht));
	whhash_clear( cx->ht );
    }
    /**
       Destroy registered entries in reverse order of
       their registration.
    */
    whgc_gc_entry * e = cx->current;
    while( e )
    {
	whgc_fire_event( cx, whgc_events.unregistered, e->key, e->value ); /* a bit of a kludge, really. */
	//MARKER;printf("Want to clean up @%p\n",e);
	whgc_gc_entry * left = e->left;
	whgc_free_gc_entry(cx,e);
	e = left;
    }
    cx->stats.entry_count = 0;
    cx->current = 0;
}
void whgc_destroy_context( whgc_context * cx )
{
    if( ! cx ) return;
#if 0
    {
	whhash_stats s = whhash_get_stats( cx->ht );
	MARKER;printf("GC context search collisions: %u\n",s.search_collisions);
    }
#endif
    whgc_fire_event( cx, whgc_events.destructing_context, 0, 0 );
    whgc_clear_context( cx );
    if( cx->ht )
    {
	whhash_destroy( cx->ht );
	cx->ht = 0;
    }

    cx->client = 0; /* needs to stay valid until all events are fires, in case clients use it as a lookup key (i do) */
    whgc_listener * L = cx->listeners;
    cx->listeners = 0;
    while( L )
    {
	whgc_listener * l = L->next;
	cx->stats.alloced -= sizeof(whgc_listener);
	whgc_free(L);
	L = l;
    }
    whgc_free(cx);
}

void whgc_context_dtor( void * p )
{
    whgc_destroy_context( (whgc_context*)p );
}

whgc_stats whgc_get_stats( whgc_context const * cx )
{
    whgc_stats s = cx ? cx->stats : whgc_stats_init;
    if( ! cx ) return s;
    whhash_stats const hs = whhash_get_stats( cx->ht );
    s.alloced += hs.alloced;
    return s;
}

#if defined(__cplusplus)
} /* extern "C" */
#endif
