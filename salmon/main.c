/* file "main.c" */

/*
 *  This file contains the main program for SalmonEye.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <string.h>
#include <stdlib.h>
#include "c_foundations/diagnostic.h"
#ifdef CHECK_MEMORY_ALLOCATION
#include "c_foundations/memory_allocation_test.h"
#endif /* CHECK_MEMORY_ALLOCATION */
#include "driver.h"
#include "platform_dependent.h"


static int try_running_through_dll(int argc, char *argv[], boolean *dll_fail);


extern int main(int argc, char *argv[])
  {
    verdict the_verdict;
    int return_code;

#ifndef ALWAYS_USE_SALMON_DLL
    if ((argc >= 2) && (strcmp(argv[1], "-use-se-dll") == 0))
#endif /* ALWAYS_USE_SALMON_DLL */
      {
        boolean dll_fail;
        int return_code;

        return_code = try_running_through_dll(argc, argv, &dll_fail);
        if (!dll_fail)
            return return_code;
      }

    the_verdict = init_salmoneye();
    if (the_verdict != MISSION_ACCOMPLISHED)
        return -1;

    return_code = run_salmoneye(argc, argv);

    cleanup_salmoneye();

#ifdef CHECK_MEMORY_ALLOCATION

    check_blocks();

    if (unfreed_blocks() != 0)
      {
        basic_error(
                "Memory leak: %lu block%s of memory %s allocated but never "
                "de-allocated.", (unsigned long)(unfreed_blocks()),
                ((unfreed_blocks() == 1) ? "" : "s"),
                ((unfreed_blocks() == 1) ? "was" : "were"));
        describe_unfreed_blocks();
        return 1;
      }

#endif /* CHECK_MEMORY_ALLOCATION */

    return return_code;
  }

static int try_running_through_dll(int argc, char *argv[], boolean *dll_fail)
  {
    boolean file_not_found;
    char *error_message;
    dynamic_library_handle *handle;
    verdict (*init_salmoneye)(void);
    int (*run_salmoneye)(int argc, char *argv[]);
    void (*cleanup_salmoneye)(void);
    verdict the_verdict;
    int return_code;
    void (*check_blocks)(void);
    size_t (*unfreed_blocks)(void);

    handle = open_dynamic_library(SALMONEYE_DLL_NAME, &file_not_found,
                                  &error_message);
    if (handle == NULL)
      {
        if ((!file_not_found) && (error_message != NULL))
            free(error_message);
        *dll_fail = TRUE;
        return 0;
      }

    init_salmoneye = (verdict (*)(void))(find_dynamic_library_symbol(handle,
            "init_salmoneye"));
    run_salmoneye = (int (*)(int argc, char *argv[]))
            (find_dynamic_library_symbol(handle, "run_salmoneye"));
    cleanup_salmoneye = (void (*)(void))(find_dynamic_library_symbol(handle,
            "cleanup_salmoneye"));

    if ((init_salmoneye == NULL) || (run_salmoneye == NULL) ||
        (cleanup_salmoneye == NULL))
      {
        close_dynamic_library(handle);
        *dll_fail = TRUE;
        return 0;
      }

    *dll_fail = FALSE;

    the_verdict = (*init_salmoneye)();
    if (the_verdict != MISSION_ACCOMPLISHED)
        return -1;

    return_code = (*run_salmoneye)(argc, argv);

    (*cleanup_salmoneye)();

    check_blocks = (void (*)(void))(find_dynamic_library_symbol(handle,
            "check_blocks"));
    if (check_blocks != NULL)
        (*check_blocks)();

    unfreed_blocks = (size_t (*)(void))(find_dynamic_library_symbol(handle,
            "unfreed_blocks"));
    if ((unfreed_blocks != NULL) && ((*unfreed_blocks)() != 0))
      {
        void (*describe_unfreed_blocks)(void);

        basic_error(
                "Memory leak: %lu block%s of memory %s allocated but never "
                "de-allocated.", (unsigned long)((*unfreed_blocks)()),
                (((*unfreed_blocks)() == 1) ? "" : "s"),
                (((*unfreed_blocks)() == 1) ? "was" : "were"));

        describe_unfreed_blocks = (void (*)(void))(find_dynamic_library_symbol(
                handle, "describe_unfreed_blocks"));
        if (describe_unfreed_blocks != NULL)
            (*describe_unfreed_blocks)();

        return 1;
      }

    close_dynamic_library(handle);

    return return_code;
  }
