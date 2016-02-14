#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "pegc.h"
#include "whgc.h"
#include "whclob.h"
#if 1
#define MARKER printf("******** MARKER: %s:%d:%s():\n",__FILE__,__LINE__,__func__);
#else
#define MARKER printf("******** MARKER: %s:%d:\n",__FILE__,__LINE__);
#endif

PegcRule pg_r_literal( pegc_parser * p );
PegcRule pg_r_larrow( pegc_parser * p );
PegcRule pg_r_skipws( pegc_parser * p, PegcRule const R );

static bool PG_mf_suffix( PegcRule const * self, pegc_parser * p );
static const PegcRule PG_r_suffix = PEGCRULE_INIT1(PG_mf_suffix);
//PegcRule pg_r_suffix() { return PG_r_suffix; }

static bool PG_mf_primary( PegcRule const * self, pegc_parser * p );
static const PegcRule PG_r_primary = PEGCRULE_INIT1(PG_mf_primary);
//PegcRule pg_r_primary() { return PG_r_primary; }


static bool PG_mf_semantic_action( PegcRule const * self, pegc_parser * p );
static const PegcRule PG_r_semantic_action = PEGCRULE_INIT1(PG_mf_semantic_action);
PegcRule pg_r_semantic_action( pegc_parser * p );

bool PG_mf_char_class( PegcRule const * self, pegc_parser * p );
static const PegcRule PG_r_char_class = PEGCRULE_INIT1(PG_mf_char_class);
PegcRule pg_r_char_class();

PegcRule pg_r_prefix();
PegcRule pg_r_sequence();
PegcRule pg_r_expr();
bool PG_mf_expr( PegcRule const * self, pegc_parser * p );
const PegcRule PG_r_expr = PEGCRULE_INIT1(PG_mf_expr);

bool PG_mf_identifier( PegcRule const * self, pegc_parser * p );
const PegcRule PG_r_identifier = PEGCRULE_INIT1(PG_mf_identifier);
PegcRule pg_r_identifier();

bool PG_mf_comment_cpp( PegcRule const * self, pegc_parser * p );
const PegcRule PG_r_comment_cpp = PEGCRULE_INIT3(PG_mf_comment_cpp,0,"CommentCPP");

static struct PGApp
{
    char const * argv0;
    pegc_parser * P;
    whgc_context * gc;
    char ** argv;
} PGApp;

bool PG_gc_reg( void * key, whgc_dtor_f kd, void * value, whgc_dtor_f vd )
{
    return whgc_register( PGApp.gc, key, kd, value, vd );
}

void * PG_gc_search( void const * key )
{
    return whgc_search( PGApp.gc, key );
}


/*
Grammar taken from: http://piumarta.com/software/peg/peg.1.html


   Grammar         <- Spacing Definition+ EndOfFile
   Definition      <- Identifier LEFTARROW Expression
   Expression      <- Sequence ( SLASH Sequence )*
   Sequence        <- Prefix*
   Prefix          <- AND Action
                    / ( AND | NOT )? Suffix
   Suffix          <- Primary ( QUERY / STAR / PLUS )?
   Primary         <- Identifier !LEFTARROW
                    / OPEN Expression CLOSE
                    / Literal
                    / Class
                    / DOT
                    / Action
                    / BEGIN
                    / END
   Identifier      <- < IdentStart IdentCont* > Spacing
   IdentStart      <- [a-zA-Z_]
   IdentCont       <- IdentStart / [0-9]
   Literal         <- ['] < ( !['] Char  )* > ['] Spacing
                    / ["] < ( !["] Char  )* > ["] Spacing
   Class           <- '[' < ( !']' Range )* > ']' Spacing
   Range           <- Char '-' Char / Char
   Char            <- '\\' [abefnrtv'"\[\]\\]
                    / '\\' [0-3][0-7][0-7]
                    / '\\' [0-7][0-7]?
                    / '\\' '-'
                    / !'\\' .
   LEFTARROW       <- '<-' Spacing
   SLASH           <- '/' Spacing
   AND             <- '&' Spacing
   NOT             <- '!' Spacing
   QUERY           <- '?' Spacing
   STAR            <- '*' Spacing
   PLUS            <- '+' Spacing
   OPEN            <- '(' Spacing
   CLOSE           <- ')' Spacing
   DOT             <- '.' Spacing
   Spacing         <- ( Space / Comment )*
   Comment         <- '#' ( !EndOfLine . )* EndOfLine
   Space           <- ' ' / '\t' / EndOfLine
   EndOfLine       <- '\r\n' / '\n' / '\r'
   EndOfFile       <- !.
   Action          <- '{' < [^}]* > '}' Spacing
   BEGIN           <- '<' Spacing
   END             <- '>' Spacing

*/

bool pg_test_action( pegc_parser * st,
		     pegc_cursor const *match,
		     void * msg )
{
#if 0
    char * c = pegc_cursor_tostring(*match);
    MARKER; printf( "%s() got a match: %s [%s]\n",
		    __func__,
		    msg ? (char const *)msg : "",
		    c ? c : "<EMPTY>");
    free(c);
#endif
    return true;
}

static bool PG_mf_spacing( PegcRule const * self, pegc_parser * p )
{
    if( !p || pegc_has_error(p) ) return false;
    //if( pegc_eof(p) ) return true;
    PegcRule const space = pegc_r_plus_p( &PegcRule_space );
    return space.rule( &space, p );
}
static const PegcRule PG_spacing = PEGCRULE_INIT1(PG_mf_spacing);
static const PegcRule PG_op_or = PEGCRULE_INIT2(PegcRule_mf_char,"/");
static const PegcRule PG_op_notat = PEGCRULE_INIT2(PegcRule_mf_char,"!");
static const PegcRule PG_op_at = PEGCRULE_INIT2(PegcRule_mf_char,"&");
static const PegcRule PG_op_star = PEGCRULE_INIT2(PegcRule_mf_char,"*");
static const PegcRule PG_op_plus = PEGCRULE_INIT2(PegcRule_mf_char,"+");
static const PegcRule PG_op_opt = PEGCRULE_INIT2(PegcRule_mf_char,"?");
static const PegcRule PG_op_dot = PEGCRULE_INIT2(PegcRule_mf_char,".");
static const PegcRule PG_op_popen = PEGCRULE_INIT2(PegcRule_mf_char,"(");
static const PegcRule PG_op_pclose = PEGCRULE_INIT2(PegcRule_mf_char,")");
static const PegcRule PG_op_bopen = PEGCRULE_INIT2(PegcRule_mf_char,"[");
static const PegcRule PG_op_bclose = PEGCRULE_INIT2(PegcRule_mf_char,"]");
static const PegcRule PG_op_sbopen = PEGCRULE_INIT2(PegcRule_mf_char,"{");
static const PegcRule PG_op_sbclose = PEGCRULE_INIT2(PegcRule_mf_char,"}");
static const PegcRule PG_op_actopen = PEGCRULE_INIT2(PegcRule_mf_string,"{{{");
static const PegcRule PG_op_actclose = PEGCRULE_INIT2(PegcRule_mf_string,"}}}");
static const PegcRule PG_op_larrow = PEGCRULE_INIT2(PegcRule_mf_string,"<-");
static const PegcRule PG_op_cap_open = PEGCRULE_INIT2(PegcRule_mf_string,"<");
static const PegcRule PG_op_cap_close = PEGCRULE_INIT2(PegcRule_mf_string,">");
static const PegcRule PG_end = PEGCRULE_INIT;
//static const PegcRule * PG_endp = 0;
static const PegcRule PG_endv = PEGCRULE_INIT;
static const PegcRule PG_alpha_uscor =
    PEGCRULE_INIT2(PegcRule_mf_oneofi,"abcdebfhijklmnopqrstuvwxyz_");

