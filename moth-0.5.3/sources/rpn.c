/*
 * $Id: rpn.c,v 1.19 2003/09/15 03:07:14 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See rpn.dev for notes and coding issues.
 *
 */



#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <time.h>

#include "../data.h"
#include "../sources.h"



typedef enum {
  MOTH_RPN_UNKNOWN = 0,
  MOTH_RPN_CONSTANT,
  MOTH_RPN_BUILTIN,
  MOTH_RPN_COLUMN
} moth_rpn_entry_type;



/*--------------------------------------------------------*\
|                      datastructures                      |
\*--------------------------------------------------------*/

/* list of tokens */
typedef struct moth_source_rpn_token_s    *moth_source_rpn_token;
struct moth_source_rpn_token_s {
  const char            *text;    /* string format */
  moth_rpn_entry_type   type;
  double                value;    /* if constant */
  moth_column           column;   /* if column */
  moth_source_rpn_token next;     /* linked list */
};
typedef struct moth_source_rpn_tokens_s   *moth_source_rpn_tokens;
struct moth_source_rpn_tokens_s {
  char                  *data;    /* text of tokens */
  moth_source_rpn_token tokens;   /* linked list */
  moth_source_rpn_token iterator;
  size_t                rows;     /* number of rows to process */
  size_t                source_columns; /* number of source columns read */
};


/* stack of values */
typedef struct moth_source_rpn_stack_entry_s    *moth_source_rpn_stack_entry;
struct moth_source_rpn_stack_entry_s {
  moth_source_rpn_token       token;
  double                      value;
  moth_source_rpn_stack_entry next;
};
typedef struct moth_source_rpn_stack_s    *moth_source_rpn_stack;
struct moth_source_rpn_stack_s {
  int                         size;
  moth_source_rpn_stack_entry top;
};



/*--------------------------------------------------------*\
|                   tokenizer functions                    |
\*--------------------------------------------------------*/

INTERNAL
void
moth_source_rpn_token_categorize
(
  moth_source_rpn_token token
)
{
  int has_alpha, has_lower;
  size_t length, c;

  /* constant */
  token->value = read_value (token->text);
  if (!isnan(token->value)) {
    token->type = MOTH_RPN_CONSTANT;
    return;
  }

  has_alpha = has_lower = 0;
  length = strlen (token->text);
  for (c=0; c<length; ++c) {
    if (isalpha(token->text[c]))
      has_alpha = 1;
    if (islower(token->text[c]))
      has_lower = 1;
  }

  /* builtin operator or function */
  if (!has_alpha || !has_lower) {
    token->type = MOTH_RPN_BUILTIN;
  }

  /* column */
  else {
    moth_column column;
    column = moth_column_store_get (runtime->file->columns, token->text);
    if (!column)
      die ("unknown column \"%s\" in <rpn>");
    token->type = MOTH_RPN_COLUMN;
    token->column = column;
  }
}



INTERNAL
moth_source_rpn_tokens
moth_source_rpn_tokens_create
(
  moth_buffer buffer
)
{
  moth_source_rpn_tokens tokens;
  moth_source_rpn_token  token, last_token;
  size_t characters, i;
  int is_space, last_is_space;

  tokens = (moth_source_rpn_tokens)
    malloc (sizeof(struct moth_source_rpn_tokens_s));
  tokens->data = NULL;
  tokens->tokens = NULL;
  tokens->iterator = NULL;
  tokens->rows = 1;   /* always process at least one row */
  tokens->source_columns = 0;

  /* 
   * We manage our own copy of the buffer because we're actually going to
   * modify it.  We'll change the space after each token into a null character
   * so that a pointer to the first character of the token can be treated as a
   * C string.
   */
  tokens->data = moth_buffer_strdup (buffer);
  if (!tokens->data)
    return tokens;
  characters = strlen (tokens->data);

  last_token = NULL;
  last_is_space = 1;  /* triggers detection of first token */
  for (i=0; i<characters; ++i) {
    char *c;
    c = tokens->data + i;
    is_space = isspace (*c);

    /* beginning of token */
    if (last_is_space && !is_space) {
      token = (moth_source_rpn_token)
        malloc (sizeof(struct moth_source_rpn_token_s));
      token->text = c;
      token->type = MOTH_RPN_UNKNOWN;
      token->value = NAN;
      token->column = NULL;
      token->next = NULL;

      /* add to END of linked list */
      if (tokens->tokens)
        last_token->next = token;
      else
        tokens->tokens = token;

      last_token = token;
    }

    /* end of token */
    else if ( !last_is_space && is_space) {
      *c = '\0';
      moth_source_rpn_token_categorize (last_token);
      if (MOTH_RPN_COLUMN == last_token->type) {
        size_t column_rows;
        column_rows = moth_column_size (last_token->column);
        if (column_rows > tokens->rows)
          tokens->rows = column_rows;
        ++tokens->source_columns;
      }
    }

    last_is_space = is_space;
  }

  /* If the data ends with a token, we need to handle that last token.  */
  if (last_token && !last_is_space) {
    moth_source_rpn_token_categorize (last_token);
    if (MOTH_RPN_COLUMN == last_token->type) {
      size_t column_rows;
      column_rows = moth_column_size (last_token->column);
      if (column_rows > tokens->rows)
        tokens->rows = column_rows;
      ++tokens->source_columns;
    }
  }

  return tokens;
}



INTERNAL
void
moth_source_rpn_tokens_reset
(
  moth_source_rpn_tokens tokens
)
{
  tokens->iterator = tokens->tokens;
}



INTERNAL
moth_source_rpn_token
moth_source_rpn_tokens_current
(
  moth_source_rpn_tokens tokens
)
{
  if (!tokens) return NULL;
  return tokens->iterator;
}



/* returns true if there is more */
INTERNAL
void
moth_source_rpn_tokens_next
(
  moth_source_rpn_tokens tokens
)
{
  if (!tokens) return;
  if (!tokens->iterator) return;
  tokens->iterator = tokens->iterator->next;
}



INTERNAL
void
moth_source_rpn_tokens_destroy
(
  moth_source_rpn_tokens tokens
)
{
  moth_source_rpn_token doomed, t;
  if (!tokens) return;
  if (tokens->tokens) {
    t = tokens->tokens;
    while (t) {
      doomed = t;
      t = t->next;
      /* DO NOT free pointer fields inside doomed token */
      free (doomed);
    }
  }
  if (tokens->data)
    free (tokens->data);
  free (tokens);
}



/*--------------------------------------------------------*\
|                     stack functions                      |
\*--------------------------------------------------------*/

INTERNAL
moth_source_rpn_stack_entry
moth_source_rpn_stack_entry_create
(
  moth_source_rpn_token token
)
{
  moth_source_rpn_stack_entry entry;
  entry = (moth_source_rpn_stack_entry)
    malloc (sizeof(struct moth_source_rpn_stack_entry_s));
  if (!entry)
    return NULL;
  entry->token = token;
  entry->value = NAN;
  entry->next = NULL;
  return entry;
}



INTERNAL
void
moth_source_rpn_stack_entry_destroy
(
  moth_source_rpn_stack_entry entry
)
{
  /* 
   * Yup, that's all there is to it.  The 'token' field is an object that's
   * actually managed by someone else (moth_source_rpn_tokens->tokens).
   */
  free (entry);
}



INTERNAL
double
moth_source_rpn_stack_entry_value
(
  moth_source_rpn_stack_entry entry,
  size_t                      row
)
{
  /* lazy evaluation of special cases */
  if (entry->token->type == MOTH_RPN_BUILTIN &&
      0==strcmp("ABORT",entry->token->text)) {
    die ("user called ABORT in <rpn>");
  }
  if (entry->token->type == MOTH_RPN_COLUMN)
    return moth_column_value (entry->token->column, row);

  /* a constant */
  return entry->value;
}



INTERNAL
moth_source_rpn_stack
moth_source_rpn_stack_create (void)
{
  moth_source_rpn_stack stack;
  stack = (moth_source_rpn_stack)
    malloc (sizeof(struct moth_source_rpn_stack_s));
  stack->size = 0;
  stack->top = NULL;
  return stack;
}



/* Use when pushing a _pop_entry()ed entry back on the
 * stack (such as for IF).  */
INTERNAL
void
moth_source_rpn_stack_push_entry
(
  moth_source_rpn_stack       stack,
  moth_source_rpn_stack_entry entry
)
{
  entry->next = stack->top;
  stack->top = entry;
  stack->size += 1;
}



