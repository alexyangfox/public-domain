/* file "tokenizer.h" */

/*
 *  This file contains the interface to the tokenizer module, which breaks an
 *  input file into tokens.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "token.h"
#include "source_location.h"


typedef struct tokenizer tokenizer;


extern tokenizer *create_tokenizer(const char *input_characters,
                                   const char *source_file_name);
extern void delete_tokenizer(tokenizer *the_tokenizer);

extern token *next_token(tokenizer *the_tokenizer);
extern token *forward_token(tokenizer *the_tokenizer, size_t forward_count);
extern verdict consume_token(tokenizer *the_tokenizer);
extern void set_tokenizer_line_number(tokenizer *the_tokenizer,
                                      size_t new_line_number);
extern void set_tokenizer_column_number(tokenizer *the_tokenizer,
                                        size_t new_column_number);
extern const char *tokenizer_raw_position(tokenizer *the_tokenizer);

extern const source_location *next_token_location(tokenizer *the_tokenizer);


#endif /* TOKENIZER_H */
