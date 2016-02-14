/* file "nb_thread.c" */

/*
 *  This file implements a thread module through the Salmon native bridge
 *  interface using Posix threads for its implementation.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "../c_foundations/memory_allocation.h"
#include "../native_bridge.h"
#include "../source_location.h"
#include "../execute.h"
#include "../o_integer.h"
#include "../jumper.h"
#include "../platform_dependent.h"


#define BILLION_LONG 1000000000L


typedef enum
  {
    TS_RUNNING, TS_FINISHED, TS_BLOCKED, TS_SLEEPING, TS_SUSPENDED
  } thread_status;

typedef enum
  {
    ES_NORMAL, ES_EXCEPTIONS, ES_KILLED, ES_DIED, ES_JUMPED_OUT
  } exit_status;

typedef struct run_mode_forced_action_data run_mode_forced_action_data;
typedef struct thread_info thread_info;

struct run_mode_forced_action_data
  {
    void (*handler)(void *data, thread_info *info, jumper *the_jumper,
                    const source_location *location);
    void *data;
    run_mode_forced_action_data *next;
    run_mode_forced_action_data **previous;
  };

struct thread_info
  {
    char *name;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    boolean is_unblocked;
    boolean terminated;
    pthread_cond_t termination_waiters;
    thread_info *next;
    thread_info **previous;
    boolean living;
    boolean has_external_reference;
    boolean has_object_hook;
    pthread_mutex_t ref_count_mutex;
    size_t high_ref_count;
    size_t low_ref_count;
    object *exception_object;
    jump_target *dummy_child_exception_target;
    object *thread_object;
    reference_cluster *cluster;
    thread_status status;
    pthread_mutex_t status_change_mutex;
    exit_status exit_status;
    value *exit_exception_information;
    jump_target *exit_jump_target;
    void (*awaken_handler)(void *data);
    void *awaken_data;
    pthread_cond_t suspend_condition;
    boolean suspend_coming;
    run_mode_forced_action_data *first_run_mode_forced_action;
    const source_location *last_location;
    salmon_thread *salmon_thread;
    boolean single_threaded_waiting;
    pthread_t pthread_handle;
  };

typedef struct
  {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    o_integer current;
    o_integer limit;
    object *base_object;
  } semaphore_info;

typedef struct
  {
    value *body_value;
    jumper *jumper;
    tracer *tracer;
    const source_location *location;
    thread_info *info;
  } starter_data;

typedef struct
  {
    watcher_handle *statement_handle;
    watcher_handle *expression_handle;
    run_mode_forced_action_data *run_mode_handle;
    void (*handler)(void *data, thread_info *info, jumper *the_jumper,
                    const source_location *location);
    void *data;
    thread_info *info;
    pthread_mutex_t mutex;
  } force_one_call_info;

typedef struct
  {
    value *exception_value;
    value *other_value;
    boolean do_block;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    force_one_call_info force_info;
  } interrupt_info;

typedef struct
  {
    force_one_call_info force_info;
    boolean unblock;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
  } suspend_info;

typedef struct
  {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
  } get_position_info;

typedef struct
  {
    thread_info *single_holder;
    pthread_cond_t others_condition;
    pthread_mutex_t single_mutex;
    pthread_cond_t single_condition;
    size_t waiting_count;
  } single_threaded_data;

typedef struct
  {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
  } resume_info;


static void *thread_starter(void *untyped_data);
static void *root_thread_back_end_data_handler(void *data);
static void wait_for_all_threads_to_finish_handler(void *data);
static void block_handler(void *data, void *thread_data, jumper *the_jumper,
                          const source_location *location);
static void unblock_handler(void *data, void *thread_data);
static void self_block_handler(void *data, void *thread_data,
        jumper *the_jumper, const source_location *location);
static void self_unblock_handler(void *data, void *thread_data,
        jumper *the_jumper, const source_location *location);
static void start_single_threaded_handler(void *data, jumper *the_jumper,
                                          const source_location *location);
static void finish_single_threaded_handler(void *data, jumper *the_jumper);
static void remove_thread_reference_handler(void *data, void *thread_data);
static void thread_info_remove_high_ref(thread_info *info, jumper *the_jumper);
static void thread_info_add_low_ref(thread_info *info);
static void thread_info_remove_low_ref(thread_info *info);
static void do_exception(jumper *the_jumper, const source_location *location,
        object *base_object, const char *tag, const char *format, ...);
static thread_info *thread_info_from_param(value *all_arguments_value,
                                           size_t param_num);
static thread_info *thread_info_from_jumper(jumper *the_jumper);
static void starting_blocked(thread_info *info,
        void (*awaken_handler)(void *data), void *awaken_data,
        jumper *the_jumper, const source_location *location);
static void finished_blocked(thread_info *info, jumper *the_jumper,
                             const source_location *location);
static void activating(void);
static void deactivating(void);
static void not_threadsafe_single_down(void);
static void not_threadsafe_single_up(void);
static verdict split_second_value(value *second_value, boolean *non_positive,
        boolean *plus_infinity, o_integer *whole_seconds, long *nanoseconds);
static void check_for_running_force(thread_info *info, jumper *the_jumper,
                                    const source_location *location);
static void start_force_section(jumper *the_jumper,
                                const source_location *location);
static void finish_force_section(void);
static verdict do_force_one_call(force_one_call_info *force_info,
        thread_info *info,
        void (*handler)(void *data, thread_info *info, jumper *the_jumper,
                        const source_location *location), void *data);
static void force_one_statement_watcher(void *data, statement *the_statement,
                                        jumper *the_jumper);
static void force_one_expression_watcher(void *data,
        expression *the_expression, jumper *the_jumper);
static void force_one_run_mode_watcher(void *data, thread_info *info,
        jumper *the_jumper, const source_location *location);
static void force_one_call_ultimate_handler(force_one_call_info *force_info,
        jumper *the_jumper, const source_location *location);
static void do_interrupt_handler(void *data, thread_info *info,
        jumper *the_jumper, const source_location *location);
static void do_kill_handler(void *data, thread_info *info, jumper *the_jumper,
                            const source_location *location);
static void do_suspend_handler(void *data, thread_info *info,
        jumper *the_jumper, const source_location *location);
static void do_resume_handler(void *data, thread_info *info,
        jumper *the_jumper, const source_location *location);
static void source_position_found_handler(void *data, thread_info *info,
        jumper *the_jumper, const source_location *location);
static void hold_for_single_threading_handler(void *data, thread_info *info,
        jumper *the_jumper, const source_location *location);
static void awaken_termination_waiters(void *data);
static void awaken_semaphore_waiters(void *data);
static void awaken_blocked(void *data);
static void compute_expiration_time(struct timespec *expiration_time,
        boolean *expiration_untimed, value *second_value, object *base_object,
        jumper *the_jumper, boolean *timed_out,
        const char *routine_description);
static void thread_cleaner(void *hook, jumper *the_jumper);
static void semaphore_cleaner(void *hook, jumper *the_jumper);
static void top_cleaner(void *hook, jumper *the_jumper);


static thread_info *living_threads;
static thread_info root_thread =
  {
    "Root Thread",
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_COND_INITIALIZER,
    FALSE,
    FALSE,
    PTHREAD_COND_INITIALIZER,
    NULL,
    &living_threads,
    TRUE,
    TRUE,
    TRUE,
    PTHREAD_MUTEX_INITIALIZER,
    3,
    1,
    NULL,
    NULL,
    NULL,
    NULL,
    TS_RUNNING,
    PTHREAD_MUTEX_INITIALIZER,
    ES_NORMAL,
    NULL,
    NULL,
    NULL,
    NULL,
    PTHREAD_COND_INITIALIZER,
    FALSE,
    NULL,
    NULL,
    NULL,
    FALSE
  };
static thread_info *living_threads = &root_thread;
static thread_info *zombie_threads = NULL;
static size_t living_count = 0;
static pthread_mutex_t module_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t zombie_mutex = PTHREAD_MUTEX_INITIALIZER;
static boolean someone_waiting_for_death = FALSE;
static boolean interpreter_not_threadsafe = FALSE;
static pthread_mutex_t single_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t single_condition = PTHREAD_COND_INITIALIZER;
static size_t single_active_count = 1;
static pthread_cond_t death_watch_condition = PTHREAD_COND_INITIALIZER;
static single_threaded_data the_single_threaded_data =
  {
    NULL,
    PTHREAD_COND_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_COND_INITIALIZER,
    0
  };
static pthread_mutex_t force_section_mutex = PTHREAD_MUTEX_INITIALIZER;
static size_t module_reference_count = 0;
static pthread_mutex_t module_reference_mutex = PTHREAD_MUTEX_INITIALIZER;


extern verdict salmoneye_plugin_initialize(void)
  {
    interpreter_not_threadsafe = !(salmon_interpreter_is_thread_safe());

    register_root_thread_back_end_data(root_thread_back_end_data_handler,
                                       NULL);
    register_wait_for_all_threads_to_finish(
            wait_for_all_threads_to_finish_handler, NULL);
    register_block(block_handler, NULL);
    register_unblock(unblock_handler, NULL);
    register_self_block(self_block_handler, NULL);
    register_self_unblock(self_unblock_handler, NULL);
    register_start_single_threaded(start_single_threaded_handler, NULL);
    register_finish_single_threaded(finish_single_threaded_handler, NULL);
    register_remove_thread_reference(remove_thread_reference_handler, NULL);

    return MISSION_ACCOMPLISHED;
  }

extern value *create_thread_internal_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *this_value;
    thread_info *info;
    starter_data *data;
    const char *name;
    salmon_thread *the_salmon_thread;
    verdict the_verdict;
    value *result;
    object *result_object;
    pthread_attr_t default_attributes;
    int return_code;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 4);

    this_value = value_component_value(all_arguments_value, 2);
    assert(get_value_kind(this_value) == VK_OBJECT);

    info = MALLOC_ONE_OBJECT(thread_info);
    if (info == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    info->is_unblocked = FALSE;
    info->terminated = FALSE;

    info->exception_object = object_value_data(this_value);
    object_add_reference(info->exception_object);

    info->dummy_child_exception_target =
            thread_info_from_jumper(the_jumper)->dummy_child_exception_target;
    jump_target_add_reference(info->dummy_child_exception_target);

    info->status = TS_RUNNING;
    PRE_PTHREAD_MUTEX_INIT(info->status_change_mutex);
    pthread_mutex_init(&(info->status_change_mutex), NULL);
    info->exit_status = ES_NORMAL;
    info->awaken_handler = NULL;
    info->awaken_data = NULL;
    pthread_cond_init(&(info->suspend_condition), NULL);
    info->suspend_coming = FALSE;
    info->first_run_mode_forced_action = NULL;
    info->last_location = location;

    data = MALLOC_ONE_OBJECT(starter_data);
    if (data == NULL)
      {
        jumper_do_abort(the_jumper);
        pthread_mutex_destroy(&(info->status_change_mutex));
        jump_target_remove_reference(info->dummy_child_exception_target);
        object_remove_reference(info->exception_object, the_jumper);
        free(info);
        return NULL;
      }

    data->body_value = value_component_value(all_arguments_value, 1);
    value_add_reference(data->body_value);

    name = string_value_data(value_component_value(all_arguments_value, 3));
    the_salmon_thread = create_salmon_thread(name, info);
    if (the_salmon_thread == NULL)
      {
        jumper_do_abort(the_jumper);
        pthread_mutex_destroy(&(info->status_change_mutex));
        jump_target_remove_reference(info->dummy_child_exception_target);
        object_remove_reference(info->exception_object, the_jumper);
        value_remove_reference(data->body_value, the_jumper);
        free(data);
        free(info);
        return NULL;
      }

    info->salmon_thread = the_salmon_thread;
    info->single_threaded_waiting = FALSE;

    info->name = MALLOC_ARRAY(char, strlen(name) + 1);
    if (info->name == NULL)
      {
        jumper_do_abort(the_jumper);
        salmon_thread_remove_reference(the_salmon_thread);
        pthread_mutex_destroy(&(info->status_change_mutex));
        jump_target_remove_reference(info->dummy_child_exception_target);
        object_remove_reference(info->exception_object, the_jumper);
        value_remove_reference(data->body_value, the_jumper);
        free(data);
        free(info);
        return NULL;
      }

    strcpy(info->name, name);

    data->jumper = create_sub_thread_jumper(the_jumper, the_salmon_thread);
    if (data->jumper == NULL)
      {
        jumper_do_abort(the_jumper);
        free(info->name);
        salmon_thread_remove_reference(the_salmon_thread);
        pthread_mutex_destroy(&(info->status_change_mutex));
        jump_target_remove_reference(info->dummy_child_exception_target);
        object_remove_reference(info->exception_object, the_jumper);
        value_remove_reference(data->body_value, the_jumper);
        free(data);
        free(info);
        return NULL;
      }

    the_verdict = jumper_push_catcher(data->jumper,
            info->dummy_child_exception_target, NULL);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        delete_jumper(data->jumper);
        free(info->name);
        salmon_thread_remove_reference(the_salmon_thread);
        pthread_mutex_destroy(&(info->status_change_mutex));
        jump_target_remove_reference(info->dummy_child_exception_target);
        object_remove_reference(info->exception_object, the_jumper);
        value_remove_reference(data->body_value, the_jumper);
        free(data);
        free(info);
        return NULL;
      }

    data->tracer = jumper_tracer(the_jumper);
    data->location = location;
    data->info = info;

    pthread_mutex_lock(&module_mutex);

    PRE_PTHREAD_MUTEX_INIT(info->mutex);
    pthread_mutex_init(&(info->mutex), NULL);
    pthread_cond_init(&(info->condition), NULL);
    pthread_cond_init(&(info->termination_waiters), NULL);

    PRE_PTHREAD_MUTEX_INIT(info->ref_count_mutex);
    pthread_mutex_init(&(info->ref_count_mutex), NULL);
    info->high_ref_count = 3;
    info->low_ref_count = 1;

    result = value_component_value(all_arguments_value, 0);
    value_add_reference(result);

    assert(get_value_kind(result) == VK_OBJECT);
    result_object = object_value_data(result);

    assert(object_hook(result_object) == NULL);
    object_set_hook(result_object, info);
    object_set_hook_cleaner(result_object, &thread_cleaner);
    info->thread_object = result_object;
    info->cluster = object_reference_cluster(result_object);
    object_add_reference(result_object);

    return_code = pthread_attr_init(&default_attributes);
    if (return_code == 0)
      {
        size_t stack_limit;
        int return_code;

        return_code =
                pthread_attr_getstacksize(&default_attributes, &stack_limit);
        if ((return_code == 0) && (stack_limit > 0))
            set_salmon_thread_stack_size_limit(the_salmon_thread, stack_limit);

        pthread_attr_destroy(&default_attributes);
      }

    return_code = pthread_create(&(info->pthread_handle), NULL, thread_starter,
                                 data);
    if (return_code != 0)
      {
        pthread_mutex_unlock(&module_mutex);
        do_exception(the_jumper, jumper_call_stack_back_site(the_jumper, 1),
                info->exception_object, "et_thread_creation_failed",
                "Failed trying to create a thread: %s.",
                strerror(return_code));
        object_remove_reference(result_object, the_jumper);
        pthread_mutex_destroy(&(info->mutex));
        pthread_cond_destroy(&(info->condition));
        pthread_cond_destroy(&(info->termination_waiters));
        pthread_mutex_destroy(&(info->ref_count_mutex));
        jumper_pop_catcher(data->jumper, info->dummy_child_exception_target);
        delete_jumper(data->jumper);
        free(info->name);
        salmon_thread_remove_reference(the_salmon_thread);
        pthread_mutex_destroy(&(info->status_change_mutex));
        jump_target_remove_reference(info->dummy_child_exception_target);
        object_remove_reference(info->exception_object, the_jumper);
        value_remove_reference(data->body_value, the_jumper);
        free(data);
        free(info);
        return NULL;
      }

    info->next = living_threads;
    info->previous = &living_threads;
    if (living_threads != NULL)
      {
        assert(living_threads->previous == &living_threads);
        living_threads->previous = &(info->next);
      }
    living_threads = info;
    info->living = TRUE;
    info->has_external_reference = TRUE;
    info->has_object_hook = TRUE;

    ++living_count;

    pthread_mutex_unlock(&module_mutex);

    salmon_thread_remove_reference(the_salmon_thread);

    return result;
  }

extern value *thread_name_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 1);
    info = thread_info_from_param(all_arguments_value, 0);

    result = create_string_value(info->name);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

extern value *thread_is_done_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 1);
    info = thread_info_from_param(all_arguments_value, 0);

    if (info->terminated)
        result = create_true_value();
    else
        result = create_false_value();
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

extern value *thread_interrupt_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;
    object *standard_library_object;
    size_t key_field_num;
    value *key_value;
    lepton_key_instance *lepton_key;
    value *exception_value;
    value *tag_value;
    verdict the_verdict;
    value *message_value;
    interrupt_info *the_interrupt_info;
    boolean do_block;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(jumper_flowing_forward(the_jumper));

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 4);

    info = thread_info_from_param(all_arguments_value, 0);
    assert(info != NULL);

    standard_library_object = jumper_standard_library_object(the_jumper);
    assert(standard_library_object != NULL);

    assert(!(object_is_closed(standard_library_object)));
            /* VERIFICATION NEEDED */
    key_field_num = object_field_lookup(standard_library_object, "exception");
    assert(key_field_num < object_field_count(standard_library_object));

    assert(!(object_is_closed(standard_library_object)));
            /* VERIFICATION NEEDED */
    key_value = object_field_read_value(standard_library_object, key_field_num,
                                        location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(key_value == NULL);
        return NULL;
      }

    assert(key_value != NULL);
    assert(get_value_kind(key_value) == VK_LEPTON_KEY);

    lepton_key = value_lepton_key(key_value);
    assert(lepton_key != NULL);

    exception_value = create_lepton_value(lepton_key);
    if (exception_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(key_value, the_jumper);
        return NULL;
      }

    value_remove_reference(key_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    tag_value = value_component_value(all_arguments_value, 1);
    assert(tag_value != NULL);
    assert(get_value_kind(tag_value) == VK_QUARK);

    the_verdict = add_field(exception_value, "tag", tag_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    message_value = value_component_value(all_arguments_value, 2);
    assert(message_value != NULL);
    assert(get_value_kind(message_value) == VK_STRING);

    the_verdict = add_field(exception_value, "message", message_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    the_interrupt_info = MALLOC_ONE_OBJECT(interrupt_info);
    if (the_interrupt_info == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    the_interrupt_info->exception_value = exception_value;
    the_interrupt_info->other_value =
            value_component_value(all_arguments_value, 3);
    assert(the_interrupt_info->other_value != NULL);

    if (!(value_has_only_named_fields(the_interrupt_info->other_value)))
      {
        do_exception(the_jumper, location, info->exception_object,
                "et_interrupt_unnamed_extra",
                "The ``other'' parameter to an interrupt() call contained an "
                "unnamed field.");
        free(the_interrupt_info);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    value_add_reference(the_interrupt_info->other_value);

    if (info == thread_info_from_jumper(the_jumper))
      {
        pthread_mutex_lock(&(info->status_change_mutex));
        assert(info->status == TS_RUNNING);
        pthread_mutex_unlock(&(info->status_change_mutex));

        assert(the_interrupt_info->other_value != NULL);
        assert(value_has_only_named_fields(the_interrupt_info->other_value));

        jumper_throw_exception_with_location(the_jumper,
                the_interrupt_info->exception_value,
                the_interrupt_info->other_value, location);

        value_remove_reference(the_interrupt_info->exception_value,
                               the_jumper);
        value_remove_reference(the_interrupt_info->other_value, the_jumper);

        free(the_interrupt_info);

        return NULL;
      }

    assert(jumper_flowing_forward(the_jumper));
    start_force_section(the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(the_interrupt_info->other_value, the_jumper);
        free(the_interrupt_info);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    pthread_mutex_lock(&(info->status_change_mutex));

    switch (info->status)
      {
        case TS_RUNNING:
          {
            the_interrupt_info->do_block = TRUE;
            do_block = TRUE;
            PRE_PTHREAD_MUTEX_INIT(the_interrupt_info->mutex);
            pthread_mutex_init(&(the_interrupt_info->mutex), NULL);
            pthread_cond_init(&(the_interrupt_info->condition), NULL);
            pthread_mutex_lock(&(the_interrupt_info->mutex));
            break;
          }
        case TS_FINISHED:
          {
            pthread_mutex_unlock(&(info->status_change_mutex));
            finish_force_section();

            value_remove_reference(the_interrupt_info->exception_value,
                                   the_jumper);
            value_remove_reference(the_interrupt_info->other_value,
                                   the_jumper);
            free(the_interrupt_info);

            do_exception(the_jumper, location, info->exception_object,
                    "et_interrupt_finished",
                    "An interrupt() call was made on a thread that had already"
                    " finished.");
            return NULL;
          }
        case TS_BLOCKED:
          {
            the_interrupt_info->do_block = FALSE;
            do_block = FALSE;

            assert(info->awaken_handler != NULL);
            (*(info->awaken_handler))(info->awaken_data);

            break;
          }
        case TS_SLEEPING:
          {
            the_interrupt_info->do_block = FALSE;
            do_block = FALSE;

            pthread_mutex_lock(&(info->mutex));
            pthread_cond_broadcast(&(info->condition));
            pthread_mutex_unlock(&(info->mutex));

            break;
          }
        case TS_SUSPENDED:
          {
            the_interrupt_info->do_block = FALSE;
            do_block = FALSE;
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    the_verdict = do_force_one_call(&(the_interrupt_info->force_info), info,
                                    &do_interrupt_handler, the_interrupt_info);

    pthread_mutex_unlock(&(info->status_change_mutex));

    finish_force_section();

    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    if (do_block)
      {
        deactivating();
        pthread_cond_wait(&(the_interrupt_info->condition),
                          &(the_interrupt_info->mutex));
        pthread_mutex_unlock(&(the_interrupt_info->mutex));
        pthread_cond_destroy(&(the_interrupt_info->condition));
        pthread_mutex_destroy(&(the_interrupt_info->mutex));
        free(the_interrupt_info);
        activating();
      }

    return NULL;
  }

extern value *thread_kill_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;
    force_one_call_info force_info;
    verdict the_verdict;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(jumper_flowing_forward(the_jumper));

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    info = thread_info_from_param(all_arguments_value, 0);

    if (info == thread_info_from_jumper(the_jumper))
      {
        pthread_mutex_lock(&(info->status_change_mutex));
        assert(info->status == TS_RUNNING);
        info->exit_status = ES_KILLED;
        pthread_mutex_unlock(&(info->status_change_mutex));

        jumper_do_abort(the_jumper);

        return NULL;
      }

    assert(jumper_flowing_forward(the_jumper));
    start_force_section(the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    pthread_mutex_lock(&(info->status_change_mutex));

    switch (info->status)
      {
        case TS_RUNNING:
          {
            info->suspend_coming = FALSE;
            break;
          }
        case TS_FINISHED:
          {
            pthread_mutex_unlock(&(info->status_change_mutex));
            finish_force_section();
            return NULL;
          }
        case TS_BLOCKED:
          {
            assert(info->awaken_handler != NULL);
            (*(info->awaken_handler))(info->awaken_data);
            break;
          }
        case TS_SLEEPING:
          {
            pthread_mutex_lock(&(info->mutex));
            pthread_cond_broadcast(&(info->condition));
            pthread_mutex_unlock(&(info->mutex));
            break;
          }
        case TS_SUSPENDED:
          {
            assert(!(info->suspend_coming));
            pthread_cond_signal(&(info->suspend_condition));
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    info->exit_status = ES_KILLED;

    the_verdict = do_force_one_call(&force_info, info, &do_kill_handler, NULL);

    pthread_mutex_unlock(&(info->status_change_mutex));

    finish_force_section();

    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    pthread_mutex_lock(&(info->mutex));
    if (!(info->terminated))
      {
        thread_info *my_info;

        my_info = thread_info_from_jumper(the_jumper);
        assert(my_info != NULL);
        starting_blocked(my_info, &awaken_termination_waiters, info,
                         the_jumper, location);

        while (jumper_flowing_forward(the_jumper))
          {
            pthread_cond_wait(&(info->termination_waiters), &(info->mutex));
            if (info->terminated)
                break;

            pthread_mutex_lock(&(my_info->status_change_mutex));
            assert(my_info->status == TS_BLOCKED);
            check_for_running_force(my_info, the_jumper, location);
            pthread_mutex_unlock(&(my_info->status_change_mutex));
          }

        finished_blocked(my_info, the_jumper, location);
      }
    pthread_mutex_unlock(&(info->mutex));

    return NULL;
  }

extern value *thread_wait_for_termination_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 1);
    info = thread_info_from_param(all_arguments_value, 0);

    pthread_mutex_lock(&(info->mutex));
    if (!(info->terminated))
      {
        thread_info *my_info;

        my_info = thread_info_from_jumper(the_jumper);
        assert(my_info != NULL);
        starting_blocked(my_info, &awaken_termination_waiters, info,
                         the_jumper, location);

        while (jumper_flowing_forward(the_jumper))
          {
            pthread_cond_wait(&(info->termination_waiters), &(info->mutex));
            if (info->terminated)
                break;

            pthread_mutex_lock(&(my_info->status_change_mutex));
            assert(my_info->status == TS_BLOCKED);
            check_for_running_force(my_info, the_jumper, location);
            pthread_mutex_unlock(&(my_info->status_change_mutex));
          }

        finished_blocked(my_info, the_jumper, location);
      }
    pthread_mutex_unlock(&(info->mutex));

    pthread_join(info->pthread_handle, NULL);

    return NULL;
  }

extern value *thread_get_status_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;
    const char *quark_name;
    size_t field_num;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 1);
    info = thread_info_from_param(all_arguments_value, 0);

    pthread_mutex_lock(&(info->status_change_mutex));

    switch (info->status)
      {
        case TS_RUNNING:
            if (info->suspend_coming)
                quark_name = "suspended";
            else
                quark_name = "running";
            break;
        case TS_FINISHED:
            quark_name = "finished";
            break;
        case TS_BLOCKED:
            quark_name = "blocked";
            break;
        case TS_SLEEPING:
            quark_name = "sleeping";
            break;
        case TS_SUSPENDED:
            quark_name = "suspended";
            break;
        default:
            assert(FALSE);
      }

    pthread_mutex_unlock(&(info->status_change_mutex));

    field_num = object_field_lookup(info->exception_object, quark_name);
    assert(field_num < object_field_count(info->exception_object));
    result = create_quark_value(instance_quark_instance(
            object_field_instance(info->exception_object, field_num)));
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

extern value *thread_suspend_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(jumper_flowing_forward(the_jumper));

    assert(value_component_count(all_arguments_value) == 1);
    info = thread_info_from_param(all_arguments_value, 0);

    assert(jumper_flowing_forward(the_jumper));
    start_force_section(the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    if (info == thread_info_from_jumper(the_jumper))
      {
        pthread_mutex_lock(&(info->status_change_mutex));

        assert(info->status == TS_RUNNING);
        assert(!info->suspend_coming);
        info->status = TS_SUSPENDED;

        finish_force_section();

        deactivating();
        pthread_cond_wait(&(info->suspend_condition),
                          &(info->status_change_mutex));
        activating();

        assert(info->status == TS_SUSPENDED);
        info->status = TS_RUNNING;
        check_for_running_force(info, the_jumper, location);

        pthread_mutex_unlock(&(info->status_change_mutex));

        return NULL;
      }

    pthread_mutex_lock(&(info->status_change_mutex));

    if (info->status == TS_FINISHED)
      {
        pthread_mutex_unlock(&(info->status_change_mutex));
        finish_force_section();
        do_exception(the_jumper, location, info->exception_object,
                "et_suspend_finished",
                "A suspend() call was made on a thread that had already "
                "finished.");
      }
    else if ((info->status == TS_SUSPENDED) || info->suspend_coming)
      {
        pthread_mutex_unlock(&(info->status_change_mutex));
        finish_force_section();
        do_exception(the_jumper, location, info->exception_object,
                "et_suspend_suspended",
                "A suspend() call was made on a thread that had already been "
                "suspended.");
      }
    else
      {
        suspend_info *the_suspend_info;
        boolean do_block;
        verdict the_verdict;

        the_suspend_info = MALLOC_ONE_OBJECT(suspend_info);
        if (the_suspend_info == NULL)
          {
            pthread_mutex_unlock(&(info->status_change_mutex));
            finish_force_section();
            jumper_do_abort(the_jumper);
            return NULL;
          }

        if (info->status == TS_RUNNING)
          {
            the_suspend_info->unblock = TRUE;
            PRE_PTHREAD_MUTEX_INIT(the_suspend_info->mutex);
            pthread_mutex_init(&(the_suspend_info->mutex), NULL);
            pthread_cond_init(&(the_suspend_info->condition), NULL);
            pthread_mutex_lock(&(the_suspend_info->mutex));
            do_block = TRUE;
          }
        else
          {
            the_suspend_info->unblock = FALSE;
            do_block = FALSE;
          }

        info->suspend_coming = TRUE;
        the_verdict = do_force_one_call(&(the_suspend_info->force_info), info,
                                        &do_suspend_handler, the_suspend_info);

        pthread_mutex_unlock(&(info->status_change_mutex));

        finish_force_section();

        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        if (do_block)
          {
            deactivating();
            pthread_cond_wait(&(the_suspend_info->condition),
                              &(the_suspend_info->mutex));
            pthread_mutex_unlock(&(the_suspend_info->mutex));
            pthread_cond_destroy(&(the_suspend_info->condition));
            pthread_mutex_destroy(&(the_suspend_info->mutex));
            activating();
            free(the_suspend_info);
          }
      }

    return NULL;
  }

extern value *thread_resume_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;
    boolean do_wait;
    force_one_call_info force_info;
    resume_info the_resume_info;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 1);
    info = thread_info_from_param(all_arguments_value, 0);

    pthread_mutex_lock(&(info->status_change_mutex));

    if (info->status == TS_SUSPENDED)
      {
        verdict the_verdict;

        assert(!(info->suspend_coming));
        pthread_cond_signal(&(info->suspend_condition));

        assert(jumper_flowing_forward(the_jumper));
        start_force_section(the_jumper, location);
        if (!(jumper_flowing_forward(the_jumper)))
            return NULL;

        PRE_PTHREAD_MUTEX_INIT(the_resume_info.mutex);
        pthread_mutex_init(&(the_resume_info.mutex), NULL);
        pthread_cond_init(&(the_resume_info.condition), NULL);
        pthread_mutex_lock(&(the_resume_info.mutex));

        the_verdict = do_force_one_call(&force_info, info, &do_resume_handler,
                                        &the_resume_info);

        finish_force_section();

        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            pthread_mutex_unlock(&(the_resume_info.mutex));
            pthread_cond_destroy(&(the_resume_info.condition));
            pthread_mutex_destroy(&(the_resume_info.mutex));
            do_wait = FALSE;
          }
        else
          {
            do_wait = TRUE;
          }
      }
    else if (!(info->suspend_coming))
      {
        do_exception(the_jumper, location, info->exception_object,
                "et_resume_not_suspended",
                "A resume() call was made on a thread that was not "
                "suspended.");
        do_wait = FALSE;
      }
    else
      {
        info->suspend_coming = FALSE;
        do_wait = FALSE;
      }

    pthread_mutex_unlock(&(info->status_change_mutex));

    if (do_wait)
      {
        deactivating();
        pthread_cond_wait(&(the_resume_info.condition),
                          &(the_resume_info.mutex));
        pthread_mutex_unlock(&(the_resume_info.mutex));
        pthread_cond_destroy(&(the_resume_info.condition));
        pthread_mutex_destroy(&(the_resume_info.mutex));
        activating();
      }

    return NULL;
  }

extern value *thread_get_source_position_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;
    get_position_info the_get_position_info;
    force_one_call_info force_info;
    boolean do_block;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(jumper_flowing_forward(the_jumper));

    assert(value_component_count(all_arguments_value) == 1);
    info = thread_info_from_param(all_arguments_value, 0);

    assert(jumper_flowing_forward(the_jumper));
    start_force_section(the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    pthread_mutex_lock(&(info->status_change_mutex));
    if (info == thread_info_from_jumper(the_jumper))
      {
        assert(info->status == TS_RUNNING);
        info->last_location = location;
        do_block = FALSE;
      }
    else if (info->status == TS_RUNNING)
      {
        verdict the_verdict;

        PRE_PTHREAD_MUTEX_INIT(the_get_position_info.mutex);
        pthread_mutex_init(&(the_get_position_info.mutex), NULL);
        pthread_cond_init(&(the_get_position_info.condition), NULL);
        pthread_mutex_lock(&(the_get_position_info.mutex));
        the_verdict = do_force_one_call(&force_info, info,
                &source_position_found_handler, &the_get_position_info);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            pthread_mutex_unlock(&(info->status_change_mutex));
            finish_force_section();
            return NULL;
          }
        do_block = TRUE;
      }
    else
      {
        do_block = FALSE;
      }
    pthread_mutex_unlock(&(info->status_change_mutex));

    finish_force_section();

    if (do_block)
      {
        deactivating();
        pthread_cond_wait(&(the_get_position_info.condition),
                          &(the_get_position_info.mutex));
        pthread_mutex_unlock(&(the_get_position_info.mutex));
        pthread_cond_destroy(&(the_get_position_info.condition));
        pthread_mutex_destroy(&(the_get_position_info.mutex));
        activating();
      }

    return create_source_location_value(info->last_location, the_jumper);
  }

extern value *thread_get_exit_status_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;
    const char *quark_name;
    size_t field_num;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 1);
    info = thread_info_from_param(all_arguments_value, 0);

    pthread_mutex_lock(&(info->status_change_mutex));
    if (info->status != TS_FINISHED)
      {
        pthread_mutex_unlock(&(info->status_change_mutex));
        do_exception(the_jumper, location, info->exception_object,
                "et_get_exit_status_not_finished",
                "A get_exit_status() call was made on a thread that had not "
                "yet exited.");
        return NULL;
      }
    pthread_mutex_unlock(&(info->status_change_mutex));

    switch (info->exit_status)
      {
        case ES_NORMAL:
            quark_name = "exit_normal";
            break;
        case ES_EXCEPTIONS:
            quark_name = "exit_exceptions";
            break;
        case ES_KILLED:
            quark_name = "exit_killed";
            break;
        case ES_DIED:
            quark_name = "exit_died";
            break;
        case ES_JUMPED_OUT:
            quark_name = "exit_jumped_out";
            break;
        default:
            assert(FALSE);
      }

    field_num = object_field_lookup(info->exception_object, quark_name);
    assert(field_num < object_field_count(info->exception_object));
    result = create_quark_value(instance_quark_instance(
            object_field_instance(info->exception_object, field_num)));
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

extern value *thread_get_exit_exceptions_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 1);
    info = thread_info_from_param(all_arguments_value, 0);

    pthread_mutex_lock(&(info->status_change_mutex));
    if (info->status != TS_FINISHED)
      {
        pthread_mutex_unlock(&(info->status_change_mutex));
        do_exception(the_jumper, location, info->exception_object,
                "et_get_exit_exceptions_not_finished",
                "A get_exit_exceptions() call was made on a thread that had "
                "not yet exited.");
        return NULL;
      }
    pthread_mutex_unlock(&(info->status_change_mutex));

    if (info->exit_status != ES_EXCEPTIONS)
      {
        do_exception(the_jumper, location, info->exception_object,
                "et_get_exit_exceptions_no_exception",
                "A get_exit_exceptions() call was made on a thread whose exit "
                "status was not exit_exceptions.");
        return NULL;
      }

    result = info->exit_exception_information;
    assert(result != NULL);
    value_add_reference(result);
    return result;
  }

