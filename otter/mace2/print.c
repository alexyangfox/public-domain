#include "Mace2.h"

extern struct sort Sorts[MAX_SORTS];
extern int Num_sorts;

extern struct symbol Symbols[MAX_SYMBOLS];
extern int Next_base;
extern int Num_symbols;

/* function_value_n() -- these routines find the value of functions at
 * given points.  It is assumed that we have a model including the
 * well-defined and closed properties.  The DP code knows the
 * model, so we call atom_value.  This is used mostly for printing
 * models.
 */

/*************
 *
 *   function_value_0()
 *
 *************/

static int function_value_0(struct symbol *s)
{
  int b, k;
  int n0 = s->args[0].n;

  b = ATOM1(s, 0);
  for (k = 0; k < n0 && atom_value(b+k) != 1; k++);
  return(k == n0 ? -1 : k);
}  /* function_value_0 */

/*************
 *
 *   function_value_1()
 *
 *************/

static int function_value_1(struct symbol *s, int i)
{
  int b, k;
  int n1 = s->args[1].n;

  b = ATOM2(s, i, 0);
  for (k = 0; k < n1 && atom_value(b+k) != 1; k++);
  return(k == n1 ? -1 : k);
}  /* function_value_1 */

/*************
 *
 *   function_value_2()
 *
 *************/

static int function_value_2(struct symbol *s, int i, int j)
{
  int b, k;
  int n2 = s->args[2].n;

  b = ATOM3(s, i, j, 0);
  for (k = 0; k < n2 && atom_value(b+k) != 1; k++);
  return(k == n2 ? -1 : k);
}  /* function_value_2 */

/*************
 *
 *   function_value_3()
 *
 *************/

static int function_value_3(struct symbol *s, int i, int j, int a)
{
  int b, k;
  int n3 = s->args[3].n;

  b = ATOM4(s, i, j, a, 0);
  for (k = 0; k < n3 && atom_value(b+k) != 1; k++);
  return(k == n3 ? -1 : k);
}  /* function_value_3 */

/*************
 *
 *   print_table_f0()
 *
 *************/

static void print_table_f0(FILE *fp, struct symbol *s)
{
  fprintf(fp, "\n %s: %d\n", s->name, function_value_0(s));
}  /* print_table_f0 */

/*************
 *
 *   print_table_f1()
 *
 *************/

static void print_table_f1(FILE *fp, struct symbol *s)
{
  int i;
  char *s1, *s2;
  int n0 = s->args[0].n;

  if (n0 <= 10) {
    s1 = "%2d";
    s2 = "--";
  }
  else {
    s1 = "%3d";
    s2 = "---";
  }
    
  fprintf(fp, "\n %s :\n", s->name);
  fprintf(fp, "       ");
  for (i = 0; i < n0; i++)
    fprintf(fp, s1, i);
  fprintf(fp, "\n    ---");
  for (i = 0; i < n0; i++)
    fprintf(fp, s2);
  fprintf(fp, "\n       ");
  for (i = 0; i < n0; i++)
    fprintf(fp, s1, function_value_1(s, i));
  fprintf(fp, "\n");
}  /* print_table_f1 */

/*************
 *
 *   print_table_f2()
 *
 *************/

static void print_table_f2(FILE *fp, struct symbol *s)
{
  int i, j, v;
  char *s1, *s2, *s3;
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;

  if (n1 <= 10) {
    s1 = "%2d";
    s2 = "--";
    s3 = " -";
  }
  else {
    s1 = "%3d";
    s2 = "---";
    s3 = "  -";
  }
    
  fprintf(fp, "\n %s :\n", s->name);
  fprintf(fp, "      |");
  for (i = 0; i < n1; i++)
    fprintf(fp, s1, i);
  fprintf(fp, "\n    --+");
  for (i = 0; i < n1; i++)
    fprintf(fp, s2);
  fprintf(fp, "\n");

  for (i = 0; i < n0; i++) {
    fprintf(fp, "%5d |", i);
    for (j = 0; j < n1; j++) {
      v = function_value_2(s, i, j);
      if (v < 0)
	fprintf(fp, s3);
      else
	fprintf(fp, s1, v);
    }
    fprintf(fp, "\n");
  }
}  /* print_table_f2 */

/*************
 *
 *   print_table_r0()
 *
 *************/

static void print_table_r0(FILE *fp, struct symbol *s)
{
  int v;
  char *s1;
  v = atom_value(ATOM0(s));
  switch(v) {
  case 0: s1 = "F"; break;
  case 1: s1 = "T"; break;
  case 2: s1 = "F"; break;
  default: s1 = "?"; break;
  }
  fprintf(fp, "\n %s: %s\n", s->name, s1);
}  /* print_table_r0 */

/*************
 *
 *   print_table_r1()
 *
 *************/

static void print_table_r1(FILE *fp, struct symbol *s)
{
  int i, v;
  char *s1;
  int n0=s->args[0].n;

  fprintf(fp, "\n %s :\n", s->name);
  fprintf(fp, "       ");
  for (i = 0; i < n0; i++)
    fprintf(fp, "%2d", i);
  fprintf(fp, "\n    ---");
  for (i = 0; i < n0; i++)
    fprintf(fp, "--");
  fprintf(fp, "\n       ");
  for (i = 0; i < n0; i++) {
    v = atom_value(ATOM1(s, i));
    switch(v) {
    case 0: s1 = "F"; break;
    case 1: s1 = "T"; break;
    case 2: s1 = "F"; break;
    default: s1 = "?"; break;
    }
    fprintf(fp, "%2s", s1);
  }
  fprintf(fp, "\n");
}  /* print_table_r1 */

/*************
 *
 *   print_table_r2()
 *
 *************/

static void print_table_r2(FILE *fp, struct symbol *s)
{
  int i, j, v;
  char *s1;
  int n0=s->args[0].n;
  int n1=s->args[1].n;

  fprintf(fp, "\n %s :\n", s->name);
  fprintf(fp, "      |");
  for (i = 0; i < n1; i++)
    fprintf(fp, "%2d", i);
  fprintf(fp, "\n    --+");
  for (i = 0; i < n1; i++)
    fprintf(fp, "--");
  fprintf(fp, "\n");

  for (i = 0; i < n0; i++) {
    fprintf(fp, "%5d |", i);
    for (j = 0; j < n1; j++) {
      v = atom_value(ATOM2(s, i, j));
      switch(v) {
      case 0: s1 = "F"; break;
      case 1: s1 = "T"; break;
      case 2: s1 = "F"; break;
      default: s1 = "?"; break;
      }
      fprintf(fp, "%2s", s1);
    }
    fprintf(fp, "\n");
  }
}  /* print_table_r2 */

/*************
 *
 *   print_model()
 *
 *   Print the current model as a first-order model.  The DP code
 *   knows about the propositional model, so we call atom_value()
 *   to get the information to print the first-order model.
 *
 *   Assume unassigned atoms are true, because dp determines that
 *   we have a model when all remaining clauses are positive.
 *
 *   Don't print equality, order, or $Connect relations.
 *
 *************/

void print_model(FILE *fp)
{
  int i;
  struct symbol *s;
  static int count;

#if 1
  fprintf(fp, "\n======================= Model #%d at %.2f seconds:\n",
	  ++count, run_time() / 1000.);
#endif

  for (i = 0; i < Num_symbols; i++) {
    s = Symbols + i;
    if (s->type == FUNCTION) {
      switch (s->arity) {
      case 1: print_table_f0(fp, s); break;
      case 2: print_table_f1(fp, s); break;
      case 3: print_table_f2(fp, s); break;
      default: fprintf(fp, "Dimension %d table for %s not printed\n",
		       s->arity-1, s->name);
      }
    }
    else if (!TP_BIT(s->properties, MACE_ORDER_BIT) &&
	     !TP_BIT(s->properties, MACE_EQ_BIT) &&
	     !initial_str("$Connect", s->name)) {
      switch (s->arity) {
      case 0: print_table_r0(fp, s); break;
      case 1: print_table_r1(fp, s); break;
      case 2: print_table_r2(fp, s); break;
      default: fprintf(fp, "Dimension %d table for %s not printed\n",
		       s->arity, s->name);
      }
    }
  }
  fprintf(fp, "end_of_model\n");
}  /* print_model */

/*************
 *
 *   print_table_f0_portable()
 *
 *************/

static void print_table_f0_portable(FILE *fp, struct symbol *s)
{
  fprintf(fp, "\n        function(%s, [%d])",s->name,function_value_0(s));
}  /* print_table_f0_portable */

/*************
 *
 *   print_table_f1_portable()
 *
 *************/

