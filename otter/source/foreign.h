/*
 *  foreign.h -- header file for user-defined foreign functions
 *
 */

/*
         HOW TO WRITE YOUR OWN C ROUTINE WHICH CAN BE
     CALLED TO DEMODULATE A TERM DURING OTTER'S DEMODULATION

The Otter types don't correspond exactly to the C types. In Otter, the
types, except `term', are all constants (in the theorem-proving sense),
and in C, the types are what you would expect them to be.

TYPES:
 - long     In C and in Otter, a long integer.
 - double   In C, a double; in Otter, a double surrounded by double quotes.
 - bool     In C, an int; in Otter, $T or $F.
 - string   In C, a pointer to a string; in Otter, any constant.
 - term     An Otter term.

To include a new function, you have to (1) declare the function,
argument types, and result type, (2) insert a call to your function in
the Otter code, (3) write your C routine, and (4) remake Otter.
The only Otter files you need to change are foreign.h and foreign.c.

When Otter is demodulating and comes across a 'call' to your function,
it will first check the argument types.  If they are incorrect (for
example a variable is not instantiated) the term will not be
rewritten; if the arguments are OK, your C routine will be called, and
the result of your C routine will be made into an appropriate Otter
term which is the result of the rewriting step.

(Don't forget that many times you can avoid having to do all of
this by just writing your function with demodulators and using
existing built in functions.  For example, if you need the
max of two doubles, you can just use the demodulator
float_max(x,y) = $IF($FGT(x,y), x, y).  For other examples of
programming with demodulators, see eval.in in the Otter test suite.
See file demod.c, routine dollar_contract, for the current set
of evaluable functions.)

STEP-BY-STEP INSTRUCTIONS

0.  Say you want to write a function foo that takes
    a long, a double, and a string as arguments and produces a long, i.e.,
           
           long foo(long n, double x, char *s)

    The Otter-language name must start with $, say $FOO, and you
    will be evaluating Otter terms like $FOO(32, "-4.5e-10", flag32)

1.  To declare the function and its types,

    (A) In the file foreign.h, #define a symbol, which will be used
        as an index into a table (see FOO_FUNC in foreign.h).

    (B) In file foreign.c, in routine declare_user_functions, declare
        the function and its types (look for FOO_FUNC in foreign.c).
        The types are LONG_TYPE, DOUBLE_TYPE, BOOL_TYPE, STRING_TYPE,
        and TERM_TYPE.

2.  In file foreign.c, in routine evaluate_user_function, insert a call
    to your function and a call to translate the result to an Otter
    term (look for FOO_FUNC in foreign.c).  The routines to translate
    C types to otter terms are long_to_term, double_to_term, bool_to_term,
    and string_to_term.  See foreign.c for examples.

3.  Write your C routine.  I have inserted the test routines into
    foreign.c, but you may wish to organize yours into separate
    files.  (If your routines involve TERM_TYPE, you must
    #include "header.h".)

4.  Remake Otter;  foreign.c is the only Otter file that will need
    to be recompiled.

5.  Test your new function by using the input file foreign.in.


*/

#define MAX_USER_ARGS    20    /* maximum arity for user-defined C functions */

#define LONG_TYPE         1    /* types for user-defined C functions */
#define DOUBLE_TYPE       2
#define BOOL_TYPE         3
#define STRING_TYPE       4
#define TERM_TYPE         5

struct user_function {    /* entry in table of user-defined functions */
    int arity;
    int arg_types[MAX_USER_ARGS];
    int result_type;
    }; 

#define MAX_USER_FUNCTIONS 100  /* size of table */

/* indexes into table of user-defined functions */

#define FOO_FUNC                 1
#define TEST_LONG_FUNC           2
#define TEST_DOUBLE_FUNC         3
#define TEST_BOOL_FUNC           4
#define TEST_STRING_FUNC         5
#define TEST_TERM_FUNC           6

