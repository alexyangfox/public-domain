#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined(__cplusplus)
extern "C" {
#  include <cassert>
#  define ARG_UNUSED(X)
#else
#  include <assert.h>
#  define ARG_UNUSED(X) X
#endif /* __cplusplus */

#if 1
#define MARKER printf("MARKER: %s:%d:%s():\n",__FILE__,__LINE__,__func__);
#else
#define MARKER printf("MARKER: %s:%d:\n",__FILE__,__LINE__);
#endif

#define DUMPPOS(P) MARKER; printf("pos = [%s]\n", pegc_eof(P) ? "<EOF>" : pegc_latin1(*pegc_pos(P)) );
#include "pegc.h"
#include "whclob.h"
#include "whgc.h"


const pegc_cursor pegc_cursor_init = PEGC_CURSOR_INIT;
const PegcRule PegcRule_init = PEGCRULE_INIT;
const PegcRule PegcRule_invalid = PEGCRULE_INIT;

size_t pegc_strnlen( size_t n, pegc_const_iterator c )
{
    size_t ret = 0;
    while( c && *c )
    {
	if( (n>0) && (ret == n) ) break;
	++ret;
	++c;
    }
    return ret;
}

size_t pegc_strlen( pegc_const_iterator c )
{
    return pegc_strnlen( 0, c );
}

/**
   Internal holder for "match listener" data. Match listeners get
   notified any time pegc_set_match() is set.
*/
struct pegc_match_listener_data
{
    pegc_match_listener func;
    void * data;
    struct pegc_match_listener_data * next;
};
typedef struct pegc_match_listener_data pegc_match_listener_data;
/**
   Empty initializer for pegc_match_listener_data.
*/
const static pegc_match_listener_data pegc_match_listener_data_init = {0,0,0};



/**
   PegcAction is a type for implementing delayed actions. That is, actions
   which are queued when a rule matches, but not executed until the user
   specifies (i.e. after a successfull parse).
   
   PegcActions should not be instantiated directly by client code. Use
   pegc_r_action_d() to create 
*/
struct PegcAction
{
    /**
       Implementation function for this object.
    */
    pegc_action_f action;
    /**
       Arbitrary client data, to be passed as the 3rd argument to the action.
    */
    void * data;
    /**
       The range of the matching characters, as determined by the rule associated
       with this action.
    */
    pegc_cursor match;
};
typedef struct PegcAction PegcAction;
    
//#define PEGCACTION_INIT {0,0,PEGC_CURSOR_INIT} /* compile error? */
#define PEGCACTION_INIT {0,0,PEGC_CURSOR_INIT}
static const PegcAction PegcAction_init = PEGCACTION_INIT;

/**
   Internal type to hold a linked list of queues actions.
*/
struct pegc_action
{
    /**
       Previous item in the list. Used for destruction traversal.
    */
    struct pegc_action * left;
    /**
       Next item in the list. Used for action trigger traversal.
    */
    struct pegc_action * right;
    /**
       This object's action.
    */
    PegcAction action;
};
typedef struct pegc_action pegc_action;
#define PEGC_ACTION_INIT {0,0,PEGCACTION_INIT}
static const pegc_action pegc_action_init = PEGC_ACTION_INIT;



#define PEGC_STATS_INIT {\
    0, /* gc_count */	\
    0, /* alloced */			\
    0 /* gc_internals_alloced */	\
}
static const pegc_stats pegc_stats_init = PEGC_STATS_INIT;

/**
   The main parser state type. It is 100% opaque - never rely on any
   internals of this type.
*/
struct pegc_parser
{
    char const * name; /* for debugging + error reporting purposes */
    pegc_cursor cursor;
    /**
       Set via pegc_set_match().
    */
    pegc_cursor match;
    /**
       Match listener list. This might be removed, as match listeners
       aren't terribly useful.
    */
    pegc_match_listener_data * listeners;
    /**
       Queued actions are stored here. This item is the right-most
       action in the list.
    */
    pegc_action * actions;
    /**
       Generic garbage collector.
    */
    whgc_context * gc;
    /**
       Holds error reporting info.

       To consider: use a whclob here and append
       all errors added via pegc_set_error_e()
       into one report string.
    */
    struct errinfo {
	char * message;
	size_t line;
	size_t col;
    } errinfo;
    pegc_stats stats;
};

static const pegc_parser
pegc_parser_init = { 0, /* name */
		    {0,0,0}, /* cursor */
		    {0,0,0}, /* match */
		     0, /* listeners */
		     0, /* actions */
		     0, /* gc */
		     {/* errinfo */
		     0, /* message */
		     0, /* line */
		     0 /* col */
		     },
		     PEGC_STATS_INIT
};

void pegc_add_match_listener( pegc_parser * st,
			      pegc_match_listener f,
			      void * cdata )
{
    if( ! st || !f ) return;
    pegc_match_listener_data * d = (pegc_match_listener_data*) malloc(sizeof(pegc_match_listener_data));
    if( ! d ) return;
    st->stats.alloced += sizeof(pegc_match_listener_data);
    d->func = f;
    d->data = cdata;
    pegc_match_listener_data * x = st->listeners;
    if( x )
    {
	while( x->next ) x = x->next;
	x->next = d;
    }
    else
    {
	st->listeners = d;
    }
}

void pegc_set_name( pegc_parser * st, char const * name )
{
    if( st )
    {
	st->name = name;
    }
}

char const * pegc_get_name( pegc_parser * st )
{
    return st ? st->name : 0;
}

void pegc_free( void * k )
{
    //MARKER; printf("Freeing GENERIC (void*) @%p\n",k);
    free(k);
}

static void pegc_free_key( void * k )
{
    //MARKER; printf("Freeing KEY (void*) @%p\n",k);
    free(k);
}

static void pegc_free_value( void * k )
{
    //MARKER; printf("Freeing VALUE (void*) @%p\n",k);
    free(k);
}

void pegc_gc_test_listener( whgc_event const * ev )
{
#if 0
    //MARKER; printf("GC event: cx=@%p event=%d key=%p value=@%p\n",ev->cx,ev->type,ev->key,ev->value);
    //if( whgc_event_destructing_context == ev->type )
    if( whgc_events.destructing_context == ev->type )
    {
	whgc_stats const st = whgc_get_stats( ev->cx );
	MARKER;printf("Approx memory allocated by gc context: %u\n", st.alloced);
	printf("GC entry/add/take count: %u/%u/%u\n", st.entry_count, st.add_count, st.take_count);
	pegc_parser const * p= (pegc_parser const*)whgc_get_context_client(ev->cx);
	pegc_stats const pst = pegc_get_stats( p );
	printf("APPROXIMATE allocated memory: parser[@%p]=%u gc[@%p]=%u\n", p, pst.alloced, ev->cx, pst.gc_internals_alloced);
    }
#endif
}

bool pegc_gc_register( pegc_parser * st,
		       void * key, void (*keyDtor)(void*),
		       void * value, void (*valDtor)(void*) )
{
    if( ! st || !key ) return false;
    if( ! st->gc )
    {
        st->gc = whgc_create_context(st);
	if( ! st->gc ) return false;
	//whgc_add_listener( st->gc, pegc_gc_test_listener );
    }
    if( ! whgc_register( st->gc, key, keyDtor, value, valDtor ) )
    {
	return false;
    }
    return true;
}

void * pegc_gc_search( pegc_parser const * st, void const * key )
{
    if( !st || !key || !st->gc ) return 0;
    return whgc_search( st->gc, key );
}

/**
   Adds item to the general-purposes garbage pool, to be cleaned
   up using free() when pegc_destroy_parser(st) is called. The
   item is stored in a hashtable and is used as its own
   key. Calling free(item) must result in defined behaviour
   or... well, results are undefined, obviously.
*/
bool pegc_gc_add( pegc_parser * st, void * item, void (*dtor)(void*) )
{
    return pegc_gc_register( st, item, dtor, item, 0 );
}

/**
   Holds internal data for pegc actions.
*/
struct pegc_action_info
{
    pegc_action_f action;
    void * data;
};
typedef struct pegc_action_info pegc_action_info;


bool pegc_init_cursor( pegc_cursor * it, pegc_const_iterator begin, pegc_const_iterator end )
{
    if( begin && (0 == end) ) end = (begin + pegc_strlen(begin));
    if( !it || (end < begin) ) return false;
    it->begin = it->pos = begin;
    it->end = end;
    return true;
}

bool pegc_set_input( pegc_parser * st, pegc_const_iterator begin, long length )
{
    return pegc_set_error_e( st, 0, 0 )
	&& pegc_init_cursor( &st->cursor, begin,
			     (length < 0)
			     ? (pegc_const_iterator)0
			     : (begin + length) );
}

pegc_parser * pegc_create_parser( char const * inp, long len )
{
    pegc_parser * p = (pegc_parser*)malloc( sizeof(pegc_parser) );
    if( ! p ) return 0;
    *p = pegc_parser_init;
    p->stats.alloced = sizeof(pegc_parser);
    pegc_set_input( p, inp, len );
    return p;
}