INTERNAL
void
moth_source_rpn_stack_push_value
(
  moth_source_rpn_stack stack,
  double                value,
  moth_source_rpn_token token
)
{
  moth_source_rpn_stack_entry e;
  if (!token)
    die ("internal error:  value must be associated with a token");
  e = moth_source_rpn_stack_entry_create (token);
  e->value = value;
  moth_source_rpn_stack_push_entry (stack, e);
}



INTERNAL
moth_source_rpn_stack_entry
moth_source_rpn_stack_pop_entry
(
  moth_source_rpn_stack stack,
  moth_source_rpn_token where   /* for error message */
)
{
  moth_source_rpn_stack_entry e;
  if (! stack->top) {
    if (where) {
      die ("<rpn> stack underflow while trying to interpret \"%s\"",
          where->text);
    }
    die ("<rpn> stack underflow");
  }
  e = stack->top;
  stack->top = stack->top->next;
  stack->size -= 1;
  e->next = NULL;
  return e;
}



INTERNAL
double
moth_source_rpn_stack_pop_value
(
  moth_source_rpn_stack stack,
  size_t                row,
  moth_source_rpn_token where
)
{
  moth_source_rpn_stack_entry e;
  double value;
  e = moth_source_rpn_stack_pop_entry (stack, where);
  value = moth_source_rpn_stack_entry_value (e, row);
  moth_source_rpn_stack_entry_destroy (e);
  return value;
}



