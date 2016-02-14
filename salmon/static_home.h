/* file "static_home.h" */

/*
 *  This file contains the interface to the static_home module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef STATIC_HOME_H
#define STATIC_HOME_H


typedef struct static_home static_home;


#include "declaration.h"


extern static_home *create_static_home(size_t static_count,
                                       declaration **static_declarations);

extern void delete_static_home(static_home *static_home);

extern size_t static_home_declaration_count(static_home *static_home);
extern declaration *static_home_declaration_by_number(static_home *static_home,
                                                      size_t declaration_num);


#endif /* STATIC_HOME_H */