void pegc_clear_actions( pegc_parser * st )
{
    if( ! st || ! st->actions ) return;
    if( st->actions->right )
    {
	/* Shouldn't happen. This would be indicative of mismanagement elsewhere. */
	while( st->actions->right ) st->actions = st->actions->right;
    }
    while( st->actions )
    {
	pegc_action * p = st->actions->left;
	//MARKER;printf("Freeing queued action entry @%p\n",st->actions);
	pegc_free(st->actions);
	st->stats.alloced -= sizeof(pegc_action);
	st->actions = p;
    }
}

bool pegc_destroy_parser( pegc_parser * st )
{
    if( ! st ) return false;
    pegc_set_error_e( st, 0, 0 );
    pegc_clear_actions( st );
    if( st->gc )
    {
        whgc_destroy_context( st->gc );
        st->gc = 0;
    }
    pegc_match_listener_data * x = st->listeners;
    while( x )
    {
	pegc_match_listener_data * X = x->next;
	pegc_free(x);
	st->stats.alloced -= sizeof(pegc_match_listener_data);
	x = X;
    }
    pegc_free( st );
    return true;
}


bool pegc_parse( pegc_parser * st, PegcRule const * r )
{
    return ( !st || !r || !r->rule )
	? false
	: r->rule( r, st );
}

pegc_const_iterator pegc_latin1(int ch)
{
    static char latin1[256 * 2];
    static bool inited = false;
    if( ! inited )
    {
	inited = true;
	int i = 0;
	int ndx = 0;
	for( ; i <= 255; ++i, ndx += 2 )
	{
	    latin1[ndx] = (char)i;
	    latin1[ndx+1] = '\0';
	}
    }
    char const * r = ( (ch < 0) || (ch>255) )
	? (pegc_const_iterator)0
	: (latin1 + (2*ch));
    return r;
}


char const * pegc_get_error( pegc_parser const * st,
			     size_t * line,
			     size_t * col )
{
    if( ! st || !st->errinfo.message ) return 0;
    if( line ) *line = st->errinfo.line;
    if( col ) *col = st->errinfo.col;
    return st->errinfo.message;
}

bool pegc_set_error_v( pegc_parser * st, char const * fmt, va_list vargs )
{
    if( ! st ) return false;
    if( st->errinfo.message )
    {
	pegc_free(st->errinfo.message);
    }
    st->errinfo.message = 0;
    st->errinfo.line = st->errinfo.col = 0;
    if( ! fmt ) return true;
    char const * at = fmt;
    for( ; at && *at; ++at ){};
    size_t len = at - fmt;
    if( ! len )
    {
	st->errinfo.message = 0;
	st->errinfo.line = 0;
	st->errinfo.col = 0;
	return true;
    }
    else
    {
	pegc_line_col( st, &(st->errinfo.line), &(st->errinfo.col) );
	whclob * cb = whclob_new();
	whclob_appendf(cb,"pegc_set_error_v(): near line %u, col %u\n",st->errinfo.line,st->errinfo.col);
	whclob_vappendf(cb,fmt,vargs);
	st->errinfo.message = whclob_take_buffer(cb);
	whclob_finalize(cb);
	if( ! st->errinfo.message ) return false;
    }
    return true;
}

bool pegc_set_error_e( pegc_parser * st, char const * fmt, ... )
{
    va_list vargs;
    va_start( vargs, fmt );
    bool ret = pegc_set_error_v(st, fmt, vargs );
    va_end(vargs);
    return ret;
}

void pegc_set_client_data( pegc_parser * st, void * data )
{
    pegc_gc_register( st, (void *)pegc_set_client_data, 0, data, 0 );
}

void * pegc_get_client_data( pegc_parser const * st )
{
    return pegc_gc_search( st, (void const *)pegc_set_client_data );
}

bool pegc_eof( pegc_parser const * st )
{
    return !st
	    || !st->cursor.pos
	    || !*(st->cursor.pos)
	    || (st->cursor.pos >= st->cursor.end)
	    ;
}

bool pegc_has_error( pegc_parser const * st )
{
    return st && (st->errinfo.message);
}
bool pegc_isgood( pegc_parser const * st )
{
    return st && !pegc_eof(st) && ! pegc_has_error(st);
}
pegc_cursor const * pegc_iter( pegc_parser const * st )
{
    return st ? &st->cursor : 0;
}

pegc_const_iterator pegc_begin( pegc_parser const * st )
{
    return st ? st->cursor.begin : 0;
}

pegc_const_iterator pegc_end( pegc_parser const * st )
{
    return st ? st->cursor.end : 0;
}


bool pegc_in_bounds( pegc_parser const * st, pegc_const_iterator p )
{
    return st && p && *p && (p>=pegc_begin(st)) && (p<pegc_end(st));
}


pegc_const_iterator pegc_pos( pegc_parser const * st )
{
    return st ? st->cursor.pos : 0;
}

bool pegc_set_pos( pegc_parser * st, pegc_const_iterator p )
{
    if( ! st || !p ) return false;
    if( pegc_in_bounds(st,p) || (p == pegc_end(st)) )
    {
	st->cursor.pos = p;
    }
    //MARKER; printf("pos=%p, p=%p, char=%c\n", pegc_iter(st)->pos, p, (p&&*p) ? *p : '!' );
    return st->cursor.pos == p;
}


bool pegc_advance( pegc_parser * st, int n )
{
    return ( 0 == n )
	? false
	: (st ? pegc_set_pos( st, pegc_pos(st) + n ) : false);
}

bool pegc_bump( pegc_parser * st )
{
    return st ? pegc_advance(st, 1) : false;
}

long pegc_distance( pegc_parser const * st, pegc_const_iterator e )
{
    return (st&&e) ? (e - pegc_pos(st)) : 0;
}

bool pegc_line_col( pegc_parser const * st,
		    size_t * line,
		    size_t * col )
{
    if( !st ) return false;
    size_t bogo;
    if( ! line ) line = &bogo;
    if( ! col ) col = &bogo;
    *line = 1;
    *col = 0;
    pegc_const_iterator pos = pegc_pos(st);
    pegc_const_iterator beg = pegc_begin(st);
    for( ; beg && *beg && (beg != pos ); ++beg )
    {
	if( *beg == '\n' )
	{
	     ++(*line);
	     *col = 0;
	}
	else
	{
	    ++(*col);
	}
    }
    return true;
}

pegc_cursor pegc_get_match_cursor( pegc_parser const * st )
{
    pegc_cursor cur = pegc_cursor_init;
    if( st )
    {
	cur.pos = cur.begin = st->match.begin;
	cur.end = st->match.end;
    }
    return cur;
}

pegc_cursor pegc_cursor_trimmed( pegc_cursor const cur )
{
    pegc_cursor r = cur;
    pegc_const_iterator c = r.begin;
    while( c && (c<r.end) && *c && isspace(*c) ) ++c;
    pegc_const_iterator mark = c;
    if( c == r.end ) return r;
    c = r.end-1;
    if( c <= r.begin ) return r;
    while( c && (c>mark) && *c && isspace(*c) ) --c;
    r.begin = r.pos = mark;
    r.end = c;
    return r;
}

pegc_iterator pegc_cursor_tostring( pegc_cursor const cur )
{
    if( !cur.begin
	||!*(cur.begin)
	||(cur.end<=cur.begin)
	)
    {
	return 0;
    }
    long sz = cur.end - cur.begin;
    if( sz <= 0 ) return 0;
    pegc_iterator ret = (pegc_iterator)calloc( sz + 1, sizeof(pegc_char_t) );
    if( ! ret ) return 0;
    pegc_const_iterator it = cur.begin;
    pegc_iterator at = ret;
    for( ; it && *it && (it != cur.end); ++it, ++at )
    {
	*at = *it;
    }
    *at = '\0';
    return ret;
}

pegc_iterator pegc_get_match_string( pegc_parser const * st )
{
    return pegc_cursor_tostring( pegc_get_match_cursor(st) );
}

bool pegc_set_match( pegc_parser * st, pegc_const_iterator begin, pegc_const_iterator end, bool movePos )
{
    if( !st
	|| (! pegc_in_bounds( st, begin ))
	|| (pegc_end(st) < end) )
    {
	/**
	   Is this worth setting an error for?
	*/
#if 0
	MARKER; fprintf(stderr,"WARNING: pegc_set_match() is out of bounds.\n");
#else
	pegc_set_error_e( st, "pegc_set_match(parser=[%p],begin=[%p],end=[%p],%d) is out of bounds",
			st, begin, end, movePos );
#endif
	return false;
    }
    //MARKER;printf("pegc_setting_match() setting match of %d characters.\n",(end-begin));
    st->match.pos = st->match.begin = begin;
    st->match.end = end;
    if( movePos )
    {
	pegc_set_pos( st, end );
    }

    pegc_match_listener_data * x = st->listeners;
    while( x )
    {
	x->func( st, x->data );
	x = x->next;
    }
    return true;
}

bool pegc_matches_char( pegc_parser const * st, int ch )
{
    return st
	? (pegc_eof(st) ? (ch == '\0') : (*pegc_pos(st) == ch))
	: false;
}

bool pegc_matches_chari( pegc_parser const * st, int ch )
{
    if( !st || pegc_has_error(st) ) return false;
    pegc_const_iterator p = pegc_pos(st);
    if( !p && !*p ) return false;
    return (p && *p)
	&& (tolower(*p) == tolower(ch));
}