extern value *thread_get_exit_jump_target_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 1);
    info = thread_info_from_param(all_arguments_value, 0);

    pthread_mutex_lock(&(info->status_change_mutex));
    if (info->status != TS_FINISHED)
      {
        pthread_mutex_unlock(&(info->status_change_mutex));
        do_exception(the_jumper, location, info->exception_object,
                "et_get_exit_jump_target_not_finished",
                "A get_exit_jump_target() call was made on a thread that had "
                "not yet exited.");
        return NULL;
      }
    pthread_mutex_unlock(&(info->status_change_mutex));

    if (info->exit_status != ES_JUMPED_OUT)
      {
        do_exception(the_jumper, location, info->exception_object,
                "et_get_exit_jump_target_no_jump",
                "A get_exit_jump_target() call was made on a thread whose exit"
                " status was not exit_jumped_out.");
        return NULL;
      }

    result = create_jump_target_value(info->exit_jump_target);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

extern value *initialize_semaphore_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    object *the_object;
    semaphore_info *info;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 4);
    the_object =
            object_value_data(value_component_value(all_arguments_value, 0));

    info = MALLOC_ONE_OBJECT(semaphore_info);
    if (info == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    PRE_PTHREAD_MUTEX_INIT(info->mutex);
    pthread_mutex_init(&(info->mutex), NULL);
    pthread_cond_init(&(info->condition), NULL);
    info->current =
            integer_value_data(value_component_value(all_arguments_value, 2));
    oi_add_reference(info->current);
    info->limit =
            integer_value_data(value_component_value(all_arguments_value, 1));
    oi_add_reference(info->limit);
    info->base_object =
            object_value_data(value_component_value(all_arguments_value, 3));
    object_add_reference(info->base_object);

    assert(object_hook(the_object) == NULL);
    object_set_hook(the_object, info);
    object_set_hook_cleaner(the_object, &semaphore_cleaner);

    return NULL;
  }

