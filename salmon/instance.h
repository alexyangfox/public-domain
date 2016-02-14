/* file "instance.h" */

/*
 *  This file contains the interface to the instance module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef INSTANCE_H
#define INSTANCE_H

#include "c_foundations/basic.h"


typedef struct instance instance;


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


extern instance *create_instance_for_variable(
        variable_instance *the_variable_instance, purity_level *level);
extern instance *create_instance_for_routine(
        routine_instance *the_routine_instance, purity_level *level);
extern instance *create_instance_for_tagalong(
        tagalong_key *the_tagalong_instance, purity_level *level);
extern instance *create_instance_for_lepton_key(
        lepton_key_instance *the_lepton_key_instance, purity_level *level);
extern instance *create_instance_for_quark(quark *the_quark_instance,
                                           purity_level *level);
extern instance *create_instance_for_lock(lock_instance *the_lock_instance,
                                          purity_level *level);

extern void delete_instance(instance *the_instance);

extern name_kind instance_kind(instance *the_instance);

extern variable_instance *instance_variable_instance(instance *the_instance);
extern routine_instance *instance_routine_instance(instance *the_instance);
extern tagalong_key *instance_tagalong_instance(instance *the_instance);
extern lepton_key_instance *instance_lepton_key_instance(
        instance *the_instance);
extern quark *instance_quark_instance(instance *the_instance);
extern lock_instance *instance_lock_instance(instance *the_instance);

extern declaration *instance_declaration(instance *the_instance);
extern boolean instance_is_instantiated(instance *the_instance);
extern boolean instance_scope_exited(instance *the_instance);
extern validator_chain *instance_validator_chain(instance *the_instance);

extern instance *create_instance_for_declaration(declaration *the_declaration,
        purity_level *level, reference_cluster *cluster);
extern void instance_set_validator_chain(instance *the_instance,
                                         validator_chain *chain);
extern void instance_set_purity_level(instance *the_instance,
                                      purity_level *level);

extern void instance_add_reference(instance *the_instance);
extern void instance_remove_reference(instance *the_instance,
                                      jumper *the_jumper);

extern void instance_add_reference_with_cluster(instance *the_instance,
                                                reference_cluster *cluster);
extern void instance_remove_reference_with_cluster(instance *the_instance,
        reference_cluster *cluster, jumper *the_jumper);

extern void set_instance_instantiated(instance *the_instance);
extern void set_instance_scope_exited(instance *the_instance,
                                      jumper *the_jumper);
extern void mark_instance_scope_exited(instance *the_instance);

extern void print_instance(
        void (*printer)(void *data, const char *format, ...), void *data,
        instance *the_instance);

extern verdict init_instance_module(void);
extern void cleanup_leaked_instances(boolean print_summary,
                                     boolean print_details);
extern void cleanup_instance_module(void);


#endif /* INSTANCE_H */
