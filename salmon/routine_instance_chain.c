/* file "routine_instance_chain.c" */

/*
 *  This file contains the implementation of the routine_instance_chain module.
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
#include "routine_instance_chain.h"
#include "value.h"
#include "semi_labeled_value_list.h"
#include "routine_instance.h"
#include "type.h"
#include "routine_declaration.h"
#include "jumper.h"
#include "execute.h"
#include "reference_cluster.h"
#include "driver.h"
#include "validator.h"
#include "platform_dependent.h"


struct routine_instance_chain
  {
    routine_instance *instance;
    routine_instance_chain *next;
    reference_cluster *reference_cluster;
    validator *validator;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
  };


static void match_call(routine_instance *instance, size_t argument_count,
        type **argument_types, const char **argument_names,
        boolean *always_matches, boolean *never_matches,
        const source_location *location, jumper *the_jumper);


extern routine_instance *resolve_overloading(
        routine_instance_chain *instance_chain,
        semi_labeled_value_list *arguments, const source_location *location,
        jumper *the_jumper)
  {
    routine_instance *instance;
    routine_instance_chain *next_chain;

    assert(instance_chain != NULL);
    assert(arguments != NULL);

    instance = instance_chain->instance;
    assert(instance != NULL);

    if (routine_instance_fits_actuals(instance, arguments, location,
                                      the_jumper))
      {
        assert((!jumper_flowing_forward(the_jumper)) ||
               routine_instance_is_instantiated(instance));
                /* VERIFIED */
        assert((!jumper_flowing_forward(the_jumper)) ||
               !(routine_instance_scope_exited(instance)));
                /* VERIFIED */
        return instance;
      }
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    next_chain = instance_chain->next;
    if (next_chain != NULL)
      {
        return resolve_overloading(next_chain, arguments, location,
                                   the_jumper);
      }

    return NULL;
  }

extern boolean routine_instance_fits_actuals(
        routine_instance *the_routine_instance,
        semi_labeled_value_list *arguments, const source_location *location,
        jumper *the_jumper)
  {
    routine_declaration *declaration;
    formal_arguments *formals;
    size_t explicit_argument_count;
    size_t actual_count;
    boolean error;
    size_t post_order_count;
    size_t duplicate_formal_argument_num;
    size_t bad_name_actual_argument_num;
    size_t *post_order_array;
    size_t post_order_num;

    assert(the_routine_instance != NULL);
    assert(arguments != NULL);

    declaration = routine_instance_declaration(the_routine_instance);
    assert(declaration != NULL);

    formals = routine_declaration_formals(declaration);
    assert(formals != NULL);

    if (!(routine_instance_is_instantiated(the_routine_instance)))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(overloading_resolution_uninstantiated),
                "An attempt was made to resolve overloading that possibly "
                "included %r before it was instantiated by executing its "
                "declaration.", declaration);
        return FALSE;
      }

    if (routine_instance_scope_exited(the_routine_instance))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(overloading_resolution_deallocated),
                "An attempt was made to resolve overloading that possibly "
                "included %r after it had ceased to exist because its scope "
                "had ended.", declaration);
        return FALSE;
      }

    assert(the_routine_instance != NULL);

    explicit_argument_count = formal_arguments_argument_count(formals);

    actual_count = semi_labeled_value_list_value_count(arguments);

    post_order_array = resolve_parameter_ordering_from_semi_ordered_value_list(
            formals, arguments,
            routine_declaration_extra_arguments_allowed(declaration), &error,
            &post_order_count, &duplicate_formal_argument_num,
            &bad_name_actual_argument_num);
    if (error)
      {
        assert(post_order_array == NULL);
        jumper_do_abort(the_jumper);
        return FALSE;
      }

    if ((post_order_array == NULL) && (actual_count > 0))
        return FALSE;

    if (post_order_count > explicit_argument_count)
      {
        assert(post_order_array != NULL);
        if (!(routine_declaration_extra_arguments_allowed(declaration)))
          {
            free(post_order_array);
            return FALSE;
          }
      }

    for (post_order_num = 0; post_order_num < post_order_count;
         ++post_order_num)
      {
        size_t actual_num;

        assert(post_order_array != NULL);
        actual_num = post_order_array[post_order_num];
        assert(actual_num <= actual_count);

        if (actual_num == actual_count)
          {
            variable_declaration *formal_declaration;

            assert(post_order_num < explicit_argument_count);
            formal_declaration =
                    formal_arguments_formal_by_number(formals, post_order_num);
            assert(formal_declaration != NULL);

            if (variable_declaration_initializer(formal_declaration) == NULL)
              {
                free(post_order_array);
                return FALSE;
              }
          }
        else if (post_order_num < explicit_argument_count)
          {
            value *actual_value;
            type *argument_type;
            boolean is_in;
            boolean doubt;
            char *why_not;

            actual_value =
                    semi_labeled_value_list_value(arguments, actual_num);
            assert(actual_value != NULL);

            assert(routine_instance_is_instantiated(the_routine_instance));
                    /* VERIFIED */
            assert(!(routine_instance_scope_exited(the_routine_instance)));
                    /* VERIFIED */
            argument_type = routine_instance_argument_type(
                    the_routine_instance, post_order_num);
            assert(type_is_valid(argument_type)); /* VERIFICATION NEEDED */
            is_in = value_is_in_type(actual_value, argument_type, &doubt,
                                     &why_not, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                free(post_order_array);
                return FALSE;
              }

            if (doubt)
              {
                free(post_order_array);
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(overloading_resolution_indeterminate),
                        "When trying to resolve overloading, %s could not "
                        "determine whether a parameter type matched an actual "
                        "parameter value because %s.", interpreter_name(),
                        why_not);
                free(why_not);
                return FALSE;
              }

            if (!is_in)
              {
                free(why_not);
                free(post_order_array);
                return FALSE;
              }
          }
      }

    for (; post_order_num < explicit_argument_count; ++post_order_num)
      {
        variable_declaration *formal_declaration;

        formal_declaration =
                formal_arguments_formal_by_number(formals, post_order_num);
        assert(formal_declaration != NULL);

        if (variable_declaration_initializer(formal_declaration) == NULL)
          {
            if (post_order_array != NULL)
                free(post_order_array);
            return FALSE;
          }
      }

    if (post_order_array != NULL)
        free(post_order_array);

    return TRUE;
  }

