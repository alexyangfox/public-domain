/* file "convertable.c" */

/*
 *  This file is part of a module for testing whether values can be converted
 *  to specified types.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include "../native_bridge.h"
#include "../type.h"
#include "../source_location.h"
#include "../driver.h"


extern value *convertable(value *all_arguments_value, context *the_context,
                          jumper *the_jumper, const source_location *location)
  {
    type *target;
    type *source;
    quark *tag;
    boolean doubt;
    boolean result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 3);
    target = type_value_data(value_component_value(all_arguments_value, 0));
    source = type_value_data(value_component_value(all_arguments_value, 1));
    tag = value_quark(value_component_value(all_arguments_value, 2));

    check_type_validity(target, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    check_type_validity(source, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    result = type_always_forceable_to(source, target, &doubt, location,
                                      the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    if (doubt)
      {
        static_exception_tag tag_info;

        tag_info.field_name = NULL;
        tag_info.u.quark = tag;
        location_exception(the_jumper, location, &tag_info,
                "%s could not determine whether type %t was convertable to "
                "%t.", interpreter_name(), source, target);
        return NULL;
      }

    return (result ? create_true_value() : create_false_value());
  }
