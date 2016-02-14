/*
 *	Header for troff to RTF translator
 *	Dedicated to the public domain
 *	No warranty expressed or implied, use at your own risk.
 */

#define	XLTSIZE	101
#define	TWIPS	1440	/* units per inch */

/* specials */
enum specials {	BEGINTABLE = 1,
	ENDTABLE,
	JOIN,
	NUMBER
};

struct xltable {
	char *key;
	char *rtf;
	int flag;	/* flag bits, below */
	enum specials special;	/* specials, below */
} xltable[XLTSIZE];

/* flags */
enum {	PARAFLAG = 1,	/* starts a new paragraph */
	ALWAYSTEXT = 2,	/* emits its own text */
	LINEWISE = 4	/* goes into line at a time mode */
};

/* lookup table for flags */
extern struct xlflags {
	char flagchar;
	int flag;
} xlflags[];

/* lookup table for specials */
extern struct xlspecials {
	char *xlname;
	enum specials xlspecial;
} xlspecials[];

struct xltable *lookup(char *key, int insert);
void insert(char *key, char *rtf, enum specials special, int flag);
void readtable(char *filename);

void xlate(FILE *f, char *filename);
char *strmatch(const char *look, const char *target);
enum specials dotcommand(char *line, FILE *fout);
char *dotext(char *intext, int flag);
void trimcomment(char *p);
void begin_table(FILE *fin, FILE *fout);

extern int debug;
#define DBPR if(debug)printf
