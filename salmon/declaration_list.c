/* file "declaration_list.c" */

/*
 *  This file contains the implementation of the declaration_list module.
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
#include "c_foundations/string_index.h"
#include "declaration_list.h"
#include "declaration.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "tagalong_declaration.h"
#include "lepton_key_declaration.h"
#include "quark_declaration.h"
#include "lock_declaration.h"
#include "statement.h"


typedef struct
  {
    name_kind kind;
    union
      {
        declaration *declaration;
        statement *jump_target;
      } u;
  } name_data;

AUTO_ARRAY(declaration_aa, declaration *);
AUTO_ARRAY(statement_aa, statement *);
AUTO_ARRAY(name_data_aa, name_data);


struct declaration_list
  {
    declaration_aa declarations;
    statement_aa jump_targets;
    name_data_aa name_data;
    string_index *name_index;
  };


AUTO_ARRAY_IMPLEMENTATION(name_data_aa, name_data, 0);


extern declaration_list *create_declaration_list(void)
  {
    declaration_list *result;
    verdict the_verdict;

    result = MALLOC_ONE_OBJECT(declaration_list);
    if (result == NULL)
        return NULL;

    the_verdict = declaration_aa_init(&(result->declarations), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    the_verdict = statement_aa_init(&(result->jump_targets), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->declarations.array);
        free(result);
        return NULL;
      }

    the_verdict = name_data_aa_init(&(result->name_data), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->jump_targets.array);
        free(result->declarations.array);
        free(result);
        return NULL;
      }

    result->name_index = create_string_index();
    if (result->name_index == NULL)
      {
        free(result->name_data.array);
        free(result->jump_targets.array);
        free(result->declarations.array);
        free(result);
        return NULL;
      }

    return result;
  }

extern void delete_declaration_list(declaration_list *the_declaration_list)
  {
    assert(the_declaration_list != NULL);

    free(the_declaration_list->declarations.array);

    free(the_declaration_list->jump_targets.array);

    free(the_declaration_list->name_data.array);

    destroy_string_index(the_declaration_list->name_index);

    free(the_declaration_list);
  }

extern size_t declaration_list_name_count(
        declaration_list *the_declaration_list)
  {
    assert(the_declaration_list != NULL);

    return the_declaration_list->name_data.element_count;
  }

extern name_kind declaration_list_name_kind(
        declaration_list *the_declaration_list, size_t name_number)
  {
    assert(the_declaration_list != NULL);

    assert(name_number < the_declaration_list->name_data.element_count);
    return the_declaration_list->name_data.array[name_number].kind;
  }

extern declaration *declaration_list_name_declaration(
        declaration_list *the_declaration_list, size_t name_number)
  {
    assert(the_declaration_list != NULL);

    assert(name_number < the_declaration_list->name_data.element_count);
    assert(the_declaration_list->name_data.array[name_number].kind !=
           NK_JUMP_TARGET);
    return the_declaration_list->name_data.array[name_number].u.declaration;
  }

extern variable_declaration *declaration_list_name_variable_declaration(
        declaration_list *the_declaration_list, size_t name_number)
  {
    assert(the_declaration_list != NULL);

    assert(name_number < the_declaration_list->name_data.element_count);
    assert(the_declaration_list->name_data.array[name_number].kind ==
           NK_VARIABLE);
    return declaration_variable_declaration(
            the_declaration_list->name_data.array[name_number].u.declaration);
  }

extern routine_declaration *declaration_list_name_routine_declaration(
        declaration_list *the_declaration_list, size_t name_number)
  {
    assert(the_declaration_list != NULL);

    assert(name_number < the_declaration_list->name_data.element_count);
    assert(the_declaration_list->name_data.array[name_number].kind ==
           NK_ROUTINE);
    return declaration_routine_declaration(
            the_declaration_list->name_data.array[name_number].u.declaration);
  }

extern tagalong_declaration *declaration_list_name_tagalong_declaration(
        declaration_list *the_declaration_list, size_t name_number)
  {
    assert(the_declaration_list != NULL);

    assert(name_number < the_declaration_list->name_data.element_count);
    assert(the_declaration_list->name_data.array[name_number].kind ==
           NK_TAGALONG);
    return declaration_tagalong_declaration(
            the_declaration_list->name_data.array[name_number].u.declaration);
  }

extern lepton_key_declaration *declaration_list_name_lepton_key_declaration(
        declaration_list *the_declaration_list, size_t name_number)
  {
    assert(the_declaration_list != NULL);

    assert(name_number < the_declaration_list->name_data.element_count);
    assert(the_declaration_list->name_data.array[name_number].kind ==
           NK_LEPTON_KEY);
    return declaration_lepton_key_declaration(
            the_declaration_list->name_data.array[name_number].u.declaration);
  }

extern quark_declaration *declaration_list_name_quark_declaration(
        declaration_list *the_declaration_list, size_t name_number)
  {
    assert(the_declaration_list != NULL);

    assert(name_number < the_declaration_list->name_data.element_count);
    assert(the_declaration_list->name_data.array[name_number].kind ==
           NK_QUARK);
    return declaration_quark_declaration(
            the_declaration_list->name_data.array[name_number].u.declaration);
  }

extern lock_declaration *declaration_list_name_lock_declaration(
        declaration_list *the_declaration_list, size_t name_number)
  {
    assert(the_declaration_list != NULL);

    assert(name_number < the_declaration_list->name_data.element_count);
    assert(the_declaration_list->name_data.array[name_number].kind == NK_LOCK);
    return declaration_lock_declaration(
            the_declaration_list->name_data.array[name_number].u.declaration);
  }

extern statement *declaration_list_name_jump_target_declaration(
        declaration_list *the_declaration_list, size_t name_number)
  {
    assert(the_declaration_list != NULL);

    assert(name_number < the_declaration_list->name_data.element_count);
    assert(the_declaration_list->name_data.array[name_number].kind ==
           NK_JUMP_TARGET);
    return the_declaration_list->name_data.array[name_number].u.jump_target;
  }

extern size_t declaration_list_lookup_name(
        declaration_list *the_declaration_list, const char *name)
  {
    assert(the_declaration_list != NULL);
    assert(name != NULL);

    if (exists_in_string_index(the_declaration_list->name_index, name))
      {
        return (size_t)(lookup_in_string_index(
                the_declaration_list->name_index, name));
      }
    else
      {
        return the_declaration_list->name_data.element_count;
      }
  }

extern size_t declaration_list_declaration_count(
        declaration_list *the_declaration_list)
  {
    assert(the_declaration_list != NULL);

    return the_declaration_list->declarations.element_count;
  }

extern declaration *declaration_list_declaration_by_number(
        declaration_list *the_declaration_list, size_t number)
  {
    assert(the_declaration_list != NULL);

    assert(number < the_declaration_list->declarations.element_count);
    return the_declaration_list->declarations.array[number];
  }

extern verdict declaration_list_append_declaration(
        declaration_list *the_declaration_list, declaration *the_declaration,
        boolean include_in_name_lookup)
  {
    verdict the_verdict;
    const char *name_chars;
    name_data the_name_data;

    assert(the_declaration_list != NULL);
    assert(the_declaration != NULL);

    the_verdict = declaration_aa_append(&(the_declaration_list->declarations),
                                        the_declaration);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    switch (declaration_kind(the_declaration))
      {
        case NK_VARIABLE:
          {
            break;
          }
        case NK_ROUTINE:
          {
            const char *name_chars;

            name_chars = declaration_name(the_declaration);

            if (exists_in_string_index(the_declaration_list->name_index,
                                       name_chars))
              {
                size_t index;

                index = (size_t)(lookup_in_string_index(
                        the_declaration_list->name_index, name_chars));
                assert(declaration_list_name_kind(the_declaration_list, index)
                       == NK_ROUTINE);

                the_declaration_list->name_data.array[index].u.declaration =
                        the_declaration;

                return MISSION_ACCOMPLISHED;
              }

            break;
          }
        case NK_TAGALONG:
          {
            break;
          }
        case NK_LEPTON_KEY:
          {
            break;
          }
        case NK_QUARK:
          {
            break;
          }
        case NK_LOCK:
          {
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    if (!include_in_name_lookup)
        return MISSION_ACCOMPLISHED;

    name_chars = declaration_name(the_declaration);
    assert(name_chars != NULL);

    assert(!(exists_in_string_index(the_declaration_list->name_index,
                                    name_chars)));
    the_verdict = enter_into_string_index(the_declaration_list->name_index,
            name_chars,
            (void *)(the_declaration_list->name_data.element_count));
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_name_data.kind = declaration_kind(the_declaration);
    the_name_data.u.declaration = the_declaration;

    the_verdict = name_data_aa_append(&(the_declaration_list->name_data),
                                      the_name_data);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    return MISSION_ACCOMPLISHED;
  }

extern verdict declaration_list_append_jump_target_declaration(
        declaration_list *the_declaration_list,
        statement *the_jump_target_declaration)
  {
    verdict the_verdict;
    const char *name_chars;
    name_data the_name_data;

    assert(the_declaration_list != NULL);
    assert(the_jump_target_declaration != NULL);

    assert(get_statement_kind(the_jump_target_declaration) == SK_LABEL);

    the_verdict = statement_aa_append(&(the_declaration_list->jump_targets),
                                      the_jump_target_declaration);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    name_chars = label_statement_name(the_jump_target_declaration);

    assert(!(exists_in_string_index(the_declaration_list->name_index,
                                    name_chars)));
    the_verdict = enter_into_string_index(the_declaration_list->name_index,
            name_chars,
            (void *)(the_declaration_list->name_data.element_count));
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_name_data.kind = NK_JUMP_TARGET;
    the_name_data.u.jump_target = the_jump_target_declaration;

    the_verdict = name_data_aa_append(&(the_declaration_list->name_data),
                                      the_name_data);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    return MISSION_ACCOMPLISHED;
  }

extern const char *name_kind_name(name_kind kind)
  {
    switch (kind)
      {
        case NK_VARIABLE:
            return "variable";
        case NK_ROUTINE:
            return "routine";
        case NK_TAGALONG:
            return "tagalong key";
        case NK_LEPTON_KEY:
            return "lepton key";
        case NK_QUARK:
            return "quark";
        case NK_LOCK:
            return "lock";
        case NK_JUMP_TARGET:
            return "jump target";
        default:
            assert(FALSE);
            return NULL;
      }
  }
