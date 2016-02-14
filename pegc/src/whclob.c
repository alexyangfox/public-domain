#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "whclob.h"
#include "vappendf.h"

#if WHCLOB_USE_ZLIB
#  include <zlib.h>
#endif

#define MARKER if(1) printf("MARKER: %s:%d:%s() ",__FILE__,__LINE__,__func__);if(1)printf

#define WHCLOB_DEBUG 0


const whclob_rc_t whclob_rc = {0, /*  OK */
			 -1, /* Err */
			 -2, /* AllocError */
			 -3, /* UnexpectedNull */
			 -4, /* RangeError */
			 -5, /* IOError */
			 -6  /* ArgError */
};


struct whclob
{
	/*
	  The address of the memory this blob refers to.  See below
	  for more info.

	  If aData is pointed to a const string or a string owned by
	  another entity, then nAlloc must be set to 0. This indicates
	  to the framework that aData is not to be deleted when the
	  blob is freed. In this case, nUsed must be set to the full
	  length of the pointed-to data object. When shallow copies
	  like this exist, any change made to pointed-to copies will
	  invalidate the state of the doing-the-pointing whclob object.

	  The framework tries to ensure that aData[nUsed] is always
	  set to 0 (assuming that aData is not manipulated outside of
	  the clob API). Note that since aData may contain binary data
	  (and embedded nulls), such a trailing null cannot be used
	  for length determination purposes.
   	 */
	char * aData;
	/**
	  long nAlloc = the number of contiguous bytes allocated to
	  aData.  See below for more info.
	*/
	long nAlloc;
	/*
	  long nUsed = the number of contiguous bytes "used" in the
	  aData array.
	*/
	long nUsed;
	/*
	  The current position marker for some read operations.
	*/
	long nCursor;
    whclob_alloc_policy_t allocPolicy;
};


/**
   Default alloc size policy, simply returns n.
*/
static long whclob_default_alloc_policy( long n )
{
	return n;
}

/*
  Clob_empty is an empty whclob used for init purposes.
*/
static const whclob Clob_empty = {0, /* aData */
				  0, /* nAlloc */
				  0, /* nUsed */
				  0, /* nCursor */
				  whclob_default_alloc_policy
};

/**
  A debugging-only function. Do not use it in client code.
*/
void whclob_dump( whclob * cb, int doString );

typedef whclob_alloc_policy_t RePol;
static RePol whclob_current_alloc_policy = whclob_default_alloc_policy;
RePol whclob_set_default_alloc_policy( RePol f )
{
	RePol old = whclob_current_alloc_policy;
	whclob_current_alloc_policy = f ? f : whclob_default_alloc_policy;
	return old;
}
whclob_alloc_policy_t whclob_set_alloc_policy( whclob * cb, whclob_alloc_policy_t f )
{
    RePol old = cb ? cb->allocPolicy : 0;
    if( cb ) cb->allocPolicy = f;
    return old;
}


#define WHCLOB_DUMP(X,B) if(WHCLOB_DEBUG) { printf(X ": blob [%s]: ", # B ); whclob_dump(B,1); }
void whclob_dump( whclob * cb, int doString )
{
    whclob * dest = whclob_new();
    if( ! dest ) return;
    whclob_appendf( dest,
		    "whclob@%p[nUsed=%d, nAlloc=%d, nCursor=%d][data@%p]",
		    cb, cb->nUsed, cb->nAlloc, cb->nCursor, cb->aData
		    );
    if( doString )
    {
	if( cb->nAlloc && cb->aData[0] )
	{
	    whclob_appendf( dest, "=[%s]", whclob_buffer(cb) );
	}
	else
	{
	    whclob_appendf( dest, "=[NULL]", cb );
	}
    }
    fappendf( stderr, "%s\n", whclob_buffer( dest ) );
    whclob_finalize( dest );
}


long whclob_reset( whclob * cb )
{
	if( cb )
	{
#if WHCLOB_DEBUG
		printf( "Freeing clob @ [%p], bytes=%ld\n", cb, cb->nAlloc );
		/* endless loop: whclob_dump(cb,1); */
#endif
		if( cb->nAlloc )
		{
		    /* crash? memset( cb->aData, 0, cb->nAlloc ); */
			free( cb->aData );
                        cb->aData = 0;
		}
		*cb = Clob_empty;
		return whclob_rc.OK;
	}
	return whclob_rc.UnexpectedNull;
}

long whclob_finalize( whclob * cb )
{
    if( ! cb ) return whclob_rc.UnexpectedNull;
    whclob_reset(cb);
    free( cb );
    return whclob_rc.OK;
}

void whclob_force_in_bounds( whclob * cb )
{
    if( cb->nUsed > cb->nAlloc ) cb->nUsed = (cb->nAlloc ? (cb->nAlloc - 1) : 0);
    if( cb->nCursor > cb->nUsed ) cb->nCursor = cb->nUsed;
}

static long whclob_do_resize( whclob * cb,
			      unsigned int sz,
			      short usePolicyHint)
{
    static const int fudge = 2;
    char const * zOld = 0;
    long oldUsed = 0;
    long oldAlloc = 0;
    long allocsize = 0;
    char * pNew = 0;
    if( !cb ) return whclob_rc.UnexpectedNull;
    if( cb->nAlloc == (long)sz ) return cb->nAlloc;
    /* ^^^ over-allocate by 1 to ensure we have space for
       a trailing 0. */
    /* const int shrinkage = 16; */
    /* if we can save more than this much, then try to do so. */
    if( 0 == sz )
    {
	whclob_reset( cb );
	return 0;
    }
    zOld = cb->aData;
    oldUsed = cb->nUsed;
    oldAlloc = cb->nAlloc;
    allocsize = 0;
    if( (usePolicyHint && (0 != cb->allocPolicy)) )
    {
	allocsize = cb->allocPolicy(sz);
    }
    else if( (usePolicyHint && (0 != whclob_current_alloc_policy)) )
    {
	allocsize = (*whclob_current_alloc_policy)(sz);
    }
    else
    {
	allocsize = (long)sz;
    }
    allocsize += fudge;
    if( allocsize < (fudge + (long)sz) ) allocsize = fudge + sz;
#if 0
    pNew = cb->aData /*  oldAlloc */
	? realloc( cb->aData, allocsize )
	: malloc( fudge + allocsize );
#else
    pNew = realloc( cb->aData, allocsize );
#endif
    if( ! pNew ) return whclob_rc.AllocError;
    /* if( !oldAlloc ) */
    if( ! cb->aData )
    { /** cb has/had no data */
	if( zOld )
	{ /* cb was pointing to shared data. Copy it. */
	    memcpy( pNew, zOld, (oldUsed > allocsize) ? allocsize : oldUsed );
	}
	else
	{ /* cb had no buffer - create one. */
	    memset( pNew, 0, allocsize );
	    cb->nUsed = cb->nCursor = 0;
	}
    }
    else
    { /** cb had data and we realloced. Zero out any new
	  memory. */
	if( oldAlloc && (oldAlloc < allocsize) )
	{
	    memset( pNew + oldAlloc, 0, allocsize - oldAlloc);
	}
    }
    pNew[allocsize-fudge] = 0;
    cb->aData = pNew;
    cb->nAlloc = allocsize;
    whclob_force_in_bounds( cb );
    return cb->nAlloc;
}

long whclob_reserve( whclob * cb, unsigned int sz )
{
    long rc = cb->nAlloc;
    if( ((long)sz > cb->nAlloc) || (sz==0) )
    {
	rc = whclob_do_resize( cb, sz, 1 );
    }
    return rc;
}

long whclob_size( whclob const * cb ) { return cb ? cb->nUsed : whclob_rc.UnexpectedNull; }
long whclob_capacity( whclob const * cb ) { return cb ? cb->nAlloc : whclob_rc.UnexpectedNull; }
char * whclob_buffer( whclob * cb ) { return cb ? cb->aData : 0; }
char const * whclob_bufferc( whclob const * cb ) { return cb ? cb->aData : 0; }

long whclob_resize( whclob * cb, unsigned int sz )
{
    unsigned long ret = whclob_do_resize( cb, sz, 1 );
    if( sz && (ret >= sz) )
    {
	cb->nUsed = sz;
	cb->aData[sz] = 0;
    }
    return ret;
}

#if 0
/*
  NOT part of the public API!

  sz is forced into bounds (>=0 and <=cb->nAlloc) then cb->nUsed is
  set to sz. cb->aData[sz] is set to 0 to enforce the null-termination
  policy.
*/
static long whclob_set_used_size( whclob * cb, long sz )
{
    if( sz < 0 ) sz = 0;
    else if( sz > cb->nAlloc ) sz = cb->nAlloc-1;
    cb->nUsed = sz;
    cb->aData[sz] = 0;
    return sz;
}
#endif /*  0|1 */

whclob * whclob_new_n( size_t reserved )
{
    whclob * c = 0;
    whclob_init( &c, 0, (long)reserved );
    return c;
}
whclob * whclob_new()
{
    return whclob_new_n(0);
}


long whclob_init( whclob ** cb, char const * data, long n )
{
    long rc = 0;
    if( ! cb )
    {
	return whclob_rc.UnexpectedNull;
    }
    *cb = (whclob *) malloc( sizeof(whclob) );
    if( ! *cb )
    {
	return whclob_rc.AllocError;
    }
    *(*cb) = Clob_empty;
    if( !data )
    {
	if( n < 1 ) return whclob_rc.OK;
    }
    else
    {
	if( !n ) return whclob_rc.OK;
    }
    if( data && (n < 0) ) n = strlen( data );
#if 1
    rc = whclob_reserve( *cb, n );
    //rc = whclob_resize( *cb, n );
    if( rc < whclob_rc.OK )
    {
	free( *cb );
	*cb = 0;
	return rc;
    }
    if( data )
    {
	memcpy( (*cb)->aData, data, n );
    }
#else
    rc = whclob_append( *cb, data, n );
    if( rc < whclob_rc.OK )
    {
	free( *cb );
	*cb = 0;
	return rc;
    }
#endif
    return whclob_rc.OK;
}


long whclob_seek( whclob * cb, long offset, int whence )
{
	long pos = cb->nCursor;
	switch( whence )
	{
	  case WHCLOB_SEEK_SET:
		  pos = offset;
		  break;
	  case WHCLOB_SEEK_CUR:
		  pos += offset;
		  break;
	  case WHCLOB_SEEK_END:
		  pos = cb->nUsed + offset;
		  break;
	  default:
		  return whclob_rc.RangeError;
	};
	if( pos < 0 ) pos = 0;
	else if( pos > cb->nUsed ) pos = cb->nUsed;
	return (cb->nCursor = pos);
}

