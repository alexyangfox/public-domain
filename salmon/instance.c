/* file "instance.c" */

/*
 *  This file contains the implementation of the instance module.
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
#include "instance.h"
#include "variable_instance.h"
#include "routine_instance.h"
#include "tagalong_key.h"
#include "lepton_key_instance.h"
#include "quark.h"
#include "lock_instance.h"
#include "declaration_list.h"
#include "jumper.h"
#include "validator.h"
#include "purity_level.h"
#include "reference_cluster.h"
#include "platform_dependent.h"


struct instance
  {
    name_kind kind;
    boolean is_instantiated;
    union
      {
        variable_instance *variable;
        routine_instance *routine;
        tagalong_key *tagalong;
        lepton_key_instance *lepton_key;
        quark *quark;
        lock_instance *lock;
      } u;
    validator_chain *validator_chain;
    purity_level *purity_level;
    instance *next_live;
    instance *previous_live;
  };


DECLARE_SYSTEM_LOCK(live_lock);
static instance *live_instances = NULL;


static instance *create_empty_instance(name_kind kind, purity_level *level);
static const char *name_kind_output_name(name_kind kind);
static void print_raw_pointer(
        void (*printer)(void *data, const char *format, ...), void *data,
        void *pointer);


extern instance *create_instance_for_variable(
        variable_instance *the_variable_instance, purity_level *level)
  {
    instance *result;

    assert(the_variable_instance != NULL);
    assert(level != NULL);

    result = create_empty_instance(NK_VARIABLE, level);
    if (result == NULL)
        return NULL;

    result->u.variable = the_variable_instance;

    return result;
  }

extern instance *create_instance_for_routine(
        routine_instance *the_routine_instance, purity_level *level)
  {
    instance *result;

    assert(the_routine_instance != NULL);
    assert(level != NULL);

    result = create_empty_instance(NK_ROUTINE, level);
    if (result == NULL)
        return NULL;

    result->u.routine = the_routine_instance;

    return result;
  }

extern instance *create_instance_for_tagalong(
        tagalong_key *the_tagalong_instance, purity_level *level)
  {
    instance *result;

    assert(the_tagalong_instance != NULL);
    assert(level != NULL);

    result = create_empty_instance(NK_TAGALONG, level);
    if (result == NULL)
        return NULL;

    result->u.tagalong = the_tagalong_instance;

    return result;
  }

extern instance *create_instance_for_lepton_key(
        lepton_key_instance *the_lepton_key_instance, purity_level *level)
  {
    instance *result;

    assert(the_lepton_key_instance != NULL);
    assert(level != NULL);

    result = create_empty_instance(NK_LEPTON_KEY, level);
    if (result == NULL)
        return NULL;

    result->u.lepton_key = the_lepton_key_instance;

    return result;
  }

extern instance *create_instance_for_quark(quark *the_quark_instance,
                                           purity_level *level)
  {
    instance *result;

    assert(the_quark_instance != NULL);
    assert(level != NULL);

    result = create_empty_instance(NK_QUARK, level);
    if (result == NULL)
        return NULL;

    result->u.quark = the_quark_instance;

    return result;
  }

extern instance *create_instance_for_lock(lock_instance *the_lock_instance,
                                          purity_level *level)
  {
    instance *result;

    assert(the_lock_instance != NULL);

    result = create_empty_instance(NK_LOCK, level);
    if (result == NULL)
        return NULL;

    result->u.lock = the_lock_instance;

    return result;
  }

extern void delete_instance(instance *the_instance)
  {
    assert(the_instance != NULL);
    assert_is_malloced_block_with_exact_size(the_instance, sizeof(instance));

    assert(the_instance->validator_chain == NULL);

    GRAB_SYSTEM_LOCK(live_lock);

    assert((live_instances == NULL) ||
           (live_instances->previous_live == NULL));
    if (the_instance->next_live != NULL)
      {
        assert(the_instance->next_live->previous_live == the_instance);
        the_instance->next_live->previous_live = the_instance->previous_live;
      }
    if (the_instance->previous_live != NULL)
      {
        assert(the_instance->previous_live->next_live == the_instance);
        the_instance->previous_live->next_live = the_instance->next_live;
      }
    else
      {
        if (live_instances == the_instance)
            live_instances = the_instance->next_live;
      }
    assert((live_instances == NULL) ||
           (live_instances->previous_live == NULL));

    RELEASE_SYSTEM_LOCK(live_lock);

    free(the_instance);
  }

extern name_kind instance_kind(instance *the_instance)
  {
    assert(the_instance != NULL);

    return the_instance->kind;
  }

extern variable_instance *instance_variable_instance(instance *the_instance)
  {
    assert(the_instance != NULL);

    assert(the_instance->kind == NK_VARIABLE);
    return the_instance->u.variable;
  }

extern routine_instance *instance_routine_instance(instance *the_instance)
  {
    assert(the_instance != NULL);

    assert(the_instance->kind == NK_ROUTINE);
    return the_instance->u.routine;
  }

extern tagalong_key *instance_tagalong_instance(instance *the_instance)
  {
    assert(the_instance != NULL);

    assert(the_instance->kind == NK_TAGALONG);
    return the_instance->u.tagalong;
  }

extern lepton_key_instance *instance_lepton_key_instance(
        instance *the_instance)
  {
    assert(the_instance != NULL);

    assert(the_instance->kind == NK_LEPTON_KEY);
    return the_instance->u.lepton_key;
  }

extern quark *instance_quark_instance(instance *the_instance)
  {
    assert(the_instance != NULL);

    assert(the_instance->kind == NK_QUARK);
    return the_instance->u.quark;
  }

extern lock_instance *instance_lock_instance(instance *the_instance)
  {
    assert(the_instance != NULL);

    assert(the_instance->kind == NK_LOCK);
    return the_instance->u.lock;
  }

extern declaration *instance_declaration(instance *the_instance)
  {
    assert(the_instance != NULL);

    switch (instance_kind(the_instance))
      {
        case NK_VARIABLE:
            return variable_declaration_declaration(
                    variable_instance_declaration(
                            instance_variable_instance(the_instance)));
        case NK_ROUTINE:
            return routine_declaration_declaration(
                    routine_instance_declaration(
                            instance_routine_instance(the_instance)));
        case NK_TAGALONG:
            return tagalong_declaration_declaration(
                    tagalong_key_declaration(
                            instance_tagalong_instance(the_instance)));
        case NK_LEPTON_KEY:
            return lepton_key_declaration_declaration(
                    lepton_key_instance_declaration(
                            instance_lepton_key_instance(the_instance)));
        case NK_QUARK:
            return quark_declaration_declaration(
                    quark_instance_declaration(
                            instance_quark_instance(the_instance)));
        case NK_LOCK:
            return lock_declaration_declaration(
                    lock_instance_declaration(
                            instance_lock_instance(the_instance)));
        default:
            assert(FALSE);
            return NULL;
      }
  }

extern boolean instance_is_instantiated(instance *the_instance)
  {
    assert(the_instance != NULL);

    return the_instance->is_instantiated;
  }

extern boolean instance_scope_exited(instance *the_instance)
  {
    assert(the_instance != NULL);

    switch (instance_kind(the_instance))
      {
        case NK_VARIABLE:
            return variable_instance_scope_exited(
                    instance_variable_instance(the_instance));
        case NK_ROUTINE:
            return routine_instance_scope_exited(
                    instance_routine_instance(the_instance));
        case NK_TAGALONG:
            return tagalong_key_scope_exited(
                    instance_tagalong_instance(the_instance));
        case NK_LEPTON_KEY:
            return lepton_key_instance_scope_exited(
                    instance_lepton_key_instance(the_instance));
        case NK_QUARK:
            return quark_scope_exited(instance_quark_instance(the_instance));
        case NK_LOCK:
            return lock_instance_scope_exited(
                    instance_lock_instance(the_instance));
        default:
            assert(FALSE);
            return FALSE;
      }
  }

extern validator_chain *instance_validator_chain(instance *the_instance)
  {
    assert(the_instance != NULL);

    return the_instance->validator_chain;
  }

extern instance *create_instance_for_declaration(declaration *the_declaration,
        purity_level *level, reference_cluster *cluster)
  {
    assert(the_declaration != NULL);
    assert(level != NULL);

    switch (declaration_kind(the_declaration))
      {
        case NK_VARIABLE:
          {
            variable_instance *the_variable_instance;

            the_variable_instance = create_variable_instance(
                    declaration_variable_declaration(the_declaration), level,
                    cluster);
            if (the_variable_instance == NULL)
                return NULL;

            return variable_instance_instance(the_variable_instance);
          }
        case NK_ROUTINE:
          {
            routine_instance *the_routine_instance;

            the_routine_instance = create_routine_instance(
                    declaration_routine_declaration(the_declaration), level,
                    cluster);
            if (the_routine_instance == NULL)
                return NULL;

            return routine_instance_instance(the_routine_instance);
          }
        case NK_TAGALONG:
          {
            tagalong_key *the_tagalong_key_instance;

            the_tagalong_key_instance = create_tagalong_key(
                    declaration_tagalong_declaration(the_declaration), level,
                    cluster);
            if (the_tagalong_key_instance == NULL)
                return NULL;

            return tagalong_key_instance(the_tagalong_key_instance);
          }
        case NK_LEPTON_KEY:
          {
            lepton_key_instance *the_lepton_key_instance;

            the_lepton_key_instance = create_lepton_key_instance(
                    declaration_lepton_key_declaration(the_declaration), level,
                    cluster);
            if (the_lepton_key_instance == NULL)
                return NULL;

            return lepton_key_instance_instance(the_lepton_key_instance);
          }
        case NK_QUARK:
          {
            quark *the_quark_instance;

            the_quark_instance = create_quark(
                    declaration_quark_declaration(the_declaration), level,
                    cluster);
            if (the_quark_instance == NULL)
                return NULL;

            return quark_instance_instance(the_quark_instance);
          }
        case NK_LOCK:
          {
            lock_instance *the_lock_instance;

            the_lock_instance = create_lock_instance(
                    declaration_lock_declaration(the_declaration), level,
                    cluster);
            if (the_lock_instance == NULL)
                return NULL;

            return lock_instance_instance(the_lock_instance);
          }
        default:
          {
            assert(FALSE);
            return NULL;
          }
      }
  }

extern void instance_set_validator_chain(instance *the_instance,
                                         validator_chain *chain)
  {
    assert(the_instance != NULL);

    the_instance->validator_chain = chain;
  }

extern void instance_set_purity_level(instance *the_instance,
                                      purity_level *level)
  {
    assert(the_instance != NULL);

    the_instance->purity_level = level;
  }

extern void instance_add_reference(instance *the_instance)
  {
    instance_add_reference_with_cluster(the_instance, NULL);
  }

extern void instance_remove_reference(instance *the_instance,
                                      jumper *the_jumper)
  {
    instance_remove_reference_with_cluster(the_instance, NULL, the_jumper);
  }

extern void instance_add_reference_with_cluster(instance *the_instance,
                                                reference_cluster *cluster)
  {
    assert(the_instance != NULL);

    switch (instance_kind(the_instance))
      {
        case NK_VARIABLE:
            variable_instance_add_reference_with_cluster(
                    instance_variable_instance(the_instance), cluster);
            break;
        case NK_ROUTINE:
            routine_instance_add_reference_with_cluster(
                    instance_routine_instance(the_instance), cluster);
            break;
        case NK_TAGALONG:
            tagalong_key_add_reference_with_cluster(
                    instance_tagalong_instance(the_instance), cluster);
            break;
        case NK_LEPTON_KEY:
            lepton_key_instance_add_reference_with_cluster(
                    instance_lepton_key_instance(the_instance), cluster);
            break;
        case NK_QUARK:
            quark_add_reference_with_cluster(
                    instance_quark_instance(the_instance), cluster);
            break;
        case NK_LOCK:
            lock_instance_add_reference_with_cluster(
                    instance_lock_instance(the_instance), cluster);
            break;
        default:
            assert(FALSE);
      }
  }

extern void instance_remove_reference_with_cluster(instance *the_instance,
        reference_cluster *cluster, jumper *the_jumper)
  {
    assert(the_instance != NULL);

    switch (instance_kind(the_instance))
      {
        case NK_VARIABLE:
            variable_instance_remove_reference_with_cluster(
                    instance_variable_instance(the_instance), the_jumper,
                    cluster);
            break;
        case NK_ROUTINE:
            routine_instance_remove_reference_with_cluster(
                    instance_routine_instance(the_instance), cluster,
                    the_jumper);
            break;
        case NK_TAGALONG:
            tagalong_key_remove_reference_with_cluster(
                    instance_tagalong_instance(the_instance), the_jumper,
                    cluster);
            break;
        case NK_LEPTON_KEY:
            lepton_key_instance_remove_reference_with_cluster(
                    instance_lepton_key_instance(the_instance), the_jumper,
                    cluster);
            break;
        case NK_QUARK:
            quark_remove_reference_with_cluster(
                    instance_quark_instance(the_instance), the_jumper,
                    cluster);
            break;
        case NK_LOCK:
            lock_instance_remove_reference_with_cluster(
                    instance_lock_instance(the_instance), the_jumper, cluster);
            break;
        default:
            assert(FALSE);
      }
  }

extern void set_instance_instantiated(instance *the_instance)
  {
    assert(the_instance != NULL);

    assert(!(the_instance->is_instantiated));
    the_instance->is_instantiated = TRUE;
    validator_chain_mark_instantiated(the_instance->validator_chain);
  }

extern void set_instance_scope_exited(instance *the_instance,
                                      jumper *the_jumper)
  {
    assert(the_instance != NULL);

    assert(!(instance_scope_exited(the_instance))); /* VERIFIED */

    switch (instance_kind(the_instance))
      {
        case NK_VARIABLE:
            assert(!(variable_instance_scope_exited(
                             instance_variable_instance(the_instance))));
                    /* VERIFIED */
            set_variable_instance_scope_exited(
                    instance_variable_instance(the_instance), the_jumper);
            break;
        case NK_ROUTINE:
            routine_instance_set_scope_exited(
                    instance_routine_instance(the_instance), the_jumper);
            break;
        case NK_TAGALONG:
            assert(!(tagalong_key_scope_exited(
                             instance_tagalong_instance(the_instance))));
                    /* VERIFIED */
            set_tagalong_key_scope_exited(
                    instance_tagalong_instance(the_instance), the_jumper);
            break;
        case NK_LEPTON_KEY:
            assert(!(lepton_key_instance_scope_exited(
                             instance_lepton_key_instance(the_instance))));
                    /* VERIFIED */
            set_lepton_key_instance_scope_exited(
                    instance_lepton_key_instance(the_instance), the_jumper);
            break;
        case NK_QUARK:
            assert(!(quark_scope_exited(
                             instance_quark_instance(the_instance))));
                    /* VERIFIED */
            set_quark_scope_exited(instance_quark_instance(the_instance));
            break;
        case NK_LOCK:
            assert(!(lock_instance_scope_exited(
                             instance_lock_instance(the_instance))));
                    /* VERIFIED */
            set_lock_instance_scope_exited(
                    instance_lock_instance(the_instance), the_jumper);
            break;
        default:
            assert(FALSE);
      }
  }

