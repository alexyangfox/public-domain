#include <stdlib.h> /* malloc/free */
#include <assert.h>

#include "whrc.h"
#include "whhash.h"
#ifdef __cplusplus
extern "C" {
#endif

#if 1
#include <stdio.h>
#define MARKER printf("******** MARKER: %s:%d:%s():\n",__FILE__,__LINE__,__func__);
#else
#define MARKER
#endif

const size_t whrc_ref_err_val = ((size_t)-1);
struct whrc_context
{
    whhash_table * ht;
};

static const whrc_context whrc_context_init =
{
0 /* ht */
};

struct whrc_entry
{
    whrc_dtor_f dtor;
    size_t refcount;
};
typedef struct whrc_entry whrc_entry;
static const whrc_entry whrc_entry_init = {0,0};

whrc_context * whrc_create_context()
{
    whrc_context * c = (whrc_context *)malloc(sizeof(whrc_context));
    if( ! c ) return 0;
    *c = whrc_context_init;
    if( (c->ht = whhash_create( 10, whhash_hash_void_ptr, whhash_cmp_void_ptr ) ) )
    {
	whhash_set_dtors( c->ht, 0, free );
    }
    if( ! c->ht )
    {
	free(c);
	return 0;
    }
    return c;
}

void whrc_clear_context( whrc_context * cx )
{
    whhash_iter * it = cx ? whhash_get_iter(cx->ht) : 0;
    //MARKER;
    if( ! it ) return;
    //MARKER;
    do
    {
	//MARKER;
	whrc_entry * e = (whrc_entry *)whhash_iter_value(it);
	if( ! e ) continue; /* shouldn't happen! */
	void * item = whhash_iter_key(it);
	if( item && e->dtor )
	{
	    //MARKER;
	    e->dtor( item );
	}
    }
    while( whhash_iter_advance(it) );
    free(it);
    whhash_clear(cx->ht);
}

void whrc_destroy_context( whrc_context * cx, bool freeItems )
{
    if( ! cx ) return;
    if( freeItems ) whrc_clear_context(cx);
    whhash_destroy( cx->ht );
    free(cx);
}

bool whrc_register( whrc_context * cx, void * item, whrc_dtor_f dtor )
{
    if( ! cx || !item ) return false;
    if( whrc_is_registered(cx,item) ) return false;
    whrc_entry * e = (whrc_entry*)malloc(sizeof(whrc_entry));
    if( ! e ) return false;
    *e = whrc_entry_init;
    e->refcount = 1;
    e->dtor = dtor;
    if( ! whhash_insert( cx->ht, item, e ) )
    {
	free(e);
	e = 0;
    }
    return 0 != e;
}

static whrc_entry * whrc_search_entry( whrc_context * cx, void const * item )
{
    return (cx && item)
	? (whrc_entry *) whhash_search(cx->ht, item)
	: 0;
}

#if 0
/**
  This searches the context for the given item and returns it (or 0 if
  no match is found (i.e. the item is not registered)).

  Why use an item to look up itself? The first reason is, this routine
  can be used in place to whrc_is_registered() (though that one is
  slightly more efficient).

  Secondly it can sometimes be used to legally get a non-const pointer
  to an object which is otherwise const (that is, without casting away
  the constness). For example, one routine may register the item, then
  downstream the item is passed as a const pointer to another
  routine. That routine can (assuming it has the whrc_context handle)
  then use whrc_search() to get a non-const pointer to the item.
  Admittedly only rarely useful, but this approach has come in handy a
  time or two.
*/
//void * whrc_search( whrc_context * cx, void const * item );
static void * whrc_search( whrc_context * cx, void const * item )
{
    whrc_entry * e = whrc_search_entry(cx,item);
    return e
	? e->item
	: 0;
}
#endif


size_t whrc_refcount( whrc_context * cx, void const * item )
{
    whrc_entry const * e = whrc_search_entry(cx, item);
    if( ! e ) return whrc_ref_err_val;
    return e->refcount;
}


bool whrc_is_registered( whrc_context * cx, void const * item )
{
    return 0 != whrc_search_entry(cx,item);
}

static whrc_entry * whrc_take_entry( whrc_context * cx, void * item )
{
    return cx
	? (whrc_entry *) whhash_take(cx->ht, item)
	: 0;
}

size_t whrc_addref( whrc_context * cx, void * item )
{
    whrc_entry * e = whrc_search_entry(cx,item);
    if( ! e ) return whrc_ref_err_val;
    return ++e->refcount;
}

size_t whrc_rmref( whrc_context * cx, void * item )
{
    whrc_entry * e = whrc_search_entry(cx,item);
    MARKER; printf("e=@%p, rc=%u\n",e,e?e->refcount:whrc_ref_err_val);
    if( ! e ) return whrc_ref_err_val;
    if( 0 == --e->refcount )
    {
        whrc_entry * check = whrc_take_entry(cx, item);
	assert( (check == e) && "Internal state error - it seems another thread has trashed our hashtable!");
	/**
	   We do this copy/free thing for the case that the dtor
	   throws a C++ exception or otherwise doesn't return (e.g. it
	   jumps). i know it's not likely, but it's easy enough to
	   avoid the leak here, so i do...
	 */
	whrc_entry E = *check;
	free(check);
	if( E.dtor )
	{
	    E.dtor( item );
	}
	return 0;
    }
    return e->refcount;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
