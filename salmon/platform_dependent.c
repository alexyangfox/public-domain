/* file "platform_dependent.c" */

/*
 *  This file contains the implementation of the platform_dependent module.
 *  This module contains code that is compiled differently for different
 *  systems because it needs functionality beyond the ANSI C 1990 standard.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifdef linux

/* On Linux, _LARGEFILE_SOURCE must be defined before including <stdio.h> to
 * get fseeko() and ftello(). */
#define _LARGEFILE_SOURCE 1

#endif /* linux */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include "c_foundations/basic.h"
#include "c_foundations/buffer_print.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/auto_array.h"
#include "platform_dependent.h"
#include "o_integer.h"


#ifndef REDIRECT_SYSTEM_STANDARD
#ifndef REDIRECT_SYSTEM_POSIX
#if (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__))
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define REDIRECT_SYSTEM_POSIX
#endif /* (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__)) */
#endif /* REDIRECT_SYSTEM_POSIX */
#endif /* REDIRECT_SYSTEM_STANDARD */


#ifndef FTELL_STANDARD
#ifndef FTELL_FTELLO
#if (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__))
#define FTELL_FTELLO
#endif /* (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__)) */
#endif /* FTELL_FTELLO */
#endif /* FTELL_STANDARD */


#ifndef FSEEK_STANDARD
#ifndef FSEEK_FSEEKO
#if (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__))
#define FSEEK_FSEEKO
#endif /* (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__)) */
#endif /* FSEEK_FSEEKO */
#endif /* FSEEK_STANDARD */

#ifndef TIME_STANDARD
#ifndef TIME_GETTIMEOFDAY
#if (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__))
#include <sys/time.h>
#define TIME_GETTIMEOFDAY
#endif /* (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__)) */
#endif /* TIME_GETTIMEOFDAY */
#endif /* TIME_STANDARD */

#ifndef TIME_ZONE_STANDARD
#if (!defined(TIME_ZONE_POSIX)) && (!defined(TIME_ZONE_CYGWIN))
#ifdef _POSIX_C_SOURCE
#define TIME_ZONE_POSIX
#elif defined(__CYGWIN__)
#define TIME_ZONE_CYGWIN
#endif /* (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__)) */
#endif /* (!defined(TIME_ZONE_POSIX)) && (!defined(TIME_ZONE_CYGWIN)) */
#endif /* TIME_ZONE_STANDARD */


#ifndef DIRECTORY_CONTENTS_STANDARD
#ifndef DIRECTORY_CONTENTS_POSIX
#if (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__))
#include <sys/types.h>
#include <dirent.h>
#define DIRECTORY_CONTENTS_POSIX
#endif /* (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__)) */
#endif /* DIRECTORY_CONTENTS_POSIX */
#endif /* DIRECTORY_CONTENTS_STANDARD */


#ifndef FILE_EXISTS_STANDARD
#ifndef FILE_EXISTS_POSIX
#if (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__))
#include <sys/stat.h>
#define FILE_EXISTS_POSIX
#endif /* (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__)) */
#endif /* FILE_EXISTS_POSIX */
#endif /* FILE_EXISTS_STANDARD */


#ifndef DIRECTORY_EXISTS_STANDARD
#ifndef DIRECTORY_EXISTS_POSIX
#if (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__))
#include <sys/stat.h>
#define DIRECTORY_EXISTS_POSIX
#endif /* (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__)) */
#endif /* DIRECTORY_EXISTS_POSIX */
#endif /* DIRECTORY_EXISTS_STANDARD */


#ifndef DYNAMIC_LIBRARY_STANDARD
#ifndef DYNAMIC_LIBRARY_DLOPEN
#if (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__))
#include <dlfcn.h>
#define DYNAMIC_LIBRARY_DLOPEN
#endif /* (defined(_POSIX_C_SOURCE) || defined(__CYGWIN__)) */
#endif /* DYNAMIC_LIBRARY_DLOPEN */
#endif /* DYNAMIC_LIBRARY_STANDARD */


#ifndef DYNAMIC_LIBRARY_ERROR_MESSAGE_NO_LINUX
#ifndef DYNAMIC_LIBRARY_ERROR_MESSAGE_LINUX
#ifdef linux
#define DYNAMIC_LIBRARY_ERROR_MESSAGE_LINUX
#endif /* linux */
#endif /* DYNAMIC_LIBRARY_ERROR_MESSAGE_LINUX */
#endif /* DYNAMIC_LIBRARY_ERROR_MESSAGE_NO_LINUX */


#ifndef STACK_LIMIT_STANDARD
#ifndef STACK_LIMIT_GETRLIMIT
#if (defined(__linux__))
#include <sys/resource.h>
#define STACK_LIMIT_GETRLIMIT
#endif /* (defined(__linux__)) */
#endif /* STACK_LIMIT_GETRLIMIT */
#endif /* STACK_LIMIT_STANDARD */


