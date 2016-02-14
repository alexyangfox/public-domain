/* file "thread.c" */

/*
 *  This file contains the implementation of the thread module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "thread.h"
#include "platform_dependent.h"
#include "jumper.h"


struct watcher_handle
  {
    union
      {
        statement_execution_watcher_type *statement_execution_watcher;
        expression_evaluation_watcher_type *expression_evaluation_watcher;
      } u;
    void *data;
    watcher_handle *next;
    watcher_handle **previous;
  };

struct salmon_thread
  {
    char *name;
    void *back_end_data;
    salmon_thread *blocked_by;
    boolean unblock_pending;
    size_t deadlock_count;
    jumper *current_jumper;
    watcher_handle *start_statement_execution_watchers;
    watcher_handle *next_statement_execution_watcher;
    DECLARE_SYSTEM_LOCK(statement_execution_watcher_lock);
    watcher_handle *start_expression_evaluation_watchers;
    watcher_handle *next_expression_evaluation_watcher;
    DECLARE_SYSTEM_LOCK(expression_evaluation_watcher_lock);
    char *stack_bottom;
    boolean have_stack_limit;
    size_t stack_limit;
    DECLARE_SYSTEM_LOCK(lock);
    size_t reference_count;
  };


static void *(*root_thread_back_end_data_handler)(void *data) = NULL;
static void (*wait_for_all_threads_to_finish_handler)(void *data) = NULL;
static void (*block_handler)(void *data, void *thread_data, jumper *the_jumper,
                             const source_location *location) = NULL;
static void (*unblock_handler)(void *data, void *thread_data) = NULL;
static void (*self_block_handler)(void *data, void *thread_data,
        jumper *the_jumper, const source_location *location) = NULL;
static void (*self_unblock_handler)(void *data, void *thread_data,
        jumper *the_jumper, const source_location *location) = NULL;
static void (*start_single_threaded_handler)(void *data, jumper *the_jumper,
        const source_location *location) = NULL;
static void (*finish_single_threaded_handler)(void *data, jumper *the_jumper) =
        NULL;
static void (*remove_thread_reference_handler)(void *data, void *thread_data) =
        NULL;

static void *root_thread_back_end_data_data = NULL;
static void *wait_for_all_threads_to_finish_data = NULL;
static void *block_data = NULL;
static void *unblock_data = NULL;
static void *self_block_data = NULL;
static void *self_unblock_data = NULL;
static void *start_single_threaded_data = NULL;
static void *finish_single_threaded_data = NULL;
static void *remove_thread_reference_data = NULL;

DECLARE_SYSTEM_LOCK(update_lock);
static boolean initialized = FALSE;


extern verdict init_thread_module(void)
  {
    assert(!initialized);
    INITIALIZE_SYSTEM_LOCK(update_lock, return MISSION_FAILED);
    initialized = TRUE;
    return MISSION_ACCOMPLISHED;
  }

extern void cleanup_thread_module(void)
  {
    assert(initialized);
    initialized = FALSE;
    DESTROY_SYSTEM_LOCK(update_lock);
  }

extern salmon_thread *create_salmon_thread(const char *name,
                                           void *back_end_thread_data)
  {
    salmon_thread *result;
    char *name_copy;

    assert(initialized);
    assert(name != NULL);

    result = MALLOC_ONE_OBJECT(salmon_thread);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->lock, free(result); return NULL);
    INITIALIZE_SYSTEM_LOCK(result->statement_execution_watcher_lock,
            DESTROY_SYSTEM_LOCK(result->lock); free(result); return NULL);
    INITIALIZE_SYSTEM_LOCK(result->expression_evaluation_watcher_lock,
            DESTROY_SYSTEM_LOCK(result->statement_execution_watcher_lock);
            DESTROY_SYSTEM_LOCK(result->lock); free(result); return NULL);

    name_copy = MALLOC_ARRAY(char, strlen(name) + 1);
    if (name_copy == NULL)
      {
        DESTROY_SYSTEM_LOCK(result->expression_evaluation_watcher_lock);
        DESTROY_SYSTEM_LOCK(result->statement_execution_watcher_lock);
        DESTROY_SYSTEM_LOCK(result->lock);
        free(result);
        return NULL;
      }

    strcpy(name_copy, name);

    result->name = name_copy;
    result->back_end_data = back_end_thread_data;
    result->blocked_by = NULL;
    result->unblock_pending = FALSE;
    result->deadlock_count = 0;
    result->current_jumper = NULL;
    result->start_statement_execution_watchers = NULL;
    result->next_statement_execution_watcher = NULL;
    result->start_expression_evaluation_watchers = NULL;
    result->next_expression_evaluation_watcher = NULL;
    result->stack_bottom = (char *)&result;
    result->have_stack_limit = FALSE;
    result->stack_limit = 0;
    result->reference_count = 1;

    return result;
  }

extern void set_salmon_thread_stack_bottom(salmon_thread *the_thread,
                                           void *bottom)
  {
    assert(initialized);
    assert(the_thread != NULL);

    the_thread->stack_bottom = (char *)bottom;
  }

extern void set_salmon_thread_stack_size_limit(salmon_thread *the_thread,
                                               size_t limit)
  {
    assert(initialized);
    assert(the_thread != NULL);

    the_thread->have_stack_limit = TRUE;
    the_thread->stack_limit = limit;
  }

extern size_t salmon_thread_stack_usage(salmon_thread *the_thread)
  {
    char current_stack;

    assert(initialized);
    assert(the_thread != NULL);

    return ((the_thread->stack_bottom <= &current_stack) ?
            (&current_stack - the_thread->stack_bottom) :
            (the_thread->stack_bottom - &current_stack));
  }

extern boolean salmon_thread_have_stack_limit(salmon_thread *the_thread)
  {
    assert(initialized);
    assert(the_thread != NULL);

    return the_thread->have_stack_limit;
  }

extern size_t salmon_thread_stack_limit(salmon_thread *the_thread)
  {
    assert(initialized);
    assert(the_thread != NULL);

    return the_thread->stack_limit;
  }

extern void *get_root_thread_back_end_data(void)
  {
    void *(*handler)(void *data);
    void *data;

    assert(initialized);

    GRAB_SYSTEM_LOCK(update_lock);
    handler = root_thread_back_end_data_handler;
    data = root_thread_back_end_data_data;
    RELEASE_SYSTEM_LOCK(update_lock);

    if (handler != NULL)
        return (*handler)(data);
    else
        return NULL;
  }

extern const char *salmon_thread_name(salmon_thread *the_thread)
  {
    assert(initialized);
    assert(the_thread != NULL);

    return the_thread->name;
  }

extern void *salmon_thread_back_end_data(salmon_thread *the_thread)
  {
    assert(initialized);
    assert(the_thread != NULL);

    return the_thread->back_end_data;
  }

extern boolean salmon_interpreter_is_thread_safe(void)
  {
    assert(initialized);

#ifdef MULTI_THREADED
    return TRUE;
#else /* !MULTI_THREADED */
    return FALSE;