/* constant, operator, or function call */
INTERNAL
void
moth_source_rpn_stack_call_builtin
(
  moth_source_rpn_stack   stack,
  moth_source_rpn_tokens  tokens,
  size_t                  row
)
{
  moth_source_rpn_token token;
  token = moth_source_rpn_tokens_current (tokens);

  /* OK, here the fun begins .... */
  errno = 0;

  /* IGNORE */
  if (    (0==strcmp("(",token->text))
       || (0==strcmp(")",token->text))) {
    /* do nothing */
  }

  /* NAMED CONSTANTS */
  else if (0==strcmp("E", token->text)) {
    moth_source_rpn_stack_push_value (stack, M_E, token);
  }
  else if (0==strcmp("PI", token->text)) {
    moth_source_rpn_stack_push_value (stack, M_PI, token);
  }
  else if (0==strcmp("SQRT2", token->text)) {
    moth_source_rpn_stack_push_value (stack, M_SQRT2, token);
  }
  else if (0==strcmp("NAN", token->text)) {
    moth_source_rpn_stack_push_value (stack, NAN, token);
  }
  else if (0==strcmp("INF", token->text)) {
    moth_source_rpn_stack_push_value (stack, HUGE_VAL, token);
  }
  else if (0==strcmp("NEGINF", token->text)) {
    moth_source_rpn_stack_push_value (stack, -HUGE_VAL, token);
  }

  /* OPERATORS */
  else if (0==strcmp("+", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, (x+y), token);
  }
  else if (0==strcmp("-", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, (x-y), token);
  }
  else if (0==strcmp("*", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, (x*y), token);
  }
  else if (0==strcmp("/", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, (x/y), token);
  }
  else if (0==strcmp("^", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, pow(x,y), token);
  }
  else if (0==strcmp("\\", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, trunc(x/y), token);
  }
  else if (0==strcmp("%", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, fmod(x,y), token);
  }

  /* TESTS */
  else if (0==strcmp("ISNAN", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, (isnan(x)?1.0:0.0), token);
  }
  else if (0==strcmp("ISINF", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, (isinf(x)?1.0:0.0), token);
  }
  else if (0==strcmp("<", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, ((x<y)?1.0:0.0), token);
  }
  else if (0==strcmp(">", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, ((x>y)?1.0:0.0), token);
  }
  else if (0==strcmp("==", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, ((x==y)?1.0:0.0), token);
  }
  else if (0==strcmp("~=", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack,
        ((fabs(x-y)<MOTH_DOUBLE_EPSILON)?1.0:0.0), token);
  }
  else if (0==strcmp("!=", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, ((x!=y)?1.0:0.0), token);
  }
  else if (0==strcmp("<=", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, ((x<=y)?1.0:0.0), token);
  }
  else if (0==strcmp(">=", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, ((x>=y)?1.0:0.0), token);
  }
  else if (0==strcmp("GT", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, ((x>y)?1.0:0.0), token);
  }
  else if (0==strcmp("GE", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, ((x>=y)?1.0:0.0), token);
  }
  else if (0==strcmp("LT", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, ((x<y)?1.0:0.0), token);
  }
  else if (0==strcmp("LE", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, ((x<=y)?1.0:0.0), token);
  }
  else if (0==strcmp("ISFIRST", token->text)) {
    moth_source_rpn_stack_push_value (stack, ((row==0)?1.0:0.0), token);
  }
  else if (0==strcmp("ISLAST", token->text)) {
    size_t rows;
    rows = tokens->rows - 1;
    moth_source_rpn_stack_push_value (stack, ((row==rows)?1.0:0.0), token);
  }
  else if (0==strcmp("BETWEEN", token->text)) {
    double x, y, z;
    z = moth_source_rpn_stack_pop_value (stack, row, token);
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, (((y<=x)&&(x<=z))?1.0:0.0),
        token);
  }

  /* LOGIC */
  else if (0==strcmp("IF", token->text)) {
    moth_source_rpn_stack_entry y, z, good, bad;
    double x_val;
    x_val = moth_source_rpn_stack_pop_value (stack, row, token);
    y = moth_source_rpn_stack_pop_entry (stack, token);
    z = moth_source_rpn_stack_pop_entry (stack, token);
    if (!isnan(x_val) && x_val) {
      good = y;
      bad = z;
    }
    else {
      good = z;
      bad = y;
    }
    moth_source_rpn_stack_push_entry (stack, good);
    moth_source_rpn_stack_entry_destroy (bad);
  }
  else if (0==strcmp("AND", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(y)) y = 0.0;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(x)) x = 0.0;
    moth_source_rpn_stack_push_value (stack, (x&&y), token );
  }
  else if (0==strcmp("OR", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(y)) y = 0.0;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(x)) x = 0.0;
    moth_source_rpn_stack_push_value (stack, (x||y), token );
  }
  else if (0==strcmp("NOT", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(x)) x = 0.0;
    moth_source_rpn_stack_push_value (stack, ((x==0.0)?1.0:0.0), token);
  }
  else if (0==strcmp("ABORT", token->text)) {
    /* Just push for now.  Lazy evaluation will abort
     * whenever we try to turn it into a value.  */
    moth_source_rpn_stack_push_value (stack, NAN, token);
  }

  /* COLUMN MANIPULATION */
  else if (0==strcmp("PREV", token->text)) {
    moth_source_rpn_stack_entry x, y;
    double x_val, ret;
    int r;
    y = moth_source_rpn_stack_pop_entry (stack, token);
    x = moth_source_rpn_stack_pop_entry (stack, token);
    if (y->token->type != MOTH_RPN_COLUMN) {
      die ("<rpn> PREV requires a column name, which \"%s\" isn't",
          y->token->text);
    }
    x_val = moth_source_rpn_stack_entry_value (x, row);
    r = row - ((int) x_val);
    if (r < 0)
      ret = NAN;
    else
      ret = moth_source_rpn_stack_entry_value (y, r);
    moth_source_rpn_stack_push_value (stack, ret, token);
    moth_source_rpn_stack_entry_destroy (x);
    moth_source_rpn_stack_entry_destroy (y);
  }
  else if (0==strcmp("ROW", token->text)) {
    moth_source_rpn_stack_push_value (stack, row+1, token);
  }
  else if (0==strcmp("SKIP", token->text)) {
    /* Just push for now.  Lazy evaluation will skip
     * whenever we try to turn it into a value.  */
    moth_source_rpn_stack_push_value (stack, NAN, token);
  }
  else if (0==strcmp("COLUMN_ROWS", token->text)
        || 0==strcmp("ROWS", token->text))
  {
    moth_source_rpn_stack_entry y;
    size_t rows;
    y = moth_source_rpn_stack_pop_entry (stack, token);
    rows = moth_column_size (y->token->column);
    moth_source_rpn_stack_push_value (stack, rows, token);
    moth_source_rpn_stack_entry_destroy (y);
  }
  else if (0==strcmp("COLUMN_MIN", token->text)) {
    moth_source_rpn_stack_entry y;
    double min;
    y = moth_source_rpn_stack_pop_entry (stack, token);
    min = moth_column_min (y->token->column, 1);
    moth_source_rpn_stack_push_value (stack, min, token);
    moth_source_rpn_stack_entry_destroy (y);
  }
  else if (0==strcmp("COLUMN_MAX", token->text)) {
    moth_source_rpn_stack_entry y;
    double max;
    y = moth_source_rpn_stack_pop_entry (stack, token);
    max = moth_column_max (y->token->column, 1);
    moth_source_rpn_stack_push_value (stack, max, token);
    moth_source_rpn_stack_entry_destroy (y);
  }
  else if (0==strcmp("COLUMN_SUM", token->text)) {
    moth_source_rpn_stack_entry y;
    size_t r, rows;
    double sum;
    y = moth_source_rpn_stack_pop_entry (stack, token);
    rows = moth_column_size (y->token->column);
    sum = 0;
    for (r=0; r<rows; ++r) {
      double val;
      val = moth_column_value (y->token->column, r);
      if (isnan(val)) continue;
      sum += val;
    }
    moth_source_rpn_stack_push_value (stack, sum, token);
    moth_source_rpn_stack_entry_destroy (y);
  }
  else if (0==strcmp("COLUMN_AVG", token->text)) {
    moth_source_rpn_stack_entry y;
    size_t r, rows, counted_rows;
    double sum;
    y = moth_source_rpn_stack_pop_entry (stack, token);
    rows = moth_column_size (y->token->column);
    sum = 0.0;
    counted_rows = 0;
    for (r=0; r<rows; ++r) {
      double val;
      val = moth_column_value (y->token->column, r);
      if (isnan(val)) continue;
      sum += val;
      ++counted_rows;
    }
    moth_source_rpn_stack_push_value (stack, sum/counted_rows, token);
    moth_source_rpn_stack_entry_destroy (y);
  }
  else if (0==strcmp("COLUMN_STD", token->text)) {
    moth_source_rpn_stack_entry y;
    size_t r, rows, counted_rows;
    double sum, avg, variance;
    y = moth_source_rpn_stack_pop_entry (stack, token);
    rows = moth_column_size (y->token->column);
    sum = 0.0;
    counted_rows = 0;
    for (r=0; r<rows; ++r) {
      double val;
      val = moth_column_value (y->token->column, r);
      if (isnan(val)) continue;
      sum += val;
      ++counted_rows;
    }
    avg = sum / counted_rows;
    variance = 0.0;
    for (r=0; r<rows; ++r) {
      double val;
      val = moth_column_value (y->token->column, r);
      if (isnan(val)) continue;
      variance += pow ((val-avg), 2.0);
    }
    variance /= counted_rows;
    moth_source_rpn_stack_push_value (stack, sqrt(variance), token);
    moth_source_rpn_stack_entry_destroy (y);
  }

  /* STACK MANIPULATION */
  else if (0==strcmp("COPY", token->text)) {
    double x;
    /* 
     * Note that this implies that it's NOT OK to copy lazy evaluated values
     * (ABORT, SKIP, etc).
     */
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, x, token);
    moth_source_rpn_stack_push_value (stack, x, token);
  }
  else if (0==strcmp("POP", token->text)) {
    moth_source_rpn_stack_entry x;
    x = moth_source_rpn_stack_pop_entry (stack, token);
    moth_source_rpn_stack_entry_destroy (x);
  }
  else if (0==strcmp("SWAP", token->text)) {
    moth_source_rpn_stack_entry x, y;
    x = moth_source_rpn_stack_pop_entry (stack, token);
    y = moth_source_rpn_stack_pop_entry (stack, token);
    moth_source_rpn_stack_push_entry (stack, x);
    moth_source_rpn_stack_push_entry (stack, y);
  }

  /* MATH FUNCTIONS */
  else if (0==strcmp("ABS", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, fabs(x), token);
  }
  else if (0==strcmp("CEIL", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, ceil(x), token);
  }
  else if (0==strcmp("FLOOR", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, floor(x), token);
  }
  else if (0==strcmp("TRUNC", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, trunc(x), token);
  }
  else if (0==strcmp("ROUND", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, round(x), token);
  }
  else if (0==strcmp("ROUNDEVEN", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, rint(x), token);
  }
  else if (0==strcmp("MOD", token->text)) {
    double x, y;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    x = modf (x, &y);
    moth_source_rpn_stack_push_value (stack, y, token);
    moth_source_rpn_stack_push_value (stack, x, token);
  }
  else if (0==strcmp("EXP", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, exp(x), token);
  }
  else if (0==strcmp("LOG", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, log(x), token);
  }
  else if (0==strcmp("LOG2", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    /* log2(x) = log(x)/log(2) */
    moth_source_rpn_stack_push_value (stack, (log(x)/log(2.0)), token);
  }
  else if (0==strcmp("LOG10", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, log10(x), token);
  }
  else if (0==strcmp("SQRT", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, sqrt(x), token);
  }
  else if (0==strcmp("CBRT", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, cbrt(x), token);
  }
  else if (0==strcmp("HYPOT", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, hypot(x,y), token);
  }
  else if (0==strcmp("SIGN", token->text)) {
    double x, ret;
    ret = NAN;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    if      (x <  0.0)  { ret = -1.0; }
    else if (x == 0.0)  { ret = 0.0; }
    else if (x >  0.0)  { ret = 1.0; }
    else                  { ret = NAN; }
    moth_source_rpn_stack_push_value (stack, ret, token);
  }
  else if (0==strcmp("COPYSIGN", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, copysign(y,x), token);
  }
  else if (0==strcmp("MAX", token->text)) {
    double x, y, ret;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    ret = (x>y) ? x : y;
    moth_source_rpn_stack_push_value (stack, ret, token);
  }
  else if (0==strcmp("MIN", token->text)) {
    double x, y, ret;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    ret = (x<y) ? x : y;
    moth_source_rpn_stack_push_value (stack, ret, token);
  }
  else if (0==strcmp("RANDOM", token->text)) {
    double x, ret;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    ret = x * random() / RAND_MAX;
    moth_source_rpn_stack_push_value (stack, ret, token);
  }

  else if (0==strcmp("DEGREES", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, x*180.0/M_PI, token);
  }
  else if (0==strcmp("RADIANS", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, x*M_PI/180.0, token);
  }
  else if (0==strcmp("SIN", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, sin(x), token);
  }
  else if (0==strcmp("COS", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, cos(x), token);
  }
  else if (0==strcmp("TAN", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, tan(x), token);
  }
  else if (0==strcmp("ASIN", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, asin(x), token);
  }
  else if (0==strcmp("ACOS", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, acos(x), token);
  }
  else if (0==strcmp("ATAN", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, atan(x), token);
  }
  else if (0==strcmp("ATAN2", token->text)) {
    double x, y;
    y = moth_source_rpn_stack_pop_value (stack, row, token);
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, atan2(x,y), token);
  }
  else if (0==strcmp("SINH", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, sinh(x), token);
  }
  else if (0==strcmp("COSH", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, cosh(x), token);
  }
  else if (0==strcmp("TANH", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, tanh(x), token);
  }
  else if (0==strcmp("ASINH", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, asinh(x), token);
  }
  else if (0==strcmp("ACOSH", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, acosh(x), token);
  }
  else if (0==strcmp("ATANH", token->text)) {
    double x;
    x = moth_source_rpn_stack_pop_value (stack, row, token);
    moth_source_rpn_stack_push_value (stack, atanh(x), token);
  }

  /* DATE MANIPULATION */
  else if (0==strcmp("SECOND", token->text)) {
#ifdef HAVE_LOCALTIME
    double    t;
    time_t    time;
    struct tm *time_s;
    t = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(t)) {
      moth_source_rpn_stack_push_value (stack, NAN, token);
    }
    else {
      time = t;
      time_s = localtime (&time);
      moth_source_rpn_stack_push_value (stack, time_s->tm_sec, token);
    }