extern value *semaphore_up_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    semaphore_info *info;
    o_integer amount;
    o_integer old_value;
    o_integer new_value;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 2);

    info = (semaphore_info *)(object_hook(object_value_data(
                   value_component_value(all_arguments_value, 0))));
    assert(info != NULL);

    amount = integer_value_data(value_component_value(all_arguments_value, 1));
    assert(!(oi_out_of_memory(amount)));

    pthread_mutex_lock(&(info->mutex));

    old_value = info->current;

    oi_add(new_value, old_value, amount);
    if (oi_out_of_memory(new_value))
      {
        pthread_mutex_unlock(&(info->mutex));
        jumper_do_abort(the_jumper);
        return NULL;
      }

    if (oi_less_than(info->limit, new_value))
      {
        pthread_mutex_unlock(&(info->mutex));
        do_exception(the_jumper, jumper_call_stack_back_site(the_jumper, 1),
                info->base_object, "et_semaphore_up_beyond_limit",
                "In a semaphore's up() method, an attempt was made to "
                "increment the value to %I, which is beyond that semaphore's "
                "limit of %I.", new_value, info->limit);
        oi_remove_reference(new_value);
        return NULL;
      }

    info->current = new_value;
    oi_remove_reference(old_value);

    pthread_cond_broadcast(&(info->condition));

    pthread_mutex_unlock(&(info->mutex));

    return NULL;
  }

