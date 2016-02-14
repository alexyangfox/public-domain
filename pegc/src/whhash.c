/* Copyright (C) 2002, 2004 Christopher Clark <firstname.lastname@cl.cam.ac.uk> */
/* Copyright (C) 2008, 2009 Stephan Beal (http://wanderinghorse.net/home/stephan/) */
 /*
   On 17 June 2009, i (Stephan Beal) got permission from the original author
   (Christopher Clark) to dual-license this code under the following terms:

   - If the code is used in a jurisdiction where Public Domain
   property is regonized, then this code may be considered to be
   in the Public Domain. Its author expressly disclaims copyright
   in jurisdictions where such a disclaimer is allowed.

   - If the code is used in a jurisdiction which does not recognize
   Public Domain, the code must be used in terms with the MIT license,
   as described clearly and concisely at:

   http://en.wikipedia.org/wiki/MIT_License

   and reproduced in full below.

   - If the code is used in a jurisdiction which recognizes Public
   Domain, the user may use the code without limits, as for Public
   Domain property, or may instead opt to use the code under the terms
   of the MIT license.

   The MIT licensing terms follow:
   ========================================================================
   Copyright (C) 2002, 2004 Christopher Clark <firstname.lastname@cl.cam.ac.uk>
   Copyright (c) 2008, 2009 Stephan Beal (http://wanderinghorse.net/home/stephan/)

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
   ========================================================================
*/
#include "whhash.h"
#include <stdlib.h>
//#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
  Internal type for holding hashtable entries.
*/
struct whhash_entry
{
    void *k, *v;
    whhash_val_t h;
    struct whhash_entry *next;
};
typedef struct whhash_entry whhash_entry;

#define WHHASH_STATS_INIT {\
	0,/*entries*/ \
	0,/*insertions*/ \
	0,/*removals*/ \
	0,/*searches*/ \
        0,/*alloced*/ \
        0,/*expansions*/ \
        0,/*search_collisions*/ \
    }
static const whhash_stats whhash_stats_init = WHHASH_STATS_INIT;


struct whhash_table {
    size_t tablelength;
    struct whhash_entry **table;
    size_t entrycount;
    size_t loadlimit;
    size_t primeindex;
    whhash_val_t (*hashfn) (void const *k);
    int (*eqfn) (void const *k1, void const *k2);
    void (*freeKey)( void * );
    void (*freeVal)( void * );
    unsigned int flags;
    whhash_stats stats;
};
static const whhash_table
whhash_init = { 0, /*tablelength*/
		   0, /* table */
		   0, /* entrycount */
		   0, /* loadlimit */
		   0, /* primeindex */
		   0, /* hashfn */
		   0, /* eqfn */
		   0, /* freeKey */
		   0, /* freeVal */
		0, /* flags */
		   WHHASH_STATS_INIT
};
const whhash_val_t whhash_hash_val_err = (whhash_val_t)-1;

struct whhash_flags {
    /**
       Signals a hash for use with the whhash_st_xxx() functions.
       Set only by whhash_sh_create().
     */
    const int SH_API;
} whhash_flags = {
    0x0001 /* SH_API */
};

/**
   Returns h->hashfn(k), or whhash_hash_val_err if either h or k are 0.
*/
static whhash_val_t whhash_hash(whhash_table *h, void const *k)
{
    if( !h || !k ) return whhash_hash_val_err;
    /* Aim to protect against poor hash functions by adding logic here
     * - logic taken from java 1.4 whhash_table source */
    whhash_val_t i = h->hashfn(k);
    i += ~(i << 9);
    i ^=  ((i >> 14) | (i << 18)); /* >>> */
    i +=  (i << 4);
    i ^=  ((i >> 10) | (i << 22)); /* >>> */
    return i;
}

/*****************************************************************************/
/* Returns (hashvalue % tablelength) */
#if 0
static size_t whhash_index(size_t tablelength, size_t hashvalue)
{
    return (hashvalue % tablelength);
}
#else
#define whhash_index(LEN,HV) (HV % LEN)
#endif

/**
   For internal use only: this func frees the memory associated with
   key, using h->freeKey (if set).  Results are undefined if key is
   not a key in h and if key is used after this func is called.
*/
static void whhash_free_key(whhash_table const * h, void * key )
{
    if( h && key && h->freeKey ) h->freeKey( key );
}

/**
   Like whhash_free_key() but applies to the value.
*/
static void whhash_free_val(whhash_table const * h, void * val )
{
    if( h  && val && h->freeVal ) h->freeVal( val );
}


/*
Credit for primes table: Aaron Krowne
 http://br.endernet.org/~akrowne/
 http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
*/
static const whhash_val_t primes[] = {
#if 0
2,3,5,7,11,13,
#endif
#if 0
17,19,23,29,31,
#endif
#if 0
37,41,43,47,
#endif
#if 1
7,13,23,
#endif
53, 97, 193, 389,
769, 1543, 3079, 6151,
12289, 24593, 49157, 98317,
196613, 393241, 786433, 1572869,
3145739, 6291469, 12582917, 25165843,
50331653, 100663319, 201326611, 402653189,
805306457, 1610612741
};
const whhash_val_t prime_table_length = sizeof(primes)/sizeof(primes[0]);
const float max_load_factor =
    /* original developer's value was 0.65. */
    0.70
    ;


/**
   Custom implementation of ceil() to avoid a dependency on
   libmath on some systems (e.g. the tcc compiler).
*/
static whhash_val_t whhash_ceil( double d )
{
    whhash_val_t x = (whhash_val_t)d;
    return (( d - (double)x )>0.0)
	? (x+1)
	: x;
}

/*****************************************************************************/
whhash_table *
whhash_create(whhash_val_t minsize,
	      whhash_val_t (*hashf) (void const *),
	      int (*eqf) (void const *,void const *))
{
    whhash_table *h;
    whhash_val_t pindex, size = primes[0];
    /* Check requested whhash_table isn't too large */
    if (minsize > (1u << 30)) return NULL;
    /* Enforce size as prime */
    for (pindex=0; pindex < prime_table_length; pindex++) {
        if (primes[pindex] > minsize) { size = primes[pindex]; break; }
    }
    h = (whhash_table *)malloc(sizeof(whhash_table));
    if (NULL == h) return NULL; /*oom*/
    *h = whhash_init;
    h->stats.alloced = sizeof(whhash_table);
    h->freeKey = 0;
    h->freeVal = 0;
#if 0
    h->table = (whhash_entry **)malloc(sizeof(whhash_entry*) * size);
    if(h->table) memset(h->table, 0, size * sizeof(whhash_entry *));
#else
    h->table = (whhash_entry **)calloc(size, sizeof(whhash_entry*));
#endif
    if (NULL == h->table) { free(h); return NULL; } /*oom*/
    h->stats.alloced += (sizeof(whhash_entry*) * size);
    h->tablelength  = size;
    h->primeindex   = pindex;
    h->entrycount   = 0;
    h->hashfn       = hashf;
    h->eqfn         = eqf;
    h->loadlimit    = whhash_ceil(size * max_load_factor);
    return h;
}
/*****************************************************************************/



static int
whhash_expand(whhash_table *h)
{
    if( ! h ) return 0;
    /* Jump up to the next size in the primes table to accomodate more entries */
    whhash_entry **newtable;
    whhash_entry *e;
    whhash_entry **pE;
    whhash_val_t newsize, i, index;
    /* Check we're not hitting max capacity */
    if (h->primeindex == (prime_table_length - 1)) return 0;
    newsize = primes[++(h->primeindex)];
    newtable = (whhash_entry **)malloc(sizeof(whhash_entry*) * newsize);
    if (NULL != newtable)
    {
        memset(newtable, 0, newsize * sizeof(whhash_entry *));
        /* This algorithm is not 'stable'. ie. it reverses the list
         * when it transfers entries between the tables */
        for (i = 0; i < h->tablelength; i++) {
            while (NULL != (e = h->table[i])) {
                h->table[i] = e->next;
                index = whhash_index(newsize,e->h);
                e->next = newtable[index];
                newtable[index] = e;
            }
        }
        free(h->table);
        h->table = newtable;
    }
    /* Plan B: realloc instead */
    else 
    {
        newtable = (whhash_entry **)
                   realloc(h->table, newsize * sizeof(whhash_entry *));
        if (NULL == newtable) { (h->primeindex)--; return 0; }
        h->table = newtable;
        memset(newtable[h->tablelength], 0, newsize - h->tablelength);
        for (i = 0; i < h->tablelength; i++) {
            for (pE = &(newtable[i]), e = *pE; e != NULL; e = *pE) {
                index = whhash_index(newsize,e->h);
                if (index == i)
                {
                    pE = &(e->next);
                }
                else
                {
                    *pE = e->next;
                    e->next = newtable[index];
                    newtable[index] = e;
                }
            }
        }
    }
    ++h->stats.expansions;
    h->stats.alloced += (sizeof(whhash_entry*) * newsize)
	- (sizeof(whhash_entry*) * h->tablelength);
    h->tablelength = newsize;
    h->loadlimit   = whhash_ceil(newsize * max_load_factor);
    return -1;
}