#endif /* !MULTI_THREADED */
  }

extern void wait_for_all_threads_to_finish(void)
  {
    void (*handler)(void *data);
    void *data;

    assert(initialized);

    GRAB_SYSTEM_LOCK(update_lock);
    handler = wait_for_all_threads_to_finish_handler;
    data = wait_for_all_threads_to_finish_data;
    RELEASE_SYSTEM_LOCK(update_lock);

    if (handler != NULL)
        (*handler)(data);
  }

extern void block(salmon_thread *blocked, salmon_thread *blocker,
        lock_instance *salmon_lock, jumper *the_jumper,
        const source_location *location)
  {
    salmon_thread *follow;
    size_t count;
    void (*handler)(void *data, void *thread_data, jumper *the_jumper,
                    const source_location *location);
    void *data;

    assert(initialized);
    assert(blocked != NULL);
    assert(blocker != NULL);

    GRAB_SYSTEM_LOCK(update_lock);

    if (blocked->unblock_pending)
      {
        blocked->unblock_pending = FALSE;
        RELEASE_SYSTEM_LOCK(update_lock);
        return;
      }

    assert(blocked->blocked_by == NULL);

    follow = blocker;
    count = 2;
    do
      {
        if (follow == blocked)
          {
            follow = blocker;
            while (TRUE)
              {
                assert(follow != NULL);
                follow->deadlock_count = count;
                if (follow == blocked)
                  {
                    RELEASE_SYSTEM_LOCK(update_lock);
                    goto deadlock_error;
                  }
                follow = follow->blocked_by;
              }
          }
        follow = follow->blocked_by;
        ++count;
      } while (follow != NULL);

    blocked->blocked_by = blocker;

    handler = block_handler;
    data = block_data;

    RELEASE_SYSTEM_LOCK(update_lock);

    if (handler != NULL)
      {
        (*handler)(data, blocked->back_end_data, the_jumper, location);
        if ((jumper_flowing_forward(the_jumper)) &&
            (blocked->deadlock_count > 0))
          {
          deadlock_error:
            location_exception(the_jumper, location, EXCEPTION_TAG(deadlock),
                    "Thread `%s' deadlocked with %lu other thread%s on %a.",
                    blocked->name,
                    (unsigned long)(blocked->deadlock_count - 1),
                    ((blocked->deadlock_count == 2) ? "" : "s"),
                    instance_declaration(lock_instance_instance(salmon_lock)));
          }
        blocked->deadlock_count = 0;
      }
  }

