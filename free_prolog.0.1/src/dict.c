/*
 * hash table/dictionary
 */
#include <stdio.h>
#include <stdlib.h>

#include "global.h"

extern struct specsyms g;

extern byte timestamp;

/* ATOM HASHING MACROS */

#define HKEY(pred,fun) MOD((pred+fun),HMAX)

#define HUSED() (htable[i].val)
#define HNEXT() i=MOD((i+1),HMAX)
#define HFOUND() (htable[i].pred==pred && htable[i].fun==fun)
#define HNOTFULL() (i!=last)

/* LINEAR HASHING FOR INDEXING */
/* assert: val is not 0, hput avoids duplications */

hentry htable;
byte *hstamp;
no hcount;

int
hinit(void)
{
    htable = (hentry) calloc(1, HMAX * sizeof(struct hentry));
    hstamp = (byte *) calloc(1, HMAX * sizeof(byte));
    hcount = -1;
    return htable && hstamp;
}

no
hdef(no pred, no fun, no val)
{
    no i = HKEY(pred, fun), last = MOD((i + hcount), HMAX);
    while (HNOTFULL() && HUSED() && !(HFOUND()))
	HNEXT();
    if (!HUSED()) {
	htable[i].pred = pred;
	htable[i].fun = fun;
	htable[i].val = val ? val : (no) (&htable[i].val);
	hcount++;
	hstamp[i] = timestamp;
	return 1;
    }
    return 0;
}

no
hset(no pred, no fun, no val)
{
    no i = HKEY(pred, fun), last = MOD((i + hcount), HMAX);
    while (HNOTFULL() && HUSED() && !(HFOUND()))
	HNEXT();
    if (HFOUND()) {
	htable[i].val = val;
	return 1;
    }
    ERREXIT("key must be defined before use in 'hset()'")
}

no
hget(no pred, no fun)
{
    no last, i = HKEY(pred, fun);
    if (HFOUND())
	return htable[i].val;
    last = MOD((i + hcount), HMAX);
    do {
	HNEXT();
	if (HFOUND())
	    return htable[i].val;
    }
    while (HNOTFULL() && HUSED());
    return (no) NULL;
}

void
hbak(int stamp)
{
    no i;
    for (i = 0; i < HMAX; i++)
	if (hstamp[i] > stamp && htable[i].pred) {
	    hcount--;
	    htable[i].pred = 0;
	    htable[i].fun = 0;
	    htable[i].val = 0;
	}
}

term
hlist(term H, int stamp)
{
    no i;
    for (i = 0; i < HMAX; i++)
	if (hstamp[i] == stamp && HUSED()) {
	    PUSH_LIST(htable[i].pred);
	    PUSH_LIST(htable[i].fun);
	    PUSH_LIST(htable[i].val);
	}
    PUSH_NIL();
    return H;
}

int
hash_args2(term regs)
{
    return ATOMIC(X(1)) && ATOMIC(X(2));
}

int
hash_args3(term regs)
{
    return ATOMIC(X(1)) && ATOMIC(X(2));
}

cell
hash_op(cell(*f) (no, no, no), term regs) {
    cell xval;
    timestamp = BBOARDTIME;
    xval = (*f) (X(1), X(2), X(3));
    timestamp = RUNTIME;
    return xval;
}