extern value *semaphore_down_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    semaphore_info *info;
    o_integer amount;
    o_integer old_value;
    o_integer new_value;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 2);

    info = (semaphore_info *)(object_hook(object_value_data(
                   value_component_value(all_arguments_value, 0))));
    assert(info != NULL);

    amount = integer_value_data(value_component_value(all_arguments_value, 1));
    assert(!(oi_out_of_memory(amount)));

    pthread_mutex_lock(&(info->mutex));

    while (TRUE)
      {
        thread_info *the_thread_info;

        old_value = info->current;

        oi_subtract(new_value, old_value, amount);
        if (oi_out_of_memory(new_value))
          {
            pthread_mutex_unlock(&(info->mutex));
            jumper_do_abort(the_jumper);
            return NULL;
          }

        if (!(oi_is_negative(new_value)))
            break;

        oi_remove_reference(new_value);

        the_thread_info = thread_info_from_jumper(the_jumper);
        starting_blocked(the_thread_info, &awaken_semaphore_waiters, info,
                         the_jumper, location);

        if (jumper_flowing_forward(the_jumper))
            pthread_cond_wait(&(info->condition), &(info->mutex));

        finished_blocked(the_thread_info, the_jumper, location);

        if (!(jumper_flowing_forward(the_jumper)))
          {
            pthread_mutex_unlock(&(info->mutex));
            return NULL;
          }
      }

    info->current = new_value;
    oi_remove_reference(old_value);

    pthread_mutex_unlock(&(info->mutex));

    return NULL;
  }

extern value *semaphore_current_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    semaphore_info *info;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 1);

    info = (semaphore_info *)(object_hook(object_value_data(
                   value_component_value(all_arguments_value, 0))));
    assert(info != NULL);

    pthread_mutex_lock(&(info->mutex));

    result = create_integer_value(info->current);

    pthread_mutex_unlock(&(info->mutex));

    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