extern void unblock(salmon_thread *blocked)
  {
    void (*handler)(void *data, void *thread_data);
    void *data;

    assert(initialized);
    assert(blocked != NULL);

    GRAB_SYSTEM_LOCK(update_lock);

    if (blocked->blocked_by == NULL)
      {
        assert(!(blocked->unblock_pending));
        blocked->unblock_pending = TRUE;

        RELEASE_SYSTEM_LOCK(update_lock);

        return;
      }

    assert(blocked->blocked_by != NULL);
    blocked->blocked_by = NULL;

    handler = unblock_handler;
    data = unblock_data;

    RELEASE_SYSTEM_LOCK(update_lock);

    if (handler != NULL)
        (*handler)(data, blocked->back_end_data);
  }

extern void self_block(salmon_thread *blocked, jumper *the_jumper,
                       const source_location *location)
  {
    void (*handler)(void *data, void *thread_data, jumper *the_jumper,
                    const source_location *location);
    void *data;

    assert(initialized);
    assert(blocked != NULL);

    GRAB_SYSTEM_LOCK(update_lock);

    assert(!(blocked->unblock_pending));
    assert(blocked->blocked_by == NULL);

    handler = self_block_handler;
    data = self_block_data;

    RELEASE_SYSTEM_LOCK(update_lock);

    if (handler != NULL)
      {
        (*handler)(data, blocked->back_end_data, the_jumper, location);
        assert(blocked->deadlock_count == 0);
      }
  }

extern void self_unblock(salmon_thread *blocked, jumper *the_jumper,
                         const source_location *location)
  {
    void (*handler)(void *data, void *thread_data, jumper *the_jumper,
                    const source_location *location);
    void *data;

    assert(initialized);
    assert(blocked != NULL);

    GRAB_SYSTEM_LOCK(update_lock);

    assert(blocked->blocked_by == NULL);

    handler = self_unblock_handler;
    data = self_unblock_data;

    RELEASE_SYSTEM_LOCK(update_lock);

    if (handler != NULL)
        (*handler)(data, blocked->back_end_data, the_jumper, location);
  }

extern void start_single_threaded(jumper *the_jumper,
                                  const source_location *location)
  {
    void (*handler)(void *data, jumper *the_jumper,
                    const source_location *location);
    void *data;

    assert(initialized);

    GRAB_SYSTEM_LOCK(update_lock);
    handler = start_single_threaded_handler;
    data = start_single_threaded_data;
    RELEASE_SYSTEM_LOCK(update_lock);

    if (handler != NULL)
        (*handler)(data, the_jumper, location);
  }

extern void finish_single_threaded(jumper *the_jumper)
  {
    void (*handler)(void *data, jumper *the_jumper);
    void *data;

    assert(initialized);

    GRAB_SYSTEM_LOCK(update_lock);
    handler = finish_single_threaded_handler;
    data = finish_single_threaded_data;
    RELEASE_SYSTEM_LOCK(update_lock);

    if (handler != NULL)
        (*handler)(data, the_jumper);
  }