#ifndef DOS_PATHS_STANDARD
#ifndef DOS_PATHS
#ifdef __CYGWIN__
#define DOS_PATHS
#endif
#endif /* DOS_PATHS */
#endif /* DOS_PATHS_STANDARD */


#ifndef NO_CYGWIN_EXIT_BRAIN_DAMAGE
#ifndef CYGWIN_EXIT_BRAIN_DAMAGE
#ifdef __CYGWIN__
#define CYGWIN_EXIT_BRAIN_DAMAGE
#endif
#endif /* CYGWIN_EXIT_BRAIN_DAMAGE */
#endif /* NO_CYGWIN_EXIT_BRAIN_DAMAGE */


#ifdef REDIRECT_SYSTEM_POSIX

static char *temporary_file_name(void);
static char *read_whole_text_file(const char *file_name);

#endif /* REDIRECT_SYSTEM_POSIX */

#ifdef FTELL_FTELLO

static o_integer oi_create_from_off_t(off_t the_off_t);

#endif /* FTELL_FTELLO */

#ifdef FSEEK_FSEEKO

static verdict oi_to_off_t(o_integer oi, off_t *result);

#endif /* FSEEK_FSEEKO */

#ifdef MULTI_THREADED
#ifndef NDEBUG

static pthread_mutex_t local_lock = PTHREAD_MUTEX_INITIALIZER;
static thread_lock_info *all_thread_info;

#endif /* !NDEBUG */
#endif /* MULTI_THREADED */


static char *dynamic_library_suffixes[] = { ".so", ".dll", NULL };


AUTO_ARRAY(string_aa, char *);


extern int redirected_system(const char *command,
        char **result_standard_output, char **result_standard_error,
        boolean *error)
  {
    assert(command != NULL);
    assert(error != NULL);

    *error = FALSE;

#ifdef REDIRECT_SYSTEM_POSIX

    pid_t child_pid;
    char *standard_output_temp_file_name;
    char *standard_error_temp_file_name;
    int result_status;
    pid_t wait_result;

    if (result_standard_error == NULL)
      {
        FILE *fp;
        string_buffer output_buffer;
        verdict the_verdict;

        assert(result_standard_output != NULL);

        fp = popen(command, "r");
        if (fp == NULL)
          {
            fprintf(stderr, "ERROR: popen(\"%s\") failed: %s.", command,
                    strerror(errno));
            *error = TRUE;
            return 0;
          }

        the_verdict = string_buffer_init(&output_buffer, 10);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            *error = TRUE;
            pclose(fp);
            return 0;
          }

        while (!feof(fp))
          {
            int new_char;

            new_char = fgetc(fp);
            if (new_char == EOF)
                break;

            the_verdict = string_buffer_append(&output_buffer, (char)new_char);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                *error = TRUE;
                free(output_buffer.array);
                pclose(fp);
                return 0;
              }
          }

        the_verdict = string_buffer_append(&output_buffer, 0);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            *error = TRUE;
            free(output_buffer.array);
            pclose(fp);
            return 0;
          }

        *result_standard_output = output_buffer.array;
        return pclose(fp);
      }

    if (result_standard_output != NULL)
      {
        standard_output_temp_file_name = temporary_file_name();
        if (standard_output_temp_file_name == NULL)
          {
            *error = TRUE;
            return 0;
          }
      }
    if (result_standard_error != NULL)
      {
        standard_error_temp_file_name = temporary_file_name();
        if (standard_error_temp_file_name == NULL)
          {
            if (result_standard_output != NULL)
              {
                unlink(standard_output_temp_file_name);
                free(standard_output_temp_file_name);
              }
            *error = TRUE;
            return 0;
          }
      }

    child_pid = fork();

    if (child_pid == -1)
      {
        fprintf(stderr, "ERROR: fork() failed: %s.\n", strerror(errno));
        if (result_standard_output != NULL)
          {
            unlink(standard_output_temp_file_name);
            free(standard_output_temp_file_name);
          }
        if (result_standard_error != NULL)
          {
            unlink(standard_error_temp_file_name);
            free(standard_error_temp_file_name);
          }
        *error = TRUE;
        return 0;
      }

    if (child_pid == 0)
      {
        int return_value;

        if (result_standard_output != NULL)
          {
            unlink(standard_output_temp_file_name);
            freopen(standard_output_temp_file_name, "w", stdout);
          }
        if (result_standard_error != NULL)
          {
            unlink(standard_error_temp_file_name);
            freopen(standard_error_temp_file_name, "w", stderr);
          }

        return_value = system(command);

        if (result_standard_output != NULL)
            fclose(stdout);
        if (result_standard_error != NULL)
            fclose(stderr);

#ifndef CYGWIN_EXIT_BRAIN_DAMAGE

        exit(WEXITSTATUS(return_value));

#else /* CYGWIN_EXIT_BRAIN_DAMAGE */

        /* For some reason, when we call exit() at this point on some versions
         * of Cygwin, the process gets an abort signal, so the return code is
         * wrong.  It only seems to happen when exit() is called from within
         * code explicitly loaded from a dll.  I tried long-jumping out of the
         * dll code and closing the dll before exiting, and that works if we're
         * not forked, but in this case we're in a process forked within the
         * dll, and in that case the abort still happens.  It seems like it may
         * be a Cygwin bug, but it's possible it's a bug in SalmonEye that
         * doesn't have any other effects I've noticed other than in this case.
         * Either way, it works fine on Linux, and since the abort signal only
         * happens after the call to exit(), there seems to be no real harm
         * done by calling _exit() instead.  The _exit() procedure on Cygwin
         * seems to do an exit while skipping whatever cleanup causes exit() to
         * crash in this case. */

        _exit(WEXITSTATUS(return_value));

#endif /* CYGWIN_EXIT_BRAIN_DAMAGE */
      }

    wait_result = waitpid(child_pid, &result_status, 0);
    if (wait_result != child_pid)
      {
        fprintf(stderr,
                "ERROR: waitpid() for a system() call re-directing standard "
                "error failed: %s.\n", strerror(errno));
        if (result_standard_output != NULL)
          {
            unlink(standard_output_temp_file_name);
            free(standard_output_temp_file_name);
          }
        if (result_standard_error != NULL)
          {
            unlink(standard_error_temp_file_name);
            free(standard_error_temp_file_name);
          }
        *error = TRUE;
        return 0;
      }

    if (result_standard_output != NULL)
      {
        *result_standard_output =
                read_whole_text_file(standard_output_temp_file_name);
        unlink(standard_output_temp_file_name);
        free(standard_output_temp_file_name);

        if (*result_standard_output == NULL)
          {
            if (result_standard_error != NULL)
              {
                unlink(standard_error_temp_file_name);
                free(standard_error_temp_file_name);
              }
            *error = TRUE;
            return 0;
          }
      }

    if (result_standard_error != NULL)
      {
        *result_standard_error =
                read_whole_text_file(standard_error_temp_file_name);
        unlink(standard_error_temp_file_name);
        free(standard_error_temp_file_name);

        if (*result_standard_error == NULL)
          {
            if (result_standard_output != NULL)
                free(*result_standard_output);
            *error = TRUE;
            return 0;
          }
      }

    return result_status;