#else
    warn ("<rpn> SECOND not supported because localtime() is not available");
#endif
  }
  else if (0==strcmp("MINUTE", token->text)) {
#ifdef HAVE_LOCALTIME
    double    t;
    time_t    time;
    struct tm *time_s;
    t = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(t)) {
      moth_source_rpn_stack_push_value (stack, NAN, token);
    }
    else {
      time = t;
      time_s = localtime (&time);
      moth_source_rpn_stack_push_value (stack, time_s->tm_min, token);
    }
#else
    warn ("<rpn> MINUTE not supported because localtime() is not available");
#endif
  }
  else if (0==strcmp("HOUR", token->text)) {
#ifdef HAVE_LOCALTIME
    double    t;
    time_t    time;
    struct tm *time_s;
    t = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(t)) {
      moth_source_rpn_stack_push_value (stack, NAN, token);
    }
    else {
      time = t;
      time_s = localtime (&time);
      moth_source_rpn_stack_push_value (stack, time_s->tm_hour, token);
    }
#else
    warn ("<rpn> HOUR not supported because localtime() is not available");
#endif
  }
  else if (0==strcmp("DAY", token->text)) {
#ifdef HAVE_LOCALTIME
    double    t;
    time_t    time;
    struct tm *time_s;
    t = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(t)) {
      moth_source_rpn_stack_push_value (stack, NAN, token);
    }
    else {
      time = t;
      time_s = localtime (&time);
      moth_source_rpn_stack_push_value (stack, time_s->tm_mday, token);
    }
