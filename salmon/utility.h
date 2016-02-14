/* file "utility.h" */

/*
 *  This file contains the interface to utility routines.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef UTILITY_H
#define UTILITY_H

#include "value.h"


extern value *c_string_to_value(const char *c_string);
extern value *execute_call_from_arrays(value *base_value, size_t actual_count,
        const char *const *actual_names, value *const *actual_values,
        boolean expect_return_value, jumper *the_jumper,
        const source_location *location);


#endif /* UTILITY_H */