size_t
whhash_count(whhash_table const * h)
{
    return h ? h->entrycount : 0;
}

int
whhash_replace(whhash_table *h, void *k, void *v)
{
    if( !h || !k
	|| !h->tablelength /* avoid a possible /-by-0 in whhash_index()*/
	) return 0;
    whhash_entry *e;
    whhash_val_t hashvalue, index;
    hashvalue = whhash_hash(h,k);
    index = whhash_index(h->tablelength,hashvalue);
    e = h->table[index];
    while (NULL != e)
    {
        /* Check hash value to short circuit heavier comparison */
        if (
	    (k == e->k)
	    ||
	    ((hashvalue == e->h) && (h->eqfn(k, e->k)))
	    )
        {
	    if( v == e->v ) return 1;
	    whhash_free_val( h, e->v );
	    e->v = v;
	    return -1;

        }
        e = e->next;
    }
    return 0;
}

int
whhash_insert(whhash_table *h, void *k, void *v)
{
    if( ! h || !k
	|| !h->tablelength /* avoid a possible /-by-0 in whhash_index()*/
	) return 0;
#if 0
    /* Stephan Beal, 13 Feb 2008: now simply replaces the value of existing entries.

    A week or three later: profiling has shown that this nearly doubles the insertion
    time. One app was spending almost 1% of its time in this code.
    */
    int rc = whhash_replace(h, k, v);
    if( 0 != rc ) return rc;
#endif
    whhash_val_t index;
    whhash_entry *e;
    if (++(h->entrycount) > h->loadlimit)
    {
        /* Ignore the return value. If expand fails, we should
         * still try cramming just this value into the existing table
         * -- we may not have memory for a larger table, but one more
         * element may be ok. Next time we insert, we'll try expanding again.*/
        whhash_expand(h);
    }
    e = (whhash_entry *)malloc(sizeof(whhash_entry));
    if (NULL == e) { --(h->entrycount); return 0; } /*oom*/
    h->stats.alloced += sizeof(whhash_entry);
    e->h = whhash_hash(h,k);
    index = whhash_index(h->tablelength,e->h);
    e->k = k;
    e->v = v;
    e->next = h->table[index];
    h->table[index] = e;
    ++h->stats.insertions;
    return 1;
}

static whhash_entry * whhash_search_entry(whhash_table *h,
					  void const *k)
{
    if( !h || !k
	|| !h->tablelength /* avoid a possible /-by-0 in whhash_index()*/
	) return 0;
    ++h->stats.searches;
    whhash_entry * e = 0;
    whhash_val_t hashvalue, index;
    hashvalue = whhash_hash(h,k);
    index = whhash_index(h->tablelength,hashvalue);
    e = h->table[index];
    while (NULL != e)
    {
        /* Check hash value to short circuit heavier comparison */
        if ((k == e->k) || ((hashvalue == e->h) && (h->eqfn(k, e->k))))
	{
	    return e;
	}
        e = e->next;
	++h->stats.search_collisions;
    }
    return NULL;
}

short whhash_contains(whhash_table *h, void const *k)
{
    whhash_entry * e = whhash_search_entry(h,k);
    return e ? 1 : 0;
}

void *
whhash_search(whhash_table *h, void const *k)
{
    if( !h || !k ) return 0;
    whhash_entry * e = whhash_search_entry(h,k);
    return e ? e->v : 0;
}