extern boolean routine_instance_fits_pattern(
        routine_instance *the_routine_instance, size_t parameter_count,
        parameter_pattern_kind *parameter_pattern_kinds,
        const char **parameter_names, value **exact_parameters,
        type **parameter_lower_types, type **parameter_upper_types,
        type **result_parameter_types, boolean *doubt,
        const source_location *location, jumper *the_jumper)
  {
    routine_declaration *declaration;
    formal_arguments *formals;
    size_t explicit_argument_count;
    boolean error;
    size_t post_order_count;
    size_t duplicate_formal_argument_num;
    size_t bad_name_actual_argument_num;
    size_t *post_order_array;
    size_t post_order_num;

    assert(the_routine_instance != NULL);
    if (parameter_count > 0)
      {
        assert(parameter_pattern_kinds != NULL);
        assert(parameter_names != NULL);
      }
    assert(doubt != NULL);
    assert(the_jumper != NULL);

    *doubt = FALSE;

    declaration = routine_instance_declaration(the_routine_instance);
    assert(declaration != NULL);

    formals = routine_declaration_formals(declaration);
    assert(formals != NULL);

    if (!(routine_instance_is_instantiated(the_routine_instance)))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(overloading_resolution_uninstantiated),
                "An attempt was made to resolve overloading that possibly "
                "included %r before it was instantiated by executing its "
                "declaration.", declaration);
        return FALSE;
      }

    if (routine_instance_scope_exited(the_routine_instance))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(overloading_resolution_deallocated),
                "An attempt was made to resolve overloading that possibly "
                "included %r after it had ceased to exist because its scope "
                "had ended.", declaration);
        return FALSE;
      }

    explicit_argument_count = formal_arguments_argument_count(formals);

    post_order_array = resolve_parameter_ordering_from_name_array(formals,
            parameter_count, parameter_names,
            routine_declaration_extra_arguments_allowed(declaration), &error,
            &post_order_count, &duplicate_formal_argument_num,
            &bad_name_actual_argument_num);
    if (error)
      {
        assert(post_order_array == NULL);
        jumper_do_abort(the_jumper);
        return FALSE;
      }

    if ((post_order_array == NULL) && (parameter_count > 0))
        return FALSE;

    if (post_order_count > explicit_argument_count)
      {
        assert(post_order_array != NULL);
        if (!(routine_declaration_extra_arguments_allowed(declaration)))
          {
            free(post_order_array);
            return FALSE;
          }
      }

    for (post_order_num = 0; post_order_num < post_order_count;
         ++post_order_num)
      {
        size_t actual_num;

        assert(post_order_array != NULL);
        actual_num = post_order_array[post_order_num];
        assert(actual_num <= parameter_count);

        if (actual_num == parameter_count)
          {
            variable_declaration *formal_declaration;

            assert(post_order_num < explicit_argument_count);
            formal_declaration =
                    formal_arguments_formal_by_number(formals, post_order_num);
            assert(formal_declaration != NULL);

            if (variable_declaration_initializer(formal_declaration) == NULL)
              {
                free(post_order_array);
                *doubt = FALSE;
                return FALSE;
              }
          }
        else if (post_order_num < explicit_argument_count)
          {
            type *formal_type;

            assert(routine_instance_is_instantiated(the_routine_instance));
                    /* VERIFICATION NEEDED */
            assert(!(routine_instance_scope_exited(the_routine_instance)));
                    /* VERIFICATION NEEDED */
            formal_type = routine_instance_argument_type(the_routine_instance,
                                                         post_order_num);
            assert(formal_type != NULL);

            if (result_parameter_types != NULL)
                result_parameter_types[actual_num] = formal_type;

            assert(parameter_pattern_kinds != NULL);
            switch (parameter_pattern_kinds[actual_num])
              {
                case PPK_EXACT:
                  {
                    value *actual_value;
                    boolean is_in;
                    boolean local_doubt;

                    assert(exact_parameters != NULL);
                    actual_value = exact_parameters[actual_num];
                    assert(actual_value != NULL);
                    assert(value_is_valid(actual_value)); /* VERIFIED */

                    assert(type_is_valid(formal_type));
                            /* VERIFICATION NEEDED */
                    is_in = value_is_in_type(actual_value, formal_type,
                            &local_doubt, NULL, location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        free(post_order_array);
                        return FALSE;
                      }

                    if (local_doubt)
                      {
                        *doubt = TRUE;
                      }
                    else if (!is_in)
                      {
                        free(post_order_array);
                        *doubt = FALSE;
                        return FALSE;
                      }

                    break;
                  }
                case PPK_ANY:
                  {
                    type *anything_type;
                    boolean is_in;
                    boolean local_doubt;

                    anything_type = get_anything_type();
                    if (anything_type == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        free(post_order_array);
                        return FALSE;
                      }

                    assert(type_is_valid(anything_type));
                            /* VERIFICATION NEEDED */
                    assert(type_is_valid(formal_type));
                            /* VERIFICATION NEEDED */
                    is_in = type_is_subset(anything_type, formal_type,
                            &local_doubt, NULL, location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        free(post_order_array);
                        return FALSE;
                      }

                    if (local_doubt || (!is_in))
                        *doubt = TRUE;

                    break;
                  }
                case PPK_TYPE:
                  {
                    type *upper_type;
                    boolean is_in;
                    boolean local_doubt;
                    type *lower_type;
                    boolean no_match;

                    assert(parameter_upper_types != NULL);
                    upper_type = parameter_upper_types[actual_num];
                    assert(upper_type != NULL);
                    assert(type_is_valid(upper_type));
                            /* VERIFICATION NEEDED */

                    assert(type_is_valid(upper_type));
                            /* VERIFICATION NEEDED */
                    assert(type_is_valid(formal_type));
                            /* VERIFICATION NEEDED */
                    is_in = type_is_subset(upper_type, formal_type,
                            &local_doubt, NULL, location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        free(post_order_array);
                        return FALSE;
                      }

                    if (local_doubt || (!is_in))
                        *doubt = TRUE;

                    assert(parameter_lower_types != NULL);
                    lower_type = parameter_lower_types[actual_num];
                    assert(lower_type != NULL);
                    assert(type_is_valid(lower_type));
                            /* VERIFICATION NEEDED */

                    assert(type_is_valid(lower_type));
                            /* VERIFICATION NEEDED */
                    assert(type_is_valid(formal_type));
                            /* VERIFICATION NEEDED */
                    no_match = intersection_empty(lower_type, formal_type,
                            &local_doubt, location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        free(post_order_array);
                        return FALSE;
                      }

                    if ((!local_doubt) && no_match)
                      {
                        free(post_order_array);
                        *doubt = FALSE;
                        return FALSE;
                      }

                    break;
                  }
                default:
                  {
                    assert(FALSE);
                  }
              }
          }
        else if (result_parameter_types != NULL)
          {
            type *anything_type;

            anything_type = get_anything_type();
            if (anything_type == NULL)
              {
                jumper_do_abort(the_jumper);
                free(post_order_array);
                return FALSE;
              }

            result_parameter_types[actual_num] = anything_type;
          }
      }

    for (; post_order_num < explicit_argument_count; ++post_order_num)
      {
        variable_declaration *formal_declaration;

        formal_declaration =
                formal_arguments_formal_by_number(formals, post_order_num);
        assert(formal_declaration != NULL);

        if (variable_declaration_initializer(formal_declaration) == NULL)
          {
            if (post_order_array != NULL)
                free(post_order_array);
            *doubt = FALSE;
            return FALSE;
          }
      }

    if (post_order_array != NULL)
        free(post_order_array);

    return TRUE;
  }

