/* file "driver.c" */

/*
 *  This file contains the top-level driver for SalmonEye.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "c_foundations/basic.h"
#include "c_foundations/diagnostic.h"
#include "c_foundations/trace.h"
#include "c_foundations/memory_allocation.h"
#include "value.h"
#include "token.h"
#include "tokenizer.h"
#include "parser.h"
#include "standard_built_ins.h"
#include "routine_declaration_chain.h"
#include "routine_instance_chain.h"
#include "unbound.h"
#include "bind.h"
#include "open_statement_block.h"
#include "execute.h"
#include "jump_target.h"
#include "type.h"
#include "file_parser.h"
#include "trace_channels.h"
#include "unicode.h"
#include "driver.h"
#include "purity_level.h"
#include "thread.h"
#include "object.h"
#include "o_integer.h"
#include "utility.h"
#include "profile.h"
#include "platform_dependent.h"


struct salmoneye_lock
  {
    int dummy;
    DECLARE_SYSTEM_LOCK(lock);
  };


static const char version_string[] =
#include "version.h"
;


static context *global_object_internal_context(object *global_object,
                                               jumper *the_jumper);
static object *object_for_class_declaration(routine_declaration *declaration,
        context *the_context, purity_level *level, tracer *the_tracer,
        salmon_thread *root_thread, jumper *the_jumper);
static routine_declaration_chain *routine_instance_chain_to_declaration_chain(
        routine_instance_chain *routine_chain);


static const char default_directory_paths[] =
#ifdef DEFAULT_DIRECTORY_PATHS
    DEFAULT_DIRECTORY_PATHS;
#else
#ifdef LIBRARY_INSTALL_DIRECTORY
    LIBRARY_INSTALL_DIRECTORY PATH_SEPARATOR "ID";
#else
    "ID";
#endif
#endif


extern verdict init_salmoneye(void)
  {
    verdict the_verdict;

    the_verdict = init_o_integer_module();
    if (the_verdict != MISSION_ACCOMPLISHED)
        goto o_integer_failure;

    the_verdict = init_thread_module();
    if (the_verdict != MISSION_ACCOMPLISHED)
        goto thread_failure;

    the_verdict = init_object_module();
    if (the_verdict != MISSION_ACCOMPLISHED)
        goto object_failure;

    the_verdict = init_include_module();
    if (the_verdict != MISSION_ACCOMPLISHED)
        goto include_failure;

    the_verdict = init_quark_module();
    if (the_verdict != MISSION_ACCOMPLISHED)
        goto quark_failure;

    the_verdict = init_type_module();
    if (the_verdict != MISSION_ACCOMPLISHED)
        goto type_failure;

    the_verdict = init_instance_module();
    if (the_verdict != MISSION_ACCOMPLISHED)
        goto instance_failure;

    the_verdict = init_profile_module();
    if (the_verdict != MISSION_ACCOMPLISHED)
        goto profile_failure;

    the_verdict = init_execute_module();
    if (the_verdict != MISSION_ACCOMPLISHED)
        goto execute_failure;

    return MISSION_ACCOMPLISHED;

  execute_failure:
    cleanup_profile_module();
  profile_failure:
    cleanup_instance_module();
  instance_failure:
    cleanup_type_module();
  type_failure:
    cleanup_rational_module();
    cleanup_quark_module();
  quark_failure:
    cleanup_include_module();
  include_failure:
    cleanup_object_module();
  object_failure:
    cleanup_thread_module();
  thread_failure:
    cleanup_o_integer_module();

  o_integer_failure:
    verify_validators_cleaned_up();

    cleanup_platform_dependent_module();
    cleanup_diagnostic();

    return the_verdict;
  }

extern void cleanup_salmoneye(void)
  {
    cleanup_execute_module();
    cleanup_profile_module();
    cleanup_instance_module();
    cleanup_type_module();
    cleanup_rational_module();
    cleanup_quark_module();
    cleanup_include_module();
    cleanup_object_module();
    cleanup_thread_module();
    cleanup_o_integer_module();

    verify_validators_cleaned_up();

    cleanup_platform_dependent_module();
    cleanup_diagnostic();
  }

extern int run_salmoneye(int argc, char *argv[])
  {
    const char *source_file_name;
    boolean print_version;
    boolean profile;
    boolean list_leak_count;
    boolean list_leak_details;
    boolean native_bridge_dll_body_allowed;
    tracer *the_tracer;
    const char *directory_paths;
    int arg_num;
    char *executable_directory;
    int return_code;

    source_file_name = NULL;
    print_version = FALSE;
    profile = FALSE;
    list_leak_count = FALSE;
    list_leak_details = FALSE;
    native_bridge_dll_body_allowed = TRUE;

    the_tracer = create_tracer(TC_COUNT, trace_channel_names);
    if (the_tracer == NULL)
        return 1;

    tracer_set_output_handler(the_tracer, &vinterpreter_zero_fprintf);

    directory_paths = getenv("SALMONEYE_INCLUDE_PATHS");
    if (directory_paths == NULL)
        directory_paths = getenv("SALMON_INCLUDE_PATHS");
    if (directory_paths == NULL)
        directory_paths = default_directory_paths;

    for (arg_num = 1; arg_num < argc; ++arg_num)
      {
        int tracer_count;

        tracer_count = handle_tracer_option(the_tracer, argc - arg_num,
                                            &(argv[arg_num]));
        if (tracer_count < 0)
          {
            delete_tracer(the_tracer);
            return 1;
          }
        else if (tracer_count > 0)
          {
            arg_num += (tracer_count - 1);
            continue;
          }

        if (strcmp(argv[arg_num], "-v") == 0)
          {
            print_version = TRUE;
            continue;
          }

        if ((strcmp(argv[arg_num], "-p") == 0) ||
            (strcmp(argv[arg_num], "-profile") == 0))
          {
            profile = TRUE;
            continue;
          }

        if (strcmp(argv[arg_num], "-use-se-dll") == 0)
            continue;

        if ((strcmp(argv[arg_num], "-ip") == 0) ||
            (strcmp(argv[arg_num], "-include-paths") == 0))
          {
            if ((arg_num + 1) == argc)
              {
                basic_error("Option %s requires a <paths> argument.",
                            argv[arg_num]);
                delete_tracer(the_tracer);
                return 1;
              }

            directory_paths = argv[arg_num + 1];
            ++arg_num;

            continue;
          }

        if ((strcmp(argv[arg_num], "-llc") == 0) ||
            (strcmp(argv[arg_num], "-list-leak-count") == 0))
          {
            list_leak_count = TRUE;
            continue;
          }

        if ((strcmp(argv[arg_num], "-lld") == 0) ||
            (strcmp(argv[arg_num], "-list-leak-details") == 0))
          {
            list_leak_details = TRUE;
            continue;
          }

        if ((strcmp(argv[arg_num], "-dnb") == 0) ||
            (strcmp(argv[arg_num], "-disallow-nb-dll") == 0))
          {
            native_bridge_dll_body_allowed = FALSE;
            continue;
          }

        if ((strcmp(argv[arg_num], "-h") == 0) ||
            (strcmp(argv[arg_num], "-help") == 0))
          {
            size_t trace_option_count;
            size_t trace_option_num;

            basic_error("Usage: %s <option>?* <input-file-name>.",
                        interpreter_name());
            basic_error("Options:");
            basic_error("    -v");
            basic_error("        Show version information.");
            basic_error("    -p");
            basic_error("    -profile");
            basic_error("        Collect and print profiling information.");
            basic_error("    -use-se-dll");
            basic_error(
                    "        Use the SalmonEye dll instead of the statically "
                    "linked in version");
            basic_error("        of the interpreter.");
            basic_error("    -ip <paths>");
            basic_error("    -include-paths <paths>");
            basic_error(
                    "        Use <paths> as the list of paths to use to find "
                    "included files.");
            basic_error("    -llc");
            basic_error("    -list-leak-count");
            basic_error(
                    "        After program execution is complete, display how "
                    "many leaks");
            basic_error(
                    "        there are.  A leak is any object or instance that"
                    " is still alive");
            basic_error(
                    "        after execution is complete, because it was "
                    "neither explicitly");
            basic_error(
                    "        de-alloced nor caught by automatic garbage "
                    "collection.");
            basic_error("    -lld");
            basic_error("    -list-leak-details");
            basic_error(
                    "        After program execution is complete, display the "
                    "details of all");
            basic_error("        the leaks, if any.");
            basic_error("    -dnb");
            basic_error("    -disallow-nb-dll");
            basic_error("        Disallow routines to specify nb-dll bodies.");

            trace_option_count = tracer_option_count(the_tracer);
            for (trace_option_num = 0; trace_option_num < trace_option_count;
                 ++trace_option_num)
              {
                basic_error("    %s",
                        tracer_option_pattern(the_tracer, trace_option_num));
                basic_error("        %s.",
                        tracer_option_description(the_tracer,
                                                  trace_option_num));
              }
            delete_tracer(the_tracer);
            return 1;
          }

        source_file_name = argv[arg_num];
        break;
      }

    if (print_version)
      {
        fprintf(stdout, "%s version %s.\n", interpreter_name(),
                version_string);
      }

    if (argv[0] != NULL)
      {
        boolean error;

        assert(argv[0] != NULL);

        executable_directory = file_name_directory(argv[0], &error);
        if (error)
          {
            delete_tracer(the_tracer);
            return 1;
          }
      }
    else
      {
        executable_directory = NULL;
      }

    return_code = run_salmon_source_file(source_file_name, argc - arg_num,
            &(argv[arg_num]), profile, list_leak_count, list_leak_details,
            native_bridge_dll_body_allowed, the_tracer, directory_paths,
            executable_directory);

    if (executable_directory != NULL)
        free(executable_directory);

    delete_tracer(the_tracer);

    return return_code;
  }

extern value *c_argv_to_salmon_arguments(int argc, char *argv[])
  {
    value *arguments_value;
    int arg_num;

    arguments_value = create_semi_labeled_value_list_value();
    if (arguments_value == NULL)
        return NULL;

    for (arg_num = 0; arg_num < argc; ++arg_num)
      {
        value *element_value;
        verdict the_verdict;

        element_value = c_string_to_value(argv[arg_num]);
        if (element_value == NULL)
          {
            value_remove_reference(arguments_value, NULL);
            return NULL;
          }

        the_verdict = add_field(arguments_value, NULL, element_value);
        value_remove_reference(element_value, NULL);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            value_remove_reference(arguments_value, NULL);
            return NULL;
          }
      }

    return arguments_value;
  }

extern int run_salmon_source_file(const char *source_file_name, int argc,
        char *argv[], boolean profile, boolean list_leak_count,
        boolean list_leak_details, boolean native_bridge_dll_body_allowed,
        tracer *the_tracer, const char *directory_paths,
        char *executable_directory)
  {
    value *arguments_value;
    open_statement_block *the_open_statement_block;
    unbound_name_manager *the_unbound_name_manager;
    statement_block *the_statement_block;
    declaration *built_ins_declaration;
    context *top_level_context;
    object *built_ins_object;
    size_t built_in_name_count;
    size_t built_in_name_num;
    salmon_thread *root_thread;
    jumper *the_jumper;
    static_home *global_static_home;
    verdict the_verdict;
    context *built_ins_context;
    context *static_global_context;
    int return_code;
    value *result_value;
    size_t sticky_count;
    size_t sticky_num;

    arguments_value = c_argv_to_salmon_arguments(argc, argv);
    if (arguments_value == NULL)
        return 1;

    the_open_statement_block = parse_statement_block_from_file(
            source_file_name, native_bridge_dll_body_allowed, directory_paths,
            executable_directory);
    if (the_open_statement_block == NULL)
      {
        value_remove_reference(arguments_value, NULL);
        return 1;
      }

    built_ins_declaration = create_standard_built_ins_class_declaration();
    if (built_ins_declaration == NULL)
      {
        delete_open_statement_block(the_open_statement_block);
        value_remove_reference(arguments_value, NULL);
        return 1;
      }

    assert(built_ins_declaration != NULL);

    top_level_context = create_top_level_context(arguments_value);
    value_remove_reference(arguments_value, NULL);
    if (top_level_context == NULL)
      {
        delete_open_statement_block(the_open_statement_block);
        declaration_remove_reference(built_ins_declaration);
        cleanup_standard_built_ins_module();
        return 1;
      }

    top_level_context_set_directory_paths(top_level_context, directory_paths);
    top_level_context_set_executable_directory(top_level_context,
                                               executable_directory);

    root_thread = create_salmon_thread("Root Thread",
                                       get_root_thread_back_end_data());
    if (root_thread == NULL)
      {
        exit_context(top_level_context, NULL);
        delete_open_statement_block(the_open_statement_block);
        declaration_remove_reference(built_ins_declaration);
        cleanup_standard_built_ins_module();
        return 1;
      }

    if (current_process_stack_size_limit_known())
      {
        set_salmon_thread_stack_size_limit(root_thread,
                                           current_process_stack_size_limit());
      }

    if (profile)
        set_profiling_enable(TRUE);

    the_jumper = create_root_jumper(root_thread, the_tracer);
    if (the_jumper == NULL)
      {
        salmon_thread_remove_reference(root_thread);
        exit_context(top_level_context, NULL);
        delete_open_statement_block(the_open_statement_block);
        declaration_remove_reference(built_ins_declaration);
        cleanup_standard_built_ins_module();
        return 1;
      }

    built_ins_object = object_for_class_declaration(
            declaration_routine_declaration(built_ins_declaration),
            top_level_context, jumper_purity_level(the_jumper), the_tracer,
            root_thread, the_jumper);
    salmon_thread_remove_reference(root_thread);
    if (built_ins_object == NULL)
      {
        delete_jumper(the_jumper);
        exit_context(top_level_context, NULL);
        delete_open_statement_block(the_open_statement_block);
        declaration_remove_reference(built_ins_declaration);
        cleanup_standard_built_ins_module();
        return 1;
      }

    the_unbound_name_manager = open_statement_block_unbound_name_manager(
            the_open_statement_block);
    assert(the_unbound_name_manager != NULL);

    the_statement_block =
            open_statement_block_statement_block(the_open_statement_block);
    assert(the_statement_block != NULL);

    global_static_home = create_static_home(
            unbound_name_manager_static_count(the_unbound_name_manager),
            unbound_name_manager_static_declarations(
                    the_unbound_name_manager));
    if (global_static_home == NULL)
      {
        assert(!(object_is_closed(built_ins_object))); /* VERIFIED */
        close_object(built_ins_object, the_jumper);
        object_remove_reference(built_ins_object, the_jumper);
        delete_jumper(the_jumper);
        exit_context(top_level_context, NULL);
        delete_open_statement_block(the_open_statement_block);
        declaration_remove_reference(built_ins_declaration);
        cleanup_standard_built_ins_module();
        return 1;
      }

    the_verdict =
            unbound_name_manager_clear_static_list(the_unbound_name_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_static_home(global_static_home);
        assert(!(object_is_closed(built_ins_object))); /* VERIFIED */
        close_object(built_ins_object, the_jumper);
        object_remove_reference(built_ins_object, the_jumper);
        delete_jumper(the_jumper);
        exit_context(top_level_context, NULL);
        delete_open_statement_block(the_open_statement_block);
        declaration_remove_reference(built_ins_declaration);
        cleanup_standard_built_ins_module();
        return 1;
      }

    assert(!(object_is_closed(built_ins_object))); /* VERIFIED */
    built_in_name_count = object_field_count(built_ins_object);

    for (built_in_name_num = 0; built_in_name_num < built_in_name_count;
         ++built_in_name_num)
      {
        const char *built_in_name;
        verdict the_verdict;

        assert(!(object_is_closed(built_ins_object))); /* VERIFIED */
        built_in_name = object_field_name(built_ins_object, built_in_name_num);
        assert(built_in_name != NULL);

        assert(!(object_is_closed(built_ins_object))); /* VERIFIED */
        if (object_field_is_variable(built_ins_object, built_in_name_num))
          {
            the_verdict = bind_variable_name(the_unbound_name_manager,
                    built_in_name,
                    variable_instance_declaration(object_field_variable(
                            built_ins_object, built_in_name_num)));
          }
        else
          {
            value *field_value;

            assert(!(object_is_closed(built_ins_object))); /* VERIFIED */
            field_value = object_field_read_value(built_ins_object,
                    built_in_name_num, NULL, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(field_value == NULL);
                assert(jumper_target(the_jumper) == NULL);
                the_verdict = MISSION_FAILED;
              }
            else
              {
                assert(field_value != NULL);

                switch (get_value_kind(field_value))
                  {
                    case VK_ROUTINE:
                      {
                        routine_instance *instance;
                        routine_declaration *declaration;
                        routine_declaration_chain *chain;

                        instance = routine_value_data(field_value);
                        declaration = routine_instance_declaration(instance);
                        chain = routine_declaration_declaration_chain(
                                declaration);
                        the_verdict = bind_routine_name(
                                the_unbound_name_manager, built_in_name,
                                chain);
                        break;
                      }
                    case VK_ROUTINE_CHAIN:
                      {
                        routine_instance_chain *routine_chain;
                        routine_declaration_chain *chain;

                        routine_chain = routine_chain_value_data(field_value);
                        assert(routine_chain != NULL);

                        chain = routine_instance_chain_to_declaration_chain(
                                routine_chain);
                        if (chain == NULL)
                          {
                            the_verdict = MISSION_FAILED;
                            break;
                          }

                        the_verdict = bind_routine_name(
                                the_unbound_name_manager, built_in_name,
                                chain);
                        routine_declaration_chain_remove_reference(chain);
                        break;
                      }
                    case VK_TAGALONG_KEY:
                      {
                        tagalong_key *instance;
                        tagalong_declaration *declaration;

                        instance = tagalong_key_value_data(field_value);
                        declaration = tagalong_key_declaration(instance);
                        the_verdict = bind_tagalong_name(
                                the_unbound_name_manager, built_in_name,
                                declaration);
                        break;
                      }
                    case VK_LEPTON_KEY:
                      {
                        lepton_key_instance *instance;
                        lepton_key_declaration *declaration;

                        instance = value_lepton_key(field_value);
                        declaration =
                                lepton_key_instance_declaration(instance);
                        the_verdict = bind_lepton_key_name(
                                the_unbound_name_manager, built_in_name,
                                declaration);
                        break;
                      }
                    case VK_QUARK:
                      {
                        quark *instance;
                        quark_declaration *declaration;

                        instance = value_quark(field_value);
                        declaration = quark_instance_declaration(instance);
                        the_verdict = bind_quark_name(the_unbound_name_manager,
                                built_in_name, declaration);
                        break;
                      }
                    case VK_LOCK:
                      {
                        lock_instance *instance;
                        lock_declaration *declaration;

                        instance = lock_value_data(field_value);
                        declaration = lock_instance_declaration(instance);
                        the_verdict = bind_lock_name(the_unbound_name_manager,
                                built_in_name, declaration);
                        break;
                      }
                    default:
                      {
                        assert(FALSE);
                        the_verdict = MISSION_FAILED;
                      }
                  }

                value_remove_reference(field_value, the_jumper);
              }
          }

        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_static_home(global_static_home);
            assert(!(object_is_closed(built_ins_object))); /* VERIFIED */
            close_object(built_ins_object, the_jumper);
            object_remove_reference(built_ins_object, the_jumper);
            delete_jumper(the_jumper);
            exit_context(top_level_context, NULL);
            delete_open_statement_block(the_open_statement_block);
            declaration_remove_reference(built_ins_declaration);
            cleanup_standard_built_ins_module();
            return 1;
          }
      }

    the_verdict = check_for_unbound(the_unbound_name_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_static_home(global_static_home);
        assert(!(object_is_closed(built_ins_object))); /* VERIFIED */
        close_object(built_ins_object, the_jumper);
        object_remove_reference(built_ins_object, the_jumper);
        delete_jumper(the_jumper);
        exit_context(top_level_context, NULL);
        delete_open_statement_block(the_open_statement_block);
        declaration_remove_reference(built_ins_declaration);
        cleanup_standard_built_ins_module();
        return 1;
      }

    set_open_statement_block_statement_block(the_open_statement_block, NULL);
    delete_open_statement_block(the_open_statement_block);

    assert(!(object_is_closed(built_ins_object))); /* VERIFIED */
    built_ins_context =
            global_object_internal_context(built_ins_object, the_jumper);

    static_global_context = create_static_context(built_ins_context,
            global_static_home, the_jumper);
    if (static_global_context == NULL)
      {
        delete_static_home(global_static_home);
        assert(!(object_is_closed(built_ins_object))); /* VERIFIED */
        close_object(built_ins_object, the_jumper);
        object_remove_reference(built_ins_object, the_jumper);
        delete_jumper(the_jumper);
        exit_context(top_level_context, NULL);
        delete_statement_block(the_statement_block);
        declaration_remove_reference(built_ins_declaration);
        cleanup_standard_built_ins_module();
        return 1;
      }

    execute_statement_block(the_statement_block, static_global_context,
                            the_jumper);

    wait_for_all_threads_to_finish();

    exit_context(static_global_context, the_jumper);
    assert(!(object_is_closed(built_ins_object))); /* VERIFIED */
    close_object(built_ins_object, the_jumper);
    object_remove_reference(built_ins_object, the_jumper);
    return_code = 0;
    result_value = get_routine_return_value(top_level_context, NULL);
    if (jumper_target(the_jumper) ==
        find_routine_return_target(top_level_context, NULL))
      {
        jumper_reached_target(the_jumper);
        if (result_value != NULL)
          {
            if (get_value_kind(result_value) == VK_INTEGER)
              {
                o_integer result_oi;
                size_t result_magnitude;

                result_oi = integer_value_data(result_value);
                if (oi_kind(result_oi) == IIK_FINITE)
                  {
                    verdict the_verdict;

                    the_verdict = oi_magnitude_to_size_t(result_oi,
                                                         &result_magnitude);
                    if (the_verdict == MISSION_ACCOMPLISHED)
                      {
                        if (oi_is_negative(result_oi))
                            return_code = -(int)result_magnitude;
                        else
                            return_code = (int)result_magnitude;
                      }
                  }
              }
          }
      }
    if (result_value != NULL)
        value_remove_reference(result_value, the_jumper);
    exit_context(top_level_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(jumper_target(the_jumper) == NULL);
        return_code = 1;
      }

    sticky_count = purity_level_sticky_lock_instance_count(
            jumper_purity_level(the_jumper));
    for (sticky_num = 0; sticky_num < sticky_count; ++sticky_num)
      {
        set_lock_instance_scope_exited(purity_level_sticky_lock_instance(
                jumper_purity_level(the_jumper), sticky_num), the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(jumper_target(the_jumper) == NULL);
            delete_jumper(the_jumper);
            delete_static_home(global_static_home);
            declaration_remove_reference(built_ins_declaration);
            delete_statement_block(the_statement_block);
            cleanup_standard_built_ins_module();
            return 1;
          }
      }

    if (profile)
        dump_profile_listing(stdout);

    delete_jumper(the_jumper);

    cleanup_type_module_instances();

    cleanup_leaked_objects(list_leak_count, list_leak_details);
    cleanup_leaked_instances(list_leak_count, list_leak_details);

    delete_static_home(global_static_home);
    declaration_remove_reference(built_ins_declaration);
    delete_statement_block(the_statement_block);
    cleanup_standard_built_ins_module();

    return return_code;
  }

extern open_statement_block *parse_statement_block_from_file(
        const char *source_file_name, boolean native_bridge_dll_body_allowed,
        const char *directory_paths, char *executable_directory)
  {
    void *include_handler_data;
    file_parser *the_file_parser;
    parser *the_parser;
    open_statement_block *the_open_statement_block;
    tokenizer *the_tokenizer;
    token *the_token;
    token_kind kind;
    verdict the_verdict;

    include_handler_data = create_local_file_include_handler_data(
            ((source_file_name != NULL) ? source_file_name : ""),
            directory_paths, executable_directory);
    if (include_handler_data == NULL)
        return NULL;

    the_file_parser = create_file_parser(
            ((source_file_name != NULL) ? source_file_name : "stdin"),
            ((source_file_name != NULL) ? NULL : stdin), NULL,
            &local_file_include_handler, &local_file_interface_include_handler,
            include_handler_data, NULL, NULL, native_bridge_dll_body_allowed);
    if (the_file_parser == NULL)
      {
        delete_local_file_include_handler_data(include_handler_data);
        return NULL;
      }

    the_parser = file_parser_parser(the_file_parser);

    the_open_statement_block = parse_statement_list(the_parser);
    if (the_open_statement_block == NULL)
      {
        delete_file_parser(the_file_parser);
        delete_local_file_include_handler_data(include_handler_data);
        return NULL;
      }

    the_tokenizer = file_parser_tokenizer(the_file_parser);

    the_token = next_token(the_tokenizer);
    if (the_token == NULL)
      {
        delete_open_statement_block(the_open_statement_block);
        delete_file_parser(the_file_parser);
        delete_local_file_include_handler_data(include_handler_data);
        return NULL;
      }

    kind = get_token_kind(the_token);

    if (kind == TK_ERROR)
      {
        delete_open_statement_block(the_open_statement_block);
        delete_file_parser(the_file_parser);
        delete_local_file_include_handler_data(include_handler_data);
        return NULL;
      }

    if (kind != TK_END_OF_INPUT)
      {
        token_error(the_token, "Syntax error -- expected end of input.");
        delete_open_statement_block(the_open_statement_block);
        delete_file_parser(the_file_parser);
        delete_local_file_include_handler_data(include_handler_data);
        return NULL;
      }

    the_verdict = consume_token(the_tokenizer);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_statement_block(the_open_statement_block);
        delete_file_parser(the_file_parser);
        delete_local_file_include_handler_data(include_handler_data);
        return NULL;
      }

    delete_file_parser(the_file_parser);
    delete_local_file_include_handler_data(include_handler_data);

    return the_open_statement_block;
  }

