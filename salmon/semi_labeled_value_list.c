/* file "semi_labeled_value_list.c" */

/*
 *  This file contains the implementation of the semi_labeled_value_list
 *  module.
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
#include "semi_labeled_value_list.h"
#include "value.h"


AUTO_ARRAY(mstring_aa, char *);
AUTO_ARRAY(value_aa, value *);

struct semi_labeled_value_list
  {
    mstring_aa labels;
    value_aa values;
  };


AUTO_ARRAY_IMPLEMENTATION(value_aa, value *, 0);


extern semi_labeled_value_list *create_semi_labeled_value_list(void)
  {
    semi_labeled_value_list *result;
    verdict the_verdict;

    result = MALLOC_ONE_OBJECT(semi_labeled_value_list);
    if (result == NULL)
        return NULL;

    the_verdict = mstring_aa_init(&(result->labels), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    the_verdict = value_aa_init(&(result->values), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->labels.array);
        free(result);
        return NULL;
      }

    return result;
  }

extern void delete_semi_labeled_value_list(
        semi_labeled_value_list *the_semi_labeled_value_list,
        jumper *the_jumper)
  {
    size_t count;
    size_t elem_num;
    char **labels;
    value **values;

    assert(the_semi_labeled_value_list != NULL);

    labels = the_semi_labeled_value_list->labels.array;
    assert(labels != NULL);
    values = the_semi_labeled_value_list->values.array;
    assert(values != NULL);

    count = the_semi_labeled_value_list->labels.element_count;
    for (elem_num = 0; elem_num < count; ++elem_num)
      {
        if (labels[elem_num] != NULL)
            free(labels[elem_num]);

        assert(values[elem_num] != NULL);
        value_remove_reference(values[elem_num], the_jumper);
      }

    free(labels);
    free(values);
    free(the_semi_labeled_value_list);
  }

extern size_t semi_labeled_value_list_value_count(
        semi_labeled_value_list *the_semi_labeled_value_list)
  {
    assert(the_semi_labeled_value_list != NULL);

    return the_semi_labeled_value_list->labels.element_count;
  }

extern const char *semi_labeled_value_list_label(
        semi_labeled_value_list *the_semi_labeled_value_list, size_t value_num)
  {
    assert(the_semi_labeled_value_list != NULL);

    assert(value_num < the_semi_labeled_value_list->labels.element_count);
    assert(value_num < the_semi_labeled_value_list->values.element_count);

    return the_semi_labeled_value_list->labels.array[value_num];
  }

extern value *semi_labeled_value_list_value(
        semi_labeled_value_list *the_semi_labeled_value_list, size_t value_num)
  {
    assert(the_semi_labeled_value_list != NULL);

    assert(value_num < the_semi_labeled_value_list->labels.element_count);
    assert(value_num < the_semi_labeled_value_list->values.element_count);

    return the_semi_labeled_value_list->values.array[value_num];
  }

extern verdict append_value_to_semi_labeled_value_list(
        semi_labeled_value_list *the_semi_labeled_value_list,
        const char *label, value *the_value)
  {
    char *copy;
    verdict the_verdict;

    assert(the_semi_labeled_value_list != NULL);
    assert(the_value != NULL);

    if (label == NULL)
      {
        copy = NULL;
      }
    else
      {
        copy = MALLOC_ARRAY(char, strlen(label) + 1);
        if (copy == NULL)
            return MISSION_FAILED;

        strcpy(copy, label);
      }

    the_verdict =
            mstring_aa_append(&(the_semi_labeled_value_list->labels), copy);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_verdict =
            value_aa_append(&(the_semi_labeled_value_list->values), the_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        --(the_semi_labeled_value_list->labels.element_count);
        if (copy != NULL)
            free(copy);
        return the_verdict;
      }

    value_add_reference(the_value);

    return MISSION_ACCOMPLISHED;
  }
