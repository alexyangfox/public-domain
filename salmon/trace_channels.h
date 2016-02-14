/* file "trace_channels.h" */

/*
 *  This file contains the interface to the trace_channels module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef TRACE_CHANNELS_H
#define TRACE_CHANNELS_H


typedef enum
  {
    TC_CALLS = 0,
    TC_ASSIGNMENTS,
    TC_LOCKS,
    TC_COUNT
  } trace_channels;


extern const char *trace_channel_names[];


#endif /* TRACE_CHANNELS_H */
