/* file "object.c" */

/*
 *  This file contains the implementation of the object module.
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
#include <string.h>
#include "c_foundations/basic.h"
#include "c_foundations/string_index.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/auto_array.h"
#include "c_foundations/auto_array_implementation.h"
#include "c_foundations/diagnostic.h"
#include "object.h"
#include "routine_instance.h"
#include "routine_instance_chain.h"
#include "statement.h"
#include "execute.h"
#include "lock_chain.h"
#include "reference_cluster.h"
#include "platform_dependent.h"


typedef enum
  {
    FK_VARIABLE,
    FK_ROUTINE,
    FK_ROUTINE_CHAIN,
    FK_TAGALONG,
    FK_LEPTON_KEY,
    FK_QUARK,
    FK_LOCK
  } field_kind;

typedef struct
  {
    char *field_name;
    field_kind kind;
    union
      {
        variable_instance *variable;
        routine_instance *routine;
        routine_instance_chain *routine_chain;
        tagalong_key *tagalong;
        lepton_key_instance *lepton_key;
        quark *quark;
        lock_instance *lock;
      } u;
  } field;

AUTO_ARRAY(statement_aa, statement *);
AUTO_ARRAY(object_field_aa, field);


struct object
  {
    routine_instance *class;
    context *routine_context;
    context *block_context;
    statement_aa cleanups;
    object_field_aa fields;
    string_index *field_index;
    boolean export_enabled;
    lock_chain *lock_chain;
    boolean closed;
    void *hook;
    void (*hook_cleaner)(void *hook, jumper *the_jumper);
    validator_chain *validator_chain;
    object *next_open;
    object *previous_open;
    object *next_open_for_class;
    object *previous_open_for_class;
    boolean is_complete;
    DECLARE_SYSTEM_LOCK(tagalong_lock);
    object_tagalong_handle *tagalong_chain;
    reference_cluster *reference_cluster;
    boolean cluster_is_owned;
    DECLARE_SYSTEM_LOCK(lock);
    size_t reference_count;
    size_t internal_reference_count;
  };

struct object_tagalong_handle
  {
    tagalong_key *key;
    value *field_value;
    object *parent;
    object_tagalong_handle *object_previous;
    object_tagalong_handle *object_next;
    object_tagalong_handle *key_previous;
    object_tagalong_handle *key_next;
  };


static object *all_open = NULL;
DECLARE_SYSTEM_LOCK(object_all_lock);
static boolean is_initialized = FALSE;


AUTO_ARRAY_IMPLEMENTATION(object_field_aa, field, 0);


static void remove_field_references(object *the_object, field *the_field,
                                    jumper *the_jumper);
static field *object_add_field(object *the_object, const char *field_name,
                               jumper *the_jumper);
static void delete_object(object *the_object);


extern verdict init_object_module(void)
  {
    assert(!is_initialized);
    INITIALIZE_SYSTEM_LOCK(object_all_lock, return MISSION_FAILED);
    is_initialized = TRUE;
    return MISSION_ACCOMPLISHED;
  }

extern void cleanup_object_module(void)
  {
    assert(is_initialized);
    is_initialized = FALSE;
    DESTROY_SYSTEM_LOCK(object_all_lock);
  }

extern object *create_object(routine_instance *class, context *routine_context,
        lock_chain *the_lock_chain, reference_cluster *parent_cluster)
  {
    object *result;
    verdict the_verdict;

    assert(is_initialized);
    assert(class != NULL);

    assert(routine_declaration_is_class(routine_instance_declaration(class)));

    result = MALLOC_ONE_OBJECT(object);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->lock, free(result); return NULL);

    INITIALIZE_SYSTEM_LOCK(result->tagalong_lock,
            DESTROY_SYSTEM_LOCK(result->lock);
            free(result);
            return NULL);

    the_verdict = statement_aa_init(&(result->cleanups), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        DESTROY_SYSTEM_LOCK(result->lock);
        DESTROY_SYSTEM_LOCK(result->tagalong_lock);
        free(result);
        return NULL;
      }

    the_verdict = object_field_aa_init(&(result->fields), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->cleanups.array);
        DESTROY_SYSTEM_LOCK(result->lock);
        DESTROY_SYSTEM_LOCK(result->tagalong_lock);
        free(result);
        return NULL;
      }

    result->field_index = create_string_index();
    if (result->field_index == NULL)
      {
        free(result->fields.array);
        free(result->cleanups.array);
        DESTROY_SYSTEM_LOCK(result->lock);
        DESTROY_SYSTEM_LOCK(result->tagalong_lock);
        free(result);
        return NULL;
      }

    if (parent_cluster != NULL)
      {
        result->reference_cluster = parent_cluster;
        result->cluster_is_owned = FALSE;
      }
    else
      {
        result->reference_cluster = create_object_reference_cluster(result);
        if (result->reference_cluster == NULL)
          {
            destroy_string_index(result->field_index);
            free(result->fields.array);
            free(result->cleanups.array);
            DESTROY_SYSTEM_LOCK(result->lock);
            DESTROY_SYSTEM_LOCK(result->tagalong_lock);
            free(result);
            return NULL;
          }
        result->cluster_is_owned = TRUE;
      }

    result->export_enabled = TRUE;

    if (the_lock_chain != NULL)
        lock_chain_add_reference(the_lock_chain);
    result->lock_chain = the_lock_chain;

    routine_instance_add_reference_with_cluster(class, parent_cluster);
    result->class = class;
    result->routine_context = routine_context;
    result->block_context = NULL;
    result->closed = FALSE;
    result->hook = NULL;
    result->hook_cleaner = NULL;
    result->validator_chain = NULL;
    result->previous_open = NULL;
    result->is_complete = FALSE;
    result->tagalong_chain = NULL;
    result->reference_count = 1;
    result->internal_reference_count = 0;

    GRAB_SYSTEM_LOCK(object_all_lock);

    result->next_open = all_open;

    assert((all_open == NULL) || (all_open->previous_open == NULL));
    if (all_open != NULL)
        all_open->previous_open = result;
    all_open = result;
    assert(all_open->previous_open == NULL);

    RELEASE_SYSTEM_LOCK(object_all_lock);

    result->previous_open_for_class = NULL;

    routine_instance_lock_live_instance_list(class);
    result->next_open_for_class = routine_instance_first_live_instance(class);
    if (result->next_open_for_class != NULL)
      {
        assert(result->next_open_for_class->previous_open_for_class == NULL);
        result->next_open_for_class->previous_open_for_class = result;
      }
    routine_instance_set_first_live_instance(class, result);
    routine_instance_unlock_live_instance_list(class);

    return result;
  }

extern void object_add_reference(object *the_object)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    GRAB_SYSTEM_LOCK(the_object->lock);
    assert(the_object->reference_count > 0);
    ++(the_object->reference_count);
    RELEASE_SYSTEM_LOCK(the_object->lock);
  }

extern void object_remove_reference(object *the_object, jumper *the_jumper)
  {
    size_t new_reference_count;
    size_t new_internal_reference_count;

    assert(is_initialized);
    assert(the_object != NULL);
    assert_is_malloced_block(the_object);

    GRAB_SYSTEM_LOCK(the_object->lock);
    assert(the_object->reference_count > 0);
    --(the_object->reference_count);
    new_reference_count = the_object->reference_count;
    new_internal_reference_count = the_object->internal_reference_count;
    RELEASE_SYSTEM_LOCK(the_object->lock);

    if (new_reference_count > 0)
        return;

    if (!(the_object->closed))
      {
        the_object->reference_count = 1;
        close_object(the_object, the_jumper);
        GRAB_SYSTEM_LOCK(the_object->lock);
        assert(the_object->reference_count == 1);
        the_object->reference_count = 0;
        new_internal_reference_count = the_object->internal_reference_count;
        RELEASE_SYSTEM_LOCK(the_object->lock);
      }

    while (the_object->tagalong_chain != NULL)
      {
        object_tagalong_handle *follow;

        follow = the_object->tagalong_chain;
        assert(follow->parent == the_object);

        grab_tagalong_key_object_tagalong_lock(follow->key);

        if (follow->key_next == follow)
          {
            assert(follow->key_previous == follow);
            assert(get_object_tagalong_handle(follow->key) == follow);
            set_object_tagalong_handle(follow->key, NULL, the_jumper);
          }
        else
          {
            assert(follow->key_previous != follow);
            follow->key_previous->key_next = follow->key_next;
            follow->key_next->key_previous = follow->key_previous;
            if (get_object_tagalong_handle(follow->key) == follow)
              {
                set_object_tagalong_handle(follow->key, follow->key_next,
                                           the_jumper);
              }
            else
              {
                release_tagalong_key_object_tagalong_lock(follow->key);
              }
          }

        the_object->tagalong_chain = follow->object_next;

        value_remove_reference(follow->field_value, the_jumper);

        free(follow);
      }

    if ((new_reference_count == 0) && (new_internal_reference_count == 0))
      {
        assert(the_object->validator_chain == NULL);
        delete_object(the_object);
      }
  }

extern void object_add_reference_with_cluster(object *the_object,
                                              reference_cluster *cluster)
  {
    assert(is_initialized);
    assert(the_object != NULL);
    assert_is_malloced_block(the_object);

    if (cluster != the_object->reference_cluster)
        object_add_reference(the_object);
    else if (the_object->cluster_is_owned)
        object_add_internal_reference(the_object);
    else
        object_add_reference(the_object);
  }

extern void object_remove_reference_with_cluster(object *the_object,
        reference_cluster *cluster, jumper *the_jumper)
  {
    assert(is_initialized);
    assert(the_object != NULL);
    assert_is_malloced_block(the_object);

    if (cluster != the_object->reference_cluster)
        object_remove_reference(the_object, the_jumper);
    else if (the_object->cluster_is_owned)
        object_remove_internal_reference(the_object);
    else
        object_remove_reference(the_object, the_jumper);
  }

extern void object_add_internal_reference(object *the_object)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    GRAB_SYSTEM_LOCK(the_object->lock);
    assert((the_object->internal_reference_count > 0) ||
           (the_object->reference_count > 0));
    ++(the_object->internal_reference_count);
    RELEASE_SYSTEM_LOCK(the_object->lock);
  }

extern void object_remove_internal_reference(object *the_object)
  {
    size_t new_reference_count;
    size_t new_internal_reference_count;

    assert(is_initialized);
    assert(the_object != NULL);
    assert_is_malloced_block(the_object);

    GRAB_SYSTEM_LOCK(the_object->lock);
    assert(the_object->internal_reference_count > 0);
    --(the_object->internal_reference_count);
    new_reference_count = the_object->reference_count;
    new_internal_reference_count = the_object->internal_reference_count;
    RELEASE_SYSTEM_LOCK(the_object->lock);

    if ((new_reference_count == 0) && (new_internal_reference_count == 0))
      {
        assert(the_object->validator_chain == NULL);
        delete_object(the_object);
      }
  }

extern void complete_object(object *the_object)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */
    assert(!(the_object->is_complete)); /* VERIFIED */

    the_object->is_complete = TRUE;
  }