bool pegc_matches_string( pegc_parser const * st, pegc_const_iterator str, long strLen, bool caseSensitive )
{
    if( !st || pegc_has_error(st) ) return false;
    if( strLen < 0 ) strLen = pegc_strlen(str);
    if( ! pegc_in_bounds(st, pegc_pos(st)+strLen-1) ) return false;
    pegc_const_iterator orig = pegc_pos(st);
    pegc_const_iterator p = orig;
    pegc_const_iterator sp = str;
    long i = 0;
    for( ; p && *p && (i < strLen); ++i, ++p, ++sp )
    {
	if( caseSensitive )
	{
	    if( tolower(*p) != tolower(*sp) ) break;
	}
	else
	{
	    if( *p != *sp ) break;
	}
    }
    //MARKER; printf("matched string? == %d, i=%ld, strLen=%ld, str=[%s]\n", (i == strLen), i, strLen, str);
    return ( i == strLen ) ? true : false;
}


PegcRule pegc_r( PegcRule_mf rule, void const * data )
{
    PegcRule r = PEGCRULE_INIT2(rule,data);
    return r;
}

static bool pegc_rule_check( PegcRule const * r,
			     pegc_parser const * st,
			     bool requireData,
			     bool requireProxy,
			     bool allowEOF )
{
    if( ! pegc_isgood(st) || !r ) return false;
    if( pegc_has_error( st ) ) return false;
    if( requireData && ! r->data ) return false;
    if( requireProxy && ! r->proxy ) return false;
    if( (!allowEOF) && pegc_eof(st) ) return false;
    return true;
}

static bool PegcRule_mf_has_error( PegcRule const * ARG_UNUSED(self), pegc_parser * st )
{
    return pegc_has_error(st);
}
const PegcRule PegcRule_has_error = PEGCRULE_INIT1(PegcRule_mf_has_error);

static bool PegcRule_mf_char_range( PegcRule const * self, pegc_parser * st )
{
    //MARKER;printf("self=%p, self->data=%p\n",self,self->data);
    if( ! pegc_rule_check( self, st, true, false, false ) ) return false;
    if( !pegc_isgood( st ) ) return false;
    size_t evil = (size_t)self->data;
    int min = ((evil >> 8) & 0x00ff);
    int max = (evil & 0x00ff);
    //MARKER;printf("min=%c, max=%c, evil=%x\n",min,max,evil);
    pegc_const_iterator orig = pegc_pos(st);
    if( (*orig >= min) && (*orig <= max) )
    {
	//MARKER;printf("matched: ch=%c, min=%c, max=%c, evil=%x\n",*orig?*orig:'!',min,max,evil);
	pegc_set_match(st,orig,orig+1,true);
	return true;
    }
    return false;
}

PegcRule pegc_r_char_range( pegc_char_t start, pegc_char_t end )
{
    if( start > end )
    {
	pegc_char_t x = end;
	start = end;
	end = x;
    }
    /**
       i'm not proud of this, but i want to avoid allocating
       for this rule, since i expect it to be used often.
     */
    assert( (sizeof(size_t) <= sizeof(void*)) && "pegc_r_char_range(): invalid use of (void*) to store int value: (void*) is too small!");
    size_t evil = ((start << 8) | (0x00ff & end));
    PegcRule r = pegc_r( PegcRule_mf_char_range, (void*)evil );
    //MARKER;printf("min=%c, max=%c, evil=%x, r.data=%p\n",start,end,evil,r.data);
    return r;
}

static bool PegcRule_mf_char_spec( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, true, false, false ) ) return false;
    if( ! pegc_isgood( st ) ) return false;
    char const * spec = (char const *) self->data;
    if( ! spec )
    {
	pegc_set_error_e(st,"PegcRule_mf_char_spec: internal char spec data not found.");
	return false;
    }
    pegc_const_iterator orig = pegc_pos(st);
    //MARKER;printf("inChar=%c format=%s strlen==%d\n",*orig,spec,pegc_strlen(spec));
    char ch[] = {0,0};
    int rc = sscanf(pegc_pos(st), spec, ch);
    //MARKER;printf("inChar=%c sscanf rc=%d, ch=%s\n",*orig,rc,ch);
    if( (EOF == rc) || (0 == rc) ) return false;
    pegc_set_match( st, orig, orig + 1, true );
    return true;
}

PegcRule pegc_r_char_spec( pegc_parser * st, char const * spec )
{
    if( ! st || !spec || (*spec != '[') ) return PegcRule_invalid;
    char * fmt = pegc_mprintf( st, "%%1%s", spec );
    if( ! fmt ) return PegcRule_invalid;
    return pegc_r(PegcRule_mf_char_spec, fmt);
}

static bool PegcRule_mf_error( PegcRule const * self, pegc_parser * st )
{
    //if( ! pegc_rule_check( self, st, false, false, true ) ) return false;
    if( ! self || !self->data || !st ) return false;
    char const * msg = self->data ? (char const *)self->data : "unspecified error";
    pegc_set_error_e( st, "%s", msg );
    return false;
}

PegcRule pegc_r_error_v( pegc_parser * st, char const * fmt, va_list vargs )
{
    if( ! st || !fmt ) return PegcRule_invalid;
    char * str = pegc_vmprintf( st, fmt, vargs );
    if( ! str ) return PegcRule_invalid;
    PegcRule r = pegc_r( PegcRule_mf_error, str );
    return r;
}

PegcRule pegc_r_error_e( pegc_parser * st, char const * fmt, ... )
{
    va_list vargs;
    va_start( vargs, fmt );
    PegcRule ret = pegc_r_error_v( st, fmt, vargs );
    va_end(vargs);
    return ret;
}

PegcRule pegc_r_error( char const * errstr  )
{
    return pegc_r( PegcRule_mf_error, errstr );
}


void pegc_clear_match( pegc_parser * st )
{
    if( st ) pegc_set_match(st, 0, 0, false);
}


bool pegc_is_rule_valid( PegcRule const * r )
{
    return r && r->rule;
}




PegcRule * pegc_alloc_r( pegc_parser * st, PegcRule_mf const func, void const * data )
{
    PegcRule * r = (PegcRule*) malloc(sizeof(PegcRule));
    if( ! r ) return 0;
    *r = pegc_r(func,data);
    if( st )
    {
	st->stats.alloced += sizeof(PegcRule);
	pegc_gc_add( st, r, pegc_free_key );
    }
    return r;
}

PegcRule * pegc_copy_r_p( pegc_parser * st, PegcRule const * src )
{
    PegcRule * r = src ? pegc_alloc_r( st, 0, 0 ) : 0;
    if( r ) *r = *src;
    return r;
}

PegcRule * pegc_copy_r_v( pegc_parser * st, PegcRule const src )
{
    //return pegc_copy_r_p(st, &src);
    return pegc_copy_r_p(st, &src);
}

char * pegc_vmprintf( pegc_parser * st, char const * fmt, va_list args )
{
    char * ret = (fmt && *fmt) ?
	whclob_vmprintf( fmt, args )
	: 0;
    if( st && ret )
    {
	pegc_gc_add( st, ret, pegc_free );
    }
    return ret;
}
char * pegc_mprintf( pegc_parser * st, char const * fmt, ... )
{
    va_list vargs;
    va_start( vargs, fmt );
    char * ret = pegc_vmprintf( st, fmt, vargs );
    va_end(vargs);
    return ret;
}

bool PegcRule_mf_failure( PegcRule const * self, pegc_parser * st )
{
    return false;
}
const PegcRule PegcRule_failure = PEGCRULE_INIT3(PegcRule_mf_failure,0,"false");
static bool PegcRule_mf_success( PegcRule const * self, pegc_parser * st )
{
    return true;
}
const PegcRule PegcRule_success = PEGCRULE_INIT3(PegcRule_mf_success,0,"true");


/**
   Internal implementation of PegcRule_mf_oneof[i]().
   Requires that self->data be a pegc_const_iterator. 
*/
static bool PegcRule_mf_oneof_impl( PegcRule const * self, pegc_parser * st, bool caseSensitive )
{
    if( ! pegc_rule_check( self, st, true, false, false ) ) return false;
    if( ! pegc_isgood(st) ) return false;
    pegc_const_iterator p = pegc_pos(st);
    if( !*p ) return false;
    pegc_const_iterator str = (pegc_const_iterator)self->data;
    size_t len = pegc_strlen(str);
    int i = 0;
    for( ; (i < len); ++i )
    {
	if( caseSensitive
	    ? (*p == str[i])
	    : (tolower(*p) == tolower(str[i])) )
	{
	    //MARKER;
	    pegc_set_match( st, p, p+1, true );
	    return true;
	}
    }
    return false;

}


bool PegcRule_mf_oneof( PegcRule const * self, pegc_parser * st )
{
    return PegcRule_mf_oneof_impl(self,st,true);
}

bool PegcRule_mf_oneofi( PegcRule const * self, pegc_parser * st )
{
    return PegcRule_mf_oneof_impl(self,st,false);
}

PegcRule pegc_r_oneof( char const * list, bool caseSensitive )
{
    return pegc_r( caseSensitive
		  ? PegcRule_mf_oneof
		  : PegcRule_mf_oneofi,
		  list );
}


typedef struct PegcRule_mf_string_params PegcRule_mf_string_params;
static bool PegcRule_mf_string_impl( PegcRule const * self, pegc_parser * st, bool caseSensitive )
{
    if( ! pegc_rule_check( self, st, true, false, false ) ) return false;
    pegc_const_iterator str = (pegc_const_iterator)self->data;
    if( ! str ) return false;
    size_t len = pegc_strlen(str);
    pegc_const_iterator p = pegc_pos(st);
    bool b = pegc_matches_string( st, str, len, caseSensitive );
    //MARKER; printf("matches? == %d\n", b);
    if( b )
    {
	pegc_set_match( st, p, p+len, true );
    }
    return b;
}

bool PegcRule_mf_string( PegcRule const * self, pegc_parser * st )
{
    return PegcRule_mf_string_impl( self, st, true );
}

bool PegcRule_mf_stringi( PegcRule const * self, pegc_parser * st )
{
    return PegcRule_mf_string_impl( self, st, false );
}

/**
   Internal implementation for PegcRule_mf_char and PegcRule_mf_chari.
*/
static bool PegcRule_mf_char_impl( PegcRule const * self, pegc_parser * st, bool caseSensitive )
{
    if( ! pegc_rule_check( self, st, true, false, true ) ) return false;
    pegc_const_iterator orig = pegc_pos(st);
    char sd = *((char const *)(self->data));
    //MARKER; printf("trying to match: [%c] =? [%c] data=[%p]\n", sd ? sd : '!', *orig, self->data );
    if( caseSensitive
	? pegc_matches_char(st,sd)
	: pegc_matches_chari(st,sd) )
    {
	pegc_set_match( st, orig, orig + 1, true );
	return true;
    }
    return false;
}

bool PegcRule_mf_char( PegcRule const * self, pegc_parser * st )
{
    return PegcRule_mf_char_impl(self,st,true);
}

bool PegcRule_mf_notchar( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, true, false, true ) ) return false;
    char const * d = (char const *)self->data;
    pegc_const_iterator orig = pegc_pos(st);
    if( *orig == *d ) return false;
    pegc_set_match( st, orig, orig+1, true );
    return true;
}
bool PegcRule_mf_notchari( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, true, false, true ) ) return false;
    char const * d = (char const *)self->data;
    pegc_const_iterator orig = pegc_pos(st);
    if( tolower(*orig) == tolower(*d) ) return false;
    pegc_set_match( st, orig, orig+1, true );
    return true;
}

PegcRule pegc_r_notchar( pegc_char_t input, bool caseSensitive )
{
    return pegc_r( caseSensitive
		   ? PegcRule_mf_notchar
		   : PegcRule_mf_notchari,
		   pegc_latin1(input));
}


bool PegcRule_mf_chari( PegcRule const * self, pegc_parser * st )
{
    return PegcRule_mf_char_impl(self,st,false);
}

/**
   This rule acts like a the regular expression (Rule)*. Always
   matches but may or may not consume input.

   Requires that self->proxy be set to an object this routine can
   use as a proxy rule.
*/
static bool PegcRule_mf_star( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, false, true, true ) ) return false;
    size_t matches = 0;
    pegc_const_iterator orig = pegc_pos(st);
    pegc_const_iterator p2 = orig;
    do
    {
	if( self->proxy->rule( self->proxy, st ) )
	{
	    ++matches;
	    if( p2 == pegc_pos(st) )
	    { // avoid endless loop on non-consuming rules
		break;
	    }
	    p2 = pegc_pos(st);
	    continue;
	}
	break;
    } while( 1 );
    if( matches > 0 )
    {
	pegc_set_match( st, orig, p2, true );
    }
    return true;
}

/**
   Requires that self->proxy be set to an object this routine can
   use as a proxy rule.

   Works like PegcRule_mf_star(), but matches 1 or more times.
   This routine is "greedy", matching as long as it can UNLESS
   self->data (the rule) does not consume input, in which case
   this routine stops at the first match to avoid an endless loop.
*/
static bool PegcRule_mf_plus( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, false, true, true ) ) return false;
    pegc_const_iterator orig = pegc_pos(st);
    int matches = self->proxy->rule( self->proxy, st )
	? 1 : 0;
    pegc_const_iterator p2 = pegc_pos(st);
    while( (matches>0)
	   && (p2 != orig)
	   && self->proxy->rule( self->proxy, st )
	   )
    {
	++matches;
	if( p2 == pegc_pos(st) ) break; // didn't consume
	p2 = pegc_pos(st);
    }
    if( matches > 0 )
    {
	//MARKER; printf("plus got %d matches\n", matches );
	pegc_set_match( st, orig, p2, true );
	return true;
    }
    pegc_set_pos(st,orig);
    return false;
}

static bool PegcRule_mf_at( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, false, true, true ) ) return false;
    pegc_const_iterator orig = pegc_pos(st);
    bool rc = self->proxy->rule( self->proxy, st );
    pegc_set_pos(st,orig);
    return rc;
}

PegcRule pegc_r_at_p( PegcRule const * proxy )
{
    if( ! pegc_is_rule_valid(proxy) ) return PegcRule_invalid;
    PegcRule r = pegc_r( PegcRule_mf_at, 0 );
    r.proxy = proxy; //pegc_copy_r_v(st, proxy);
    return r;
}
PegcRule pegc_r_at_v( pegc_parser * st, PegcRule const proxy )
{
    return pegc_r_at_p( pegc_copy_r_v(st,proxy) );
}
static bool PegcRule_mf_notat( PegcRule const * self, pegc_parser * st )
{
    return pegc_rule_check( self, st, false, false, true )
	&& ! PegcRule_mf_at(self,st);
}

//PegcRule pegc_r_notat( pegc_parser * st, PegcRule const proxy )
PegcRule pegc_r_notat_p( PegcRule const * proxy )
{
    if( ! pegc_is_rule_valid(proxy) ) return PegcRule_invalid;
    PegcRule r = PEGCRULE_INIT1(PegcRule_mf_notat);
    r.proxy = proxy; // pegc_copy_r_v( st, proxy );
    return r;
}

PegcRule pegc_r_notat_v( pegc_parser * st, PegcRule const proxy )
{
    return pegc_r_notat_p( pegc_copy_r_v(st,proxy) );
}


static bool PegcRule_mf_or( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, true, false, true ) ) return false;
    pegc_const_iterator orig = pegc_pos(st);
#if 0
    PegcRule const ** li = (PegcRule const **)self->data;
    for( ; li && *li && (*li)->rule; ++li )
    {
	//MARKER;
	if( (*li)->rule( *li, st ) )
	{
	    pegc_set_match( st, orig, pegc_pos(st), true );
	    return true;
	}
    }
#else
    PegcRule const * li = (PegcRule const *)self->data;
    for( ; li && li->rule; ++li )
    {
	//MARKER;
	if( li->rule( li, st ) )
	{
	    pegc_set_match( st, orig, pegc_pos(st), true );
	    return true;
	}
    }
#endif
    //MARKER;
    pegc_set_pos(st,orig);
    return false;
}

static bool PegcRule_mf_and( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, true, false, true ) ) return false;
    pegc_const_iterator orig = pegc_pos(st);
#if 0
    PegcRule const ** li = (PegcRule const **)self->data;
    if(!li) return false;
    for( ; li && *li && (*li)->rule; ++li )
    {
	if( ! (*li)->rule( *li, st ) )
	{
	    pegc_set_pos(st,orig);
	    return false;
	}
    }
#else
    PegcRule const * li = (PegcRule const *)self->data;
    for( ; li && li->rule; ++li )
    {
	if( ! li->rule( li, st ) )
	{
	    pegc_set_pos(st,orig);
	    return false;
	}
    }
#endif
    pegc_set_match( st, orig, pegc_pos(st), true );
    return true;
}

static char * pegc_list_to_string( bool orOp, PegcRule const ** li )
{
    if( !li || !*li) return 0;
    whclob * cb = whclob_new();
    whclob_append( cb, "(", 1 );
    int i = 0;
    char const * sep = orOp ? " / " : " ";
    const int sepLen = pegc_strlen(sep);
    PegcRule const * r = *li;
    for( ; r && r->rule; ++i, ++r )
    {
	whclob_appendf(cb,"%s",r->name ? r->name : "UnnamedRule");
#if 1
	PegcRule const * n = r+1;
	if( n && n->rule )
	{
	    whclob_append(cb,sep,sepLen);
	}
#endif
    }
    whclob_append( cb, ")", 1 );
    char * ret = whclob_take_buffer(cb);
    whclob_finalize(cb);
    return ret;
}

PegcRule pegc_r_list_a( bool orOp, PegcRule const * li )
{
    return li
	? pegc_r( orOp ? PegcRule_mf_or : PegcRule_mf_and, li )
	: PegcRule_invalid;
}
PegcRule pegc_nonono_r_list_a( pegc_parser * st, bool orOp, PegcRule const * li )
{
    if( ! st || !li ) return PegcRule_invalid;
    PegcRule r = pegc_r( orOp ? PegcRule_mf_or : PegcRule_mf_and, 0 );
#if 1
    r.data = li;
    char * name = pegc_list_to_string( orOp, (PegcRule const **)&li );
    r.name = name;
    pegc_gc_add(st, name, pegc_free );
#else
    int count = 0;
    if(st && li)
    {
	PegcRule const * counter = li;
	for( ; counter && counter->rule; ++counter )
	{
	    ++count;
	}
    }
    if( ! count )
    {
	return PegcRule_invalid;
    }
    PegcRule * list = (PegcRule *)calloc(count+1,sizeof(PegcRule));
    if( ! list )
    {
	if(0)
	{ /* this might need to alloc. */
	    MARKER;
	    fprintf(stderr,
		    "%s:%d:pegc_r_list_a() serious error: calloc() of %d (PegcRule*) failed!\n",
		    __FILE__,__LINE__,count);
	}
	return PegcRule_invalid;
    }
    pegc_gc_add( st, list, pegc_free_value );
    r.data = list;
    int i = 0;
    for( ; i < count; ++i )
    {
	list[i] = li[i];
    }
    list[i] = PegcRule_invalid;
    char * name = pegc_list_to_string( orOp, (PegcRule const **)&list );
    r.name = name;
    pegc_gc_add(st, name, pegc_free );
    //MARKER;printf("Added %d items to rule list. count=%d\n",i,count);
#endif
    //MARKER;printf("Created list rule. Name=[%s]\n",r.name);
    return r;
}

PegcRule pegc_r_list_vp( pegc_parser * st, bool orOp, va_list ap )
{
    /*
      FIXME: This shouldn't copy each element. This point of this func
      is that it refers back to the original. We need only copy the
      pointers to a new array. That said, doing so would mean we need
      to change the implementations of the list rules and
      pegc_r_list_a().  Having all of the rules in a contiguous array
      simplifies those implementations.
    */
    if( !st ) return PegcRule_invalid;
    enum { blockSize = 5 }; /* number of rules to allocate at a time. */
    int count = 1;
    PegcRule * li = 0;
    int pos = 0;
    bool ok = true;
    while( true )
    {
	PegcRule const * vr = va_arg(ap,PegcRule const *);
	if( ! pegc_is_rule_valid(vr) ) break;
	//MARKER;printf("checking va_arg() rule @%p\n",vr);
	if( (!pos) || (pos >=(count-1)) )
	{ /* (re)allocate list */
	    count += blockSize;
	    if( ! li )
	    {
		//MARKER;printf("allocating list for %u items.\n",count);
		li = (PegcRule *)calloc( count, sizeof(PegcRule) );
		if( ! li )
		{
		    ok = false;
		    break;
		}
	    }
	    else
	    {
		//MARKER;printf("re-allocating list for %u items.\n",count);
		PegcRule * re = (PegcRule *)realloc( li, sizeof(PegcRule) * count  );
		if( ! re )
		{
		    ok = false;
		    break;
		}
		li = re;
	    }
	}
	if( ! li ) break;
	li[pos++] = *vr;
    }
    //MARKER;printf("pos=%u count=%u\n",pos,count);
    if( !ok || !pos || !li )
    {
	if(li) pegc_free(li);
	return PegcRule_invalid;
    }
    st->stats.alloced += (sizeof(PegcRule)*count);
    //MARKER;printf("pos=%u count=%u\n",pos,count);
    li[pos] = PegcRule_invalid;
    pegc_gc_add( st, li, pegc_free );
    //PegcRule r = pegc_r( orOp ? PegcRule_mf_or : PegcRule_mf_and, li );
    //return r;
    return pegc_r_list_a( orOp, li );
}


PegcRule pegc_r_list_ep( pegc_parser * st, bool orOp, ... )
{
    va_list vargs;
    va_start( vargs, orOp );
    PegcRule ret = pegc_r_list_vp( st, orOp, vargs );
    va_end(vargs);
    return ret;
}

PegcRule pegc_r_or_ep( pegc_parser * st, ... )
{
    va_list vargs;
    va_start( vargs, st );
    PegcRule ret = pegc_r_list_vp( st, true, vargs );
    va_end(vargs);
    return ret;
}

PegcRule pegc_r_and_ep( pegc_parser * st, ... )
{
    va_list vargs;
    va_start( vargs, st );
    PegcRule ret = pegc_r_list_vp( st, false, vargs );
    va_end(vargs);
    return ret;
}


static bool PegcRule_mf_or_v( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, true, false, true ) ) return false;
    pegc_const_iterator orig = pegc_pos(st);
    PegcRule const * li = (PegcRule const *)self->data;
    int i = 0;
    for( ; li && li[i].rule; ++i )
    {
	//MARKER;
	if( li[i].rule( &li[i], st ) )
	{
	    pegc_set_match( st, orig, pegc_pos(st), true );
	    return true;
	}
    }
    //MARKER;
    pegc_set_pos(st,orig);
    return false;
}

static bool PegcRule_mf_and_v( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, true, false, true ) ) return false;
    pegc_const_iterator orig = pegc_pos(st);
    PegcRule const * li = (PegcRule const *)self->data;
    if(!li) return false;
    int i = 0;
    for( ; li[i].rule; ++i )
    {
	if( ! li[i].rule( &li[i], st ) )
	{
	    pegc_set_pos(st,orig);
	    return false;
	}
    }
    pegc_set_match( st, orig, pegc_pos(st), true );
    return true;
}

PegcRule pegc_r_list_vv( pegc_parser * st, bool orOp, va_list ap )
{
    if( !st ) return PegcRule_invalid;
    const size_t blockSize = 3; /* number of rules to allocate at a time. */
    int count = 1;
    PegcRule * li = 0;
    int pos = 0;
    while( true )
    {
	PegcRule const r = va_arg(ap,PegcRule const);
	if( ! pegc_is_rule_valid(&r) ) break;
	if( (!pos) || (pos >= (count-1)) )
	{ /* (re)allocate list */
	    count += blockSize;
	    if( ! li )
	    {
		//MARKER;printf("allocating list for %u items.\n",count);
		li = calloc( count, sizeof(PegcRule) );
		if( ! li ) break;
	    }
	    else
	    {
		//MARKER;printf("REallocating list for %u items.\n",count);
		PegcRule * re = (PegcRule *)realloc( li, sizeof(PegcRule) * (count)  );
		if( ! re ) break;
		li = re;
	    }
	}
	//MARKER;printf("Added list item #%u\n",pos);
	if( ! li ) break;
	li[pos++] = r;
    }
    if( ! pos || !li )
    {
	if(li) pegc_free(li);
	return PegcRule_invalid;
    }
    st->stats.alloced += (sizeof(PegcRule) * pos);
    li[pos] = PegcRule_invalid;
    pegc_gc_add( st, li, pegc_free );
    PegcRule r = pegc_r( orOp ? PegcRule_mf_or_v : PegcRule_mf_and_v, li );
    char * name = pegc_list_to_string( orOp, (PegcRule const **)&li );
    r.name = name;
    //MARKER;printf("Added %d item(s) to rule list %s.\n",pos,name);
    pegc_gc_add( st, name, pegc_free );
    return r;
}

PegcRule pegc_r_list_ev( pegc_parser * st, bool orOp, ... )
{
    va_list vargs;
    va_start( vargs, orOp );
    PegcRule ret = pegc_r_list_vv( st, orOp, vargs );
    va_end(vargs);
    return ret;
}

PegcRule pegc_r_or_ev( pegc_parser * st, ... )
{
    va_list vargs;
    va_start( vargs, st );
    PegcRule ret = pegc_r_list_vv( st, true, vargs );
    va_end(vargs);
    return ret;
}
PegcRule pegc_r_and_ev( pegc_parser * st, ... )
{
    va_list vargs;
    va_start( vargs, st );
    PegcRule ret = pegc_r_list_vv( st, false, vargs );
    va_end(vargs);
    return ret;
}

static bool PegcRule_mf_action_d( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, true, true, true ) ) return false;
    pegc_const_iterator orig = pegc_pos(st);
    //MARKER; printf("trying rule for delayed action @%p\n", self->data);
    if( ! self->proxy->rule( self->proxy, st ) ) return false;
    PegcAction * theact = (PegcAction*) pegc_gc_search(st,self->data);
    //MARKER; printf("setting up delayed action @%p\n", theact);
    if( ! theact ) return false;
    pegc_action * info = (pegc_action*)malloc(sizeof(pegc_action));
    if( ! info )
    { /* we should report an error, but we don't want to malloc now! */
	return false;
    }
    st->stats.alloced += sizeof(pegc_action);
    //pegc_gc_add( st, info, 0 );
    *info = pegc_action_init;
    info->action = *theact;
    info->action.match.begin = orig;
    info->action.match.end = pegc_pos(st);
    if( ! st->actions )
    {
	st->actions = info;
    }
    else
    {
	info->left = st->actions;
	st->actions->right = info;
	st->actions = info;
    }
    return true;
}

PegcRule pegc_r_action_d_p( pegc_parser * st,
			  PegcRule const * rule,
			  pegc_action_f onMatch,
			  void * clientData )
{
    if( ! st || !pegc_is_rule_valid(rule) ) return PegcRule_invalid;
    //MARKER;
    PegcAction * act = (PegcAction*)malloc(sizeof(PegcAction));
    if( ! act ) return PegcRule_invalid;
    st->stats.alloced += sizeof(PegcAction);
    pegc_gc_add( st, act, pegc_free );
    act->action = onMatch;
    act->data = clientData;
    PegcRule r = pegc_r( PegcRule_mf_action_d, act );
    r.proxy = rule;
    return r;
}
PegcRule pegc_r_action_d_v( pegc_parser * st,
			   PegcRule const rule,
			   pegc_action_f onMatch,
			   void * clientData )
{
    return pegc_r_action_d_p( st, pegc_copy_r_v(st,rule), onMatch, clientData );
}


