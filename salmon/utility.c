/* file "utility.c" */

/*
 *  This file contains the implementation of utility routines.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include "c_foundations/basic.h"
#include "utility.h"
#include "value.h"
#include "unicode.h"
#include "o_integer.h"
#include "execute.h"


extern value *c_string_to_value(const char *c_string)
  {
    value *result;
    const char *follow;

    if (c_string == NULL)
        return create_null_value();

    if (string_is_utf8(c_string))
        return create_string_value(c_string);

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
        return NULL;

    for (follow = c_string; *follow != 0; ++follow)
      {
        o_integer byte_oi;
        value *byte_value;
        verdict the_verdict;

        oi_create_from_long_int(byte_oi, (unsigned char)*follow);
        if (oi_out_of_memory(byte_oi))
          {
            value_remove_reference(result, NULL);
            return NULL;
          }

        byte_value = create_integer_value(byte_oi);
        oi_remove_reference(byte_oi);
        if (byte_value == NULL)
          {
            value_remove_reference(result, NULL);
            return NULL;
          }

        the_verdict = add_field(result, NULL, byte_value);
        value_remove_reference(byte_value, NULL);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            value_remove_reference(result, NULL);
            return NULL;
          }
      }
    return result;
  }

extern value *execute_call_from_arrays(value *base_value, size_t actual_count,
        const char *const *actual_names, value *const *actual_values,
        boolean expect_return_value, jumper *the_jumper,
        const source_location *location)
  {
    semi_labeled_value_list *pre_order_actuals;
    size_t actual_num;
    value *result;
    boolean was_flowing_forward;

    assert(base_value != NULL);
    assert((actual_count == 0) || (actual_values != NULL));
    assert(the_jumper != NULL);

    pre_order_actuals = create_semi_labeled_value_list();
    if (pre_order_actuals == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    for (actual_num = 0; actual_num < actual_count; ++actual_num)
      {
        verdict the_verdict;

        the_verdict = append_value_to_semi_labeled_value_list(
                pre_order_actuals,
                ((actual_names == NULL) ? NULL : actual_names[actual_num]),
                actual_values[actual_num]);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
            return NULL;
          }
      }

    result = execute_call_from_values(base_value, pre_order_actuals,
            expect_return_value, the_jumper, location);

    was_flowing_forward = jumper_flowing_forward(the_jumper);
    delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
    if (was_flowing_forward && (!(jumper_flowing_forward(the_jumper))) &&
        (result != NULL))
      {
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }
