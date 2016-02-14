/* file "validator.c" */

/*
 *  This file contains the implementation of the validator module.
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
#include <assert.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "validator.h"
#include "instance.h"
#include "jump_target.h"
#include "object.h"
#include "source_location.h"
#include "jumper.h"
#include "platform_dependent.h"


typedef enum
  {
    VAK_TRIVIAL,
    VAK_INSTANCE,
    VAK_JUMP_TARGET,
    VAK_OBJECT
  } validator_kind;

struct validator_chain
  {
    boolean locally_valid;
    boolean globally_valid;
    validator_chain *base;
    validator_chain *local_next;
    validator_chain *local_previous;
    validator_chain *next_for_base;
    validator_chain *previous_for_base;
    validator_chain *users;
  };

struct validator
  {
    validator_chain chain_element;
    validator_kind kind;
    union
      {
        instance *instance;
        jump_target *jump_target;
        object *object;
      } u;
    validator *next;
    DECLARE_SYSTEM_LOCK(lock);
    size_t reference_count;
  };


static validator trivial_validator;
static boolean initialized = FALSE;
DECLARE_SYSTEM_LOCK(validator_update);


static validator *create_empty_validator(validator_kind kind, validator *next,
        boolean locally_valid, validator_chain *local_next);
static void mark_chain_globally_invalid(validator_chain *chain);
static void mark_chain_globally_valid(validator_chain *chain);
static validator *validator_remove_one_validator(validator *base_validator,
                                                 validator *to_remove);
static void handle_uninstantiated_instance(instance *the_instance,
        const source_location *location, jumper *the_jumper);
static void handle_scope_exited_instance(instance *the_instance,
        const source_location *location, jumper *the_jumper);
static void handle_closed_object(object *the_object,
        const source_location *location, jumper *the_jumper);
static void validator_check_validity_internal(validator *the_validator,
        const source_location *location, jumper *the_jumper);


#define ASSERT_IS_GOOD_VALIDATOR(the_validator) \
    assert(the_validator != NULL); \
    if (the_validator != &trivial_validator) \
        assert_is_malloced_block_with_exact_size(the_validator, \
                                                 sizeof(validator));


extern validator *get_trivial_validator(void)
  {
    if (!initialized)
      {
        trivial_validator.chain_element.locally_valid = TRUE;
        trivial_validator.chain_element.globally_valid = TRUE;
        trivial_validator.chain_element.base = NULL;
        trivial_validator.chain_element.local_next = NULL;
        trivial_validator.chain_element.local_previous = NULL;
        trivial_validator.chain_element.next_for_base = NULL;
        trivial_validator.chain_element.previous_for_base = NULL;
        trivial_validator.kind = VAK_TRIVIAL;
        trivial_validator.next = NULL;
        INITIALIZE_SYSTEM_LOCK(trivial_validator.lock, assert(FALSE));
        trivial_validator.reference_count = 0;

        INITIALIZE_SYSTEM_LOCK(validator_update, assert(FALSE));

        initialized = TRUE;
      }

    GRAB_SYSTEM_LOCK(trivial_validator.lock);
    ++trivial_validator.reference_count;
    RELEASE_SYSTEM_LOCK(trivial_validator.lock);
    return &trivial_validator;
  }

extern validator *validator_add_validator(validator *base_validator,
                                          validator *to_add)
  {
    validator *new_base;

    ASSERT_IS_GOOD_VALIDATOR(base_validator);
    ASSERT_IS_GOOD_VALIDATOR(to_add);

    if (base_validator->kind == VAK_TRIVIAL)
      {
        validator_remove_reference(base_validator);
        validator_add_reference(to_add);
        return to_add;
      }

    if (to_add->kind == VAK_TRIVIAL)
        return base_validator;

    new_base = validator_add_validator(base_validator, to_add->next);
    if (new_base == NULL)
        return NULL;

    switch (to_add->kind)
      {
        case VAK_TRIVIAL:
            assert(FALSE);
            return NULL;
        case VAK_INSTANCE:
            return validator_add_instance(new_base, to_add->u.instance);
        case VAK_JUMP_TARGET:
            return validator_add_jump_target(new_base, to_add->u.jump_target);
        case VAK_OBJECT:
            return validator_add_object(new_base, to_add->u.object);
        default:
            assert(FALSE);
            return NULL;
      }
  }

extern validator *validator_add_instance(validator *base_validator,
                                         instance *the_instance)
  {
    validator *result;

    ASSERT_IS_GOOD_VALIDATOR(base_validator);
    assert(the_instance != NULL);

    GRAB_SYSTEM_LOCK(validator_update);

    result = create_empty_validator(VAK_INSTANCE, base_validator,
            instance_is_instantiated(the_instance) &&
            !instance_scope_exited(the_instance),
            instance_validator_chain(the_instance));
    if (result == NULL)
      {
        RELEASE_SYSTEM_LOCK(validator_update);
        validator_remove_reference(base_validator);
        return NULL;
      }

    instance_set_validator_chain(the_instance, &(result->chain_element));
    result->u.instance = the_instance;

    RELEASE_SYSTEM_LOCK(validator_update);
    validator_remove_reference(base_validator);

    return result;
  }

extern validator *validator_add_jump_target(validator *base_validator,
                                            jump_target *target)
  {
    validator *result;

    ASSERT_IS_GOOD_VALIDATOR(base_validator);
    assert(target != NULL);

    GRAB_SYSTEM_LOCK(validator_update);

    result = create_empty_validator(VAK_JUMP_TARGET, base_validator,
            !jump_target_scope_exited(target),
            jump_target_validator_chain(target));
    if (result == NULL)
      {
        RELEASE_SYSTEM_LOCK(validator_update);
        validator_remove_reference(base_validator);
        return NULL;
      }

    jump_target_set_validator_chain(target, &(result->chain_element));
    result->u.jump_target = target;

    RELEASE_SYSTEM_LOCK(validator_update);
    validator_remove_reference(base_validator);

    return result;
  }

extern validator *validator_add_object(validator *base_validator,
                                       object *the_object)
  {
    validator *result;

    ASSERT_IS_GOOD_VALIDATOR(base_validator);
    assert(the_object != NULL);

    GRAB_SYSTEM_LOCK(validator_update);

    result = create_empty_validator(VAK_OBJECT, base_validator,
            !object_is_closed(the_object), object_validator_chain(the_object));
    if (result == NULL)
      {
        RELEASE_SYSTEM_LOCK(validator_update);
        validator_remove_reference(base_validator);
        return NULL;
      }

    object_set_validator_chain(the_object, &(result->chain_element));
    result->u.object = the_object;

    RELEASE_SYSTEM_LOCK(validator_update);
    validator_remove_reference(base_validator);

    return result;
  }

extern validator *validator_remove_validator(validator *base_validator,
                                             validator *to_remove)
  {
    validator *new_base;
    validator *result;

    ASSERT_IS_GOOD_VALIDATOR(base_validator);
    ASSERT_IS_GOOD_VALIDATOR(to_remove);

    if (to_remove->kind == VAK_TRIVIAL)
        return base_validator;

    assert(base_validator->kind != VAK_TRIVIAL);

    new_base = validator_remove_validator(base_validator, to_remove->next);
    if (new_base == NULL)
        return NULL;

    GRAB_SYSTEM_LOCK(validator_update);
    result = validator_remove_one_validator(new_base, to_remove);
    return result;
  }

extern boolean validator_is_valid(validator *the_validator)
  {
    ASSERT_IS_GOOD_VALIDATOR(the_validator);

    return the_validator->chain_element.globally_valid;
  }

extern void validator_check_validity(validator *the_validator,
        const source_location *location, jumper *the_jumper)
  {
    ASSERT_IS_GOOD_VALIDATOR(the_validator);
    assert(the_jumper != NULL);

    if (the_validator->chain_element.globally_valid)
        return;

    GRAB_SYSTEM_LOCK(validator_update);
    validator_check_validity_internal(the_validator, location, the_jumper);
  }

extern void instance_check_validity(instance *the_instance,
        const source_location *location, jumper *the_jumper)
  {
    assert(the_instance != NULL);
    assert(the_jumper != NULL);

    if (!(instance_is_instantiated(the_instance)))
        handle_uninstantiated_instance(the_instance, location, the_jumper);
    else if (instance_scope_exited(the_instance))
        handle_scope_exited_instance(the_instance, location, the_jumper);
  }

extern void object_check_validity(object *the_object,
        const source_location *location, jumper *the_jumper)
  {
    assert(the_object != NULL);
    assert(the_jumper != NULL);

    if (object_is_closed(the_object))
        handle_closed_object(the_object, location, the_jumper);
  }

extern void validator_add_reference(validator *the_validator)
  {
    ASSERT_IS_GOOD_VALIDATOR(the_validator);

    GRAB_SYSTEM_LOCK(the_validator->lock);
    assert(the_validator->reference_count > 0);
    ++(the_validator->reference_count);
    RELEASE_SYSTEM_LOCK(the_validator->lock);
  }

extern void validator_remove_reference(validator *the_validator)
  {
    size_t new_reference_count;
    validator_chain *local_next;
    validator_chain *local_previous;
    validator_chain *next_for_base;
    validator_chain *previous_for_base;

    ASSERT_IS_GOOD_VALIDATOR(the_validator);

    GRAB_SYSTEM_LOCK(the_validator->lock);
    assert(the_validator->reference_count > 0);
    --(the_validator->reference_count);
    new_reference_count = the_validator->reference_count;
    RELEASE_SYSTEM_LOCK(the_validator->lock);

    if (new_reference_count > 0)
        return;

    if (the_validator->kind == VAK_TRIVIAL)
      {
        assert(the_validator == &trivial_validator);
        return;
      }

    GRAB_SYSTEM_LOCK(validator_update);

    local_next = the_validator->chain_element.local_next;
    local_previous = the_validator->chain_element.local_previous;

    if (local_next != NULL)
      {
        assert(local_next->local_previous == &(the_validator->chain_element));
        local_next->local_previous = local_previous;
      }

    if (local_previous != NULL)
      {
        assert(local_previous->local_next == &(the_validator->chain_element));
        local_previous->local_next = local_next;
      }
    else
      {
        switch (the_validator->kind)
          {
            case VAK_TRIVIAL:
                assert(FALSE);
                break;
            case VAK_INSTANCE:
                assert(instance_validator_chain(the_validator->u.instance) ==
                       &(the_validator->chain_element));
                instance_set_validator_chain(the_validator->u.instance,
                                             local_next);
                break;
            case VAK_JUMP_TARGET:
                assert(jump_target_validator_chain(
                               the_validator->u.jump_target) ==
                       &(the_validator->chain_element));
                jump_target_set_validator_chain(the_validator->u.jump_target,
                                                local_next);
                break;
            case VAK_OBJECT:
                assert(object_validator_chain(the_validator->u.object) ==
                       &(the_validator->chain_element));
                object_set_validator_chain(the_validator->u.object,
                                           local_next);
                break;
            default:
                assert(FALSE);
          }
      }

    next_for_base = the_validator->chain_element.next_for_base;
    previous_for_base = the_validator->chain_element.previous_for_base;

    if (next_for_base != NULL)
      {
        assert(next_for_base->previous_for_base ==
               &(the_validator->chain_element));
        next_for_base->previous_for_base = previous_for_base;
      }

    if (previous_for_base != NULL)
      {
        assert(previous_for_base->next_for_base ==
               &(the_validator->chain_element));
        previous_for_base->next_for_base = next_for_base;
      }
    else
      {
        assert(the_validator->chain_element.base->users ==
               &(the_validator->chain_element));
        the_validator->chain_element.base->users = next_for_base;
      }

    RELEASE_SYSTEM_LOCK(validator_update);

    assert(the_validator->next != NULL);
    validator_remove_reference(the_validator->next);

    DESTROY_SYSTEM_LOCK(the_validator->lock);

    free(the_validator);
  }

extern void validator_chain_mark_instantiated(validator_chain *chain)
  {
    validator_chain *follow;

    if (chain == NULL)
        return;

    follow = chain;

    GRAB_SYSTEM_LOCK(validator_update);

    while (follow != NULL)
      {
        assert(!(follow->locally_valid));
        follow->locally_valid = TRUE;
        assert(follow->base != NULL);
        if (follow->base->globally_valid)
            mark_chain_globally_valid(follow);
        follow = follow->local_next;
      }

    RELEASE_SYSTEM_LOCK(validator_update);
  }

extern void validator_chain_mark_deallocated(validator_chain *chain)
  {
    validator_chain *follow;

    if (chain == NULL)
        return;

    follow = chain;

    GRAB_SYSTEM_LOCK(validator_update);

    while (follow != NULL)
      {
        assert(follow->locally_valid);
        follow->locally_valid = FALSE;
        mark_chain_globally_invalid(follow);
        follow = follow->local_next;
      }

    RELEASE_SYSTEM_LOCK(validator_update);
  }

extern void print_validator(
        void (*text_out)(void *data, const char *format, ...), void *data,
        validator *the_validator)
  {
    validator *follow;

    assert(text_out != NULL);
    assert(the_validator != NULL);

    (*text_out)(data, "[");

    follow = the_validator;

    while (TRUE)
      {
        assert(follow != NULL);

        if (follow->kind == VAK_TRIVIAL)
          {
            assert(follow->next == NULL);
            break;
          }

        if (follow != the_validator)
            (*text_out)(data, ", ");

        switch (follow->kind)
          {
            case VAK_TRIVIAL:
                assert(FALSE);
            case VAK_INSTANCE:
                (*text_out)(data, "i: ");
                print_instance(text_out, data, follow->u.instance);
                break;
            case VAK_JUMP_TARGET:
                (*text_out)(data, "j: ");
                print_jump_target(text_out, data, FALSE,
                                  follow->u.jump_target);
                break;
            case VAK_OBJECT:
                (*text_out)(data, "o: ");
                print_object(text_out, data, FALSE, follow->u.object);
                break;
            default:
                assert(FALSE);
          }

        follow = follow->next;
        assert(follow != NULL);
      }

    (*text_out)(data, "]");
  }

extern void verify_validators_cleaned_up(void)
  {
    assert(!initialized || (trivial_validator.reference_count == 0));
    DESTROY_SYSTEM_LOCK(trivial_validator.lock);
    DESTROY_SYSTEM_LOCK(validator_update);
  }


static validator *create_empty_validator(validator_kind kind, validator *next,
        boolean locally_valid, validator_chain *local_next)
  {
    validator *result;

    assert(next != NULL);

    if (local_next != NULL)
        assert(local_next->local_previous == NULL);
    if (next->chain_element.users != NULL)
        assert(next->chain_element.users->previous_for_base == NULL);

    result = MALLOC_ONE_OBJECT(validator);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->lock, free(result); return NULL);

    result->chain_element.locally_valid = locally_valid;
    result->chain_element.globally_valid =
            locally_valid && next->chain_element.globally_valid;
    result->chain_element.base = &(next->chain_element);
    result->chain_element.local_next = local_next;
    if (local_next != NULL)
        local_next->local_previous = &(result->chain_element);
    result->chain_element.local_previous = NULL;
    result->chain_element.next_for_base = next->chain_element.users;
    result->chain_element.previous_for_base = NULL;
    if (next->chain_element.users != NULL)
      {
        next->chain_element.users->previous_for_base =
                &(result->chain_element);
      }
    next->chain_element.users = &(result->chain_element);
    result->chain_element.users = NULL;
    result->kind = kind;

    validator_add_reference(next);
    result->next = next;
    result->reference_count = 1;

    return result;
  }

static void mark_chain_globally_invalid(validator_chain *chain)
  {
    validator_chain *follow;

    assert(chain != NULL);

    if (!chain->globally_valid)
        return;

    chain->globally_valid = FALSE;

    follow = chain->users;
    while (follow != NULL)
      {
        mark_chain_globally_invalid(follow);
        follow = follow->next_for_base;
      }
  }

static void mark_chain_globally_valid(validator_chain *chain)
  {
    validator_chain *follow;

    assert(chain != NULL);

    assert(!chain->globally_valid);
    assert(chain->locally_valid);
    assert(chain->base != NULL);
    assert(chain->base->globally_valid);

    chain->globally_valid = TRUE;

    follow = chain->users;
    while (follow != NULL)
      {
        if (follow->locally_valid)
            mark_chain_globally_valid(follow);
        follow = follow->next_for_base;
      }
  }

static validator *validator_remove_one_validator(validator *base_validator,
                                                 validator *to_remove)
  {
    validator *new_base;
    validator *result;

    assert(base_validator != NULL);
    assert(to_remove != NULL);

    assert(base_validator->kind != VAK_TRIVIAL);
    assert(to_remove->kind != VAK_TRIVIAL);

    if (to_remove->kind == base_validator->kind)
      {
        boolean is_match;

        switch (to_remove->kind)
          {
            case VAK_TRIVIAL:
                assert(FALSE);
            case VAK_INSTANCE:
                is_match =
                        (to_remove->u.instance == base_validator->u.instance);
                break;
            case VAK_JUMP_TARGET:
                is_match = (to_remove->u.jump_target ==
                            base_validator->u.jump_target);
                break;
            case VAK_OBJECT:
                is_match = (to_remove->u.object == base_validator->u.object);
                break;
            default:
                assert(FALSE);
          }

        if (is_match)
          {
            validator *result;

            result = base_validator->next;
            RELEASE_SYSTEM_LOCK(validator_update);
            validator_add_reference(result);
            validator_remove_reference(base_validator);
            return result;
          }
      }

    validator_add_reference(base_validator->next);
    new_base = validator_remove_one_validator(base_validator->next, to_remove);
    if (new_base == NULL)
      {
        validator_remove_reference(base_validator);
        return NULL;
      }

    switch (base_validator->kind)
      {
        case VAK_TRIVIAL:
            assert(FALSE);
        case VAK_INSTANCE:
            result = validator_add_instance(new_base,
                                            base_validator->u.instance);
            break;
        case VAK_JUMP_TARGET:
            result = validator_add_jump_target(new_base,
                                               base_validator->u.jump_target);
            break;
        case VAK_OBJECT:
            result = validator_add_object(new_base, base_validator->u.object);
            break;
        default:
            assert(FALSE);
      }

    validator_remove_reference(base_validator);
    return result;
  }

static void handle_uninstantiated_instance(instance *the_instance,
        const source_location *location, jumper *the_jumper)
  {
    static_exception_tag *tag;

    assert(the_instance != NULL);
    assert(the_jumper != NULL);

    switch (instance_kind(the_instance))
      {
        case NK_VARIABLE:
            tag = EXCEPTION_TAG(variable_use_before_instantiation);
            break;
        case NK_ROUTINE:
            tag = EXCEPTION_TAG(routine_use_before_instantiation);
            break;
        case NK_TAGALONG:
            tag = EXCEPTION_TAG(tagalong_use_before_instantiation);
            break;
        case NK_LEPTON_KEY:
            tag = EXCEPTION_TAG(lepton_key_use_before_instantiation);
            break;
        case NK_QUARK:
            tag = EXCEPTION_TAG(quark_use_before_instantiation);
            break;
        case NK_LOCK:
            tag = EXCEPTION_TAG(lock_use_before_instantiation);
            break;
        case NK_JUMP_TARGET:
            assert(FALSE);
        default:
            assert(FALSE);
      }

    location_exception(the_jumper, location, tag,
            "%A was used before it was instantiated by executing its "
            "declaration.", instance_declaration(the_instance));
  }

static void handle_scope_exited_instance(instance *the_instance,
        const source_location *location, jumper *the_jumper)
  {
    static_exception_tag *tag;

    assert(the_instance != NULL);
    assert(the_jumper != NULL);

    switch (instance_kind(the_instance))
      {
        case NK_VARIABLE:
            tag = EXCEPTION_TAG(variable_use_after_deallocation);
            break;
        case NK_ROUTINE:
            tag = EXCEPTION_TAG(routine_use_after_deallocation);
            break;
        case NK_TAGALONG:
            tag = EXCEPTION_TAG(tagalong_use_after_deallocation);
            break;
        case NK_LEPTON_KEY:
            tag = EXCEPTION_TAG(lepton_key_use_after_deallocation);
            break;
        case NK_QUARK:
            tag = EXCEPTION_TAG(quark_use_after_deallocation);
            break;
        case NK_LOCK:
            tag = EXCEPTION_TAG(lock_use_after_deallocation);
            break;
        case NK_JUMP_TARGET:
            assert(FALSE);
        default:
            assert(FALSE);
      }

    location_exception(the_jumper, location, tag,
            "%A was used after it had been deallocated.",
            instance_declaration(the_instance));
  }

static void handle_closed_object(object *the_object,
        const source_location *location, jumper *the_jumper)
  {
    assert(the_object != NULL);
    assert(the_jumper != NULL);

    location_exception(the_jumper, location,
            EXCEPTION_TAG(object_use_after_deallocation),
            "An object was used after it had been deallocated.");
  }

static void validator_check_validity_internal(validator *the_validator,
        const source_location *location, jumper *the_jumper)
  {
    assert(the_validator != NULL);
    assert(the_jumper != NULL);

    if (the_validator->chain_element.globally_valid)
      {
        RELEASE_SYSTEM_LOCK(validator_update);
        return;
      }

    if (the_validator->chain_element.locally_valid)
      {
        assert(the_validator->next != NULL);
        assert(!(the_validator->next->chain_element.globally_valid));
        validator_check_validity_internal(the_validator->next, location,
                                          the_jumper);
      }
    else
      {
        switch (the_validator->kind)
          {
            case VAK_TRIVIAL:
              {
                assert(FALSE);
                break;
              }
            case VAK_INSTANCE:
              {
                instance *the_instance;

                the_instance = the_validator->u.instance;
                assert(the_instance != NULL);

                if (!(instance_is_instantiated(the_instance)))
                  {
                    RELEASE_SYSTEM_LOCK(validator_update);
                    handle_uninstantiated_instance(the_instance, location,
                                                   the_jumper);
                  }
                else
                  {
                    assert(instance_scope_exited(the_instance));
                    RELEASE_SYSTEM_LOCK(validator_update);
                    handle_scope_exited_instance(the_instance, location,
                                                 the_jumper);
                  }

                break;
              }
            case VAK_JUMP_TARGET:
              {
                jump_target *target;

                target = the_validator->u.jump_target;
                assert(target != NULL);

                assert(jump_target_scope_exited(target));
                RELEASE_SYSTEM_LOCK(validator_update);
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(jump_target_use_after_deallocation),
                        "%J was used after it had been deallocated.", target);

                break;
              }
            case VAK_OBJECT:
              {
                object *the_object;

                the_object = the_validator->u.object;
                assert(the_object != NULL);

                assert(object_is_closed(the_object));
                RELEASE_SYSTEM_LOCK(validator_update);
                handle_closed_object(the_object, location, the_jumper);

                break;
              }
            default:
              {
                assert(FALSE);
              }
          }
      }
  }
