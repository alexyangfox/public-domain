/*
 * reader
 */
#include <stdio.h>
#include <ctype.h>

#include "global.h"

extern string *atomtable;

extern struct specsyms g;

#define MAXARGS MAXARITY

string cursor;
term funtop;

cell
make_funcell(string from, string to, int arity)
{
    cell t;
    char c = *to;
    *to = '\0';
    t = input_fun(from, arity);
    *to = c;
    return t;
}

term
make_term(string from, string to, int arity)
{
    term oldtop = funtop;
    SETREF(funtop++, make_funcell(from, to, arity));
    funtop += arity;
    return oldtop;
}

term
cons(term hd, term tl)
{
    term t = funtop;
    SETREF(t, g.DOT);
    SETREF(t + 1, hd);
    SETREF(t + 2, tl);
    funtop += 3;
    return t;
}

/*********************** LEXICAL ANALYSER *********************/

#define SKIP_SPACE() while(isspace(*cursor)) ++cursor

#define IF(Test) (Test)? ++cursor : 0

#define ALNUM()  IF(isalnum(*cursor) || '_'==*cursor)

#define UPPER() IF(isupper(*cursor) || '_' == *cursor)

#define LOWER() IF(islower(*cursor) || '$'== *cursor)

#define PUNCT() IF(ispunct(*cursor)&& '$'!= *cursor && ','!= *cursor )

#define DIGIT() IF(isdigit(*cursor))

/****************************** PARSER ****************************/
/* (actually a way to translate DCGs to C, for lazy programmers)  */

int
match_sym(string * from, string * to)
{
    *from = cursor;
    SKIP_SPACE();
    if (LOWER()) {
	while (ALNUM());
	*to = cursor;
	return TRUE;
    }
    else if (PUNCT()) {
	while (PUNCT())
	    ;
	*to = cursor;
	return TRUE;
    }
    else {
	cursor = *from;
	return FALSE;
    }
}

int
match_var(string * from, string * to)
{
    *from = cursor;
    SKIP_SPACE();
    if (UPPER()) {
	while (ALNUM())
	    ;
	*to = cursor;
	return TRUE;
    }
    else {
	cursor = *from;
	return FALSE;
    }
}

int
match_int(string * from, string * to)
{
    *from = cursor;
    SKIP_SPACE();
    if (DIGIT()) {
	while (DIGIT())
	    ;
	*to = cursor;
	return TRUE;
    }
    else {
	cursor = *from;
	return FALSE;
    }
}

int
match_char(int c)
{
    SKIP_SPACE();
    if (c != *cursor)
	return FALSE;
    ++cursor;
    return TRUE;
}

int
match_args(int *pctr, term argvect[])
{
    int ok = TRUE;
    if (match_char(')'))
	;
    else if (match_char(',')) {
	ok = match_term(argvect++);
	if (ok) {
	    ++(*pctr);
	    ok = match_args(pctr, argvect);
	}
    }
    else
	ok = FALSE;
    return ok;
}

int
match_list_elements(term t)
{
    int ok = TRUE;
    term hd, tl;
    SETREF(t, g.NIL);
    if (match_char(']'))
	;
    else if (match_char('|')) {
	ok = match_term(&hd) && match_char(']');
	if (ok)
	    SETREF(t, hd);
    }
    else if (match_char(',')) {
	ok = match_term(&hd) && match_list_elements(&tl);
	if (ok)
	    SETREF(t, cons(hd, tl));
    }
    else
	ok = FALSE;
    return ok;
}

int
match_term(term t)
{
    int ok = TRUE;
    string from, to;

    if (match_char('[')) {
	if (match_char(']'))
	    SETREF(t, g.NIL);
	else {
	    term hd, tl;
	    ok = match_term(&hd) && match_list_elements(&tl);
	    if (ok)
		SETREF(t, cons(hd, tl));
	}
    }
    else if (match_var(&from, &to)) {
	SETREF(t, make_funcell(from, to, 0));
    }
    else if (match_int(&from, &to)) {
	SETREF(t, make_funcell(from, to, 0));
    }
    else if (match_sym(&from, &to)) {
	if (match_char('(')) {
	    term first;
	    ok = match_term(&first);
	    if (ok) {
		int argctr = 1;
		term argvect[MAXARGS];
		argvect[0] = first;
		ok = match_args(&argctr, argvect + 1);
		if (ok) {
		    int i;
		    term v = make_term(from, to, argctr);
		    SETREF(t, v);
		    for (i = 1; i <= argctr; i++)
			SETREF(v + i, argvect[i - 1]);
		}
	    }
	}
	else
	    SETREF(t, make_funcell(from, to, 0));
    }
    else
	ok = FALSE;
    return ok;
}

cell
sread(term H, cell xval)
{
    cell v;
    cursor = NAME(xval);
    funtop = H;
    if (!match_term(&v))
	return 0;
    SETREF(funtop, v);
    return (cell) funtop;
}
