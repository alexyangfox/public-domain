/* file "trace_channels.c" */

/*
 *  This file contains the implementation of the trace_channels module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include "trace_channels.h"


const char *trace_channel_names[] =
  {
    "calls",
    "assignments",
    "locks"
  };