extern void close_object(object *the_object, jumper *the_jumper)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    routine_instance_lock_live_instance_list(the_object->class);
    close_object_for_class_exit(the_object, the_jumper);
  }

extern void close_object_for_class_exit(object *the_object, jumper *the_jumper)
  {
    routine_instance *class;
    size_t field_count;
    size_t field_num;

    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    class = the_object->class;
    assert(class != NULL);

    if (the_object->previous_open_for_class == NULL)
      {
        assert(the_object == routine_instance_first_live_instance(class));
        routine_instance_set_first_live_instance(class,
                the_object->next_open_for_class);
      }
    else
      {
        assert(the_object->previous_open_for_class->next_open_for_class ==
               the_object);
        the_object->previous_open_for_class->next_open_for_class =
                the_object->next_open_for_class;
      }
    if (the_object->next_open_for_class != NULL)
      {
        assert(the_object->next_open_for_class->previous_open_for_class ==
               the_object);
        the_object->next_open_for_class->previous_open_for_class =
                the_object->previous_open_for_class;
      }
    routine_instance_unlock_live_instance_list(class);

    object_add_reference(the_object);

    if ((the_jumper != NULL) &&
        ((jumper_flowing_forward(the_jumper)) ||
         (jumper_target(the_jumper) != NULL)))
      {
        size_t cleanup_num;

        cleanup_num = the_object->cleanups.element_count;
        while (cleanup_num > 0)
          {
            statement *cleanup_statement;
            jumper *child_jumper;

            --cleanup_num;

            cleanup_statement = the_object->cleanups.array[cleanup_num];
            assert(cleanup_statement != NULL);

            child_jumper = create_sub_jumper(the_jumper);
            if (child_jumper == NULL)
              {
                jumper_do_abort(the_jumper);
                break;
              }

            execute_statement_block(cleanup_statement_body(cleanup_statement),
                                    the_object->block_context, child_jumper);
            if (!(jumper_flowing_forward(child_jumper)))
              {
                jump_target *target;
                jump_target *parent_target;

                target = jumper_target(child_jumper);
                assert((target == NULL) ||
                       !(jump_target_scope_exited(target))); /* VERIFIED */
                if (target == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    delete_jumper(child_jumper);
                    break;
                  }
                assert(!(jump_target_scope_exited(target))); /* VERIFIED */

                if (jump_target_context(target) == the_object->block_context)
                  {
                    statement_exception(the_jumper, cleanup_statement,
                            EXCEPTION_TAG(cleanup_jump),
                            "A jump was attempted from within a cleanup "
                            "statement to another part of the statement block "
                            "containing that cleanup statement.");
                    delete_jumper(child_jumper);
                    break;
                  }

                parent_target = jumper_target(the_jumper);
                assert((parent_target == NULL) ||
                       !(jump_target_scope_exited(parent_target)));
                        /* VERIFIED */
                assert(!(jump_target_scope_exited(target))); /* VERIFIED */
                if ((parent_target == NULL) ||
                    (jump_target_depth(parent_target) >=
                     jump_target_depth(target)))
                  {
                    assert(!(jump_target_scope_exited(target))); /* VERIFIED */
                    jumper_set_target(the_jumper, target);
                  }
              }

            delete_jumper(child_jumper);
          }
      }

    the_object->closed = TRUE;

    validator_chain_mark_deallocated(the_object->validator_chain);

    if (the_object->block_context != NULL)
        exit_context(the_object->block_context, the_jumper);
    if (the_object->routine_context != NULL)
        exit_context(the_object->routine_context, the_jumper);

    routine_instance_remove_reference_with_cluster(the_object->class,
            (the_object->cluster_is_owned ? NULL :
             the_object->reference_cluster), the_jumper);
    free(the_object->cleanups.array);

    field_count = the_object->fields.element_count;
    for (field_num = 0; field_num < field_count; ++field_num)
      {
        field *this_field;

        this_field = &(the_object->fields.array[field_num]);
        free(this_field->field_name);
        remove_field_references(the_object, this_field, the_jumper);
      }
    free(the_object->fields.array);

    destroy_string_index(the_object->field_index);

    if (the_object->lock_chain != NULL)
        lock_chain_remove_reference(the_object->lock_chain, the_jumper);

    GRAB_SYSTEM_LOCK(object_all_lock);

    assert((all_open == NULL) || (all_open->previous_open == NULL));
    if (the_object->next_open != NULL)
      {
        assert(the_object->next_open->previous_open == the_object);
        the_object->next_open->previous_open = the_object->previous_open;
      }
    if (the_object->previous_open != NULL)
      {
        assert(the_object->previous_open->next_open == the_object);
        the_object->previous_open->next_open = the_object->next_open;
      }
    else
      {
        assert(all_open == the_object);
        all_open = the_object->next_open;
      }
    assert((all_open == NULL) || (all_open->previous_open == NULL));
    assert(the_object != all_open);

    RELEASE_SYSTEM_LOCK(object_all_lock);

    if (the_object->hook_cleaner != NULL)
        (*(the_object->hook_cleaner))(the_object->hook, the_jumper);

    object_remove_reference(the_object, the_jumper);
  }

