/* file "type_expression.c" */

/*
 *  This file contains the implementation of the type_expression module.
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
#include "type_expression.h"
#include "type.h"
#include "expression.h"
#include "regular_expression.h"
#include "source_location.h"


typedef struct
  {
    type_expression *argument_type;
    char *name;
    boolean has_default_value;
  } formal_argument;

typedef struct
  {
    type_expression *type;
    char *name;
  } field;

typedef struct
  {
    type_expression *type;
    char *name;
    boolean writing_allowed;
  } interface_item;

AUTO_ARRAY(expression_aa, expression *);
AUTO_ARRAY(formal_argument_aa, formal_argument);
AUTO_ARRAY(field_aa, field);
AUTO_ARRAY(interface_item_aa, interface_item);


struct type_expression
  {
    type_expression_kind kind;
    union
      {
        struct
          {
            type *the_type;
          } constant;
        struct
          {
            expression *name_expression;
          } name;
        struct
          {
            expression_aa cases;
          } enumeration;
        struct
          {
            type_expression *base;
          } not;
        struct
          {
            type_expression *left;
            type_expression *right;
          } intersection;
        struct
          {
            type_expression *left;
            type_expression *right;
          } union_type_expression;
        struct
          {
            type_expression *left;
            type_expression *right;
          } xor;
        struct
          {
            expression *expression;
          } expression;
        struct
          {
            type_expression *base;
            expression *lower_bound;
            expression *upper_bound;
          } array;
        struct
          {
            expression *lower_bound;
            expression *upper_bound;
            boolean lower_is_inclusive;
            boolean upper_is_inclusive;
          } range;
        struct
          {
            type_expression *base;
            boolean read_allowed;
            boolean write_allowed;
            boolean null_allowed;
          } pointer;
        struct
          {
            type_expression *base;
          } type;
        struct
          {
            type_expression *key;
            type_expression *target;
          } map;
        struct
          {
            type_expression *return_type;
            boolean extra_arguments_allowed;
            boolean extra_arguments_unspecified;
            formal_argument_aa formals;
          } routine;
        struct
          {
            boolean extra_fields_allowed;
            field_aa fields;
          } fields;
        struct
          {
            expression *lepton;
            boolean extra_fields_allowed;
            field_aa fields;
          } lepton;
        struct
          {
            boolean extra_fields_allowed;
            field_aa fields;
          } multiset;
        struct
          {
            boolean null_allowed;
            interface_item_aa items;
          } interface;
        struct
          {
            boolean extra_elements_allowed;
            field_aa fields;
          } semi_labeled_value_list;
        struct
          {
            regular_expression *regular_expression;
          } regular_expression;
      } u;
    source_location location;
  };


AUTO_ARRAY_IMPLEMENTATION(formal_argument_aa, formal_argument, 0);
AUTO_ARRAY_IMPLEMENTATION(field_aa, field, 0);
AUTO_ARRAY_IMPLEMENTATION(interface_item_aa, interface_item, 0);


static type_expression *create_empty_type_expression(void);


extern type_expression *create_constant_type_expression(type *the_type)
  {
    type_expression *result;

    assert(the_type != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
        return NULL;

    type_add_reference(the_type);

    result->kind = TEK_CONSTANT;
    result->u.constant.the_type = the_type;

    return result;
  }

extern type_expression *create_name_type_expression(
        expression *name_expression)
  {
    type_expression *result;

    assert(name_expression != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
      {
        delete_expression(name_expression);
        return NULL;
      }

    result->kind = TEK_NAME;
    result->u.name.name_expression = name_expression;

    return result;
  }

extern type_expression *create_enumeration_type_expression(void)
  {
    type_expression *result;
    verdict the_verdict;

    result = create_empty_type_expression();
    if (result == NULL)
        return NULL;

    result->kind = TEK_ENUMERATION;
    the_verdict = expression_aa_init(&(result->u.enumeration.cases), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    return result;
  }

extern type_expression *create_not_type_expression(type_expression *base)
  {
    type_expression *result;

    assert(base != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
      {
        delete_type_expression(base);
        return NULL;
      }

    result->kind = TEK_NOT;
    result->u.not.base = base;

    return result;
  }

extern type_expression *create_intersection_type_expression(
        type_expression *left, type_expression *right)
  {
    type_expression *result;

    assert(left != NULL);
    assert(right != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
      {
        delete_type_expression(left);
        delete_type_expression(right);
        return NULL;
      }

    result->kind = TEK_INTERSECTION;
    result->u.intersection.left = left;
    result->u.intersection.right = right;

    return result;
  }

extern type_expression *create_union_type_expression(type_expression *left,
                                                     type_expression *right)
  {
    type_expression *result;

    assert(left != NULL);
    assert(right != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
      {
        delete_type_expression(left);
        delete_type_expression(right);
        return NULL;
      }

    result->kind = TEK_UNION;
    result->u.union_type_expression.left = left;
    result->u.union_type_expression.right = right;

    return result;
  }

extern type_expression *create_xor_type_expression(type_expression *left,
                                                   type_expression *right)
  {
    type_expression *result;

    assert(left != NULL);
    assert(right != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
      {
        delete_type_expression(left);
        delete_type_expression(right);
        return NULL;
      }

    result->kind = TEK_XOR;
    result->u.xor.left = left;
    result->u.xor.right = right;

    return result;
  }

extern type_expression *create_expression_type_expression(
        expression *the_expression)
  {
    type_expression *result;

    assert(the_expression != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
      {
        delete_expression(the_expression);
        return NULL;
      }

    result->kind = TEK_EXPRESSION;
    result->u.expression.expression = the_expression;

    return result;
  }

extern type_expression *create_array_type_expression(type_expression *base,
        expression *lower_bound, expression *upper_bound)
  {
    type_expression *result;

    assert(base != NULL);
    assert(lower_bound != NULL);
    assert(upper_bound != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
      {
        delete_type_expression(base);
        delete_expression(lower_bound);
        delete_expression(upper_bound);
        return NULL;
      }

    result->kind = TEK_ARRAY;
    result->u.array.base = base;
    result->u.array.lower_bound = lower_bound;
    result->u.array.upper_bound = upper_bound;

    return result;
  }

extern type_expression *create_integer_range_type_expression(
        expression *lower_bound, expression *upper_bound,
        boolean lower_is_inclusive, boolean upper_is_inclusive)
  {
    type_expression *result;

    assert(lower_bound != NULL);
    assert(upper_bound != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
      {
        delete_expression(lower_bound);
        delete_expression(upper_bound);
        return NULL;
      }

    result->kind = TEK_INTEGER_RANGE;
    result->u.range.lower_bound = lower_bound;
    result->u.range.upper_bound = upper_bound;
    result->u.range.lower_is_inclusive = lower_is_inclusive;
    result->u.range.upper_is_inclusive = upper_is_inclusive;

    return result;
  }

extern type_expression *create_rational_range_type_expression(
        expression *lower_bound, expression *upper_bound,
        boolean lower_is_inclusive, boolean upper_is_inclusive)
  {
    type_expression *result;

    assert(lower_bound != NULL);
    assert(upper_bound != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
      {
        delete_expression(lower_bound);
        delete_expression(upper_bound);
        return NULL;
      }

    result->kind = TEK_RATIONAL_RANGE;
    result->u.range.lower_bound = lower_bound;
    result->u.range.upper_bound = upper_bound;
    result->u.range.lower_is_inclusive = lower_is_inclusive;
    result->u.range.upper_is_inclusive = upper_is_inclusive;

    return result;
  }

extern type_expression *create_pointer_type_expression(type_expression *base,
        boolean read_allowed, boolean write_allowed, boolean null_allowed)
  {
    type_expression *result;

    assert(base != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
      {
        delete_type_expression(base);
        return NULL;
      }

    result->kind = TEK_POINTER;
    result->u.pointer.base = base;
    result->u.pointer.read_allowed = read_allowed;
    result->u.pointer.write_allowed = write_allowed;
    result->u.pointer.null_allowed = null_allowed;

    return result;
  }

extern type_expression *create_type_type_expression(type_expression *base)
  {
    type_expression *result;

    assert(base != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
      {
        delete_type_expression(base);
        return NULL;
      }

    result->kind = TEK_TYPE;
    result->u.type.base = base;

    return result;
  }

extern type_expression *create_map_type_expression(type_expression *key,
                                                   type_expression *target)
  {
    type_expression *result;

    assert(key != NULL);
    assert(target != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
      {
        delete_type_expression(key);
        delete_type_expression(target);
        return NULL;
      }

    result->kind = TEK_MAP;
    result->u.map.key = key;
    result->u.map.target = target;

    return result;
  }

extern type_expression *create_routine_type_expression(
        type_expression *return_type, boolean extra_arguments_allowed)
  {
    type_expression *result;
    verdict the_verdict;

    assert(return_type != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
      {
        delete_type_expression(return_type);
        return NULL;
      }

    result->kind = TEK_ROUTINE;
    result->u.routine.return_type = return_type;
    result->u.routine.extra_arguments_allowed = extra_arguments_allowed;
    result->u.routine.extra_arguments_unspecified = FALSE;

    the_verdict = formal_argument_aa_init(&(result->u.routine.formals), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        delete_type_expression(return_type);
        return NULL;
      }

    return result;
  }

extern type_expression *create_fields_type_expression(
        boolean extra_fields_allowed)
  {
    type_expression *result;
    verdict the_verdict;

    result = create_empty_type_expression();
    if (result == NULL)
        return NULL;

    result->kind = TEK_FIELDS;
    result->u.fields.extra_fields_allowed = extra_fields_allowed;

    the_verdict = field_aa_init(&(result->u.fields.fields), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    return result;
  }

extern type_expression *create_lepton_type_expression(expression *lepton,
        boolean extra_fields_allowed)
  {
    type_expression *result;
    verdict the_verdict;

    assert(lepton != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
      {
        delete_expression(lepton);
        return NULL;
      }

    result->kind = TEK_LEPTON;
    result->u.lepton.lepton = lepton;
    result->u.lepton.extra_fields_allowed = extra_fields_allowed;

    the_verdict = field_aa_init(&(result->u.lepton.fields), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        delete_expression(lepton);
        return NULL;
      }

    return result;
  }

extern type_expression *create_multiset_type_expression(
        boolean extra_fields_allowed)
  {
    type_expression *result;
    verdict the_verdict;

    result = create_empty_type_expression();
    if (result == NULL)
        return NULL;

    result->kind = TEK_MULTISET;
    result->u.multiset.extra_fields_allowed = extra_fields_allowed;

    the_verdict = field_aa_init(&(result->u.multiset.fields), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    return result;
  }

extern type_expression *create_interface_type_expression(boolean null_allowed)
  {
    type_expression *result;
    verdict the_verdict;

    result = create_empty_type_expression();
    if (result == NULL)
        return NULL;

    result->kind = TEK_INTERFACE;
    result->u.interface.null_allowed = null_allowed;

    the_verdict = interface_item_aa_init(&(result->u.interface.items), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    return result;
  }

extern type_expression *create_semi_labeled_value_list_type_expression(
        boolean extra_elements_allowed)
  {
    type_expression *result;
    verdict the_verdict;

    result = create_empty_type_expression();
    if (result == NULL)
        return NULL;

    result->kind = TEK_SEMI_LABELED_VALUE_LIST;
    result->u.semi_labeled_value_list.extra_elements_allowed =
            extra_elements_allowed;

    the_verdict =
            field_aa_init(&(result->u.semi_labeled_value_list.fields), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    return result;
  }

extern type_expression *create_regular_expression_type_expression(
        regular_expression *the_regular_expression)
  {
    type_expression *result;

    assert(the_regular_expression != NULL);

    result = create_empty_type_expression();
    if (result == NULL)
        return NULL;

    regular_expression_add_reference(the_regular_expression);

    result->kind = TEK_REGULAR_EXPRESSION;
    result->u.regular_expression.regular_expression = the_regular_expression;

    return result;
  }

extern void delete_type_expression(type_expression *the_type_expression)
  {
    assert(the_type_expression != NULL);

    switch (the_type_expression->kind)
      {
        case TEK_CONSTANT:
          {
            type_remove_reference(the_type_expression->u.constant.the_type,
                                  NULL);
            break;
          }
        case TEK_NAME:
          {
            delete_expression(the_type_expression->u.name.name_expression);
            break;
          }
        case TEK_ENUMERATION:
          {
            expression **array;
            size_t count;
            size_t number;

            array = the_type_expression->u.enumeration.cases.array;
            assert(array != NULL);

            count = the_type_expression->u.enumeration.cases.element_count;
            for (number = 0; number < count; ++number)
                delete_expression(array[number]);

            free(array);

            break;
          }
        case TEK_NOT:
          {
            delete_type_expression(the_type_expression->u.not.base);
            break;
          }
        case TEK_INTERSECTION:
          {
            delete_type_expression(the_type_expression->u.intersection.left);
            delete_type_expression(the_type_expression->u.intersection.right);
            break;
          }
        case TEK_UNION:
          {
            delete_type_expression(
                    the_type_expression->u.union_type_expression.left);
            delete_type_expression(
                    the_type_expression->u.union_type_expression.right);
            break;
          }
        case TEK_XOR:
          {
            delete_type_expression(the_type_expression->u.xor.left);
            delete_type_expression(the_type_expression->u.xor.right);
            break;
          }
        case TEK_EXPRESSION:
          {
            delete_expression(the_type_expression->u.expression.expression);
            break;
          }
        case TEK_ARRAY:
          {
            delete_type_expression(the_type_expression->u.array.base);
            delete_expression(the_type_expression->u.array.lower_bound);
            delete_expression(the_type_expression->u.array.upper_bound);
            break;
          }
        case TEK_INTEGER_RANGE:
          {
            delete_expression(the_type_expression->u.range.lower_bound);
            delete_expression(the_type_expression->u.range.upper_bound);
            break;
          }
        case TEK_RATIONAL_RANGE:
          {
            delete_expression(the_type_expression->u.range.lower_bound);
            delete_expression(the_type_expression->u.range.upper_bound);
            break;
          }
        case TEK_POINTER:
          {
            delete_type_expression(the_type_expression->u.pointer.base);
            break;
          }
        case TEK_TYPE:
          {
            delete_type_expression(the_type_expression->u.type.base);
            break;
          }
        case TEK_MAP:
          {
            delete_type_expression(the_type_expression->u.map.key);
            delete_type_expression(the_type_expression->u.map.target);
            break;
          }
        case TEK_ROUTINE:
          {
            formal_argument *array;
            size_t count;
            size_t number;

            delete_type_expression(the_type_expression->u.routine.return_type);

            array = the_type_expression->u.routine.formals.array;
            assert(array != NULL);

            count = the_type_expression->u.routine.formals.element_count;
            for (number = 0; number < count; ++number)
              {
                delete_type_expression(array[number].argument_type);
                if (array[number].name != NULL)
                    free(array[number].name);
              }

            free(array);

            break;
          }
        case TEK_FIELDS:
          {
            field *array;
            size_t count;
            size_t number;

            array = the_type_expression->u.fields.fields.array;
            assert(array != NULL);

            count = the_type_expression->u.fields.fields.element_count;
            for (number = 0; number < count; ++number)
              {
                delete_type_expression(array[number].type);
                if (array[number].name != NULL)
                    free(array[number].name);
              }

            free(array);

            break;
          }
        case TEK_LEPTON:
          {
            field *array;
            size_t count;
            size_t number;

            delete_expression(the_type_expression->u.lepton.lepton);

            array = the_type_expression->u.lepton.fields.array;
            assert(array != NULL);

            count = the_type_expression->u.lepton.fields.element_count;
            for (number = 0; number < count; ++number)
              {
                delete_type_expression(array[number].type);
                if (array[number].name != NULL)
                    free(array[number].name);
              }

            free(array);

            break;
          }
        case TEK_MULTISET:
          {
            field *array;
            size_t count;
            size_t number;

            array = the_type_expression->u.multiset.fields.array;
            assert(array != NULL);

            count = the_type_expression->u.multiset.fields.element_count;
            for (number = 0; number < count; ++number)
              {
                delete_type_expression(array[number].type);
                if (array[number].name != NULL)
                    free(array[number].name);
              }

            free(array);

            break;
          }
        case TEK_INTERFACE:
          {
            interface_item *array;
            size_t count;
            size_t number;

            array = the_type_expression->u.interface.items.array;
            assert(array != NULL);

            count = the_type_expression->u.interface.items.element_count;
            for (number = 0; number < count; ++number)
              {
                delete_type_expression(array[number].type);
                if (array[number].name != NULL)
                    free(array[number].name);
              }

            free(array);

            break;
          }
        case TEK_SEMI_LABELED_VALUE_LIST:
          {
            field *array;
            size_t count;
            size_t number;

            array = the_type_expression->u.semi_labeled_value_list.fields.
                    array;
            assert(array != NULL);

            count = the_type_expression->u.semi_labeled_value_list.fields.
                    element_count;
            for (number = 0; number < count; ++number)
              {
                delete_type_expression(array[number].type);
                if (array[number].name != NULL)
                    free(array[number].name);
              }

            free(array);

            break;
          }
        case TEK_REGULAR_EXPRESSION:
          {
            regular_expression_remove_reference(
                    the_type_expression->u.regular_expression.
                            regular_expression);
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    source_location_remove_reference(&(the_type_expression->location));

    free(the_type_expression);
  }

extern type_expression_kind get_type_expression_kind(
        type_expression *the_type_expression)
  {
    assert(the_type_expression != NULL);

    return the_type_expression->kind;
  }

extern type *constant_type_expression_type(
        type_expression *constant_type_expression)
  {
    assert(constant_type_expression != NULL);
    assert(constant_type_expression->kind == TEK_CONSTANT);

    return constant_type_expression->u.constant.the_type;
  }

extern expression *name_type_expression_name_expression(
        type_expression *name_type_expression)
  {
    assert(name_type_expression != NULL);
    assert(name_type_expression->kind == TEK_NAME);

    return name_type_expression->u.name.name_expression;
  }

extern size_t enumeration_type_expression_case_count(
        type_expression *enumeration_type_expression)
  {
    assert(enumeration_type_expression != NULL);
    assert(enumeration_type_expression->kind == TEK_ENUMERATION);

    return enumeration_type_expression->u.enumeration.cases.element_count;
  }

extern expression *enumeration_type_expression_case(
        type_expression *enumeration_type_expression, size_t case_num)
  {
    assert(enumeration_type_expression != NULL);
    assert(enumeration_type_expression->kind == TEK_ENUMERATION);

    assert(case_num <
           enumeration_type_expression_case_count(
                   enumeration_type_expression));
    return enumeration_type_expression->u.enumeration.cases.array[case_num];
  }

extern type_expression *not_type_expression_base(
        type_expression *not_type_expression)
  {
    assert(not_type_expression != NULL);
    assert(not_type_expression->kind == TEK_NOT);

    return not_type_expression->u.not.base;
  }

extern type_expression *intersection_type_expression_left(
        type_expression *intersection_type_expression)
  {
    assert(intersection_type_expression != NULL);
    assert(intersection_type_expression->kind == TEK_INTERSECTION);

    return intersection_type_expression->u.intersection.left;
  }

extern type_expression *intersection_type_expression_right(
        type_expression *intersection_type_expression)
  {
    assert(intersection_type_expression != NULL);
    assert(intersection_type_expression->kind == TEK_INTERSECTION);

    return intersection_type_expression->u.intersection.right;
  }

extern type_expression *union_type_expression_left(
        type_expression *union_type_expression)
  {
    assert(union_type_expression != NULL);
    assert(union_type_expression->kind == TEK_UNION);

    return union_type_expression->u.union_type_expression.left;
  }

extern type_expression *union_type_expression_right(
        type_expression *union_type_expression)
  {
    assert(union_type_expression != NULL);
    assert(union_type_expression->kind == TEK_UNION);

    return union_type_expression->u.union_type_expression.right;
  }

extern type_expression *xor_type_expression_left(
        type_expression *xor_type_expression)
  {
    assert(xor_type_expression != NULL);
    assert(xor_type_expression->kind == TEK_XOR);

    return xor_type_expression->u.xor.left;
  }

extern type_expression *xor_type_expression_right(
        type_expression *xor_type_expression)
  {
    assert(xor_type_expression != NULL);
    assert(xor_type_expression->kind == TEK_XOR);

    return xor_type_expression->u.xor.right;
  }

extern expression *expression_type_expression_expression(
        type_expression *expression_type_expression)
  {
    assert(expression_type_expression != NULL);
    assert(expression_type_expression->kind == TEK_EXPRESSION);

    return expression_type_expression->u.expression.expression;
  }

extern type_expression *array_type_expression_base(
        type_expression *array_type_expression)
  {
    assert(array_type_expression != NULL);
    assert(array_type_expression->kind == TEK_ARRAY);

    return array_type_expression->u.array.base;
  }

extern expression *array_type_expression_lower_bound(
        type_expression *array_type_expression)
  {
    assert(array_type_expression != NULL);
    assert(array_type_expression->kind == TEK_ARRAY);

    return array_type_expression->u.array.lower_bound;
  }

extern expression *array_type_expression_upper_bound(
        type_expression *array_type_expression)
  {
    assert(array_type_expression != NULL);
    assert(array_type_expression->kind == TEK_ARRAY);

    return array_type_expression->u.array.upper_bound;
  }

extern expression *integer_range_type_expression_lower_bound(
        type_expression *integer_range_type_expression)
  {
    assert(integer_range_type_expression != NULL);
    assert(integer_range_type_expression->kind == TEK_INTEGER_RANGE);

    return integer_range_type_expression->u.range.lower_bound;
  }

extern expression *integer_range_type_expression_upper_bound(
        type_expression *integer_range_type_expression)
  {
    assert(integer_range_type_expression != NULL);
    assert(integer_range_type_expression->kind == TEK_INTEGER_RANGE);

    return integer_range_type_expression->u.range.upper_bound;
  }

extern boolean integer_range_type_expression_lower_is_inclusive(
        type_expression *integer_range_type_expression)
  {
    assert(integer_range_type_expression != NULL);
    assert(integer_range_type_expression->kind == TEK_INTEGER_RANGE);

    return integer_range_type_expression->u.range.lower_is_inclusive;
  }

extern boolean integer_range_type_expression_upper_is_inclusive(
        type_expression *integer_range_type_expression)
  {
    assert(integer_range_type_expression != NULL);
    assert(integer_range_type_expression->kind == TEK_INTEGER_RANGE);

    return integer_range_type_expression->u.range.upper_is_inclusive;
  }

extern expression *rational_range_type_expression_lower_bound(
        type_expression *rational_range_type_expression)
  {
    assert(rational_range_type_expression != NULL);
    assert(rational_range_type_expression->kind == TEK_RATIONAL_RANGE);

    return rational_range_type_expression->u.range.lower_bound;
  }

extern expression *rational_range_type_expression_upper_bound(
        type_expression *rational_range_type_expression)
  {
    assert(rational_range_type_expression != NULL);
    assert(rational_range_type_expression->kind == TEK_RATIONAL_RANGE);

    return rational_range_type_expression->u.range.upper_bound;
  }

extern boolean rational_range_type_expression_lower_is_inclusive(
        type_expression *rational_range_type_expression)
  {
    assert(rational_range_type_expression != NULL);
    assert(rational_range_type_expression->kind == TEK_RATIONAL_RANGE);

    return rational_range_type_expression->u.range.lower_is_inclusive;
  }

extern boolean rational_range_type_expression_upper_is_inclusive(
        type_expression *rational_range_type_expression)
  {
    assert(rational_range_type_expression != NULL);
    assert(rational_range_type_expression->kind == TEK_RATIONAL_RANGE);

    return rational_range_type_expression->u.range.upper_is_inclusive;
  }

extern type_expression *pointer_type_expression_base(
        type_expression *pointer_type_expression)
  {
    assert(pointer_type_expression != NULL);
    assert(pointer_type_expression->kind == TEK_POINTER);

    return pointer_type_expression->u.pointer.base;
  }

extern boolean pointer_type_expression_read_allowed(
        type_expression *pointer_type_expression)
  {
    assert(pointer_type_expression != NULL);
    assert(pointer_type_expression->kind == TEK_POINTER);

    return pointer_type_expression->u.pointer.read_allowed;
  }

extern boolean pointer_type_expression_write_allowed(
        type_expression *pointer_type_expression)
  {
    assert(pointer_type_expression != NULL);
    assert(pointer_type_expression->kind == TEK_POINTER);

    return pointer_type_expression->u.pointer.write_allowed;
  }

extern boolean pointer_type_expression_null_allowed(
        type_expression *pointer_type_expression)
  {
    assert(pointer_type_expression != NULL);
    assert(pointer_type_expression->kind == TEK_POINTER);

    return pointer_type_expression->u.pointer.null_allowed;
  }

extern type_expression *type_type_expression_base(
        type_expression *type_type_expression)
  {
    assert(type_type_expression != NULL);
    assert(type_type_expression->kind == TEK_TYPE);

    return type_type_expression->u.type.base;
  }

extern type_expression *map_type_expression_key(
        type_expression *map_type_expression)
  {
    assert(map_type_expression != NULL);
    assert(map_type_expression->kind == TEK_MAP);

    return map_type_expression->u.map.key;
  }

extern type_expression *map_type_expression_target(
        type_expression *map_type_expression)
  {
    assert(map_type_expression != NULL);
    assert(map_type_expression->kind == TEK_MAP);

    return map_type_expression->u.map.target;
  }

extern type_expression *routine_type_expression_return_type(
        type_expression *routine_type_expression)
  {
    assert(routine_type_expression != NULL);
    assert(routine_type_expression->kind == TEK_ROUTINE);

    return routine_type_expression->u.routine.return_type;
  }

extern boolean routine_type_expression_extra_arguments_allowed(
        type_expression *routine_type_expression)
  {
    assert(routine_type_expression != NULL);
    assert(routine_type_expression->kind == TEK_ROUTINE);

    return routine_type_expression->u.routine.extra_arguments_allowed;
  }

extern boolean routine_type_expression_extra_arguments_unspecified(
        type_expression *routine_type_expression)
  {
    assert(routine_type_expression != NULL);
    assert(routine_type_expression->kind == TEK_ROUTINE);

    return routine_type_expression->u.routine.extra_arguments_unspecified;
  }

extern size_t routine_type_expression_formal_count(
        type_expression *routine_type_expression)
  {
    assert(routine_type_expression != NULL);
    assert(routine_type_expression->kind == TEK_ROUTINE);

    return routine_type_expression->u.routine.formals.element_count;
  }

extern type_expression *routine_type_expression_formal_argument_type(
        type_expression *routine_type_expression, size_t formal_num)
  {
    assert(routine_type_expression != NULL);
    assert(routine_type_expression->kind == TEK_ROUTINE);

    assert(formal_num <
           routine_type_expression_formal_count(routine_type_expression));
    return routine_type_expression->u.routine.formals.array[formal_num].
            argument_type;
  }

extern const char *routine_type_expression_formal_name(
        type_expression *routine_type_expression, size_t formal_num)
  {
    assert(routine_type_expression != NULL);
    assert(routine_type_expression->kind == TEK_ROUTINE);

    assert(formal_num <
           routine_type_expression_formal_count(routine_type_expression));
    return routine_type_expression->u.routine.formals.array[formal_num].name;
  }

extern boolean routine_type_expression_formal_has_default_value(
        type_expression *routine_type_expression, size_t formal_num)
  {
    assert(routine_type_expression != NULL);
    assert(routine_type_expression->kind == TEK_ROUTINE);

    assert(formal_num <
           routine_type_expression_formal_count(routine_type_expression));
    return routine_type_expression->u.routine.formals.array[formal_num].
            has_default_value;
  }

extern boolean fields_type_expression_extra_fields_allowed(
        type_expression *fields_type_expression)
  {
    assert(fields_type_expression != NULL);
    assert(fields_type_expression->kind == TEK_FIELDS);

    return fields_type_expression->u.fields.extra_fields_allowed;
  }

extern size_t fields_type_expression_field_count(
        type_expression *fields_type_expression)
  {
    assert(fields_type_expression != NULL);
    assert(fields_type_expression->kind == TEK_FIELDS);

    return fields_type_expression->u.fields.fields.element_count;
  }

extern type_expression *fields_type_expression_field_type(
        type_expression *fields_type_expression, size_t field_num)
  {
    assert(fields_type_expression != NULL);
    assert(fields_type_expression->kind == TEK_FIELDS);

    assert(field_num <
           fields_type_expression_field_count(fields_type_expression));
    return fields_type_expression->u.fields.fields.array[field_num].type;
  }

extern const char *fields_type_expression_field_name(
        type_expression *fields_type_expression, size_t field_num)
  {
    assert(fields_type_expression != NULL);
    assert(fields_type_expression->kind == TEK_FIELDS);

    assert(field_num <
           fields_type_expression_field_count(fields_type_expression));
    return fields_type_expression->u.fields.fields.array[field_num].name;
  }

extern expression *lepton_type_expression_lepton(
        type_expression *lepton_type_expression)
  {
    assert(lepton_type_expression != NULL);
    assert(lepton_type_expression->kind == TEK_LEPTON);

    return lepton_type_expression->u.lepton.lepton;
  }

extern boolean lepton_type_expression_extra_fields_allowed(
        type_expression *lepton_type_expression)
  {
    assert(lepton_type_expression != NULL);
    assert(lepton_type_expression->kind == TEK_LEPTON);

    return lepton_type_expression->u.lepton.extra_fields_allowed;
  }

extern size_t lepton_type_expression_field_count(
        type_expression *lepton_type_expression)
  {
    assert(lepton_type_expression != NULL);
    assert(lepton_type_expression->kind == TEK_LEPTON);

    return lepton_type_expression->u.lepton.fields.element_count;
  }

extern type_expression *lepton_type_expression_field_type(
        type_expression *lepton_type_expression, size_t field_num)
  {
    assert(lepton_type_expression != NULL);
    assert(lepton_type_expression->kind == TEK_LEPTON);

    assert(field_num <
           lepton_type_expression_field_count(lepton_type_expression));
    return lepton_type_expression->u.lepton.fields.array[field_num].type;
  }

extern const char *lepton_type_expression_field_name(
        type_expression *lepton_type_expression, size_t field_num)
  {
    assert(lepton_type_expression != NULL);
    assert(lepton_type_expression->kind == TEK_LEPTON);

    assert(field_num <
           lepton_type_expression_field_count(lepton_type_expression));
    return lepton_type_expression->u.lepton.fields.array[field_num].name;
  }

extern boolean multiset_type_expression_extra_fields_allowed(
        type_expression *multiset_type_expression)
  {
    assert(multiset_type_expression != NULL);
    assert(multiset_type_expression->kind == TEK_MULTISET);

    return multiset_type_expression->u.multiset.extra_fields_allowed;
  }

extern size_t multiset_type_expression_field_count(
        type_expression *multiset_type_expression)
  {
    assert(multiset_type_expression != NULL);
    assert(multiset_type_expression->kind == TEK_MULTISET);

    return multiset_type_expression->u.multiset.fields.element_count;
  }

extern type_expression *multiset_type_expression_field_type(
        type_expression *multiset_type_expression, size_t field_num)
  {
    assert(multiset_type_expression != NULL);
    assert(multiset_type_expression->kind == TEK_MULTISET);

    assert(field_num <
           multiset_type_expression_field_count(multiset_type_expression));
    return multiset_type_expression->u.multiset.fields.array[field_num].type;
  }

extern const char *multiset_type_expression_field_name(
        type_expression *multiset_type_expression, size_t field_num)
  {
    assert(multiset_type_expression != NULL);
    assert(multiset_type_expression->kind == TEK_MULTISET);

    assert(field_num <
           multiset_type_expression_field_count(multiset_type_expression));
    return multiset_type_expression->u.multiset.fields.array[field_num].name;
  }

extern boolean interface_type_expression_null_allowed(
        type_expression *interface_type_expression)
  {
    assert(interface_type_expression != NULL);
    assert(interface_type_expression->kind == TEK_INTERFACE);

    return interface_type_expression->u.interface.null_allowed;
  }

extern size_t interface_type_expression_item_count(
        type_expression *interface_type_expression)
  {
    assert(interface_type_expression != NULL);
    assert(interface_type_expression->kind == TEK_INTERFACE);

    return interface_type_expression->u.interface.items.element_count;
  }

extern type_expression *interface_type_expression_item_type(
        type_expression *interface_type_expression, size_t item_num)
  {
    assert(interface_type_expression != NULL);
    assert(interface_type_expression->kind == TEK_INTERFACE);

    assert(item_num <
           interface_type_expression_item_count(interface_type_expression));
    return interface_type_expression->u.interface.items.array[item_num].type;
  }

extern const char *interface_type_expression_item_name(
        type_expression *interface_type_expression, size_t item_num)
  {
    assert(interface_type_expression != NULL);
    assert(interface_type_expression->kind == TEK_INTERFACE);

    assert(item_num <
           interface_type_expression_item_count(interface_type_expression));
    return interface_type_expression->u.interface.items.array[item_num].name;
  }

extern boolean interface_type_expression_item_writing_allowed(
        type_expression *interface_type_expression, size_t item_num)
  {
    assert(interface_type_expression != NULL);
    assert(interface_type_expression->kind == TEK_INTERFACE);

    assert(item_num <
           interface_type_expression_item_count(interface_type_expression));
    return interface_type_expression->u.interface.items.array[item_num].
            writing_allowed;
  }

extern boolean semi_labeled_value_list_type_expression_extra_elements_allowed(
        type_expression *semi_labeled_value_list_type_expression)
  {
    assert(semi_labeled_value_list_type_expression != NULL);
    assert(semi_labeled_value_list_type_expression->kind ==
           TEK_SEMI_LABELED_VALUE_LIST);

    return semi_labeled_value_list_type_expression->u.semi_labeled_value_list.
            extra_elements_allowed;
  }

extern size_t semi_labeled_value_list_type_expression_element_count(
        type_expression *semi_labeled_value_list_type_expression)
  {
    assert(semi_labeled_value_list_type_expression != NULL);
    assert(semi_labeled_value_list_type_expression->kind ==
           TEK_SEMI_LABELED_VALUE_LIST);

    return semi_labeled_value_list_type_expression->u.semi_labeled_value_list.
            fields.element_count;
  }

extern type_expression *semi_labeled_value_list_type_expression_element_type(
        type_expression *semi_labeled_value_list_type_expression,
        size_t element_num)
  {
    assert(semi_labeled_value_list_type_expression != NULL);
    assert(semi_labeled_value_list_type_expression->kind ==
           TEK_SEMI_LABELED_VALUE_LIST);

    assert(element_num <
           semi_labeled_value_list_type_expression_element_count(
                   semi_labeled_value_list_type_expression));
    return semi_labeled_value_list_type_expression->u.semi_labeled_value_list.
            fields.array[element_num].type;
  }

extern const char *semi_labeled_value_list_type_expression_element_name(
        type_expression *semi_labeled_value_list_type_expression,
        size_t element_num)
  {
    assert(semi_labeled_value_list_type_expression != NULL);
    assert(semi_labeled_value_list_type_expression->kind ==
           TEK_SEMI_LABELED_VALUE_LIST);

    assert(element_num <
           semi_labeled_value_list_type_expression_element_count(
                   semi_labeled_value_list_type_expression));
    return semi_labeled_value_list_type_expression->u.semi_labeled_value_list.
            fields.array[element_num].name;
  }

extern regular_expression *
        regular_expression_type_expression_regular_expression(
                type_expression *regular_expression_type_expression)
  {
    assert(regular_expression_type_expression != NULL);
    assert(regular_expression_type_expression->kind == TEK_REGULAR_EXPRESSION);

    return regular_expression_type_expression->u.regular_expression.
            regular_expression;
  }

extern void set_type_expression_start_location(
        type_expression *the_type_expression, const source_location *location)
  {
    assert(the_type_expression != NULL);

    set_location_start(&(the_type_expression->location), location);
  }

extern void set_type_expression_end_location(
        type_expression *the_type_expression, const source_location *location)
  {
    assert(the_type_expression != NULL);

    set_location_end(&(the_type_expression->location), location);
  }

extern verdict enumeration_type_expression_add_case(
        type_expression *enumeration_type_expression,
        expression *case_expression)
  {
    verdict the_verdict;

    assert(enumeration_type_expression != NULL);
    assert(enumeration_type_expression->kind == TEK_ENUMERATION);
    assert(case_expression != NULL);

    the_verdict = expression_aa_append(
            &(enumeration_type_expression->u.enumeration.cases),
            case_expression);
    if (the_verdict != MISSION_ACCOMPLISHED)
        delete_expression(case_expression);

    return the_verdict;
  }

extern verdict routine_type_expression_add_formal(
        type_expression *routine_type_expression,
        type_expression *argument_type, const char *name,
        boolean has_default_value)
  {
    formal_argument new_formal;
    verdict the_verdict;

    assert(routine_type_expression != NULL);
    assert(routine_type_expression->kind == TEK_ROUTINE);
    assert(argument_type != NULL);

    new_formal.argument_type = argument_type;

    if (name == NULL)
      {
        new_formal.name = NULL;
      }
    else
      {
        new_formal.name = MALLOC_ARRAY(char, strlen(name) + 1);
        if (new_formal.name == NULL)
          {
            delete_type_expression(argument_type);
            return MISSION_FAILED;
          }
        strcpy(new_formal.name, name);
      }

    new_formal.has_default_value = has_default_value;

    the_verdict = formal_argument_aa_append(
            &(routine_type_expression->u.routine.formals), new_formal);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_type_expression(argument_type);
        if (new_formal.name != NULL)
            free(new_formal.name);
      }

    return the_verdict;
  }

extern verdict routine_type_expression_set_extra_arguments_allowed(
        type_expression *routine_type_expression,
        boolean extra_arguments_allowed)
  {
    assert(routine_type_expression != NULL);
    assert(routine_type_expression->kind == TEK_ROUTINE);

    routine_type_expression->u.routine.extra_arguments_allowed =
            extra_arguments_allowed;
    return MISSION_ACCOMPLISHED;
  }

extern verdict routine_type_expression_set_extra_arguments_unspecified(
        type_expression *routine_type_expression,
        boolean extra_arguments_unspecified)
  {
    assert(routine_type_expression != NULL);
    assert(routine_type_expression->kind == TEK_ROUTINE);

    routine_type_expression->u.routine.extra_arguments_unspecified =
            extra_arguments_unspecified;
    return MISSION_ACCOMPLISHED;
  }

extern verdict fields_type_expression_add_field(
        type_expression *fields_type_expression, type_expression *field_type,
        const char *name)
  {
    field new_field;
    verdict the_verdict;

    assert(fields_type_expression != NULL);
    assert(fields_type_expression->kind == TEK_FIELDS);
    assert(field_type != NULL);

    new_field.type = field_type;

    if (name == NULL)
      {
        new_field.name = NULL;
      }
    else
      {
        new_field.name = MALLOC_ARRAY(char, strlen(name) + 1);
        if (new_field.name == NULL)
          {
            delete_type_expression(field_type);
            return MISSION_FAILED;
          }
        strcpy(new_field.name, name);
      }

    the_verdict = field_aa_append(&(fields_type_expression->u.fields.fields),
                                  new_field);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_type_expression(field_type);
        if (new_field.name != NULL)
            free(new_field.name);
      }

    return the_verdict;
  }

extern verdict fields_type_expression_set_extra_fields_allowed(
        type_expression *fields_type_expression, boolean extra_fields_allowed)
  {
    assert(fields_type_expression != NULL);
    assert(fields_type_expression->kind == TEK_FIELDS);

    fields_type_expression->u.fields.extra_fields_allowed =
            extra_fields_allowed;
    return MISSION_ACCOMPLISHED;
  }

extern verdict lepton_type_expression_add_field(
        type_expression *lepton_type_expression, type_expression *field_type,
        const char *name)
  {
    field new_field;
    verdict the_verdict;

    assert(lepton_type_expression != NULL);
    assert(lepton_type_expression->kind == TEK_LEPTON);
    assert(field_type != NULL);

    new_field.type = field_type;

    if (name == NULL)
      {
        new_field.name = NULL;
      }
    else
      {
        new_field.name = MALLOC_ARRAY(char, strlen(name) + 1);
        if (new_field.name == NULL)
          {
            delete_type_expression(field_type);
            return MISSION_FAILED;
          }
        strcpy(new_field.name, name);
      }

    the_verdict = field_aa_append(&(lepton_type_expression->u.lepton.fields),
                                  new_field);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_type_expression(field_type);
        if (new_field.name != NULL)
            free(new_field.name);
      }

    return the_verdict;
  }

extern verdict lepton_type_expression_set_extra_fields_allowed(
        type_expression *lepton_type_expression, boolean extra_fields_allowed)
  {
    assert(lepton_type_expression != NULL);
    assert(lepton_type_expression->kind == TEK_LEPTON);

    lepton_type_expression->u.lepton.extra_fields_allowed =
            extra_fields_allowed;
    return MISSION_ACCOMPLISHED;
  }

extern verdict multiset_type_expression_add_field(
        type_expression *multiset_type_expression, type_expression *field_type,
        const char *name)
  {
    field new_field;
    verdict the_verdict;

    assert(multiset_type_expression != NULL);
    assert(multiset_type_expression->kind == TEK_MULTISET);
    assert(field_type != NULL);

    new_field.type = field_type;

    if (name == NULL)
      {
        new_field.name = NULL;
      }
    else
      {
        new_field.name = MALLOC_ARRAY(char, strlen(name) + 1);
        if (new_field.name == NULL)
          {
            delete_type_expression(field_type);
            return MISSION_FAILED;
          }
        strcpy(new_field.name, name);
      }

    the_verdict = field_aa_append(
            &(multiset_type_expression->u.multiset.fields), new_field);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_type_expression(field_type);
        if (new_field.name != NULL)
            free(new_field.name);
      }

    return the_verdict;
  }

extern verdict multiset_type_expression_set_extra_fields_allowed(
        type_expression *multiset_type_expression,
        boolean extra_fields_allowed)
  {
    assert(multiset_type_expression != NULL);
    assert(multiset_type_expression->kind == TEK_MULTISET);

    multiset_type_expression->u.multiset.extra_fields_allowed =
            extra_fields_allowed;
    return MISSION_ACCOMPLISHED;
  }

extern verdict interface_type_expression_add_item(
        type_expression *interface_type_expression, type_expression *item_type,
        const char *name, boolean writing_allowed)
  {
    interface_item new_item;
    verdict the_verdict;

    assert(interface_type_expression != NULL);
    assert(interface_type_expression->kind == TEK_INTERFACE);
    assert(item_type != NULL);

    new_item.type = item_type;

    if (name == NULL)
      {
        new_item.name = NULL;
      }
    else
      {
        new_item.name = MALLOC_ARRAY(char, strlen(name) + 1);
        if (new_item.name == NULL)
          {
            delete_type_expression(item_type);
            return MISSION_FAILED;
          }
        strcpy(new_item.name, name);
      }

    new_item.writing_allowed = writing_allowed;

    the_verdict = interface_item_aa_append(
            &(interface_type_expression->u.interface.items), new_item);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_type_expression(item_type);
        if (new_item.name != NULL)
            free(new_item.name);
      }

    return the_verdict;
  }

extern verdict semi_labeled_value_list_type_expression_add_element(
        type_expression *semi_labeled_value_list_type_expression,
        type_expression *element_type, const char *name)
  {
    field new_field;
    verdict the_verdict;

    assert(semi_labeled_value_list_type_expression != NULL);
    assert(semi_labeled_value_list_type_expression->kind ==
           TEK_SEMI_LABELED_VALUE_LIST);
    assert(element_type != NULL);

    new_field.type = element_type;

    if (name == NULL)
      {
        new_field.name = NULL;
      }
    else
      {
        new_field.name = MALLOC_ARRAY(char, strlen(name) + 1);
        if (new_field.name == NULL)
          {
            delete_type_expression(element_type);
            return MISSION_FAILED;
          }
        strcpy(new_field.name, name);
      }

    the_verdict = field_aa_append(
            &(semi_labeled_value_list_type_expression->u.
                      semi_labeled_value_list.fields), new_field);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_type_expression(element_type);
        if (new_field.name != NULL)
            free(new_field.name);
      }

    return the_verdict;
  }

extern verdict
        semi_labeled_value_list_type_expression_set_extra_elements_allowed(
                type_expression *semi_labeled_value_list_type_expression,
                boolean extra_elements_allowed)
  {
    assert(semi_labeled_value_list_type_expression != NULL);
    assert(semi_labeled_value_list_type_expression->kind ==
           TEK_SEMI_LABELED_VALUE_LIST);

    semi_labeled_value_list_type_expression->u.semi_labeled_value_list.
            extra_elements_allowed = extra_elements_allowed;
    return MISSION_ACCOMPLISHED;
  }

extern const source_location *get_type_expression_location(
        type_expression *the_type_expression)
  {
    assert(the_type_expression != NULL);

    return &(the_type_expression->location);
  }

extern void type_expression_error(type_expression *the_type_expression,
                                  const char *format, ...)
  {
    va_list ap;

    assert(the_type_expression != NULL);
    assert(format != NULL);

    va_start(ap, format);
    vlocation_error(get_type_expression_location(the_type_expression), format,
                    ap);
    va_end(ap);
  }

extern void vtype_expression_error(type_expression *the_type_expression,
                                   const char *format, va_list arg)
  {
    assert(the_type_expression != NULL);
    assert(format != NULL);

    vlocation_error(get_type_expression_location(the_type_expression), format,
                    arg);
  }


static type_expression *create_empty_type_expression(void)
  {
    type_expression *result;

    result = MALLOC_ONE_OBJECT(type_expression);
    if (result == NULL)
        return NULL;

    set_source_location(&(result->location), NULL);

    return result;
  }
