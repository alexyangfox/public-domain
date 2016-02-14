/* file "purity_level.h" */

/*
 *  This file contains the interface to the purity_level module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef PURITY_LEVEL_H
#define PURITY_LEVEL_H

#include "c_foundations/basic.h"


typedef struct purity_level purity_level;


#include "lock_instance.h"


extern purity_level *create_purity_level(purity_level *parent);

extern size_t purity_level_depth(purity_level *level);
extern purity_level *moved_level(purity_level *level);
extern purity_level *purity_level_first_level(purity_level *level);
extern size_t purity_level_sticky_lock_instance_count(purity_level *level);
extern lock_instance *purity_level_sticky_lock_instance(purity_level *level,
                                                        size_t instance_num);

extern void purity_level_move_out(purity_level *level);
extern verdict purity_level_add_sticky_lock_instance(purity_level *level,
                                                     lock_instance *instance);

extern void purity_level_add_reference(purity_level *level);
extern void purity_level_remove_reference(purity_level *level);


#endif /* PURITY_LEVEL_H */