static void print_table_f1_portable(FILE *fp, struct symbol *s)
{
  int i;
  int n0 = s->args[0].n;

  fprintf(fp, "\n        function(%s(_), [", s->name);
  for (i = 0; i < n0; i++) {
    fprintf(fp, "%d", function_value_1(s, i));
    if (i < n0-1)
      fprintf(fp, ", ");
    else
      fprintf(fp, "])");
  }
}  /* print_table_f1_portable */

/*************
 *
 *   print_table_f2_portable()
 *
 *************/

static void print_table_f2_portable(FILE *fp, struct symbol *s)
{
  int i, j, v;
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;

  fprintf(fp, "\n        function(%s(_,_), [\n", s->name);

  for (i = 0; i < n0; i++) {
    fprintf(fp, "                ");
    /* print row */
    for (j = 0; j < n1; j++) {
      v = function_value_2(s, i, j);
      if (j < n1-1)
	fprintf(fp, "%d, ", v);
      else if (i < n0-1)
	fprintf(fp, "%d,\n", v);
      else
	fprintf(fp, "%d    ])", v);
    }
  }
}  /* print_table_f2_portable */

/*************
 *
 *   print_table_f3_portable()
 *
 *************/

static void print_table_f3_portable(FILE *fp, struct symbol *s)
{
  int i, j, k, v;
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;
  int n2 = s->args[2].n;

  fprintf(fp, "\n        function(%s(_,_), [\n", s->name);

  for (i = 0; i < n0; i++) {
    for (j = 0; j < n1; j++) {
      fprintf(fp, "                ");
      for (k = 0; k < n2; k++) {
	v = function_value_3(s, i, j, k);
	if (k < n2-1)
	  fprintf(fp, "%d, ", v);
	else if (j < n1-1)
	  fprintf(fp, "%d,\n", v);
	else if (i < n0-1)
	  fprintf(fp, "%d,\n\n", v);
	else
	  fprintf(fp, "%d    ])", v);
      }
    }
  }
}  /* print_table_f3_portable */

/*************
 *
 *   print_table_r0_portable()
 *
 *************/

static void print_table_r0_portable(FILE *fp, struct symbol *s)
{
  int v;
  char *s1;
  v = atom_value(ATOM0(s));
  switch(v) {
  case 0: s1 = "0"; break;
  case 1: s1 = "1"; break;
  case 2: s1 = "0"; break;
  default: s1 = "?"; break;
  }
  fprintf(fp, "\n        predicate(%s, [%s])",s->name, s1);
}  /* print_table_r0_portable */

/*************
 *
 *   print_table_r1_portable()
 *
 *************/

static void print_table_r1_portable(FILE *fp, struct symbol *s)
{
  int i, v;
  char *s1;
  int n0=s->args[0].n;

  fprintf(fp, "\n        predicate(%s(_), [", s->name);
  for (i = 0; i < n0; i++) {
    v = atom_value(ATOM1(s, i));
    switch(v) {
    case 0: s1 = "0"; break;
    case 1: s1 = "1"; break;
    case 2: s1 = "0"; break;
    default: s1 = "?"; break;
    }
    fprintf(fp, "%s", s1);
    if (i < n0-1)
      fprintf(fp, ", ");
    else
      fprintf(fp, "])");
  }

  for (i = 0; i < n0; i++) {
  }
}  /* print_table_r1_portable */

/*************
 *
 *   print_table_r2_portable()
 *
 *************/

static void print_table_r2_portable(FILE *fp, struct symbol *s)
{
  int i, j, v;
  char *s1;
  int n0=s->args[0].n;
  int n1=s->args[1].n;

  fprintf(fp, "\n        predicate(%s(_,_), [\n", s->name);

  for (i = 0; i < n0; i++) {
    fprintf(fp, "                ");
    /* print row */
    for (j = 0; j < n1; j++) {
      v = atom_value(ATOM2(s, i, j));
      switch(v) {
      case 0: s1 = "0"; break;
      case 1: s1 = "1"; break;
      case 2: s1 = "0"; break;
      default: s1 = "?"; break;
      }
      if (j < n1-1)
	fprintf(fp, "%s, ", s1);
      else if (i < n0-1)
	fprintf(fp, "%s,\n", s1);
      else
	fprintf(fp, "%s   ])", s1);
    }
  }
}  /* print_table_r2_portable */

/*************
 *
 *   print_table_r3_portable()
 *
 *************/

