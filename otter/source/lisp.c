#include "lisp.h"

#define MAX_WORD 100
static char Word[MAX_WORD];
static int Gets, Frees;

#if 0
    NOTE: the purpose of the /**/ comments before the function
    definitions is to prevent the prototype-making scripts from
    making prototypes for the functions.
#endif

/*************************************************************************/

static BOOLEAN str_ident(char *s, char *t)
{
  return (strcmp(s, t) == 0);
}  /* str_ident */

/*************************************************************************/

static char *new_str_copy(char *str)
{
  char *p = (void *) malloc((size_t) strlen(str)+1);
  strcpy(p, str);
  return p;
}  /* new_str_copy */

/*************************************************************************/

static Bnode get_bnode(void)
{
  Bnode p = (void *) malloc((size_t) sizeof(struct bnode));
  Gets++;
  p->car = NULL;
  p->cdr = NULL;
  p->label = NULL;
  p->atom = TRUE;
  return p;
}  /* get_bnode */

/*************************************************************************/

static void free_bnode(Bnode p)
{
  Frees++;
  if (p->atom)
    free(p->label);
  free(p);
}  /* get_bnode */

/*************************************************************************/

/**/ void zap_btree(Bnode p)
{
  if (!p->atom) {
    zap_btree(p->car);
    zap_btree(p->cdr);
  }
  free_bnode(p);
}  /* get_bnode */

/*************************************************************************/

/**/ BOOLEAN true_listp(Bnode p)
{
  if (p->atom)
    return str_ident(p->label, "nil");
  else
    return true_listp(p->cdr);
}  /* true_listp */

/*************************************************************************/

/**/ void fprint_btree(FILE *fp, Bnode p)
{
  if (p->atom)
    fprintf(fp, "%s", p->label);
  else if (true_listp(p)) {
    Bnode p2;
    fprintf(fp,"(");
    for (p2 = p; p2->cdr != NULL; p2 = p2->cdr) {
      fprint_btree(fp, p2->car);
      if (p2->cdr->cdr)
	fprintf(fp," ");
    }
    fprintf(fp,")");
  }
  else {
    fprintf(fp,"(");
    fprint_btree(fp, p->car);
    fprintf(fp," . ");
    fprint_btree(fp, p->cdr);
    fprintf(fp,")");
  }
}  /* fprint_btree */

/*************************************************************************/

/**/ void p_btree(Bnode p)
{
  fprint_btree(stdout, p);
  printf("\n");
  fflush(stdout);
}  /* p_btree */

/*************************************************************************/

static BOOLEAN white_char(char c)
{
  return (c == ' '  ||
          c == '\t' ||  /* tab */
          c == '\n' ||  /* newline */
          c == '\v' ||  /* vertical tab */
          c == '\r' ||  /* carriage return */
          c == '\f');   /* form feed */
}  /* white_char */

static BOOLEAN paren(char c)
{
  return (c == '('  ||
          c == ')');
}  /* paren */

/*************************************************************************/

static int fill_word(FILE *fp)
{
  int c;
  int i = 0;
  c = getc(fp);
  while (c != EOF && white_char(c))
    c = getc(fp);
  if (c != EOF) {
    while (c != EOF && !white_char(c) && !paren(c)) {
      Word[i] = c;
      i++;
      if (i == MAX_WORD) {
	Word[i] = '\0';
	fprintf(stderr, "fill_word, word too big: |%s|\n", Word);
	exit(2);
      }
      c = getc(fp);
    }
    if (c == ')' || (i != 0 && c == '('))
      ungetc(c, fp);
  }
  Word[i] = '\0';
  return(c);
}  /* fill_word */

/*************************************************************************/

/**/ BOOLEAN nullp(Bnode p)
{
  return (p->atom && str_ident(p->label,"nil"));
}  /* nullp */

static BOOLEAN dotp(Bnode p)
{
  return (p->atom && str_ident(p->label,"."));
}  /* nullp */

/*************************************************************************/

static void dot_trans (Bnode p)
{
  Bnode curr = p;
  Bnode prev = NULL;
  while (!curr->atom) {
    if (dotp(curr->car)) {
      if (!curr->cdr->atom &&
	  nullp(curr->cdr->cdr) &&
	  prev != NULL &&
	  !dotp(curr->cdr->car)) {
	prev->cdr = curr->cdr->car;
	free_bnode(curr->cdr->cdr);
	free_bnode(curr->cdr);
	free_bnode(curr->car);
	free_bnode(curr);
      }
      else {
	fprintf(stderr, "dot_trans, bad dot notation\n");
	exit(2);
      }
    }
    prev = curr;
    curr = curr->cdr;
  }
}  /* dot_trans */

/*************************************************************************/

/**/ Bnode parse_lisp(FILE *fp)
{
  int c;
  c = fill_word(fp);
  if (!str_ident(Word, "")) {
    Bnode p = get_bnode();
    p->atom = TRUE;
    p->label = new_str_copy(Word);
    return p;
  }
  else if (c == ')' )
    return NULL;
  else if (c == '(') {
    Bnode top = get_bnode();
    Bnode curr = top;
    Bnode p = parse_lisp(fp);
    while (p != NULL) {
      curr->atom = FALSE;
      curr->car = p;
      curr->cdr = get_bnode();
      curr = curr->cdr;
      p = parse_lisp(fp);
    }
    c = getc(fp);  /* step past ')' */
    curr->label = new_str_copy("nil");
    curr->atom = TRUE;
    dot_trans(top);
    return top;
  }
  else
    return NULL;
}  /* parse_lisp */

/*************************************************************************/

/**/ int atom(Bnode p)
{ return p->atom; }  /* atom */

/*************************************************************************/

/**/ Bnode car(Bnode p)
{ return p->car;}  /* car */

/*************************************************************************/

/**/ Bnode cdr(Bnode p)
{ return p->cdr;}  /* cdr */

/*************************************************************************/

/**/ Bnode cadr(Bnode p)
{ return p->cdr->car;}  /* cadr */

/*************************************************************************/

/**/ Bnode caddr(Bnode p)
{ return p->cdr->cdr->car;}  /* caddr */

/*************************************************************************/

/**/ int length(Bnode p)
{
  return (atom(p) ? 0 : length(cdr(p)) + 1);
}  /* length */

/*************************************************************************/

#ifdef SOLO

/**/ int main(int argc, char **argv)
{
  Bnode p;

  p = parse_lisp(stdin);
  fprint_btree(stdout, p);
  printf("length = %d\n", length(p));
  zap_btree(p);
  printf("Gets=%d, Frees=%d.\n", Gets, Frees);
}  /* main */

#endif
