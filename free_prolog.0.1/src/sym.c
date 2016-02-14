/*
 * symbol table management
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"

extern byte timestamp;

string lextable, newlex;

string *atomtable;
byte *atomstamp;
no newatom;

no
make_symtable(void)
{
    no errctr = 0;
    errctr += !(atomtable = (string *) calloc(1, MAXATOM * sizeof(string)));
    errctr += !(atomstamp = (byte *) calloc(1, MAXATOM * sizeof(byte)));
    newatom = 0;
    errctr += !(lextable = newlex = (string) calloc(1, MAXLEX));
    return !errctr;
}

string
insert_lex(string from)
{
    string currlex = newlex;
    if (newlex + strlen(from) + 1 >= lextable + MAXLEX)
	return NULL;
    while (*newlex++ = *from++)
	;
    return currlex;
}

#define SAME_STRING(s,t) (0==strcmp(s,t))

/* STRING HASHING */

#define SUSED() (atomtable[i])
#define SNEXT() i=MOD((i+1),MAXATOM)
#define SFOUND() (atomtable[i] && SAME_STRING(s,atomtable[i]))

no
skey(string s)
{
    no key = 0;
    while (*s)
	key += (key << 4) + *s++;
    return MOD(key, MAXATOM);
}

no
atomno(string s)
{
    no i = skey(s), last = MOD((i + newatom - 1), MAXATOM);
    while (i != last && SUSED() && !SFOUND())
	SNEXT();
    if (!SUSED()) {
	atomtable[i] = insert_lex(s);
	newatom++;
	atomstamp[i] = timestamp;
	return i;
    }
    else if (i == last) {
	fatal_error("string hash table full");
	return MAXATOM;
    }
 /* else if found */
    return i;

}

void
atombak(int stamp)
{
    no i;
    for (i = 0; i < MAXATOM; i++)
	if (atomstamp[i] > stamp && atomtable[i]) {
	    atomtable[i] = NULL;
	    newatom--;
	}
}

cell
new_func(string name, no argctr)
{
    if (argctr > MAXARITY) {
	fprintf(stderr, "%s/%d", name, argctr);
	warnmes("arity limit exceeded");
	argctr = 0;
    }
    return PUTARITY(PUTSYMNO(FUNTAG, atomno(name)), argctr);
}
