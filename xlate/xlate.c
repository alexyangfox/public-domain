/*
 *	Translate troff to RTF
 *	John Levine, 10/91
 *	Dedicated to the public domain
 *	No warranty expressed or implied, use at your own risk.
 */

#include <stdio.h>
#include <varargs.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "xlate.h"

struct xlspecials xlspecials[] = {
	{ "BEGINTABLE", BEGINTABLE},
	{ "ENDTABLE", ENDTABLE},
	{ "JOIN", JOIN},
	{ "NUMBER", NUMBER},
	{ NULL, 0 }
};

struct xlflags xlflags[] = {
	{ 'p', PARAFLAG},
	{ 'a', ALWAYSTEXT},
	{ 'l', LINEWISE},
	{ 0, 0 }
};

char *tablefile = "xltable";	/* default name of table file */
char *paramsg;			/* translation for PARA */
char *linemsg;			/* translation for LINE */
int inpara;			/* some paragraph text output */
int linewise;			/* respect line boundaries */
int debug;

int
main(int ac, char **av)
{
	int c;
	extern char *optarg;
	extern int optind, opterr;
	       
	while((c = getopt(ac, av, "t:d")) != -1)
	switch(c) {
	case 't':	tablefile = optarg; break;
	case 'd':	debug = 1; break;
	default:	fprintf(stderr,
				"Syntax: %s [-t table ] files ...\n", av[0]);
			return 1;
	}

	if(tablefile)
		readtable(tablefile);
	
	if(optind >= ac)
		xlate(stdin, "stdin");
	else for ( ; optind < ac; optind++) {
		FILE *f = fopen(av[optind], "r");

		if(f) {
			xlate(f, av[optind]);
			fclose(f);
		} else
			perror(av[optind]);
	} /* for args */
} /* main */

/* look up an entry in the table, flag for insert OK */

struct xltable *
lookup(char *key, int insert)
{
	struct xltable *xp;
	char *p;
	int i = 0;
	
	/* hash the key */
	for(p = key; *p; p++)
		i = i*3 + *p;
	
	for(xp = &xltable[i%XLTSIZE]; ; xp++) {
		if( xp >= xltable+XLTSIZE)
			xp = xltable;
		
		if(!xp->key) {	/* new entry ? */
			if(!insert) return NULL;
			xp->key = key;
			return xp;
		}
		if(!strcmp(key, xp->key))
			return xp;
	} /* for table */
} /* lookup */

void
insert(char *key, char *rtf, enum specials special, int flag)
{
	struct xltable *xp = lookup(key, 1);
		
	xp->rtf = rtf;
	xp->special = special;
	xp->flag = flag;
} /* insert */

/*	read the translation table, entries of the form
 *	NAME<tab>entry
 *	where the entry is one line or {} matched
 */

void
readtable(char *filename)
{
	char *buf, *p;
	int f;
	struct stat statbuf;

	f = open(filename, 0);
	if(f < 0) {
		perror(filename);
		exit(1);
	}

	/* read in the whole thing */
	fstat(f, &statbuf);
	buf = malloc(statbuf.st_size+1);
	if(read(f, buf, statbuf.st_size) != statbuf.st_size) {
		printf("Bad read of %s\n", filename);
		exit(1);
	}
	close(f);
	buf[statbuf.st_size] = 0;	/* null terminated string */

	/* process a line at a time */
	for(p = buf; *p; ) {
		char *key, *rtf;
		char *flagch, flags;
		int bracecount;

		if(*p == '#') {	/* comment */
			while(*p && *p != '\n')	/* to end of line */
				*p++;
			continue;	
		}
		if(*p == '\n') {
			p++;
			continue;	/* blank line */
		}
		key = p;
		p = strchr(key, '\t');
		if(!p) {
			printf("Bad table line: %s", key);
			exit(1);
		}
		*p++ = 0;	/* tokenize */

		/* look for flags after a colon */
		flagch = strchr(key, ':');
		flags = 0;
		if(flagch) {
			struct xlflags *xf;
				
			*flagch++ = 0;	/* tidy up the key */
			
			for(xf = xlflags; xf->flagchar; xf++)
				if(strchr(flagch, xf->flagchar))
					flags |= xf->flag;
		} /* flag string */

		DBPR("install %s\n", key);
		if(*p != '{') {
			struct xlspecials *xp;
			
			for(xp = xlspecials; xp->xlname; xp++) {
				if( !strncmp(xp->xlname, p,
					strlen(xp->xlname)))
					break;
			}
			if(xp->xlname) {	/* this is a special */
				insert(key, NULL, xp->xlspecial, 0);
				p = strchr(p, '\n');	/* skip rest of line*/
				continue;
			}
			printf("Unknown keyword after %s", key);
			exit(1);
		}

		/* don't include the braces */
		rtf = ++p;

		for(bracecount = 1; bracecount > 0; p++) {
			switch(*p) {
		case '{':	bracecount++;	break;
		case '}':	bracecount--;	break;
			}
		} /* for bracecount */

		p[-1] = 0;	/* clobbber }, tokenize the rtf */
		insert(key, rtf, 0, flags);
	} /* while *p */
} /* readtable */