void whclob_rewind( whclob * cb )
{
	cb->nCursor = 0;
}

long whclob_tell( whclob const * cb )
{
	return cb->nCursor;
}

long whclob_pos_in_bounds( whclob * cb, long pos )
{
	return (cb && (pos >= 0) && (pos < cb->nUsed))
		? whclob_rc.OK
		: whclob_rc.RangeError;
}

long whclob_char_filln( whclob * cb, char ch, long startPos, long n )
{
    long rc = 0;
    if( ! cb->nAlloc ) return whclob_rc.Err;
    if( n <= 0 ) return whclob_rc.RangeError;
    rc = whclob_pos_in_bounds(cb,startPos);
    if( whclob_rc.OK !=  rc ) return rc;
    /* if( whclob_rc.OK ==  rc ) rc = whclob_is_in_bounds(cb,startPos + n); */
    if( (startPos + n) > cb->nAlloc ) n = cb->nAlloc - startPos;
    memset( cb->aData + startPos, ch, n );
    return n;
}

long whclob_zero_fill( whclob * cb )
{
	return whclob_char_filln( cb, '\0', 0, cb->nAlloc );
}

/**
   This function ensures that the one-past-the-last item in the blob
   to 0.  The "used" size of cb does not change.

   Returns one of the values defined in whclob_rc:

   - whclob_rc.AllocError if a memory (re)allocation fails.

   - whclob_rc.OK on success.
*/
long whclob_null_terminate( whclob * cb )
{
    if( ! cb->nAlloc ) return whclob_rc.Err; /*  do not modify a data obj we don't own. */
	if( (cb->nUsed + 1) >= cb->nAlloc )
	{
		long rc = whclob_reserve( cb, cb->nUsed + 1 );
		if( rc < whclob_rc.OK ) return rc;
	}
	cb->aData[cb->nUsed] = 0;
	return whclob_rc.OK;
}

/**
   Writes n bytes of data from cb, starting at startPos. It expands cb
   if needed. It does NOT move the cursor.

   If n is -1 then strlen(data) is used for n.

   On success returns the number of bytes added. On failure it returns
   a negative integer error code defined in whclob_rc. If either data or
   dsize are 0 then 0 is returned. If cb is null then
   whclob_rc.UnexpectedNull is returned.
*/
static long whclob_writeat( whclob * cb, long startPos, char const * data, long dsize )
{
	if( ! cb ) return whclob_rc.UnexpectedNull;
	if( ! data || !dsize ) return 0;
	if( dsize < 0 ) dsize = strlen(data);
	if( (startPos + dsize) >= cb->nAlloc )
	{
		long allocsz = startPos + dsize; /* + (cb->nAlloc * 11 / 20); */
		long rc = whclob_reserve( cb, allocsz );
		if( rc < whclob_rc.OK ) return rc;
	}
	memcpy( cb->aData + startPos, data, dsize );
	return dsize;
}


long whclob_write( whclob * cb, char const * data, long dsize )
{
	long old = cb->nCursor;
	WHCLOB_DUMP("whclob_write()", cb );
	cb->nCursor += whclob_writeat( cb, cb->nCursor, data, dsize );
	if( cb->nUsed < cb->nCursor )
	{
		cb->nUsed = cb->nCursor;
	}
	WHCLOB_DUMP("whclob_write()", cb );
	return cb->nCursor - old;
}


long whclob_append( whclob * cb, char const * data, long dsize )
{
	long old = cb->nUsed;
	long rc = whclob_writeat( cb, cb->nUsed, data, dsize );
	if( rc < whclob_rc.OK ) return rc;
	cb->nUsed += rc;
	//cb->aData[cb->nUsed] = 0;
	whclob_null_terminate(cb);
	return cb->nUsed - old;
}

long whclob_append_char_n( whclob * cb, char c, long n )
{
    long rc = 0;
    if( !cb || (n <= 0) ) return whclob_rc.RangeError;
    rc = whclob_reserve( cb, cb->nUsed + n );
    if( rc < 0 ) return rc;
    memset( cb->aData + cb->nUsed, c, n );
    cb->nUsed += n;
    /* do we want to do a whclob_null_terminate() here? */
    /* whclob_null_terminate(cb); */
    return n;
}


long whclob_copy( whclob * src, whclob * dest )
{
    long allocsz = 0;
    long rc = 0;
    if( src == dest ) return whclob_rc.RangeError;
    if( src->aData == dest->aData ) return whclob_rc.RangeError;
    allocsz = src->nAlloc;
    WHCLOB_DUMP( "copy src before",src );
    WHCLOB_DUMP( "copy dest before",dest );
    whclob_reset( dest );
    rc = whclob_reserve( dest, allocsz );
    if( rc < whclob_rc.OK ) return rc;
    dest->nUsed = src->nUsed;
    dest->nAlloc = allocsz;
    dest->nCursor = src->nCursor;
    memcpy( dest->aData, src->aData, allocsz );
    WHCLOB_DUMP( "copy src after",src );
    WHCLOB_DUMP( "copy dest after",dest );
    return whclob_rc.OK;
}