#else /* !REDIRECT_SYSTEM_POSIX */

    fprintf(stderr,
            "ERROR: Redirection of output from the system() call is not "
            "supported on this platform.\n");
    *error = TRUE;
    return 0;

#endif /* !REDIRECT_SYSTEM_POSIX */
  }

extern o_integer oi_tell(FILE *fp,
        void (*error_handler)(void *data, const char *format, ...), void *data)
  {
#ifdef FTELL_FTELLO

    off_t off_t_result;

    assert(fp != NULL);
    assert(error_handler != NULL);

    off_t_result = ftello(fp);

    if (off_t_result == (off_t)-1)
      {
        (*error_handler)(data, "tell() on a file stream failed: %s.",
                         strerror(errno));
        return oi_null;
      }

    return oi_create_from_off_t(off_t_result);

#else /* !FTELL_FTELLO */

    long int long_int_result;
    o_integer oi_result;

    assert(fp != NULL);
    assert(error_handler != NULL);

    long_int_result = ftell(fp);

    if (long_int_result == (long int)-1)
      {
        (*error_handler)(data, "tell() on a file stream failed: %s.",
                         strerror(errno));
        return oi_null;
      }

    oi_create_from_long_int(oi_result, long_int_result);
    return oi_result;

#endif /* !FTELL_FTELLO */
  }

extern void oi_seek(FILE *fp, o_integer position,
        void (*error_handler)(void *data, const char *format, ...), void *data)
  {
#ifdef FSEEK_FSEEKO

    off_t off_t_position;
    verdict the_verdict;
    int return_value;

    assert(fp != NULL);
    assert(!(oi_out_of_memory(position)));
    assert(error_handler != NULL);

    the_verdict = oi_to_off_t(position, &off_t_position);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        (*error_handler)(data,
                "In a seek() call, the position %I was used, which is outside "
                "the range of legal positions.", position);
        return;
      }

    return_value = fseeko(fp, off_t_position, SEEK_SET);
    if (return_value != 0)
      {
        (*error_handler)(data, "seek() on a file stream failed: %s.",
                         strerror(errno));
      }

#else /* !FSEEK_FSEEKO */

    long int long_int_position;
    verdict the_verdict;
    int return_value;

    assert(fp != NULL);
    assert(!(oi_out_of_memory(position)));
    assert(error_handler != NULL);

    the_verdict = oi_to_long_int(position, &long_int_position);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        (*error_handler)(data,
                "In a seek() call, the position %I was used, which is outside "
                "the range of legal positions.", position);
        return;
      }

    return_value = fseek(fp, long_int_position, SEEK_SET);
    if (return_value != 0)
      {
        (*error_handler)(data, "seek() on a file stream failed: %s.",
                         strerror(errno));
      }

#endif /* !FSEEK_FSEEKO */
  }

