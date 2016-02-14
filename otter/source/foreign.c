/*
 *  foreign.c -- routines for interface to user-supplied evaluable functions
 *
 */

#include "header.h"

/*************
 *
 *  foo
 *
 *************/

long foo(long int l,
	 double d,
	 char *s)
{
  printf("enter foo with args: %ld %f %s.\n", l, d, s);
  return(l+d);
}  /* foo */

/*************
 *
 *  user_test_long
 *
 *************/

long user_test_long(long int l,
		    double d,
		    int b,
		    char *s,
		    struct term *t)
{
  printf("enter user_test_long: %ld %f %d %s ", l, d, b, s);
  p_term(t);
  return(l+3);
}  /* user_test_long */

/*************
 *
 *  user_test_double
 *
 *************/

double user_test_double(long int l,
			double d,
			int b,
			char *s,
			struct term *t)
{
  printf("enter user_test_double: %ld %f %d %s ", l, d, b, s);
  p_term(t);
  return(d+3.0);
}  /* user_test_double */

/*************
 *
 *  user_test_bool
 *
 *************/

int user_test_bool(long int l,
		   double d,
		   int b,
		   char *s,
		   struct term *t)
{
  printf("enter user_test_bool: %ld %f %d %s ", l, d, b, s);
  p_term(t);
  return(!b);
}  /* user_test_bool */

/*************
 *
 *  user_test_string
 *
 *************/

char *user_test_string(long int l,
		       double d,
		       int b,
		       char *s,
		       struct term *t)
{
  printf("enter user_test_string: %ld %f %d %s ", l, d, b, s);
  p_term(t);
  return("\"Returned string\"");
}  /* user_test_string */

/*************
 *
 *  user_test_term
 *
 *************/

struct term *user_test_term(long int l,
			    double d,
			    int b,
			    char *s,
			    struct term *t)
{
  struct term *t1;
  printf("enter user_test_term: %ld %f %d %s ", l, d, b, s);
  p_term(t);
  t1 = get_term(); t1->type = NAME; t1->sym_num = str_to_sn("new_term", 0);
  return(t1);

}  /* user_test_term */

/*************
 *
 *   declare_user_functions()
 *
 *************/

