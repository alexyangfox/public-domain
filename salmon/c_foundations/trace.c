/* file "trace.c" */

/*
 *  This file contains the implementation of code for doing tracing.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#ifdef MULTI_THREADED
#include <pthread.h>
#endif /* MULTI_THREADED */
#include "basic.h"
#include "trace.h"
#include "diagnostic.h"
#include "memory_allocation.h"
#include "print_formatting.h"
#include "buffer_print.h"
#include "auto_array.h"
#include "auto_array_implementation.h"


/*
 *      Data Structures
 *
 *  @@@
 *  @@@
 *
 *
 *      Algorithm
 *
 *  @@@
 *  @@@
 *
 *      Functions
 *
 *  @@@
 *  @@@
 *
 *
 *      History
 *
 *  See the history discussion in trace.h for the history of this code.
 *
 *
 *      Legal Issues
 *
 *  I've written this code from scratch, without using or refering to any other
 *  code.  I've written it on my own equipment and not for hire for anyone
 *  else, so I have full legal rights to place it in the public domain.
 *
 *  I've chosen to put this software in the public domain rather than
 *  copyrighting it and using the FSF's GPL or a Berkeley-style ``vanity''
 *  license because my personal opinion is that making it public domain
 *  maximizes its potential usefulness to others.  Anyone can feel free to use
 *  it for any purpose, including with their own proprietary code or with GPL
 *  code, without fear of intellectual property issues.  I have no desire to
 *  stop anyone else from making money on this code or getting any other
 *  advantages they can from it.
 *
 *  I do request that anyone who finds this software useful let me know about
 *  it.  You can drop me e-mail at "Chris Wilson" <chris@chriswilson.info> to
 *  let me know how you are using it and what is good and bad about it for you.
 *  Bug reports are also appreciated.  Also, if you release a product or
 *  software package of some sort that includes this software, I would like you
 *  to give me credit in the documentation as appropriate for the importance of
 *  my code in your product.  These are requests, not requirements, so you are
 *  not legally bound to do them, they're just a nice way to show appreciation.
 *
 *  Note that even though this software is public domain and there are no
 *  copyright laws that limit what you can do with it, other laws may apply.
 *  For example, if you lie and claim that you wrote this code when you did
 *  not, or you claim that I endorse a product of yours when I do not, that
 *  could be fraud and you could be legally liable.
 *
 *  There is absolutely no warranty for this software!  I am warning you now
 *  that it may or may not work.  It may have bugs that cause you a lot of
 *  problems.  I disclaim any implied warranties for merchantability or fitness
 *  for a particular purpose.  The fact that I have written some documentation
 *  on what I intended this software for should not be taken as any kind of
 *  warranty that it will actually behave that way.  I am providing this
 *  software as-is in the hope that it will be useful.
 *
 *          Chris Wilson, 2005, 2007, 2009
 */


typedef struct
  {
    char *name;
    FILE *file_pointer;
  } open_file;

AUTO_ARRAY(open_file_aa, open_file);

struct tracer
  {
    void (*vfile_printer)(FILE *fp, const char *format, va_list arg);
    size_t channel_count;
    const char **channel_names;
    char **patterns;
    char **descriptions;
    open_file_aa open_files;
    FILE **channel_file_pointers;
    FILE *open_channel;
#ifdef MULTI_THREADED
    pthread_mutex_t mutex;
#endif /* MULTI_THREADED */
  };


#define TRACE_OPTION_PREFIX "-trace-"


AUTO_ARRAY_IMPLEMENTATION(open_file_aa, open_file, 0);


static void file_print(tracer *the_tracer, FILE *fp, const char *format, ...);
static void vfile_print(FILE *fp, const char *format, va_list arg);
static verdict file_send_character(void *data, char output_character);


