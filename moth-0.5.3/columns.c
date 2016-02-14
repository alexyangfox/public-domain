/*
 * $Id: columns.c,v 1.9 2003/06/12 17:44:23 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See columns.dev for implementation notes.
 *
 */



#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "data.h"
#include "columns.h"



/*--------------------------------------------------------*\
|                    constants / macros                    |
\*--------------------------------------------------------*/

/* Funky number is intended to create 1k chunks for most platforms:
 *     8bytes/double  * 127 doubles 
 *   + 4bytes/pointer * 1 pointer
 *   + 4bytes/size_t  * 1 size_t
 *   = 1024bytes = 1kilobyte
 */
#define MOTH_COLUMN_CHUNKSIZE 127



/*--------------------------------------------------------*\
|                      data structures                     |
\*--------------------------------------------------------*/

typedef struct moth_column_chunk_s  *moth_column_chunk;
struct moth_column_chunk_s {
  moth_column_chunk     next;
  size_t                size;   /* number of values stored */
  double                data[MOTH_COLUMN_CHUNKSIZE];
};
struct moth_column_s {
  char                  *name;  /* we manage this memory */
  moth_column_chunk     head;
  moth_column_chunk     tail;
  size_t                size;   /* number of values stored */

  /* A "constant" column is a dynamically sized columns that always returns
   * the number.  */
  int                   is_constant;
  double                value;
};



/* linked list */
typedef struct moth_column_list_entry_s *moth_column_list_entry;
struct moth_column_list_entry_s {
  moth_column_list_entry  next;
  moth_column             column;
};
struct moth_column_list_s {
  moth_column_list_entry  head;
  moth_column_list_entry  iter;
  size_t                  size;   /* number of columns in the list */
};



typedef struct moth_column_store_entry_s *moth_column_store_entry;
struct moth_column_store_entry_s {
  /* structure */
  moth_column_store_entry before;
  moth_column_store_entry after;
  /* datum */
  moth_column     column;     /* we manage this memory */
};



struct moth_column_store_s {
  moth_column_store_entry root;
};



/*--------------------------------------------------------*\
|                         column                           |
\*--------------------------------------------------------*/

INTERNAL
moth_column
moth_column_create
(
  const char  *name,
  double      value   /* pass NAN for a named column */
)
{
  moth_column column;
  column = (moth_column) malloc (sizeof(struct moth_column_s));
  if (!column) {
    warn ("couldn't create a new column object");
    return NULL;
  }
  column->name = NULL;
  column->head = NULL;
  column->tail = NULL;
  column->size = 0;
  column->is_constant = isnan(value) ? 0 : 1;
  column->value = value;

  column->name = strdup (name);
  if (!column->name) {
    warn ("couldn't create a space for column name");
    free (column);
    return NULL;
  }

  return column;
}



void
moth_column_add
(
  moth_column column,
  double      new
)
{
  moth_column_chunk new_chunk;

  /* first item added */
  if (column->head == NULL) {
    new_chunk = (moth_column_chunk)
      malloc (sizeof(struct moth_column_chunk_s));
    new_chunk->next = NULL;
    new_chunk->size = 0;
    column->head = new_chunk;
    column->tail = new_chunk;
  }

  /* chunk has space */
  if (column->tail->size < MOTH_COLUMN_CHUNKSIZE) {
    column->tail->data[column->tail->size] = new;
    column->tail->size += 1;
    column->size += 1;
  }
  /* add a new chunk */
  else {
    new_chunk = (moth_column_chunk)
      malloc (sizeof(struct moth_column_chunk_s));
    new_chunk->next = NULL;
    new_chunk->data[0] = new;
    new_chunk->size = 1;
    column->size += 1;
    column->tail->next = new_chunk;
    column->tail = new_chunk;
  }
}



const char *
moth_column_name
(
  moth_column column
)
{
  if (!column) return NULL;
  return column->name;
}



size_t
moth_column_size
(
  moth_column column
)
{
  if (!column) return 0;
  if (column->is_constant) return 1;
  return column->size;
}



double
moth_column_value
(
  moth_column column,
  size_t      row
)
{
  size_t            chunk;    /* which chunk */
  size_t            offset;   /* offset within chunk */
  moth_column_chunk c;

  if (!column)
    return NAN;
  if (column->is_constant)
    return column->value;
  if (row >= column->size)
    return NAN;

  chunk = row / MOTH_COLUMN_CHUNKSIZE;
  offset = row % MOTH_COLUMN_CHUNKSIZE;
  c = column->head;
  while (chunk > 0) {
    c = c->next;
    --chunk;
  }
  if (!c)
    return NAN;
  if (offset >= c->size)
    return NAN;

  return c->data[offset];
}