long whclob_copy_slice( whclob * src, whclob * dest, long startPos, long n )
{
    long bpos = 0;
    long epos = 0;
    long ret = 0;
    if( ! src || !dest ) return whclob_rc.UnexpectedNull;
    if( n<1 ) return whclob_rc.RangeError;
    if( whclob_rc.OK != whclob_pos_in_bounds( src, startPos ) ) return whclob_rc.RangeError;
    bpos = startPos;
    if( bpos >= src->nUsed ) return 0;
    epos = bpos + n; /* 1-past-end marker */
    if( epos > src->nUsed ) epos = src->nUsed;
    ret = whclob_append( dest, src->aData + bpos, epos - bpos );
    return ret;
}

long whclob_read( whclob * src, whclob * dest, long n )
{
    long bpos = 0;
    long epos = 0;
    long ret = 0;
    if( ! src || !dest || n<1 ) return 0;
    bpos = src->nCursor;
    if( bpos == src->nUsed ) return 0;
    epos = bpos + n; /* 1-past-end marker */
    if( epos > src->nUsed ) epos = src->nUsed;
    ret = whclob_append( dest, src->aData + bpos, epos - bpos );
    src->nCursor += ret;
    return ret;
}

long whclob_truncate( whclob * cb, long pos, int memPolicy )
{
    long rc = 0;
    if( ! cb ) return whclob_rc.UnexpectedNull;
    /* if( cb->nUsed <= pos ) return whclob_rc.OK; */

    if( pos < cb->nAlloc )
    { /* adding a single \0 to the data isn't sufficient here. */
	memset( cb->aData + pos, 0, (cb->nAlloc-pos));
    }

    if( cb->nAlloc <= pos )
    {
	//return whclob_rc.OK;
	memPolicy = 1;
    }
    rc = whclob_rc.OK;
    if( memPolicy > 0 )
    {
	rc = whclob_reserve( cb, pos );
    }
    else if( memPolicy < 0 )
    {
#if 0
	/* try a simple heuristic to calculate whether a
	   realloc is worth it... */
	const long diff = (cb->nAlloc - pos);
	const int rel = 4; /* ((diff) >= (cb->nAlloc/rel)) = do realloc */
	const int abs = 512; /* (diff >= abs) = do realloc */
	if(
	   ( diff >= abs )
	   ||
	   ((diff) >= (cb->nAlloc/rel) )
	   )
	{
	    /* rc = whclob_resize( cb, cb->nUsed ); */
	    rc = whclob_do_resize( cb, pos, 0 );
	}
#else
	//MARKER("TRUNCATE alloc=%ld len=%ld\n", cb->nAlloc, cb->nUsed );
	rc = whclob_resize( cb, pos );
	/* rc = whclob_do_resize( cb, cb->nUsed, 0 ); */
	//MARKER("TRUNCATED: alloc=%ld len=%ld, rc=%ld\n", cb->nAlloc, cb->nUsed, rc );
#endif
    }
    cb->nUsed = pos;
    //whclob_null_terminate( cb );
    return rc;
}


long whclob_memmove( whclob * cb, int start1, int n, int start2 )
{
	return whclob_memmove_fill( cb, 0, start1, n, start2 );
}
long whclob_memmove_fill( whclob * cb, char const filler, int start1, int n, int start2 )
{
    char * src = 0;
    char * dest = 0;
    long pos = 0;  /* here we'll take a long "pos" ... */
    if( ! cb ) return whclob_rc.UnexpectedNull;
    if(
       ( (n<1) || (start2 == start1) )
       ||
       (whclob_rc.OK != whclob_pos_in_bounds(cb,start1))
       ||
       (whclob_rc.OK != whclob_pos_in_bounds(cb,start1+n))
       ||
       (whclob_rc.OK != whclob_pos_in_bounds(cb,start2))
       ||
       (whclob_rc.OK != whclob_pos_in_bounds(cb,start2+n))
       )
    {
	return whclob_rc.RangeError;
    }
    src = cb->aData + start1;
    dest = cb->aData + start2;
    pos = 0;
    while( pos < n )
    {
	dest[start2++] = src[pos];
	src[pos++] = filler;
    }
    return n;
}

/**
   Swaps n bytes of memory contents between m1 and m2. No bounds
   checking is (or can be) done here, so m1 and m2 must both be large
   enough for the given start1, start2, and n parameters.
*/
static long memswap( char * m1,
		     long n,
		     char * m2 )
{
	char buf;
	long pos = 0;
	for( ; pos < n; ++pos )
	{
		buf = m2[pos];
		m2[pos] = m1[pos];
		m1[pos] = buf;
	}
	return pos;
}

long whclob_memswap( whclob * c1, int start1, int n, whclob * c2, int start2 )
{
	if( ! c1 || ! c2 ) return whclob_rc.UnexpectedNull;
	if(
	   ( (n<1) )
	   ||
	   (whclob_rc.OK != whclob_pos_in_bounds(c1,start1))
	   ||
	   (whclob_rc.OK != whclob_pos_in_bounds(c1,start1+n))
	   ||
	   (whclob_rc.OK != whclob_pos_in_bounds(c2,start2))
	   ||
	   (whclob_rc.OK != whclob_pos_in_bounds(c2,start2+n))
	   )
	{
		return whclob_rc.RangeError;
	}
	return memswap( c1->aData + start1, n, c2->aData + start2 );
}


