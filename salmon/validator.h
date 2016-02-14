/* file "validator.h" */

/*
 *  This file contains the interface to the validator module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef VALIDATOR_H
#define VALIDATOR_H


typedef struct validator validator;
typedef struct validator_chain validator_chain;


#include "instance.h"
#include "jump_target.h"
#include "object.h"
#include "source_location.h"
#include "jumper.h"


extern validator *get_trivial_validator(void);
extern validator *validator_add_validator(validator *base_validator,
                                          validator *to_add);
extern validator *validator_add_instance(validator *base_validator,
                                         instance *the_instance);
extern validator *validator_add_jump_target(validator *base_validator,
                                            jump_target *target);
extern validator *validator_add_object(validator *base_validator,
                                       object *the_object);
extern validator *validator_remove_validator(validator *base_validator,
                                             validator *to_remove);

extern boolean validator_is_valid(validator *the_validator);

extern void validator_check_validity(validator *the_validator,
        const source_location *location, jumper *the_jumper);
extern void instance_check_validity(instance *the_instance,
        const source_location *location, jumper *the_jumper);
extern void object_check_validity(object *the_object,
        const source_location *location, jumper *the_jumper);

extern void validator_add_reference(validator *the_validator);
extern void validator_remove_reference(validator *the_validator);

extern void validator_chain_mark_instantiated(validator_chain *chain);
extern void validator_chain_mark_deallocated(validator_chain *chain);

extern void print_validator(
        void (*text_out)(void *data, const char *format, ...), void *data,
        validator *the_validator);

extern void verify_validators_cleaned_up(void);

DEFINE_EXCEPTION_TAG(variable_use_before_instantiation);
DEFINE_EXCEPTION_TAG(routine_use_before_instantiation);
DEFINE_EXCEPTION_TAG(tagalong_use_before_instantiation);
DEFINE_EXCEPTION_TAG(lepton_key_use_before_instantiation);
DEFINE_EXCEPTION_TAG(quark_use_before_instantiation);
DEFINE_EXCEPTION_TAG(lock_use_before_instantiation);
DEFINE_EXCEPTION_TAG(variable_use_after_deallocation);
DEFINE_EXCEPTION_TAG(routine_use_after_deallocation);
DEFINE_EXCEPTION_TAG(tagalong_use_after_deallocation);
DEFINE_EXCEPTION_TAG(lepton_key_use_after_deallocation);
DEFINE_EXCEPTION_TAG(quark_use_after_deallocation);
DEFINE_EXCEPTION_TAG(lock_use_after_deallocation);
DEFINE_EXCEPTION_TAG(jump_target_use_after_deallocation);
DEFINE_EXCEPTION_TAG(object_use_after_deallocation);


#endif /* VALIDATOR_H */