static void print_table_r3_portable(FILE *fp, struct symbol *s)
{
  int i, j, k, v;
  char *s1;
  int n0=s->args[0].n;
  int n1=s->args[1].n;
  int n2=s->args[2].n;

  fprintf(fp, "\n        predicate(%s(_,_,_), [\n", s->name);

  for (i = 0; i < n0; i++) {
    for (j = 0; j < n1; j++) {
      fprintf(fp, "                ");
      for (k = 0; k < n2; k++) {
	v = atom_value(ATOM3(s, i, j, k));
	switch(v) {
	case 0: s1 = "0"; break;
	case 1: s1 = "1"; break;
	case 2: s1 = "0"; break;
	default: s1 = "?"; break;
	}
	if (k < n2-1)
	  fprintf(fp, "%s, ", s1);
	else if (j < n1-1)
	  fprintf(fp, "%s,\n", s1);
	else if (i < n0-1)
	  fprintf(fp, "%s,\n\n", s1);
	else
	  fprintf(fp, "%s   ])", s1);
      }
    }
  }
}  /* print_table_r3_portable */

/*************
 *
 *   print_table_r4_portable()
 *
 *************/

static void print_table_r4_portable(FILE *fp, struct symbol *s)
{
  int i, j, k, l, v;
  char *s1;
  int n0=s->args[0].n;
  int n1=s->args[1].n;
  int n2=s->args[2].n;
  int n3=s->args[3].n;

  fprintf(fp, "\n        predicate(%s(_,_,_,_), [\n", s->name);

  for (i = 0; i < n0; i++) {
    for (j = 0; j < n1; j++) {
      for (k = 0; k < n2; k++) {
	fprintf(fp, "                ");
	for (l = 0; l < n3; l++) {
	  v = atom_value(ATOM4(s, i, j, k, l));
	  switch(v) {
	  case 0: s1 = "0"; break;
	  case 1: s1 = "1"; break;
	  case 2: s1 = "0"; break;
	  default: s1 = "?"; break;
	  }
	  if (l < n3-1)
	    fprintf(fp, "%s, ", s1);  /* in i */
	  else if (k < n2-1)
	    fprintf(fp, "%s,\n", s1);  /* end of i; in j */
	  else if (j < n1-1)
	    fprintf(fp, "%s,\n\n", s1);  /* end of i,j; in k */
	  else if (i < n0-1)
	    fprintf(fp, "%s,\n\n", s1);  /* end of i,j,k; in l */
	  else
	    fprintf(fp, "%s   ])", s1);  /* end of i,j,k,l */
	}
      }
    }
  }
}  /* print_table_r4_portable */

/*************
 *
 *   print_model_portable()
 *
 *   Print the current model as a first-order model.  The DP code
 *   knows about the propositional model, so we call atom_value()
 *   to get the information to print the first-order model.
 *
 *   Assume unassigned atoms are true, because dp determines that
 *   we have a model when all remaining clauses are positive.
 *
 *   Don't print equality or $Connect relations.
 *
 *************/

void print_model_portable(FILE *fp)
{
  int i, domain_size, syms_printed;
  struct symbol *s;
  static int count;

  domain_size = Sorts[0].n;

  fprintf(fp, "\n======================= Model #%d at %.2f seconds:\n",
	  ++count, run_time() / 1000.);
    
  fprintf(fp, "interpretation( %d, [\n", domain_size);

  syms_printed = 0;
  for (i = 0; i < Num_symbols; i++) {
    s = Symbols + i;
    if (!TP_BIT(s->properties, MACE_EQ_BIT) &&
	!initial_str("$Connect", s->name)) {
      if (syms_printed > 0)
	fprintf(fp, ",\n");

      if (s->type == FUNCTION) {
	switch (s->arity) {
	case 1: print_table_f0_portable(fp, s); break;
	case 2: print_table_f1_portable(fp, s); break;
	case 3: print_table_f2_portable(fp, s); break;
	case 4: print_table_f3_portable(fp, s); break;
	default: fprintf(fp, "Dimension %d table for %s not printed",
			 s->arity-1, s->name);
	}
      }
      else {
		
	switch (s->arity) {
	case 0: print_table_r0_portable(fp, s); break;
	case 1: print_table_r1_portable(fp, s); break;
	case 2: print_table_r2_portable(fp, s); break;
	case 3: print_table_r3_portable(fp, s); break;
	case 4: print_table_r4_portable(fp, s); break;
	default: fprintf(fp, "Dimension %d table for %s not printed\n",
			 s->arity, s->name);
	}
      }
      syms_printed++;
    }
  }

  fprintf(fp, "\n]).\n");

  fprintf(fp, "end_of_model\n");
}  /* print_model_portable */