extern void seek_end(FILE *fp,
        void (*error_handler)(void *data, const char *format, ...), void *data)
  {
#ifdef FSEEK_FSEEKO

    int return_value;

    assert(fp != NULL);
    assert(error_handler != NULL);

    return_value = fseeko(fp, 0, SEEK_END);
    if (return_value != 0)
      {
        (*error_handler)(data, "seek_end() on a file stream failed: %s.",
                         strerror(errno));
      }

#else /* !FSEEK_FSEEKO */

    int return_value;

    assert(fp != NULL);
    assert(error_handler != NULL);

    return_value = fseek(fp, 0, SEEK_END);
    if (return_value != 0)
      {
        (*error_handler)(data, "seek_end() on a file stream failed: %s.",
                         strerror(errno));
      }

#endif /* !FSEEK_FSEEKO */
  }

extern time_t get_time_with_microseconds(unsigned long *microseconds)
  {
#ifdef TIME_GETTIMEOFDAY

    struct timeval the_timeval;

    assert(microseconds != NULL);
    gettimeofday(&the_timeval, NULL);
    *microseconds = the_timeval.tv_usec;
    return the_timeval.tv_sec;

#else /* !TIME_GETTIMEOFDAY */

    assert(microseconds != NULL);
    *microseconds = 0;
    return time(NULL);

#endif /* !TIME_GETTIMEOFDAY */
  }

extern const char *time_zone_name_from_tm(struct tm *the_tm,
        boolean *know_if_daylight_savings, boolean *is_daylight_savings,
        boolean *know_seconds_ahead_of_utc, long *seconds_ahead_of_utc)
  {
    assert(the_tm != NULL);
    assert(know_if_daylight_savings != NULL);
    assert(is_daylight_savings != NULL);
    assert(know_seconds_ahead_of_utc != NULL);
    assert(seconds_ahead_of_utc != NULL);

#ifdef TIME_ZONE_POSIX

    *know_if_daylight_savings = TRUE;
    *is_daylight_savings = (the_tm->tm_isdst != 0);
    *know_seconds_ahead_of_utc = TRUE;
    *seconds_ahead_of_utc = -timezone;

    return tzname[the_tm->tm_isdst];

#elif defined(TIME_ZONE_CYGWIN)

    *know_if_daylight_savings = TRUE;
    *is_daylight_savings = (the_tm->tm_isdst != 0);
    *know_seconds_ahead_of_utc = TRUE;
    *seconds_ahead_of_utc = -_timezone;

    return tzname[the_tm->tm_isdst];

#else /* !TIME_ZONE_POSIX */

    *know_if_daylight_savings = FALSE;
    *is_daylight_savings = FALSE;
    *know_seconds_ahead_of_utc = FALSE;
    *seconds_ahead_of_utc = 0;

    return NULL;

#endif /* !TIME_ZONE_POSIX */
  }

extern char **directory_contents(const char *directory_name,
        void (*error_handler)(void *data, const char *format, ...), void *data)
  {
    assert(directory_name != NULL);
    assert(error_handler != NULL);

#ifdef DIRECTORY_CONTENTS_POSIX

    string_aa result;
    verdict the_verdict;
    DIR *dp;
    int return_code;

    the_verdict = string_aa_init(&result, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    dp = opendir(directory_name);
    if (dp == NULL)
      {
        free(result.array);
        (*error_handler)(data,
                "Unable to open directory `%s' for reading: %s.",
                directory_name, strerror(errno));
        return NULL;
      }

    while (TRUE)
      {
        struct dirent *entry;
        char *name_copy;

        entry = readdir(dp);
        if (entry == NULL)
            break;

        name_copy = MALLOC_ARRAY(char, strlen(entry->d_name) + 1);
        if (name_copy == NULL)
          {
            size_t element_num;

            for (element_num = 0; element_num < result.element_count;
                 ++element_num)
              {
                free(result.array[element_num]);
              }
            free(result.array);
            return NULL;
          }
        strcpy(name_copy, entry->d_name);

        the_verdict = string_aa_append(&result, name_copy);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            size_t element_num;

            free(name_copy);
            for (element_num = 0; element_num < result.element_count;
                 ++element_num)
              {
                free(result.array[element_num]);
              }
            free(result.array);
            return NULL;
          }
      }

    return_code = closedir(dp);
    if (return_code != 0)
      {
        size_t element_num;

        for (element_num = 0; element_num < result.element_count;
             ++element_num)
          {
            free(result.array[element_num]);
          }
        free(result.array);
        (*error_handler)(data,
                "Unable to close directory `%s' after reading: %s.",
                directory_name, strerror(errno));
        return NULL;
      }

    the_verdict = string_aa_append(&result, NULL);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        size_t element_num;

        for (element_num = 0; element_num < result.element_count;
             ++element_num)
          {
            free(result.array[element_num]);
          }
        free(result.array);
        return NULL;
      }

    return result.array;

#else /* !DIRECTORY_CONTENTS_POSIX */

    (*error_handler)(data,
            "directory_contents() is not supported on this platform.");
    return NULL;