extern void get_overloading_information(routine_instance_chain *instance_chain,
        size_t argument_count, type **argument_types,
        const char **argument_names, type **return_type,
        boolean *always_resolves, boolean *never_resolves,
        boolean *always_pure, routine_instance **only_possible_resolution,
        const source_location *location, jumper *the_jumper)
  {
    type *local_return_type;
    boolean local_always_pure;
    size_t match_count;
    routine_instance_chain *follow;
    routine_instance *instance;

    assert(instance_chain != NULL);
    assert((argument_count == 0) ||
           ((argument_types != NULL) && (argument_names != NULL)));
    assert(return_type != NULL);
    assert(always_resolves != NULL);
    assert(never_resolves != NULL);
    assert(always_pure != NULL);
    assert(only_possible_resolution != NULL);
    assert(the_jumper != NULL);

    local_return_type = get_nothing_type();
    if (local_return_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    assert(type_is_valid(local_return_type)); /* VERIFIED */
    type_add_reference(local_return_type);

    local_always_pure = TRUE;

    match_count = 0;

    *always_resolves = FALSE;

    for (follow = instance_chain; follow != NULL; follow = follow->next)
      {
        boolean always_matches;
        boolean never_matches;
        type *instance_return_type;
        type *new_return_type;

        instance = follow->instance;
        assert(instance != NULL);

        match_call(instance, argument_count, argument_types, argument_names,
                   &always_matches, &never_matches, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(local_return_type, the_jumper);
            return;
          }

        if (never_matches)
            continue;

        ++match_count;

        if (local_always_pure &&
            !(routine_declaration_is_pure(
                      routine_instance_declaration(instance))))
          {
            local_always_pure = FALSE;
          }

        assert(routine_instance_is_instantiated(instance));
                /* VERIFICATION NEEDED */
        assert(!(routine_instance_scope_exited(instance)));
                /* VERIFICATION NEEDED */
        instance_return_type = routine_instance_valid_return_type(instance,
                location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(instance_return_type == NULL);
            type_remove_reference(local_return_type, the_jumper);
            return;
          }

        assert(type_is_valid(local_return_type)); /* VERIFIED */
        assert(type_is_valid(instance_return_type)); /* VERIFIED */
        new_return_type =
                get_union_type(local_return_type, instance_return_type);
        if (new_return_type == NULL)
          {
            jumper_do_abort(the_jumper);
            type_remove_reference(local_return_type, the_jumper);
            return;
          }

        assert(type_is_valid(new_return_type)); /* VERIFIED */

        type_remove_reference(local_return_type, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(new_return_type, the_jumper);
            return;
          }

        assert(type_is_valid(new_return_type)); /* VERIFIED */
        local_return_type = new_return_type;
        assert(type_is_valid(local_return_type)); /* VERIFIED */

        if (always_matches)
          {
            *always_resolves = TRUE;
            break;
          }
      }

    assert(type_is_valid(local_return_type)); /* VERIFIED */
    *return_type = local_return_type;
    *never_resolves = (match_count == 0);
    *always_pure = local_always_pure;

    if (match_count == 1)
        *only_possible_resolution = instance;
    else
        *only_possible_resolution = NULL;

    assert(routine_instance_chain_is_valid(instance_chain)); /* VERIFIED */
    assert(routine_instance_chain_is_valid(instance_chain)); /* VERIFIED */
    if (*only_possible_resolution != NULL)
      {
        assert(routine_instance_is_instantiated(
                       *only_possible_resolution)); /* VERIFIED */
        assert(!(routine_instance_scope_exited(
                         *only_possible_resolution))); /* VERIFIED */
      }
  }

extern routine_instance_chain *create_routine_instance_chain(
        routine_instance *head, routine_instance_chain *tail)
  {
    return create_routine_instance_chain_with_cluster(head, tail, NULL);
  }

extern routine_instance_chain *create_routine_instance_chain_with_cluster(
        routine_instance *head, routine_instance_chain *tail,
        reference_cluster *cluster)
  {
    routine_instance_chain *result;
    reference_cluster *cluster2;

    assert(head != NULL);

    result = MALLOC_ONE_OBJECT(routine_instance_chain);
    if (result == NULL)
        return result;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock, free(result); return NULL);

    cluster2 = routine_instance_reference_cluster(head);
    if ((cluster2 == NULL) && (tail != NULL))
        cluster2 = tail->reference_cluster;
    if ((cluster2 != NULL) && (cluster2 != cluster))
        reference_cluster_add_reference(cluster2);
    routine_instance_add_reference_with_cluster(head, cluster2);
    if (tail != NULL)
        routine_instance_chain_add_reference_with_cluster(tail, cluster2);

    result->instance = head;
    result->next = tail;
    result->reference_cluster = cluster2;
    result->reference_count = 1;

    result->validator = validator_add_instance(get_trivial_validator(),
            routine_instance_instance(head));
    if (result->validator == NULL)
      {
        routine_instance_chain_remove_reference_with_cluster(result, cluster,
                                                             NULL);
        return NULL;
      }

    return result;
  }