bool pegc_trigger_actions( pegc_parser * st )
{
    if( pegc_has_error(st) ) return false;
    pegc_action * a = st->actions;
    if( ! a ) return true;
    while( a && a->left ) a = a->left;
    while( a )
    {
	if( a->action.action
	    &&
	    !a->action.action( st, &a->action.match, a->action.data ) )
	{
	    if( ! pegc_has_error(st) )
	    {
		pegc_set_error_e(st,"%s(): action @%p->%p(cursor=@%p,data=@%p) failed.",
				 a, a->action,&a->action.match, a->action.data );
	    }
	    return false;
	}
	a = a->right;
    }
    return true;
}

const PegcRule PegcRule_flush_actions = PEGCRULE_INIT1(PegcRule_mf_flush_actions);
bool PegcRule_mf_flush_actions( PegcRule const * self, pegc_parser * st )
{
    if( !self || !st ) return false;
    bool ret = pegc_trigger_actions(st);
    pegc_clear_actions(st);
    return ret;
}

PegcRule pegc_r_flush_actions()
{
    return PegcRule_flush_actions;
}

static bool PegcRule_mf_action( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, true, true, true ) ) return false;
    //pegc_const_iterator orig = pegc_pos(st);
    bool rc = self->proxy->rule( self->proxy, st );
    //MARKER; printf("rule matched =? %d\n", rc);
    if( rc )
    {
	pegc_action_info * const act = (pegc_action_info * const)self->data;
	//MARKER; printf("action = %p\n", act);
	if( act )
	{
	    rc = act->action( st, &st->match, act->data );
	}
	// Treat an empty action as true?
    }
#if 0
    if( ! rc )
    {
	pegc_set_pos( st, orig );
    }
#endif
    return rc;
}

PegcRule pegc_r_action_i_p( pegc_parser * st,
			   PegcRule const * rule,
			   pegc_action_f onMatch,
			   void * clientData )
{
    //MARKER;
    pegc_action_info * info = 0;
    if( ! st || !rule ) return PegcRule_invalid;
    if( onMatch )
    {
	info = (pegc_action_info*)malloc(sizeof(pegc_action_info));
	if( ! info )
	{
	    return PegcRule_invalid;
	}
	st->stats.alloced += sizeof(pegc_action_info);
	pegc_gc_add( st, info, pegc_free_value );
	info->action = onMatch;
	info->data = clientData;
    }
    PegcRule r = pegc_r( PegcRule_mf_action, info );
    r.proxy = rule;
    //MARKER; printf("action key = %p\n", l);
    r.name = pegc_mprintf( st, "(IF(%s) THEN (ACTION))",rule->name?rule->name:"???");
    return r;
}

PegcRule pegc_r_action_i_v( pegc_parser * st,
			   PegcRule const rule,
			   pegc_action_f onMatch,
			   void * clientData )
{
    return pegc_r_action_i_p( st, pegc_copy_r_v(st,rule), onMatch, clientData );
}


static bool PegcRule_mf_noteof( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, false, false, true ) ) return false;
    pegc_const_iterator orig = pegc_pos(st);
    if( !pegc_eof(st) )
    {
	pegc_set_match( st, orig, orig +1, true );
	return true;
    }
    return false;
}
const PegcRule PegcRule_noteof = PEGCRULE_INIT1(PegcRule_mf_noteof);

/**
   Internal implementation of star/plus rules.
*/
static PegcRule pegc_r_plusstar( PegcRule const * proxy, bool isPlus )
{
    if( ! proxy ) return PegcRule_invalid;
    PegcRule r = pegc_r( isPlus
			 ? PegcRule_mf_plus
			 : PegcRule_mf_star,
			 0 );
    r.name = isPlus ? "(Rule)+" : "(Rule)*";
    r.proxy = proxy;
    return r;
}

PegcRule pegc_r_star_p( PegcRule const * proxy )
{
    return pegc_r_plusstar(proxy, false);
}

PegcRule pegc_r_star_v( pegc_parser * st, PegcRule const proxy )
{
    return pegc_r_plusstar( pegc_copy_r_v( st, proxy ), false );
}

PegcRule pegc_r_plus_p( PegcRule const * proxy )
{
    return pegc_r_plusstar(proxy, true);
}

PegcRule pegc_r_plus_v( pegc_parser * st, PegcRule const proxy )
{
    return pegc_r_plusstar( pegc_copy_r_v( st, proxy ), true );
}

PegcRule pegc_r_string( pegc_const_iterator input, bool caseSensitive )
{
    return pegc_r( caseSensitive
		   ? PegcRule_mf_string
		   : PegcRule_mf_stringi,
		   input );
}

PegcRule pegc_r_char( pegc_char_t input, bool caseSensitive )
{
    return pegc_r( caseSensitive
		   ? PegcRule_mf_char
		   : PegcRule_mf_chari,
		   pegc_latin1(input));
}


/*
  Generate some PegcRule_XXX routines. F is the name of one of C's
  standard isXXX(int) funcs, without the 'is' prefix
*/
#define ACPRULE_ISA(F) \
static bool PegcRule_mf_ ## F( PegcRule const * self, pegc_parser * st ) \
{ \
    if( !pegc_rule_check( self, st, false, false, false ) ) return false; \
    pegc_const_iterator pos = pegc_pos(st); \
    if( is ## F(*pos) ) { \
	pegc_set_match( st, pos, pos+1, true ); \
	return true; \
    } \
    return false; \
}\
const PegcRule PegcRule_ ## F = PEGCRULE_INIT2(PegcRule_mf_ ## F,0);
/* SunStudio compiler says: warning: syntax error:  empty declaration. No clue what he's talking about. */

ACPRULE_ISA(alnum);
ACPRULE_ISA(alpha);
ACPRULE_ISA(cntrl);
ACPRULE_ISA(digit);
ACPRULE_ISA(graph);
ACPRULE_ISA(lower);
ACPRULE_ISA(print);
ACPRULE_ISA(punct);
ACPRULE_ISA(space);
ACPRULE_ISA(upper);
ACPRULE_ISA(xdigit);


#undef ACPRULE_ISA
#define ACPRULE_ISA(F, string) const PegcRule PegcRule_ ## F = PEGCRULE_INIT2(PegcRule_mf_oneof, string)
ACPRULE_ISA(blank," \t");
#undef ACPRULE_ISA

static bool PegcRule_mf_blanks( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, false, false, false ) ) return false;
    const PegcRule r = pegc_r_star_p( &PegcRule_blank );
    return r.rule( &r, st );
}
const PegcRule PegcRule_blanks = PEGCRULE_INIT1(PegcRule_mf_blanks);


static bool PegcRule_mf_opt( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, false, true, true ) ) return false;
    self->proxy->rule( self->proxy, st );
    return true;
}

PegcRule pegc_r_opt_p( PegcRule const * proxy )
{
    if( ! pegc_is_rule_valid(proxy) ) return PegcRule_invalid;
    PegcRule r = pegc_r( PegcRule_mf_opt, 0 );
    r.proxy = proxy;
    return r;
}
PegcRule pegc_r_opt_v( pegc_parser * st, PegcRule const proxy )
{
    return pegc_r_opt_p( pegc_copy_r_v( st, proxy ) );
}

bool PegcRule_mf_eof( PegcRule const * ARG_UNUSED(self), pegc_parser * st )
{
    return pegc_eof(st);
}

const PegcRule PegcRule_eof = PEGCRULE_INIT2(PegcRule_mf_eof,0);
static bool PegcRule_mf_eol( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, false, false, false ) ) return false;
    const PegcRule crnl = pegc_r_string("\r\n",true);
    if( crnl.rule( &crnl, st ) ) return true;
    const PegcRule nl = pegc_r_oneof("\n\r",true);
    return nl.rule( &nl, st );
}

const PegcRule PegcRule_eol = PEGCRULE_INIT2(PegcRule_mf_eol,0);

static bool PegcRule_mf_bol( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, false, false, true ) ) return false;
    pegc_const_iterator orig = pegc_pos(st);
    return ( pegc_begin(st) == orig )
	|| ('\n' == orig[-1]);
}
const PegcRule PegcRule_bol = PEGCRULE_INIT2(PegcRule_mf_bol,0);

static bool PegcRule_mf_digits( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, false, false, false ) ) return false;
    const PegcRule digs = pegc_r_plus_p( &PegcRule_digit );
    return digs.rule( &digs, st );
}
const PegcRule PegcRule_digits = PEGCRULE_INIT1(PegcRule_mf_digits);

static bool PegcRule_mf_int_dec( PegcRule const * self, pegc_parser * st )
{
    pegc_const_iterator orig = pegc_pos(st);
    long myv = 0;
    int len = 0;
    int rc = sscanf(pegc_pos(st), "%ld%n",&myv,&len);
    //MARKER;printf("sscanf(%%ld) rc=%d len=%ld\n", rc, len );
    if( (1 != rc) || (0 == len) ) return false;
    //MARKER;printf("sscanf(%%ld) rc=%d len=%ld\n", rc, len );
    //if( ! pegc_advance(st,len) ) return false;
    //MARKER;printf("sscanf(%%ld) rc=%d len=%ld\n", rc, len );
    pegc_set_match( st, orig, orig + len, true );
    return true;
}
const PegcRule PegcRule_int_dec = PEGCRULE_INIT1(PegcRule_mf_int_dec);


