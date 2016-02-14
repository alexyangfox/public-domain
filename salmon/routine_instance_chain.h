/* file "routine_instance_chain.h" */

/*
 *  This file contains the interface to the routine_instance_chain module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef ROUTINE_INSTANCE_CHAIN_H
#define ROUTINE_INSTANCE_CHAIN_H


typedef struct routine_instance_chain routine_instance_chain;

typedef enum
  {
    PPK_EXACT,
    PPK_ANY,
    PPK_TYPE,
  } parameter_pattern_kind;


#include "semi_labeled_value_list.h"
#include "routine_instance.h"
#include "type.h"
#include "jumper.h"
#include "source_location.h"
#include "reference_cluster.h"
#include "validator.h"


extern routine_instance *resolve_overloading(
        routine_instance_chain *instance_chain,
        semi_labeled_value_list *arguments, const source_location *location,
        jumper *the_jumper);
extern boolean routine_instance_fits_actuals(
        routine_instance *the_routine_instance,
        semi_labeled_value_list *arguments, const source_location *location,
        jumper *the_jumper);
extern boolean routine_instance_fits_pattern(
        routine_instance *the_routine_instance, size_t parameter_count,
        parameter_pattern_kind *parameter_pattern_kinds,
        const char **parameter_names, value **exact_parameters,
        type **parameter_lower_types, type **parameter_upper_types,
        type **result_parameter_types, boolean *doubt,
        const source_location *location, jumper *the_jumper);

extern void get_overloading_information(routine_instance_chain *instance_chain,
        size_t argument_count, type **argument_types,
        const char **argument_names, type **return_type,
        boolean *always_resolves, boolean *never_resolves,
        boolean *always_pure, routine_instance **only_possible_resolution,
        const source_location *location, jumper *the_jumper);

extern routine_instance_chain *create_routine_instance_chain(
        routine_instance *head, routine_instance_chain *tail);
extern routine_instance_chain *create_routine_instance_chain_with_cluster(
        routine_instance *head, routine_instance_chain *tail,
        reference_cluster *cluster);

extern void routine_instance_chain_add_reference(
        routine_instance_chain *instance_chain);
extern void routine_instance_chain_remove_reference(
        routine_instance_chain *instance_chain, jumper *the_jumper);

extern void routine_instance_chain_add_reference_with_cluster(
        routine_instance_chain *instance_chain, reference_cluster *cluster);
extern void routine_instance_chain_remove_reference_with_cluster(
        routine_instance_chain *instance_chain, reference_cluster *cluster,
        jumper *the_jumper);

extern reference_cluster *routine_instance_chain_reference_cluster(
        routine_instance_chain *instance_chain);

extern routine_instance *routine_instance_chain_instance(
        routine_instance_chain *instance_chain);
extern routine_instance_chain *routine_instance_chain_next(
        routine_instance_chain *instance_chain);

extern routine_instance_chain *combine_routine_chains(
        routine_instance_chain *back, routine_instance_chain *front,
        reference_cluster *cluster);

extern boolean routine_instance_chain_is_valid(
        routine_instance_chain *instance_chain);
extern void check_routine_instance_chain_validity(
        routine_instance_chain *instance_chain,
        const source_location *location, jumper *the_jumper);
extern validator *routine_instance_chain_validator(
        routine_instance_chain *instance_chain);

extern boolean routine_instance_chains_are_equal(
        routine_instance_chain *chain1, routine_instance_chain *chain2);
extern int routine_instance_chain_structural_order(
        routine_instance_chain *left, routine_instance_chain *right);


DEFINE_EXCEPTION_TAG(overloading_resolution_uninstantiated);
DEFINE_EXCEPTION_TAG(overloading_resolution_deallocated);
DEFINE_EXCEPTION_TAG(overloading_resolution_indeterminate);


#endif /* ROUTINE_INSTANCE_CHAIN_H */