extern void routine_instance_chain_add_reference(
        routine_instance_chain *instance_chain)
  {
    routine_instance_chain_add_reference_with_cluster(instance_chain, NULL);
  }

extern void routine_instance_chain_remove_reference(
        routine_instance_chain *instance_chain, jumper *the_jumper)
  {
    routine_instance_chain_remove_reference_with_cluster(instance_chain, NULL,
                                                         the_jumper);
  }

extern void routine_instance_chain_add_reference_with_cluster(
        routine_instance_chain *instance_chain, reference_cluster *cluster)
  {
    assert(instance_chain != NULL);

    GRAB_SYSTEM_LOCK(instance_chain->reference_lock);
    assert(instance_chain->reference_count > 0);
    ++(instance_chain->reference_count);
    RELEASE_SYSTEM_LOCK(instance_chain->reference_lock);

    if ((instance_chain->reference_cluster != NULL) &&
        (instance_chain->reference_cluster != cluster))
      {
        reference_cluster_add_reference(instance_chain->reference_cluster);
      }
  }

extern void routine_instance_chain_remove_reference_with_cluster(
        routine_instance_chain *instance_chain, reference_cluster *cluster,
        jumper *the_jumper)
  {
    size_t new_reference_count;

    assert(instance_chain != NULL);

    GRAB_SYSTEM_LOCK(instance_chain->reference_lock);
    assert(instance_chain->reference_count > 0);
    RELEASE_SYSTEM_LOCK(instance_chain->reference_lock);

    if ((instance_chain->reference_cluster != NULL) &&
        (instance_chain->reference_cluster != cluster))
      {
        reference_cluster_remove_reference(instance_chain->reference_cluster,
                                           the_jumper);
      }

    GRAB_SYSTEM_LOCK(instance_chain->reference_lock);
    assert(instance_chain->reference_count > 0);
    --(instance_chain->reference_count);
    new_reference_count = instance_chain->reference_count;
    RELEASE_SYSTEM_LOCK(instance_chain->reference_lock);

    if (new_reference_count > 0)
        return;

    validator_remove_reference(instance_chain->validator);

    assert(instance_chain->instance != NULL);
    routine_instance_remove_reference_with_cluster(instance_chain->instance,
            instance_chain->reference_cluster, the_jumper);

    if (instance_chain->next != NULL)
      {
        routine_instance_chain_remove_reference_with_cluster(
                instance_chain->next, instance_chain->reference_cluster,
                the_jumper);
      }

    DESTROY_SYSTEM_LOCK(instance_chain->reference_lock);

    free(instance_chain);
  }