static bool PG_ws( PegcRule const * self, pegc_parser * p )
{
    if( !self || !self->proxy || !self->proxy->rule || !p || pegc_has_error(p) ) return false;
    //pegc_const_iterator orig = pegc_pos(p);
    PegcRule const spaces = pegc_r_star_p(&PegcRule_space);
    spaces.rule(&spaces,p);
    pegc_const_iterator p1 = pegc_pos(p);
    if( ! self->proxy->rule( self->proxy, p ) )
    {
	/*
	  We're gonna break a pegc prime rule here and possibly
	  consume even though we return false. We don't want to parse
	  those spaces again.
	 */
	pegc_set_pos( p, p1 );
	return false;
    }
    pegc_const_iterator p2 = pegc_pos(p);
#if 1
    spaces.rule(&spaces,p);
#endif
    pegc_set_match( p, p1, p2, false );
    return true;
}


PegcRule pg_r_skipws( pegc_parser * p, PegcRule const R )
{
    //return pegc_r_pad_v(p, PG_spacing, R, PegcRule_success, true );
    PegcRule r = pegc_r(PG_ws,0);
    r.proxy = pegc_copy_r_p(p,&R);
    return r;
}
PegcRule pg_r_skipws_p( pegc_parser * p, PegcRule const * R )
{
    //return pegc_r_pad_v(p, PG_spacing, R, PegcRule_success, true );
    PegcRule r = pegc_r(PG_ws,0);
    r.proxy = R;
    return r;
}

bool PG_mf_identifier( PegcRule const * self, pegc_parser * p )
{
    //MARKER;
    /*
      Identifier      <- < IdentStart IdentCont* > Spacing
      IdentStart      <- [a-zA-Z_]
      IdentCont       <- IdentStart / [0-9]
    */
    PegcRule * r = 0;
    void * v = pegc_gc_search( p, (void const *)PG_mf_identifier );
    if( v )
    {
	r = (PegcRule *)v;
    }
    else
    {
#if 0
#define cp pegc_copy_r_v
	PegcRule const * idstart = &PG_alpha_uscor;
	PegcRule const * subsequent = cp(p,pegc_r_or_ep(p,idstart,&PegcRule_digit,PG_endp));
	PegcRule const * idcont = cp(p,pegc_r_star_p(subsequent));
	PegcRule const * id = cp(p,pegc_r_and_ep(p, idstart, idcont, PG_endp));
	PegcRule const * pad = cp(p,pg_r_skipws_p( p, id ));
	//PegcRule const id = pegc_r_and_ev(p, idstart, idcont, PG_spacing, PG_end);
	PegcRule * act = cp(p,pegc_r_action_i_p( p, pad, pg_test_action, "PG_mf_identifier()"));
	r = act;
	pegc_gc_register(p , (void *)PG_mf_identifier, 0, r, 0 );
#undef cp
#else
	PegcRule const idstart = PG_alpha_uscor;
	PegcRule const subsequent = pegc_r_or_ev(p,idstart,PegcRule_digit,PG_end);
	PegcRule const idcont = pegc_r_star_v(p,subsequent);
	PegcRule const id = pegc_r_and_ev(p, idstart, idcont, PG_end);
	PegcRule const pad = pg_r_skipws( p, id );
	//PegcRule const id = pegc_r_and_ev(p, idstart, idcont, PG_spacing, PG_end);
	PegcRule const act = pegc_r_action_i_v( p, pad, pg_test_action, "PG_mf_identifier()");
	r = pegc_copy_r_v(p, act );
	pegc_gc_register( p, (void *)PG_mf_identifier, 0, r, 0 );
#endif
	MARKER;printf("Created IDENTIFIER rule @%p w/ rule @%p()\n",r,r->rule);
    }
    if( r && r->rule ) return r->rule(r,p);
    else return false;
}
PegcRule pg_r_identifier()
{
    return PG_r_identifier;
}