static char rtfext[] = ".rtf";

/* translate a file */
void
xlate(FILE *f, char *filename)
{
	char *p;
	FILE *fo;	/* translated file */
	char line[200];
	
	strcpy(line, filename);
	p = strchr(line, '.');
	if(p)
		strcpy(p, rtfext);
	else
		strcat(line, rtfext);
	printf("%s => %s\n", filename, line);

	/* header first */
	fo = fopen(line, "w");
	if(!fo) {
		perror(line);
		exit(1);
	}
	
	/* write hard-wired header character to start outer group */
	putc('{', fo);
	/* dump BEGIN stuff */
	{	struct xltable *xp = lookup("BEGIN", 0);
	
		if(xp && xp->rtf)
			fputs(xp->rtf, fo);

		/* get PARA and LINE translation */
		xp = lookup("PARA", 0);
		if(xp && xp->rtf)
			paramsg = xp->rtf;
		else
			paramsg = "";
		xp = lookup("LINE", 0);
		if(xp && xp->rtf)
			linemsg = xp->rtf;
		else
			linemsg = "";
	}

	inpara = 0;
	while(fgets(line, sizeof(line), f) != NULL) {
		char *p = strchr(line, '\n');
		
		if(p)
			*p = 0;	/* null terminate */

		trimcomment(line);

		if(!line[0]) {	/* blank line */
			struct xltable *xp = lookup("BLANK", 0);

			if(xp && xp->rtf)
				fputs(xp->rtf, fo);
			inpara = 0;
			continue;
		}
		if(line[0] == '.') {
			switch(dotcommand(line, fo)) {
	case 0:
				break;	/* normal, nothing funny */
	case BEGINTABLE:
				begin_table(f, fo);
				break;
	default:
				printf("Mysterious translation for %s\n",
					line);
				break;
			} /* switch */
		} else {
			p = dotext(line, 1);
			if(linewise && inpara)
				fputs(linemsg, fo);
			if(*p)
				inpara = 1;	/* paragraph text */
			fputs(p, fo);
			free(p);
		}
	} /* while lines */

	/* close the outer level */
	fputs("}\n", fo);
	fclose(fo);
} /* xlate */

/* look for one string in another */

char *
strmatch(const char *look, const char *target)
{
	int len = strlen(look);

	while( !!(target = strchr(target, *look)) ) {
		if(!strncmp(target, look, len))
			return (char *)target;
		target++;	/* don't hang on the same char */
	}
	return NULL;	/* not found */
} /* strmatch */

/* throw away \" comments */
void
trimcomment(char *p)
{
	p = strmatch("\\\"", p);
	if(p)
		*p = 0;
} /* trimcomment */

/* parse up a dot command, then expand the results and output them */
/* may also be handed a pseudo-token from tables and other */

