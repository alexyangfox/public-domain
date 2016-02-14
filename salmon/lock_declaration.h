/* file "lock_declaration.h" */

/*
 *  This file contains the interface to the lock_declaration module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef LOCK_DECLARATION_H
#define LOCK_DECLARATION_H

#include "c_foundations/basic.h"


typedef struct lock_declaration lock_declaration;


#include "expression.h"
#include "declaration.h"


extern lock_declaration *create_lock_declaration(expression *single_lock);

extern void lock_declaration_add_reference(lock_declaration *declaration);
extern void lock_declaration_remove_reference(lock_declaration *declaration);
extern void delete_lock_declaration(lock_declaration *declaration);

extern declaration *lock_declaration_declaration(
        lock_declaration *declaration);
extern const char *lock_declaration_name(lock_declaration *declaration);
extern boolean lock_declaration_is_static(lock_declaration *declaration);
extern boolean lock_declaration_is_virtual(lock_declaration *declaration);
extern expression *lock_declaration_single_lock(lock_declaration *declaration);
extern boolean lock_declaration_automatic_allocation(
        lock_declaration *declaration);

extern void set_lock_declaration_declaration(
        lock_declaration *the_lock_declaration, declaration *the_declaration);


#endif /* LOCK_DECLARATION_H */
