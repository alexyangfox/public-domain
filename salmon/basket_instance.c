/* file "basket_instance.c" */

/*
 *  This file contains the implementation of the basket_instance module.
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
#include "basket_instance.h"
#include "slot_location.h"


typedef struct
  {
    char *label;
    basket_instance *child;
    boolean force;
  } basket_instance_item;

struct basket_instance
  {
    basket_instance_kind kind;
    union
      {
        slot_location *slot;
        struct
          {
            size_t count;
            basket_instance_item *items;
          } list;
      } u;
  };


extern basket_instance *create_slot_basket_instance(
        slot_location *the_slot_location)
  {
    basket_instance *result;

    assert(the_slot_location != NULL);

    result = MALLOC_ONE_OBJECT(basket_instance);
    if (result == NULL)
        return NULL;

    slot_location_add_reference(the_slot_location);

    result->kind = BIK_SLOT;
    result->u.slot = the_slot_location;

    return result;
  }

extern basket_instance *create_list_basket_instance(size_t element_count)
  {
    basket_instance *result;

    result = MALLOC_ONE_OBJECT(basket_instance);
    if (result == NULL)
        return NULL;

    result->kind = BIK_LIST;
    result->u.list.count = element_count;
    if (element_count == 0)
      {
        result->u.list.items = NULL;
      }
    else
      {
        size_t number;

        result->u.list.items =
                MALLOC_ARRAY(basket_instance_item, element_count);
        if (result->u.list.items == NULL)
          {
            free(result);
            return NULL;
          }

        for (number = 0; number < element_count; ++number)
          {
            result->u.list.items[number].label = NULL;
            result->u.list.items[number].child = NULL;
            result->u.list.items[number].force = FALSE;
          }
      }

    return result;
  }

extern void delete_basket_instance(basket_instance *the_basket_instance,
                                   jumper *the_jumper)
  {
    assert(the_basket_instance != NULL);

    switch (the_basket_instance->kind)
      {
        case BIK_SLOT:
          {
            slot_location_remove_reference(the_basket_instance->u.slot,
                                           the_jumper);
            break;
          }
        case BIK_LIST:
          {
            size_t count;
            basket_instance_item *items;

            count = the_basket_instance->u.list.count;
            items = the_basket_instance->u.list.items;

            if (count == 0)
              {
                assert(items == NULL);
              }
            else
              {
                size_t number;

                assert(items != NULL);

                for (number = 0; number < count; ++number)
                  {
                    if (items[number].label != NULL)
                        free(items[number].label);
                    if (items[number].child != NULL)
                      {
                        delete_basket_instance(items[number].child,
                                               the_jumper);
                      }
                  }

                free(items);
              }

            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    free(the_basket_instance);
  }

extern basket_instance_kind get_basket_instance_kind(
        basket_instance *the_basket_instance)
  {
    assert(the_basket_instance != NULL);

    return the_basket_instance->kind;
  }

extern slot_location *slot_basket_instance_slot(
        basket_instance *the_basket_instance)
  {
    assert(the_basket_instance != NULL);

    assert(the_basket_instance->kind == BIK_SLOT);
    return the_basket_instance->u.slot;
  }

extern size_t list_basket_instance_element_count(
        basket_instance *the_basket_instance)
  {
    assert(the_basket_instance != NULL);

    assert(the_basket_instance->kind == BIK_LIST);
    return the_basket_instance->u.list.count;
  }

extern const char *list_basket_instance_label(
        basket_instance *the_basket_instance, size_t child_num)
  {
    assert(the_basket_instance != NULL);

    assert(the_basket_instance->kind == BIK_LIST);
    assert(child_num < the_basket_instance->u.list.count);
    assert(the_basket_instance->u.list.items != NULL);
    return the_basket_instance->u.list.items[child_num].label;
  }

extern basket_instance *list_basket_instance_child(
        basket_instance *the_basket_instance, size_t child_num)
  {
    assert(the_basket_instance != NULL);

    assert(the_basket_instance->kind == BIK_LIST);
    assert(child_num < the_basket_instance->u.list.count);
    assert(the_basket_instance->u.list.items != NULL);
    return the_basket_instance->u.list.items[child_num].child;
  }

extern boolean list_basket_instance_force(basket_instance *the_basket_instance,
                                          size_t child_num)
  {
    assert(the_basket_instance != NULL);

    assert(the_basket_instance->kind == BIK_LIST);
    assert(child_num < the_basket_instance->u.list.count);
    assert(the_basket_instance->u.list.items != NULL);
    return the_basket_instance->u.list.items[child_num].force;
  }

extern verdict set_list_basket_instance_label(
        basket_instance *the_basket_instance, size_t child_num,
        const char *label)
  {
    char *name_copy;

    assert(the_basket_instance != NULL);

    assert(the_basket_instance->kind == BIK_LIST);
    assert(child_num < the_basket_instance->u.list.count);
    assert(the_basket_instance->u.list.items != NULL);

    if (label == NULL)
      {
        name_copy = NULL;
      }
    else
      {
        name_copy = MALLOC_ARRAY(char, strlen(label) + 1);
        if (name_copy == NULL)
            return MISSION_FAILED;
        strcpy(name_copy, label);
      }

    the_basket_instance->u.list.items[child_num].label = name_copy;

    return MISSION_ACCOMPLISHED;
  }

extern verdict set_list_basket_instance_child(
        basket_instance *the_basket_instance, size_t child_num,
        basket_instance *child, boolean force)
  {
    assert(the_basket_instance != NULL);

    assert(the_basket_instance->kind == BIK_LIST);
    assert(child_num < the_basket_instance->u.list.count);
    assert(the_basket_instance->u.list.items != NULL);

    the_basket_instance->u.list.items[child_num].child = child;
    the_basket_instance->u.list.items[child_num].force = force;

    return MISSION_ACCOMPLISHED;
  }
