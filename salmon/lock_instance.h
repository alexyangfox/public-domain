/* file "lock_instance.h" */

/*
 *  This file contains the interface to the lock_instance module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef LOCK_INSTANCE_H
#define LOCK_INSTANCE_H

#include "c_foundations/basic.h"


typedef struct lock_instance lock_instance;

#include "lock_declaration.h"
#include "jumper.h"
#include "lock_chain.h"
#include "instance.h"
#include "reference_cluster.h"
#include "purity_level.h"


extern lock_instance *create_lock_instance(lock_declaration *declaration,
        purity_level *level, reference_cluster *cluster);

extern lock_declaration *lock_instance_declaration(lock_instance *instance);
extern boolean lock_instance_is_instantiated(lock_instance *instance);
extern boolean lock_instance_scope_exited(lock_instance *instance);
extern instance *lock_instance_instance(lock_instance *instance);

extern void set_lock_instance_lock_chain(lock_instance *instance,
        lock_chain *the_lock_chain, jumper *the_jumper);
extern void set_lock_instance_is_context_switching(lock_instance *instance);
extern void set_lock_instance_scope_exited(lock_instance *instance,
                                           jumper *the_jumper);

extern void lock_instance_grab(lock_instance *instance,
        const source_location *location, jumper *the_jumper);
extern void lock_instance_release(lock_instance *instance,
        const source_location *location, jumper *the_jumper);

extern void lock_instance_add_reference(lock_instance *instance);
extern void lock_instance_remove_reference(lock_instance *instance,
                                           jumper *the_jumper);
extern void lock_instance_add_reference_with_cluster(lock_instance *instance,
        reference_cluster *cluster);
extern void lock_instance_remove_reference_with_cluster(
        lock_instance *instance, jumper *the_jumper,
        reference_cluster *cluster);
extern reference_cluster *lock_instance_reference_cluster(
        lock_instance *instance);

extern boolean lock_instances_are_equal(lock_instance *lock_instance1,
                                        lock_instance *lock_instance2);
extern int lock_instance_structural_order(lock_instance *left,
                                          lock_instance *right);

DEFINE_EXCEPTION_TAG(lock_grab_uninstantiated);
DEFINE_EXCEPTION_TAG(lock_grab_deallocated);
DEFINE_EXCEPTION_TAG(lock_release_uninstantiated);
DEFINE_EXCEPTION_TAG(lock_release_deallocated);
DEFINE_EXCEPTION_TAG(lock_release_not_held);


#endif /* LOCK_INSTANCE_H */