/*************
 *
 *   print_table_f0_ivy()
 *
 *************/

static void print_table_f0_ivy(FILE *fp, struct symbol *s)
{
  fprintf(fp, "   ((%s . 0) . ((nil . %d)))\n",s->name,function_value_0(s));
}  /* print_table_f0_ivy */

/*************
 *
 *   print_table_f1_ivy()
 *
 *************/

static void print_table_f1_ivy(FILE *fp, struct symbol *s)
{
  int i;
  int n0 = s->args[0].n;

  fprintf(fp, "   ((%s . 1) . (\n", s->name);
  for (i = 0; i < n0; i++) {
    fprintf(fp, "               ((%d) . %d)\n", i, function_value_1(s, i));
  }
  fprintf(fp, "              ))\n");
}  /* print_table_f1_ivy */

/*************
 *
 *   print_table_f2_ivy()
 *
 *************/

static void print_table_f2_ivy(FILE *fp, struct symbol *s)
{
  int i, j;
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;

  fprintf(fp, "   ((%s . 2) . (\n", s->name);
  for (i = 0; i < n0; i++) {
    for (j = 0; j < n1; j++) {
      fprintf(fp, "               ((%d %d) . %d)\n", i, j, function_value_2(s, i, j));
    }
  }
  fprintf(fp, "              ))\n");
}  /* print_table_f2_ivy */

/*************
 *
 *   print_table_f3_ivy()
 *
 *************/

static void print_table_f3_ivy(FILE *fp, struct symbol *s)
{
  int i, j, k;
  int n0 = s->args[0].n;
  int n1 = s->args[1].n;
  int n2 = s->args[2].n;

  fprintf(fp, "   ((%s . 3) . (\n", s->name);
  for (i = 0; i < n0; i++) {
    for (j = 0; j < n1; j++) {
      for (k = 0; k < n2; k++) {
	fprintf(fp, "               ((%d %d %d) . %d)\n",i,j,k,function_value_3(s, i, j, k));
      }
    }
  }
  fprintf(fp, "              ))\n");
}  /* print_table_f3_ivy */

/*************
 *
 *   print_table_r0_ivy()
 *
 *************/

static void print_table_r0_ivy(FILE *fp, struct symbol *s)
{
  int v;
  char *s1;
  v = atom_value(ATOM0(s));
  switch(v) {
  case 0: s1 = "nil"; break;
  case 1: s1 = "t"; break;
  case 2: s1 = "nil"; break;
  default: s1 = "?"; break;
  }
  fprintf(fp, "   ((%s . 0) . ((nil . %s)))\n", s->name, s1);
}  /* print_table_r0_ivy */

/*************
 *
 *   print_table_r1_ivy()
 *
 *************/

static void print_table_r1_ivy(FILE *fp, struct symbol *s)
{
  int i, v;
  char *s1;
  int n0=s->args[0].n;

  fprintf(fp, "   ((%s . 1) . (\n", s->name);
  for (i = 0; i < n0; i++) {
    v = atom_value(ATOM1(s, i));
    switch(v) {
    case 0: s1 = "nil"; break;
    case 1: s1 = "t"; break;
    case 2: s1 = "nil"; break;
    default: s1 = "nil"; break;
    }
    fprintf(fp, "               ((%d) . %s)\n", i, s1);
  }
  fprintf(fp, "              ))\n");
}  /* print_table_r1_ivy */

/*************
 *
 *   print_table_r2_ivy()
 *
 *************/

static void print_table_r2_ivy(FILE *fp, struct symbol *s)
{
  int i, j, v;
  char *s1;
  int n0=s->args[0].n;
  int n1=s->args[1].n;

  fprintf(fp, "   ((%s . 2) . (\n", s->name);
  for (i = 0; i < n0; i++) {
    for (j = 0; j < n1; j++) {
      v = atom_value(ATOM2(s, i, j));
      switch(v) {
      case 0: s1 = "nil"; break;
      case 1: s1 = "t"; break;
      case 2: s1 = "nil"; break;
      default: s1 = "nil"; break;
      }
      fprintf(fp, "               ((%d %d) . %s)\n", i, j, s1);
    }
  }
  fprintf(fp, "              ))\n");
}  /* print_table_r2_ivy */

/*************
 *
 *   print_table_r3_ivy()
 *
 *************/

