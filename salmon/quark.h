/* file "quark.h" */

/*
 *  This file contains the interface to the quark module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef QUARK_H
#define QUARK_H

#include "c_foundations/basic.h"


typedef struct quark quark;


#include "quark_declaration.h"
#include "instance.h"
#include "reference_cluster.h"
#include "jumper.h"
#include "purity_level.h"


extern quark *create_quark(quark_declaration *declaration, purity_level *level,
                           reference_cluster *cluster);

extern quark_declaration *quark_instance_declaration(quark *the_quark);
extern boolean quark_is_instantiated(quark *the_quark);
extern boolean quark_scope_exited(quark *the_quark);
extern instance *quark_instance_instance(quark *the_quark);

extern verdict set_quark_scope_exited(quark *the_quark);

extern void quark_add_reference(quark *the_quark);
extern void quark_remove_reference(quark *the_quark, jumper *the_jumper);
extern void quark_add_reference_with_cluster(quark *the_quark,
                                             reference_cluster *cluster);
extern void quark_remove_reference_with_cluster(quark *the_quark,
        jumper *the_jumper, reference_cluster *cluster);
extern reference_cluster *quark_reference_cluster(quark *the_quark);

extern boolean quarks_are_equal(quark *quark1, quark *quark2);
extern int quark_structural_order(quark *left, quark *right);

extern verdict init_quark_module(void);
extern void cleanup_quark_module(void);


#endif /* QUARK_H */