extern const char *interpreter_name(void)
  {
    return "SalmonEye";
  }

extern salmoneye_lock *create_salmoneye_lock(void)
  {
    salmoneye_lock *result;

    result = MALLOC_ONE_OBJECT(salmoneye_lock);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->lock, free(result); return NULL);

    return result;
  }

extern void destroy_salmoneye_lock(salmoneye_lock *lock)
  {
    assert(lock != NULL);

    DESTROY_SYSTEM_LOCK(lock->lock);
    free(lock);
  }

extern void lock_salmoneye_lock(salmoneye_lock *lock)
  {
    assert(lock != NULL);

    GRAB_SYSTEM_LOCK(lock->lock);
  }

extern void unlock_salmoneye_lock(salmoneye_lock *lock)
  {
    assert(lock != NULL);

    RELEASE_SYSTEM_LOCK(lock->lock);
  }

extern void *salmoneye_allocate(size_t byte_count)
  {
    assert(byte_count > 0);

    return MALLOC_ARRAY(char, byte_count);
  }

extern void salmoneye_free(void *to_free)
  {
    assert(to_free != NULL);

    free(to_free);
  }


static context *global_object_internal_context(object *global_object,
                                               jumper *the_jumper)
  {
    size_t print_field_num;
    value *print_value;
    routine_instance *the_routine_instance;

    assert(global_object != NULL);
    assert(the_jumper != NULL);

    assert(!(object_is_closed(global_object))); /* VERIFIED */
    print_field_num = object_field_lookup(global_object, "print");
    assert(print_field_num < object_field_count(global_object));

    assert(!(object_is_closed(global_object))); /* VERIFIED */
    print_value = object_field_read_value(global_object, print_field_num, NULL,
                                          the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(print_value == NULL);
        return NULL;
      }

    assert(print_value != NULL);
    assert(value_is_valid(print_value)); /* VERIFIED */

    assert(get_value_kind(print_value) == VK_ROUTINE);
    the_routine_instance = routine_value_data(print_value);
    assert(the_routine_instance != NULL);

    value_remove_reference(print_value, NULL);

    assert(routine_instance_is_instantiated(the_routine_instance));
            /* VERIFIED */
    assert(!(routine_instance_scope_exited(the_routine_instance)));
            /* VERIFIED */
    return routine_instance_context(the_routine_instance);
  }

