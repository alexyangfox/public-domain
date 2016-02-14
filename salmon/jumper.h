/* file "jumper.h" */

/*
 *  This file contains the interface to the jumper module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef JUMPER_H
#define JUMPER_H

#include <stddef.h>
#include "c_foundations/basic.h"
#include "c_foundations/trace.h"


typedef struct jumper jumper;
typedef struct static_exception_tag static_exception_tag;


#define EXCEPTION_TAG(tag)  &exception_tag_ ## tag

#define DEFINE_EXCEPTION_TAG(tag)  \
        extern static_exception_tag exception_tag_ ## tag


#include "jump_target.h"
#include "source_location.h"
#include "expression.h"
#include "statement.h"
#include "type_expression.h"
#include "object.h"
#include "quark.h"
#include "purity_level.h"
#include "thread.h"
#include "type.h"
#include "platform_dependent.h"


struct static_exception_tag
  {
    const char *field_name;
    union
      {
        object *object;
        quark *quark;
      } u;
  };


extern jumper *create_root_jumper(salmon_thread *the_salmon_thread,
                                  tracer *the_tracer);
extern jumper *create_sub_thread_jumper(jumper *parent_jumper,
                                        salmon_thread *the_salmon_thread);
extern jumper *create_sub_jumper(jumper *parent_jumper);
extern jumper *create_purer_jumper(jumper *parent_jumper);
extern jumper *create_test_jumper(void);

extern void delete_jumper(jumper *the_jumper);

extern tracer *jumper_tracer(jumper *the_jumper);
extern object *jumper_standard_library_object(jumper *the_jumper);
extern lepton_key_instance *jumper_region_key(jumper *the_jumper);
extern lepton_key_instance *jumper_exception_key(jumper *the_jumper);
extern purity_level *jumper_purity_level(jumper *the_jumper);
extern salmon_thread *jumper_thread(jumper *the_jumper);

extern size_t jumper_call_stack_size(jumper *the_jumper);
extern routine_instance *jumper_call_stack_callee(jumper *the_jumper,
                                                  size_t call_num);
extern const source_location *jumper_call_stack_site(jumper *the_jumper,
                                                     size_t call_num);
extern CLOCK_T jumper_call_stack_non_local_time(jumper *the_jumper,
                                                size_t call_num);
extern void jumper_call_stack_add_non_local_time(jumper *the_jumper,
        size_t call_num, CLOCK_T to_add);
extern routine_instance *jumper_call_stack_back_callee(jumper *the_jumper,
                                                       size_t call_num);
extern const source_location *jumper_call_stack_back_site(jumper *the_jumper,
                                                          size_t call_num);
extern CLOCK_T jumper_call_stack_back_non_local_time(jumper *the_jumper,
                                                     size_t call_num);
extern void jumper_call_stack_back_add_non_local_time(jumper *the_jumper,
        size_t call_num, CLOCK_T to_add);

extern boolean have_start_statement_execution_watcher(jumper *the_jumper);
extern boolean have_start_expression_evaluation_watcher(jumper *the_jumper);
extern void set_have_start_statement_execution_watcher(jumper *the_jumper,
                                                       boolean have_it);
extern void set_have_start_expression_evaluation_watcher(jumper *the_jumper,
                                                         boolean have_it);

extern void jumper_set_standard_library_object(jumper *the_jumper,
        object *standard_library_object);
extern void jumper_set_region_key(jumper *the_jumper,
                                  lepton_key_instance *region_key);
extern void jumper_set_exception_key(jumper *the_jumper,
                                     lepton_key_instance *exception_key);

extern boolean jumper_flowing_forward(jumper *the_jumper);
extern jump_target *jumper_target(jumper *the_jumper);

extern void location_exception(jumper *the_jumper,
        const source_location *location, static_exception_tag *tag,
        const char *message, ...);
extern void vlocation_exception(jumper *the_jumper,
        const source_location *location, static_exception_tag *tag,
        const char *message, va_list ap);
extern void expression_exception(jumper *the_jumper,
        expression *the_expression, static_exception_tag *tag,
        const char *message, ...);
extern void statement_exception(jumper *the_jumper, statement *the_statement,
        static_exception_tag *tag, const char *message, ...);
extern void type_expression_exception(jumper *the_jumper,
        type_expression *the_type_expression, static_exception_tag *tag,
        const char *message, ...);
extern void jumper_throw_exception(jumper *the_jumper, value *exception_value,
                                   value *other_value);
extern void jumper_throw_exception_with_location(jumper *the_jumper,
        value *exception_value, value *other_value,
        const source_location *location);
extern void jumper_set_target(jumper *the_jumper, jump_target *new_target);
extern void jumper_do_abort(jumper *the_jumper);
extern void jumper_reached_target(jumper *the_jumper);
extern verdict jumper_push_catcher(jumper *the_jumper, jump_target *target,
                                   type *tag_type);
extern void jumper_pop_catcher(jumper *the_jumper, jump_target *catcher);
extern verdict jumper_push_handler(jumper *the_jumper, value *handler);
extern void jumper_pop_handler(jumper *the_jumper, value *handler);
extern boolean jumper_has_unreleased_exception_information(jumper *the_jumper);
extern value *jumper_unreleased_exception_information(jumper *the_jumper);
extern value *jumper_exception_information(jumper *the_jumper);
extern verdict jumper_release_exception_information(jumper *from_jumper,
                                                    jumper *to_jumper);
extern void jumper_clear_exception_information(jumper *the_jumper);
extern void jumper_clear_unreleased_exception_information(jumper *the_jumper);
extern void jumper_transfer_to_parent(jumper *the_jumper);
extern void jumper_push_call_stack(jumper *the_jumper,
        routine_instance *callee, const source_location *call_site);
extern void jumper_pop_call_stack(jumper *the_jumper);

DEFINE_EXCEPTION_TAG(try_catch_tag_match_indeterminate);


#endif /* JUMPER_H */