long whclob_swap( whclob * c1, whclob * c2 )
{
    whclob x;
    if( ! c1 || ! c2 ) return whclob_rc.UnexpectedNull;
    x = *c1;
    *c1 = *c2;
    *c2 = x;
    return whclob_rc.OK;
}

long whclob_clone( whclob * src, whclob ** dest )
{
    long rc = 0;
    if( ! src ) return whclob_rc.UnexpectedNull;
    rc = whclob_init( dest, src->aData, src->nUsed );
    if( whclob_rc.OK != rc ) return rc;
    memcpy( (*dest)->aData, src->aData, src->nUsed );
    (*dest)->nUsed = src->nUsed;
    (*dest)->nCursor = src->nCursor;
    return whclob_rc.OK;
}

/**
   A whclob vappendf_appender implementation. It assumes that
   arg is a (whclob *) and uses whclob_append() to push
   all data to that clob.

   If s is null then 0 is returned.

   Returns the number of bytes appended.
*/
static long whclob_vappendf_appender( void * arg, char const * s, long n )
{
	whclob * c = (whclob *)arg;
	if( ! c ) return 0;
	return whclob_append( c, s, n );
}

long whclob_vappendf( whclob * cb, char const * fmt, va_list vargs )
{
	return vappendf( whclob_vappendf_appender, cb, fmt, vargs );
}

long whclob_appendf( whclob * cb, const char * fmt, ... )
{
	va_list vargs;
	long ret = 0;
	va_start( vargs, fmt );
	ret = whclob_vappendf( cb, fmt, vargs );
	va_end(vargs);
	return ret;
}



long whclob_export( whclob const * cb, void * varg, whclob_exporter pf )
{
	if( ! cb->nAlloc ) return whclob_rc.UnexpectedNull;
	return pf( varg, cb->aData, cb->nUsed );
}


long whclob_import( whclob * dest, void * arg, whclob_importer pf )
{
    return pf( dest, arg );
}


char * whclob_vmprintf( char const * fmt, va_list vargs )
{
    whclob * buf = 0;
    char * ret = 0;
    if( whclob_rc.OK != whclob_init(&buf,0,0) ) return 0;
    whclob_vappendf( buf, fmt, vargs );
    ret = buf->aData;
    *buf = Clob_empty;
    whclob_finalize( buf );
    return ret;
}
char * whclob_mprintf( char const * fmt, ... )
{
    va_list vargs;
    char * ret = 0;
    va_start( vargs, fmt );
    ret = whclob_vmprintf( fmt, vargs );
    va_end(vargs);
    return ret;
}

char * whclob_take_buffer( whclob * cb )
{
    char * c = cb->aData;
    *cb = Clob_empty;
    return c;
}



#if WHCLOB_USE_ZLIB
/* ! whclob_zheader_prefix == magic cookie for our compressed whclob.header */
static char const whclob_zheader_version = '1';
static char const * whclob_zheader_prefix = "clob32";
static int const whclob_zheader_prefix_len = 7; /* ^^^^ must be 1 + strlen(whclob_zheader_prefix)! */
/* ! whclob_zheader_lensize == number of bytes used to store size in compressed data */
static short const whclob_zheader_lensize = 4;
/* ! whclob_zheader_crcsize == number of bytes used to store size in compressed data */
static short const whclob_zheader_crcsize = 4;
static int const whclob_zheader_size = 15; /* 1 + prefix_len + lensize + whclob_zheader_crcsize */

/**
   Stamps the initial header in cIn, which must be an initialized clob
   with at least whclob_zheader_size bytes allocated. srcSize must be
   the decompressed size of data which will presumably be compressed
   into the cIn clob at some point and crc must be the adler32
   checksum of the uncompressed data. Returns whclob_zheader_size on
   success, or a negative number on error.
*/
static int whclob_write_zheader( whclob * cIn, unsigned int srcSize, unsigned long crc )
{
    if( whclob_size(cIn) < whclob_zheader_size )
    {
	return whclob_rc.RangeError;
    }
    whclob_writeat( cIn, 0, whclob_zheader_prefix, whclob_zheader_prefix_len );
    {
	int i = whclob_zheader_prefix_len - 1;
	unsigned char * oBuf = (unsigned char *)whclob_buffer(cIn);
	oBuf[i++] = whclob_zheader_version;
	oBuf[i++] = srcSize>>24 & 0xff;
	oBuf[i++] = srcSize>>16 & 0xff;
	oBuf[i++] = srcSize>>8 & 0xff;
	oBuf[i++] = srcSize & 0xff;

	oBuf[i++] = crc>>24 & 0xff;
	oBuf[i++] = crc>>16 & 0xff;
	oBuf[i++] = crc>>8 & 0xff;
	oBuf[i++] = crc & 0xff;
	/* MARKER; printf( "Writing zsize header at pos %d-%d: %u\n", whclob_zheader_prefix_len, i, srcSize ); */
	MARKER( "Writing zsize header: version=%c uSize=%u  adler32=%lx\n", whclob_zheader_version, srcSize, crc );
    }
    return whclob_zheader_size;
}