extern void object_set_validator_chain(object *the_object,
                                       validator_chain *chain)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    the_object->validator_chain = chain;
  }

extern routine_instance *object_class(object *the_object)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    return the_object->class;
  }

extern size_t object_field_lookup(object *the_object, const char *field_name)
  {
    assert(is_initialized);
    assert(the_object != NULL);
    assert(field_name != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    if (!(exists_in_string_index(the_object->field_index, field_name)))
        return the_object->fields.element_count;

    return (size_t)(lookup_in_string_index(the_object->field_index,
                                           field_name));
  }

extern size_t object_field_count(object *the_object)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    return the_object->fields.element_count;
  }

extern const char *object_field_name(object *the_object, size_t field_num)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */
    assert(field_num < the_object->fields.element_count);

    return the_object->fields.array[field_num].field_name;
  }

extern boolean object_field_is_variable(object *the_object, size_t field_num)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */
    assert(field_num < the_object->fields.element_count);

    return (the_object->fields.array[field_num].kind == FK_VARIABLE);
  }

extern boolean object_field_is_routine_chain(object *the_object,
                                             size_t field_num)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */
    assert(field_num < the_object->fields.element_count);

    return (the_object->fields.array[field_num].kind == FK_ROUTINE_CHAIN);
  }

