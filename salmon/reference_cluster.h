/* file "reference_cluster.h" */

/*
 *  This file contains the interface to the reference_cluster module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef REFERENCE_CLUSTER_H
#define REFERENCE_CLUSTER_H


typedef struct reference_cluster reference_cluster;


#include "object.h"
#include "context.h"


extern reference_cluster *create_object_reference_cluster(object *the_object);
extern reference_cluster *create_context_reference_cluster(
        context *the_context);

extern void delete_reference_cluster(reference_cluster *cluster);

extern void reference_cluster_add_reference(reference_cluster *cluster);
extern void reference_cluster_remove_reference(reference_cluster *cluster,
                                               jumper *the_jumper);
extern void reference_cluster_add_internal_reference(
        reference_cluster *cluster);
extern void reference_cluster_remove_internal_reference(
        reference_cluster *cluster);


#endif /* REFERENCE_CLUSTER_H */
