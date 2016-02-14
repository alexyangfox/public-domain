/* file "basket.h" */

/*
 *  This file contains the interface to the basket module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef BASKET_H
#define BASKET_H

#include <stddef.h>
#include "c_foundations/basic.h"


typedef enum basket_kind
  {
    BK_EXPRESSION,
    BK_LIST
  } basket_kind;

typedef struct basket basket;


#include "expression.h"


extern basket *create_expression_basket(expression *the_expression);
extern basket *create_list_basket(void);

extern void delete_basket(basket *the_basket);

extern basket_kind get_basket_kind(basket *the_basket);

extern expression *expression_basket_expression(basket *the_basket);
extern size_t list_basket_element_count(basket *the_basket);
extern const char *list_basket_label(basket *the_basket,
                                     size_t sub_basket_num);
extern basket *list_basket_sub_basket(basket *the_basket,
                                      size_t sub_basket_num);
extern boolean list_basket_force(basket *the_basket, size_t sub_basket_num);

extern verdict basket_add_sub_basket(basket *base, const char *label,
                                     basket *sub_basket, boolean force);


#endif /* BASKET_H */
