#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "pegc.h"

#if 1
#define MARKER printf("MARKER: %s:%d:%s():\n",__FILE__,__LINE__,__func__);
#else
#define MARKER printf("MARKER: %s:%d:\n",__FILE__,__LINE__);
#endif

bool my_pegc_action( pegc_parser * st,
		     pegc_cursor const *match,
		     void * unused )
{
    char * c = pegc_cursor_tostring(*match);
    MARKER; printf( "my_pegc_action got a match: [%s]\n", c ? c : "<EMPTY>");
    free(c);
    return true;
}

void my_match_listener( pegc_parser const * st, void * d )
{
    char * c = pegc_get_match_string(st);
    MARKER; printf( "my_match_listener got a match: [%s]\ndata=%p\n",
		    c ? c : "<EMPTY>",
		    d );
    free(c);
}

int test_one()
{
    char const * src = "hihi \t world";
    pegc_parser * st = pegc_create_parser( src, -1 );
    //pegc_add_match_listener( st, my_match_listener, st );
#define rulecount 50
    PegcRule Rules[rulecount];
    memset( Rules, 0, rulecount * sizeof(PegcRule) );
#undef rulecount

    unsigned int atRule = 0;
#define NR Rules[atRule++]
    PegcRule const end = PegcRule_invalid;
    NR = pegc_r_oneof("abcxyz",false);
    PegcRule const rH = pegc_r_char( 'h', true );
    PegcRule const rI = pegc_r_char( 'i', true );
    PegcRule const rHI = pegc_r_or_ev( st, rI, rH, end );
    PegcRule rHIPlus = pegc_r_plus_p(&rHI);
    NR = pegc_r_action_i_p( st, &rHIPlus, my_pegc_action, 0 );
    NR = pegc_r_star_p( &PegcRule_blank );
#if 0
    PegcRule const starAlpha = pegc_r_star_p(&PegcRule_alpha);
#else
    PegcRule const starAlpha =
	pegc_r_repeat(st,&PegcRule_alpha,3,10)
	//pegc_r_opt(&PegcRule_alpha)
	;
#endif
    NR = pegc_r_notat_p(&PegcRule_digit);
    NR = pegc_r_action_i_p( st, &starAlpha, my_pegc_action, 0 );
    //NR = *pegc_r_string("world",false); // will fail
    NR = pegc_r(0,0); // end of list
#undef ACPMF
#undef NR


    PegcRule * R = Rules;
    pegc_const_iterator at = 0;
    MARKER; printf("Input string=[%s]\n", src);
    bool rc;
    int i = 0;
    for( ; R && R->rule && !pegc_eof(st); ++R, ++i )
    {
	printf("Trying PegcRule[#%d, rule=%p, data=[%p]]\n",
	       i,
	       (void const *)R->rule,
	       (void const *)R->data );
	rc = R->rule( R, st );
	at = pegc_pos(st);
	printf("\trc == %d, current pos=", rc );
        if( pegc_eof(st) ) printf("<EOF>\n");
	else printf("[%c]\n", (at && *at) ? *at : '!' );
	pegc_iterator m = rc ? pegc_get_match_string(st) : 0;
	if( m )
	{
	    printf("\tMatched string=[%s]\n", m );
	    free(m);
	}
    }
    pegc_destroy_parser(st);
    return 0;
}



int test_two()
{
    MARKER; printf("test two...\n");
    char const * src = "hiaF!";
    if( 0 )
    {
	char const * x = src;
	for( ; *x; ++x )
	{
	    printf("pegc_latin1(%d/%c) = %s\n",(int)*x, *x, pegc_latin1(*x));
	}
    }

#define TRY_STRICT 1
#if TRY_STRICT
    src = "-3492 . xyz . asa";
#else
    src = "-3492"; // ".323asa";
#endif
    pegc_parser * P = pegc_create_parser( src, -1 );
    //pegc_add_match_listener( P, my_match_listener, P );
    int rc = 1;
#if ! TRY_STRICT
    //PegcRule sign = pegc_r_oneof("+-",true);
    //PegcRule R = pegc_r_notat( &PegcRule_alpha );
    const PegcRule R = PegcRule_int_dec;
    //const PegcRule R = PegcRule_double;
#else
    const PegcRule R = PegcRule_int_dec_strict;
    //const PegcRule R2 = pegc_r_int_dec_strict(P); // just checking the cache hit
#endif
#undef TRY_STRICT
    printf("Source string = [%s]\n", src );
    if( pegc_parse(P, &R) )
    {
	rc = 0;
	char * m  = pegc_get_match_string(P);
	printf("Got match on [%s]: [%s]\n",src, m?m:"<EMPTY>");
	free(m);
    }
    else
    {
	printf("number parse failed to match [%s]\nError string=[%s]\n",src,
	       pegc_get_error(P,0,0));
	
    }
    printf("pos = [%s]\n", pegc_eof(P) ? "<EOF>" : pegc_latin1(*pegc_pos(P)) );
    pegc_destroy_parser(P);
    return rc;
}