void declare_user_functions(void)
{
  struct sym_ent *se;
  struct user_function *p;

  /*  Here is an example of how to declare a function.

      START OF TEMPLATE  (note that arity is specified twice)

      se = insert_sym("$FOO_BAR", 5);   se->eval_code = FOO_BAR_FUNC;
      p = &(User_functions[FOO_BAR_FUNC]);
      p->arity = 5;
      p->arg_types[0] = LONG_TYPE;
      p->arg_types[1] = DOUBLE_TYPE;
      p->arg_types[2] = BOOL_TYPE;
      p->arg_types[3] = STRING_TYPE;
      p->arg_types[4] = TERM_TYPE;
      p->result_type = LONG_TYPE;

      END OF TEMPLATE
  */

  /********************************/
  se = insert_sym("$FOO", 3);   se->eval_code = FOO_FUNC;
  p = &(User_functions[FOO_FUNC]);
  p->arity = 3;
  p->arg_types[0] = LONG_TYPE;
  p->arg_types[1] = DOUBLE_TYPE;
  p->arg_types[2] = STRING_TYPE;
  p->result_type = LONG_TYPE;

  /********************************/
  se = insert_sym("$TEST_LONG", 5);   se->eval_code = TEST_LONG_FUNC;
  p = &(User_functions[TEST_LONG_FUNC]);
  p->arity = 5;
  p->arg_types[0] = LONG_TYPE;
  p->arg_types[1] = DOUBLE_TYPE;
  p->arg_types[2] = BOOL_TYPE;
  p->arg_types[3] = STRING_TYPE;
  p->arg_types[4] = TERM_TYPE;
  p->result_type = LONG_TYPE;

  /********************************/
  se = insert_sym("$TEST_DOUBLE", 5);   se->eval_code = TEST_DOUBLE_FUNC;
  p = &(User_functions[TEST_DOUBLE_FUNC]);
  p->arity = 5;
  p->arg_types[0] = LONG_TYPE;
  p->arg_types[1] = DOUBLE_TYPE;
  p->arg_types[2] = BOOL_TYPE;
  p->arg_types[3] = STRING_TYPE;
  p->arg_types[4] = TERM_TYPE;
  p->result_type = DOUBLE_TYPE;

  /********************************/
  se = insert_sym("$TEST_BOOL", 5);   se->eval_code = TEST_BOOL_FUNC;
  p = &(User_functions[TEST_BOOL_FUNC]);
  p->arity = 5;
  p->arg_types[0] = LONG_TYPE;
  p->arg_types[1] = DOUBLE_TYPE;
  p->arg_types[2] = BOOL_TYPE;
  p->arg_types[3] = STRING_TYPE;
  p->arg_types[4] = TERM_TYPE;
  p->result_type = BOOL_TYPE;
  /********************************/

  se = insert_sym("$TEST_STRING", 5);   se->eval_code = TEST_STRING_FUNC;
  p = &(User_functions[TEST_STRING_FUNC]);
  p->arity = 5;
  p->arg_types[0] = LONG_TYPE;
  p->arg_types[1] = DOUBLE_TYPE;
  p->arg_types[2] = BOOL_TYPE;
  p->arg_types[3] = STRING_TYPE;
  p->arg_types[4] = TERM_TYPE;
  p->result_type = STRING_TYPE;
  /********************************/

  se = insert_sym("$TEST_TERM", 5);   se->eval_code = TEST_TERM_FUNC;
  p = &(User_functions[TEST_TERM_FUNC]);
  p->arity = 5;
  p->arg_types[0] = LONG_TYPE;
  p->arg_types[1] = DOUBLE_TYPE;
  p->arg_types[2] = BOOL_TYPE;
  p->arg_types[3] = STRING_TYPE;
  p->arg_types[4] = TERM_TYPE;
  p->result_type = TERM_TYPE;
  /********************************/

}  /* declare_user_functions */

/*************
 *
 *   int get_args_for_user_function
 *
 *************/

int get_args_for_user_function(struct term *t,
			       int op_code,
			       long int *long_args,
			       double *double_args,
			       int *bool_args,
			       char **string_args,
			       struct term **term_args)
{
  int i;
  long l;
  double d;
  struct rel *r;
  struct term *ti;
  struct user_function *p;
  char *s;

  p = &(User_functions[op_code]);

  for (r = t->farg, i=0; r; r = r->narg, i++);

  if (i != p->arity) {
    abend("get_args, bad arity.");
  }

  for (r = t->farg, i=0; r; r = r->narg, i++) {
    ti = r->argval;
    switch (p->arg_types[i]) {
    case LONG_TYPE:
      if (ti->type != NAME)
	return(0);
      else if (!str_long(sn_to_str(ti->sym_num), &l))
	return(0);
      else
	long_args[i] = l;
      break;
    case DOUBLE_TYPE:
      if (ti->type != NAME)
	return(0);
      else if (!str_double(sn_to_str(ti->sym_num), &d))
	return(0);
      else
	double_args[i] = d;
      break;
    case BOOL_TYPE:
      if (ti->type != NAME)
	return(0);
      else {
	s = sn_to_str(ti->sym_num);
	if (str_ident(s,"$T"))
	  bool_args[i] = 1;
	else if (str_ident(s,"$F"))
	  bool_args[i] = 0;
	else
	  return(0);
      }
      break;
    case STRING_TYPE:
      if (ti->type != NAME)
	return(0);
      else
	string_args[i] = sn_to_str(ti->sym_num);
      break;
    case TERM_TYPE:
      term_args[i] = ti;
      break;
    default:
      abend("get_args, bad arg type.");
    }
  }
  return(1);
}  /* get_args_for_user_function */

/*************
 *
 *    long_to_term -- Build a constant (NAME) term corresp. to a C long.
 *
 *************/

