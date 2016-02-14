/* file "semi_labeled_value_list.h" */

/*
 *  This file contains the interface to the semi_labeled_value_list module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef SEMI_LABELED_VALUE_LIST_H
#define SEMI_LABELED_VALUE_LIST_H

#include "c_foundations/basic.h"


typedef struct semi_labeled_value_list semi_labeled_value_list;


#include "value.h"
#include "jumper.h"


extern semi_labeled_value_list *create_semi_labeled_value_list(void);

extern void delete_semi_labeled_value_list(
        semi_labeled_value_list *the_semi_labeled_value_list,
        jumper *the_jumper);

extern size_t semi_labeled_value_list_value_count(
        semi_labeled_value_list *the_semi_labeled_value_list);
extern const char *semi_labeled_value_list_label(
        semi_labeled_value_list *the_semi_labeled_value_list,
        size_t value_num);
extern value *semi_labeled_value_list_value(
        semi_labeled_value_list *the_semi_labeled_value_list,
        size_t value_num);

extern verdict append_value_to_semi_labeled_value_list(
        semi_labeled_value_list *the_semi_labeled_value_list,
        const char *label, value *the_value);


#endif /* SEMI_LABELED_VALUE_LIST_H */