void * whhash_take(whhash_table *h, void const *k)
{
    if( !h || !k
	|| !h->tablelength /* avoid a possible /-by-0 in whhash_index()*/
	) return 0;
	
    /* TODO: consider compacting the table when the load factor drops enough,
     *       or provide a 'compact' method. */
    whhash_entry *e;
    whhash_entry **pE;
    void *v;
    whhash_val_t hashvalue, index;
    // Can we reimplement this using whhash_search_entry()?
    hashvalue = whhash_hash(h,k);
    index = whhash_index(h->tablelength,whhash_hash(h,k));
    pE = &(h->table[index]);
    e = *pE;
    while (NULL != e)
    {
        /* Check hash value to short circuit heavier comparison */
        if ( (e->k==k)
             || ((hashvalue == e->h) && (h->eqfn(k, e->k)))
             )
        {
            *pE = e->next;
            --h->entrycount;
            v = e->v;
	    h->stats.alloced -= sizeof(whhash_entry);
            free(e);
	    ++h->stats.removals;
            return v;
        }
        pE = &(e->next);
        e = e->next;
    }
    return NULL;
}

short whhash_remove(whhash_table *h, void *k)
{
    if( !h || !k ) return 0;
    void * v = whhash_take(h,k);
    whhash_free_key(h,k);
    if( v )
    {
	whhash_free_val(h,v);
    }
    return NULL != v;
}

void whhash_set_val_dtor( whhash_table * h, void (*dtor)( void * ) )
{
    if( h ) h->freeVal = dtor;
}
void whhash_set_key_dtor( whhash_table * h, void (*dtor)( void * ) )
{
    if( h ) h->freeKey = dtor;
}
void whhash_set_dtors( whhash_table * h, void (*keyDtor)( void * ), void (*valDtor)( void * ) )
{
    whhash_set_key_dtor( h, keyDtor );
    whhash_set_val_dtor( h, valDtor );
}

static whhash_entry * whhash_free_entry( whhash_table * h, whhash_entry * e )
{
    if( !h || !e ) return 0;
    whhash_entry * next = e->next;
    h->entrycount--;
    whhash_free_key( h, e->k );
    whhash_free_val( h, e->v );
    free(e);
    h->stats.alloced -= sizeof(whhash_entry);
    return next;
}

void whhash_clear(whhash_table *h)
{
    if( ! h || !h->table ) return;
    //printf("whhash_clear(): alloced=%u\n",h->stats.alloced);
    whhash_val_t i;
    whhash_entry *e;
    whhash_entry **table = h->table;
    for (i = 0; i < h->tablelength; i++)
    {
        e = table[i];
        while (NULL != e)
        {
            e = whhash_free_entry( h, e );
        }
    }
    //printf("whhash_clear(): alloced=%u\n",h->stats.alloced);
    // Which of these is correct?
    h->stats.alloced -= (sizeof(whhash_entry*) * h->tablelength);
    //printf("tablelen=%u sizeof... = %u\n",h->tablelength,(sizeof(whhash_entry*) * h->tablelength));
    free(h->table);
    h->table = 0;
    h->entrycount = 0;
    h->tablelength = 0;
    h->loadlimit = 0;
    //printf("whhash_clear(): alloced=%u\n",h->stats.alloced); // why is this 0 instead of sizeof(whhash_table)?
    h->stats.alloced = sizeof(whhash_table);
    //printf("whhash_clear(): alloced=%u\n",h->stats.alloced);
}
void
whhash_destroy(whhash_table *h)
{
    if( h )
    {
	whhash_clear(h);
	free(h);
    }
}

whhash_stats whhash_get_stats( whhash_table const * h )
{
    return h ? h->stats : whhash_stats_init;
}

int whhash_cmp_cstring( void const * k1, void const * k2 )
{
    if( (k1 == k2) ) return 1;
    else if( (!k1 && k2) || (k1 && !k2) ) return 0; /* protect against passing null to strcmp() */
    return (0 == strcmp( (char const *) k1, (char const *) k2 ));
}

int whhash_cmp_long( void const * k1, void const * k2 )
{
    return *((long const*)k1) == *((long const*)k2);
}

int whhash_cmp_void_ptr( void const * k1, void const * k2 )
{
    return k1 == k2;
}