#endif /* !DIRECTORY_CONTENTS_POSIX */
  }

extern boolean file_exists(const char *name)
  {
    assert(name != NULL);

#ifdef FILE_EXISTS_POSIX

    struct stat stat_buffer;
    int return_code;

    return_code = stat(name, &stat_buffer);
    if (return_code != 0)
        return FALSE;
    return !(S_ISDIR(stat_buffer.st_mode));

#else /* !FILE_EXISTS_POSIX */

    fprintf(stderr, "file_exists() is not supported on this platform.\n");
    return FALSE;

#endif /* !FILE_EXISTS_POSIX */
  }

extern boolean directory_exists(const char *name)
  {
    assert(name != NULL);

#ifdef DIRECTORY_EXISTS_POSIX

    struct stat stat_buffer;
    int return_code;

    return_code = stat(name, &stat_buffer);
    if (return_code != 0)
        return FALSE;
    return (S_ISDIR(stat_buffer.st_mode));

#else /* !DIRECTORY_EXISTS_POSIX */

    fprintf(stderr, "directory_exists() is not supported on this platform.\n");
    return FALSE;

#endif /* !DIRECTORY_EXISTS_POSIX */
  }

extern boolean path_is_absolute(const char *path)
  {
    assert(path != NULL);

#ifdef DOS_PATHS

    if ((path[0] == '/') || (path[0] == '\\') ||
        ((((path[0] >= 'a') && (path[0] <= 'z')) ||
          ((path[0] >= 'A') && (path[0] <= 'Z'))) && (path[1] == ':') &&
         ((path[2] == '/') || (path[2] == '\\'))))
      {
        return TRUE;
      }
    return FALSE;

#else /* !DOS_PATHS */

    if (path[0] == '/')
        return TRUE;
    return FALSE;

#endif /* !DOS_PATHS */
  }

extern char *file_name_directory(const char *name, boolean *error)
  {
    const char *last_slash;
    const char *follow;
    size_t char_count;
    char *directory;

    assert(name != NULL);

    last_slash = NULL;
    follow = name;
    while (*follow != 0)
      {
#ifdef DOS_PATHS
        if ((*follow == '/') || (*follow == '\\'))
#else /* !DOS_PATHS */
        if (*follow == '/')
#endif /* !DOS_PATHS */
          {
            last_slash = follow;
          }
        ++follow;
      }

    if (last_slash == NULL)
      {
        *error = FALSE;
        return NULL;
      }

    assert(last_slash >= name);

    char_count = (last_slash - name);
    directory = MALLOC_ARRAY(char, (char_count + 1));
    if (directory == NULL)
      {
        *error = TRUE;
        return NULL;
      }

    memcpy(directory, name, char_count);
    directory[char_count] = 0;

    *error = FALSE;
    return directory;
  }

extern boolean is_dynamic_library_name(const char *name)
  {
#ifdef DYNAMIC_LIBRARY_DLOPEN

    size_t suffix_num;

    for (suffix_num = 0; dynamic_library_suffixes[suffix_num] != NULL;
         ++suffix_num)
      {
        char *suffix;

        suffix = dynamic_library_suffixes[suffix_num];
        if ((strlen(name) > strlen(suffix)) &&
            (strcmp(&(name[strlen(name) - strlen(suffix)]), suffix) == 0))
          {
            return TRUE;
          }
      }

    return FALSE;

#else /* !DYNAMIC_LIBRARY_DLOPEN */

    return FALSE;

#endif /* !DYNAMIC_LIBRARY_DLOPEN */
  }