static object *object_for_class_declaration(routine_declaration *declaration,
        context *the_context, purity_level *level, tracer *the_tracer,
        salmon_thread *root_thread, jumper *the_jumper)
  {
    routine_instance *class;
    type *return_type;
    value *routine_value;
    expression *routine_expression;
    call *the_call;
    value *object_value;
    object *result;

    assert(declaration != NULL);
    assert(the_context != NULL);
    assert(level != NULL);

    class = create_routine_instance(declaration, level, NULL);
    if (class == NULL)
        return NULL;

    return_type = get_class_type(class);
    if (return_type == NULL)
      {
        routine_instance_remove_reference(class, NULL);
        return NULL;
      }

    assert(!(routine_instance_scope_exited(class))); /* VERIFIED */
    routine_instance_set_return_type(class, return_type, 1, the_jumper);
    type_remove_reference(return_type, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        routine_instance_remove_reference(class, the_jumper);
        return NULL;
      }

    assert(routine_instance_instance(class) != NULL);
    set_instance_instantiated(routine_instance_instance(class));

    assert(!(routine_instance_scope_exited(class))); /* VERIFIED */
    routine_instance_set_up_static_context(class, the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        routine_instance_remove_reference(class, the_jumper);
        return NULL;
      }

    routine_value = create_routine_value(class);
    routine_instance_remove_reference(class, the_jumper);
    if (routine_value == NULL)
        return NULL;

    routine_expression = create_constant_expression(routine_value);
    value_remove_reference(routine_value, NULL);
    if (routine_expression == NULL)
        return NULL;

    the_call = create_call(routine_expression, 0, NULL, NULL, NULL);
    if (the_call == NULL)
        return NULL;

    object_value = execute_call(the_call, TRUE, NULL, NULL, 0, the_context,
                                the_jumper);
    delete_call(the_call);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(object_value == NULL);
        assert(jumper_target(the_jumper) == NULL);
        return NULL;
      }

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    result = object_value_data(object_value);
    object_add_reference(result);
    value_remove_reference(object_value, NULL);
    return result;
  }

static routine_declaration_chain *routine_instance_chain_to_declaration_chain(
        routine_instance_chain *routine_chain)
  {
    routine_instance_chain *next;
    routine_declaration_chain *tail;
    routine_declaration *declaration;
    routine_declaration_chain *result;

    assert(routine_chain != NULL);

    next = routine_instance_chain_next(routine_chain);

    if (next == NULL)
      {
        tail = NULL;
      }
    else
      {
        tail = routine_instance_chain_to_declaration_chain(next);
        if (tail == NULL)
            return NULL;
      }

    declaration = routine_instance_declaration(
            routine_instance_chain_instance(routine_chain));
    assert(declaration != NULL);

    result = create_routine_declaration_chain(declaration, tail);
    if (tail != NULL)
        routine_declaration_chain_remove_reference(tail);
    return result;
  }