double
moth_column_min
(
  moth_column column,
  int         withinf
)
{
  double            min, val;
  moth_column_chunk c;
  size_t            d;

  if (column->is_constant)
    return column->value;

  min = NAN;
  c = column->head;
  while (c) {
    for (d=0; d<c->size; d++) {
      val = c->data[d];
      if (!withinf && isinf(val)) continue;
      if (isnan(min))
        min = val;
      else
        min = (val<min) ? val : min;
    }
    c = c->next;
  }

  return min;
}



double
moth_column_max
(
  moth_column column,
  int         withinf
)
{
  double            max, val;
  moth_column_chunk c;
  size_t            d;

  if (column->is_constant)
    return column->value;

  max = NAN;
  c = column->head;
  while (c) {
    for (d=0; d<c->size; d++) {
      val = c->data[d];
      if (!withinf && isinf(val)) continue;
      if (isnan(max))
        max = val;
      else
        max = (val>max) ? val : max;
    }
    c = c->next;
  }

  return max;
}



int
moth_column_mark_constant
(
  moth_column column
)
{
  if (! column->head)
    return 0;
  column->value = column->head->data[0];

  column->is_constant = 1;
  return 1;
}



INTERNAL
void
moth_column_destroy
(
  moth_column column
)
{
  moth_column_chunk c, doomed;
  c = column->head;
  if (column->name)
    free (column->name);
  while (c) {
    doomed = c;
    c = c->next;
    free (doomed);
  }
  free (column);
}



/*--------------------------------------------------------*\
|                      column list                         |
\*--------------------------------------------------------*/

moth_column_list
moth_column_list_create (void)
{
  moth_column_list list;
  list = (moth_column_list) malloc (sizeof(struct moth_column_list_s));
  if (!list) {
    warn ("couldn't create a new column list object");
    return NULL;
  }
  list->head = NULL;
  list->iter = NULL;
  list->size = 0;
  return list;
}



void
moth_column_list_add
(
  moth_column_list  list,
  moth_column       column
)
{
  moth_column_list_entry new, last, end;
  new = (moth_column_list_entry)
    malloc (sizeof(struct moth_column_list_entry_s));
  new->next = NULL;
  new->column = column;

  last = end = list->head;
  while (end) {
    if (end->column == column) {
      free (new);
      return;
    }
    last = end;
    end = end->next;
  }
  if (last)
    last->next = new;
  else
    list->head = new;
  list->size += 1;
}



size_t
moth_column_list_size
(
  moth_column_list  list
)
{
  return list->size;
}



void
moth_column_list_reset
(
  moth_column_list list
)
{
  list->iter = list->head;
}



moth_column
moth_column_list_next
(
  moth_column_list list
)
{
  moth_column_list_entry e;
  e = list->iter;
  if (!e)
    return NULL;
  list->iter = list->iter->next;
  return e->column;
}



double
moth_column_list_min
(
  moth_column_list  list,
  int               withinf
)
{
  double                  min, val;
  moth_column_list_entry  e;
  min = NAN;

  e = list->head;
  while (e) {
    val = moth_column_min (e->column, withinf);
    if (isnan(min))
      min = val;
    else
      min = (val<min) ? val : min;
    e = e->next;
  }
  return min;
}



double
moth_column_list_max
(
  moth_column_list  list,
  int               withinf
)
{
  double                  max, val;
  moth_column_list_entry  e;
  max = NAN;

  e = list->head;
  while (e) {
    val = moth_column_max (e->column, withinf);
    if (isnan(max))
      max = val;
    else
      max = (val>max) ? val : max;
    e = e->next;
  }
  return max;
}



