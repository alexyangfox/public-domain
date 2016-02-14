/*
 * $Id: columns.h,v 1.6 2003/05/01 00:01:23 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See columns.dev for implementation notes.
 *
 */

#ifndef MOTH_COLUMNS_H
#define MOTH_COLUMNS_H



#include "data.h"



/*--------------------------------------------------------*\
|                          column                          |
\*--------------------------------------------------------*/

void            moth_column_add   (moth_column column, double value);

const char *    moth_column_name  (moth_column);
/* Returns the name of the column.  */

size_t          moth_column_size  (moth_column);
/* Returns the number of values in the column.  */

double          moth_column_value (moth_column, size_t row);
/* Returns the value at the specified row of the column, or
 * NaN if there is not a value at that row.  */

double          moth_column_min   (moth_column, int withinf);
/* Finds minimum value.  Boolean second argument specifies 
 * whether negative infinity values are included.  */

double          moth_column_max   (moth_column, int withinf);
/* Finds maximum value.  Boolean second argument specifies 
 * whether positive infinity values are included.  */

int             moth_column_mark_constant (moth_column);
/* Used by the <rpn> processor to mark that this source has a single constant
 * value (first added value).  Returns success as boolean.  */



/*--------------------------------------------------------*\
|                      column list                         |
\*--------------------------------------------------------*/

moth_column_list  moth_column_list_create (void);

void          moth_column_list_add      (moth_column_list, moth_column);
/* Does not add a column if it is already in the list.
 * Does not disturb the iterator used in _reset() and _next(). */

size_t        moth_column_list_size     (moth_column_list);
/* Returns number of columns.  */

void          moth_column_list_reset    (moth_column_list);
/* Resets internal iterator to beginning.  */

moth_column   moth_column_list_next     (moth_column_list);
/* Returns current column and advances iterator.
 * Returns NULL if no more columns.  */

double        moth_column_list_min      (moth_column_list, int withinf);
/* Finds minimum value.  Boolean second argument specifies
 * whether negative infinity values are included.  */

double        moth_column_list_max      (moth_column_list, int withinf);
/* Finds minimum value.  Boolean second argument specifies
 * whether positive infinity values are included.  */

moth_axis_scaling
              moth_column_list_find_best_scaling (moth_column_list);
/* Determines which scaling provides the best presentation of
 * values in the lists.  */

void          moth_column_list_dump     (moth_column_list);
/* Prints a comma-separated list of the names of the columns.  */

void          moth_column_list_destroy  (moth_column_list);
/* Destroys the list.  Does not destroy the columns contained
 * in the list.  */



/*--------------------------------------------------------*\
|                       column store                       |
\*--------------------------------------------------------*/

moth_column_store moth_column_store_create   (void);

moth_column       moth_column_store_get      (moth_column_store, const char *);
/* Get a column by name. */

moth_column_list  moth_column_store_get_list (moth_column_store, const char *);
/* Get the list of a columns based on a comma-separated string of column
 * names.  These columns will be created, and it is an error to specify a
 * column that already exists.  */

void              moth_column_store_destroy  (moth_column_store);
/* Destroys store, as well as columns returned through _get()
 * and columns in lists returned through _get_list().  */



#endif