static void print_table_r3_ivy(FILE *fp, struct symbol *s)
{
  int i, j, k, v;
  char *s1;
  int n0=s->args[0].n;
  int n1=s->args[1].n;
  int n2=s->args[2].n;

  fprintf(fp, "   ((%s . 3) . (\n", s->name);
  for (i = 0; i < n0; i++) {
    for (j = 0; j < n1; j++) {
      for (k = 0; k < n2; k++) {
	v = atom_value(ATOM3(s, i, j, k));
	switch(v) {
	case 0: s1 = "nil"; break;
	case 1: s1 = "t"; break;
	case 2: s1 = "nil"; break;
	default: s1 = "nil"; break;
	}
	fprintf(fp, "               ((%d %d %d) . %s)\n", i, j, k, s1);
      }
    }
  }
  fprintf(fp, "              ))\n");
}  /* print_table_r3_ivy */

/*************
 *
 *   print_table_r4_ivy()
 *
 *************/

static void print_table_r4_ivy(FILE *fp, struct symbol *s)
{
  int i, j, k, l, v;
  char *s1;
  int n0=s->args[0].n;
  int n1=s->args[1].n;
  int n2=s->args[2].n;
  int n3=s->args[3].n;

  fprintf(fp, "   ((%s . 4) . (\n", s->name);
  for (i = 0; i < n0; i++) {
    for (j = 0; j < n1; j++) {
      for (k = 0; k < n2; k++) {
	for (l = 0; l < n3; l++) {
	  v = atom_value(ATOM4(s, i, j, k, l));
	  switch(v) {
	  case 0: s1 = "nil"; break;
	  case 1: s1 = "t"; break;
	  case 2: s1 = "nil"; break;
	  default: s1 = "nil"; break;
	  }
	  fprintf(fp, "               ((%d %d %d %d) . %s)\n", i, j, k, l, s1);
	}
      }
    }
  }
  fprintf(fp, "              ))\n");
}  /* print_table_r4_ivy */

/*************
 *
 *   print_model_ivy()
 *
 *   Print the current model as a first-order model.  The DP code
 *   knows about the propositional model, so we call atom_value()
 *   to get the information to print the first-order model.
 *
 *   Assume unassigned atoms are true, because dp determines that
 *   we have a model when all remaining clauses are positive.
 *
 *   Don't print equality relation "=".
 *
 *************/

void print_model_ivy(FILE *fp)
{
  int i, domain_size;
  struct symbol *s;
  static int count;

  domain_size = Sorts[0].n;

  fprintf(fp, "\n======================= Model #%d at %.2f seconds:\n",
	  ++count, run_time() / 1000.);
  fprintf(fp, "\n;; BEGINNING OF IVY MODEL\n");

  if (domain_size == 1)
    fprintf(fp, "(0 .\n");
  else {
    int i;
    fprintf(fp, "((");
    for (i = 0; i < domain_size-1; i++)
      fprintf(fp, "%d ", i);
    fprintf(fp, ". %d) .\n (\n  (\n", domain_size-1);
  }

  for (i = 0; i < Num_symbols; i++) {
    s = Symbols + i;
    if (s->type == RELATION) {
      if (!(TP_BIT(s->properties, MACE_EQ_BIT) && str_ident(s->name, "=")) &&
	  !initial_str("$Connect", s->name)) {
	switch (s->arity) {
	case 0: print_table_r0_ivy(fp, s); break;
	case 1: print_table_r1_ivy(fp, s); break;
	case 2: print_table_r2_ivy(fp, s); break;
	case 3: print_table_r3_ivy(fp, s); break;
	case 4: print_table_r4_ivy(fp, s); break;
	default: MACE_abend("print_model_ivy, max relation arity is 4");
	}
      }
    }
  }

  fprintf(fp, "  )\n  .\n  (\n");

  for (i = 0; i < Num_symbols; i++) {
    s = Symbols + i;
    if (s->type == FUNCTION) {
      switch (s->arity) {
      case 1: print_table_f0_ivy(fp, s); break;
      case 2: print_table_f1_ivy(fp, s); break;
      case 3: print_table_f2_ivy(fp, s); break;
      case 4: print_table_f3_ivy(fp, s); break;
      default: MACE_abend("print_model_ivy, max function arity is 3");
      }
    }
  }

  fprintf(fp, "  )\n )\n)\n");
  fprintf(fp, ";; END OF IVY MODEL\n");
}  /* print_model_ivy */