extern void set_salmon_thread_current_jumper(salmon_thread *thread,
                                             jumper *the_jumper)
  {
    assert(thread != NULL);

    GRAB_SYSTEM_LOCK(thread->statement_execution_watcher_lock);
    GRAB_SYSTEM_LOCK(thread->expression_evaluation_watcher_lock);

    thread->current_jumper = the_jumper;

    if (the_jumper != NULL)
      {
        set_have_start_statement_execution_watcher(the_jumper,
                (thread->start_statement_execution_watchers != NULL));
        set_have_start_expression_evaluation_watcher(the_jumper,
                (thread->start_expression_evaluation_watchers != NULL));
      }

    RELEASE_SYSTEM_LOCK(thread->expression_evaluation_watcher_lock);
    RELEASE_SYSTEM_LOCK(thread->statement_execution_watcher_lock);
  }

extern void do_thread_start_statement_execution_watchers(salmon_thread *thread,
        statement *the_statement, jumper *the_jumper)
  {
    assert(thread != NULL);
    assert(the_statement != NULL);
    assert(the_jumper != NULL);

    GRAB_SYSTEM_LOCK(thread->statement_execution_watcher_lock);

    thread->next_statement_execution_watcher =
            thread->start_statement_execution_watchers;
    while (thread->next_statement_execution_watcher != NULL)
      {
        statement_execution_watcher_type *watcher;
        void *data;

        watcher = thread->next_statement_execution_watcher->u.
                statement_execution_watcher;
        data = thread->next_statement_execution_watcher->data;
        thread->next_statement_execution_watcher =
                thread->next_statement_execution_watcher->next;

        RELEASE_SYSTEM_LOCK(thread->statement_execution_watcher_lock);

        (*watcher)(data, the_statement, the_jumper);

        GRAB_SYSTEM_LOCK(thread->statement_execution_watcher_lock);
      }

    RELEASE_SYSTEM_LOCK(thread->statement_execution_watcher_lock);
  }

extern void do_thread_start_expression_evaluation_watchers(
        salmon_thread *thread, expression *the_expression, jumper *the_jumper)
  {
    assert(thread != NULL);
    assert(the_expression != NULL);
    assert(the_jumper != NULL);

    GRAB_SYSTEM_LOCK(thread->expression_evaluation_watcher_lock);

    thread->next_expression_evaluation_watcher =
            thread->start_expression_evaluation_watchers;
    while (thread->next_expression_evaluation_watcher != NULL)
      {
        expression_evaluation_watcher_type *watcher;
        void *data;

        watcher = thread->next_expression_evaluation_watcher->u.
                expression_evaluation_watcher;
        data = thread->next_expression_evaluation_watcher->data;
        thread->next_expression_evaluation_watcher =
                thread->next_expression_evaluation_watcher->next;

        RELEASE_SYSTEM_LOCK(thread->expression_evaluation_watcher_lock);

        (*watcher)(data, the_expression, the_jumper);

        GRAB_SYSTEM_LOCK(thread->expression_evaluation_watcher_lock);
      }

    RELEASE_SYSTEM_LOCK(thread->expression_evaluation_watcher_lock);
  }

extern watcher_handle *register_start_statement_execution_watcher(
        salmon_thread *thread, statement_execution_watcher_type *new_watcher,
        void *data)
  {
    watcher_handle *result;

    assert(thread != NULL);
    assert(new_watcher != NULL);

    result = MALLOC_ONE_OBJECT(watcher_handle);
    if (result == NULL)
        return NULL;

    result->u.statement_execution_watcher = new_watcher;
    result->data = data;

    GRAB_SYSTEM_LOCK(thread->statement_execution_watcher_lock);

    if ((thread->current_jumper != NULL) &&
        (thread->start_statement_execution_watchers == NULL))
      {
        set_have_start_statement_execution_watcher(thread->current_jumper,
                                                   TRUE);
      }

    result->next = thread->start_statement_execution_watchers;
    result->previous = &(thread->start_statement_execution_watchers);
    if (thread->start_statement_execution_watchers != NULL)
        thread->start_statement_execution_watchers->previous = &(result->next);
    thread->start_statement_execution_watchers = result;

    RELEASE_SYSTEM_LOCK(thread->statement_execution_watcher_lock);

    return result;
  }