extern dynamic_library_handle *open_dynamic_library(const char *name,
        boolean *file_not_found, char **error_message)
  {
#ifdef DYNAMIC_LIBRARY_DLOPEN

    const char *follow;
    char *munged_name;
    dynamic_library_handle *result;

    /*
     * If the name doesn't have a slash in it, the dlopen() library uses its
     * own list of directories to look for the name.  We don't want that -- we
     * want to use it is a regular file name.  We have our own list of
     * directories to use outside of this call.  So here we prepend "./" if
     * there wasn't already a slash in the name.
     */

    follow = name;
    while (TRUE)
      {
        if (*follow == 0)
          {
            munged_name = MALLOC_ARRAY(char, strlen(name) + 3);
            if (munged_name == NULL)
              {
                if (file_not_found != NULL)
                    *file_not_found = FALSE;
                if (error_message != NULL)
                  {
                    static const char memory_message[] =
                            "An attempt to allocate memory failed.";
                    *error_message =
                            MALLOC_ARRAY(char, sizeof(memory_message) + 1);
                    if (*error_message != NULL)
                        strcpy(*error_message, memory_message);
                  }
                return NULL;
              }
            munged_name[0] = '.';
            munged_name[1] = '/';
            strcpy(&(munged_name[2]), name);
            break;
          }
        if (*follow == '/')
          {
            munged_name = (char *)name;
            break;
          }
        ++follow;
      }

    result = (dynamic_library_handle *)(dlopen(munged_name, RTLD_NOW));

    if (result == NULL)
      {
#ifdef DYNAMIC_LIBRARY_ERROR_MESSAGE_LINUX
        static const char not_found_suffix[] =
                "cannot open shared object file: No such file or directory";

#endif /* DYNAMIC_LIBRARY_ERROR_MESSAGE_LINUX */
        const char *reason;

        reason = dlerror();
#ifdef DYNAMIC_LIBRARY_ERROR_MESSAGE_LINUX
        size_t munged_length = strlen(munged_name) + 2;

        if ((strlen(reason) > munged_length) &&
            (strcmp(&(reason[munged_length]), not_found_suffix) == 0))
#else /* DYNAMIC_LIBRARY_ERROR_MESSAGE_LINUX */

        if (!(file_exists(munged_name)))
#endif /* !DYNAMIC_LIBRARY_ERROR_MESSAGE_LINUX */
          {
            if (file_not_found != NULL)
                *file_not_found = TRUE;
          }
        else
          {
            if (file_not_found != NULL)
                *file_not_found = FALSE;
            if (error_message != NULL)
              {
                *error_message = MALLOC_ARRAY(char, strlen(reason) + 1);
                if (*error_message != NULL)
                    strcpy(*error_message, reason);
              }
          }
      }

    if (munged_name != name)
        free(munged_name);

    return result;

#else /* !DYNAMIC_LIBRARY_DLOPEN */

    if (file_not_found != NULL)
        *file_not_found = FALSE;
    if (error_message != NULL)
      {
        static const char unsupported_message[] =
                "Shared libraries not supported on this platform.";

        *error_message = MALLOC_ARRAY(char, sizeof(unsupported_message) + 1);
        if (*error_message != NULL)
            strcpy(*error_message, unsupported_message);
      }

    return NULL;

#endif /* !DYNAMIC_LIBRARY_DLOPEN */
  }

extern void *find_dynamic_library_symbol(dynamic_library_handle *handle,
                                         const char *symbol_name)
  {
#ifdef DYNAMIC_LIBRARY_DLOPEN

    return dlsym(handle, symbol_name);

#else /* !DYNAMIC_LIBRARY_DLOPEN */

    return NULL;

#endif /* !DYNAMIC_LIBRARY_DLOPEN */
  }

extern void close_dynamic_library(dynamic_library_handle *handle)
  {
#ifdef DYNAMIC_LIBRARY_DLOPEN

    dlclose(handle);

#else /* !DYNAMIC_LIBRARY_DLOPEN */

#endif /* !DYNAMIC_LIBRARY_DLOPEN */
  }

extern char **alternate_dynamic_library_names(const char *original_name)
  {
    size_t original_length;
    size_t suffix_num;

    original_length = strlen(original_name);

    for (suffix_num = 0; dynamic_library_suffixes[suffix_num] != NULL;
         ++suffix_num)
      {
        char *suffix;

        suffix = dynamic_library_suffixes[suffix_num];
        if ((strlen(original_name) > strlen(suffix)) &&
            (strcmp(&(original_name[original_length - strlen(suffix)]), suffix)
             == 0))
          {
            char **result;
            size_t replacement_num;

            result = MALLOC_ARRAY(char *,
                    (sizeof(dynamic_library_suffixes) / sizeof(char *)) - 1);
            if (result == NULL)
                return NULL;

            for (replacement_num = 0;
                 dynamic_library_suffixes[replacement_num] != NULL;
                 ++replacement_num)
              {
                char *new_suffix;
                char *new_name;

                if (replacement_num == suffix_num)
                    continue;
                new_suffix = dynamic_library_suffixes[replacement_num];

                new_name = MALLOC_ARRAY(char,
                        (original_length + 1 + strlen(new_suffix)) -
                        strlen(suffix));
                if (new_name == NULL)
                  {
                    while (replacement_num > 0)
                      {
                        --replacement_num;
                        if (replacement_num != suffix_num)
                            free(result[replacement_num]);
                      }
                    free(result);
                    return NULL;
                  }

                memcpy(new_name, original_name,
                       original_length - strlen(suffix));
                strcpy(&(new_name[original_length - strlen(suffix)]),
                       new_suffix);

                result[(replacement_num < suffix_num) ? replacement_num :
                       (replacement_num - 1)] = new_name;
              }

            result[replacement_num - 1] = NULL;

            return result;
          }
      }

    return NULL;
  }

extern boolean current_process_stack_size_limit_known(void)
  {
#ifdef STACK_LIMIT_GETRLIMIT

    return TRUE;

#else /* !STACK_LIMIT_GETRLIMIT */

    return FALSE;

#endif /* !STACK_LIMIT_GETRLIMIT */
  }

extern size_t current_process_stack_size_limit(void)
  {
#ifdef STACK_LIMIT_GETRLIMIT

    struct rlimit limit;

    getrlimit(RLIMIT_STACK, &limit);
    return (size_t)(limit.rlim_cur);

#else /* !STACK_LIMIT_GETRLIMIT */

    assert(FALSE);
    return 0;

#endif /* !STACK_LIMIT_GETRLIMIT */
  }

