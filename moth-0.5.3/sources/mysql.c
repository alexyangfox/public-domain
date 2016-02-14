/*
 * $Id: mysql.c,v 1.8 2003/06/06 16:51:46 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 */



#include <stdlib.h>
#include <math.h>
#include <mysql/mysql.h>

#include "../sources.h"
#include "../data.h"



#define MOTH_SOURCE_MYSQL_HOST_DEFAULT "localhost"


typedef struct moth_source_mysql_specifics_s *moth_source_mysql_specifics;
struct moth_source_mysql_specifics_s {
  char  *host;
  char  *database;
  char  *username;
  char  *password;
};



int
moth_source_create_mysql
(
  moth_source source,
  const char  **attributes
)
{
  moth_source_mysql_specifics specs;
  int a;

  specs = (moth_source_mysql_specifics)
    malloc (sizeof(struct moth_source_mysql_specifics_s));
  if (!specs)
    return 0;
  specs->host = NULL;
  specs->database = NULL;
  specs->username = NULL;
  specs->password = NULL;

  for (a=0; attributes[a]; a+=2) {
    const char  *attname = attributes[a];
    const char  *attvalue = attributes[a+1];
    if (0==strcasecmp("columns",attname)) {
      /* ignore.  already been taken care of.  */
    }
    else if (0==strcasecmp("host",attname)) {
      specs->host = strdup (attvalue);
    }
    else if (0==strcasecmp("database",attname)) {
      specs->database = strdup (attvalue);
    }
    else if (0==strcasecmp("username",attname)) {
      specs->username = strdup (attvalue);
    }
    else if (0==strcasecmp("password",attname)) {
      specs->password = strdup (attvalue);
    }
    else {
      warn ("unknown <mysql> attribute %s=\"%s\"", attname, attvalue);
    }
  }

  if (!specs->host)
    specs->host = strdup (MOTH_SOURCE_MYSQL_HOST_DEFAULT);
  if (!specs->database)
    die ("missing <mysql> attribute \"database\"");

  source->specifics = specs;
  return 1;
}



void
moth_source_dump_specifics_mysql
(
  moth_source source
)
{
  moth_source_mysql_specifics specs;
  specs = (moth_source_mysql_specifics) source->specifics;
  if (specs->host)
    fprintf (stdout, " host=\"%s\"", specs->host);
  if (specs->database)
    fprintf (stdout, " database=\"%s\"", specs->database);
  if (specs->username)
    fprintf (stdout, " username=\"%s\"", specs->username);
  if (specs->password)
    fprintf (stdout, " password=\"%s\"", specs->password);

}



void
moth_source_process_mysql
(
  moth_source source
)
{
  moth_source_mysql_specifics specs;
  char          *query;
  MYSQL         *mysql;
  MYSQL_RES     *result;
  MYSQL_ROW     row;
  unsigned int  num_fields, num_columns;
  unsigned int  i;
  moth_column   *columns;

  specs = (moth_source_mysql_specifics) source->specifics;

  mysql = mysql_init (NULL);
  if (!mysql)
    die ("can't create MYSQL object");

  if (! mysql_real_connect(mysql,specs->host,specs->username,
          specs->password,specs->database,0,NULL,0))
  {
    /*
     * The mysql documentation doesn't specify whether mysql_close() frees the
     * memory pointed to by mysql_error().  We'll be paranoid and assume that
     * it does.  (This leaks memory, but that's OK since we're going to die
     * right away.)
     */
    char *error;
    error = strdup (mysql_error(mysql));
    mysql_close (mysql);
    die ("MYSQL:  %s", error);
  }

  query = moth_buffer_strdup (source->buffer);
  if (!query)
    die ("empty query in <mysql>");
  if (mysql_query(mysql,query)) {
    /*
     * The mysql documentation doesn't specify whether mysql_close() frees the
     * memory pointed to by mysql_error().  We'll be paranoid and assume that
     * it does.  (This leaks memory, but that's OK since we're going to die
     * right away.)
     */
    char *error;
    free (query);
    error = strdup (mysql_error(mysql));
    mysql_close (mysql);
    die ("MYSQL:  %s", error);
  }
  result = mysql_store_result (mysql);
  if (!result) {
    free (query);
    if (mysql_errno(mysql)) {
      /*
       * The mysql documentation doesn't specify whether mysql_close() frees
       * the memory pointed to by mysql_error().  We'll be paranoid and assume
       * that it does.  (This leaks memory, but that's OK since we're going to
       * die right away.)
       */
      char *error;
      error = strdup (mysql_error(mysql));
      mysql_close (mysql);
      die ("MYSQL:  %s", error);
    }
    die ("MYSQL:  query returned no data");
  }

  num_fields = mysql_num_fields (result);
  num_columns = moth_column_list_size (source->columns);
  if (num_fields != num_columns) {
    free (query);
    die ("MYSQL:  number of query fields (%d) doens't "
        "equal number of columns (%d)",
        num_fields, num_columns);
  }

  /* make integer-indexed list of columns, for easier reference */
  columns = (moth_column *) calloc (num_columns, sizeof(moth_column));
  i = 0;
  moth_column_list_reset (source->columns);
  while (i<num_columns) {
    columns[i] = moth_column_list_next (source->columns);
    ++i;
  }

  while (( row = mysql_fetch_row(result) )) {
    for (i = 0; i < num_fields; i++) {
      double val = NAN;
      if (row[i])
        val = read_value (row[i]);
      moth_column_add (columns[i], val);
    }
  }

  free (columns);
  mysql_free_result (result);

  if (mysql_errno(mysql)) {
    /*
     * The mysql documentation doesn't specify whether mysql_close() frees the
     * memory pointed to by mysql_error().  We'll be paranoid and assume that
     * it does.  (This leaks memory, but that's OK since we're going to die
     * right away.)
     */
    char *error;
    free (query);
    error = strdup (mysql_error(mysql));
    mysql_close (mysql);
    die ("MYSQL:  %s", error);
  }

  mysql_close (mysql);
  free (query);
}



void
moth_source_destroy_mysql
(
  void *specifics
)
{
  moth_source_mysql_specifics specs;
  specs = (moth_source_mysql_specifics) specifics;
  if (!specs)
    return;
  if (specs->host)
    free (specs->host);
  if (specs->database)
    free (specs->database);
  if (specs->username)
    free (specs->username);
  if (specs->password)
    free (specs->password);
  free (specs);
}



