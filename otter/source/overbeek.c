/*

overbeek.c -- experimental weighting method,  Feb 2001.

To use this feature, input a list of weight/term pairs like this.

overbeek_world(junk).
0 # P(e(e(x,y),e(e(x,z),e(z,y)))).
0 # P(e(e(e(x,y),z),e(z,e(e(x,u),e(u,y))))).
0 # P(e(e(e(x,y),e(y,z)),e(e(x,u),e(u,z)))).
0 # P(e(e(e(x,x),y),e(y,e(z,z)))).
0 # P(e(e(e(x,x),e(y,y)),e(z,z))).
0 # P(e(x,x)).
0 # P(e(e(x,y),e(y,x))).
0 # P(e(e(e(x,y),z),e(z,e(y,x)))).
0 # P(e(e(e(x,y),e(y,z)),e(x,z))).
0 # P(e(e(x,y),e(e(z,y),e(x,z)))).
end_of_list.

Variable renumbering/renaming is applied to each term
before it is inserted into the hash table.

The ordinary term weighting routine, weight(t), first
calls overbeek_weight(t) below to try to (exact) match
a renumberd copy of t with one of the overbeek_world terms.
If nothing matches, the ordinary weighting method is applied.

Here's an example input file:

%
%  Equivalential calculus (EC): YQF -> YQL  (both are single axioms)
%

set(hyper_res).

assign(max_weight, 0).

list(usable).
-P(e(x,y)) | -P(x) | P(y).  % condensed detachment
-P(e(e(a,b),e(e(c,b),e(a,c)))).  % YQL
end_of_list.

list(sos).
P(e(e(x,y),e(e(x,z),e(z,y)))).   % YQF
end_of_list.

overbeek_world(junk).
0 # P(e(e(x,y),e(e(x,z),e(z,y)))).
0 # P(e(e(e(x,y),z),e(z,e(e(x,u),e(u,y))))).
0 # P(e(e(e(x,y),e(y,z)),e(e(x,u),e(u,z)))).
0 # P(e(e(e(x,x),y),e(y,e(z,z)))).
0 # P(e(e(e(x,x),e(y,y)),e(z,z))).
0 # P(e(x,x)).
0 # P(e(e(x,y),e(y,x))).
0 # P(e(e(e(x,y),z),e(z,e(y,x)))).
0 # P(e(e(e(x,y),e(y,z)),e(x,z))).
0 # P(e(e(x,y),e(e(z,y),e(x,z)))).
end_of_list.

*/

#include "header.h"

#define OVERBEEK_WORLD_SIZE 25000  /* size of hash table */

/*************
 *
 *    hash_term2(term)
 *
 *    Return a hash value of a term: just a word of bits.
 *
 *************/

static unsigned hash_term2(struct term *t)
{
  unsigned hashval = 0;
  if (t->type == NAME)
    hashval = t->sym_num;
  else if (t->type == VARIABLE)
    hashval = t->varnum;
  else {  /* complex */
    struct rel *r;
    hashval = t->sym_num;
    for (r = t->farg; r != NULL; r = r->narg) {
      hashval <<= 1;  /* shift left */
      hashval ^= hash_term2(r->argval); /* exclusive or */
    }
  }
  return(hashval);
}  /* hash_term2 */

/*************
 *
 *    void overbeek_insert()
 *
 *************/

void overbeek_insert(struct term *t)
{
  int i;

  /* If this is the first call, allocate a hash table. */

  if (Overbeek_world == NULL) {
    int i;
    Overbeek_world = (void *) malloc((size_t) (OVERBEEK_WORLD_SIZE * sizeof(void *)));
    for (i = 0; i < OVERBEEK_WORLD_SIZE; i++)
      Overbeek_world[i] = NULL;
  }

  if (!is_symbol(t, "#", 2))
    abend("overbeek_insert, bad term");
  else if (!str_int(sn_to_str(t->farg->argval->sym_num), &i))
    abend("overbeek_insert, bad weight in term");
  else {
    int hashval = abs(hash_term2(t->farg->narg->argval) % OVERBEEK_WORLD_SIZE);
    struct term_ptr *p = get_term_ptr();
    p->term = t;
    p->next = Overbeek_world[hashval];
    Overbeek_world[hashval] = p;
  }
}  /* overbeek_insert */

/*************
 *
 *    int overbeek_weight()
 *
 *************/

int overbeek_weight(struct term *t, int *ip)
{
  if (Overbeek_world == NULL)
    return 0;
  else {
    int hashval;
    struct term_ptr *p;
    int found = 0;
    struct term *copy = copy_term(t);

    renumber_vars_term(copy);
    hashval = abs(hash_term2(copy) % OVERBEEK_WORLD_SIZE);
    for (p = Overbeek_world[hashval]; p && !found; p = p->next) {
      if (term_ident(copy, p->term->farg->narg->argval)) {
	int rc;
	found = 1;
	rc = str_int(sn_to_str(p->term->farg->argval->sym_num), ip);
      }
    }
    zap_term(copy);
    return found;
  }
}  /* overbeek_weight */

/*************
 *
 *    void print_overbeek_world()
 *
 *************/

void print_overbeek_world(void)
{
  if (Overbeek_world == NULL)
    printf("There is no Overbeek World to print!\n");
  else {
    int i;
    int terms = 0;
    int excess = 0;
    int max = 0;

    printf("\nstart of Overbeek_world\n\n");
    for (i = 0; i < OVERBEEK_WORLD_SIZE; i++) {
      if (Overbeek_world[i]) {
	int n = 0;
	struct term_ptr *p;
	printf("%d:\n", i);
	excess--;
	for (p = Overbeek_world[i]; p; p = p->next) {
	  terms++;
	  excess++;
	  n++;
	  printf("        ");
	  p_term(p->term);
	}
	max = (n > max ? n : max);
      }
    }
    printf("\nend of Overbeek_world, terms=%d, overflow=%d, max_overflow=%d.\n\n",terms, excess, max);
  }
}  /* print_overbeek_world */

/*************
 *
 *    void check_overbeek_world()
 *
 *************/

void check_overbeek_world(void)
{
  if (Overbeek_world == NULL)
    printf("There is no Overbeek World to check!\n");
  else {
    int i;
  
    for (i = 0; i < OVERBEEK_WORLD_SIZE; i++) {
      if (Overbeek_world[i]) {
	struct term_ptr *p;
	for (p = Overbeek_world[i]; p; p = p->next) {
	  int wt1, wt2, rc;
	  struct term *t = p->term->farg->narg->argval;
	  rc = str_int(sn_to_str(p->term->farg->argval->sym_num), &wt1);
	  if (overbeek_weight(t, &wt2)) {
	    if (wt1 != wt2) {
	      printf("check_overbeek_world, wrong weight: %d %d ", wt1, wt2);
	      p_term(t);
	    }
	  }
	  else {
	    printf("check_overbeek_world, term not found: ");
	    p_term(t);
	  }
	}
      }
    }
  }
}  /* check_overbeek_world */