PegcRule pg_r_larrow( pegc_parser * p )
{
    PegcRule * r = 0;
    void * v = pegc_gc_search( p, (void const *)pg_r_larrow );
    if( v )
    {
	r = (PegcRule *)v;
    }
    else
    {
	r = pegc_copy_r_v( p, pg_r_skipws( p, PG_op_larrow ) );
	pegc_gc_register( p, (void *)pg_r_larrow, 0, r, 0 );
    }
    if( r ) return *r;
    else return PegcRule_invalid;
}

/**
   A rule for parsing character classes. The following special cases
   are handled:

   If you want to use a ']' character in a list it must be
   the first item after the opening brace, e.g. []abc].

   If you want to use a ']' character in a negated list it must be
   the first item after the caret, e.g. [^]abc].

   An empty set (e.g. []) is not supported.

   Either way, it's likely to really play hacking with syntax
   highlighters.
*/
bool PG_mf_char_class( PegcRule const * self, pegc_parser * p )
{
    PegcRule * r = 0;
    void * v = pegc_gc_search( p, (void const *)PG_mf_char_class );
    if( v )
    {
	r = (PegcRule*)v;
    }
    else
    {
	//const PegcRule empty = pegc_r_string("[]",true);
	const PegcRule open1 = pegc_r_char( '[', true );
	const PegcRule close = pegc_r_char( ']', true );
	const PegcRule open2 = pegc_r_string( "^]", true );
	const PegcRule notclose = pegc_r_notchar(']',true);
	const PegcRule plus = pegc_r_plus_v( p, notclose );
	const PegcRule star = pegc_r_star_v( p, notclose );
	const PegcRule R =
		 pegc_r_and_ev( p,
			//pegc_r_notat_v(p,empty),
			open1,
			pegc_r_if_then_else_v(p,
			      /* special cases: []...] and [^]...] */
			      pegc_r_or_ev(p,
					   open2,
					   close,
					   PG_end),
			      pegc_r_opt_v(p,star),
			      plus),
			close,
			PG_end );
	PegcRule const pad = pg_r_skipws( p, R );
#if 0
	PegcRule const act = pegc_r_action_i_v( p, pad, pg_test_action, "PG_mf_char_class()");
	r = pegc_copy_r_v(p, act);
#else
	r = pegc_copy_r_v(p, pad);
#endif
	pegc_gc_register(p, (void*)PG_mf_char_class, 0, r, 0 );
    }
    if( r && r->rule ) return r->rule(r,p);
    else return false;

}
/**
   Returns PG_r_char_class.
*/
PegcRule pg_r_char_class()
{
    return PG_r_char_class;
}
/**
   Parses single- and double-quoted strings.
 */