extern reference_cluster *routine_instance_chain_reference_cluster(
        routine_instance_chain *instance_chain)
  {
    assert(instance_chain != NULL);

    return instance_chain->reference_cluster;
  }

extern routine_instance *routine_instance_chain_instance(
        routine_instance_chain *instance_chain)
  {
    assert(instance_chain != NULL);

    return instance_chain->instance;
  }

extern routine_instance_chain *routine_instance_chain_next(
        routine_instance_chain *instance_chain)
  {
    assert(instance_chain != NULL);

    return instance_chain->next;
  }

extern routine_instance_chain *combine_routine_chains(
        routine_instance_chain *back, routine_instance_chain *front,
        reference_cluster *cluster)
  {
    routine_instance_chain *tail;
    routine_instance_chain *result;

    assert(back != NULL);

    if (front == NULL)
      {
        routine_instance_chain_add_reference_with_cluster(back, cluster);
        return back;
      }

    tail = combine_routine_chains(back, routine_instance_chain_next(front),
                                  cluster);
    if (tail == NULL)
        return NULL;

    result = create_routine_instance_chain_with_cluster(
            routine_instance_chain_instance(front), tail, cluster);
    routine_instance_chain_remove_reference_with_cluster(tail, cluster, NULL);
    return result;
  }

