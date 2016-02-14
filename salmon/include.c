/* file "include.c" */

/*
 *  This file contains the implementation of the include module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/auto_array.h"
#include "c_foundations/auto_array_implementation.h"
#include "c_foundations/string_index.h"
#include "include.h"
#include "statement_block.h"
#include "unbound.h"
#include "parser.h"
#include "file_parser.h"
#include "platform_dependent.h"


AUTO_ARRAY(mutable_string_aa, char *);
AUTO_ARRAY(dynamic_library_handle_aa, dynamic_library_handle *);


typedef struct paths_data paths_data;

typedef enum
  {
    PK_ABSOLUTE,
    PK_INCLUDER,
    PK_INCLUDER_RELATIVE,
    PK_RUNER,
    PK_RUNER_RELATIVE
  } path_kind;

struct paths_data
  {
    path_kind kind;
    char *path;
    paths_data *next;
  };

typedef struct
  {
    char *including_directory;
    char *executable_directory;
    boolean owns_paths_data;
    paths_data *paths_data;
    string_index *currently_including;
  } local_file_include_data;


typedef verdict try_include_file_type(local_file_include_data *typed_data,
        char *include_file_name, void *data, const char *to_include,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        type_expression *interface_type_expression,
        unbound_name_manager **manager, const source_location *location,
        alias_manager *parent_alias_manager, boolean *cant_open_file,
        boolean native_bridge_dll_body_allowed);

AUTO_ARRAY(try_include_aa, try_include_file_type *);

static verdict init(void);
static local_file_include_data *create_local_data(const char *base_file_name,
        boolean owns_paths_data, paths_data *new_paths_data,
        char *executable_directory);
static verdict try_include_file(local_file_include_data *typed_data,
        char *include_file_name, void *data, const char *to_include,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        type_expression *interface_type_expression,
        unbound_name_manager **manager, const source_location *location,
        alias_manager *parent_alias_manager, boolean *cant_open_file,
        boolean native_bridge_dll_body_allowed);
static verdict try_salmon_include_file(local_file_include_data *typed_data,
        char *include_file_name, void *data, const char *to_include,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        type_expression *interface_type_expression,
        unbound_name_manager **manager, const source_location *location,
        alias_manager *parent_alias_manager, boolean *cant_open_file,
        boolean native_bridge_dll_body_allowed);
static verdict try_native_bridge_include_file(
        local_file_include_data *typed_data, char *include_file_name,
        void *data, const char *to_include,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        type_expression *interface_type_expression,
        unbound_name_manager **manager, const source_location *location,
        alias_manager *parent_alias_manager, boolean *cant_open_file,
        boolean native_bridge_dll_body_allowed);
static verdict create_include_parser(void *data, const char *to_include,
        const source_location *location, alias_manager *parent_alias_manager,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        type_expression *interface_type_expression,
        unbound_name_manager **manager,
        boolean native_bridge_dll_body_allowed);
static void free_paths_data(paths_data *to_free);
static char *include_file_for_path_data(paths_data *this_path,
        const char *to_include, local_file_include_data *data);
static dynamic_library_handle *find_dynamic_library(const char *to_include,
        boolean *file_not_found, char **error_message);


AUTO_ARRAY_IMPLEMENTATION(string_aa, const char *, 0);
AUTO_ARRAY_IMPLEMENTATION(mutable_string_aa, char *, 0);
AUTO_ARRAY_IMPLEMENTATION(dynamic_library_handle_aa, dynamic_library_handle *,
                          0);
AUTO_ARRAY_IMPLEMENTATION(try_include_aa, try_include_file_type *, 0);


static try_include_aa includers;
static boolean initialized = FALSE;
DECLARE_SYSTEM_LOCK(dll_lookup_lock);
static string_index *dll_lookup = NULL;
static dynamic_library_handle_aa open_plugins;


extern verdict init_include_module(void)
  {
    verdict the_verdict;

    INITIALIZE_SYSTEM_LOCK(dll_lookup_lock, return MISSION_FAILED);

    dll_lookup = create_string_index();
    if (dll_lookup == NULL)
      {
        DESTROY_SYSTEM_LOCK(dll_lookup_lock);
        return MISSION_FAILED;
      }

    the_verdict = dynamic_library_handle_aa_init(&open_plugins, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        destroy_string_index(dll_lookup);
        DESTROY_SYSTEM_LOCK(dll_lookup_lock);
        return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

extern void *create_local_file_include_handler_data(const char *base_file_name,
        const char *directory_paths, const char *executable_directory)
  {
    verdict the_verdict;
    paths_data *paths_data_start;
    paths_data **paths_data_end;
    const char *follow;
    char *executable_directory_copy;
    void *result;

    the_verdict = init();
    if (the_verdict != MISSION_ACCOMPLISHED)
         return NULL;

    assert(directory_paths != NULL);

    paths_data_start = NULL;
    paths_data_end = &paths_data_start;

    follow = directory_paths;
    while (*follow != 0)
      {
        const char *end;
        paths_data *new_data;
        const char *path_start;

        end = follow;
        while ((*end != 0) && (*end != PATH_SEPARATOR[0]))
            ++end;

        new_data = MALLOC_ONE_OBJECT(paths_data);
        if (new_data == NULL)
          {
            free_paths_data(paths_data_start);
            return NULL;
          }

        if ((strncmp(follow, "ID", strlen("ID")) == 0) &&
            (end == follow + strlen("ID")))
          {
            new_data->kind = PK_INCLUDER;
            path_start = NULL;
          }
        else if (strncmp(follow, "ID/", strlen("ID/")) == 0)
          {
            new_data->kind = PK_INCLUDER_RELATIVE;
            path_start = follow + strlen("ID/");
          }
        else if ((strncmp(follow, "R", strlen("R")) == 0) &&
                 (end == follow + strlen("R")))
          {
            new_data->kind = PK_RUNER;
            path_start = NULL;
          }
        else if (strncmp(follow, "R/", strlen("R/")) == 0)
          {
            new_data->kind = PK_RUNER_RELATIVE;
            path_start = follow + strlen("R/");
          }
        else if (strncmp(follow, "ABS/", strlen("ABS/")) == 0)
          {
            new_data->kind = PK_ABSOLUTE;
            path_start = follow + strlen("ABS/");
          }
        else
          {
            new_data->kind = PK_ABSOLUTE;
            path_start = follow;
          }

        if (path_start == NULL)
          {
            new_data->path = NULL;
          }
        else
          {
            new_data->path = MALLOC_ARRAY(char, ((end - path_start) + 1));
            if (new_data->path == NULL)
              {
                free(new_data);
                free_paths_data(paths_data_start);
                return NULL;
              }

            memcpy(new_data->path, path_start, (end - path_start));
            new_data->path[end - path_start] = 0;
          }

        new_data->next = NULL;
        assert(*paths_data_end == NULL);
        *paths_data_end = new_data;
        paths_data_end = &(new_data->next);
        assert(*paths_data_end == NULL);

        if (*end == 0)
            follow = end;
        else
            follow = end + 1;
      }

    assert(*paths_data_end == NULL);

    if (executable_directory == NULL)
      {
        executable_directory_copy = NULL;
      }
    else
      {
        executable_directory_copy =
                MALLOC_ARRAY(char, strlen(executable_directory) + 1);
        if (executable_directory_copy == NULL)
          {
            free_paths_data(paths_data_start);
            return NULL;
          }

        strcpy(executable_directory_copy, executable_directory);
      }

    result = create_local_data(base_file_name, TRUE, paths_data_start,
                               executable_directory_copy);
    if (result == NULL)
      {
        if (executable_directory_copy != NULL)
            free(executable_directory_copy);
        free_paths_data(paths_data_start);
      }
    return result;
  }

extern verdict local_file_include_handler(void *data, const char *to_include,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        const source_location *location, alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed)
  {
    verdict the_verdict;

    assert(data != NULL);
    assert(to_include != NULL);
    assert(the_statement_block != NULL);
    assert(parent_manager != NULL);
    assert(current_labels != NULL);

    the_verdict = init();
    if (the_verdict != MISSION_ACCOMPLISHED)
         return the_verdict;

    return create_include_parser(data, to_include, location,
            parent_alias_manager, the_statement_block, parent_manager,
            current_labels, NULL, NULL, native_bridge_dll_body_allowed);
  }

extern verdict local_file_interface_include_handler(void *data,
        const char *to_include, type_expression *interface_type_expression,
        unbound_name_manager **manager, const source_location *location,
        alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed)
  {
    verdict the_verdict;

    assert(data != NULL);
    assert(to_include != NULL);
    assert(interface_type_expression != NULL);
    assert(manager != NULL);

    the_verdict = init();
    if (the_verdict != MISSION_ACCOMPLISHED)
         return the_verdict;

    return create_include_parser(data, to_include, location,
            parent_alias_manager, NULL, NULL, NULL, interface_type_expression,
            manager, native_bridge_dll_body_allowed);
  }

extern dynamic_library_handle *open_dynamic_library_in_path(void *data,
        const char *to_include, boolean *file_not_found, char **error_message)
  {
    local_file_include_data *typed_data;

    assert(data != NULL);
    assert(to_include != NULL);

    typed_data = (local_file_include_data *)data;

    if (path_is_absolute(to_include))
      {
        return find_dynamic_library(to_include, file_not_found, error_message);
      }
    else
      {
        paths_data *follow_paths;

        for (follow_paths = typed_data->paths_data; TRUE;
             follow_paths = follow_paths->next)
          {
            char *include_file_name;
            boolean local_file_not_found;
            dynamic_library_handle *result;

            if (follow_paths == NULL)
              {
                *file_not_found = TRUE;
                return NULL;
              }

            include_file_name = include_file_for_path_data(follow_paths,
                    to_include, typed_data);
            if (include_file_name == NULL)
              {
                *file_not_found = FALSE;
                *error_message = NULL;
                return NULL;
              }

            result = find_dynamic_library(include_file_name,
                    &local_file_not_found, error_message);
            free(include_file_name);

            if (result != NULL)
                return result;
            if (!local_file_not_found)
              {
                *file_not_found = FALSE;
                return NULL;
              }
          }
      }
  }

extern void delete_local_file_include_handler_data(void *data)
  {
    local_file_include_data *typed_data;

    assert(initialized);
    assert(data != NULL);

    typed_data = (local_file_include_data *)data;

    if (typed_data->including_directory != NULL)
        free(typed_data->including_directory);

    if (typed_data->owns_paths_data)
      {
        free_paths_data(typed_data->paths_data);

        free(typed_data->executable_directory);
      }

    destroy_string_index(typed_data->currently_including);

    free(typed_data);
  }

extern void cleanup_include_module(void)
  {
    size_t plugin_num;

    initialized = FALSE;
    free(includers.array);
    DESTROY_SYSTEM_LOCK(dll_lookup_lock);
    destroy_string_index(dll_lookup);
    for (plugin_num = 0; plugin_num < open_plugins.element_count; ++plugin_num)
      {
        dynamic_library_handle *handle;
        void (*cleaner)(void);

        handle = open_plugins.array[plugin_num];

        cleaner = (void (*)(void))(find_dynamic_library_symbol(handle,
                "salmoneye_plugin_clean_up"));
        if (cleaner != NULL)
            (*cleaner)();

        close_dynamic_library(handle);
      }
    free(open_plugins.array);
  }


static verdict init(void)
  {
    verdict the_verdict;

    if (initialized)
        return MISSION_ACCOMPLISHED;

    the_verdict = try_include_aa_init(&includers, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_verdict = try_include_aa_append(&includers, &try_salmon_include_file);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(includers.array);
        return the_verdict;
      }

    the_verdict =
            try_include_aa_append(&includers, &try_native_bridge_include_file);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(includers.array);
        return the_verdict;
      }

    initialized = TRUE;
    return MISSION_ACCOMPLISHED;
  }

static local_file_include_data *create_local_data(const char *base_file_name,
        boolean owns_paths_data, paths_data *new_paths_data,
        char *executable_directory)
  {
    local_file_include_data *result;
    boolean error;

    assert(base_file_name != NULL);

    result = MALLOC_ONE_OBJECT(local_file_include_data);
    if (result == NULL)
        return NULL;

    result->including_directory = file_name_directory(base_file_name, &error);
    if (error)
      {
        free(result);
        return NULL;
      }

    result->executable_directory = executable_directory;

    result->owns_paths_data = owns_paths_data;
    result->paths_data = new_paths_data;

    result->currently_including = create_string_index();
    if (result->currently_including == NULL)
      {
        free(result);
        return NULL;
      }

    return result;
  }

static verdict try_include_file(local_file_include_data *typed_data,
        char *include_file_name, void *data, const char *to_include,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        type_expression *interface_type_expression,
        unbound_name_manager **manager, const source_location *location,
        alias_manager *parent_alias_manager, boolean *cant_open_file,
        boolean native_bridge_dll_body_allowed)
  {
    verdict the_verdict;
    int includer_num;

    if (lookup_in_string_index(typed_data->currently_including,
                               include_file_name) != NULL)
      {
        location_error(location, "Include cycle through `%s' detected.",
                       include_file_name);
        if (cant_open_file != NULL)
            *cant_open_file = TRUE;
        return MISSION_FAILED;
      }

    the_verdict = enter_into_string_index(typed_data->currently_including,
            include_file_name, include_file_name);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    for (includer_num = 0; includer_num < includers.element_count;
         ++includer_num)
      {
        verdict the_verdict;
        boolean local_cant_open;

        the_verdict = (*(includers.array[
                includers.element_count - (includer_num + 1)]))(typed_data,
                include_file_name, data, to_include, the_statement_block,
                parent_manager, current_labels, interface_type_expression,
                manager, location, parent_alias_manager, &local_cant_open,
                native_bridge_dll_body_allowed);
        if (cant_open_file != NULL)
            *cant_open_file = local_cant_open;
        if (the_verdict == MISSION_ACCOMPLISHED)
            return the_verdict;
        if (local_cant_open)
          {
            remove_from_string_index(typed_data->currently_including,
                                     include_file_name);
            return the_verdict;
          }
      }

    if (cant_open_file != NULL)
        *cant_open_file = FALSE;
    remove_from_string_index(typed_data->currently_including,
                             include_file_name);
    return MISSION_FAILED;
  }

static verdict try_salmon_include_file(local_file_include_data *typed_data,
        char *include_file_name, void *data, const char *to_include,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        type_expression *interface_type_expression,
        unbound_name_manager **manager, const source_location *location,
        alias_manager *parent_alias_manager, boolean *cant_open_file,
        boolean native_bridge_dll_body_allowed)
  {
    file_parser *the_file_parser;
    local_file_include_data *sub_data;
    verdict the_verdict;

    the_file_parser = create_file_parser(include_file_name, NULL, location,
            &local_file_include_handler, &local_file_interface_include_handler,
            data, parent_alias_manager, cant_open_file,
            native_bridge_dll_body_allowed);
    if (the_file_parser == NULL)
        return MISSION_FAILED;

    sub_data = create_local_data(include_file_name, FALSE,
            typed_data->paths_data, typed_data->executable_directory);
    if (sub_data == NULL)
      {
        delete_file_parser(the_file_parser);
        if (cant_open_file != NULL)
            *cant_open_file = FALSE;
        return MISSION_FAILED;
      }

    if (the_statement_block != NULL)
      {
        parser *include_parser;

        assert(parent_manager != NULL);
        assert(current_labels != NULL);
        assert(interface_type_expression == NULL);
        assert(manager == NULL);

        include_parser = file_parser_parser(the_file_parser);

        the_verdict = parse_statements_for_statement_block(include_parser,
                the_statement_block, parent_manager, current_labels);
        if (the_verdict == MISSION_ACCOMPLISHED)
            the_verdict = verify_end_of_input(include_parser);
      }
    else
      {
        assert(parent_manager == NULL);
        assert(current_labels == NULL);
        assert(interface_type_expression != NULL);
        assert(manager != NULL);

        the_verdict = parse_interface_item_list(
                file_parser_parser(the_file_parser), interface_type_expression,
                manager, TK_END_OF_INPUT);
      }

    delete_file_parser(the_file_parser);
    delete_local_file_include_handler_data(sub_data);

    if ((the_verdict != MISSION_ACCOMPLISHED) && (cant_open_file != NULL))
        *cant_open_file = TRUE;

    if (the_verdict == MISSION_ACCOMPLISHED)
      {
        remove_from_string_index(typed_data->currently_including,
                                 include_file_name);
      }

    return the_verdict;
  }

static verdict try_native_bridge_include_file(
        local_file_include_data *typed_data, char *include_file_name,
        void *data, const char *to_include,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        type_expression *interface_type_expression,
        unbound_name_manager **manager, const source_location *location,
        alias_manager *parent_alias_manager, boolean *cant_open_file,
        boolean native_bridge_dll_body_allowed)
  {
    boolean file_not_found;
    char *error_message;
    dynamic_library_handle *handle;
    native_bridge_function_info *(*table_generator)();
    size_t (*count_generator)();
    const char *(*source_file_name_generator)();
    native_bridge_function_info *table;
    size_t count;
    const char *source_file_name;
    verdict result;

    if (the_statement_block == NULL)
      {
        if (cant_open_file != NULL)
            *cant_open_file = FALSE;
        return MISSION_FAILED;
      }

    if (!(is_dynamic_library_name(include_file_name)))
      {
        if (cant_open_file != NULL)
            *cant_open_file = FALSE;
        return MISSION_FAILED;
      }

    handle = find_dynamic_library(include_file_name, &file_not_found,
                                  &error_message);

    if ((handle == NULL) && file_not_found)
      {
        char **alternates;

        alternates = alternate_dynamic_library_names(include_file_name);
        if (alternates != NULL)
          {
            size_t alt_num;

            for (alt_num = 0; alternates[alt_num] != NULL; ++alt_num)
              {
                handle = find_dynamic_library(alternates[alt_num],
                                              &file_not_found, &error_message);
                if ((handle != NULL) || !file_not_found)
                    break;
              }

            for (alt_num = 0; alternates[alt_num] != NULL; ++alt_num)
                free(alternates[alt_num]);
            free(alternates);
          }
      }

    if (handle == NULL)
      {
        if (!file_not_found)
          {
            if (cant_open_file != NULL)
                *cant_open_file = TRUE;
            if (error_message == NULL)
              {
                location_error(location,
                        "Failed trying to open native extension file `%s'.",
                        include_file_name);
              }
            else
              {
                location_error(location,
                        "Failed trying to open native extension file `%s': "
                        "%s.", include_file_name, error_message);
                free(error_message);
              }
          }
        else
          {
            if (cant_open_file != NULL)
                *cant_open_file = FALSE;
          }
        return MISSION_FAILED;
      }

    const char *statement_table_name = "salmon_native_bridge_statement_table";
    const char *statement_count_name = "salmon_native_bridge_statement_count";
    const char *source_file_name_name =
            "salmon_native_bridge_source_file_name";

    table_generator = (native_bridge_function_info *(*)())
            (find_dynamic_library_symbol(handle, statement_table_name));
    if (table_generator == NULL)
      {
        location_error(location,
                "Can't find symbol `%s' in native extension file `%s'.",
                statement_table_name, include_file_name);
        if (cant_open_file != NULL)
            *cant_open_file = TRUE;
        return MISSION_FAILED;
      }

    count_generator = (size_t (*)())(find_dynamic_library_symbol(handle,
            statement_count_name));
    if (count_generator == NULL)
      {
        location_error(location,
                "Can't find symbol `%s' in native extension file `%s'.",
                statement_count_name, include_file_name);
        if (cant_open_file != NULL)
            *cant_open_file = TRUE;
        return MISSION_FAILED;
      }

    source_file_name_generator = (const char *(*)())
            (find_dynamic_library_symbol(handle, source_file_name_name));
    if (source_file_name_generator == NULL)
      {
        location_error(location,
                "Can't find symbol `%s' in native extension file `%s'.",
                source_file_name_name, include_file_name);
        if (cant_open_file != NULL)
            *cant_open_file = TRUE;
        return MISSION_FAILED;
      }

    table = (*table_generator)();
    count = (*count_generator)();
    source_file_name = (*source_file_name_generator)();

    result = parse_statements_for_statement_block_from_native_bridge(table,
            count, source_file_name, the_statement_block, parent_manager,
            current_labels, parent_alias_manager);
    if (result != MISSION_ACCOMPLISHED)
      {
        if (cant_open_file != NULL)
            *cant_open_file = TRUE;
      }
    else
      {
        remove_from_string_index(typed_data->currently_including,
                                 include_file_name);
      }
    return result;
  }

static verdict create_include_parser(void *data, const char *to_include,
        const source_location *location, alias_manager *parent_alias_manager,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        type_expression *interface_type_expression,
        unbound_name_manager **manager, boolean native_bridge_dll_body_allowed)
  {
    local_file_include_data *typed_data;

    assert(data != NULL);
    assert(to_include != NULL);

    typed_data = (local_file_include_data *)data;

    if (path_is_absolute(to_include))
      {
        return try_include_file(typed_data, (char *)to_include, data,
                to_include, the_statement_block, parent_manager,
                current_labels, interface_type_expression, manager, location,
                parent_alias_manager, NULL, native_bridge_dll_body_allowed);
      }
    else
      {
        paths_data *follow_paths;

        for (follow_paths = typed_data->paths_data; TRUE;
             follow_paths = follow_paths->next)
          {
            char *include_file_name;
            boolean cant_open_file;
            verdict the_verdict;

            if (follow_paths == NULL)
              {
                location_error(location, "Can't find include file `%s'.",
                               to_include);
                return MISSION_FAILED;
              }

            include_file_name = include_file_for_path_data(follow_paths,
                    to_include, typed_data);
            if (include_file_name == NULL)
                return MISSION_FAILED;

            assert(include_file_name != NULL);

            the_verdict = try_include_file(typed_data, include_file_name, data,
                    to_include, the_statement_block, parent_manager,
                    current_labels, interface_type_expression, manager,
                    location, parent_alias_manager, &cant_open_file,
                    native_bridge_dll_body_allowed);
            free(include_file_name);

            if (the_verdict == MISSION_ACCOMPLISHED)
                return the_verdict;
            if (cant_open_file)
                return the_verdict;
          }
      }
  }

static void free_paths_data(paths_data *to_free)
  {
    paths_data *follow;

    follow = to_free;
    while (follow != NULL)
      {
        paths_data *going;

        if (follow->path != NULL)
            free(follow->path);
        going = follow;
        follow = follow->next;
        free(going);
      }
  }

static char *include_file_for_path_data(paths_data *this_path,
        const char *to_include, local_file_include_data *data)
  {
    char *result;

    assert(this_path != NULL);
    assert(to_include != NULL);
    assert(data != NULL);

    switch (this_path->kind)
      {
        case PK_ABSOLUTE:
          do_absolute:
            assert(this_path->path != NULL);

            result = MALLOC_ARRAY(char,
                    strlen(this_path->path) + strlen(to_include) + 2);
            if (result == NULL)
                return NULL;

            sprintf(result, "%s/%s", this_path->path, to_include);

            break;
        case PK_INCLUDER:
            if (data->including_directory == NULL)
              {
              do_simple:
                result = MALLOC_ARRAY(char, strlen(to_include) + 1);
                if (result == NULL)
                    return NULL;

                strcpy(result, to_include);

                break;
              }

            result = MALLOC_ARRAY(char,
                    strlen(data->including_directory) + strlen(to_include) +
                    2);
            if (result == NULL)
                return NULL;

            sprintf(result, "%s/%s", data->including_directory, to_include);

            break;
        case PK_INCLUDER_RELATIVE:
            assert(this_path->path != NULL);

            if (data->including_directory == NULL)
                goto do_absolute;

            result = MALLOC_ARRAY(char,
                    strlen(data->including_directory) + strlen(this_path->path)
                    + strlen(to_include) + 3);
            if (result == NULL)
                return NULL;

            sprintf(result, "%s/%s/%s", data->including_directory,
                    this_path->path, to_include);

            break;
        case PK_RUNER:
            if (data->executable_directory == NULL)
                goto do_simple;

            result = MALLOC_ARRAY(char,
                    strlen(data->executable_directory) + strlen(to_include) +
                    2);
            if (result == NULL)
                return NULL;

            sprintf(result, "%s/%s", data->executable_directory, to_include);

            break;
        case PK_RUNER_RELATIVE:
            assert(this_path->path != NULL);

            if (data->executable_directory == NULL)
                goto do_absolute;

            result = MALLOC_ARRAY(char,
                    strlen(data->executable_directory) +
                    strlen(this_path->path) + strlen(to_include) + 3);
            if (result == NULL)
                return NULL;

            sprintf(result, "%s/%s/%s", data->executable_directory,
                    this_path->path, to_include);

            break;
        default:
            assert(FALSE);
      }

    return result;
  }

static dynamic_library_handle *find_dynamic_library(const char *to_include,
        boolean *file_not_found, char **error_message)
  {
    dynamic_library_handle *result;

    GRAB_SYSTEM_LOCK(dll_lookup_lock);

    result = lookup_in_string_index(dll_lookup, to_include);
    if (result != NULL)
      {
        RELEASE_SYSTEM_LOCK(dll_lookup_lock);
        return result;
      }

    result = open_dynamic_library(to_include, file_not_found, error_message);

    if (result != NULL)
      {
        verdict (*initializer)(void);
        verdict the_verdict;

        initializer = (verdict (*)(void))(find_dynamic_library_symbol(result,
                "salmoneye_plugin_initialize"));
        if (initializer != NULL)
          {
            verdict the_verdict;

            the_verdict = (*initializer)();
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                RELEASE_SYSTEM_LOCK(dll_lookup_lock);
                location_error(NULL, "Initialization of `%s' failed.",
                               to_include);
                close_dynamic_library(result);
                *file_not_found = FALSE;
                *error_message = NULL;
                return NULL;
              }
          }

        the_verdict = enter_into_string_index(dll_lookup, to_include, result);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            RELEASE_SYSTEM_LOCK(dll_lookup_lock);
            close_dynamic_library(result);
            *file_not_found = FALSE;
            *error_message = NULL;
            return NULL;
          }

        the_verdict = dynamic_library_handle_aa_append(&open_plugins, result);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            RELEASE_SYSTEM_LOCK(dll_lookup_lock);
            close_dynamic_library(result);
            *file_not_found = FALSE;
            *error_message = NULL;
            return NULL;
          }
      }

    RELEASE_SYSTEM_LOCK(dll_lookup_lock);

    return result;
  }