extern void unregister_start_statement_execution_watcher(salmon_thread *thread,
        watcher_handle *handle)
  {
    assert(thread != NULL);
    assert(handle != NULL);

    GRAB_SYSTEM_LOCK(thread->statement_execution_watcher_lock);

    assert(thread->start_statement_execution_watchers != NULL);
    if (handle->next != NULL)
      {
        assert(handle->next->previous == &(handle->next));
        handle->next->previous = handle->previous;
      }
    *(handle->previous) = handle->next;
    if (thread->next_statement_execution_watcher == handle)
        thread->next_statement_execution_watcher = handle->next;

    if ((thread->current_jumper != NULL) &&
        (thread->start_statement_execution_watchers == NULL))
      {
        set_have_start_statement_execution_watcher(thread->current_jumper,
                                                   FALSE);
      }

    RELEASE_SYSTEM_LOCK(thread->statement_execution_watcher_lock);

    free(handle);
  }

extern watcher_handle *register_start_expression_evaluation_watcher(
        salmon_thread *thread, expression_evaluation_watcher_type *new_watcher,
        void *data)
  {
    watcher_handle *result;

    assert(thread != NULL);
    assert(new_watcher != NULL);

    result = MALLOC_ONE_OBJECT(watcher_handle);
    if (result == NULL)
        return NULL;

    result->u.expression_evaluation_watcher = new_watcher;
    result->data = data;

    GRAB_SYSTEM_LOCK(thread->expression_evaluation_watcher_lock);

    if ((thread->current_jumper != NULL) &&
        (thread->start_expression_evaluation_watchers == NULL))
      {
        set_have_start_expression_evaluation_watcher(thread->current_jumper,
                                                     TRUE);
      }

    result->next = thread->start_expression_evaluation_watchers;
    result->previous = &(thread->start_expression_evaluation_watchers);
    if (thread->start_expression_evaluation_watchers != NULL)
      {
        thread->start_expression_evaluation_watchers->previous =
                &(result->next);
      }
    thread->start_expression_evaluation_watchers = result;

    RELEASE_SYSTEM_LOCK(thread->expression_evaluation_watcher_lock);

    return result;
  }

extern void unregister_start_expression_evaluation_watcher(
        salmon_thread *thread, watcher_handle *handle)
  {
    assert(thread != NULL);
    assert(handle != NULL);

    GRAB_SYSTEM_LOCK(thread->expression_evaluation_watcher_lock);

    assert(thread->start_expression_evaluation_watchers != NULL);
    if (handle->next != NULL)
      {
        assert(handle->next->previous == &(handle->next));
        handle->next->previous = handle->previous;
      }
    *(handle->previous) = handle->next;
    if (thread->next_expression_evaluation_watcher == handle)
        thread->next_expression_evaluation_watcher = handle->next;

    if ((thread->current_jumper != NULL) &&
        (thread->start_expression_evaluation_watchers == NULL))
      {
        set_have_start_expression_evaluation_watcher(thread->current_jumper,
                                                     FALSE);
      }

    RELEASE_SYSTEM_LOCK(thread->expression_evaluation_watcher_lock);

    free(handle);
  }

extern void salmon_thread_add_reference(salmon_thread *the_thread)
  {
    assert(initialized);
    assert(the_thread != NULL);

    GRAB_SYSTEM_LOCK(the_thread->lock);
    assert(the_thread->reference_count > 0);
    ++(the_thread->reference_count);
    RELEASE_SYSTEM_LOCK(the_thread->lock);
  }

