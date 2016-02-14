/*
 *  attrib.c - attributes of clauses
 *
 */

#include "header.h"

struct {
  char *name;
  int type;
} Attributes[MAX_ATTRIBUTES];

/*************
 *
 *   init_attributes()
 *
 *************/

void init_attributes(void)
{
  int i;

  for (i = 0; i < MAX_ATTRIBUTES; i++)
    Attributes[i].name = "";

  Attributes[BSUB_HINT_WT_ATTR].name = "bsub_hint_wt";
  Attributes[BSUB_HINT_WT_ATTR].type = INT_ATTR;

  Attributes[FSUB_HINT_WT_ATTR].name = "fsub_hint_wt";
  Attributes[FSUB_HINT_WT_ATTR].type = INT_ATTR;

  Attributes[EQUIV_HINT_WT_ATTR].name = "equiv_hint_wt";
  Attributes[EQUIV_HINT_WT_ATTR].type = INT_ATTR;

  Attributes[BSUB_HINT_ADD_WT_ATTR].name = "bsub_hint_add_wt";
  Attributes[BSUB_HINT_ADD_WT_ATTR].type = INT_ATTR;

  Attributes[FSUB_HINT_ADD_WT_ATTR].name = "fsub_hint_add_wt";
  Attributes[FSUB_HINT_ADD_WT_ATTR].type = INT_ATTR;

  Attributes[EQUIV_HINT_ADD_WT_ATTR].name = "equiv_hint_add_wt";
  Attributes[EQUIV_HINT_ADD_WT_ATTR].type = INT_ATTR;

  Attributes[LABEL_ATTR].name = "label";
  Attributes[LABEL_ATTR].type = STRING_ATTR;

}  /* init_attributes */

/*************
 *
 *   get_attribute_index()
 *
 *   Return -1 if s is not any known attribute.
 *
 *************/

int get_attribute_index(char *s)
{
  int i, found;

  if (str_ident(s, ""))
    return(-1);
  else {
    for (i = 0, found = 0; i < MAX_ATTRIBUTES && !found; i++)
      found = str_ident(s, Attributes[i].name);
    return(found ? i-1 : -1);
  }
}  /* get_attribute_index */

/*************
 *
 *   attribute_type()
 *
 *************/

int attribute_type(int name)
{
  switch (name) {
  case BSUB_HINT_WT_ATTR: return(INT_ATTR);
  case FSUB_HINT_WT_ATTR: return(INT_ATTR);
  case EQUIV_HINT_WT_ATTR: return(INT_ATTR);
  case BSUB_HINT_ADD_WT_ATTR: return(INT_ATTR);
  case FSUB_HINT_ADD_WT_ATTR: return(INT_ATTR);
  case EQUIV_HINT_ADD_WT_ATTR: return(INT_ATTR);
  case LABEL_ATTR: return(STRING_ATTR);
  default: 
    printf("%d ", name);
    abend("attribute_type: unknown attribute name");
    return(-1);
  }
}  /* attribute_type */

/*************
 *
 *   get_attribute()
 *
 *   Return the attribute (the whole node) associated with an
 *   attribute name.  If it does not exist, return NULL.
 *
 *************/

struct cl_attribute *get_attribute(struct clause *c,
				   int name)
{
  struct cl_attribute *a;
  for (a = c->attributes; a && a->name != name; a = a->next);
  return(a);
}  /* get_attribute */

/*************
 *
 *   set_attribute()
 *
 *************/

void set_attribute(struct clause *c,
		   int name,
		   void *val_ptr)
{
  struct cl_attribute *a1;

  a1 = get_cl_attribute();
  a1->next = c->attributes;
  c->attributes = a1;

  a1->name = name;
  switch (attribute_type(name)) {
  case INT_ATTR:
  case BOOL_ATTR:
    a1->u.i = *((int *) val_ptr);
    break;
  case DOUBLE_ATTR:
    a1->u.d = *((double *) val_ptr);
    break;
  case STRING_ATTR:
    a1->u.s = (char *) val_ptr;
    break;
  case TERM_ATTR:
    a1->u.t = (struct term *) val_ptr;
    break;
  }

}  /* set_attribute */

/*************
 *
 *   delete_attributes()
 *
 *************/

void delete_attributes(struct clause *c)
{
  struct cl_attribute *a1, *a2;

  a1 = c->attributes;
  while (a1) {
    if (attribute_type(a1->name) == TERM_ATTR)
      zap_term(a1->u.t);
    a2 = a1;
    a1 = a1->next;
    free_cl_attribute(a2);
  }
}  /* delete_attributes */

/*************
 *
 *   term_to_attributes()
 *
 *   If error, print message to stdout and return NULL;
 *
 *************/

struct cl_attribute *term_to_attributes(struct term *t)
{
  if (!is_symbol(t, "#", 2)) {
    struct cl_attribute *a = get_cl_attribute();
    struct term *attr_val;
    int i;

    if (sn_to_arity(t->sym_num) != 1) {
      printf("attributes must have arity 1: ");
      p_term(t);
      return(NULL);
    }

    a->name = get_attribute_index(sn_to_str(t->sym_num));
    attr_val = t->farg->argval;
    switch(attribute_type(a->name)) {
    case INT_ATTR:
      if (str_int(sn_to_str(attr_val->sym_num), &i)) {
	a->u.i = i;
	return(a);
      }
      else {
	printf("attribute value should be integer: ");
	p_term(t);
	return(NULL);
      }

    case BOOL_ATTR:
      a->u.i = 0;         /* FIX THIS! */
      return(a);
    case DOUBLE_ATTR:
      a->u.d = 123.4567E89;          /* FIX THIS! */
      return(a);
    case STRING_ATTR:
      a->u.s = sn_to_str(attr_val->sym_num);
      return(a);
    case TERM_ATTR:
      a->u.t = copy_term(attr_val);
      return(a);
    default:
      return(NULL);
    }
  }
  else {
    struct cl_attribute *a1, *a2, *a3;
	
    a1 = term_to_attributes(t->farg->argval);
    a2 = term_to_attributes(t->farg->narg->argval);
    if (!a1 || !a2)
      return(NULL);
    else {
      /* Append attribute lists. */
      for (a3 = a1; a3->next; a3 = a3->next);
      a3->next = a2;
      return(a1);
    }
  }
}  /* term_to_attributes */

/*************
 *
 *   print_attributes()
 *
 *************/

void print_attributes(FILE *fp,
		      struct cl_attribute *a)
{
  for ( ; a; a = a->next) {
    fprintf(fp, "   # %s(", Attributes[a->name].name);
    switch (attribute_type(a->name)) {
    case INT_ATTR:
      fprintf(fp, "%d", a->u.i); break;
    case BOOL_ATTR:
      fprintf(fp, "%s", (a->u.i ? "true" : "false")); break;
    case DOUBLE_ATTR:
      fprintf(fp, "%f", a->u.d); break;
    case STRING_ATTR:
      fprintf(fp, "%s", a->u.s); break;
    case TERM_ATTR:
      print_term(fp, a->u.t); break;
    }
    fprintf(fp, ")");
  }
}  /* print_attributes */