extern value *semaphore_try_down_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    semaphore_info *info;
    o_integer amount;
    boolean expiration_time_computed;
    o_integer old_value;
    o_integer new_value;
    boolean expiration_untimed;
    struct timespec expiration_time;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 4);

    info = (semaphore_info *)(object_hook(object_value_data(
                   value_component_value(all_arguments_value, 0))));
    assert(info != NULL);

    amount = integer_value_data(value_component_value(all_arguments_value, 1));
    assert(!(oi_out_of_memory(amount)));

    expiration_time_computed = FALSE;

    while (TRUE)
      {
        thread_info *the_thread_info;
        int wait_result;

        pthread_mutex_lock(&(info->mutex));

        old_value = info->current;

        oi_subtract(new_value, old_value, amount);
        if (oi_out_of_memory(new_value))
          {
            pthread_mutex_unlock(&(info->mutex));
            jumper_do_abort(the_jumper);
            return NULL;
          }

        if (!(oi_is_negative(new_value)))
            break;

        oi_remove_reference(new_value);

        if (!expiration_time_computed)
          {
            boolean did_time_out;

            compute_expiration_time(&expiration_time, &expiration_untimed,
                    value_component_value(all_arguments_value, 2),
                    object_value_data(
                            value_component_value(all_arguments_value, 3)),
                    the_jumper, &did_time_out,
                    "a semaphore's try_down() method");
            if (!(jumper_flowing_forward(the_jumper)))
              {
                pthread_mutex_unlock(&(info->mutex));
                return NULL;
              }
            if (did_time_out)
              {
                pthread_mutex_unlock(&(info->mutex));
                goto timed_out;
              }
            expiration_time_computed = TRUE;
          }

        the_thread_info = thread_info_from_jumper(the_jumper);
        starting_blocked(the_thread_info, &awaken_semaphore_waiters, info,
                         the_jumper, location);

        if (jumper_flowing_forward(the_jumper))
          {
            if (expiration_untimed)
              {
                wait_result =
                        pthread_cond_wait(&(info->condition), &(info->mutex));
              }
            else
              {
                wait_result = pthread_cond_timedwait(&(info->condition),
                        &(info->mutex), &expiration_time);
              }
          }

        pthread_mutex_unlock(&(info->mutex));

        finished_blocked(the_thread_info, the_jumper, location);
        if (!(jumper_flowing_forward(the_jumper)))
            return NULL;

        if (wait_result == ETIMEDOUT)
          {
            value *result;

          timed_out:
            result = create_false_value();
            if (result == NULL)
                jumper_do_abort(the_jumper);
            return result;
          }
      }

    info->current = new_value;
    oi_remove_reference(old_value);

    pthread_mutex_unlock(&(info->mutex));

    result = create_true_value();
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

extern value *wait_for_all_threads_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    info = thread_info_from_jumper(the_jumper);
    if (info != &root_thread)
      {
        do_exception(the_jumper, location, info->exception_object,
                "et_wait_all_not_root",
                "A thread that was not the root thread called "
                "wait_for_all_threads().");
        return NULL;
      }

    wait_for_all_threads_to_finish_handler(NULL);

    return NULL;
  }

extern value *all_threads_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *result;
    thread_info *follow;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    pthread_mutex_lock(&module_mutex);

    follow = living_threads;
    while (follow != NULL)
      {
        value *item;
        verdict the_verdict;

        item = create_object_value(follow->thread_object);
        if (item == NULL)
          {
            pthread_mutex_unlock(&module_mutex);
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        the_verdict = add_field(result, NULL, item);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            pthread_mutex_unlock(&module_mutex);
            jumper_do_abort(the_jumper);
            value_remove_reference(item, the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        value_remove_reference(item, the_jumper);
        assert(jumper_flowing_forward(the_jumper));

        follow = follow->next;
      }

    pthread_mutex_unlock(&module_mutex);

    return result;
  }

extern value *current_thread_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    info = thread_info_from_jumper(the_jumper);

    assert(info->thread_object != NULL);
    result = create_object_value(info->thread_object);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

extern value *sleep_handler(value *all_arguments_value, context *the_context,
        jumper *the_jumper, const source_location *location)
  {
    thread_info *info;
    value *seconds_value;
    struct timespec expiration_time;
    boolean plus_infinity;
    boolean non_positive;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    info = thread_info_from_jumper(the_jumper);
    assert(info != NULL);

    assert(value_component_count(all_arguments_value) == 1);
    seconds_value = value_component_value(all_arguments_value, 0);
    assert(seconds_value != NULL);

    pthread_mutex_lock(&(info->status_change_mutex));
    assert(info->status == TS_RUNNING);
    check_for_running_force(info, the_jumper, location);
    info->status = TS_SLEEPING;
    pthread_mutex_unlock(&(info->status_change_mutex));

    compute_expiration_time(&expiration_time, &plus_infinity, seconds_value,
            info->exception_object, the_jumper, &non_positive, "sleep()");

    deactivating();

    if (!non_positive)
      {
        pthread_mutex_lock(&(info->mutex));

        while (jumper_flowing_forward(the_jumper))
          {
            int wait_result;

            if (plus_infinity)
              {
                wait_result =
                        pthread_cond_wait(&(info->condition), &(info->mutex));
              }
            else
              {
                wait_result = pthread_cond_timedwait(&(info->condition),
                        &(info->mutex), &expiration_time);
              }

            if (wait_result == ETIMEDOUT)
                break;

            pthread_mutex_lock(&(info->status_change_mutex));
            assert(info->status == TS_SLEEPING);
            info->status = TS_RUNNING;
            check_for_running_force(info, the_jumper, location);
            assert(info->status == TS_RUNNING);
            info->status = TS_SLEEPING;
            pthread_mutex_unlock(&(info->status_change_mutex));
          }

        pthread_mutex_unlock(&(info->mutex));
      }

    pthread_mutex_lock(&(info->status_change_mutex));
    assert(info->status == TS_SLEEPING);
    info->status = TS_RUNNING;
    check_for_running_force(info, the_jumper, location);
    pthread_mutex_unlock(&(info->status_change_mutex));

    activating();

    return NULL;
  }

extern value *yield_handler(value *all_arguments_value, context *the_context,
        jumper *the_jumper, const source_location *location)
  {
    thread_info *info;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    info = thread_info_from_jumper(the_jumper);
    assert(info != NULL);

    assert(value_component_count(all_arguments_value) == 0);

    pthread_mutex_lock(&(info->status_change_mutex));
    assert(info->status == TS_RUNNING);
    check_for_running_force(info, the_jumper, location);
    pthread_mutex_unlock(&(info->status_change_mutex));

    deactivating();

    activating();

    return NULL;
  }

extern value *initialize_module_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    thread_info *info;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    info = thread_info_from_jumper(the_jumper);
    assert(info != NULL);

    assert(value_component_count(all_arguments_value) == 3);

    pthread_mutex_lock(&(module_reference_mutex));

    ++module_reference_count;

    if (module_reference_count == 1)
      {
        assert(info->exception_object == NULL);
        info->exception_object = object_value_data(
                value_component_value(all_arguments_value, 1));
        assert(info->thread_object == NULL);
        info->thread_object = object_value_data(
                value_component_value(all_arguments_value, 0));
        assert(info->cluster == NULL);
        info->cluster = object_reference_cluster(info->thread_object);
        object_add_reference(info->thread_object);
        info->last_location = location;
        assert(info->salmon_thread == NULL);
        info->salmon_thread = jumper_thread(the_jumper);
        info->pthread_handle = pthread_self();
        assert(info->dummy_child_exception_target == NULL);
        info->dummy_child_exception_target =
                jump_target_value_data(
                        value_component_value(all_arguments_value, 2));
        jump_target_add_reference(info->dummy_child_exception_target);

        object_set_hook(info->thread_object, info);
        object_set_hook_cleaner(info->thread_object, &top_cleaner);
      }
    else
      {
        assert(info->exception_object != NULL);
        assert(info->thread_object != NULL);
        assert(info->salmon_thread != NULL);
        assert(info->dummy_child_exception_target != NULL);
      }

    pthread_mutex_unlock(&(module_reference_mutex));

    return NULL;
  }


static void *thread_starter(void *untyped_data)
  {
    starter_data *data;
    semi_labeled_value_list *actuals;

    data = (starter_data *)untyped_data;

    assert(data->info->status == TS_RUNNING);

    set_salmon_thread_stack_bottom(data->info->salmon_thread, &data);

    activating();

    actuals = create_semi_labeled_value_list();
    if (actuals != NULL)
      {
        value *result;

        result = execute_call_from_values(data->body_value, actuals, FALSE,
                                          data->jumper, data->location);
        assert(result == NULL);
        delete_semi_labeled_value_list(actuals, NULL);
      }

    value_remove_reference(data->body_value, NULL);

    pthread_mutex_lock(&module_mutex);

    pthread_mutex_lock(&(data->info->status_change_mutex));
    assert(data->info->status == TS_RUNNING);
    check_for_running_force(data->info, NULL, NULL);
    data->info->status = TS_FINISHED;

    if ((data->info->exit_status == ES_NORMAL) &&
        !(jumper_flowing_forward(data->jumper)))
      {
        if (jumper_has_unreleased_exception_information(data->jumper))
          {
            data->info->exit_status = ES_EXCEPTIONS;
            data->info->exit_exception_information =
                    jumper_unreleased_exception_information(data->jumper);
            value_add_reference_with_reference_cluster(
                    data->info->exit_exception_information,
                    data->info->cluster);
            jumper_clear_unreleased_exception_information(data->jumper);
          }
        else if (jumper_target(data->jumper) != NULL)
          {
            data->info->exit_status = ES_JUMPED_OUT;
            data->info->exit_jump_target = jumper_target(data->jumper);
            jump_target_add_reference(data->info->exit_jump_target);
          }
        else
          {
            data->info->exit_status = ES_DIED;
          }
      }

    pthread_mutex_unlock(&(data->info->status_change_mutex));

    assert(data->info->living);
    data->info->living = FALSE;

    if (data->info->next != NULL)
      {
        assert(data->info->next->previous == &(data->info->next));
        data->info->next->previous = data->info->previous;
      }
    assert(*(data->info->previous) == data->info);
    *(data->info->previous) = data->info->next;

    pthread_mutex_unlock(&module_mutex);

    thread_info_add_low_ref(data->info);
    thread_info_remove_high_ref(data->info, NULL);

    object_remove_reference(data->info->thread_object, data->jumper);

    jumper_pop_catcher(data->jumper, data->info->dummy_child_exception_target);
    delete_jumper(data->jumper);

    pthread_mutex_lock(&(data->info->mutex));
    assert(!(data->info->terminated));
    data->info->terminated = TRUE;
    pthread_cond_broadcast(&(data->info->termination_waiters));
    pthread_mutex_unlock(&(data->info->mutex));

    thread_info_remove_low_ref(data->info);

    free(data);

    deactivating();

    pthread_mutex_lock(&module_mutex);
    --living_count;
    if (someone_waiting_for_death)
      {
        someone_waiting_for_death = FALSE;
        pthread_cond_signal(&death_watch_condition);
      }
    pthread_mutex_unlock(&module_mutex);

    return NULL;
  }

static void *root_thread_back_end_data_handler(void *data)
  {
    assert(data == NULL);

    return &root_thread;
  }