#else
    warn ("<rpn> DAY not supported because localtime() is not available");
#endif
  }
  else if (0==strcmp("MONTH", token->text)) {
#ifdef HAVE_LOCALTIME
    double    t;
    time_t    time;
    struct tm *time_s;
    t = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(t)) {
      moth_source_rpn_stack_push_value (stack, NAN, token);
    }
    else {
      time = t;
      time_s = localtime (&time);
      moth_source_rpn_stack_push_value (stack, time_s->tm_mon + 1, token);
    }
#else
    warn ("<rpn> MONTH not supported because localtime() is not available");
#endif
  }
  else if (0==strcmp("YEAR", token->text)) {
#ifdef HAVE_LOCALTIME
    double    t;
    time_t    time;
    struct tm *time_s;
    t = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(t)) {
      moth_source_rpn_stack_push_value (stack, NAN, token);
    }
    else {
      time = t;
      time_s = localtime (&time);
      moth_source_rpn_stack_push_value (stack, time_s->tm_year + 1900, token);
    }
#else
    warn ("<rpn> YEAR not supported because localtime() is not available");
#endif
  }
  else if (0==strcmp("DAYOFWEEK", token->text)) {
#ifdef HAVE_LOCALTIME
    double    t;
    time_t    time;
    struct tm *time_s;
    t = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(t)) {
      moth_source_rpn_stack_push_value (stack, NAN, token);
    }
    else {
      time = t;
      time_s = localtime (&time);
      moth_source_rpn_stack_push_value (stack, 1+time_s->tm_wday, token);
    }