/**
Checks first bytes of cIn to see if this is data compressed
by this API. If it is, the decompressed size of the compressed
data is set in sz and the adler32 checksum of the uncompressed
data is written to crc. On success 0 or greater is returned, else
a negative value from whclob_rc is returned:

- whclob_rc.RangeError: cIn is too small to contain the header or
the header was written by a different API version.

- whclob_rc.ArgError: header seems to be invalid, or maybe not
produced by the same version of this API.

*/
static int whclob_confirm_zheader( whclob const * cIn, unsigned int * sz, unsigned long * crc )
{
    int i;
    char ver;
    if( whclob_size(cIn) < whclob_zheader_size )
    {
	return whclob_rc.RangeError;
    }
    unsigned char const * inBuf = (unsigned char *) whclob_bufferc(cIn);
    for( i = 0; i < (whclob_zheader_prefix_len-1); ++i, ++inBuf )
    {
	if( *inBuf != whclob_zheader_prefix[i] )
	{
	    return whclob_rc.ArgError;
	}
	/* MARKER( "confirm header: %c\n", *inBuf ); */
    }
    /* MARKER( "confirm header: %d\n", *inBuf ); */
    inBuf = (unsigned const char *) whclob_bufferc(cIn) + (whclob_zheader_prefix_len - 1);
    ver = *inBuf;
    if( ver != whclob_zheader_version )
    {
	return whclob_rc.RangeError;
    }
    ++inBuf;
    /* MARKER( "confirm header: %d\n", *inBuf ); */
    *sz = (inBuf[0]<<24) + (inBuf[1]<<16) + (inBuf[2]<<8) + inBuf[3];
    *crc = (inBuf[4]<<24) + (inBuf[5]<<16) + (inBuf[6]<<8) + inBuf[7];
    /* MARKER( "header says zsize == %u\n", *sz ); */
    /* MARKER( "header says adler32 == %lx\n", *crc ); */
    return whclob_rc.OK;
}


int whclob_compress( whclob * cIn, whclob * cOut )
{
    const unsigned int szIn = whclob_size(cIn);
    const unsigned int szOut = 13 + szIn + (szIn+999)/1000;
    unsigned long nOut;
    int rc;
    whclob * tmp;
    unsigned long adler;
    tmp = 0;
    if( cOut != cIn ) whclob_reset(cOut);
    nOut = (long int) szOut;
    rc = whclob_init( &tmp, 0, nOut+whclob_zheader_size );
    if( 0 != rc ) return rc;

    adler = adler32(0L, Z_NULL, 0);
    adler = adler32( adler, (const Byte *) whclob_bufferc(cIn), szIn );
    /* MARKER( "adler=%lx\n", adler ); */
    whclob_write_zheader( tmp, szIn, adler );
    rc = compress( (unsigned char *)(whclob_buffer(tmp) + whclob_zheader_size),
		   &nOut,
		   (unsigned char *)whclob_buffer(cIn),
		   szIn );
    if( Z_OK != rc )
    {
	whclob_finalize(tmp);
	return whclob_rc.IOError;
    }
    if( cOut == cIn )
    {
	whclob_reset(cOut);
    }

    rc = whclob_resize( tmp, nOut+whclob_zheader_size );
    if( rc < (nOut+whclob_zheader_size) )
    {
	whclob_finalize( tmp );
	return rc;
    }
    whclob_swap( cOut, tmp );
    whclob_finalize( tmp );
    return whclob_rc.OK;
}



int whclob_uncompress(whclob *pIn, whclob *pOut)
{
    unsigned int unzSize;
    unsigned int nIn = whclob_size(pIn);
    whclob * temp;
    int rc;
    unsigned long int nOut2;
    unsigned long adlerExp;
    unsigned long adlerGot;
    if( nIn<=4 )
    {
	/* MARKER; */
	return whclob_rc.Err;
    }
    if( pOut != pIn ) whclob_reset(pOut);

    rc = whclob_confirm_zheader( pIn, &unzSize, &adlerExp );
    /* MARKER( "zsize = %u\n", unzSize ); */
    /* MARKER( "zsize = %u, adlerExp=%lx\n", unzSize, adlerExp ); */
    if( whclob_rc.OK > rc)
    {
	/* MARKER; */
	return rc;
    }
    rc = whclob_init(&temp,0,unzSize+1);
    if( whclob_rc.OK != rc )
    {
	/* MARKER; */
	return rc;
    }

    {
	unsigned char *inBuf;
	inBuf = (unsigned char*) (whclob_buffer(pIn) + whclob_zheader_size);
	nOut2 = whclob_size( temp );
	/* MARKER( "nOut2 = %ld\n", nOut2 ); */
	/* MARKER( "input length = %u\n", nIn - whclob_zheader_size ); */
	/* MARKER( "zsize = %u\n", unzSize ); */
	/* MARKER( "zblob size = %u\n",nIn ); */
	rc = uncompress((unsigned char*)whclob_buffer(temp), &nOut2, 
			inBuf, nIn - whclob_zheader_size);
    }
    if( rc!=Z_OK )
    {
	/* MARKER; */
	whclob_finalize(temp);
	return whclob_rc.IOError;
    }
    adlerGot = adler32(0L,Z_NULL,0);
    adlerGot = adler32( adlerGot, (Bytef const *) whclob_bufferc(temp), nOut2 );
    if( adlerGot != adlerExp )
    {
	/* MARKER( "adler32 mismatch: %lx != %lx\n", adlerExp, adlerGot ); */
	return whclob_rc.RangeError;
    }

    rc = whclob_resize(temp, nOut2);
    if( rc < nOut2 )
    {
	/* MARKER; */
	whclob_finalize(temp);
	return rc;
    }
    whclob_swap( temp, pOut );
    whclob_finalize(temp);
    return whclob_rc.OK;
}