enum specials
dotcommand(char *line, FILE *fout)
{
	char *stringargs[10];
	int argno = 0;
	char *funcname;		/* what command this is */
	struct xltable *xp;
	char *p = line;
	
	while(*p) {	/* separate args, 0th is command */
		while(isspace(*p))	/* skip up to arg */
			p++;
		if(!*p)
			break;
		if(*p != '"') {		/* normal unquoted */
			stringargs[argno++] = p;
			while(*p && !isspace(*p))
				p++;
			if(!*p)
				break;
			*p++ = 0;	/* null term arg */
			continue;
		}
		/* quoted string */
		stringargs[argno++] = ++p;
		while(*p && *p != '"')	/* find end */
			p++;
		if(!*p)
			break;
		else
			*p++ = 0;	/* null term, skip quote */
	} /* while *p */
	stringargs[argno] = 0;
	funcname = stringargs[0];

	if(debug) {	/* dump args */
		int i;

		printf("Call of %s with:\n", funcname);
		for(i = 1; i < argno; i++)
			printf("\t\"%s\"\n", stringargs[i]);
		printf("\n");
	}
	xp = lookup(funcname, 0);
	if(!xp) {
		DBPR("no command %s\n", funcname);
		return;
	}

	/* need a new paragraph? */
	if(xp->flag&PARAFLAG) {
		linewise = 0;
		if(inpara)
			fputs(paramsg, fout);
		inpara = 0;
	}

	/* starts a new paragraph? */
	if(xp->flag&ALWAYSTEXT)
		inpara = 1;

	if(xp->flag&LINEWISE)
		linewise = 1;

	if(xp->special)
		return xp->special;

	/* now output the RTF, interpolating the $ arguments */
	p = xp->rtf;
	while(*p) {
		if(*p == '$') {
			int xan = p[1] - '0';
			
			if(xan < argno) {
				char *rtfp = dotext(stringargs[xan], 0);
				
				if(*rtfp)
					inpara = 1;	/* text arg */
				fputs(rtfp, fout);
				free(rtfp);
			}
			p += 2;
		} else
			putc(*p++, fout);
	} /* while *p */
	return 0;	/* no special */
} /* dotcommand */

struct text {
	char *base, *lim;
	char *current;
};

struct text *
newtext(void)
{
	struct text *tp = (struct text *)malloc(sizeof(struct text));

	tp->base = tp->current = malloc(100);
	tp->lim = tp->base + 100;

	return tp;
} /* newtext */

void
addtchar(struct text *tp, int c)
{
	char *p;
	int size;

	*tp->current++ = c;
	if(tp->current < tp->lim)
		return;
	
	/* make bigger */
	size = tp->lim - tp->base;
	p = realloc(tp->base, size+100);
	free(tp->base);
	tp->base = p;
	tp->current = p + size;
	tp->lim = tp->base + size + 100;
} /* addtchar */

#if 0
/* sprintf to var string */
void
tprintf(struct text *tp, char *fmt, void *arg, ...)
{
	char buf[100];
	char *p;
	
	vsprintf(buf, fmt, &arg);	/* crock and a half */
	for(p = buf; *p; p++)
		addtchar(tp, *p);
} /* tprintf */
#endif

/* retrieve the text */
char *
gettext(struct text *tp)
{
	char *p = tp->base;
	
	*tp->current = 0;	/* null term */
	free(tp);
	return p;
} /* gettext */

/* translate a chunk of text, handling all the escape sequences. */
/* eolflag means chunk is at end of input line */
char *
dotext(char *intext, int eolflag)
{
	struct text *tp = newtext();
	int joinflag = 0;
	
	while(*intext) {
		char tbuf[10];
		char *p;
		int c = *intext++;
		struct xltable *xp;

		switch(c) {
	default:
		addtchar(tp, c);
		continue;

	case '\t':	/* treat all tabs as \t */
		strcpy(tbuf, "\\t");
		goto xlcom;

	case '\\':
		p = tbuf;
		*p++ = c;
		*p++ = c = *intext++;
		if(c == '*' || c == 'n' || c == 'f')	/* take a name */
			*p++ = c = *intext++;
		if(c == '+')
			*p++ = c = *intext++;
		if(c == '(') {	/* long name */
			*p++ = *intext++;
			*p++ = *intext++;
		}
		*p = 0;
	xlcom:
		DBPR("translate %s\n", tbuf);
		xp = lookup(tbuf, 0);
		if(!xp) {
			printf("Mysterious escape %s\n", tbuf);
			continue;
		}
		if(xp->rtf) {	/* install text */
			for(p = xp->rtf; *p; p++)
				addtchar(tp, *p);
			continue;
		}
		switch(xp->special) {
		case JOIN:	joinflag = 1; break;
		case NUMBER:	donumber(tbuf, tp);
				break;
		default:	printf("Odd escape %s\n", tbuf);
				break;
		}
		continue;
	}
	} /* for *intext */
	if(!joinflag && eolflag)
		addtchar(tp, ' ');	/* implied word space unless \c */
	return gettext(tp);
} /* dotext */