#else
    warn ("<rpn> DAYOFWEEK not supported because localtime() is not available");
#endif
  }
  else if (0==strcmp("DAYOFYEAR", token->text)) {
#ifdef HAVE_LOCALTIME
    double    t;
    time_t    time;
    struct tm *time_s;
    t = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(t)) {
      moth_source_rpn_stack_push_value (stack, NAN, token);
    }
    else {
      time = t;
      time_s = localtime (&time);
      moth_source_rpn_stack_push_value (stack, 1+time_s->tm_yday, token);
    }
#else
    warn ("<rpn> DAYOFYEAR not supported because localtime() is not available");
#endif
  }
  else if (0==strcmp("ADDMONTHS", token->text)) {
#ifdef HAVE_LOCALTIME
    double    months, t;
    time_t    time;
    struct tm *time_s;
    months = moth_source_rpn_stack_pop_value (stack, row, token);
    t = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(months) || isnan(t)) {
      moth_source_rpn_stack_push_value (stack, NAN, token);
    }
    else {
      time = t;
      time_s = localtime (&time);
      time_s->tm_mon += months;
      time = mktime (time_s);
      moth_source_rpn_stack_push_value (stack, time, token);
    }
#else
    warn ("<rpn> ADDMONTHS not supported because localtime() is not available");
#endif
  }
  else if (0==strcmp("ADDYEARS", token->text)) {
#ifdef HAVE_LOCALTIME
    double    years, t;
    time_t    time;
    struct tm *time_s;
    years = moth_source_rpn_stack_pop_value (stack, row, token);
    t = moth_source_rpn_stack_pop_value (stack, row, token);
    if (isnan(years) || isnan(t)) {
      moth_source_rpn_stack_push_value (stack, NAN, token);
    }
    else {
      time = t;
      time_s = localtime (&time);
      time_s->tm_mon += years;
      time = mktime (time_s);
      moth_source_rpn_stack_push_value (stack, time, token);
    }
#else
    warn ("<rpn> ADDYEARS not supported because localtime() is not available");
#endif
  }

  /* UH OH */
  else {
    die ("unknown <rpn> token \"%s\"", token->text);
  }

  if (errno)
    warn ("math error in <rpn> token %s:  %s", token->text, strerror(errno));
}



