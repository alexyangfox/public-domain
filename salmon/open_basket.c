/* file "open_basket.c" */

/*
 *  This file contains the implementation of the open_basket module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include "c_foundations/memory_allocation.h"
#include "open_basket.h"
#include "basket.h"
#include "unbound.h"


struct open_basket
  {
    basket *the_basket;
    unbound_name_manager *the_unbound_name_manager;
  };


extern open_basket *create_open_basket(basket *the_basket,
        unbound_name_manager *the_unbound_name_manager)
  {
    open_basket *result;

    result = MALLOC_ONE_OBJECT(open_basket);
    if (result == NULL)
      {
        if (the_basket != NULL)
            delete_basket(the_basket);
        if (the_unbound_name_manager != NULL)
            delete_unbound_name_manager(the_unbound_name_manager);
        return NULL;
      }

    result->the_basket = the_basket;
    result->the_unbound_name_manager = the_unbound_name_manager;

    return result;
  }

extern void delete_open_basket(open_basket *the_open_basket)
  {
    assert(the_open_basket != NULL);

    if (the_open_basket->the_basket != NULL)
        delete_basket(the_open_basket->the_basket);

    if (the_open_basket->the_unbound_name_manager != NULL)
        delete_unbound_name_manager(the_open_basket->the_unbound_name_manager);

    free(the_open_basket);
  }

extern basket *open_basket_basket(open_basket *the_open_basket)
  {
    assert(the_open_basket != NULL);

    return the_open_basket->the_basket;
  }

extern unbound_name_manager *open_basket_unbound_name_manager(
        open_basket *the_open_basket)
  {
    assert(the_open_basket != NULL);

    return the_open_basket->the_unbound_name_manager;
  }

extern void set_open_basket_basket(open_basket *the_open_basket,
                                   basket *the_basket)
  {
    assert(the_open_basket != NULL);

    the_open_basket->the_basket = the_basket;
  }

extern void set_open_basket_unbound_name_manager(open_basket *the_open_basket,
        unbound_name_manager *the_unbound_name_manager)
  {
    assert(the_open_basket != NULL);

    the_open_basket->the_unbound_name_manager = the_unbound_name_manager;
  }
