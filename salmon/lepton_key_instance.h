/* file "lepton_key_instance.h" */

/*
 *  This file contains the interface to the lepton_key_instance module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef LEPTON_KEY_INSTANCE_H
#define LEPTON_KEY_INSTANCE_H

#include "c_foundations/basic.h"


typedef struct lepton_key_instance lepton_key_instance;


#include "lepton_key_declaration.h"
#include "type.h"
#include "jumper.h"
#include "instance.h"
#include "reference_cluster.h"
#include "purity_level.h"


extern lepton_key_instance *create_lepton_key_instance(
        lepton_key_declaration *declaration, purity_level *level,
        reference_cluster *cluster);

extern lepton_key_declaration *lepton_key_instance_declaration(
        lepton_key_instance *instance);
extern boolean lepton_key_instance_is_instantiated(
        lepton_key_instance *instance);
extern boolean lepton_key_instance_scope_exited(lepton_key_instance *instance);
extern type *lepton_key_instance_field_type(lepton_key_instance *instance,
                                            size_t type_num);
extern instance *lepton_key_instance_instance(lepton_key_instance *instance);

extern void set_lepton_key_instance_field_type(lepton_key_instance *instance,
        type *field_type, size_t type_num, jumper *the_jumper);
extern void set_lepton_key_instance_scope_exited(lepton_key_instance *instance,
                                                 jumper *the_jumper);

extern void lepton_key_instance_add_reference(lepton_key_instance *instance);
extern void lepton_key_instance_remove_reference(lepton_key_instance *instance,
                                                 jumper *the_jumper);
extern void lepton_key_instance_add_reference_with_cluster(
        lepton_key_instance *instance, reference_cluster *cluster);
extern void lepton_key_instance_remove_reference_with_cluster(
        lepton_key_instance *instance, jumper *the_jumper,
        reference_cluster *cluster);
extern reference_cluster *lepton_key_instance_reference_cluster(
        lepton_key_instance *instance);
extern boolean lepton_key_instances_are_equal(lepton_key_instance *instance1,
                                              lepton_key_instance *instance2);
extern int lepton_key_instance_structural_order(lepton_key_instance *left,
                                                lepton_key_instance *right);


#endif /* LEPTON_KEY_INSTANCE_H */
