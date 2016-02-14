/* file "file_parser.c" */

/*
 *  This file contains the implementation of the file_parser module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "c_foundations/basic.h"
#include "c_foundations/diagnostic.h"
#include "c_foundations/buffer_print.h"
#include "c_foundations/memory_allocation.h"
#include "file_parser.h"
#include "parser.h"
#include "tokenizer.h"
#include "include.h"


#define BLOCK_SIZE 512


struct file_parser
  {
    string_buffer input_buffer;
    tokenizer *tokenizer;
    parser *parser;
  };


static verdict read_file_to_buffer(const char *file_name,
        string_buffer *buffer, const source_location *location,
        boolean *cant_open_file);
static verdict read_fp_to_buffer(FILE *fp, const char *file_name,
        string_buffer *buffer, const source_location *location,
        boolean *cant_open_file);


extern file_parser *create_file_parser(const char *file_name, FILE *fp,
        const source_location *location, include_handler_type include_handler,
        interface_include_handler_type interface_include_handler,
        void *include_handler_data, alias_manager *parent_alias_manager,
        boolean *cant_open_file, boolean native_bridge_dll_body_allowed)
  {
    file_parser *result;
    verdict the_verdict;

    if (cant_open_file != NULL)
        *cant_open_file = FALSE;

    result = MALLOC_ONE_OBJECT(file_parser);
    if (result == NULL)
        return NULL;

    the_verdict = string_buffer_init(&(result->input_buffer), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    if (fp == NULL)
      {
        the_verdict = read_file_to_buffer(file_name, &(result->input_buffer),
                                          location, cant_open_file);
      }
    else
      {
        the_verdict = read_fp_to_buffer(fp, file_name, &(result->input_buffer),
                                        location, cant_open_file);
      }
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->input_buffer.array);
        free(result);
        return NULL;
      }

    result->tokenizer =
            create_tokenizer(result->input_buffer.array, file_name);
    if (result->tokenizer == NULL)
      {
        free(result->input_buffer.array);
        free(result);
        return NULL;
      }

    result->parser = create_parser(result->tokenizer, include_handler,
            interface_include_handler, include_handler_data,
            parent_alias_manager, native_bridge_dll_body_allowed);
    if (result->parser == NULL)
      {
        delete_tokenizer(result->tokenizer);
        free(result->input_buffer.array);
        free(result);
        return NULL;
      }

    return result;
  }

extern void delete_file_parser(file_parser *the_file_parser)
  {
    assert(the_file_parser != NULL);

    delete_tokenizer(the_file_parser->tokenizer);
    free(the_file_parser->input_buffer.array);
    delete_parser(the_file_parser->parser);
    free(the_file_parser);
  }

extern parser *file_parser_parser(file_parser *the_file_parser)
  {
    assert(the_file_parser != NULL);

    return the_file_parser->parser;
  }

extern tokenizer *file_parser_tokenizer(file_parser *the_file_parser)
  {
    assert(the_file_parser != NULL);

    return the_file_parser->tokenizer;
  }


static verdict read_file_to_buffer(const char *file_name,
        string_buffer *buffer, const source_location *location,
        boolean *cant_open_file)
  {
    FILE *fp;
    verdict result;

    assert(file_name != NULL);

    fp = fopen(file_name, "r");
    if (fp == NULL)
      {
        if (cant_open_file == NULL)
          {
            location_error(location, "Unable to open file `%s': %s.",
                           file_name, strerror(errno));
          }
        else
          {
            *cant_open_file = FALSE;
          }

        return MISSION_FAILED;
      }

    result =
            read_fp_to_buffer(fp, file_name, buffer, location, cant_open_file);
    fclose(fp);
    return result;
  }

static verdict read_fp_to_buffer(FILE *fp, const char *file_name,
        string_buffer *buffer, const source_location *location,
        boolean *cant_open_file)
  {
    verdict the_verdict;

    while (TRUE)
      {
        char block_buffer[BLOCK_SIZE];
        size_t bytes_read;
        verdict the_verdict;

        bytes_read = fread(block_buffer, 1, BLOCK_SIZE, fp);
        if (ferror(fp))
          {
            basic_error("Failed while reading file `%s': %s.", file_name,
                        strerror(errno));
            return MISSION_FAILED;
          }

        the_verdict =
                string_buffer_append_array(buffer, bytes_read, block_buffer);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return MISSION_FAILED;

        if (bytes_read < BLOCK_SIZE)
            break;
      }

    the_verdict = string_buffer_append(buffer, 0);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return MISSION_FAILED;

    return MISSION_ACCOMPLISHED;
  }