extern boolean routine_instance_chain_is_valid(
        routine_instance_chain *instance_chain)
  {
    assert(instance_chain != NULL);

    return validator_is_valid(instance_chain->validator);
  }

extern void check_routine_instance_chain_validity(
        routine_instance_chain *instance_chain,
        const source_location *location, jumper *the_jumper)
  {
    assert(instance_chain != NULL);
    assert(the_jumper != NULL);

    validator_check_validity(instance_chain->validator, location, the_jumper);
  }

extern validator *routine_instance_chain_validator(
        routine_instance_chain *instance_chain)
  {
    assert(instance_chain != NULL);

    return instance_chain->validator;
  }

extern boolean routine_instance_chains_are_equal(
        routine_instance_chain *chain1, routine_instance_chain *chain2)
  {
    routine_instance_chain *next1;
    routine_instance_chain *next2;

    assert(chain1 != NULL);
    assert(chain2 != NULL);

    assert(routine_instance_chain_is_valid(chain1)); /* VERIFIED */
    assert(routine_instance_chain_is_valid(chain2)); /* VERIFIED */

    if (chain1 == chain2)
        return TRUE;

    assert(routine_instance_is_instantiated(chain1->instance)); /* VERIFIED */
    assert(routine_instance_is_instantiated(chain2->instance)); /* VERIFIED */
    assert(!(routine_instance_scope_exited(chain1->instance))); /* VERIFIED */
    assert(!(routine_instance_scope_exited(chain2->instance))); /* VERIFIED */
    if (!(routine_instances_are_equal(chain1->instance, chain2->instance)))
        return FALSE;

    next1 = chain1->next;
    next2 = chain2->next;

    if (next1 == NULL)
        return (next2 == NULL);

    if (next2 == NULL)
        return FALSE;

    assert(routine_instance_chain_is_valid(next1)); /* VERIFICATION NEEDED */
    assert(routine_instance_chain_is_valid(next2)); /* VERIFICATION NEEDED */
    return routine_instance_chains_are_equal(next1, next2);
  }

extern int routine_instance_chain_structural_order(
        routine_instance_chain *left, routine_instance_chain *right)
  {
    int local_order;
    routine_instance_chain *next_left;
    routine_instance_chain *next_right;

    assert(left != NULL);
    assert(right != NULL);

    assert(routine_instance_chain_is_valid(left)); /* VERIFIED */
    assert(routine_instance_chain_is_valid(right)); /* VERIFIED */

    if (left == right)
        return 0;

    assert(routine_instance_is_instantiated(left->instance)); /* VERIFIED */
    assert(routine_instance_is_instantiated(right->instance)); /* VERIFIED */
    assert(!(routine_instance_scope_exited(left->instance))); /* VERIFIED */
    assert(!(routine_instance_scope_exited(right->instance))); /* VERIFIED */
    local_order =
            routine_instance_structural_order(left->instance, right->instance);
    if (local_order != 0)
        return local_order;

    next_left = left->next;
    next_right = right->next;

    if (next_left == NULL)
      {
        if (next_right == NULL)
            return 0;
        return -1;
      }

    if (next_right == NULL)
        return 1;

    assert(routine_instance_chain_is_valid(next_left));
            /* VERIFICATION NEEDED */
    assert(routine_instance_chain_is_valid(next_right));
            /* VERIFICATION NEEDED */
    return routine_instance_chain_structural_order(next_left, next_right);
  }