int whclob_deflate( whclob *cIn, whclob *cOut )
{
    const int kludge = 4;
    z_stream zS;
    unsigned int szIn = whclob_size(cIn);
    unsigned int szOut = kludge + 13 + szIn + (szIn+999)/1000;
    int rc;
    whclob * tmp;
    tmp = 0;
    zS.zalloc = Z_NULL;
    zS.zfree = Z_NULL;
    zS.opaque = Z_NULL;

    if( cOut != cIn ) whclob_reset(cOut);
    rc = whclob_init( &tmp, 0, szOut );
    if( 0 != rc )
    {
	return rc;
    }

    {
	unsigned char * cBuf;
	cBuf = (unsigned char *) whclob_buffer(tmp);
	cBuf[0] = szIn>>24 & 0xff;
	cBuf[1] = szIn>>16 & 0xff;
	cBuf[2] = szIn>>8 & 0xff;
	cBuf[3] = szIn & 0xff;

	zS.next_in = (Bytef*) whclob_buffer(cIn);
	zS.avail_in = whclob_size(cIn);
	zS.next_out = (Bytef*) (cBuf + kludge);
	zS.avail_out = szOut;
    }

    if( Z_OK != deflateInit( &zS, Z_DEFAULT_COMPRESSION ) )
    {
	whclob_finalize(tmp);
	return whclob_rc.IOError;
    }

    /* printf("sizes: in=%d out=%d\n", zS.avail_in, zS.avail_out ); */

    rc = deflate( &zS, Z_FINISH );
    szOut = szOut - zS.avail_out;
    deflateEnd( &zS );
    /* printf("deflate() rc == %d\n", rc ); */
    /* printf("sizes: in=%d out=%d\n", zS.avail_in, zS.avail_out ); */
    if( Z_STREAM_END != rc )
    {
	/* printf("deflate() rc == %d\n", rc ); */
	whclob_finalize(tmp);
	return whclob_rc.IOError;
    }
    if( cOut == cIn )
    {
	whclob_reset(cOut);
    }
    rc = whclob_resize( tmp, szOut );
    if( rc < (szOut) )
    {
	whclob_finalize( tmp );
	return rc;
    }
    whclob_swap( cOut, tmp );
    whclob_finalize( tmp );
    return whclob_rc.OK;
}


int whclob_inflate(whclob *cIn, whclob *cOut)
{
    unsigned int uSize;
    unsigned int nIn = whclob_size(cIn);
    whclob * temp;
    int rc;
    z_stream zS;
    const int kludge = 4;
    unsigned int infSize;
    zS.zalloc = Z_NULL;
    zS.zfree = Z_NULL;
    zS.opaque = Z_NULL;

    if( cOut != cIn ) whclob_reset(cOut);
    if( nIn<=kludge )
    {
	return whclob_rc.Err;
    }


    {
	unsigned char * cBuf = (unsigned char *) whclob_buffer(cIn);
	uSize = (cBuf[0]<<24) + (cBuf[1]<<16) + (cBuf[2]<<8) + cBuf[3];
    }
    rc = whclob_init( &temp, 0, uSize + (uSize * 10 / 100));
    if( whclob_rc.OK != rc )
    {
	return rc;
    }
    /* fprintf(stderr,"uSize = %u, whclob_capacity(temp) = %ld\n", uSize, whclob_capacity(temp) ); */
    zS.avail_out = whclob_capacity(temp);
    zS.next_out = (Bytef*) whclob_buffer(temp);
    zS.next_in = (Bytef*) (whclob_buffer(cIn) + kludge);
    zS.avail_in = whclob_size(cIn) - kludge;

    /* fprintf(stderr,"avail_in == %d avail_out == %d\n", zS.avail_in, zS.avail_out ); */

    rc = inflateInit( &zS );
    if( Z_OK != rc )
    {
	/* fprintf(stderr,"inflateInit() failed. rc == %d\n", rc ); */
	whclob_finalize(temp);
	return whclob_rc.IOError;
    }
    /** weird: the return from inflate() is always an error (normally
	Z_DATA_ERR) for me, but inflateEnd() succeeds and the data is
	correct.
    */
    rc = inflate( &zS, Z_FINISH );
    /* fprintf(stderr,"inflate() rc == %d\n", rc ); */
    infSize = whclob_capacity(temp) - zS.avail_out;
    /* fprintf(stderr,"post: infSize == %d uSize == %d\n", infSize, uSize ); */
    /* fprintf(stderr,"post: avail_in == %d avail_out == %d\n", zS.avail_in, zS.avail_out ); */
    rc = inflateEnd( &zS );
    if( rc != Z_OK ) /*  STREAM_END ) */
    {
	/* fprintf(stderr, "inflate() rc == %d\n",rc); */
	whclob_finalize(temp);
	return whclob_rc.IOError;
    }
    rc = whclob_resize(temp, infSize);
    if( rc < infSize )
    {
	whclob_finalize(temp);
	return rc;
    }
    if( cOut == cIn ) whclob_reset(cOut);
    whclob_swap( temp, cOut );
    whclob_finalize(temp);
    return whclob_rc.OK;
}
#endif /* WHCLOB_USE_ZLIB */