extern void mark_instance_scope_exited(instance *the_instance)
  {
    assert(the_instance != NULL);

    GRAB_SYSTEM_LOCK(live_lock);

    assert((live_instances == NULL) ||
           (live_instances->previous_live == NULL));
    if (the_instance->next_live != NULL)
      {
        assert(the_instance->next_live->previous_live == the_instance);
        the_instance->next_live->previous_live = the_instance->previous_live;
      }
    if (the_instance->previous_live != NULL)
      {
        assert(the_instance->previous_live->next_live == the_instance);
        the_instance->previous_live->next_live = the_instance->next_live;
      }
    else
      {
        assert(live_instances == the_instance);
        live_instances = the_instance->next_live;
      }
    the_instance->next_live = NULL;
    the_instance->previous_live = NULL;
    assert((live_instances == NULL) ||
           (live_instances->previous_live == NULL));

    RELEASE_SYSTEM_LOCK(live_lock);

    if (the_instance->is_instantiated)
        validator_chain_mark_deallocated(the_instance->validator_chain);
  }

extern void print_instance(
        void (*printer)(void *data, const char *format, ...), void *data,
        instance *the_instance)
  {
    declaration *the_declaration;

    assert(printer != NULL);
    assert(the_instance != NULL);

    the_declaration = instance_declaration(the_instance);
    assert(the_declaration != NULL);

    if (declaration_name(the_declaration) != NULL)
      {
        (*printer)(data, "%s", declaration_name(the_declaration));
      }
    else
      {
        (*printer)(data, "%s_", name_kind_output_name(the_instance->kind));
        print_raw_pointer(printer, data, the_instance);
      }
  }