int test_three()
{
    if(1)
    {
	char const * in = "fbdedf";
	char out[50];
	memset(out,0,50);
	int rc = sscanf(in,"%[bdf]",&out[0]);
	MARKER; printf("sscanf rc==%d, out=%s\n",rc,out);
	//return -1;
    }


    MARKER; printf("test three...\n");
    char const * src = "ZYXtokenCBA!end";
    pegc_parser * P = pegc_create_parser( src, -1 );

    //PegcRule const colon = pegc_r_char( ':',true);
#if 0
    PegcRule const rangeU = pegc_r_char_spec( P, "[A-Z]" );
    PegcRule const rangeL = pegc_r_char_spec( P, "[a-z]" );
#else
    PegcRule const rangeU = pegc_r_char_range( 'A','Z' );
    PegcRule const rangeL = pegc_r_char_range( 'a','z' );
#endif
    PegcRule const delim = pegc_r_plus_p( &rangeU );
    PegcRule const word = pegc_r_plus_p( &rangeL );
    PegcRule const R = pegc_r_pad_p( P, &delim, &word, &delim, true );
    int rc = 0;
    if( pegc_parse(P, &R) )
    {
	rc = 0;
	char * m  = pegc_get_match_string(P);
	pegc_const_iterator pos = pegc_pos(P);
	printf("Got match on [%s]: [%s] current pos=[%s]\n",
	       src,
	       m?m:"<EMPTY>",
	       (pos && *pos) ? pegc_latin1(*pos) : "<NULL>"
	       );
	free(m);
    }
    else
    {
	rc = 1;
	printf("failed to match [%s]\n",src);
    }

    pegc_destroy_parser( P );
    return rc;
}

int test_strings()
{
    MARKER; printf("test strings...\n");
    char const * src[] = {
    "\"a \\\"double-quoted\\\"  \\b\\bstring\"",
    "\"a mis-quoted string",
    "'a \\\'single-quoted\\\' string!!'",
    "\"\"",
    //"'!t*!t*!t'",
    0 };
    pegc_parser * P = pegc_create_parser( 0, 0 );
    char * tgt = 0;
    PegcRule const QD = pegc_r_string_quoted( P, '"', '\\', &tgt );
    PegcRule const QS = pegc_r_string_quoted( P, '\'', '\\', &tgt );
    PegcRule const R = pegc_r_list_ev( P, true, QD, QS, PegcRule_invalid );
    //PegcRule const R = pegc_r_list_ep( P, true, &QD, &QS, &PegcRule_invalid );
    //PegcRule const R = pegc_r_or( P, &QD, &QS );
    int rc = 0;
    int i = 0;
    for( i = 0 ; src[i]; ++i )
    {
	tgt = 0;
	pegc_set_input( P, src[i], -1 );
	MARKER;printf("\tset input to [%s]\n",src[i]);
	if( pegc_parse(P, &R) )
	{
	    rc = 0;
	    char * m  = pegc_get_match_string(P);
	    pegc_const_iterator pos = pegc_pos(P);
	    printf("Got match [unescaped=[%s]] on [src=[%s]]==[match=[%s]] current pos=[%s]\n",
		   tgt ? tgt : "<EMPTY>",
		   src[i],
		   m?m:"<EMPTY>",
		   (pos && *pos) ? pegc_latin1(*pos) : "<EOF>"
		   );
	    free(m);
	}
	else
	{
	    rc = 1;
	    printf("failed to match [%s]\n",src[i]);
	}
	if( pegc_has_error( P ) )
	{
	    printf("Parser error message: [%s]\n",pegc_get_error(P,0,0));
	}
    }
    pegc_destroy_parser( P );
    return rc;
}

