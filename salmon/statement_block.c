/* file "statement_block.c" */

/*
 *  This file contains the implementation of the statement_block module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/auto_array.h"
#include "c_foundations/auto_array_implementation.h"
#include "statement_block.h"
#include "statement.h"
#include "declaration.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "tagalong_declaration.h"
#include "lepton_key_declaration.h"
#include "quark_declaration.h"
#include "lock_declaration.h"
#include "routine_declaration_chain.h"


AUTO_ARRAY(statement_aa, statement *);
AUTO_ARRAY(routine_declaration_aa, routine_declaration *);


struct statement_block
  {
    statement_aa statements;
    declaration_list *declaration_list;
    routine_declaration_aa dangling_routines;
    statement_aa use_statements;
    source_location location;
  };


AUTO_ARRAY_IMPLEMENTATION(statement_aa, statement *, 0);
AUTO_ARRAY_IMPLEMENTATION(routine_declaration_aa, routine_declaration *, 0);


extern statement_block *create_statement_block(void)
  {
    statement_block *result;
    verdict the_verdict;

    result = MALLOC_ONE_OBJECT(statement_block);
    if (result == NULL)
        return NULL;

    the_verdict = statement_aa_init(&(result->statements), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    result->declaration_list = create_declaration_list();
    if (result->declaration_list == NULL)
      {
        free(result->statements.array);
        free(result);
        return NULL;
      }

    the_verdict =
            routine_declaration_aa_init(&(result->dangling_routines), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_declaration_list(result->declaration_list);
        free(result->statements.array);
        free(result);
        return NULL;
      }

    the_verdict = statement_aa_init(&(result->use_statements), 1);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->dangling_routines.array);
        delete_declaration_list(result->declaration_list);
        free(result->statements.array);
        free(result);
        return NULL;
      }

    set_source_location(&(result->location), NULL);

    return result;
  }

extern void delete_statement_block(statement_block *the_statement_block)
  {
    statement **array;
    size_t element_count;
    size_t element_num;

    assert(the_statement_block != NULL);

    array = the_statement_block->statements.array;
    assert(array != NULL);

    element_count = the_statement_block->statements.element_count;
    for (element_num = 0; element_num < element_count; ++element_num)
        delete_statement(array[element_num]);
    free(array);

    delete_declaration_list(the_statement_block->declaration_list);

    free(the_statement_block->dangling_routines.array);

    free(the_statement_block->use_statements.array);

    source_location_remove_reference(&(the_statement_block->location));

    free(the_statement_block);
  }

extern size_t statement_block_statement_count(
        statement_block *the_statement_block)
  {
    assert(the_statement_block != NULL);

    return the_statement_block->statements.element_count;
  }

extern statement *statement_block_statement(
        statement_block *the_statement_block, size_t statement_number)
  {
    assert(the_statement_block != NULL);
    assert(statement_number < the_statement_block->statements.element_count);

    assert(the_statement_block->statements.array != NULL);
    return the_statement_block->statements.array[statement_number];
  }

extern size_t statement_block_name_count(statement_block *the_statement_block)
  {
    assert(the_statement_block != NULL);

    return declaration_list_name_count(the_statement_block->declaration_list);
  }

extern name_kind statement_block_name_kind(
        statement_block *the_statement_block, size_t name_number)
  {
    assert(the_statement_block != NULL);

    return declaration_list_name_kind(the_statement_block->declaration_list,
                                      name_number);
  }

extern declaration *statement_block_name_declaration(
        statement_block *the_statement_block, size_t name_number)
  {
    assert(the_statement_block != NULL);

    return declaration_list_name_declaration(
            the_statement_block->declaration_list, name_number);
  }

extern variable_declaration *statement_block_name_variable_declaration(
        statement_block *the_statement_block, size_t name_number)
  {
    assert(the_statement_block != NULL);

    return declaration_list_name_variable_declaration(
            the_statement_block->declaration_list, name_number);
  }

extern routine_declaration *statement_block_name_routine_declaration(
        statement_block *the_statement_block, size_t name_number)
  {
    assert(the_statement_block != NULL);

    return declaration_list_name_routine_declaration(
            the_statement_block->declaration_list, name_number);
  }

extern tagalong_declaration *statement_block_name_tagalong_declaration(
        statement_block *the_statement_block, size_t name_number)
  {
    assert(the_statement_block != NULL);

    return declaration_list_name_tagalong_declaration(
            the_statement_block->declaration_list, name_number);
  }

extern lepton_key_declaration *statement_block_name_lepton_key_declaration(
        statement_block *the_statement_block, size_t name_number)
  {
    assert(the_statement_block != NULL);

    return declaration_list_name_lepton_key_declaration(
            the_statement_block->declaration_list, name_number);
  }

extern quark_declaration *statement_block_name_quark_declaration(
        statement_block *the_statement_block, size_t name_number)
  {
    assert(the_statement_block != NULL);

    return declaration_list_name_quark_declaration(
            the_statement_block->declaration_list, name_number);
  }

extern lock_declaration *statement_block_name_lock_declaration(
        statement_block *the_statement_block, size_t name_number)
  {
    assert(the_statement_block != NULL);

    return declaration_list_name_lock_declaration(
            the_statement_block->declaration_list, name_number);
  }

extern statement *statement_block_name_jump_target_declaration(
        statement_block *the_statement_block, size_t name_number)
  {
    assert(the_statement_block != NULL);

    return declaration_list_name_jump_target_declaration(
            the_statement_block->declaration_list, name_number);
  }

extern size_t statement_block_lookup_name(statement_block *the_statement_block,
                                          const char *name)
  {
    assert(the_statement_block != NULL);

    return declaration_list_lookup_name(the_statement_block->declaration_list,
                                        name);
  }

extern size_t statement_block_use_statement_count(
        statement_block *the_statement_block)
  {
    assert(the_statement_block != NULL);

    return the_statement_block->use_statements.element_count;
  }

extern statement *statement_block_use_statement(
        statement_block *the_statement_block, size_t use_statement_number)
  {
    assert(the_statement_block != NULL);
    assert(use_statement_number <
           the_statement_block->use_statements.element_count);

    assert(the_statement_block->use_statements.array != NULL);
    return the_statement_block->use_statements.array[use_statement_number];
  }

extern size_t statement_block_dangling_routine_count(
        statement_block *the_statement_block)
  {
    assert(the_statement_block != NULL);

    return the_statement_block->dangling_routines.element_count;
  }

extern routine_declaration *statement_block_dangling_routine_declaration(
        statement_block *the_statement_block, size_t number)
  {
    assert(the_statement_block != NULL);

    assert(number < the_statement_block->dangling_routines.element_count);
    return the_statement_block->dangling_routines.array[number];
  }

extern size_t statement_block_declaration_count(
        statement_block *the_statement_block)
  {
    assert(the_statement_block != NULL);

    return declaration_list_declaration_count(
            the_statement_block->declaration_list);
  }

extern declaration *statement_block_declaration_by_number(
        statement_block *the_statement_block, size_t number)
  {
    assert(the_statement_block != NULL);

    return declaration_list_declaration_by_number(
            the_statement_block->declaration_list, number);
  }

extern void set_statement_block_start_location(
        statement_block *the_statement_block, const source_location *location)
  {
    assert(the_statement_block != NULL);

    set_location_start(&(the_statement_block->location), location);
  }

extern void set_statement_block_end_location(
        statement_block *the_statement_block, const source_location *location)
  {
    assert(the_statement_block != NULL);

    set_location_end(&(the_statement_block->location), location);
  }

extern verdict append_statement_to_block(statement_block *the_statement_block,
                                         statement *the_statement)
  {
    verdict the_verdict;

    assert(the_statement_block != NULL);
    assert(the_statement != NULL);

    the_verdict = statement_aa_append(&(the_statement_block->statements),
                                      the_statement);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_statement(the_statement);
        return the_verdict;
      }

    if (get_statement_kind(the_statement) == SK_DECLARATION)
      {
        size_t declaration_count;
        size_t declaration_num;

        declaration_count =
                declaration_statement_declaration_count(the_statement);
        for (declaration_num = 0; declaration_num < declaration_count;
             ++declaration_num)
          {
            declaration *the_declaration;
            verdict the_verdict;

            the_declaration = declaration_statement_declaration(the_statement,
                    declaration_num);

            if (declaration_kind(the_declaration) == NK_ROUTINE)
              {
                routine_declaration *the_routine_declaration;
                const char *name_chars;
                size_t index;

                the_routine_declaration =
                        declaration_routine_declaration(the_declaration);

                name_chars = routine_declaration_name(the_routine_declaration);

                index = declaration_list_lookup_name(
                        the_statement_block->declaration_list, name_chars);

                if (index <
                    declaration_list_name_count(
                            the_statement_block->declaration_list))
                  {
                    routine_declaration *last_declaration;
                    routine_declaration_chain *new_chain;
                    routine_declaration_chain *old_chain;

                    assert(statement_block_name_kind(the_statement_block,
                                                     index) == NK_ROUTINE);

                    last_declaration =
                            statement_block_name_routine_declaration(
                                    the_statement_block, index);

                    new_chain = routine_declaration_declaration_chain(
                            the_routine_declaration);
                    old_chain = routine_declaration_declaration_chain(
                            last_declaration);

                    routine_declaration_chain_set_next(new_chain, old_chain);
                  }
                else
                  {
                    verdict the_verdict;

                    the_verdict = routine_declaration_aa_append(
                            &(the_statement_block->dangling_routines),
                            the_routine_declaration);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                        return the_verdict;
                  }
              }

            declaration_set_parent_pointer(the_declaration,
                                           the_statement_block);
            declaration_set_parent_index(the_declaration,
                    declaration_list_declaration_count(
                            the_statement_block->declaration_list));

            the_verdict = declaration_list_append_declaration(
                    the_statement_block->declaration_list, the_declaration,
                    TRUE);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;
          }

        return MISSION_ACCOMPLISHED;
      }
    else if (get_statement_kind(the_statement) == SK_LABEL)
      {
        return declaration_list_append_jump_target_declaration(
                the_statement_block->declaration_list, the_statement);
      }
    else if (get_statement_kind(the_statement) == SK_USE)
      {
        declaration *the_declaration;
        verdict the_verdict;

        the_declaration = variable_declaration_declaration(
                use_statement_container(the_statement));
        assert(the_declaration != NULL);

        declaration_set_parent_pointer(the_declaration, the_statement_block);
        declaration_set_parent_index(the_declaration,
                declaration_list_declaration_count(
                        the_statement_block->declaration_list));

        the_verdict = declaration_list_append_declaration(
                the_statement_block->declaration_list, the_declaration,
                use_statement_named(the_statement));
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        use_statement_set_parent(the_statement, the_statement_block,
                the_statement_block->use_statements.element_count);

        return statement_aa_append(&(the_statement_block->use_statements),
                                   the_statement);
      }

    return MISSION_ACCOMPLISHED;
  }

extern const source_location *get_statement_block_location(
        statement_block *the_statement_block)
  {
    assert(the_statement_block != NULL);

    return &(the_statement_block->location);
  }