extern void cleanup_platform_dependent_module(void)
  {
#ifdef MULTI_THREADED
#ifndef NDEBUG

    pthread_mutex_lock(&local_lock);

    while (all_thread_info != NULL)
      {
        thread_lock_info *next;

        next = all_thread_info->next;
        assert(all_thread_info->waiting_for == NULL);
        free(all_thread_info);
        all_thread_info = next;
      }

    pthread_mutex_unlock(&local_lock);

#endif /* !NDEBUG */
#endif /* MULTI_THREADED */
  }


#ifdef MULTI_THREADED

#ifndef NDEBUG

pthread_mutex_t meta_lock = PTHREAD_MUTEX_INITIALIZER;

extern thread_lock_info *current_thread_lock_info(void)
  {

    pthread_t handle;
    thread_lock_info **follow;
    thread_lock_info *result;

    pthread_mutex_lock(&local_lock);

    handle = pthread_self();
    follow = &all_thread_info;
    while (TRUE)
      {
        assert(follow != NULL);

        if (*follow == NULL)
          {
            result = MALLOC_ONE_OBJECT(thread_lock_info);
            assert(result != NULL);
            result->waiting_for = NULL;
            result->handle = handle;
            result->next = all_thread_info;
            all_thread_info = result;
            break;
          }

        if ((*follow)->handle == handle)
          {
            result = *follow;
            if (follow != &(all_thread_info))
              {
                *follow = result->next;
                result->next = all_thread_info;
                all_thread_info = result;
              }
            break;
          }

        follow = &((*follow)->next);
      }

    pthread_mutex_unlock(&local_lock);

    return result;
  }

#endif /* !NDEBUG */

#endif /* MULTI_THREADED */


#ifdef REDIRECT_SYSTEM_POSIX

static char *temporary_file_name(void)
  {
    const char *base_name;
    char *result_name;
    int file_descriptor;
    int close_result;

    base_name = tmpnam(NULL);
    assert(base_name != NULL);

    result_name = MALLOC_ARRAY(char, strlen(base_name) + 7);
    if (result_name == NULL)
        return NULL;

    sprintf(result_name, "%sXXXXXX", base_name);

    file_descriptor = mkstemp(result_name);
    if (file_descriptor == -1)
      {
        free(result_name);
        fprintf(stderr,
                "ERROR: mkstemp() failed while trying to generate a temporary "
                "file for redirecting standard error.\n");
        return NULL;
      }

    close_result = close(file_descriptor);
    if (close_result != 0)
      {
        fprintf(stderr, "ERROR: close() on temporary file `%s' failed: %s.\n",
                result_name, strerror(errno));
        free(result_name);
        return NULL;
      }

    return result_name;
  }

static char *read_whole_text_file(const char *file_name)
  {
    FILE *fp;
    string_buffer result_buffer;
    verdict the_verdict;

    fp = fopen(file_name, "r");
    if (fp == NULL)
      {
        fprintf(stderr, "ERROR: Unable to read temporary file `%s': %s.\n",
                file_name, strerror(errno));
        return NULL;
      }

    the_verdict = string_buffer_init(&result_buffer, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        fclose(fp);
        return NULL;
      }

    while (!feof(fp))
      {
        int new_char;

        new_char = fgetc(fp);
        if (new_char == EOF)
            break;

        the_verdict = string_buffer_append(&result_buffer, (char)new_char);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            fclose(fp);
            free(result_buffer.array);
            return NULL;
          }
      }

    if (ferror(fp))
      {
        fprintf(stderr, "ERROR: Unable to read temporary file `%s': %s.\n",
                file_name, strerror(errno));
        fclose(fp);
        free(result_buffer.array);
        return NULL;
      }

    fclose(fp);

    the_verdict = string_buffer_append(&result_buffer, 0);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result_buffer.array);
        return NULL;
      }

    return result_buffer.array;
  }

#endif /* REDIRECT_SYSTEM_POSIX */

#ifdef FTELL_FTELLO

