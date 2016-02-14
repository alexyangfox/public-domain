/* file "variable_instance.h" */

/*
 *  This file contains the interface to the variable_instance module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef VARIABLE_INSTANCE_H
#define VARIABLE_INSTANCE_H

#include "c_foundations/basic.h"


typedef struct variable_instance variable_instance;


#include "variable_declaration.h"
#include "type.h"
#include "value.h"
#include "lock_chain.h"
#include "jumper.h"
#include "instance.h"
#include "reference_cluster.h"
#include "purity_level.h"


extern variable_instance *create_variable_instance(
        variable_declaration *declaration, purity_level *level,
        reference_cluster *cluster);

extern variable_declaration *variable_instance_declaration(
        variable_instance *the_variable_instance);
extern boolean variable_instance_is_instantiated(
        variable_instance *the_variable_instance);
extern boolean variable_instance_scope_exited(
        variable_instance *the_variable_instance);
extern type *variable_instance_type(variable_instance *the_variable_instance);
extern value *variable_instance_value(
        variable_instance *the_variable_instance);
extern lock_chain *variable_instance_lock_chain(
        variable_instance *the_variable_instance);
extern instance *variable_instance_instance(
        variable_instance *the_variable_instance);

extern void set_variable_instance_type(
        variable_instance *the_variable_instance, type *the_type,
        jumper *the_jumper);
extern void set_variable_instance_value(
        variable_instance *the_variable_instance, value *the_value,
        jumper *the_jumper);
extern void set_variable_instance_lock_chain(
        variable_instance *the_variable_instance, lock_chain *the_lock_chain,
        jumper *the_jumper);
extern void set_variable_instance_instantiated(
        variable_instance *the_variable_instance);
extern void set_variable_instance_scope_exited(
        variable_instance *the_variable_instance, jumper *the_jumper);

extern void variable_instance_add_reference(
        variable_instance *the_variable_instance);
extern void variable_instance_remove_reference(
        variable_instance *the_variable_instance, jumper *the_jumper);
extern void variable_instance_add_reference_with_cluster(
        variable_instance *the_variable_instance, reference_cluster *cluster);
extern void variable_instance_remove_reference_with_cluster(
        variable_instance *the_variable_instance, jumper *the_jumper,
        reference_cluster *cluster);
extern reference_cluster *variable_instance_reference_cluster(
        variable_instance *the_variable_instance);


#endif /* VARIABLE_INSTANCE_H */
