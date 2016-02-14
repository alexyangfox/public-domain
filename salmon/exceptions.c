/* file "exceptions.c" */

/*
 *  This file contains the implementation of the exceptions module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include "jumper.h"

#undef DEFINE_EXCEPTION_TAG


#define DEFINE_EXCEPTION_TAG(tag)  \
        static_exception_tag exception_tag_ ## tag = \
          { "exception_tag_" #tag, { NULL } };

#include "all_exceptions.h"