struct numregs {
	short regname;	/* X or XX */
	int regval;
} numregs[26];


/* handle a number register */
donumber(char *nstr, struct text *tp)
{
	int plusflag = 0;
	char *cp;
	struct numregs *p;
	char nbuf[10];
	
	nstr += 2;	/* skip \n */
	if(*nstr == '+') {
		plusflag = 1;
		nstr++;
	}
	for(p = numregs; p < numregs+(sizeof(numregs)/sizeof(numregs[0]));
	 p++) {
		if(p->regname == *nstr || !p->regname)
			break;
	}
	p->regname = *nstr;	/* handle multi-letter someday */
	if(plusflag)
		++p->regval;
	sprintf(nbuf, "%d", p->regval);
	for(cp = nbuf; *cp; cp++)
		addtchar(tp, *cp);

} /* donumber */

/* output paragraph boiler plate for tab table */
void
tabparout(char *fmt, FILE *fout);

void
begin_table(FILE *fin, FILE *fout)
{
	char tabfmt[50];
	char line[100];
	int intab = 0;
	char *p, *po;

	DBPR("table");
	fgets(line, sizeof(line), fin);
	
	for(p = line, po = tabfmt; *p; p++) {
		int c = tolower(*p);
		switch(c) {
	case ' ': case '\n':
			continue;
	case 'c': case 'l': case 'r': case 'n': case '|':
			*po++ = c;
			continue;
	case 's':		/* ignore spanned, only in headings */
			continue;
	case ',':
			po = tabfmt;
			continue;	/* only use last line */
	case '.':
			break;
	default:
			printf("Mysterious table char %c in %s", c,
				line);
			exit(1);
		} /* switch */
		break;
	} /* for p */
	*po = 0;

	for(; fgets(line, sizeof(line), fin); ) {
		char *p = strchr(line, '\n');
		
		if(p)
			*p = 0;	/* null terminate */

		trimcomment(line);

		if(line[0] == '.') {
			int i = dotcommand(line, fout);
			
			if(i == ENDTABLE)
				break;
			else if(i)
				printf("Control in table: %s\n", line);
			continue;
		}

		if(line[0] == '_' || line[0] == '=') {
			/* horizontal line need to start new paragraph */
			if(!intab)
				tabparout(tabfmt, fout);
			/* end the PP */
			dotcommand(line[0]=='='?"TDHORZ":"THORZ", fout);
			intab = 0;
			inpara = 1;
			continue;
		}
		/* regular text, cook into RTF */
		p = dotext(line, 0);
		if(!intab)
			tabparout(tabfmt, fout);
		else
			dotcommand("TLINE", fout);	/* start new line */
		intab = 1;

		/* need leading tab for other than l */
		if(tabfmt[0] != 'l')
			fputs(lookup("\\t", 0)->rtf, fout);

		fputs(p, fout);
		free(p);
		continue;
	} /* for lines */
	
	/* end the paragraph of the last one */
	fputs(lookup("TPEND", 0)->rtf, fout);
	inpara = 0;
} /* begin_table */

/* write paragraph and tab stops for table */
void
tabparout(char *fmt, FILE *fout)
{
	int pos = 0;	/* current line position */
		
	dotcommand("TPARA", fout);	/* start the paragraph */

	while(*fmt) {
		char *p = NULL;
		int newpos = pos;

		switch(*fmt++) {
	case 'c':	/* fake center as left, for now */
	case 'l':
		if(pos > 0)
			p = lookup("TLEFT", 0)->rtf;
		newpos = pos + TWIPS;	/* 1 inch apart */
		break;
	case '|':
		p = lookup("TBAR", 0)->rtf;
		newpos = pos + TWIPS/10;	/* narrow space for bar */
		break;
	case 'n':
		p = lookup("TDEC", 0)->rtf;
		newpos = pos + TWIPS;	/* 1 inch apart */
		pos += TWIPS/2;		/* center in that inch */
		break;
	case 'r':
		p = lookup("TDEC", 0)->rtf;
		newpos = pos + TWIPS;	/* 1 inch apart */
		pos = newpos - TWIPS/10;	/* mostly to right */
		break;
		
		} /* switch */

		/* use the format in p */
		if(p)
			fprintf(fout, p, pos);

		pos = newpos;
	} /* while *fmt */

	putc('\n', fout);	/* pretty up a little */
} /* tabparout */
