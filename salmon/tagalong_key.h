/* file "tagalong_key.h" */

/*
 *  This file contains the interface to the tagalong_key module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef TAGALONG_KEY_H
#define TAGALONG_KEY_H


typedef struct tagalong_key tagalong_key;


#include "tagalong_declaration.h"
#include "value.h"
#include "type.h"
#include "lock_chain.h"
#include "instance.h"
#include "object.h"
#include "purity_level.h"
#include "reference_cluster.h"


extern tagalong_key *create_tagalong_key(tagalong_declaration *declaration,
        purity_level *level, reference_cluster *cluster);

extern tagalong_declaration *tagalong_key_declaration(tagalong_key *key);
extern boolean tagalong_key_is_instantiated(tagalong_key *key);
extern boolean tagalong_key_scope_exited(tagalong_key *key);
extern instance *tagalong_key_instance(tagalong_key *key);

extern void grab_tagalong_key_value_tagalong_lock(tagalong_key *key);
extern void release_tagalong_key_value_tagalong_lock(tagalong_key *key);
extern value_tagalong_handle *get_value_tagalong_handle(tagalong_key *key);
extern void set_value_tagalong_handle(tagalong_key *key,
        value_tagalong_handle *handle, jumper *the_jumper);

extern void grab_tagalong_key_object_tagalong_lock(tagalong_key *key);
extern void release_tagalong_key_object_tagalong_lock(tagalong_key *key);
extern object_tagalong_handle *get_object_tagalong_handle(tagalong_key *key);
extern void set_object_tagalong_handle(tagalong_key *key,
        object_tagalong_handle *handle, jumper *the_jumper);

extern value *tagalong_key_default_value(tagalong_key *key);
extern lock_chain *tagalong_key_lock_chain(tagalong_key *key);

extern type *tagalong_key_type(tagalong_key *key);
extern type *tagalong_key_on_type(tagalong_key *key);

extern void set_tagalong_key_type(tagalong_key *key, type *the_type,
                                  jumper *the_jumper);
extern void set_tagalong_key_on_type(tagalong_key *key, type *on_type,
                                     jumper *the_jumper);
extern void set_tagalong_key_default_value(tagalong_key *key,
        value *default_value, jumper *the_jumper);
extern void set_tagalong_key_lock_chain(tagalong_key *key,
        lock_chain *the_lock_chain, jumper *the_jumper);
extern void set_tagalong_key_scope_exited(tagalong_key *key,
                                          jumper *the_jumper);

extern void tagalong_key_add_reference(tagalong_key *key);
extern void tagalong_key_remove_reference(tagalong_key *key,
                                          jumper *the_jumper);
extern void tagalong_key_add_reference_with_cluster(tagalong_key *key,
        reference_cluster *cluster);
extern void tagalong_key_remove_reference_with_cluster(tagalong_key *key,
        jumper *the_jumper, reference_cluster *cluster);
extern reference_cluster *tagalong_key_reference_cluster(tagalong_key *key);

extern boolean tagalong_keys_are_equal(tagalong_key *key1, tagalong_key *key2);
extern int tagalong_key_structural_order(tagalong_key *left,
                                         tagalong_key *right);


#endif /* TAGALONG_KEY_H */