extern tracer *create_tracer(size_t channel_count, const char **channel_names)
  {
    tracer *result;
    verdict the_verdict;
    size_t channel_num;

    assert(channel_count > 0);
    assert(channel_names != NULL);

    result = MALLOC_ONE_OBJECT(tracer);
    if (result == NULL)
        return NULL;

    result->vfile_printer = &vfile_print;
    result->channel_count = channel_count;
    result->channel_names = channel_names;

    result->patterns = MALLOC_ARRAY(char *, channel_count * 3);
    if (result->patterns == NULL)
      {
        free(result);
        return NULL;
      }

    result->descriptions = MALLOC_ARRAY(char *, channel_count * 3);
    if (result->descriptions == NULL)
      {
        free(result->patterns);
        free(result);
        return NULL;
      }

    the_verdict = open_file_aa_init(&(result->open_files), 2);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->descriptions);
        free(result->patterns);
        free(result);
        return NULL;
      }

    result->channel_file_pointers = MALLOC_ARRAY(FILE *, channel_count);
    if (result->channel_file_pointers == NULL)
      {
        free(result->open_files.array);
        free(result->descriptions);
        free(result->patterns);
        free(result);
        return NULL;
      }

    for (channel_num = 0; channel_num < channel_count; ++channel_num)
      {
        size_t sub_num;

        for (sub_num = 0; sub_num < 3; ++sub_num)
          {
            string_buffer buffer;
            verdict the_verdict;

            the_verdict = string_buffer_init(&buffer, 10);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                while (sub_num > 0)
                  {
                    --sub_num;
                    free(result->patterns[(channel_num * 3) + sub_num]);
                    free(result->descriptions[(channel_num * 3) + sub_num]);
                  }
                while (channel_num > 0)
                  {
                    --channel_num;
                    free(result->patterns[(channel_num * 3) + 0]);
                    free(result->patterns[(channel_num * 3) + 1]);
                    free(result->patterns[(channel_num * 3) + 2]);
                    free(result->descriptions[(channel_num * 3) + 0]);
                    free(result->descriptions[(channel_num * 3) + 1]);
                    free(result->descriptions[(channel_num * 3) + 2]);
                  }
                free(result->channel_file_pointers);
                free(result->open_files.array);
                free(result->descriptions);
                free(result->patterns);
                free(result);
                return NULL;
              }

              {
                static const char *formats[3] =
                  {
                    TRACE_OPTION_PREFIX "%s-stdout",
                    TRACE_OPTION_PREFIX "%s-stderr",
                    TRACE_OPTION_PREFIX "%s-file <file>"
                  };

                buffer_printf(&buffer, 0, formats[sub_num],
                              channel_names[channel_num]);
              }

            result->patterns[(channel_num * 3) + sub_num] = buffer.array;

            the_verdict = string_buffer_init(&buffer, 10);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                free(result->patterns[(channel_num * 3) + sub_num]);
                while (sub_num > 0)
                  {
                    --sub_num;
                    free(result->patterns[(channel_num * 3) + sub_num]);
                    free(result->descriptions[(channel_num * 3) + sub_num]);
                  }
                while (channel_num > 0)
                  {
                    --channel_num;
                    free(result->patterns[(channel_num * 3) + 0]);
                    free(result->patterns[(channel_num * 3) + 1]);
                    free(result->patterns[(channel_num * 3) + 2]);
                    free(result->descriptions[(channel_num * 3) + 0]);
                    free(result->descriptions[(channel_num * 3) + 1]);
                    free(result->descriptions[(channel_num * 3) + 2]);
                  }
                free(result->channel_file_pointers);
                free(result->open_files.array);
                free(result->descriptions);
                free(result->patterns);
                free(result);
                return NULL;
              }

              {
                static const char *formats[3] =
                  {
                    "Trace the %s channel to standard output",
                    "Trace the %s channel to standard error",
                    "Trace the %s channel to file <file>"
                  };

                buffer_printf(&buffer, 0, formats[sub_num],
                              channel_names[channel_num]);
              }

            result->descriptions[(channel_num * 3) + sub_num] = buffer.array;
          }

        result->channel_file_pointers[channel_num] = NULL;
      }

    result->open_channel = NULL;

#ifdef MULTI_THREADED
    pthread_mutex_init(&(result->mutex), NULL);