PegcRule pg_r_literal( pegc_parser * p )
{
    PegcRule * r = 0;
    void * v = pegc_gc_search( p, (void const *)pg_r_literal );
    if( v )
    {
	r = (PegcRule*)v;
    }
    else
    {
	PegcRule const pad =
	    pg_r_skipws( p,
		 pegc_r_or_ev( p,
		       pegc_r_string_quoted( p, '\'', '\\', 0 ),
		       pegc_r_string_quoted( p, '"', '\\', 0 ),
		       PG_end
		   )
	     );
	PegcRule const act = pegc_r_action_i_v( p, pad, pg_test_action, "pg_r_literal()");
	r = pegc_copy_r_v(p, act);
	pegc_gc_register(p, (void*)pg_r_literal, 0, r, 0 );
    }
    if( r ) return *r;
    else return PegcRule_invalid;
}
bool PG_mf_primary( PegcRule const * self, pegc_parser * p )
{
    /**
           Primary         <- Identifier !LEFTARROW
                            / OPEN Expression CLOSE
                            / Literal
                            / Class
                            / DOT
                            / Action
                            / BEGIN
                            / END
    */
    //MARKER;
    PegcRule * r = 0;
    void * v = pegc_gc_search( p, (void const *)PG_mf_primary );
    if( v )
    {
	r = (PegcRule*)v;
	//MARKER;printf("CACHED OBJECT\n");
    }
    else
    {
	/**
	   Because of the recursiveness of this rule, we have to
	   allocate and register the new object before any of the sub rule
	   are created. If we don't get get in an endless loop.
	*/
	r = pegc_alloc_r(p,0,0);
	//MARKER;printf("REGISTERING r: @%p = @%p\n",PG_mf_primary,r);
	pegc_gc_register(p, (void*)PG_mf_primary, 0, r, 0 );
	
	//MARKER;printf("NEW OBJECT\n");
	PegcRule const iden =
	    pegc_r_and_ev(p,
			  pg_r_identifier(p),
			  pegc_r_notat_v(p, pg_r_larrow(p)),
			  PG_end );
	PegcRule const expr =
	    pg_r_skipws(p,
	    pegc_r_and_ev(p,
			  PG_op_popen,
			  PG_spacing,
			  PG_r_expr,
			  PG_spacing,
			  PG_op_pclose,
			  PG_end
			  ) );
	PegcRule const R = pegc_r_or_ev( p,
					 iden,
					 expr,
					 pg_r_literal(p),
					 pg_r_char_class(p),
					 PG_op_dot,
					 PG_r_semantic_action,
					 PG_end );
	PegcRule const pad = pg_r_skipws(p, R);
	PegcRule const act = pegc_r_action_i_v( p, pad, pg_test_action, "PG_mf_primary()");
	*r = act;
    }
    if( r && r->rule ) return r->rule(r,p);
    else return false;
}

static bool PG_mf_suffix( PegcRule const * self, pegc_parser * p )
{
    /**
       Suffix          <- Primary ( QUERY / STAR / PLUS )?
    */
    //MARKER;
    PegcRule * r = 0;
    void * v = pegc_gc_search( p, (void const *)PG_mf_suffix );
    if( v )
    {
	r = (PegcRule*)v;
    }
    else
    {
	r = pegc_alloc_r(p,0,0);
	pegc_gc_register(p, (void*)PG_mf_suffix, 0, r, 0 );

	PegcRule const opt =
	    pegc_r_opt_v( p,
			  pegc_r_or_ev( p,
					PG_op_opt,
					PG_op_star,
					PG_op_plus,
					PG_end )
			  );
	PegcRule const R =
	    pegc_r_and_ev(p,
			  PG_r_primary,
			  PG_spacing,
			  opt,
			  PG_end );
	PegcRule const pad = pg_r_skipws(p, R);
	PegcRule const act = pegc_r_action_i_v( p, pad, pg_test_action, "PG_mf_suffix()");
	*r = act;
    }
    if( r && r->rule ) return r->rule(r,p);
    else return false;
}

bool PG_mf_semantic_action( PegcRule const * self, pegc_parser * p )
{
    //MARKER;
    PegcRule * r = 0;
    void * v = pegc_gc_search( p, (void const *)PG_mf_semantic_action );
    if( v )
    {
	r = (PegcRule*)v;
    }
    else
    {
	r = pegc_alloc_r(p,0,0);
	pegc_gc_register(p, (void*)PG_mf_semantic_action, 0, r, 0 );
	PegcRule const R =
	    pegc_r_and_ev(p,
			  PG_op_actopen,
			  pegc_r_until_p(&PG_op_actclose),
			  PG_end );
	PegcRule const pad = pg_r_skipws( p, R );
	PegcRule const act = pegc_r_action_i_v( p, pad, pg_test_action, "pg_r_semantic_action()");
	*r = act;
    }
    if( r && r->rule ) return r->rule(r,p);
    else return false;
}