moth_axis_scaling
moth_column_list_find_best_scaling
(
  moth_column_list columns
)
{
  moth_axis_scaling s, best_s;
  moth_column       c;
  size_t            count, n_sorted, i;
  double            min, max;
  double            *sorted, *variances, best;

  count = 0;
  moth_column_list_reset (columns);
  while (( c = moth_column_list_next(columns) )) {
    count += moth_column_size (c);
  }
  if (count <= 1) return MOTH_LINEAR;
  n_sorted = count;
  count -= 1;
  
  min = moth_column_list_min (columns, 0);
  max = moth_column_list_max (columns, 0);

  variances = (double *)
    calloc (MOTH_AUTO-MOTH_LINEAR, sizeof(double));

  /* make a sorted list of all values (skipping NaN and INF values) */
  sorted = (double *) calloc (n_sorted, sizeof(double));
  n_sorted = 0;
  moth_column_list_reset (columns);
  while (( c = moth_column_list_next(columns) )) {
    size_t n_column = moth_column_size (c);
    for (i=0; i<n_column; ++i) {
      double value = moth_column_value (c, i);
      if (isnan(value) || isinf(value))
        continue;
      sorted[n_sorted] = value;
      ++n_sorted;
    }
  }

  /* guard against case all NaN/inf values */
  if (0 == n_sorted) {
    free (variances);
    free (sorted);
    return MOTH_LINEAR;
  }

  qsort (sorted, n_sorted, sizeof(double), double_compare);

  for (s=MOTH_LINEAR; s<MOTH_AUTO; ++s) {
    double base_min, base_max, mean, sum;
    base_min = scale_value (s, min);
    base_max = scale_value (s, max);
    mean = (base_max - base_min) / count;
    sum = 0.0;
    for (i=0; i<(n_sorted-1); ++i) {
      double part = mean - 
        (scale_value(s,sorted[i+1]) - scale_value(s,sorted[i]));
      /* normalize to unit range */
      part = (part-min) / (max-min);
      sum += part * part;
    }
    variances[s-MOTH_LINEAR] = sum / count;
  }

  best = HUGE_VAL;
  best_s = MOTH_AUTO;
  for (s=MOTH_LINEAR; s<MOTH_AUTO; ++s) {
    if (variances[s-MOTH_LINEAR] <= best) {
      best = variances[s-MOTH_LINEAR];
      best_s = s;
    }
  }
  if (MOTH_AUTO == best_s)
    best_s = MOTH_LINEAR;

  /* 
   * If the chosen scaling is not much better than linear, use linear since
   * it's easier for the user to understand.
   */
  if (MOTH_LINEAR != best_s) {
    double diff = fabs (variances[MOTH_LINEAR-MOTH_LINEAR] - best);
    if ( (diff/best) < MOTH_COLUMN_LIST_LINEAR_PREFERENCE )
      best_s = MOTH_LINEAR;
  }

  free (variances);
  free (sorted);
  return best_s;
}



void
moth_column_list_dump
(
  moth_column_list list
)
{
  int         first;
  moth_column column;

  moth_column_list_reset (list);
  first = 1;
  while (( column = moth_column_list_next(list) )) {
    if (!first)
      fprintf (stdout, "%c", MOTH_LIST_SEPARATOR);
    fprintf (stdout, "%s", moth_column_name(column));
    first = 0;
  }
}



void
moth_column_list_destroy
(
  moth_column_list list
)
{
  moth_column_list_entry e, doomed;
  e = list->head;
  while (e) {
    doomed = e;
    e = e->next;
    free (doomed);
  }
  free (list);
}



/*--------------------------------------------------------*\
|                       column store                       |
\*--------------------------------------------------------*/

INTERNAL
moth_column_store_entry
moth_column_store_entry_create
(
  moth_column   column
)
{
  moth_column_store_entry entry;
  entry = (moth_column_store_entry)
    malloc (sizeof(struct moth_column_store_entry_s));

  entry->before = NULL;
  entry->after = NULL;
  entry->column = column;

  return entry;
}



/* destroys entry, and all child entries */
INTERNAL
void
moth_column_store_entry_destroy
(
  moth_column_store_entry entry
)
{
  if (!entry) return;

  if (entry->before)
    moth_column_store_entry_destroy (entry->before);
  if (entry->after)
    moth_column_store_entry_destroy (entry->after);
  if (entry->column)
    moth_column_destroy (entry->column);
  free (entry);
}



INTERNAL
moth_column_store_entry
moth_column_store_entry_find_name
(
  moth_column_store_entry  start,
  const char              *name
)
{
  int compare;
  if (!start)
    return NULL;

  compare = strcmp (name, start->column->name);
  if (STRCMP_MATCH(compare)) {
    return start;
  }
  else if (STRCMP_BEFORE(compare)) {
    return moth_column_store_entry_find_name (start->before, name);
  }
  else {
    return moth_column_store_entry_find_name (start->after, name);
  }
}



INTERNAL
void
moth_column_store_entry_insert
(
  moth_column_store_entry start,
  moth_column_store_entry new   /* mustn't already belong */
)
{
  int thisCompare;
  if (!start || !new)
    return;

  thisCompare = strcmp (new->column->name, start->column->name);
  if (STRCMP_BEFORE(thisCompare)) {
    if (start->before)
      moth_column_store_entry_insert (start->before, new);
    else
      start->before = new;
  }

  else if (STRCMP_AFTER(thisCompare)) {
    if (start->after)
      moth_column_store_entry_insert (start->after, new);
    else
      start->after = new;
  }

  else {
    /* OK, this should really be an exception */
  }
}



