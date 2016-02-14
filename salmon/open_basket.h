/* file "open_basket.h" */

/*
 *  This file contains the interface to the open_basket module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef OPEN_BASKET_H
#define OPEN_BASKET_H


typedef struct open_basket open_basket;


#include "basket.h"
#include "unbound.h"


extern open_basket *create_open_basket(basket *the_basket,
        unbound_name_manager *the_unbound_name_manager);

extern void delete_open_basket(open_basket *the_open_basket);

extern basket *open_basket_basket(open_basket *the_open_basket);
extern unbound_name_manager *open_basket_unbound_name_manager(
        open_basket *the_open_basket);

extern void set_open_basket_basket(open_basket *the_open_basket,
                                   basket *the_basket);
extern void set_open_basket_unbound_name_manager(open_basket *the_open_basket,
        unbound_name_manager *the_unbound_name_manager);


#endif /* OPEN_BASKET_H */