static bool PG_mf_prefix( PegcRule const * self, pegc_parser * p )
{
    //MARKER;
    PegcRule * r = 0;
    void * v = pegc_gc_search( p, (void const *)PG_mf_prefix );
    if( v )
    {
	r = (PegcRule*)v;
    }
    else
    {
	r = pegc_alloc_r(p,0,0);
	pegc_gc_register(p, (void*)PG_mf_prefix, 0, r, 0 );
	PegcRule const at = PG_op_at;
	PegcRule const not = PG_op_notat;
	PegcRule const atact =
	    pegc_r_and_ev( p,
			   at,
			   PG_spacing,
			   PG_r_semantic_action,
			   PG_end
			   );
	PegcRule const andornot =
	    pegc_r_opt_v( p,
			  pegc_r_or_ev( p,
					at, not,
					PG_end
					)
			  );
	PegcRule const tail =
	    pegc_r_and_ev(p,
			  andornot,
			  PG_spacing,
			  PG_r_suffix,
			  PG_end
			  );
	PegcRule const Prefix =
	    pg_r_skipws(p,
	    pegc_r_or_ev(p,
			 atact,
			 PG_spacing,
			 tail,
			 PG_end
			 ));
	PegcRule const act = pegc_r_action_i_v( p, Prefix, pg_test_action, "pg_r_prefix()");
	*r = act;
    }
    if( r && r->rule ) return r->rule( r, p );
    else return false;
}
static const PegcRule PG_r_prefix = PEGCRULE_INIT1(PG_mf_prefix);

PegcRule pg_r_sequence()
{
    //MARKER;
    return pegc_r_star_p(&PG_r_prefix);
}

bool PG_mf_expr( PegcRule const * self, pegc_parser * p )
{
   PegcRule * r = 0;
   void * v = pegc_gc_search( p, (void const *)PG_mf_expr );
   if( v )
   {
       r = (PegcRule*)v;
   }
   else
   {
       r = pegc_alloc_r(p,0,0);
       pegc_gc_register(p, (void*)PG_mf_expr, 0, r, 0 );
       PegcRule const seq = pg_r_sequence();
       PegcRule const tail = 
	   pegc_r_opt_v(p,
			pegc_r_and_ev(p,
				      pg_r_skipws(p,PG_op_or),
				      seq,
				      PG_end)
			);
       PegcRule const Expr =
	   pegc_r_and_ev(p,
			 seq,
			 tail,
			 PG_end);
       PegcRule const pad = pg_r_skipws( p, Expr );
       PegcRule const act = pegc_r_action_i_v( p, pad, pg_test_action, "pg_r_expr()");
       *r = act;
   }
   if( r && r->rule ) return r->rule(r,p);
   else return false;
}
PegcRule pg_r_expr()
{
    return PG_r_expr;
    //MARKER;
}

bool PG_mf_comment_cpp( PegcRule const * self, pegc_parser * p )
{
   PegcRule * r = 0;
   void * v = pegc_gc_search( p, (void const *)PG_mf_comment_cpp );
   if( v )
   {
       r = (PegcRule*)v;
   }
   else
   {
       r = pegc_alloc_r(p,0,0);
       pegc_gc_register(p, (void*)PG_mf_comment_cpp, 0, r, 0 );
       PegcRule const open = pegc_r_string("/*",true);
       PegcRule const atopen = pegc_r_at_v(p,open);
       PegcRule const close = pegc_r_string("*/",true);
       PegcRule const atclose = pegc_r_at_v(p,close);
       PegcRule const err_opener = pegc_r_error_e(p,"Comment opener '/*' found inside a comment.");
       PegcRule const err_eof = pegc_r_error_e(p,"EOF reached inside a comment block.");
       // ^^^ i can't get err_eof to trigger... my rule's broke, apparently.

       PegcRule const opencheck= 
	   pegc_r_if_then_else_v(p,
				 atopen,
				 err_opener,
				 PegcRule_success);
       PegcRule const eofcheck = 
	   pegc_r_if_then_else_v(p,
				 PegcRule_eof,
				 err_eof,
				 PegcRule_success);
       PegcRule const content =
	   pegc_r_plus_v(p,
           pegc_r_and_ev(p,
			opencheck,
			pegc_r_or_ev(p,
				     atclose,
				     pegc_r_and_ev(p,eofcheck,PegcRule_noteof,PG_end),
				     PG_end ),
			PG_end
			)
			 );
       PegcRule const R =
	   pg_r_skipws(p,
	   pegc_r_and_ev(p,
			 open,
			 content,
			 close,
			 PG_end)
		       );
       *r = R;
   }
   if( r && r->rule ) return r->rule(r,p);
   else return false;
}