bool PegcRule_mf_int_dec_strict( PegcRule const * self, pegc_parser * st )
{
    //if( ! pegc_rule_check( self, st, false, false, false ) ) return false;
    if( ! pegc_isgood(st) ) return false;
    pegc_const_iterator orig = pegc_pos(st);
    PegcRule * r = 0;
    void * x = pegc_gc_search( st, (void const *)PegcRule_mf_int_dec_strict );
    if( x )
    {
	r = (PegcRule *)x;
	//MARKER;printf("Got cached rule for pegc_r_int_dec_strict()\n");
    }
    else
    {
	r = pegc_alloc_r( st, PegcRule_mf_int_dec_strict, 0 );
	pegc_gc_register( st, (void *)PegcRule_mf_int_dec_strict, 0, r, 0 );
	//MARKER;printf("Creating shared proxy rule for pegc_r_int_dec_strict()\n");
	/**
	   After we've matched digits we need to ensure that the next
	   character is [what we consider to be] legal.
	*/
#if 0
#define DECL(N) PegcRule const * N 
#define CP(X) pegc_copy_r_v( st, X )
	DECL(aend) = 0;
	DECL(punct) = CP(pegc_r_oneof("._",true));
	DECL(illegaltail) = CP( pegc_r_or_ep( st, &PegcRule_alpha, punct, aend ) );
	DECL(next) = CP( pegc_r_notat_p( illegaltail ) );
	DECL(end) = CP(pegc_r_or_ep( st, &PegcRule_eof, next, aend ));
	DECL(R) = CP(pegc_r_and_ep( st, &PegcRule_int_dec, end, aend ));
	r->proxy = R;
#else
#define DECL(N) PegcRule const N 
#define CP(X) X
	DECL(aend) = PegcRule_invalid;
	DECL(punct) = pegc_r_oneof("._",true);
	DECL(illegaltail) = pegc_r_or_ev( st, PegcRule_alpha, punct, aend );
	DECL(next) = pegc_r_notat_v( st, illegaltail );
	DECL(end) = pegc_r_or_ev( st, PegcRule_eof, next, aend );
	DECL(R) = pegc_r_and_ev( st, PegcRule_int_dec, end, aend );
	r = pegc_copy_r_v(st, R );
#endif
#undef CP
#undef DECL
    }
    if( r && r->rule && r->rule( r, st ) )
    {
	//DUMPPOS(st);
	pegc_set_match( st, orig, pegc_pos(st), true );
	return true;
    }
    //MARKER;
    //pegc_set_pos(st,orig);
    return false;
}
const PegcRule PegcRule_int_dec_strict = PEGCRULE_INIT1(PegcRule_mf_int_dec_strict);


PegcRule pegc_r_int_dec_strict()
{
    return PegcRule_int_dec_strict;
}

static bool PegcRule_mf_double( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, false, false, false ) ) return false;
    pegc_const_iterator orig = pegc_pos(st);
    double myv = 0.0;
    int len = 0;
    int rc = sscanf(pegc_pos(st), "%lf%n",&myv,&len);
    if( (0 == rc) || (EOF == rc) || (0 == len) ) return false;
    return pegc_set_match( st, orig, orig + len, true );
}

const PegcRule PegcRule_double = PEGCRULE_INIT1(PegcRule_mf_double);


static bool PegcRule_mf_ascii_impl( PegcRule const * ARG_UNUSED(self),
				    pegc_parser * st, int max )
{
    if(  st && pegc_isgood(st) )
    {
	pegc_const_iterator p = pegc_pos(st);
	int ch = *p;
	if( (ch >= 0) && (ch <=max) )
	{
	    pegc_set_match( st, p, p+1, true );
	    return true;
	}
    }	
    return false;
}
static bool PegcRule_mf_latin1( PegcRule const * self, pegc_parser * st )
{
    return PegcRule_mf_ascii_impl(self,st,255);
}
static bool PegcRule_mf_ascii( PegcRule const * self, pegc_parser * st )
{
    return PegcRule_mf_ascii_impl(self,st,127);
}
const PegcRule PegcRule_latin1 = PEGCRULE_INIT1(PegcRule_mf_latin1);
const PegcRule PegcRule_ascii = PEGCRULE_INIT1(PegcRule_mf_ascii);


struct pegc_range_info
{
    size_t min;
    size_t max;
};
typedef struct pegc_range_info pegc_range_info;

static bool PegcRule_mf_repeat( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, true, true, false ) ) return false;
    pegc_range_info const * info = self->data;
    if( ! info ) return false;
    pegc_const_iterator orig = pegc_pos(st);
    size_t count = 0;
    while( self->proxy->rule( self->proxy, st ) )
    {
	if( (++count == info->max)
	    || pegc_eof(st)
	    || (orig == pegc_pos(st))
	    ) break;
    }
    if( !pegc_has_error(st) &&
       ((count >= info->min) && (count <= info->max) )
       )
    {
	pegc_set_match( st, orig, pegc_pos(st), true );
	return true;
    }
    pegc_set_pos( st, orig );
    return false;
}


PegcRule pegc_r_repeat( pegc_parser * st,
			PegcRule const * rule,
			size_t min,
			size_t max )
{
    if( ! st || !rule ) return PegcRule_invalid;
    if( (max < min) || (0==max) ) return PegcRule_invalid;
    if( (min == 1) && (max ==1) ) return *rule;
    if( (min == 0) && (max == 1) ) return pegc_r_opt_p( rule );
    /**
       TODO: consider stuffing min into st->data and max into st->proxy, to
       avoid an allocation here.
    */
    pegc_range_info * info = (pegc_range_info *)malloc(sizeof(pegc_range_info));
    if( ! info ) return PegcRule_invalid;
    st->stats.alloced += sizeof(pegc_range_info);
    pegc_gc_add( st, info, pegc_free_key );
    info->min = min;
    info->max = max;
    PegcRule r = pegc_r( PegcRule_mf_repeat, info );
    r.proxy = rule;
    return r;
}

struct pegc_pad_info
{
    PegcRule left;
    PegcRule right;
    bool discard;
};
typedef struct pegc_pad_info pegc_pad_info ;
static bool PegcRule_mf_pad( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, true, true, true ) ) return false;
    pegc_pad_info const * info = (pegc_pad_info const *)self->data;
    pegc_const_iterator orig = pegc_pos(st);
    pegc_const_iterator tail = 0;
    if( info && info->left.rule )
    {
	info->left.rule( &(info->left), st );
	if( info->discard ) orig = pegc_pos(st);
    }
    bool ret = self->proxy->rule( self->proxy, st );
    tail = pegc_pos(st);
    if( ret && info && info->right.rule )
    {
	info->right.rule( &(info->right), st );
	if( ! info->discard ) tail = pegc_pos(st);
    }
    if( ret )
    {
	pegc_set_match( st, orig, tail, false );
    }
    else
    {
	pegc_set_pos( st, orig );
    }
    return ret;
}

PegcRule pegc_r_pad_p( pegc_parser * st,
		       PegcRule const * left,
		       PegcRule const * rule,
		       PegcRule const * right,
		       bool discardLeftRight )
{
    if( !st || !rule ) return PegcRule_invalid;
    if( ! left && !right ) return *rule;
    pegc_pad_info * d = (pegc_pad_info *) malloc(sizeof(pegc_pad_info));
    if( ! d ) return PegcRule_invalid;
    st->stats.alloced += sizeof(pegc_pad_info);
    d->discard = discardLeftRight;
    PegcRule r = pegc_r( PegcRule_mf_pad, d );
    r.proxy = rule;
    pegc_gc_add( st, d, pegc_free_value );
    d->left = d->right = PegcRule_invalid;
    if( left )
    {
	d->left = pegc_r_star_p( left );
    }
    if( right )
    {
	d->right = pegc_r_star_p( right );
    }
    return r;
}

PegcRule pegc_r_pad_v( pegc_parser * st,
		       PegcRule const leftRule,
		       PegcRule const mainRule,
		       PegcRule const rightRule,
		       bool discardLeftRight)
{
    return !st
	? PegcRule_invalid
	: pegc_r_pad_p( st,
			pegc_copy_r_p(st,&leftRule),
			pegc_copy_r_p(st,&mainRule),
			pegc_copy_r_p(st,&rightRule),
			discardLeftRight );
}


/**
   Internal data for PegcRule_mf_if_then_else.
*/
struct pegc_if_then_else
{
    PegcRule const * If;
    PegcRule const * Then;
    PegcRule const * Else;
};

typedef struct pegc_if_then_else pegc_if_then_else;
static bool PegcRule_mf_if_then_else( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, true, false, true ) ) return false;
    pegc_if_then_else const * ite = (pegc_if_then_else const *)self->data;
    pegc_const_iterator orig = pegc_pos(st);
    if( ite->If->rule && ite->If->rule( ite->If, st ) )
    {
	//MARKER;printf("IF succeeded.\n");
	if( ite->Then && ite->Then->rule( ite->Then, st ) )
	{
	    //MARKER;printf("THEN succeeded.\n");
	    pegc_set_match( st, orig, pegc_pos(st), false );
	    return true;
	}
	pegc_set_pos( st, orig );
	//MARKER;printf("THEN failed.\n");
	return false;
    }
    else if( ite->Else && ite->Else->rule && ite->Else->rule( ite->Else, st ) )
    {
	//MARKER;printf("ELSE succeeded.\n");
	pegc_set_match( st, orig, pegc_pos(st), false );
	return true;
    }
    //MARKER;printf("IF/THEN/ELSE succeeded.\n");
    pegc_set_pos( st, orig );
    return false;
}