INTERNAL
void
moth_source_rpn_stack_destroy
(
  moth_source_rpn_stack stack
)
{
  moth_source_rpn_stack_entry e, doomed;
  if (!stack) return;
  e = stack->top;
  while (e) {
    doomed = e;
    e = e->next;
    moth_source_rpn_stack_entry_destroy (doomed);
  }
  free (stack);
}



/*--------------------------------------------------------*\
|                        interface                         |
\*--------------------------------------------------------*/

int
moth_source_create_rpn
(
  moth_source source,
  const char  **attributes
)
{
  source->specifics = NULL;
  return 1;
}



void
moth_source_destroy_rpn
(
  void *specifics
)
{
  if (specifics)
    die ("wrong destructor [moth_source_destroy_rpn()] called for a source");
}



void
moth_source_dump_specifics_rpn
(
  moth_source source
)
{
  /* nothing to do, since we don't have any specifics */
  return;
}



void
moth_source_process_rpn
(
  moth_source source
)
{
  moth_source_rpn_tokens  tokens;
  moth_source_rpn_token   token;
  moth_source_rpn_stack   stack;
  moth_source_rpn_stack_entry entry;
  size_t                  r;
  moth_column             column, *columns_reverse;
  int                     n_columns, c;

  tokens = moth_source_rpn_tokens_create (source->buffer);

  /* create backwards list of columns to facilitate
   * moving stack remainders into columns */
  n_columns = moth_column_list_size (source->columns);
  columns_reverse = (moth_column *) calloc (n_columns, sizeof(moth_column));
  c = 0;
  moth_column_list_reset (source->columns);
  while (( column = moth_column_list_next(source->columns) )) {
    columns_reverse[n_columns-1-c] = column;
    ++c;
  }

  /* iterate over rows, processing */
  for (r=0; r<tokens->rows; ++r) {
    int skip_row;
    stack = moth_source_rpn_stack_create ();

    /* iterate over tokens */
    for (        moth_source_rpn_tokens_reset(tokens);
          (token=moth_source_rpn_tokens_current(tokens));
                 moth_source_rpn_tokens_next(tokens) )
    {
      double value;
      switch (token->type) {
        case MOTH_RPN_CONSTANT:
          moth_source_rpn_stack_push_value (stack, token->value, token);
          break;
        case MOTH_RPN_BUILTIN:
          moth_source_rpn_stack_call_builtin (stack, tokens, r);
          break;
        case MOTH_RPN_COLUMN:
          value = moth_column_value (token->column, r);
          moth_source_rpn_stack_push_value (stack, value, token);
          break;
        default:
          die ("unknown token in <rpn>:  %s", token->text);
          break;
      }
    } /* tokens */

    /* More lazy evaluation:  check stack for SKIP values.  */
    skip_row = 0;
    entry = stack->top;
    while (entry) {
      if (0==strcmp("SKIP",entry->token->text))
        skip_row = 1;
      entry = entry->next;
    }
    if (skip_row) {
      moth_source_rpn_stack_destroy (stack);
      continue;
    }

    /* put stack remainders into destination columns */
    if (stack->size < n_columns) {
      /* This would have triggered a stack underflow anyway,
       * but we can give a more specific error message.  */
      const char *plural = "";
      if (n_columns > 1) plural = "s";
      die ("not enough values left on <rpn> stack to assign to %d column%s",
          n_columns, plural);
    }
    for (c=0; c<n_columns; ++c) {
      double value;
      value = moth_source_rpn_stack_pop_value (stack, r, NULL);
      moth_column_add (columns_reverse[c], value);
    }
    /* turning this on/off might become a flag */
    if (0 != stack->size)
      warn ("%d leftover item(s) in stack in <rpn>", stack->size);

    moth_source_rpn_stack_destroy (stack);
    stack = NULL;
  } /* rows */

  /* 
   * We can use <rpn>s to make a named column out of a value.  That is done
   * here:  if this <rpn> doesn't actually read from any existing column, then
   * we've just created a column out of constant values (and possibly
   * builtins).
   */
  if (0 == tokens->source_columns) {
    moth_column_list_reset (source->columns);
    while (( column = moth_column_list_next(source->columns) )) {
      moth_column_mark_constant (column);
    }
  }

  free (columns_reverse);
  moth_source_rpn_tokens_destroy (tokens);
}