extern void salmon_thread_remove_reference(salmon_thread *the_thread)
  {
    size_t new_reference_count;
    void (*handler)(void *data, void *thread_data);
    void *data;
    watcher_handle *follow_watcher;

    assert(initialized);

    GRAB_SYSTEM_LOCK(the_thread->lock);
    assert(the_thread->reference_count > 0);
    --(the_thread->reference_count);
    new_reference_count = the_thread->reference_count;
    RELEASE_SYSTEM_LOCK(the_thread->lock);

    if (new_reference_count > 0)
        return;

    assert(the_thread->name != NULL);
    free(the_thread->name);

    assert(the_thread->blocked_by == NULL);
    assert(!(the_thread->unblock_pending));
    assert(the_thread->deadlock_count == 0);

    GRAB_SYSTEM_LOCK(update_lock);
    handler = remove_thread_reference_handler;
    data = remove_thread_reference_data;
    RELEASE_SYSTEM_LOCK(update_lock);

    if (handler != NULL)
        (*handler)(data, the_thread->back_end_data);

    follow_watcher = the_thread->start_statement_execution_watchers;
    while (follow_watcher != NULL)
      {
        watcher_handle *going;

        going = follow_watcher;
        follow_watcher = follow_watcher->next;
        free(going);
      }

    follow_watcher = the_thread->start_expression_evaluation_watchers;
    while (follow_watcher != NULL)
      {
        watcher_handle *going;

        going = follow_watcher;
        follow_watcher = follow_watcher->next;
        free(going);
      }

    DESTROY_SYSTEM_LOCK(the_thread->expression_evaluation_watcher_lock);
    DESTROY_SYSTEM_LOCK(the_thread->statement_execution_watcher_lock);
    DESTROY_SYSTEM_LOCK(the_thread->lock);
    free(the_thread);
  }

extern void register_root_thread_back_end_data(void *(*finder)(void *data),
                                               void *data)
  {
    assert(initialized);

    GRAB_SYSTEM_LOCK(update_lock);
    root_thread_back_end_data_handler = finder;
    root_thread_back_end_data_data = data;
    RELEASE_SYSTEM_LOCK(update_lock);
  }

extern void register_wait_for_all_threads_to_finish(void (*waiter)(void *data),
                                                    void *data)
  {
    assert(initialized);

    GRAB_SYSTEM_LOCK(update_lock);
    wait_for_all_threads_to_finish_handler = waiter;
    wait_for_all_threads_to_finish_data = data;
    RELEASE_SYSTEM_LOCK(update_lock);
  }

extern void register_block(
        void (*blocker)(void *data, void *thread_data, jumper *the_jumper,
                        const source_location *location), void *data)
  {
    assert(initialized);

    GRAB_SYSTEM_LOCK(update_lock);
    block_handler = blocker;
    block_data = data;
    RELEASE_SYSTEM_LOCK(update_lock);
  }

extern void register_unblock(void (*unblocker)(void *data, void *thread_data),
                             void *data)
  {
    assert(initialized);

    GRAB_SYSTEM_LOCK(update_lock);
    unblock_handler = unblocker;
    unblock_data = data;
    RELEASE_SYSTEM_LOCK(update_lock);
  }

extern void register_self_block(
        void (*blocker)(void *data, void *thread_data, jumper *the_jumper,
                        const source_location *location), void *data)
  {
    assert(initialized);

    GRAB_SYSTEM_LOCK(update_lock);
    self_block_handler = blocker;
    self_block_data = data;
    RELEASE_SYSTEM_LOCK(update_lock);
  }

extern void register_self_unblock(
        void (*unblocker)(void *data, void *thread_data, jumper *the_jumper,
                          const source_location *location), void *data)
  {
    assert(initialized);

    GRAB_SYSTEM_LOCK(update_lock);
    self_unblock_handler = unblocker;
    self_unblock_data = data;
    RELEASE_SYSTEM_LOCK(update_lock);
  }

extern void register_start_single_threaded(
        void (*starter)(void *data, jumper *the_jumper,
                        const source_location *location), void *data)
  {
    assert(initialized);

    GRAB_SYSTEM_LOCK(update_lock);
    start_single_threaded_handler = starter;
    start_single_threaded_data = data;
    RELEASE_SYSTEM_LOCK(update_lock);
  }

extern void register_finish_single_threaded(
        void (*finisher)(void *data, jumper *the_jumper), void *data)
  {
    assert(initialized);

    GRAB_SYSTEM_LOCK(update_lock);
    finish_single_threaded_handler = finisher;
    finish_single_threaded_data = data;
    RELEASE_SYSTEM_LOCK(update_lock);
  }

extern void register_remove_thread_reference(
        void (*handler)(void *data, void *thread_data), void *data)
  {
    assert(initialized);

    GRAB_SYSTEM_LOCK(update_lock);
    remove_thread_reference_handler = handler;
    remove_thread_reference_data = data;
    RELEASE_SYSTEM_LOCK(update_lock);
  }
