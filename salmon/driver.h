/* file "driver.h" */

/*
 *  This file contains an interface for the functionality that the top-level
 *  driver for this interpreter needs to provide to other parts of the
 *  interpreter.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef DRIVER_H
#define DRIVER_H


typedef struct salmoneye_lock salmoneye_lock;


#include "value.h"
#include "c_foundations/basic.h"
#include "c_foundations/trace.h"
#include "open_statement_block.h"


extern verdict init_salmoneye(void);
extern void cleanup_salmoneye(void);
extern int run_salmoneye(int argc, char *argv[]);
extern value *c_argv_to_salmon_arguments(int argc, char *argv[]);
extern int run_salmon_source_file(const char *source_file_name, int argc,
        char *argv[], boolean profile, boolean list_leak_count,
        boolean list_leak_details, boolean native_bridge_dll_body_allowed,
        tracer *the_tracer, const char *directory_paths,
        char *executable_directory);
extern open_statement_block *parse_statement_block_from_file(
        const char *source_file_name, boolean native_bridge_dll_body_allowed,
        const char *directory_paths, char *executable_directory);

extern const char *interpreter_name(void);

extern salmoneye_lock *create_salmoneye_lock(void);
extern void destroy_salmoneye_lock(salmoneye_lock *lock);
extern void lock_salmoneye_lock(salmoneye_lock *lock);
extern void unlock_salmoneye_lock(salmoneye_lock *lock);

extern void *salmoneye_allocate(size_t byte_count);
extern void salmoneye_free(void *to_free);


#endif /* DRIVER_H */
