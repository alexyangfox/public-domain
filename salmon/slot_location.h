/* file "slot_location.h" */

/*
 *  This file contains the interface to the slot_location module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef SLOT_LOCATION_H
#define SLOT_LOCATION_H


typedef enum
  {
    SLK_VARIABLE,
    SLK_LOOKUP,
    SLK_FIELD,
    SLK_TAGALONG,
    SLK_CALL,
    SLK_PASS
  } slot_location_kind;

typedef struct slot_location slot_location;


#include "variable_instance.h"
#include "lookup_actual_arguments.h"
#include "tagalong_key.h"
#include "source_location.h"
#include "jumper.h"
#include "precedence.h"
#include "validator.h"
#include "reference_cluster.h"


extern slot_location *create_variable_slot_location(
        variable_instance *instance,
        const source_location *the_source_location, jumper *the_jumper);
extern slot_location *create_lookup_slot_location(slot_location *base_slot,
        lookup_actual_arguments *actuals, value *overload_base,
        const source_location *the_source_location, jumper *the_jumper);
extern slot_location *create_field_slot_location(slot_location *base_slot,
        const char *field_name, value *overload_base,
        const source_location *the_source_location, jumper *the_jumper);
extern slot_location *create_tagalong_field_slot_location(
        slot_location *base_slot, tagalong_key *key,
        const source_location *the_source_location, jumper *the_jumper);
extern slot_location *create_call_slot_location(value *base,
        const char *operation_name, value *argument0,
        const char *argument_name0, value *argument1,
        const char *argument_name1, value *overload_base,
        value *argument_for_overload_base,
        const source_location *the_source_location, jumper *the_jumper);
extern slot_location *create_pass_slot_location(slot_location *base_slot,
        value *overload_base, const source_location *the_source_location,
        jumper *the_jumper);

extern void slot_location_add_reference(slot_location *the_slot_location);
extern void slot_location_remove_reference(slot_location *the_slot_location,
                                           jumper *the_jumper);
extern void slot_location_add_reference_with_cluster(
        slot_location *the_slot_location, reference_cluster *cluster);
extern void slot_location_remove_reference_with_cluster(
        slot_location *the_slot_location, reference_cluster *cluster,
        jumper *the_jumper);
extern reference_cluster *slot_location_reference_cluster(
        slot_location *the_slot_location);

extern slot_location_kind get_slot_location_kind(
        slot_location *the_slot_location);
extern variable_instance *variable_slot_location_variable(
        slot_location *the_slot_location);
extern slot_location *lookup_slot_location_base(
        slot_location *the_slot_location);
extern lookup_actual_arguments *lookup_slot_location_actuals(
        slot_location *the_slot_location);
extern slot_location *field_slot_location_base(
        slot_location *the_slot_location);
extern const char *field_slot_location_field_name(
        slot_location *the_slot_location);
extern slot_location *tagalong_slot_location_base(
        slot_location *the_slot_location);
extern tagalong_key *tagalong_slot_location_key(
        slot_location *the_slot_location);
extern value *call_slot_location_base(slot_location *the_slot_location);
extern const char *call_slot_location_operation_name(
        slot_location *the_slot_location);
extern value *call_slot_location_argument0(slot_location *the_slot_location);
extern const char *call_slot_location_argument_name0(
        slot_location *the_slot_location);
extern value *call_slot_location_argument1(slot_location *the_slot_location);
extern const char *call_slot_location_argument_name1(
        slot_location *the_slot_location);
extern value *call_slot_location_argument_for_overload_base(
        slot_location *the_slot_location);
extern slot_location *pass_slot_location_base(
        slot_location *the_slot_location);

extern boolean slot_location_is_valid(slot_location *the_slot_location);
extern void check_slot_location_validity(slot_location *the_slot_location,
        const source_location *location, jumper *the_jumper);
extern validator *slot_location_validator(slot_location *the_slot_location);

extern boolean slot_locations_are_equal(slot_location *slot_location1,
        slot_location *slot_location2, boolean *doubt,
        const source_location *location, jumper *the_jumper);
extern int slot_location_structural_order(slot_location *left,
                                          slot_location *right);
extern boolean slot_location_is_slippery(slot_location *the_slot_location);
extern value *slot_location_overload_base(slot_location *the_slot_location);

extern void slot_location_dereference_type_bounds(
        slot_location *the_slot_location, type **lower_read, type **upper_read,
        type **lower_write, type **upper_write,
        variable_instance **base_variable, const source_location *location,
        jumper *the_jumper);
extern void slot_location_read_type_bounds(slot_location *the_slot_location,
        type **lower, type **upper, const source_location *location,
        jumper *the_jumper);
extern void slot_location_write_type_bounds(slot_location *the_slot_location,
        type **lower, type **upper, variable_instance **base_variable,
        const source_location *location, jumper *the_jumper);

extern void print_slot_location(slot_location *the_slot_location,
        void (*text_printer)(void *data, const char *format, ...), void *data,
        void (*value_printer)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data), expression_parsing_precedence precedence);

DEFINE_EXCEPTION_TAG(tagalong_reference_scope_exited);


#endif /* SLOT_LOCATION_H */