extern variable_instance *object_field_variable(object *the_object,
                                                size_t field_num)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */
    assert(field_num < the_object->fields.element_count);

    assert(the_object->fields.array[field_num].kind == FK_VARIABLE);
    return the_object->fields.array[field_num].u.variable;
  }

extern value *object_field_read_value(object *the_object, size_t field_num,
        const source_location *location, jumper *the_jumper)
  {
    field *the_field;
    value *result;

    assert(is_initialized);
    assert(the_object != NULL);
    assert(the_jumper != NULL);

    assert(!(the_object->closed)); /* VERIFIED */
    assert(field_num < the_object->fields.element_count);

    the_field = (&(the_object->fields.array[field_num]));
    switch (the_field->kind)
      {
        case FK_VARIABLE:
            return read_variable_value(the_field->u.variable, NULL, location,
                                       the_jumper);
        case FK_ROUTINE:
            result = create_routine_value(the_field->u.routine);
            break;
        case FK_ROUTINE_CHAIN:
            result = create_routine_chain_value(the_field->u.routine_chain);
            break;
        case FK_TAGALONG:
            result = create_tagalong_key_value(the_field->u.tagalong);
            break;
        case FK_LEPTON_KEY:
            result = create_lepton_key_value(the_field->u.lepton_key);
            break;
        case FK_QUARK:
            result = create_quark_value(the_field->u.quark);
            break;
        case FK_LOCK:
            result = create_lock_value(the_field->u.lock);
            break;
        default:
            assert(FALSE);
            return NULL;
      }

    if (result == NULL)
        jumper_do_abort(the_jumper);

    return result;
  }

extern instance *object_field_instance(object *the_object, size_t field_num)
  {
    field *the_field;

    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */
    assert(field_num < the_object->fields.element_count);

    the_field = (&(the_object->fields.array[field_num]));
    switch (the_field->kind)
      {
        case FK_VARIABLE:
            return variable_instance_instance(the_field->u.variable);
        case FK_ROUTINE:
            return routine_instance_instance(the_field->u.routine);
        case FK_ROUTINE_CHAIN:
            assert(FALSE);
            return NULL;
        case FK_TAGALONG:
            return tagalong_key_instance(the_field->u.tagalong);
        case FK_LEPTON_KEY:
            return lepton_key_instance_instance(the_field->u.lepton_key);
        case FK_QUARK:
            return quark_instance_instance(the_field->u.quark);
        case FK_LOCK:
            return lock_instance_instance(the_field->u.lock);
        default:
            assert(FALSE);
            return NULL;
      }
  }

extern routine_instance_chain *object_field_routine_chain(object *the_object,
                                                          size_t field_num)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */
    assert(field_num < the_object->fields.element_count);

    assert(the_object->fields.array[field_num].kind == FK_ROUTINE_CHAIN);
    return the_object->fields.array[field_num].u.routine_chain;
  }

extern boolean object_export_enabled(object *the_object)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    return the_object->export_enabled;
  }

extern lock_chain *object_lock_chain(object *the_object)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    return the_object->lock_chain;
  }

extern boolean object_is_closed(object *the_object)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    return the_object->closed;
  }

extern validator_chain *object_validator_chain(object *the_object)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    return the_object->validator_chain;
  }

