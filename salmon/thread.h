/* file "thread.h" */

/*
 *  This file contains the interface to the thread module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef THREAD_H
#define THREAD_H

#include "c_foundations/basic.h"


typedef struct salmon_thread salmon_thread;
typedef struct watcher_handle watcher_handle;


#include "jumper.h"
#include "source_location.h"
#include "statement.h"
#include "expression.h"
#include "lock_instance.h"


typedef void statement_execution_watcher_type(void *data,
        statement *the_statement, jumper *the_jumper);
typedef void expression_evaluation_watcher_type(void *data,
        expression *the_expression, jumper *the_jumper);


extern verdict init_thread_module(void);
extern void cleanup_thread_module(void);
extern salmon_thread *create_salmon_thread(const char *name,
                                           void *back_end_thread_data);
extern void set_salmon_thread_stack_bottom(salmon_thread *the_thread,
                                           void *bottom);
extern void set_salmon_thread_stack_size_limit(salmon_thread *the_thread,
                                               size_t limit);
extern size_t salmon_thread_stack_usage(salmon_thread *the_thread);
extern boolean salmon_thread_have_stack_limit(salmon_thread *the_thread);
extern size_t salmon_thread_stack_limit(salmon_thread *the_thread);
extern void *get_root_thread_back_end_data(void);
extern const char *salmon_thread_name(salmon_thread *the_thread);
extern void *salmon_thread_back_end_data(salmon_thread *the_thread);
extern boolean salmon_interpreter_is_thread_safe(void);
extern void wait_for_all_threads_to_finish(void);
extern void block(salmon_thread *blocked, salmon_thread *blocker,
        lock_instance *salmon_lock, jumper *the_jumper,
        const source_location *location);
extern void unblock(salmon_thread *blocked);
extern void self_block(salmon_thread *blocked, jumper *the_jumper,
                       const source_location *location);
extern void self_unblock(salmon_thread *blocked, jumper *the_jumper,
                         const source_location *location);
extern void start_single_threaded(jumper *the_jumper,
                                  const source_location *location);
extern void finish_single_threaded(jumper *the_jumper);

extern void set_salmon_thread_current_jumper(salmon_thread *thread,
                                             jumper *the_jumper);
extern void do_thread_start_statement_execution_watchers(salmon_thread *thread,
        statement *the_statement, jumper *the_jumper);
extern void do_thread_start_expression_evaluation_watchers(
        salmon_thread *thread, expression *the_expression, jumper *the_jumper);
extern watcher_handle *register_start_statement_execution_watcher(
        salmon_thread *thread, statement_execution_watcher_type *new_watcher,
        void *data);
extern void unregister_start_statement_execution_watcher(salmon_thread *thread,
        watcher_handle *handle);
extern watcher_handle *register_start_expression_evaluation_watcher(
        salmon_thread *thread, expression_evaluation_watcher_type *new_watcher,
        void *data);
extern void unregister_start_expression_evaluation_watcher(
        salmon_thread *thread, watcher_handle *handle);

extern void salmon_thread_add_reference(salmon_thread *the_thread);
extern void salmon_thread_remove_reference(salmon_thread *the_thread);

extern void register_root_thread_back_end_data(void *(*finder)(void *data),
                                               void *data);
extern void register_wait_for_all_threads_to_finish(void (*waiter)(void *data),
                                                    void *data);
extern void register_block(
        void (*blocker)(void *data, void *thread_data, jumper *the_jumper,
                        const source_location *location), void *data);
extern void register_unblock(void (*unblocker)(void *data, void *thread_data),
                             void *data);
extern void register_self_block(
        void (*blocker)(void *data, void *thread_data, jumper *the_jumper,
                        const source_location *location), void *data);
extern void register_self_unblock(
        void (*unblocker)(void *data, void *thread_data, jumper *the_jumper,
                          const source_location *location), void *data);
extern void register_start_single_threaded(
        void (*starter)(void *data, jumper *the_jumper,
                        const source_location *location), void *data);
extern void register_finish_single_threaded(
        void (*finisher)(void *data, jumper *the_jumper), void *data);
extern void register_remove_thread_reference(
        void (*handler)(void *data, void *thread_data), void *data);

DEFINE_EXCEPTION_TAG(deadlock);


#endif /* THREAD_H */
