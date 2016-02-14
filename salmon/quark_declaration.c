/* file "quark_declaration.c" */

/*
 *  This file contains the implementation of the quark_declaration module.
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
#include <string.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "quark_declaration.h"
#include "declaration.h"


struct quark_declaration
  {
    declaration *declaration;
  };


extern quark_declaration *create_quark_declaration(void)
  {
    quark_declaration *result;

    result = MALLOC_ONE_OBJECT(quark_declaration);
    if (result == NULL)
        return NULL;

    result->declaration = NULL;

    return result;
  }

extern void quark_declaration_add_reference(quark_declaration *declaration)
  {
    assert(declaration != NULL);

    declaration_add_reference(declaration->declaration);
  }

extern void quark_declaration_remove_reference(quark_declaration *declaration)
  {
    assert(declaration != NULL);

    declaration_remove_reference(declaration->declaration);
  }

extern void delete_quark_declaration(quark_declaration *declaration)
  {
    assert(declaration != NULL);

    free(declaration);
  }

extern declaration *quark_declaration_declaration(
        quark_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration->declaration;
  }

extern const char *quark_declaration_name(quark_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_name(declaration->declaration);
  }

extern boolean quark_declaration_is_static(quark_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_is_static(declaration->declaration);
  }

extern boolean quark_declaration_is_virtual(quark_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_is_virtual(declaration->declaration);
  }

extern boolean quark_declaration_automatic_allocation(
        quark_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_automatic_allocation(declaration->declaration);
  }

extern void set_quark_declaration_declaration(
        quark_declaration *the_quark_declaration, declaration *the_declaration)
  {
    assert(the_quark_declaration != NULL);
    assert(the_declaration != NULL);

    assert(the_quark_declaration->declaration == NULL);
    the_quark_declaration->declaration = the_declaration;
  }