struct term *long_to_term(long int i)
{
  struct term *t;
  char s[MAX_NAME];

  t = get_term();
  t->type = NAME;
  long_str(i, s);
  t->sym_num = str_to_sn(s, 0);
  return(t);
}  /* long_to_term */

/*************
 *
 *    double_to_term -- Build a constant (NAME) term corresp. to a C double.
 *
 *************/

struct term *double_to_term(double d)
{
  struct term *t;
  char s[MAX_NAME];

  t = get_term();
  t->type = NAME;
  double_str(d, s);
  t->sym_num = str_to_sn(s, 0);
  return(t);
}  /* double_to_term */

/*************
 *
 *    bool_to_term -- Build a constant (NAME) term corresp. to a C boolean.
 *
 *************/

struct term *bool_to_term(int i)
{
  struct term *t;

  t = get_term();
  t->type = NAME;
  t->sym_num = str_to_sn(i ? "$T" : "$F", 0);
  return(t);
}  /* bool_to_term */

/*************
 *
 *    string_to_term -- Build a constant (NAME) term corresp. to a string.
 *
 *************/

struct term *string_to_term(char *s)
{
  struct term *t;

  t = get_term();
  t->type = NAME;
  t->sym_num = str_to_sn(s, 0);
  return(t);
}  /* string_to_term */

/*************
 *
 *   evaluate_user_function
 *
 *************/

struct term *evaluate_user_function(struct term *t,
				    int op_code)
{
  long long_args[MAX_USER_ARGS];
  double double_args[MAX_USER_ARGS];
  int bool_args[MAX_USER_ARGS];
  char *string_args[MAX_USER_ARGS];
  struct term *term_args[MAX_USER_ARGS];

  long l;
  double d;
  int b;
  char *s;
  struct term *t1;

  if (!get_args_for_user_function(t, op_code, long_args, double_args,
				  bool_args, string_args, term_args))
    return(NULL);  /* arg types wrong, so do nothing */
  else {
    switch (op_code) {

      /*        Here is an example of how to call your function.  This example
		takes one arg of each type.  The call for a function taking
		two doubles and returning a double would be
		d = foo_bar(double_args[0],double_args[1]);
		The routines to translate the C result to an Otter term are
		long_to_term, double_to_term, bool_to_term, and string_to_term.

		START OF TEMPLATE

		case FOO_BAR_FUNC:
		l = foo_bar(long_args[0],
		double_args[1],
		bool_args[2],
		string_args[3],
		term_args[4]);

		return(long_to_term(l));

		END OF TEMPLATE
      */
      /******************************/
    case FOO_FUNC:
      l = foo(long_args[0],
	      double_args[1],
	      string_args[2]);

      return(long_to_term(l));

      /******************************/
    case TEST_LONG_FUNC:
      l = user_test_long(long_args[0],
			 double_args[1],
			 bool_args[2],
			 string_args[3],
			 term_args[4]);

      return(long_to_term(l));

      /******************************/
    case TEST_DOUBLE_FUNC:
      d = user_test_double(long_args[0],
			   double_args[1],
			   bool_args[2],
			   string_args[3],
			   term_args[4]);

      return(double_to_term(d));

      /******************************/
    case TEST_BOOL_FUNC:
      b = user_test_bool(long_args[0],
			 double_args[1],
			 bool_args[2],
			 string_args[3],
			 term_args[4]);

      return(bool_to_term(b));

      /******************************/
    case TEST_STRING_FUNC:
      s = user_test_string(long_args[0],
			   double_args[1],
			   bool_args[2],
			   string_args[3],
			   term_args[4]);

      return(string_to_term(s));

      /******************************/
    case TEST_TERM_FUNC:
      t1 = user_test_term(long_args[0],
			  double_args[1],
			  bool_args[2],
			  string_args[3],
			  term_args[4]);

      return(t1);

      /******************************/

    default:
      abend("evaluate_user_function, bad code.");
    }
    return(NULL);  /* to quiet lint */
  }
}  /* evaluate_user_function */