static void match_call(routine_instance *instance, size_t argument_count,
        type **argument_types, const char **argument_names,
        boolean *always_matches, boolean *never_matches,
        const source_location *location, jumper *the_jumper)
  {
    routine_declaration *declaration;
    formal_arguments *formals;
    size_t formal_count;
    boolean local_error;
    size_t post_order_count;
    size_t duplicate_formal_argument_num;
    size_t bad_name_actual_argument_num;
    size_t *post_order_array;
    size_t post_order_num;

    assert(instance != NULL);
    assert((argument_count == 0) || (argument_types != NULL));
    assert((argument_count == 0) || (argument_names != NULL));
    assert(always_matches != NULL);
    assert(never_matches != NULL);
    assert(the_jumper != NULL);

    declaration = routine_instance_declaration(instance);
    assert(declaration != NULL);

    formals = routine_declaration_formals(declaration);
    assert(formals != NULL);

    if (!(routine_instance_is_instantiated(instance)))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(routine_type_uninstantiated),
                "%R was used as part of a type before it was instantiated by "
                "executing its declaration.", declaration);
        return;
      }

    if (routine_instance_scope_exited(instance))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(routine_type_deallocated),
                "%R was used as part of a type after it had been deallocated.",
                declaration);
        return;
      }

    formal_count = formal_arguments_argument_count(formals);

    post_order_array = resolve_parameter_ordering_from_name_array(formals,
            argument_count, argument_names,
            routine_declaration_extra_arguments_allowed(declaration),
            &local_error, &post_order_count, &duplicate_formal_argument_num,
            &bad_name_actual_argument_num);
    if (local_error)
      {
        assert(post_order_array == NULL);
        jumper_do_abort(the_jumper);
        return;
      }

    if ((post_order_array == NULL) && (argument_count > 0))
      {
        *always_matches = FALSE;
        *never_matches = TRUE;
        return;
      }

    if (post_order_count > formal_count)
      {
        assert(post_order_array != NULL);
        if (!(routine_declaration_extra_arguments_allowed(declaration)))
          {
            free(post_order_array);
            *always_matches = FALSE;
            *never_matches = TRUE;
            return;
          }
      }

    *always_matches = TRUE;

    for (post_order_num = 0; post_order_num < post_order_count;
         ++post_order_num)
      {
        size_t actual_num;

        assert(post_order_array != NULL);
        actual_num = post_order_array[post_order_num];
        assert(actual_num <= argument_count);

        if (actual_num == argument_count)
          {
            variable_declaration *formal_declaration;

            assert(post_order_num < formal_count);
            formal_declaration =
                    formal_arguments_formal_by_number(formals, post_order_num);
            assert(formal_declaration != NULL);

            if (variable_declaration_initializer(formal_declaration) == NULL)
              {
                free(post_order_array);
                *always_matches = FALSE;
                *never_matches = TRUE;
                return;
              }
          }
        else if (post_order_num < formal_count)
          {
            type *argument_type;
            type *formal_type;
            boolean is_subset;
            boolean doubt;

            assert(routine_instance_is_instantiated(instance));
                    /* VERIFICATION NEEDED */
            assert(!(routine_instance_scope_exited(instance)));
                    /* VERIFICATION NEEDED */
            argument_type = argument_types[actual_num];
            assert(argument_type != NULL);
            formal_type =
                    routine_instance_argument_type(instance, post_order_num);
            assert(formal_type != NULL);
            assert(type_is_valid(argument_type)); /* VERIFICATION NEEDED */
            assert(type_is_valid(formal_type)); /* VERIFICATION NEEDED */
            is_subset = type_is_subset(argument_type, formal_type, &doubt,
                                       NULL, location, the_jumper);

            if (doubt)
              {
                *always_matches = FALSE;
              }
            else if (!is_subset)
              {
                free(post_order_array);
                *always_matches = FALSE;
                *never_matches = FALSE;
                return;
              }
          }
      }

    if (post_order_array != NULL)
        free(post_order_array);

    *never_matches = FALSE;
  }