whhash_val_t whhash_hash_void_ptr( void const * k )
{
     /* This next typedef must *apparently* be the same size as the platform's (void*) */
    typedef long kludge_t;
    /**
       Originally we used the void* address as a hashvalue, until it
       came to me that those doesn't distribute well within the hash
       range (they tend to clump together).

       So below we do a variant of the "Modified Bernstein Hash"
       on the bytes of the (void*)'s numeric value. For details:

       http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx

       Initial (but minimal) tests show the distribution to be much improved
       over using the (void*) as the hash value.
    */
    whhash_val_t h = 0;
    kludge_t const x = (kludge_t const) k;
    unsigned char const * c = (unsigned char const *)&x;
    size_t i = 0;
    for( i = 0; i < (sizeof(kludge_t) / sizeof(char)); ++i )
    {
	h = 33 * h ^ c[i];
    }
    //MARKER; printf("hash of %p: x=%ld h=%ld, i=%d, sizeof1=%u sizeof2=%u\n",k,x,h,i,sizeof(kludge_t),sizeof(char));
    return h;
}


whhash_val_t
whhash_hash_cstring_djb2( void const * vstr)
{ /* "djb2" algo code taken from: http://www.cse.yorku.ca/~oz/hash.html */
    if( ! vstr ) return whhash_hash_val_err;
    whhash_val_t hash = 5381;
    int c = 0;
    char const * str = (char const *)vstr;
    if( ! str ) return whhash_hash_val_err;
    while( (c = *str++) )
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

whhash_val_t whhash_hash_cstring_djb2m( void const * vstr )
{
    char const * str = (char const *)vstr;
    if( ! str ) return whhash_hash_val_err;
    whhash_val_t h = 0;
    int c = 0;
    while( (c = *(str++)) )
    {
	h = 33 * h ^ c;
    }
    return h;
}

#if 0
/**
   Implements the Fowler/Noll/Vo (FNV) hash, as described at:

   http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx
*/
//whhash_val_t whhash_hash_cstring_fnv( void const * str );
whhash_val_t whhash_hash_cstring_fnv( void const * vstr )
{
    char const * str = (char const *)vstr;
    if( ! str ) return whhash_hash_val_err;
    whhash_val_t h = (unsigned long long) 2166136261L;
    /**
       ^^^^
       gcc says: warning: this decimal constant is unsigned only in ISO C90
    */
    int c = 0;
    while( (c = *(str++)) )
    {
	h = (h * 16777619L) ^ c;
    }
    return h;
}
#endif

whhash_val_t whhash_hash_cstring_oaat( void const * vstr )
{
    char const * str = (char const *)vstr;
    if( ! str ) return whhash_hash_val_err;
    whhash_val_t h = 0;
    int c = 0;
    while( (c = *(str++)) )
    {
	h += c;
	h += (h << 10);
	h ^= (h >> 6);
    }
    h += ( h << 3 );
    h ^= ( h >> 11 );
    h += ( h << 15 );
    return h;
}


whhash_val_t
whhash_hash_cstring_sdbm(void const *vstr)
{ /* "sdbm" algo code taken from: http://www.cse.yorku.ca/~oz/hash.html */
    char const * str = (char const *)vstr;
    if( ! str ) return whhash_hash_val_err;
    whhash_val_t h = 0;
    int c = 0;
    while( str && (c = *str++) )
    {
        h = c + (h << 6) + (h << 16) - h;
    }
    return h;
}

whhash_val_t
whhash_hash_cstring_rot( void const * vstr)
{
    char const * str = (char const *)vstr;
    if( ! str ) return whhash_hash_val_err;
    whhash_val_t h = 0;
    int c = 0;
    while( (c = *(str++)) )
    {
	h = (h << 4) ^ (h >> 28 ) ^ c;
    }
    return h;
}

whhash_val_t
whhash_hash_cstring_sax( void const * vstr)
{
    char const * str = (char const *)vstr;
    if( ! str ) return whhash_hash_val_err;
    whhash_val_t h = 0;
    int c = 0;
    while( (c = *(str++)) )
    {
	h ^= (h << 5) + (h >> 2) + c;
    }
    return h;
}


whhash_val_t
whhash_hash_long( void const * n )
{
    return n
        ? *((whhash_val_t const *)n)
        : whhash_hash_val_err;
}

struct whhash_iter
{
    whhash_table *h;
    whhash_entry *e;
    whhash_entry *parent;
    unsigned int index;
};
static const whhash_iter whhash_itr_init = {0,0,0,0};

whhash_iter *
whhash_get_iter(whhash_table *h)
{
    if( !whhash_count(h) ) return 0;
    unsigned int i, tablelength;
    whhash_iter *itr = (whhash_iter *)
        malloc(sizeof(whhash_iter));
    if (NULL == itr) return NULL;
    h->stats.alloced += sizeof(whhash_iter);
    *itr = whhash_itr_init;
    itr->h = h;
    itr->e = NULL;
    itr->parent = NULL;
    tablelength = h->tablelength;
    itr->index = tablelength;
    //if (0 == h->entrycount) return itr;

    for (i = 0; i < tablelength; i++)
    {
        if (NULL != h->table[i])
        {
            itr->e = h->table[i];
            itr->index = i;
            break;
        }
    }
    return itr;
}

void *
whhash_iter_key(whhash_iter *i)
{ return i ? i->e->k : 0; }

void *
whhash_iter_value(whhash_iter *i)
{ return i ? i->e->v : 0; }

int
whhash_iter_advance(whhash_iter *itr)
{
    unsigned int j,tablelength;
    whhash_entry **table;
    whhash_entry *next;
    if (!itr || (!itr->e)) return 0;

    next = itr->e->next;
    if (NULL != next)
    {
        itr->parent = itr->e;
        itr->e = next;
        return -1;
    }
    tablelength = itr->h->tablelength;
    itr->parent = NULL;
    if (tablelength <= (j = ++(itr->index)))
    {
        itr->e = NULL;
        return 0;
    }
    table = itr->h->table;
    while (NULL == (next = table[j]))
    {
        if (++j >= tablelength)
        {
            itr->index = tablelength;
            itr->e = NULL;
            return 0;
        }
    }
    itr->index = j;
    itr->e = next;
    return -1;
}

/*****************************************************************************/
/* remove - remove the entry at the current iterator position
 *          and advance the iterator, if there is a successive
 *          element.
 * The key and value of the element are freed using
 * whhash_free_key/val(), which may or may not actually
 * free them.
 *          Returns zero if end of iteration.
 */

int
whhash_iter_remove(whhash_iter *itr)
{
    whhash_entry *remember_e, *remember_parent;
    int ret = 0;

    /* Do the removal */
    if (NULL == (itr->parent))
    {
        /* element is head of a chain */
        itr->h->table[itr->index] = itr->e->next;
    } else {
        /* element is mid-chain */
        itr->parent->next = itr->e->next;
    }
    /* itr->e is now outside the whhash_table */
    remember_e = itr->e;
    itr->h->entrycount--;
    whhash_free_key( itr->h, remember_e->k );
    whhash_free_val( itr->h, remember_e->v );

    /* Advance the iterator, correcting the parent */
    remember_parent = itr->parent;
    ret = whhash_iter_advance(itr);
    if (itr->parent == remember_e) { itr->parent = remember_parent; }
    free(remember_e);
    itr->h->stats.alloced -= sizeof(whhash_entry);
    return ret;
}

int
whhash_iter_search(whhash_iter *itr,
		   void *k)
{
    if( ! itr  || !itr->h || !k
	|| !itr->h->tablelength /* avoid a possible /-by-0 in whhash_index()*/
	) return 0;
    whhash_entry *e, *parent;
    unsigned int hashvalue, index;
    whhash_table *h = itr->h;
    hashvalue = whhash_hash(h,k);
    index = whhash_index(h->tablelength,hashvalue);
    e = h->table[index];
    parent = NULL;
    while (NULL != e)
    {
        /* Check hash value to short circuit heavier comparison */
        if ((e->k == k)
	    ||
	    ((hashvalue == e->h) && (h->eqfn(k, e->k)))
	    )
        {
            itr->index = index;
            itr->e = e;
            itr->parent = parent;
            itr->h = h;
            return -1;
        }
        parent = e;
        e = e->next;
    }
    return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

/*
 Copyright (c) 2002, Christopher Clark
 Copyright (C) 2008 Stephan Beal (http://wanderinghorse.net/home/stephan/)
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 
 * Neither the name of the original author; nor the names of any contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.
 
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