static bool my_delayed_action( pegc_parser * st,
			       pegc_cursor const *match,
			       void * clientData )
{
    char * m = pegc_cursor_tostring(*match);
    MARKER;printf("match=%p [%s], clientData=%p\n",match,m,clientData);
    free(m);
    return true;
}

int test_actions()
{
    char const * src = "abcdef";
    pegc_parser * st = pegc_create_parser( src, -1 );
    PegcRule const ABC = pegc_r_string("abc",false);
    PegcRule const ActI = pegc_r_action_i_p( st, &ABC, my_pegc_action, 0 );
    PegcRule const ActD = pegc_r_action_d_p( st, &ActI, my_delayed_action, 0 );
    int rc = 0;
    if( pegc_parse( st, &ActD ) )
    {
	MARKER;printf("rc=%d\n",rc);
	char * m = pegc_get_match_string(st);
	MARKER; printf( "matched string=[%s]\n",m);
	free(m);
	rc = pegc_trigger_actions(st) ? 0 : 1;
	MARKER;printf("rc=%d\n",rc);
    }
    else
    {
	MARKER; printf("Action Match failed :(\n");
	rc = 1;
    }
    if( pegc_has_error(st) )
    {
	rc = 2;
	MARKER;printf("Parser error: [%s]\n",pegc_get_error(st,0,0));
    }
    pegc_destroy_parser(st);
    return rc;
}

int test_until()
{
    MARKER; printf("test until...\n");
    char const * src[] = {
    "z12?abcdef-ghji",
    "def!?:546",
    "xyz134as:jla",
    "!",
    //"xyzla", // should fail
    0 };
    pegc_parser * P = pegc_create_parser( 0, 0 );
#if 0
    PegcRule AnIf =
	pegc_r_if_then_else_v( P,
			       pegc_r_char( 'a', false ),
			       pegc_r_plus_v( P, PegcRule_alpha ),
			       pegc_r_plus_v( P, PegcRule_digit )
			       );
#endif
	//PegcRule Dubious  = pegc_r_or_ep( P,&PegcRule_alpha,&PegcRule_digit,&PegcRule_invalid );
    // pegc_r_or_ev( P,PegcRule_alpha,PegcRule_digit,PegcRule_invalid )
    /* ^^^^ segfaults at cleanup time??? */
    PegcRule const NotAtAlpha = pegc_r_notat_p(&PegcRule_alpha);
    PegcRule const UntilNonAlpha = pegc_r_until_v(P,NotAtAlpha);
    PegcRule const R =
#if 1
	//NotAtAlpha
	UntilNonAlpha
	//pegc_r_plus_p( &NotAtAlpha )
#endif
	;
    //PegcRule const R = PegcRule_alpha;
    int i = 0;
    int rc = 0;
    for( i = 0 ; src[i]; ++i )
    {
	pegc_set_input( P, src[i], -1 );
	MARKER;printf("\tset input to [%s]\n",src[i]);
	if( pegc_parse(P, &R) )
	{
	    rc = 0;
	    char * m  = pegc_get_match_string(P);
	    pegc_const_iterator pos = pegc_pos(P);
	    printf("Got match [src=[%s]]==[match=[%s]] current pos=[%s]\n",
		   src[i],
		   m?m:"<EMPTY>",
		   (pos && *pos) ? pegc_latin1(*pos) : "<EOF>"
		   );
	    free(m);
	}
	else
	{
	    rc = 1;
	    printf("failed to match [%s]\n",src[i]);
	}
	if( pegc_has_error( P ) )
	{
	    printf("Parser error message: [%s]\n",pegc_get_error(P,0,0));
	}
	if( rc ) break;
    }
    pegc_destroy_parser( P );
    return rc;
}


int main( int argc, char ** argv )
{
    int rc = 0;
    rc = test_one();
    if(!rc) rc = test_two();
    if(!rc) rc = test_three();
    if(!rc) rc = test_strings();
    if(!rc) rc = test_actions();
    if(!rc) rc = test_until();
    printf("Done rc=%d=[%s].\n",rc,
	   (0==rc)
	   ? "You win :)"
	   : "You lose :(");
    return rc;
}
