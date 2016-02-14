/* file "object.h" */

/*
 *  This file contains the interface to the object module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef OBJECT_H
#define OBJECT_H

#include "c_foundations/basic.h"


typedef struct object object;
typedef struct object_tagalong_handle object_tagalong_handle;


#include "routine_instance.h"
#include "statement.h"
#include "context.h"
#include "source_location.h"
#include "variable_instance.h"
#include "routine_instance_chain.h"
#include "tagalong_key.h"
#include "lepton_key_instance.h"
#include "quark.h"
#include "lock_instance.h"
#include "jumper.h"
#include "lock_chain.h"
#include "validator.h"
#include "reference_cluster.h"


extern verdict init_object_module(void);
extern void cleanup_object_module(void);
extern object *create_object(routine_instance *class, context *routine_context,
        lock_chain *the_lock_chain, reference_cluster *parent_cluster);

extern void object_add_reference(object *the_object);
extern void object_remove_reference(object *the_object, jumper *the_jumper);
extern void object_add_reference_with_cluster(object *the_object,
                                              reference_cluster *cluster);
extern void object_remove_reference_with_cluster(object *the_object,
        reference_cluster *cluster, jumper *the_jumper);
extern void object_add_internal_reference(object *the_object);
extern void object_remove_internal_reference(object *the_object);

extern void complete_object(object *the_object);
extern void close_object(object *the_object, jumper *the_jumper);
extern void close_object_for_class_exit(object *the_object,
                                        jumper *the_jumper);
extern void object_set_validator_chain(object *the_object,
                                       validator_chain *chain);

extern routine_instance *object_class(object *the_object);
extern size_t object_field_lookup(object *the_object, const char *field_name);
extern size_t object_field_count(object *the_object);
extern const char *object_field_name(object *the_object, size_t field_num);
extern boolean object_field_is_variable(object *the_object, size_t field_num);
extern boolean object_field_is_routine_chain(object *the_object,
                                             size_t field_num);
extern variable_instance *object_field_variable(object *the_object,
                                                size_t field_num);
extern value *object_field_read_value(object *the_object, size_t field_num,
        const source_location *location, jumper *the_jumper);
extern instance *object_field_instance(object *the_object, size_t field_num);
extern routine_instance_chain *object_field_routine_chain(object *the_object,
                                                          size_t field_num);
extern boolean object_export_enabled(object *the_object);
extern lock_chain *object_lock_chain(object *the_object);
extern boolean object_is_closed(object *the_object);
extern validator_chain *object_validator_chain(object *the_object);
extern void *object_hook(object *the_object);
extern boolean object_is_complete(object *the_object);
extern reference_cluster *object_reference_cluster(object *the_object);

extern verdict object_append_cleanup(object *the_object,
                                     statement *cleanup_statement);
extern void object_set_block_context(object *the_object,
                                     context *block_context);
extern void object_add_variable_field(object *the_object,
        variable_instance *field_instance, const char *field_name,
        jumper *the_jumper);
extern void object_add_routine_field(object *the_object,
        routine_instance *field_instance, const char *field_name,
        jumper *the_jumper);
extern void object_add_routine_chain_field(object *the_object,
        routine_instance_chain *field_instance, const char *field_name,
        jumper *the_jumper);
extern void object_add_tagalong_field(object *the_object,
        tagalong_key *field_instance, const char *field_name,
        jumper *the_jumper);
extern void object_add_lepton_key_field(object *the_object,
        lepton_key_instance *field_instance, const char *field_name,
        jumper *the_jumper);
extern void object_add_quark_field(object *the_object, quark *field_instance,
                                   const char *field_name, jumper *the_jumper);
extern void object_add_lock_field(object *the_object,
        lock_instance *field_instance, const char *field_name,
        jumper *the_jumper);
extern void object_remove_field(object *the_object, const char *field_name,
                                jumper *the_jumper);
extern void object_set_export_mode(object *the_object, boolean export_enabled);
extern void object_set_hook(object *the_object, void *new_hook);
extern void object_set_hook_cleaner(object *the_object,
        void (*cleaner)(void *hook, jumper *the_jumper));

extern value *object_lookup_tagalong(object *the_object, tagalong_key *key);
extern void object_set_tagalong(object *the_object, tagalong_key *key,
                                value *new_value, jumper *the_jumper);

extern void kill_object_tagalong(object_tagalong_handle *handle,
                                 jumper *the_jumper);

extern boolean objects_are_equal(object *object1, object *object2);
extern int object_structural_order(object *left, object *right);

extern void cleanup_leaked_objects(boolean print_summary,
                                   boolean print_details);


#endif /* OBJECT_H */