#if WHCLOB_USE_FILE
long whclob_exporter_FILE( void * arg, char const * data, long n )
{
	FILE * fp = (FILE *) arg;
	if( ! fp ) return whclob_rc.UnexpectedNull;
	return (n == (long)fwrite( data, 1, n, fp ))
		? n
		: whclob_rc.IOError;
}

long whclob_export_FILE( whclob const * cb, FILE * dest )
{
	return whclob_export( cb, dest, whclob_exporter_FILE );
}

long whclob_exporter_filename( void * arg, char const * data, long n )
{
	char const * fname = (char const *)arg;
	FILE * fp = 0;
	long ret = 0;
	if( ! fname ) return whclob_rc.UnexpectedNull;
	fp = fopen( fname, "wb" );
	if( ! fp ) return whclob_rc.IOError;
	ret = whclob_exporter_FILE( fp, data, n );
	fclose( fp );
	return ret;
}

long whclob_export_filename( whclob const * cb, char const * dest )
{
    return whclob_export( cb, (char *)dest, whclob_exporter_filename );
    /* ^^ i hate that cast, but it's the least evil option here. */
}
long whclob_importer_FILE( whclob * dest, void * arg )
{
	FILE * fp = (FILE *) arg;
	long oldUsed = whclob_size(dest);
	/*const long blocksize = 4096;*/
#define blocksize 4096
	long rdsz = 0;
	char * bcbuf[blocksize];
	int rc;
	if( ! fp ) return whclob_rc.ArgError;
	while( ! feof(fp) )
	{
		rdsz = fread( bcbuf, sizeof(char), blocksize, fp );
		if( rdsz == 0 ) break;
		rc = whclob_append( dest, (char const *)bcbuf, rdsz );
		if( rc < 0 )
		{
		    return rc;
		}
	}
#undef blocksize
	return whclob_size(dest) - oldUsed;
}

long whclob_importer_filename( whclob * dest, void * arg )
{
	char const * fname = (char const *)arg;
	FILE * fh = 0;
	long ret = 0;
	if( ! fname ) return whclob_rc.ArgError;
	fh = fopen( fname, "rb" );
	if( !fh )
	{
	    ret = whclob_rc.IOError;
	}
	else
	{
	    ret = whclob_import( dest, fh, whclob_importer_FILE );
	    fclose( fh );
	}
	return ret;
}

long whclob_import_FILE( whclob * dest, FILE * fp )
{
    return whclob_import( dest, fp, whclob_importer_FILE );
}

long whclob_import_filename( whclob * dest, char const * fn )
{
    return whclob_import( dest, (void*)fn, whclob_importer_filename );
    /* i HATE that cast, but we know the importer won't change fn. */
}
#endif /* WHCLOB_USE_FILE */

#if WHCLOB_USE_BASE64
#include "s11n.net/c11n/detail/b64/cencode.h"
#include "s11n.net/c11n/detail/b64/cdecode.h"
long whclob_base64_enc( whclob const *cIn, whclob *cOut )
{
    unsigned int szIn = whclob_size(cIn);
    unsigned int szOut = (unsigned int) ((szIn+1) * 1.4);
    long rc = 0;
    whclob * tmp = 0;
    if( ! cIn || !cOut || !szIn ) return whclob_rc.ArgError;
    if( szOut < (szIn-5) ) szOut = szIn + 5;
    if( cOut != cIn ) whclob_reset( cOut );
    rc = whclob_init( &tmp, 0, szOut );
    if( whclob_rc.OK != rc )
    {
	return rc;
    }
    base64_encodestate state;
    base64_init_encodestate( &state );
    char * outBuf = whclob_buffer(tmp);
    rc = base64_encode_block( whclob_bufferc(cIn), whclob_size(cIn), outBuf, &state );
    //MARKER("enc inSize=%ld, rc=%ld\n",whclob_size(cIn), rc);
    rc += base64_encode_blockend( outBuf+rc, &state);
    //MARKER("enc rc=%ld\n",rc);
    whclob_resize( tmp, rc );
    whclob_swap( cOut, tmp );
    whclob_finalize( tmp );
    return whclob_rc.OK;
}

long whclob_base64_dec( whclob const *cIn, whclob *cOut )
{
    unsigned int szIn = whclob_size(cIn);
    unsigned int szOut = szIn;
    long rc = 0;
    whclob * tmp = 0;
    if( ! cIn || !cOut || !szIn ) return whclob_rc.ArgError;
    if( cOut != cIn ) whclob_reset( cOut );
    rc = whclob_init( &tmp, 0, szOut );
    if( whclob_rc.OK != rc )
    {
	return rc;
    }
    base64_decodestate state;
    base64_init_decodestate( &state );
    rc = base64_decode_block( whclob_bufferc(cIn), whclob_size(cIn), whclob_buffer(tmp), &state );
    whclob_resize( tmp, rc );
    //MARKER("dec inSize=%ld, outSize=%ld, rc=%ld\n",whclob_size(cIn), whclob_size(tmp), rc);
    whclob_swap( cOut, tmp );
    whclob_finalize( tmp );
    return whclob_rc.OK;
}
#endif /* WHCLOB_USE_BASE64 */

#undef WHCLOB_DEBUG
