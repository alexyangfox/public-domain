/* file "lock_chain.h" */

/*
 *  This file contains the interface to the lock_chain module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef LOCK_CHAIN_H
#define LOCK_CHAIN_H


typedef struct lock_chain lock_chain;

#include "lock_instance.h"
#include "source_location.h"
#include "jumper.h"
#include "reference_cluster.h"


extern lock_chain *create_lock_chain(lock_instance *head,
                                     lock_chain *remainder);

extern lock_instance *lock_chain_head(lock_chain *chain);
extern lock_chain *lock_chain_remainder(lock_chain *chain);

extern void lock_chain_grab(lock_chain *chain, const source_location *location,
                            jumper *the_jumper);
extern void lock_chain_release(lock_chain *chain,
        const source_location *location, jumper *the_jumper);

extern void lock_chain_add_reference(lock_chain *chain);
extern void lock_chain_remove_reference(lock_chain *chain, jumper *the_jumper);
extern void lock_chain_add_reference_with_cluster(lock_chain *chain,
                                                  reference_cluster *cluster);
extern void lock_chain_remove_reference_with_cluster(lock_chain *chain,
        reference_cluster *cluster, jumper *the_jumper);


#endif /* LOCK_CHAIN_H */