static void wait_for_all_threads_to_finish_handler(void *data)
  {
    assert(data == NULL);

    pthread_mutex_lock(&module_mutex);
    while (TRUE)
      {
        if (living_count == 0)
          {
            pthread_mutex_unlock(&module_mutex);

            while (TRUE)
              {
                thread_info *info;

                pthread_mutex_lock(&zombie_mutex);

                info = zombie_threads;
                if (info == NULL)
                  {
                    pthread_mutex_unlock(&zombie_mutex);
                    break;
                  }
                zombie_threads = info->next;

                pthread_mutex_unlock(&zombie_mutex);

                pthread_join(info->pthread_handle, NULL);
                free(info);
              }

            return;
          }

        assert(!someone_waiting_for_death);
        someone_waiting_for_death = TRUE;
        deactivating();
        pthread_cond_wait(&death_watch_condition, &module_mutex);
        assert(!someone_waiting_for_death);
        if (interpreter_not_threadsafe)
          {
            pthread_mutex_unlock(&module_mutex);
            pthread_mutex_lock(&single_mutex);
            not_threadsafe_single_up();
            pthread_mutex_unlock(&single_mutex);
            pthread_mutex_lock(&module_mutex);
          }
      }
  }

static void block_handler(void *data, void *thread_data, jumper *the_jumper,
                          const source_location *location)
  {
    thread_info *info;

    assert(data == NULL);

    info = (thread_info *)thread_data;
    assert(info != NULL);

    starting_blocked(info, &awaken_blocked, info, the_jumper, location);

    pthread_mutex_lock(&(info->mutex));

    while (jumper_flowing_forward(the_jumper))
      {
        if (info->is_unblocked)
            break;
        pthread_cond_wait(&(info->condition), &(info->mutex));
        if (info->is_unblocked)
            break;

        pthread_mutex_lock(&(info->status_change_mutex));
        assert(info->status == TS_BLOCKED);
        check_for_running_force(info, the_jumper, location);
        pthread_mutex_unlock(&(info->status_change_mutex));
      }

    info->is_unblocked = FALSE;

    pthread_mutex_unlock(&(info->mutex));

    finished_blocked(info, the_jumper, location);
  }

static void unblock_handler(void *data, void *thread_data)
  {
    thread_info *info;

    assert(data == NULL);

    info = (thread_info *)thread_data;
    assert(info != NULL);

    pthread_mutex_lock(&(info->mutex));
    assert(!(info->is_unblocked));
    info->is_unblocked = TRUE;
    pthread_cond_signal(&(info->condition));
    pthread_mutex_unlock(&(info->mutex));
  }

static void self_block_handler(void *data, void *thread_data,
        jumper *the_jumper, const source_location *location)
  {
    thread_info *info;

    assert(data == NULL);

    info = (thread_info *)thread_data;
    assert(info != NULL);

    starting_blocked(info, &awaken_blocked, info, the_jumper, location);
  }

static void self_unblock_handler(void *data, void *thread_data,
        jumper *the_jumper, const source_location *location)
  {
    thread_info *info;

    assert(data == NULL);

    info = (thread_info *)thread_data;
    assert(info != NULL);

    finished_blocked(info, the_jumper, location);
  }

static void start_single_threaded_handler(void *data, jumper *the_jumper,
                                          const source_location *location)
  {
    thread_info *locking_thread_info;
    boolean aborted;
    thread_info *follow;

    assert(data == NULL);
    assert(the_jumper != NULL);

    assert(jumper_flowing_forward(the_jumper));

    locking_thread_info = thread_info_from_jumper(the_jumper);
    assert(locking_thread_info != NULL);

    assert(jumper_flowing_forward(the_jumper));
    start_force_section(the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return;

    pthread_mutex_lock(&(the_single_threaded_data.single_mutex));

    assert(the_single_threaded_data.single_holder == NULL);
    the_single_threaded_data.single_holder = locking_thread_info;

    pthread_mutex_lock(&module_mutex);

    aborted = FALSE;

    for (follow = living_threads; follow != NULL; follow = follow->next)
      {
        force_one_call_info *force_info;
        verdict the_verdict;

        assert(!(follow->single_threaded_waiting));

        if (follow == locking_thread_info)
            continue;

        pthread_mutex_lock(&(follow->status_change_mutex));

        if (follow->status == TS_RUNNING)
          {
            follow->single_threaded_waiting = TRUE;
            ++(the_single_threaded_data.waiting_count);
          }

        force_info = MALLOC_ONE_OBJECT(force_one_call_info);
        if (force_info == NULL)
          {
            pthread_mutex_unlock(&(follow->status_change_mutex));
            jumper_do_abort(the_jumper);
            aborted = TRUE;
            break;
          }
        the_verdict = do_force_one_call(force_info, follow,
                &hold_for_single_threading_handler, force_info);

        pthread_mutex_unlock(&(follow->status_change_mutex));

        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            aborted = TRUE;
            break;
          }
      }

    pthread_mutex_unlock(&module_mutex);

    finish_force_section();

    if (the_single_threaded_data.waiting_count > 0)
      {
        deactivating();
        pthread_cond_wait(&(the_single_threaded_data.single_condition),
                          &(the_single_threaded_data.single_mutex));
        activating();
      }

    assert(the_single_threaded_data.waiting_count == 0);

    if (aborted)
      {
        assert(the_single_threaded_data.single_holder == locking_thread_info);
        the_single_threaded_data.single_holder = NULL;
        pthread_cond_broadcast(&(the_single_threaded_data.others_condition));
      }

    pthread_mutex_unlock(&(the_single_threaded_data.single_mutex));
  }

static void finish_single_threaded_handler(void *data, jumper *the_jumper)
  {
    thread_info *locking_thread_info;

    assert(data == NULL);
    assert(the_jumper != NULL);

    locking_thread_info = thread_info_from_jumper(the_jumper);
    assert(locking_thread_info != NULL);

    pthread_mutex_lock(&(the_single_threaded_data.single_mutex));

    assert(the_single_threaded_data.single_holder == locking_thread_info);
    the_single_threaded_data.single_holder = NULL;

    pthread_cond_broadcast(&(the_single_threaded_data.others_condition));

    pthread_mutex_unlock(&(the_single_threaded_data.single_mutex));
  }

static void remove_thread_reference_handler(void *data, void *thread_data)
  {
    thread_info *info;

    assert(data == NULL);

    info = (thread_info *)thread_data;
    assert(info != NULL);

    pthread_mutex_lock(&module_mutex);

    assert(info->has_external_reference);
    info->has_external_reference = FALSE;

    pthread_mutex_unlock(&module_mutex);

    thread_info_remove_high_ref(info, NULL);
  }

static void thread_info_remove_high_ref(thread_info *info, jumper *the_jumper)
  {
    size_t new_high_ref_count;

    pthread_mutex_lock(&(info->ref_count_mutex));
    assert(info->high_ref_count > 0);
    --(info->high_ref_count);
    new_high_ref_count = info->high_ref_count;
    pthread_mutex_unlock(&(info->ref_count_mutex));

    if (new_high_ref_count > 0)
        return;

    if (info->exit_status == ES_EXCEPTIONS)
      {
        value_remove_reference_with_reference_cluster(
                info->exit_exception_information, the_jumper, info->cluster);
      }
    else if (info->exit_status == ES_JUMPED_OUT)
      {
        jump_target_remove_reference(info->exit_jump_target);
      }
    jump_target_remove_reference(info->dummy_child_exception_target);
    object_remove_reference(info->exception_object, the_jumper);

    thread_info_remove_low_ref(info);
  }

static void thread_info_add_low_ref(thread_info *info)
  {
    pthread_mutex_lock(&(info->ref_count_mutex));
    assert(info->low_ref_count > 0);
    ++(info->low_ref_count);
    pthread_mutex_unlock(&(info->ref_count_mutex));
  }

static void thread_info_remove_low_ref(thread_info *info)
  {
    size_t new_low_ref_count;

    pthread_mutex_lock(&(info->ref_count_mutex));
    assert(info->low_ref_count > 0);
    --(info->low_ref_count);
    new_low_ref_count = info->low_ref_count;
    pthread_mutex_unlock(&(info->ref_count_mutex));

    if (new_low_ref_count > 0)
        return;

    pthread_mutex_destroy(&(info->mutex));
    pthread_cond_destroy(&(info->condition));
    pthread_cond_destroy(&(info->termination_waiters));
    pthread_mutex_destroy(&(info->ref_count_mutex));
    pthread_mutex_destroy(&(info->status_change_mutex));
    free(info->name);

    if (info->pthread_handle != pthread_self())
      {
        pthread_join(info->pthread_handle, NULL);
        free(info);
      }
    else
      {
        pthread_mutex_lock(&zombie_mutex);
        info->next = zombie_threads;
        zombie_threads = info;
        pthread_mutex_unlock(&zombie_mutex);
      }
  }

static void do_exception(jumper *the_jumper, const source_location *location,
        object *base_object, const char *tag, const char *format, ...)
  {
    static_exception_tag static_tag;
    va_list ap;

    static_tag.field_name = tag;
    static_tag.u.object = base_object;
    va_start(ap, format);
    vlocation_exception(the_jumper, location, &static_tag, format, ap);
    va_end(ap);
  }

static thread_info *thread_info_from_param(value *all_arguments_value,
                                           size_t param_num)
  {
    assert(all_arguments_value != NULL);

    return (thread_info *)(object_hook(object_value_data(
            value_component_value(all_arguments_value, param_num))));
  }

static thread_info *thread_info_from_jumper(jumper *the_jumper)
  {
    assert(the_jumper != NULL);

    return (thread_info *)(salmon_thread_back_end_data(jumper_thread(
            the_jumper)));
  }

static void starting_blocked(thread_info *info,
        void (*awaken_handler)(void *data), void *awaken_data,
        jumper *the_jumper, const source_location *location)
  {
    assert(info != NULL);
    assert(awaken_handler != NULL);

    pthread_mutex_lock(&(info->status_change_mutex));
    assert(info->status == TS_RUNNING);
    check_for_running_force(info, the_jumper, location);
    info->status = TS_BLOCKED;
    assert(info->awaken_handler == NULL);
    assert(info->awaken_data == NULL);
    info->awaken_handler = awaken_handler;
    info->awaken_data = awaken_data;
    pthread_mutex_unlock(&(info->status_change_mutex));

    deactivating();
  }

static void finished_blocked(thread_info *info, jumper *the_jumper,
                             const source_location *location)
  {
    assert(info != NULL);

    pthread_mutex_lock(&(info->status_change_mutex));
    assert(info->status == TS_BLOCKED);
    info->status = TS_RUNNING;
    assert(info->awaken_handler != NULL);
    info->awaken_handler = NULL;
    info->awaken_data = NULL;
    check_for_running_force(info, the_jumper, location);
    pthread_mutex_unlock(&(info->status_change_mutex));

    activating();
  }

static void activating(void)
  {
    if (interpreter_not_threadsafe)
      {
        pthread_mutex_lock(&single_mutex);
        not_threadsafe_single_up();
        pthread_mutex_unlock(&single_mutex);
      }
  }

