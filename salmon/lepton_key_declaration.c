/* file "lepton_key_declaration.c" */

/*
 *  This file contains the implementation of the lepton_key_declaration module.
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
#include <string.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/auto_array.h"
#include "c_foundations/auto_array_implementation.h"
#include "c_foundations/string_index.h"
#include "lepton_key_declaration.h"
#include "expression.h"
#include "type_expression.h"
#include "declaration.h"


typedef struct
  {
    char *name;
    type_expression *type;
  } field_data;

AUTO_ARRAY(field_data_aa, field_data);

struct lepton_key_declaration
  {
    declaration *declaration;
    boolean additional_fields_allowed;
    field_data_aa fields;
    string_index *index;
  };


AUTO_ARRAY_IMPLEMENTATION(field_data_aa, field_data, 0);


extern lepton_key_declaration *create_lepton_key_declaration(
        boolean additional_fields_allowed)
  {
    lepton_key_declaration *result;
    verdict the_verdict;

    result = MALLOC_ONE_OBJECT(lepton_key_declaration);
    if (result == NULL)
        return NULL;

    result->declaration = NULL;
    result->additional_fields_allowed = additional_fields_allowed;

    the_verdict = field_data_aa_init(&(result->fields), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    result->index = create_string_index();
    if (result->index == NULL)
      {
        free(result->fields.array);
        free(result);
        return NULL;
      }

    return result;
  }

extern void lepton_key_declaration_add_reference(
        lepton_key_declaration *declaration)
  {
    assert(declaration != NULL);

    declaration_add_reference(declaration->declaration);
  }

extern void lepton_key_declaration_remove_reference(
        lepton_key_declaration *declaration)
  {
    assert(declaration != NULL);

    declaration_remove_reference(declaration->declaration);
  }

extern void delete_lepton_key_declaration(lepton_key_declaration *declaration)
  {
    size_t index;

    assert(declaration != NULL);

    for (index = 0; index < declaration->fields.element_count; ++index)
      {
        free(declaration->fields.array[index].name);
        delete_type_expression(declaration->fields.array[index].type);
      }
    free(declaration->fields.array);

    destroy_string_index(declaration->index);

    free(declaration);
  }

extern declaration *lepton_key_declaration_declaration(
        lepton_key_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration->declaration;
  }

extern const char *lepton_key_declaration_name(
        lepton_key_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_name(declaration->declaration);
  }

extern boolean lepton_key_declaration_additional_fields_allowed(
        lepton_key_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->additional_fields_allowed;
  }

extern boolean lepton_key_declaration_is_static(
        lepton_key_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_is_static(declaration->declaration);
  }

extern boolean lepton_key_declaration_is_virtual(
        lepton_key_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_is_virtual(declaration->declaration);
  }

extern boolean lepton_key_declaration_automatic_allocation(
        lepton_key_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_automatic_allocation(declaration->declaration);
  }

extern size_t lepton_key_field_count(lepton_key_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->fields.element_count;
  }

extern const char *lepton_key_field_name(lepton_key_declaration *declaration,
                                         size_t field_num)
  {
    assert(declaration != NULL);

    assert(field_num < declaration->fields.element_count);
    return declaration->fields.array[field_num].name;
  }

extern type_expression *lepton_key_field_type(
        lepton_key_declaration *declaration, size_t field_num)
  {
    assert(declaration != NULL);

    assert(field_num < declaration->fields.element_count);
    return declaration->fields.array[field_num].type;
  }

extern size_t lepton_key_lookup_field_by_name(
        lepton_key_declaration *declaration, const char *field_name)
  {
    assert(declaration != NULL);
    assert(field_name != NULL);

    if (!(exists_in_string_index(declaration->index, field_name)))
        return declaration->fields.element_count;

    return (size_t)(lookup_in_string_index(declaration->index, field_name));
  }

extern verdict lepton_key_add_field(lepton_key_declaration *declaration,
        const char *field_name, type_expression *field_type)
  {
    char *name_copy;
    field_data new_element;
    verdict the_verdict;

    assert(declaration != NULL);
    assert(field_name != NULL);
    assert(field_type != NULL);

    name_copy = MALLOC_ARRAY(char, strlen(field_name) + 1);
    if (name_copy == NULL)
      {
        delete_type_expression(field_type);
        return MISSION_FAILED;
      }

    strcpy(name_copy, field_name);

    new_element.name = name_copy;
    new_element.type = field_type;

    the_verdict = field_data_aa_append(&(declaration->fields), new_element);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(name_copy);
        delete_type_expression(field_type);
        return the_verdict;
      }

    return enter_into_string_index(declaration->index, field_name,
            (void *)(declaration->fields.element_count - 1));
  }

extern verdict lepton_key_declaration_set_additional_fields_allowed(
        lepton_key_declaration *declaration, boolean additional_fields_allowed)
  {
    assert(declaration != NULL);

    declaration->additional_fields_allowed = additional_fields_allowed;
    return MISSION_ACCOMPLISHED;
  }

extern void set_lepton_key_declaration_declaration(
        lepton_key_declaration *the_lepton_key_declaration,
        declaration *the_declaration)
  {
    assert(the_lepton_key_declaration != NULL);
    assert(the_declaration != NULL);

    assert(the_lepton_key_declaration->declaration == NULL);
    the_lepton_key_declaration->declaration = the_declaration;
  }
