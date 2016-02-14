/* file "basket_instance.h" */

/*
 *  This file contains the interface to the basket_instance module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef BASKET_INSTANCE_H
#define BASKET_INSTANCE_H

#include "c_foundations/basic.h"


typedef enum
  {
    BIK_SLOT,
    BIK_LIST
  } basket_instance_kind;

typedef struct basket_instance basket_instance;


#include "slot_location.h"


extern basket_instance *create_slot_basket_instance(
        slot_location *the_slot_location);
extern basket_instance *create_list_basket_instance(size_t element_count);

extern void delete_basket_instance(basket_instance *the_basket_instance,
                                   jumper *the_jumper);

extern basket_instance_kind get_basket_instance_kind(
        basket_instance *the_basket_instance);
extern slot_location *slot_basket_instance_slot(
        basket_instance *the_basket_instance);
extern size_t list_basket_instance_element_count(
        basket_instance *the_basket_instance);
extern const char *list_basket_instance_label(
        basket_instance *the_basket_instance, size_t child_num);
extern basket_instance *list_basket_instance_child(
        basket_instance *the_basket_instance, size_t child_num);
extern boolean list_basket_instance_force(basket_instance *the_basket_instance,
                                          size_t child_num);

extern verdict set_list_basket_instance_label(
        basket_instance *the_basket_instance, size_t child_num,
        const char *label);
extern verdict set_list_basket_instance_child(
        basket_instance *the_basket_instance, size_t child_num,
        basket_instance *child, boolean force);

#endif /* BASKET_INSTANCE_H */