static void deactivating(void)
  {
    if (interpreter_not_threadsafe)
      {
        pthread_mutex_lock(&single_mutex);
        not_threadsafe_single_down();
        pthread_mutex_unlock(&single_mutex);
      }
  }

static void not_threadsafe_single_down(void)
  {
    assert(interpreter_not_threadsafe);
    assert(single_active_count == 1);
    --single_active_count;
    pthread_cond_signal(&single_condition);
  }

static void not_threadsafe_single_up(void)
  {
    while (single_active_count == 1)
        pthread_cond_wait(&single_condition, &single_mutex);
    assert(single_active_count == 0);
    ++single_active_count;
  }

static verdict split_second_value(value *second_value, boolean *non_positive,
        boolean *plus_infinity, o_integer *whole_seconds, long *nanoseconds)
  {
    o_integer numerator;
    o_integer denominator;
    o_integer one_billion;
    o_integer num_product;
    o_integer num_sum;
    o_integer num_diff;
    o_integer nanos;
    o_integer remainder;
    o_integer nano_part;
    verdict the_verdict;

    switch (get_value_kind(second_value))
      {
        case VK_INTEGER:
          {
            numerator = integer_value_data(second_value);
            denominator = oi_null;
            break;
          }
        case VK_RATIONAL:
          {
            rational *the_rational;

            the_rational = rational_value_data(second_value);
            numerator = rational_numerator(the_rational);
            denominator = rational_denominator(the_rational);
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    if (oi_kind(numerator) == IIK_POSITIVE_INFINITY)
      {
        *non_positive = FALSE;
        *plus_infinity = TRUE;
        return MISSION_ACCOMPLISHED;
      }

    *plus_infinity = FALSE;

    assert(oi_kind(numerator) == IIK_FINITE);
    assert(!(oi_is_negative(numerator)));

    if (oi_equal(numerator, oi_zero))
      {
        *non_positive = TRUE;
        return MISSION_ACCOMPLISHED;
      }

    *non_positive = FALSE;

    if (oi_out_of_memory(denominator))
      {
        oi_add_reference(numerator);
        *whole_seconds = numerator;
        *nanoseconds = 0;
        return MISSION_ACCOMPLISHED;
      }

    oi_create_from_long_int(one_billion, BILLION_LONG);
    if (oi_out_of_memory(one_billion))
        return MISSION_FAILED;

    oi_multiply(num_product, numerator, one_billion);
    if (oi_out_of_memory(num_product))
      {
        oi_remove_reference(one_billion);
        return MISSION_FAILED;
      }

    oi_add(num_sum, num_product, denominator);
    oi_remove_reference(num_product);
    if (oi_out_of_memory(num_sum))
      {
        oi_remove_reference(one_billion);
        return MISSION_FAILED;
      }

    oi_subtract(num_diff, num_sum, oi_one);
    oi_remove_reference(num_sum);
    if (oi_out_of_memory(num_diff))
      {
        oi_remove_reference(one_billion);
        return MISSION_FAILED;
      }

    oi_divide(nanos, num_diff, denominator, &remainder);
    oi_remove_reference(num_diff);
    if (oi_out_of_memory(nanos))
      {
        oi_remove_reference(one_billion);
        return MISSION_FAILED;
      }
    oi_remove_reference(remainder);

    oi_divide(*whole_seconds, nanos, one_billion, &nano_part);
    oi_remove_reference(nanos);
    oi_remove_reference(one_billion);
    if (oi_out_of_memory(*whole_seconds))
        return MISSION_FAILED;

    the_verdict = oi_to_long_int(nano_part, nanoseconds);
    assert(the_verdict == MISSION_ACCOMPLISHED);
    oi_remove_reference(nano_part);
    assert(*nanoseconds < BILLION_LONG);

    return MISSION_ACCOMPLISHED;
  }

static void check_for_running_force(thread_info *info, jumper *the_jumper,
                                    const source_location *location)
  {
    assert(info != NULL);

    info->last_location = location;

    assert(info->status == TS_RUNNING);

    while (info->first_run_mode_forced_action != NULL)
      {
        run_mode_forced_action_data *item;
        void (*handler)(void *data, thread_info *info, jumper *the_jumper,
                        const source_location *location);
        void *data;

        item = info->first_run_mode_forced_action;
        assert(item != NULL);
        assert(item->previous == &(info->first_run_mode_forced_action));
        info->first_run_mode_forced_action = item->next;
        if (item->next != NULL)
          {
            assert(item->next->previous == &(item->next));
            item->next->previous = &(info->first_run_mode_forced_action);
          }

        handler = item->handler;
        assert(handler != NULL);
        data = item->data;

        free(item);

        (*handler)(data, info, the_jumper, location);
      }
  }

static void start_force_section(jumper *the_jumper,
                                const source_location *location)
  {
    thread_info *my_info;

    assert(the_jumper != NULL);

    assert(jumper_flowing_forward(the_jumper));

    my_info = thread_info_from_jumper(the_jumper);
    assert(my_info != NULL);

    while (TRUE)
      {
        assert(jumper_flowing_forward(the_jumper));

        pthread_mutex_lock(&force_section_mutex);

        pthread_mutex_lock(&(my_info->status_change_mutex));
        assert(my_info->status == TS_RUNNING);
        if (my_info->first_run_mode_forced_action == NULL)
          {
            pthread_mutex_unlock(&(my_info->status_change_mutex));
            assert(jumper_flowing_forward(the_jumper));
            return;
          }

        pthread_mutex_unlock(&force_section_mutex);

        check_for_running_force(my_info, the_jumper, location);
        assert(my_info->status == TS_RUNNING);
        pthread_mutex_unlock(&(my_info->status_change_mutex));

        if (!(jumper_flowing_forward(the_jumper)))
            return;
      }
  }

static void finish_force_section(void)
  {
    pthread_mutex_unlock(&force_section_mutex);
  }

static verdict do_force_one_call(force_one_call_info *force_info,
        thread_info *info,
        void (*handler)(void *data, thread_info *info, jumper *the_jumper,
                        const source_location *location), void *data)
  {
    assert(force_info != NULL);
    assert(info != NULL);
    assert(handler != NULL);

    PRE_PTHREAD_MUTEX_INIT(force_info->mutex);
    pthread_mutex_init(&(force_info->mutex), NULL);
    pthread_mutex_lock(&(force_info->mutex));

    force_info->handler = handler;
    force_info->data = data;
    force_info->info = info;

    force_info->statement_handle = register_start_statement_execution_watcher(
            info->salmon_thread, &force_one_statement_watcher, force_info);
    if (force_info->statement_handle == NULL)
        return MISSION_FAILED;

    force_info->expression_handle =
            register_start_expression_evaluation_watcher(info->salmon_thread,
                    &force_one_expression_watcher, force_info);
    if (force_info->expression_handle == NULL)
      {
        unregister_start_statement_execution_watcher(info->salmon_thread,
                force_info->statement_handle);
        return MISSION_FAILED;
      }

    force_info->run_mode_handle =
            MALLOC_ONE_OBJECT(run_mode_forced_action_data);
    if (force_info->run_mode_handle == NULL)
      {
        unregister_start_statement_execution_watcher(info->salmon_thread,
                force_info->statement_handle);
        unregister_start_expression_evaluation_watcher(info->salmon_thread,
                force_info->expression_handle);
        return MISSION_FAILED;
      }
    force_info->run_mode_handle->handler = &force_one_run_mode_watcher;
    force_info->run_mode_handle->data = force_info;
    force_info->run_mode_handle->next = info->first_run_mode_forced_action;
    force_info->run_mode_handle->previous =
            &(info->first_run_mode_forced_action);
    info->first_run_mode_forced_action = force_info->run_mode_handle;
    if (force_info->run_mode_handle->next != NULL)
      {
        assert(force_info->run_mode_handle->next->previous ==
               &(info->first_run_mode_forced_action));
        force_info->run_mode_handle->next->previous =
                &(force_info->run_mode_handle->next);
      }

    pthread_mutex_unlock(&(force_info->mutex));

    return MISSION_ACCOMPLISHED;
  }

static void force_one_statement_watcher(void *data, statement *the_statement,
                                        jumper *the_jumper)
  {
    force_one_call_info *typed_data;
    run_mode_forced_action_data *run_mode_handle;
    thread_info *the_thread_info;

    assert(data != NULL);
    assert(the_statement != NULL);

    typed_data = (force_one_call_info *)data;

    run_mode_handle = typed_data->run_mode_handle;
    assert(run_mode_handle != NULL);

    the_thread_info = thread_info_from_jumper(the_jumper);
    assert(the_thread_info != NULL);

    pthread_mutex_lock(&(the_thread_info->status_change_mutex));

    *(run_mode_handle->previous) = run_mode_handle->next;
    if (run_mode_handle->next != NULL)
      {
        assert(run_mode_handle->next->previous == &(run_mode_handle->next));
        run_mode_handle->next->previous = run_mode_handle->previous;
      }
    free(run_mode_handle);

    force_one_call_ultimate_handler(typed_data, the_jumper,
                                    get_statement_location(the_statement));

    pthread_mutex_unlock(&(the_thread_info->status_change_mutex));
  }

static void force_one_expression_watcher(void *data,
        expression *the_expression, jumper *the_jumper)
  {
    force_one_call_info *typed_data;
    run_mode_forced_action_data *run_mode_handle;
    thread_info *the_thread_info;

    assert(data != NULL);
    assert(the_expression != NULL);

    typed_data = (force_one_call_info *)data;

    run_mode_handle = typed_data->run_mode_handle;
    assert(run_mode_handle != NULL);

    the_thread_info = thread_info_from_jumper(the_jumper);
    assert(the_thread_info != NULL);

    pthread_mutex_lock(&(the_thread_info->status_change_mutex));

    *(run_mode_handle->previous) = run_mode_handle->next;
    if (run_mode_handle->next != NULL)
      {
        assert(run_mode_handle->next->previous == &(run_mode_handle->next));
        run_mode_handle->next->previous = run_mode_handle->previous;
      }
    free(run_mode_handle);

    force_one_call_ultimate_handler(typed_data, the_jumper,
                                    get_expression_location(the_expression));

    pthread_mutex_unlock(&(the_thread_info->status_change_mutex));
  }

static void force_one_run_mode_watcher(void *data, thread_info *info,
        jumper *the_jumper, const source_location *location)
  {
    force_one_call_info *typed_data;

    assert(data != NULL);

    typed_data = (force_one_call_info *)data;
    force_one_call_ultimate_handler(typed_data, the_jumper, location);
  }

static void force_one_call_ultimate_handler(force_one_call_info *force_info,
        jumper *the_jumper, const source_location *location)
  {
    assert(force_info != NULL);

    pthread_mutex_lock(&(force_info->mutex));

    unregister_start_statement_execution_watcher(
            force_info->info->salmon_thread, force_info->statement_handle);
    unregister_start_expression_evaluation_watcher(
            force_info->info->salmon_thread, force_info->expression_handle);

    pthread_mutex_unlock(&(force_info->mutex));
    pthread_mutex_destroy(&(force_info->mutex));

    (*(force_info->handler))(force_info->data, force_info->info, the_jumper,
                             location);
  }

static void do_interrupt_handler(void *data, thread_info *info,
        jumper *the_jumper, const source_location *location)
  {
    interrupt_info *the_interrupt_info;

    assert(data != NULL);
    assert(info != NULL);

    the_interrupt_info = (interrupt_info *)data;

    assert(the_interrupt_info->other_value != NULL);
    assert(value_has_only_named_fields(the_interrupt_info->other_value));

    if (the_jumper != NULL)
      {
        jumper_throw_exception_with_location(the_jumper,
                the_interrupt_info->exception_value,
                the_interrupt_info->other_value, location);
      }

    value_remove_reference(the_interrupt_info->exception_value, the_jumper);
    value_remove_reference(the_interrupt_info->other_value, the_jumper);

    if (the_interrupt_info->do_block)
      {
        pthread_mutex_lock(&(the_interrupt_info->mutex));
        pthread_cond_signal(&(the_interrupt_info->condition));
        pthread_mutex_unlock(&(the_interrupt_info->mutex));
      }
    else
      {
        free(the_interrupt_info);
      }
  }

static void do_kill_handler(void *data, thread_info *info, jumper *the_jumper,
                            const source_location *location)
  {
    assert(data == NULL);
    assert(info != NULL);

    if (the_jumper != NULL)
        jumper_do_abort(the_jumper);
  }

static void do_suspend_handler(void *data, thread_info *info,
        jumper *the_jumper, const source_location *location)
  {
    suspend_info *the_suspend_info;

    assert(data != NULL);
    assert(info != NULL);

    the_suspend_info = (suspend_info *)data;

    deactivating();

    assert(info->status == TS_RUNNING);

    if (the_suspend_info->unblock)
      {
        pthread_mutex_lock(&(the_suspend_info->mutex));
        pthread_cond_signal(&(the_suspend_info->condition));
        pthread_mutex_unlock(&(the_suspend_info->mutex));
      }
    else
      {
        free(the_suspend_info);
      }

    if (info->suspend_coming)
      {
        info->suspend_coming = FALSE;
        info->status = TS_SUSPENDED;
        pthread_cond_wait(&(info->suspend_condition),
                          &(info->status_change_mutex));
        assert(info->status == TS_SUSPENDED);
        info->status = TS_RUNNING;
        check_for_running_force(info, the_jumper, location);
      }

    pthread_mutex_unlock(&(info->status_change_mutex));
    activating();
    pthread_mutex_lock(&(info->status_change_mutex));
  }

static void do_resume_handler(void *data, thread_info *info,
        jumper *the_jumper, const source_location *location)
  {
    resume_info *the_resume_info;

    assert(data != NULL);
    assert(info != NULL);

    the_resume_info = (resume_info *)data;

    pthread_mutex_lock(&(the_resume_info->mutex));
    pthread_cond_signal(&(the_resume_info->condition));
    pthread_mutex_unlock(&(the_resume_info->mutex));
  }

static void source_position_found_handler(void *data, thread_info *info,
        jumper *the_jumper, const source_location *location)
  {
    get_position_info *typed_data;

    assert(data != NULL);
    assert(info != NULL);

    info->last_location = location;

    typed_data = (get_position_info *)data;
    pthread_mutex_lock(&(typed_data->mutex));
    pthread_cond_signal(&(typed_data->condition));
    pthread_mutex_unlock(&(typed_data->mutex));
  }

static void hold_for_single_threading_handler(void *data, thread_info *info,
        jumper *the_jumper, const source_location *location)
  {
    assert(data != NULL);
    assert(info != NULL);

    free(data);

    pthread_mutex_lock(&(the_single_threaded_data.single_mutex));

    if (the_single_threaded_data.single_holder != NULL)
      {
        if (info->single_threaded_waiting)
          {
            assert(the_single_threaded_data.waiting_count > 0);
            --(the_single_threaded_data.waiting_count);
            if (the_single_threaded_data.waiting_count == 0)
              {
                pthread_cond_signal(
                        &(the_single_threaded_data.single_condition));
              }
          }

        deactivating();
        pthread_cond_wait(&(the_single_threaded_data.others_condition),
                          &(the_single_threaded_data.single_mutex));
        activating();
      }
    else
      {
        assert(!(info->single_threaded_waiting));
      }

    pthread_mutex_unlock(&(the_single_threaded_data.single_mutex));
  }

static void awaken_termination_waiters(void *data)
  {
    thread_info *info;

    assert(data != NULL);

    info = (thread_info *)data;

    pthread_mutex_lock(&(info->mutex));
    pthread_cond_broadcast(&(info->termination_waiters));
    pthread_mutex_unlock(&(info->mutex));
  }

static void awaken_semaphore_waiters(void *data)
  {
    semaphore_info *info;

    assert(data != NULL);

    info = (semaphore_info *)data;

    pthread_mutex_lock(&(info->mutex));
    pthread_cond_broadcast(&(info->condition));
    pthread_mutex_unlock(&(info->mutex));
  }

static void awaken_blocked(void *data)
  {
    thread_info *info;

    assert(data != NULL);

    info = (thread_info *)data;

    pthread_mutex_lock(&(info->mutex));
    pthread_cond_broadcast(&(info->condition));
    pthread_mutex_unlock(&(info->mutex));
  }

static void compute_expiration_time(struct timespec *expiration_time,
        boolean *expiration_untimed, value *second_value, object *base_object,
        jumper *the_jumper, boolean *timed_out,
        const char *routine_description)
  {
    boolean non_positive;
    boolean plus_infinity;
    o_integer whole_seconds;
    long nanoseconds;
    verdict the_verdict;

    assert(expiration_time != NULL);
    assert(expiration_untimed != NULL);
    assert(second_value != NULL);
    assert(base_object != NULL);
    assert(the_jumper != NULL);
    assert(timed_out != NULL);
    assert(routine_description != NULL);

    *timed_out = FALSE;

    the_verdict = split_second_value(second_value, &non_positive,
            &plus_infinity, &whole_seconds, &nanoseconds);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    if (plus_infinity)
      {
        *expiration_untimed = TRUE;
      }
    else if (non_positive)
      {
        *timed_out = FALSE;
        return;
      }
    else
      {
        *expiration_untimed = FALSE;

        clock_gettime(CLOCK_REALTIME, expiration_time);
        assert(expiration_time->tv_nsec < BILLION_LONG);

        assert(nanoseconds < BILLION_LONG);
        if (expiration_time->tv_nsec >= (BILLION_LONG - nanoseconds))
          {
            if (expiration_time->tv_sec < (expiration_time->tv_sec + 1))
              {
                ++(expiration_time->tv_sec);
                expiration_time->tv_nsec -= (BILLION_LONG - nanoseconds);
              }
            else
              {
                oi_remove_reference(whole_seconds);
              too_far_future:
                do_exception(the_jumper,
                        jumper_call_stack_back_site(the_jumper, 1),
                        base_object, "et_time_too_far",
                        "In %s, the `seconds' parameter specified a finite "
                        "time farther in the future than the underlying system"
                        " can support.", routine_description);
                return;
              }
          }
        else
          {
            expiration_time->tv_nsec += nanoseconds;
            assert(expiration_time->tv_nsec < BILLION_LONG);
          }

        while (TRUE)
          {
            long new_seconds;
            verdict the_verdict;
            o_integer max_oi;
            o_integer next;

            the_verdict = oi_to_long_int(whole_seconds, &new_seconds);
            if (the_verdict != MISSION_ACCOMPLISHED)
                new_seconds = LONG_MAX;

            if ((((long)(time_t)new_seconds) != new_seconds) ||
                (expiration_time->tv_sec + (time_t)new_seconds <
                 expiration_time->tv_sec))
              {
                oi_remove_reference(whole_seconds);
                goto too_far_future;
              }
            expiration_time->tv_sec += (time_t)new_seconds;

            if (the_verdict == MISSION_ACCOMPLISHED)
                break;

            oi_create_from_long_int(max_oi, LONG_MAX);
            if (oi_out_of_memory(max_oi))
              {
                oi_remove_reference(whole_seconds);
                jumper_do_abort(the_jumper);
                return;
              }

            oi_subtract(next, whole_seconds, max_oi);
            oi_remove_reference(max_oi);
            if (oi_out_of_memory(next))
              {
                oi_remove_reference(whole_seconds);
                jumper_do_abort(the_jumper);
                return;
              }

            oi_remove_reference(whole_seconds);
            whole_seconds = next;
            assert(!(oi_is_negative(whole_seconds)));
          }

        oi_remove_reference(whole_seconds);
      }
  }

static void thread_cleaner(void *hook, jumper *the_jumper)
  {
    thread_info *info;

    assert(hook != NULL);

    info = (thread_info *)hook;

    pthread_mutex_lock(&module_mutex);

    assert(info->has_object_hook);
    info->has_object_hook = FALSE;

    pthread_mutex_unlock(&module_mutex);

    thread_info_remove_high_ref(info, the_jumper);
  }

static void semaphore_cleaner(void *hook, jumper *the_jumper)
  {
    semaphore_info *info;

    assert(hook != NULL);

    info = (semaphore_info *)hook;

    pthread_mutex_destroy(&(info->mutex));
    pthread_cond_destroy(&(info->condition));
    oi_remove_reference(info->current);
    oi_remove_reference(info->limit);
    object_remove_reference(info->base_object, the_jumper);
    free(info);
  }

static void top_cleaner(void *hook, jumper *the_jumper)
  {
    assert(hook != NULL);

    pthread_mutex_lock(&(module_reference_mutex));

    --module_reference_count;

    if (module_reference_count == 0)
      {
        wait_for_all_threads_to_finish_handler(NULL);

        assert(root_thread.exception_object != NULL);
        root_thread.exception_object = NULL;
        assert(root_thread.thread_object != NULL);
        object_remove_reference(root_thread.thread_object, the_jumper);
        root_thread.thread_object = NULL;
        root_thread.last_location = NULL;
        assert(root_thread.salmon_thread != NULL);
        root_thread.salmon_thread = NULL;
        assert(root_thread.dummy_child_exception_target != NULL);
        jump_target_remove_reference(root_thread.dummy_child_exception_target);
        root_thread.dummy_child_exception_target = NULL;
      }

    pthread_mutex_unlock(&(module_reference_mutex));
  }
