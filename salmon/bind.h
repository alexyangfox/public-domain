/* file "bind.h" */

/*
 *  This file contains the interface to the bind module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef BIND_H
#define BIND_H

#include "c_foundations/basic.h"
#include "unbound.h"


extern verdict check_for_unbound(
        unbound_name_manager *the_unbound_name_manager);


#endif /* BIND_H */
