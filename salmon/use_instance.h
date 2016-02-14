/* file "use_instance.h" */

/*
 *  This file contains the interface to the use_instance module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef USE_INSTANCE_H
#define USE_INSTANCE_H

#include <stddef.h>


typedef struct use_instance use_instance;


#include "instance.h"
#include "routine_instance_chain.h"
#include "reference_cluster.h"


extern use_instance *create_use_instance(size_t used_for_count,
                                         reference_cluster *cluster);

extern void delete_use_instance(use_instance *the_use_instance,
                                jumper *the_jumper);

extern boolean use_instance_is_instantiated(use_instance *the_use_instance);
extern instance *use_instance_instance(use_instance *the_use_instance,
                                       size_t used_for_number);
extern routine_instance_chain *use_instance_chain(
        use_instance *the_use_instance, size_t used_for_number);

extern void use_instance_set_instantiated(use_instance *the_use_instance);
extern void use_instance_set_instance(use_instance *the_use_instance,
        size_t used_for_number, instance *the_instance);
extern void use_instance_set_chain(use_instance *the_use_instance,
        size_t used_for_number, routine_instance_chain *chain);


#endif /* USE_INSTANCE_H */