static o_integer oi_create_from_off_t(off_t the_off_t)
  {
    o_integer accumulator;
    o_integer factor;
    o_integer factor_mult;
    off_t remainder;
    off_t divisor;

    if ((the_off_t >= 0) && (the_off_t <= ~(size_t)0))
      {
        o_integer result;

        oi_create_from_size_t(result, (size_t)the_off_t);
        return result;
      }

    if ((the_off_t >= LONG_MIN) && (the_off_t <= LONG_MAX))
      {
        o_integer result;

        oi_create_from_long_int(result, (size_t)the_off_t);
        return result;
      }

    accumulator = oi_zero;
    assert(!(oi_out_of_memory(accumulator)));
    oi_add_reference(accumulator);

    factor = oi_one;
    assert(!(oi_out_of_memory(factor)));
    oi_add_reference(factor);

    oi_create_from_long_int(factor_mult,
                            (the_off_t < 0) ? LONG_MIN : LONG_MAX);
    if (oi_out_of_memory(factor_mult))
      {
        oi_remove_reference(factor);
        oi_remove_reference(accumulator);
        return oi_null;
      }

    remainder = the_off_t;

    if (the_off_t < 0)
        divisor = LONG_MIN;
    else
        divisor = LONG_MAX;

    while (TRUE)
      {
        off_t next_remainder;
        off_t chunk;
        o_integer chunk_oi;
        o_integer to_add;
        o_integer new_accumulator;
        o_integer new_factor;

        next_remainder = remainder / divisor;
        assert(next_remainder != 0);

        chunk = remainder % divisor;
        if (the_off_t > 0)
          {
            assert(chunk >= 0);
            assert(chunk < divisor);
          }
        else
          {
            if (chunk > 0)
              {
                chunk += divisor;
                next_remainder -= 1;
              }

            assert(chunk <= 0);
            assert(chunk > divisor);
          }

        assert((chunk >= LONG_MIN) && (chunk <= LONG_MAX));

        oi_create_from_long_int(chunk_oi, (size_t)chunk);
        if (oi_out_of_memory(chunk_oi))
          {
            oi_remove_reference(factor_mult);
            oi_remove_reference(factor);
            oi_remove_reference(accumulator);
            return oi_null;
          }

        oi_multiply(to_add, chunk_oi, factor);
        oi_remove_reference(chunk_oi);
        if (oi_out_of_memory(to_add))
          {
            oi_remove_reference(factor_mult);
            oi_remove_reference(factor);
            oi_remove_reference(accumulator);
            return oi_null;
          }

        oi_add(new_accumulator, accumulator, to_add);
        oi_remove_reference(accumulator);
        oi_remove_reference(to_add);
        accumulator = new_accumulator;

        if ((next_remainder >= LONG_MIN) && (next_remainder <= LONG_MAX))
          {
            o_integer last_chunk_oi;
            o_integer last_to_add;
            o_integer result;

            oi_remove_reference(factor_mult);

            oi_create_from_long_int(last_chunk_oi, (size_t)next_remainder);
            if (oi_out_of_memory(last_chunk_oi))
              {
                oi_remove_reference(factor);
                oi_remove_reference(accumulator);
                return oi_null;
              }

            oi_multiply(last_to_add, last_chunk_oi, factor);
            oi_remove_reference(last_chunk_oi);
            oi_remove_reference(factor);
            if (oi_out_of_memory(last_to_add))
              {
                oi_remove_reference(accumulator);
                return oi_null;
              }

            oi_add(result, accumulator, last_to_add);
            oi_remove_reference(accumulator);
            oi_remove_reference(last_to_add);
            return result;
          }

        oi_multiply(new_factor, factor, factor_mult);
        oi_remove_reference(factor);
        if (oi_out_of_memory(new_factor))
          {
            oi_remove_reference(factor_mult);
            oi_remove_reference(accumulator);
            return oi_null;
          }
        factor = new_factor;

        remainder = next_remainder;
      }
  }

#endif /* FTELL_FTELLO */

#ifdef FSEEK_FSEEKO

static verdict oi_to_off_t(o_integer oi, off_t *result)
  {
    long int the_long_int;
    verdict the_verdict;
    size_t digit_count;
    char digit_buffer[sizeof(off_t) * 3 + 2];
    off_t accumulator;
    size_t digit_num;

    assert(oi_kind(oi) == IIK_FINITE);

    the_verdict = oi_to_long_int(oi, &the_long_int);
    if (the_verdict == MISSION_ACCOMPLISHED)
      {
        off_t the_off_t;

        the_off_t = (off_t)the_long_int;
        if (((long int)the_off_t) != the_long_int)
            return MISSION_FAILED;

        *result = the_off_t;
        return MISSION_ACCOMPLISHED;
      }

    the_verdict = oi_decimal_digit_count(oi, &digit_count);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return MISSION_FAILED;

    if (digit_count > (sizeof(off_t) * 3))
        return MISSION_FAILED;

    the_verdict = oi_write_decimal_digits(oi, &(digit_buffer[0]));
    if (the_verdict != MISSION_ACCOMPLISHED)
        return MISSION_FAILED;

    accumulator = 0;

    for (digit_num = 0; digit_num < digit_count; ++digit_num)
      {
        off_t new_accumulator;
        off_t digit;

        new_accumulator = accumulator * 10;
        if ((new_accumulator / 10) != accumulator)
            return MISSION_FAILED;
        accumulator = new_accumulator;

        digit = digit_buffer[digit_num] - '0';
        if (oi_is_negative(oi))
            digit = -digit;

        new_accumulator = accumulator + digit;
        if (new_accumulator - digit != accumulator)
            return MISSION_FAILED;
        accumulator = new_accumulator;
      }

    *result = accumulator;
    return MISSION_ACCOMPLISHED;
  }

#endif /* FSEEK_FSEEKO */