extern verdict init_instance_module(void)
  {
    INITIALIZE_SYSTEM_LOCK(live_lock, return MISSION_FAILED);
    return MISSION_ACCOMPLISHED;
  }

extern void cleanup_leaked_instances(boolean print_summary,
                                     boolean print_details)
  {
    if ((live_instances != NULL) && (print_summary || print_details))
      {
        instance *follow;
        size_t count;

        follow = live_instances;
        count = 0;
        while (follow != NULL)
          {
            if (print_details)
              {
                location_notice(NULL, "An instance of %a was leaked.",
                                instance_declaration(follow));
              }
            follow = follow->next_live;
            ++count;
          }

        if (print_summary)
          {
            location_notice(NULL, "%lu instance%s leaked.",
                    (unsigned long)count, ((count == 1) ? " was" : "s were"));
          }
      }

    while (live_instances != NULL)
      {
        instance *follow;

        follow = live_instances;
        while (declaration_automatic_allocation(instance_declaration(follow)))
          {
            follow = follow->next_live;
            assert(follow != NULL);
          }
        set_instance_scope_exited(follow, NULL);
      }
  }

extern void cleanup_instance_module(void)
  {
    DESTROY_SYSTEM_LOCK(live_lock);
  }


static instance *create_empty_instance(name_kind kind, purity_level *level)
  {
    instance *result;

    result = MALLOC_ONE_OBJECT(instance);
    if (result == NULL)
        return NULL;

    result->kind = kind;
    result->is_instantiated = FALSE;
    result->validator_chain = NULL;
    result->purity_level = level;

    GRAB_SYSTEM_LOCK(live_lock);

    assert((live_instances == NULL) ||
           (live_instances->previous_live == NULL));
    result->next_live = live_instances;
    result->previous_live = NULL;
    if (live_instances != NULL)
      {
        assert(live_instances->previous_live == NULL);
        live_instances->previous_live = result;
      }
    live_instances = result;
    assert((live_instances == NULL) ||
           (live_instances->previous_live == NULL));

    RELEASE_SYSTEM_LOCK(live_lock);

    return result;
  }

static const char *name_kind_output_name(name_kind kind)
  {
    switch (kind)
      {
        case NK_VARIABLE:
            return "variable";
        case NK_ROUTINE:
            return "routine";
        case NK_TAGALONG:
            return "tagalong_key";
        case NK_LEPTON_KEY:
            return "lepton_key";
        case NK_QUARK:
            return "quark";
        case NK_LOCK:
            return "lock";
        default:
            assert(FALSE);
            return NULL;
      }
  }

static void print_raw_pointer(
        void (*printer)(void *data, const char *format, ...), void *data,
        void *pointer)
  {
    union
      {
        void *pointer;
        unsigned char chars[sizeof(pointer)];
      } u;
    size_t byte_num;

    (*printer)(data, "0x", pointer);

    u.pointer = pointer;
    for (byte_num = 0; byte_num < sizeof(pointer); ++byte_num)
        (*printer)(data, "%02x", (unsigned)(u.chars[byte_num]));
  }