#endif /* MULTI_THREADED */

    return result;
  }

extern void delete_tracer(tracer *the_tracer)
  {
    size_t open_count;
    size_t open_num;
    size_t option_count;
    size_t option_num;

    assert(the_tracer != NULL);

    open_count = the_tracer->open_files.element_count;
    for (open_num = 0; open_num < open_count; ++open_num)
      {
        free(the_tracer->open_files.array[open_num].name);
        fclose(the_tracer->open_files.array[open_num].file_pointer);
      }
    free(the_tracer->open_files.array);

    free(the_tracer->channel_file_pointers);

    option_count = (the_tracer->channel_count * 3);

    for (option_num = 0; option_num < option_count; ++option_num)
      {
        free(the_tracer->patterns[option_num]);
        free(the_tracer->descriptions[option_num]);
      }

    free(the_tracer->patterns);
    free(the_tracer->descriptions);

    free(the_tracer);
  }

extern void tracer_set_output_handler(tracer *the_tracer,
        void (*handler)(FILE *fp, const char *format, va_list arg))
  {
    assert(the_tracer != NULL);
    assert(handler != NULL);

    the_tracer->vfile_printer = handler;
  }

extern int handle_tracer_option(tracer *the_tracer, int argc, char *argv[])
  {
    const char *tail;
    size_t channel_count;
    size_t channel_num;

    assert(the_tracer != NULL);
    assert(argv != NULL);

    if (argc < 1)
        return 0;

    assert(argv[0] != NULL);

    if (strncmp(argv[0], TRACE_OPTION_PREFIX, strlen(TRACE_OPTION_PREFIX)) !=
        0)
      {
        return 0;
      }

    tail = argv[0] + strlen(TRACE_OPTION_PREFIX);

    channel_count = the_tracer->channel_count;

    for (channel_num = 0; channel_num < channel_count; ++channel_num)
      {
        const char *channel_name;
        const char *suffix;

        channel_name = the_tracer->channel_names[channel_num];
        if (strncmp(tail, channel_name, strlen(channel_name)) != 0)
            continue;

        suffix = tail + strlen(channel_name);

        if (strcmp(suffix, "-stdout") == 0)
          {
            the_tracer->channel_file_pointers[channel_num] = stdout;
            return 1;
          }
        else if (strcmp(suffix, "-stderr") == 0)
          {
            the_tracer->channel_file_pointers[channel_num] = stderr;
            return 1;
          }
        else if (strcmp(suffix, "-file") == 0)
          {
            const char *file_name;
            size_t open_count;
            size_t open_num;
            FILE *file_pointer;

            if (argc < 2)
              {
                basic_error("Option `%s' requires a <file-name> argument.",
                            argv[0]);
                return -1;
              }

            file_name = argv[1];
            assert(file_name != NULL);

            open_count = the_tracer->open_files.element_count;
            open_num = 0;

            while (TRUE)
              {
                if (open_num >= open_count)
                  {
                    char *name_copy;
                    open_file new_item;
                    verdict the_verdict;

                    file_pointer = fopen(file_name, "w");
                    if (file_pointer == NULL)
                      {
                        basic_error(
                                "Unable to open trace output file `%s': %s.",
                                file_name, strerror(errno));
                        return -1;
                      }

                    name_copy = MALLOC_ARRAY(char, strlen(file_name) + 1);
                    if (name_copy == NULL)
                      {
                        fclose(file_pointer);
                        return -1;
                      }
                    strcpy(name_copy, file_name);

                    new_item.name = name_copy;
                    new_item.file_pointer = file_pointer;

                    the_verdict = open_file_aa_append(
                            &(the_tracer->open_files), new_item);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        free(name_copy);
                        fclose(file_pointer);
                        return -1;
                      }

                    break;
                  }

                if (strcmp(the_tracer->open_files.array[open_num].name,
                           file_name) == 0)
                  {
                    file_pointer = the_tracer->open_files.array[open_num].
                            file_pointer;
                    assert(file_pointer != NULL);
                    break;
                  }

                ++open_num;
              }

            assert(file_pointer != NULL);
            the_tracer->channel_file_pointers[channel_num] = file_pointer;

            return 2;
          }
      }

    return 0;
  }