INTERNAL
moth_column
moth_column_store_entry_remove
(
  moth_column_store_entry *start,
  const char              *name
)
{
  moth_column_store_entry doomed;
  moth_column             column;
  int                     compare;

  if (!start || !(*start))
    return NULL;

  compare = strcmp (name, (*start)->column->name);
  if (STRCMP_BEFORE(compare)) {
    return moth_column_store_entry_remove (&((*start)->before), name);
  }
  if (STRCMP_AFTER(compare)) {
    return moth_column_store_entry_remove (&((*start)->after), name);
  }

  doomed = *start;
  if (doomed->before && doomed->after) {
    /* OK, this is the tricky case, when the doomed
     * entry has two children.  We need to find a home
     * for both of them.
     */
    moth_column_store_entry newStart;
    newStart = NULL;
    if (doomed->after->before) {
      /* smallest entry greater than doomed, and less than
       * doomed->after, becomes the new root of the tree
       */
      moth_column_store_entry newStart_parent;
      newStart_parent = doomed->after;
      newStart = doomed->after->before;
      while (newStart->before) {
        newStart_parent = newStart;
        newStart = newStart->before;
      }
      newStart_parent->before = newStart->after;
      newStart->before = doomed->before;
      newStart->after = doomed->after;
      (*start) = newStart;
    }
    else if (doomed->before->after) {
      /* greatest entry less than doomed, and greater than
       * doomed->before, becomes the new root of the tree
       */
      moth_column_store_entry newStart_parent;
      newStart_parent = doomed->before;
      newStart = doomed->before->after;
      while (newStart->after) {
        newStart_parent = newStart;
        newStart = newStart->after;
      }
      newStart_parent->after = newStart->before;
      newStart->before = doomed->before;
      newStart->after = doomed->after;
      (*start) = newStart;
    }
    else {
      newStart = doomed->after;
      newStart->before = doomed->before;
      (*start) = newStart;
    }
  }

  else if (doomed->before) {
    (*start) = doomed->before;
  }

  else if (doomed->after) {
    (*start) = doomed->after;
  }

  else {
    (*start) = NULL;
  }

  column = doomed->column;
  doomed->before = NULL;
  doomed->after = NULL;
  moth_column_store_entry_destroy (doomed);
  return column;
}



moth_column_store
moth_column_store_create
(
  void
)
{
  moth_column_store hash;
  hash = (moth_column_store) malloc (sizeof(struct moth_column_store_s));
  hash->root = NULL;
  return hash;
}



void
moth_column_store_destroy
(
  moth_column_store hash
)
{
  if (hash->root)
    moth_column_store_entry_destroy (hash->root);
  free (hash);
}



INTERNAL
moth_column
moth_column_store_create_column
(
  moth_column_store hash,
  const char        *name
)
{
  moth_column_store_entry entry;
  moth_column             column;

  entry = moth_column_store_entry_find_name (hash->root, name);
  if (entry) {
    warn ("column \"%s\" already exists", name);
    return NULL;
  }

  if (!isnan(read_value(name))) {
    warn ("can't redefine numbers");
    return NULL;
  }

  column = moth_column_create (name, NAN);
  entry = moth_column_store_entry_create (column);
  if (hash->root)
    moth_column_store_entry_insert (hash->root, entry);
  else
    hash->root = entry;
  return column;
}



moth_column
moth_column_store_get
(
  moth_column_store hash,
  const char        *name
)
{
  moth_column_store_entry entry;
  moth_column             column;
  double                  value;

  entry = moth_column_store_entry_find_name (hash->root, name);
  if (entry)
    return entry->column;

  value = read_value (name);
  if (isnan(value))
    return NULL;

  /* always create a constant column */
  column = moth_column_create (name, value);
  entry = moth_column_store_entry_create (column);
  if (hash->root)
    moth_column_store_entry_insert (hash->root, entry);
  else
    hash->root = entry;
  return column;
}



moth_column_list
moth_column_store_get_list
(
  moth_column_store hash,
  const char        *string
)
{
  moth_column_list  list;
  moth_column       new;
  const char        *start, *comma;
  list = moth_column_list_create ();
  if (!list)
    return NULL;

  start = string;
  while (( comma = strchr(start,MOTH_LIST_SEPARATOR) )) {
    char    *source;
    size_t  size;
    size = comma - start;
    source = (char *) calloc (size+1, sizeof(char));
    strncpy (source, start, size);
    source[size] = '\0';

    new = moth_column_store_create_column (hash, source);
    if (!new) {
      free (source);
      moth_column_list_destroy (list);
      return NULL;
    }
    moth_column_list_add (list, new);
    free (source);
    start = comma + 1;
  }
  /* add column that is after last delimiter */
  new = moth_column_store_create_column (hash, start);
  if (!new) {
    moth_column_list_destroy (list);
    return NULL;
  }
  moth_column_list_add (list, new);

  return list;
}