int a_test()
{
    char const * src = 0;
    whclob * cb = 0;
    if( PGApp.argv && *PGApp.argv )
    {
	cb = whclob_new();
	if( whclob_rc.IOError != whclob_importer_filename( cb, *PGApp.argv ) )
	{
	    src = whclob_bufferc(cb);
	    MARKER;printf("Read in file [%s]\n",*PGApp.argv);
	}
    }
    if( ! cb )
    {
	src = "_abcd _1212 t930_9 'hi world' {{{.....}}} (abc / def) (abc) ";
    }
    pegc_parser * P = PGApp.P;//pegc_create_parser( src, -1 );
    pegc_set_input( P, src, -1 );
    
    PegcRule const Comment = pegc_r_action_i_p(P,
					       &PG_r_comment_cpp, pg_test_action,
					       "CPP-style comment");

#if 0
    PegcRule const R = pegc_r_or_ep(P,
				    &PG_spacing,
				    &Comment,
				    &PG_r_primary,
				    0);
#else
    PegcRule const R = pegc_r_or_ev(P,
				    Comment,
				    PG_spacing,
				    PG_r_primary,
				    PG_end);
#endif
    int rc = 0;
    MARKER;printf("src=[%s]\nRule name=[%s]\n",src,R.name);
    pegc_const_iterator p1 = pegc_pos(P);
    while( pegc_parse( P, &R ) && !pegc_eof(P) )
    {
	pegc_const_iterator p2= pegc_pos(P);
	if( p1 == p2 )
	{
	    printf("Rule did not consume! Breaking!\n");
	    rc = 3;
	    break;
	}
	p1 = p2;
	char * m = pegc_get_match_string(P);
	MARKER;printf("matched: [%s]\n", m ? m : "<EMPTY>");
	if( 1 ) free(m);
	else pegc_gc_add(P,m,pegc_free);
    }
    if( ! pegc_eof( P ) )
    {
	MARKER;printf("Didn't parse to EOF. Current pos=[%c]\n", *pegc_pos(P) );
	rc = 1;
    }
    if( pegc_has_error(P) )
    {
	rc = 2;
	MARKER;printf("Parser error: [%s]\n",pegc_get_error(P,0,0));
    }
    if( 0 == rc )
    {
	pegc_trigger_actions( PGApp.P );
	pegc_clear_actions( PGApp.P );
    }
    pegc_set_input(P,0,0);
    whclob_finalize(cb);
    //pegc_destroy_parser(P);
    return rc;
}

int main( int argc, char ** argv )
{
    MARKER; printf("This is an unfinished app! Don't use it!\n");
    PGApp.argv = argv+1;
    PGApp.gc = whgc_create_context( &PGApp );
    PGApp.P = pegc_create_parser( 0, 0 );
    if( ! PGApp.P || ! PGApp.gc )
    {
	MARKER;printf("%s: ERROR: could not allocate parser and/or GC context!\n",argv[0]);
	return 1;
    }
    int i = 0;
    PGApp.argv0 = argv[0];
    if(0) for( i = 0; i < argc; ++i )
    {
	printf("argv[%d]=[%s]\n",i,argv[i]);
    }
    int rc = 0;
    if(!rc) rc = a_test();
    //if(!rc) rc = test_actions();
    printf("Done rc=%d=[%s].\n",rc,
	   (0==rc)
	   ? "You win :)"
	   : "You lose :(");

    whgc_destroy_context( PGApp.gc );
    pegc_destroy_parser( PGApp.P );
    return rc;
}
