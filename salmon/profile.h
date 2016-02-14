/* file "profile.h" */

/*
 *  This file contains the interface to the profile module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef PROFILE_H
#define PROFILE_H

#include <stdio.h>
#include "routine_declaration.h"


extern verdict init_profile_module(void);
extern void cleanup_profile_module(void);

extern void profile_register_routine(routine_declaration *routine);
extern void dump_profile_listing(FILE *fp);


#endif /* PROFILE_H */