PegcRule pegc_r_if_then_else_p( pegc_parser * st,
				PegcRule const * If,
				PegcRule const * Then,
				PegcRule const * Else )
{
    if( !st || ! If || ! Then ) return PegcRule_invalid;
    pegc_if_then_else * ite = (pegc_if_then_else*)malloc(sizeof(pegc_if_then_else));
    if( ! ite ) return PegcRule_invalid;
    st->stats.alloced += sizeof(pegc_if_then_else);
    pegc_gc_add( st, ite, pegc_free_value );
    ite->If = If;
    ite->Then = Then;
    ite->Else = Else;
    PegcRule r = pegc_r( PegcRule_mf_if_then_else, ite );
    return r;
}

PegcRule pegc_r_if_then_else_v( pegc_parser * st,
				PegcRule const If,
				PegcRule const Then,
				PegcRule const Else )
{
    return pegc_r_if_then_else_p( st,
				  pegc_copy_r_p( st, &If ),
				  pegc_copy_r_p( st, &Then ),
				  pegc_copy_r_p( st, &Else ) );
}

static bool PegcRule_mf_until( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, false, true, true ) ) return false;
    pegc_const_iterator orig = pegc_pos(st);
    pegc_const_iterator pos = orig;
    bool matched = self->proxy->rule( self->proxy, st );
    bool isConsumer = (matched && (pos==pegc_pos(st)));
    while( !matched )
    {
	if( ! pegc_bump(st) ) break;
	matched = self->proxy->rule( self->proxy, st );
	if(0) if( matched && !isConsumer )
	{
	    matched = false;
	}
    }
    if( ! matched )
    {
	pegc_set_pos( st, orig );
    }
    else
    {
	pegc_set_match( st, orig, pegc_pos(st), false );
    }
    return matched;
}

PegcRule pegc_r_until_p( PegcRule const * proxy )
{
    if( ! proxy ) return PegcRule_invalid;
    PegcRule r = pegc_r( PegcRule_mf_until, 0 );
    r.proxy = proxy;
    return r;
}
PegcRule pegc_r_until_v( pegc_parser * st,
			 PegcRule const proxy )
{
    return st ?
	pegc_r_until_p( pegc_copy_r_v(st,proxy) )
	: PegcRule_invalid;
}



/**
   Internal data for pegc_r_string_quoted.
*/
struct pegc_string_quoted_data
{
    char * freeme;
    pegc_char_t quote;
    pegc_char_t esc;
    pegc_char_t ** dest;
};
typedef struct pegc_string_quoted_data pegc_string_quoted_data;
/**
   GC dtor for pegc_string_quoted_data objects.
 */
static void pegc_free_string_quoted_data(void *x)
{
    pegc_string_quoted_data * p = (pegc_string_quoted_data*)x;
    if( p )
    {
	//MARKER; printf("freeing old match string @%p / %p / %p: [%s]\n", p, p->dest, *p->dest, *p->dest);
	if( p->freeme )
	{
	    pegc_free(p->freeme);
	}
	pegc_free(p);
    }
}

char * pegc_unescape_quoted_string( pegc_const_iterator inp,
		      long inlen,
		      int quoteChar,
		      int escChar
		      )
{
    if( !inp || !inlen || (quoteChar != *inp) ) return 0;
    if( inlen < 0 ) inlen = pegc_strlen(inp);
    ++inp;
    whclob * cb = whclob_new();
    char const * at = inp;
    long i = 1;
    bool ok = true;
    for( ; at && *at && (i<inlen-1); ++at, ++i )
    {
	char ch = *at;
	if( escChar && (escChar == ch) )
	{
	    if( ++i == inlen )
	    {
		ok = false;
		break;
	    }
	    ch = *++at;
	    if(escChar == '\\') switch(ch)
	    {
	      case 't': ch = '\t'; break;
	      case 'n': ch = '\n'; break;
	      case 'r': ch = '\r'; break;
	      case 'v': ch = '\v'; break;
	      case 'b': ch = '\b'; break;
	      default:
		  break;
	    };
	}
	else if( quoteChar == ch )
	{
	    break;
	}
	whclob_append_char_n( cb, ch, 1 );
    }
    ++i;
    ok = ok && (quoteChar == *at) && (i==inlen);
    //MARKER;printf("ok=%d i=%ld inlen=%ld\n",ok,i,inlen);
    char * ret = 0;
    if( ok )
    {
	ret = whclob_take_buffer(cb);
    }
    whclob_finalize(cb);
    return ret;
}

/**
   Main implementation of the quoted string parser.
*/
static bool PegcRule_mf_string_quoted( PegcRule const * self, pegc_parser * st )
{
    if( ! pegc_rule_check( self, st, true, false, false ) ) return false;
    pegc_string_quoted_data * sd = (pegc_string_quoted_data*)pegc_gc_search( st, self->data );
    if( ! sd ) return false;
    pegc_const_iterator orig = pegc_pos(st);
    if( *pegc_pos(st) != sd->quote ) return false;
    pegc_bump(st);
    bool ok = true;
    while( pegc_isgood(st) )
    {
	pegc_char_t ch = *pegc_pos(st);
	if( sd->esc && (sd->esc == ch) )
	{
	    if( ! pegc_bump(st) )
	    {
		ok = false;
		break;
	    }
	    ch = *pegc_pos(st);
	}
	else if( sd->quote == ch )
	{
	    break;
	}
	pegc_bump(st);
    }
    if( (!ok)
	|| (!pegc_isgood(st))
	|| (*pegc_pos(st) != sd->quote) )
    {
	if( ! pegc_has_error(st) )
	{
	    pegc_const_iterator pos = pegc_pos(st);
	    size_t line1 = 0, line2 = 0;
	    size_t col1 = 0, col2 = 0;
	    pegc_line_col( st, &line1, &col1 );
	    pegc_set_pos( st, orig );
	    pegc_line_col( st, &line2, &col2 );
	    pegc_set_pos(st,pos);
	    char const * detail = 0;
	    if( *pegc_pos(st) != sd->quote )
	    {
		detail = "unexpected end of quoted string.";
	    }
	    else if( pegc_eof(st) )
	    {
		detail = "hit EOF while inside a quoted string.";
	    }
	    else
	    {
		detail = "unknown error parsing quoted string.";
	    }
	    pegc_set_error_e(st,
			     "%s() parse error near line %u, col %u: %s"
			     "\nPossibly started near line %u, col %u.",
			     __func__, line1, col1, detail,
			     line2, col2);
	}
	pegc_set_pos( st, orig );
	return false;
    }
    pegc_bump(st);
    if( sd->freeme )
    {
	//MARKER; printf("freeing old match string @%p / %p / %p: [%s]\n", sd, sd->dest, *sd->dest, *sd->dest);
	st->stats.alloced -= pegc_strlen(sd->freeme);
	pegc_free(sd->freeme);
	if( sd->dest ) *sd->dest = 0;
	sd->freeme = 0;
    }
    if( 0 != sd->dest )
    {
	sd->freeme = pegc_unescape_quoted_string( orig, pegc_pos(st)-orig, sd->quote, sd->esc );
	if( sd->freeme ) st->stats.alloced += pegc_strlen( sd->freeme );
	*sd->dest = sd->freeme;
    }
    //MARKER; printf("setting match string @%p / %p / %p: [%s]\n", sd, sd->dest, *sd->dest, *sd->dest);
    pegc_set_match( st, orig, pegc_pos(st), false );
    return true;
}

PegcRule pegc_r_string_quoted( pegc_parser * st,
			       pegc_char_t quoteChar,
			       pegc_char_t escChar,
			       pegc_char_t ** target )
{
    if( ! st || !quoteChar ) return PegcRule_invalid;
    pegc_string_quoted_data * sd = (pegc_string_quoted_data*)malloc(sizeof(pegc_string_quoted_data));
    if( ! sd ) return PegcRule_invalid;
    sd->quote = quoteChar;
    sd->esc = escChar;
    sd->freeme = 0;
    sd->dest = target;
    if( sd->dest ) *sd->dest = 0;
    pegc_gc_add( st, sd, pegc_free_string_quoted_data );
    //MARKER; printf("Register dest %c-style string @%p / %p / %p\n", sd->quote,sd, sd->dest, *sd->dest);
    return pegc_r( PegcRule_mf_string_quoted, sd );
}

pegc_stats pegc_get_stats( pegc_parser const * cx )
{
    whgc_stats const wh = whgc_get_stats( cx ? cx->gc : 0 );
    pegc_stats st = cx ? cx->stats : pegc_stats_init;
    if( cx )
    {
	st.gc_count = wh.entry_count;
	//st.alloced += wh.alloced;
	st.gc_internals_alloced = wh.alloced;
    }
    return st;
}


#undef MARKER
#undef DUMPPOS
#undef PEGC_ACTION_INIT
#undef PEGCACTION_INIT

#if defined(__cplusplus)
} /* extern "C" */
#endif