extern void *object_hook(object *the_object)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    return the_object->hook;
  }

extern boolean object_is_complete(object *the_object)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    return the_object->is_complete;
  }

extern reference_cluster *object_reference_cluster(object *the_object)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    return the_object->reference_cluster;
  }

extern verdict object_append_cleanup(object *the_object,
                                     statement *cleanup_statement)
  {
    assert(is_initialized);
    assert(the_object != NULL);
    assert(cleanup_statement != NULL);

    assert(!(the_object->closed)); /* VERIFIED */
    assert(get_statement_kind(cleanup_statement) == SK_CLEANUP);

    return statement_aa_append(&(the_object->cleanups), cleanup_statement);
  }

extern void object_set_block_context(object *the_object,
                                     context *block_context)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    the_object->block_context = block_context;
  }

extern void object_add_variable_field(object *the_object,
        variable_instance *field_instance, const char *field_name,
        jumper *the_jumper)
  {
    field *the_field;

    assert(is_initialized);
    assert(the_object != NULL);
    assert(field_instance != NULL);
    assert(field_name != NULL);
    assert(the_jumper != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    the_field = object_add_field(the_object, field_name, the_jumper);
    if (the_field == NULL)
        return;

    variable_instance_add_reference_with_cluster(field_instance,
            the_object->reference_cluster);

    the_field->kind = FK_VARIABLE;
    the_field->u.variable = field_instance;
  }

extern void object_add_routine_field(object *the_object,
        routine_instance *field_instance, const char *field_name,
        jumper *the_jumper)
  {
    field *the_field;

    assert(is_initialized);
    assert(the_object != NULL);
    assert(field_instance != NULL);
    assert(field_name != NULL);
    assert(the_jumper != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    if (exists_in_string_index(the_object->field_index, field_name))
      {
        size_t field_num;
        field *the_field;

        field_num = (size_t)(lookup_in_string_index(the_object->field_index,
                                                    field_name));
        assert(field_num < the_object->fields.element_count);

        the_field = &(the_object->fields.array[field_num]);

        assert(strcmp(the_field->field_name, field_name) == 0);

        switch (the_field->kind)
          {
            case FK_ROUTINE:
              {
                routine_instance *old_routine;
                routine_instance_chain *old_chain;
                routine_instance_chain *new_chain;

                old_routine = the_field->u.routine;
                assert(old_routine != NULL);

                old_chain = create_routine_instance_chain(old_routine, NULL);
                if (old_chain == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    return;
                  }

                new_chain = create_routine_instance_chain_with_cluster(
                        field_instance, old_chain,
                        the_object->reference_cluster);
                if (new_chain == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    routine_instance_chain_remove_reference(old_chain,
                                                            the_jumper);
                    return;
                  }

                routine_instance_chain_remove_reference(old_chain, the_jumper);
                routine_instance_remove_reference_with_cluster(old_routine,
                        the_object->reference_cluster, the_jumper);
                the_field->kind = FK_ROUTINE_CHAIN;
                the_field->u.routine_chain = new_chain;
                return;
              }
            case FK_ROUTINE_CHAIN:
              {
                routine_instance_chain *old_chain;
                routine_instance_chain *new_chain;

                old_chain = the_field->u.routine_chain;
                assert(old_chain != NULL);

                new_chain = create_routine_instance_chain_with_cluster(
                        field_instance, old_chain,
                        the_object->reference_cluster);
                if (new_chain == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    return;
                  }

                routine_instance_chain_remove_reference_with_cluster(old_chain,
                        the_object->reference_cluster, the_jumper);
                the_field->u.routine_chain = new_chain;
                return;
              }
            default:
              {
                break;
              }
          }
      }

    assert(!(the_object->closed)); /* VERIFICATION NEEDED */

    the_field = object_add_field(the_object, field_name, the_jumper);
    if (the_field == NULL)
        return;

    routine_instance_add_reference_with_cluster(field_instance,
                                                the_object->reference_cluster);

    the_field->kind = FK_ROUTINE;
    the_field->u.routine = field_instance;
  }

extern void object_add_routine_chain_field(object *the_object,
        routine_instance_chain *field_instance, const char *field_name,
        jumper *the_jumper)
  {
    field *the_field;

    assert(is_initialized);
    assert(the_object != NULL);
    assert(field_instance != NULL);
    assert(field_name != NULL);
    assert(the_jumper != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    if (exists_in_string_index(the_object->field_index, field_name))
      {
        size_t field_num;
        field *the_field;

        field_num = (size_t)(lookup_in_string_index(the_object->field_index,
                                                    field_name));
        assert(field_num < the_object->fields.element_count);

        the_field = &(the_object->fields.array[field_num]);

        assert(strcmp(the_field->field_name, field_name) == 0);

        switch (the_field->kind)
          {
            case FK_ROUTINE:
              {
                routine_instance *old_routine;
                routine_instance_chain *old_chain;
                routine_instance_chain *new_chain;

                old_routine = the_field->u.routine;
                assert(old_routine != NULL);

                old_chain = create_routine_instance_chain(old_routine, NULL);
                if (old_chain == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    return;
                  }

                new_chain = combine_routine_chains(old_chain, field_instance,
                        the_object->reference_cluster);
                if (new_chain == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    routine_instance_chain_remove_reference(old_chain,
                                                            the_jumper);
                    return;
                  }

                routine_instance_chain_remove_reference(old_chain, the_jumper);
                routine_instance_remove_reference_with_cluster(old_routine,
                        the_object->reference_cluster, the_jumper);
                the_field->kind = FK_ROUTINE_CHAIN;
                the_field->u.routine_chain = new_chain;
                return;
              }
            case FK_ROUTINE_CHAIN:
              {
                routine_instance_chain *old_chain;
                routine_instance_chain *new_chain;

                old_chain = the_field->u.routine_chain;
                assert(old_chain != NULL);

                new_chain = combine_routine_chains(old_chain, field_instance,
                        the_object->reference_cluster);
                if (new_chain == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    return;
                  }

                routine_instance_chain_remove_reference_with_cluster(old_chain,
                        the_object->reference_cluster, the_jumper);
                the_field->u.routine_chain = new_chain;
                return;
              }
            default:
              {
                break;
              }
          }
      }

    assert(!(the_object->closed)); /* VERIFICATION NEEDED */

    the_field = object_add_field(the_object, field_name, the_jumper);
    if (the_field == NULL)
        return;

    routine_instance_chain_add_reference_with_cluster(field_instance,
            the_object->reference_cluster);

    the_field->kind = FK_ROUTINE_CHAIN;
    the_field->u.routine_chain = field_instance;
  }

extern void object_add_tagalong_field(object *the_object,
        tagalong_key *field_instance, const char *field_name,
        jumper *the_jumper)
  {
    field *the_field;

    assert(is_initialized);
    assert(the_object != NULL);
    assert(field_instance != NULL);
    assert(field_name != NULL);
    assert(the_jumper != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    the_field = object_add_field(the_object, field_name, the_jumper);
    if (the_field == NULL)
        return;

    tagalong_key_add_reference_with_cluster(field_instance,
                                            the_object->reference_cluster);

    the_field->kind = FK_TAGALONG;
    the_field->u.tagalong = field_instance;
  }

extern void object_add_lepton_key_field(object *the_object,
        lepton_key_instance *field_instance, const char *field_name,
        jumper *the_jumper)
  {
    field *the_field;

    assert(is_initialized);
    assert(the_object != NULL);
    assert(field_instance != NULL);
    assert(field_name != NULL);
    assert(the_jumper != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    the_field = object_add_field(the_object, field_name, the_jumper);
    if (the_field == NULL)
        return;

    lepton_key_instance_add_reference_with_cluster(field_instance,
            the_object->reference_cluster);

    the_field->kind = FK_LEPTON_KEY;
    the_field->u.lepton_key = field_instance;
  }

extern void object_add_quark_field(object *the_object, quark *field_instance,
                                   const char *field_name, jumper *the_jumper)
  {
    field *the_field;

    assert(is_initialized);
    assert(the_object != NULL);
    assert(field_instance != NULL);
    assert(field_name != NULL);
    assert(the_jumper != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    the_field = object_add_field(the_object, field_name, the_jumper);
    if (the_field == NULL)
        return;

    quark_add_reference_with_cluster(field_instance,
                                     the_object->reference_cluster);

    the_field->kind = FK_QUARK;
    the_field->u.quark = field_instance;
  }

extern void object_add_lock_field(object *the_object,
        lock_instance *field_instance, const char *field_name,
        jumper *the_jumper)
  {
    field *the_field;

    assert(is_initialized);
    assert(the_object != NULL);
    assert(field_instance != NULL);
    assert(field_name != NULL);
    assert(the_jumper != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    the_field = object_add_field(the_object, field_name, the_jumper);
    if (the_field == NULL)
        return;

    lock_instance_add_reference_with_cluster(field_instance,
                                             the_object->reference_cluster);

    the_field->kind = FK_LOCK;
    the_field->u.lock = field_instance;
  }

extern void object_remove_field(object *the_object, const char *field_name,
                                jumper *the_jumper)
  {
    size_t field_count;
    size_t field_num;
    field *old_field;

    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    if (!(exists_in_string_index(the_object->field_index, field_name)))
        return;

    field_count = the_object->fields.element_count;
    assert(field_count > 0);
    field_num = (size_t)(lookup_in_string_index(the_object->field_index,
                                                field_name));
    assert(field_num < field_count);

    old_field = &(the_object->fields.array[field_num]);
    free(old_field->field_name);
    remove_field_references(the_object, old_field, the_jumper);

    --field_count;

    if (field_count != field_num)
      {
        verdict the_verdict;

        *old_field = the_object->fields.array[field_count];

        the_verdict = enter_into_string_index(the_object->field_index,
                old_field->field_name, (void *)field_num);
        if (the_verdict != MISSION_ACCOMPLISHED)
            jumper_do_abort(the_jumper);
      }

    remove_from_string_index(the_object->field_index, field_name);

    the_object->fields.element_count = field_count;
  }

extern void object_set_export_mode(object *the_object, boolean export_enabled)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    the_object->export_enabled = export_enabled;
  }

extern void object_set_hook(object *the_object, void *new_hook)
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    the_object->hook = new_hook;
  }

extern void object_set_hook_cleaner(object *the_object,
        void (*cleaner)(void *hook, jumper *the_jumper))
  {
    assert(is_initialized);
    assert(the_object != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    the_object->hook_cleaner = cleaner;
  }

extern value *object_lookup_tagalong(object *the_object, tagalong_key *key)
  {
    object_tagalong_handle *follow;

    assert(is_initialized);
    assert(the_object != NULL);
    assert(key != NULL);

    assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
    assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */

    assert(tagalong_declaration_is_object(tagalong_key_declaration(key)));

    GRAB_SYSTEM_LOCK(the_object->tagalong_lock);

    follow = the_object->tagalong_chain;
    while (follow != NULL)
      {
        assert(follow->parent == the_object);
        if (follow->key == key)
          {
            RELEASE_SYSTEM_LOCK(the_object->tagalong_lock);
            return follow->field_value;
          }

        follow = follow->object_next;
      }

    RELEASE_SYSTEM_LOCK(the_object->tagalong_lock);

    assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
    assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */
    return tagalong_key_default_value(key);
  }

extern void object_set_tagalong(object *the_object, tagalong_key *key,
                                value *new_value, jumper *the_jumper)
  {
    object_tagalong_handle *follow;
    object_tagalong_handle *new;
    object_tagalong_handle *old;

    assert(is_initialized);
    assert(the_object != NULL);
    assert(key != NULL);
    assert(new_value != NULL);

    assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
    assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */

    assert(tagalong_declaration_is_object(tagalong_key_declaration(key)));

    GRAB_SYSTEM_LOCK(the_object->tagalong_lock);

    follow = the_object->tagalong_chain;
    while (follow != NULL)
      {
        assert(follow->parent == the_object);
        if (follow->key == key)
          {
            value *old_value;

            assert(follow->field_value != NULL);
            value_add_reference(new_value);
            old_value = follow->field_value;
            follow->field_value = new_value;
            RELEASE_SYSTEM_LOCK(the_object->tagalong_lock);
            value_remove_reference(old_value, the_jumper);
            return;
          }

        follow = follow->object_next;
      }

    new = MALLOC_ONE_OBJECT(object_tagalong_handle);
    if (new == NULL)
      {
        RELEASE_SYSTEM_LOCK(the_object->tagalong_lock);
        jumper_do_abort(the_jumper);
        return;
      }

    new->key = key;
    new->parent = the_object;
    new->object_next = the_object->tagalong_chain;
    new->object_previous = NULL;

    grab_tagalong_key_object_tagalong_lock(key);

    old = get_object_tagalong_handle(key);
    if (old == NULL)
      {
        new->key_next = new;
        new->key_previous = new;
        set_object_tagalong_handle(key, new, the_jumper);
        assert((jumper_flowing_forward(the_jumper)));
      }
    else
      {
        new->key_previous = old;
        new->key_next = old->key_next;
        old->key_next->key_previous = new;
        old->key_next = new;
        release_tagalong_key_object_tagalong_lock(key);
      }

    value_add_reference(new_value);
    new->field_value = new_value;

    the_object->tagalong_chain = new;
    if (new->object_next != NULL)
        new->object_next->object_previous = new;

    RELEASE_SYSTEM_LOCK(the_object->tagalong_lock);
  }

extern void kill_object_tagalong(object_tagalong_handle *handle,
                                 jumper *the_jumper)
  {
    object *parent;
    object_tagalong_handle *object_previous;
    object_tagalong_handle *object_next;
    object_tagalong_handle *key_previous;
    object_tagalong_handle *key_next;

    assert(is_initialized);
    assert(handle != NULL);

    assert(handle->field_value != NULL);

    parent = handle->parent;
    assert(parent != NULL);

    GRAB_SYSTEM_LOCK(parent->tagalong_lock);

    object_previous = handle->object_previous;
    object_next = handle->object_next;

    if (object_previous != NULL)
      {
        object_previous->object_next = object_next;
      }
    else
      {
        assert(parent->tagalong_chain == handle);
        parent->tagalong_chain = object_next;
      }

    if (object_next != NULL)
        object_next->object_previous = object_previous;

    key_previous = handle->key_previous;
    key_next = handle->key_next;

    grab_tagalong_key_object_tagalong_lock(handle->key);

    if (key_next == handle)
      {
        assert(key_previous == handle);
        assert(get_object_tagalong_handle(handle->key) == handle);
        set_object_tagalong_handle(handle->key, NULL, the_jumper);
      }
    else
      {
        assert(key_previous != handle);
        key_previous->key_next = key_next;
        key_next->key_previous = key_previous;
        if (get_object_tagalong_handle(handle->key) == handle)
            set_object_tagalong_handle(handle->key, key_next, the_jumper);
        else
            release_tagalong_key_object_tagalong_lock(handle->key);
      }

    RELEASE_SYSTEM_LOCK(parent->tagalong_lock);

    value_remove_reference(handle->field_value, the_jumper);

    free(handle);
  }

extern boolean objects_are_equal(object *object1, object *object2)
  {
    assert(object1 != NULL);
    assert(object2 != NULL);

    assert(is_initialized);
    assert(!(object1->closed)); /* VERIFIED */
    assert(!(object2->closed)); /* VERIFIED */

    return (object1 == object2);
  }

extern int object_structural_order(object *left, object *right)
  {
    assert(left != NULL);
    assert(right != NULL);

    assert(is_initialized);
    assert(!(left->closed)); /* VERIFIED */
    assert(!(right->closed)); /* VERIFIED */

    if (left == right)
        return 0;
    else if (left < right)
        return -1;
    else
        return 1;
  }

extern void cleanup_leaked_objects(boolean print_summary,
                                   boolean print_details)
  {
    assert(is_initialized);

    GRAB_SYSTEM_LOCK(object_all_lock);

    if ((all_open != NULL) && (print_summary || print_details))
      {
        object *follow;
        size_t count;

        follow = all_open;
        count = 0;
        while (follow != NULL)
          {
            if (print_details)
                location_notice(NULL, "%B was leaked.", follow);
            follow = follow->next_open;
            ++count;
          }

        if (print_summary)
          {
            location_notice(NULL, "%lu object%s leaked.", (unsigned long)count,
                            ((count == 1) ? " was" : "s were"));
          }
      }

    while (all_open != NULL)
      {
        object *to_release;

        to_release = all_open;
        object_add_reference(to_release);

        RELEASE_SYSTEM_LOCK(object_all_lock);

        close_object(to_release, NULL);
        object_remove_reference(to_release, NULL);

        GRAB_SYSTEM_LOCK(object_all_lock);
      }

    RELEASE_SYSTEM_LOCK(object_all_lock);
  }


static void remove_field_references(object *the_object, field *the_field,
                                    jumper *the_jumper)
  {
    assert(the_object != NULL);
    assert(the_field != NULL);

    switch(the_field->kind)
      {
        case FK_VARIABLE:
            variable_instance_remove_reference_with_cluster(
                    the_field->u.variable, the_jumper,
                    the_object->reference_cluster);
            break;
        case FK_ROUTINE:
            routine_instance_remove_reference_with_cluster(
                    the_field->u.routine, the_object->reference_cluster,
                    the_jumper);
            break;
        case FK_ROUTINE_CHAIN:
            routine_instance_chain_remove_reference_with_cluster(
                    the_field->u.routine_chain, the_object->reference_cluster,
                    the_jumper);
            break;
        case FK_TAGALONG:
            tagalong_key_remove_reference_with_cluster(the_field->u.tagalong,
                    the_jumper, the_object->reference_cluster);
            break;
        case FK_LEPTON_KEY:
            lepton_key_instance_remove_reference_with_cluster(
                    the_field->u.lepton_key, the_jumper,
                    the_object->reference_cluster);
            break;
        case FK_QUARK:
            quark_remove_reference_with_cluster(the_field->u.quark, the_jumper,
                                                the_object->reference_cluster);
            break;
        case FK_LOCK:
            lock_instance_remove_reference_with_cluster(the_field->u.lock,
                    the_jumper, the_object->reference_cluster);
            break;
        default:
            assert(FALSE);
      }
  }

static field *object_add_field(object *the_object, const char *field_name,
                               jumper *the_jumper)
  {
    char *name_copy;
    field new_field;
    size_t field_num;
    verdict the_verdict;

    assert(the_object != NULL);
    assert(field_name != NULL);
    assert(the_jumper != NULL);

    assert(!(the_object->closed)); /* VERIFIED */

    if (exists_in_string_index(the_object->field_index, field_name))
      {
        size_t field_num;
        field *the_field;

        field_num = (size_t)(lookup_in_string_index(the_object->field_index,
                                                    field_name));
        assert(field_num < the_object->fields.element_count);

        the_field = &(the_object->fields.array[field_num]);

        assert(strcmp(the_field->field_name, field_name) == 0);

        remove_field_references(the_object, the_field, the_jumper);

        return the_field;
      }

    name_copy = MALLOC_ARRAY(char, strlen(field_name) + 1);
    if (name_copy == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    strcpy(name_copy, field_name);

    new_field.field_name = name_copy;

    field_num = the_object->fields.element_count;

    the_verdict = object_field_aa_append(&(the_object->fields), new_field);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        free(name_copy);
        return NULL;
      }

    the_verdict = enter_into_string_index(the_object->field_index, field_name,
            (void *)field_num);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        --(the_object->fields.element_count);
        free(name_copy);
        return NULL;
      }

    return &(the_object->fields.array[field_num]);
  }

static void delete_object(object *the_object)
  {
    assert(the_object != NULL);

    assert(the_object->closed);
    assert(the_object->validator_chain == NULL);
    assert(the_object->tagalong_chain == NULL);

    if (the_object->cluster_is_owned)
        delete_reference_cluster(the_object->reference_cluster);

    GRAB_SYSTEM_LOCK(object_all_lock);

    assert(the_object != all_open);
    assert((all_open == NULL) || (all_open->previous_open == NULL));
    DESTROY_SYSTEM_LOCK(the_object->lock);
    DESTROY_SYSTEM_LOCK(the_object->tagalong_lock);
    free(the_object);
    assert((all_open == NULL) || (all_open->previous_open == NULL));

    RELEASE_SYSTEM_LOCK(object_all_lock);
  }
