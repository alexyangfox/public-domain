/* file "basket.c" */

/*
 *  This file contains the implementation of the basket module.
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
#include "basket.h"
#include "expression.h"


AUTO_ARRAY(mstring_aa, char *);
AUTO_ARRAY(basket_aa, basket *);
AUTO_ARRAY(boolean_aa, boolean);


struct basket
  {
    basket_kind kind;
    union
      {
        expression *expression;
        struct
          {
            mstring_aa labels;
            basket_aa sub_baskets;
            boolean_aa forces;
          } list;
      } u;
  };


AUTO_ARRAY_IMPLEMENTATION(basket_aa, basket *, 0);


extern basket *create_expression_basket(expression *the_expression)
  {
    basket *result;

    assert(the_expression != NULL);

    result = MALLOC_ONE_OBJECT(basket);
    if (result == NULL)
        return NULL;

    result->kind = BK_EXPRESSION;
    result->u.expression = the_expression;

    return result;
  }

extern basket *create_list_basket(void)
  {
    basket *result;
    verdict the_verdict;

    result = MALLOC_ONE_OBJECT(basket);
    if (result == NULL)
        return NULL;

    result->kind = BK_LIST;

    the_verdict = mstring_aa_init(&(result->u.list.labels), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    the_verdict = basket_aa_init(&(result->u.list.sub_baskets), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->u.list.labels.array);
        return NULL;
      }

    the_verdict = boolean_aa_init(&(result->u.list.forces), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->u.list.sub_baskets.array);
        free(result->u.list.labels.array);
        return NULL;
      }

    return result;
  }

extern void delete_basket(basket *the_basket)
  {
    assert(the_basket != NULL);

    switch (the_basket->kind)
      {
        case BK_EXPRESSION:
          {
            delete_expression(the_basket->u.expression);
            break;
          }
        case BK_LIST:
          {
            size_t count;
            char **labels;
            basket **sub_baskets;
            boolean *forces;
            size_t sub_num;

            count = the_basket->u.list.labels.element_count;
            assert(count == the_basket->u.list.sub_baskets.element_count);
            assert(count == the_basket->u.list.forces.element_count);

            labels = the_basket->u.list.labels.array;
            sub_baskets = the_basket->u.list.sub_baskets.array;
            forces = the_basket->u.list.forces.array;

            for (sub_num = 0; sub_num < count; ++sub_num)
              {
                if (labels[sub_num] != NULL)
                    free(labels[sub_num]);
                if (sub_baskets[sub_num] != NULL)
                    delete_basket(sub_baskets[sub_num]);
              }

            free(labels);
            free(sub_baskets);
            free(forces);

            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    free(the_basket);
  }

extern basket_kind get_basket_kind(basket *the_basket)
  {
    assert(the_basket != NULL);

    return the_basket->kind;
  }

extern expression *expression_basket_expression(basket *the_basket)
  {
    assert(the_basket != NULL);

    assert(the_basket->kind == BK_EXPRESSION);
    return the_basket->u.expression;
  }

extern size_t list_basket_element_count(basket *the_basket)
  {
    size_t count;

    assert(the_basket != NULL);

    assert(the_basket->kind == BK_LIST);
    count = the_basket->u.list.labels.element_count;
    assert(count == the_basket->u.list.sub_baskets.element_count);
    assert(count == the_basket->u.list.forces.element_count);
    return count;
  }

extern const char *list_basket_label(basket *the_basket, size_t sub_basket_num)
  {
    assert(the_basket != NULL);

    assert(the_basket->kind == BK_LIST);
    assert(sub_basket_num < list_basket_element_count(the_basket));
    return the_basket->u.list.labels.array[sub_basket_num];
  }

extern basket *list_basket_sub_basket(basket *the_basket,
                                      size_t sub_basket_num)
  {
    assert(the_basket != NULL);

    assert(the_basket->kind == BK_LIST);
    assert(sub_basket_num < list_basket_element_count(the_basket));
    return the_basket->u.list.sub_baskets.array[sub_basket_num];
  }

extern boolean list_basket_force(basket *the_basket, size_t sub_basket_num)
  {
    assert(the_basket != NULL);

    assert(the_basket->kind == BK_LIST);
    assert(sub_basket_num < list_basket_element_count(the_basket));
    return the_basket->u.list.forces.array[sub_basket_num];
  }

extern verdict basket_add_sub_basket(basket *base, const char *label,
                                     basket *sub_basket, boolean force)
  {
    char *copy;
    verdict the_verdict;

    assert(base != NULL);

    assert(base->kind == BK_LIST);

    if (label == NULL)
      {
        copy = NULL;
      }
    else
      {
        copy = MALLOC_ARRAY(char, strlen(label) + 1);
        if (copy == NULL)
          {
            if (sub_basket != NULL)
                delete_basket(sub_basket);
            return MISSION_FAILED;
          }
        strcpy(copy, label);
      }

    the_verdict = mstring_aa_append(&(base->u.list.labels), copy);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (copy != NULL)
            free(copy);
        if (sub_basket != NULL)
            delete_basket(sub_basket);
        return the_verdict;
      }

    the_verdict = basket_aa_append(&(base->u.list.sub_baskets), sub_basket);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(base->u.list.labels.element_count > 0);
        --(base->u.list.labels.element_count);
        if (copy != NULL)
            free(copy);
        if (sub_basket != NULL)
            delete_basket(sub_basket);
        return the_verdict;
      }

    the_verdict = boolean_aa_append(&(base->u.list.forces), force);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(base->u.list.sub_baskets.element_count > 0);
        --(base->u.list.sub_baskets.element_count);
        assert(base->u.list.labels.element_count > 0);
        --(base->u.list.labels.element_count);
        if (copy != NULL)
            free(copy);
        if (sub_basket != NULL)
            delete_basket(sub_basket);
        return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }
