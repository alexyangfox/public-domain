/* file "bind.c" */

/*
 *  This file contains the implementation of the bind module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <assert.h>
#include <string.h>
#include "c_foundations/basic.h"
#include "c_foundations/diagnostic.h"
#include "bind.h"
#include "unbound.h"


extern verdict check_for_unbound(
        unbound_name_manager *the_unbound_name_manager)
  {
    verdict result;
    size_t name_count;
    size_t name_num;

    assert(the_unbound_name_manager != NULL);

    result = MISSION_ACCOMPLISHED;
    name_count = unbound_name_count(the_unbound_name_manager);
    for (name_num = 0; name_num < name_count; ++name_num)
      {
        unbound_name *the_unbound_name;
        size_t use_count;
        size_t use_num;

        the_unbound_name =
                unbound_name_number(the_unbound_name_manager, name_num);
        assert(the_unbound_name != NULL);

        use_count = unbound_name_use_count(the_unbound_name);
        for (use_num = 0; use_num < use_count; ++use_num)
          {
            unbound_use *use;
            const char *expression_or_statement = "expression";

            use = unbound_name_use_by_number(the_unbound_name, use_num);
            assert(use != NULL);

            switch (get_unbound_use_kind(use))
              {
                case UUK_VARIABLE_FOR_EXPRESSION:
                    location_error(unbound_use_location(use),
                            "Unbound identifier `%s' used as an expression.",
                            unbound_name_string(the_unbound_name));
                    result = MISSION_FAILED;
                    break;
                case UUK_ROUTINE_FOR_EXPORT_STATEMENT:
                    location_error(unbound_use_location(use),
                            "An export statement was not enclosed in any "
                            "class.");
                    result = MISSION_FAILED;
                    break;
                case UUK_ROUTINE_FOR_HIDE_STATEMENT:
                    location_error(unbound_use_location(use),
                            "A hide statement was not enclosed in any class.");
                    result = MISSION_FAILED;
                    break;
                case UUK_ROUTINE_FOR_RETURN_STATEMENT:
                    if (strcmp(unbound_name_string(the_unbound_name), "") != 0)
                      {
                        location_error(unbound_use_location(use),
                                "A return statement used unbound identifier "
                                "`%s'.",
                                unbound_name_string(the_unbound_name));
                        result = MISSION_FAILED;
                      }
                    break;
                case UUK_ROUTINE_FOR_ARGUMENTS_EXPRESSION:
                    if (strcmp(unbound_name_string(the_unbound_name), "") == 0)
                      {
                        /* Do nothing in this case -- it's OK because the
                         * arugments expression refers to the program's
                         * arguments in this case. */
                      }
                    else
                      {
                        location_error(unbound_use_location(use),
                                "An arguments expression used unbound "
                                "identifier `%s'.",
                                unbound_name_string(the_unbound_name));
                        result = MISSION_FAILED;
                      }
                    break;
                case UUK_ROUTINE_FOR_THIS_EXPRESSION:
                    if (strcmp(unbound_name_string(the_unbound_name), "") == 0)
                      {
                        /* Do nothing in this case -- it's OK because the
                         * arugments expression refers to the program's
                         * arguments in this case. */
                        location_error(unbound_use_location(use),
                                "A `this' expression was not enclosed in any "
                                "class.");
                      }
                    else
                      {
                        location_error(unbound_use_location(use),
                                "A `this' expression used unbound identifier "
                                "`%s'.",
                                unbound_name_string(the_unbound_name));
                      }
                    result = MISSION_FAILED;
                    break;
                case UUK_ROUTINE_FOR_OPERATOR_EXPRESSION:
                    break;
                case UUK_ROUTINE_FOR_OPERATOR_STATEMENT:
                    break;
                case UUK_LOOP_FOR_BREAK_STATEMENT:
                    expression_or_statement = "statement";
                case UUK_LOOP_FOR_BREAK_EXPRESSION:
                    if (strcmp(unbound_name_string(the_unbound_name), "") == 0)
                      {
                        location_error(unbound_use_location(use),
                                "A break %s was not enclosed in any loop.",
                                expression_or_statement);
                      }
                    else
                      {
                        location_error(unbound_use_location(use),
                                "A break %s used unbound identifier `%s'.",
                                expression_or_statement,
                                unbound_name_string(the_unbound_name));
                      }
                    result = MISSION_FAILED;
                    break;
                case UUK_LOOP_FOR_CONTINUE_STATEMENT:
                    expression_or_statement = "statement";
                case UUK_LOOP_FOR_CONTINUE_EXPRESSION:
                    if (strcmp(unbound_name_string(the_unbound_name), "") == 0)
                      {
                        location_error(unbound_use_location(use),
                                "A continue %s was not enclosed in any loop.",
                                expression_or_statement);
                      }
                    else
                      {
                        location_error(unbound_use_location(use),
                                "A continue %s used unbound identifier `%s'.",
                                expression_or_statement,
                                unbound_name_string(the_unbound_name));
                      }
                    result = MISSION_FAILED;
                    break;
                case UUK_DANGLING_OVERLOADED_ROUTINE:
                    break;
                case UUK_USE_FLOW_THROUGH:
                    break;
                default:
                    assert(FALSE);
              }
          }
      }

    return result;
  }