extern size_t tracer_option_count(tracer *the_tracer)
  {
    assert(the_tracer != NULL);

    return ((the_tracer->channel_count) * 3);
  }

extern const char *tracer_option_pattern(tracer *the_tracer, size_t option_num)
  {
    assert(the_tracer != NULL);

    assert(option_num < ((the_tracer->channel_count) * 3));

    return the_tracer->patterns[option_num];
  }

extern const char *tracer_option_description(tracer *the_tracer,
                                             size_t option_num)
  {
    assert(the_tracer != NULL);

    assert(option_num < ((the_tracer->channel_count) * 3));

    return the_tracer->descriptions[option_num];
  }

extern void trace(tracer *the_tracer, size_t channel, const char *format, ...)
  {
    va_list ap;

    assert(the_tracer != NULL);

    assert(channel < the_tracer->channel_count);

    if (the_tracer->channel_file_pointers[channel] == NULL)
        return;

    va_start(ap, format);
    vtrace(the_tracer, channel, format, ap);
    va_end(ap);
  }

extern void vtrace(tracer *the_tracer, size_t channel, const char *format,
                   va_list arg)
  {
    assert(the_tracer != NULL);

    assert(channel < the_tracer->channel_count);

    open_trace_item(the_tracer, channel);
    vtrace_text(the_tracer, format, arg);
    close_trace_item(the_tracer);
  }

extern void open_trace_item(tracer *the_tracer, size_t channel)
  {
    assert(the_tracer != NULL);

    assert(channel < the_tracer->channel_count);

#ifdef MULTI_THREADED
    pthread_mutex_lock(&(the_tracer->mutex));
#endif /* MULTI_THREADED */

    assert(the_tracer->open_channel == NULL);
    the_tracer->open_channel = the_tracer->channel_file_pointers[channel];

    if (the_tracer->open_channel != NULL)
      {
        file_print(the_tracer, the_tracer->open_channel, "%s: ",
                   the_tracer->channel_names[channel]);
      }
  }

extern void trace_text(tracer *the_tracer, const char *format, ...)
  {
    va_list ap;

    assert(the_tracer != NULL);

    va_start(ap, format);
    vtrace_text(the_tracer, format, ap);
    va_end(ap);
  }

extern void vtrace_text(tracer *the_tracer, const char *format, va_list arg)
  {
    assert(the_tracer != NULL);

    if (the_tracer->open_channel == NULL)
        return;

    (*(the_tracer->vfile_printer))(the_tracer->open_channel, format, arg);
  }

extern void close_trace_item(tracer *the_tracer)
  {
    assert(the_tracer != NULL);

    if (the_tracer->open_channel != NULL)
        file_print(the_tracer, the_tracer->open_channel, "\n");

    the_tracer->open_channel = NULL;

#ifdef MULTI_THREADED
    pthread_mutex_unlock(&(the_tracer->mutex));
#endif /* MULTI_THREADED */
  }


static void file_print(tracer *the_tracer, FILE *fp, const char *format, ...)
  {
    va_list ap;

    assert(the_tracer != NULL);
    assert(fp != NULL);
    assert(format != NULL);

    va_start(ap, format);
    (*(the_tracer->vfile_printer))(fp, format, ap);
    va_end(ap);
  }

static void vfile_print(FILE *fp, const char *format, va_list arg)
  {
    assert(fp != NULL);
    assert(format != NULL);

    do_print_formatting(fp, &file_send_character, format, arg);
  }

static verdict file_send_character(void *data, char output_character)
  {
    FILE *fp;
    int return_value;

    assert(data != NULL);

    fp = (FILE *)data;
    return_value = fprintf(fp, "%c", output_character);
    if (return_value != 1)
      {
        basic_error("Failed writing to file: %s.", strerror(errno));
        return MISSION_FAILED;
      }

    return MISSION_ACCOMPLISHED;
  }
