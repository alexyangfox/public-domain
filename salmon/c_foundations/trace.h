/* file "trace.h" */

/*
 *  This file contains the interface to code for doing tracing.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef TRACE_H
#define TRACE_H

#include <stdio.h>
#include <stdarg.h>


/*
 *      Usage
 *
 *  This module provides a mechanism for doing tracing.  What we mean by
 *  tracing is giving information to the user about what is happening within a
 *  piece of software in the form of a series of text messages.  The user must
 *  explicitly turn on dumping of these messages to access them.  The user
 *  turns them on through command-line options.
 *
 *  Tracing is organized into channels, where each channel corresponds to a
 *  particular set of events that the user might be interested in.  The user
 *  can pick and choose which channels to see.  For each channel, the user can
 *  decide to send the trace information for that channel to standard output,
 *  standard error, or to a named file.
 *
 *  This module is not hard-coded for a particular set of, or even number of,
 *  tracing channels.  This module may be used by different programs that use
 *  different numbers of tracing channels.
 *
 *  The interface to this module consists of one type and 13 functions.
 *
 *  The one type is the ``tracer'' structure.  This structure is declared here,
 *  but its contents are private, so clients of this module can only use
 *  pointers to tracer objects.  A single tracer object manages multiple trace
 *  channels.  Typically, a program will only need a single tracer object.
 *
 *  At the start of the program, the client code should create a tracer object
 *  with the create_tracer() function.  It must tell this object about the
 *  tracing channels it will be using.  The first parameter to create_tracer()
 *  is the number of tracing channels.  This must be at least one -- if there
 *  are zero tracing channels, it's pointless to create the tracer object in
 *  the first place.  The second parameter to create_tracer() specifies the
 *  names of those channels.  It is an array of channel_count elements, each of
 *  which must be a non-NULL character pointer pointing to a string specifying
 *  the name of the corresponding channel.
 *
 *  These names are used in two ways.  The first way is to form the names of
 *  command-line options.  For each channel, there will be three command-line
 *  options, of the form ``-trace-XXX-stdout'', ``-trace-XXX-stderr'', and
 *  ``-trace-XXX-file''.  For example, if a trace channel is named ``locks'',
 *  then there will be command-line options named ``-trace-locks-stdout'',
 *  ``trace-locks-stderr'', and ``-trace-locks-file''.  The first two
 *  command-line options take no argument and the third takes a single file
 *  name argument.
 *
 *  The second way in which channel names are used is in writing out the actual
 *  trace data.  Each trace item is put on a line of its own with the channel
 *  name followed by a colon and a space as a prefix.  This allows trace output
 *  items from one channel to be distinguished from those of another channel
 *  even in the same file and makes it easy to extract particular channels from
 *  a file with a script.
 *
 *  Note that the tracer doesn't make a copy of the channel name table, so it
 *  is illegal to de-allocate the table or any of the strings in it until the
 *  tracer object is de-allocated.  Typically, the table of channel names will
 *  be statically allocated, so this will not be an issue.
 *
 *  Optionally, after creating the tracer object, a function may be supplied to
 *  do the formatting of text to be sent out to a file.  If this function is
 *  not supplied, the default will be to do formatting in the same way as other
 *  parts of this library, such as the diagnostic message handling code.  The
 *  reason the client code is allowed to supply its own function to do the
 *  formatting is to support client code that implements its own formatting
 *  directives.  For example, the client code could implement a ``%K''
 *  directive that handles a data type that is specific to that client code,
 *  since the %K format specifier is unused in the standard formatting
 *  specification.  The tracer_set_output_handler() function is used to allow
 *  the client code to set its own function to do formatting.  Once set, the
 *  new function will be used for all formatting done by that particular tracer
 *  object.
 *
 *  Next, the client code should use the handle_tracer_option() function as
 *  part of its command-line parsing.  This function should be called on each
 *  command-line argument that might be a tracing option.  This function is
 *  given a list of remaining command-line options and checks to see whether
 *  the start of the arguments is a tracing command-line option.  It returns
 *  the number of items that it used, or a negative number to indicate an
 *  error.  It returns zero for the case that the arguments given to it don't
 *  start with a tracing option.  Note that in many cases the
 *  handle_tracer_option() function will need to be called several times to
 *  handle the case of tracing options appearing a different places in the
 *  command line.  The argv parameter should point to the first remaining
 *  command-line argument and the argc parameter should specify how many
 *  remaining command-line arguments there are.
 *
 *  Optionally, the client code may also use three more command-line
 *  option-related functions provided by this module: tracer_option_count(),
 *  tracer_option_pattern(), and tracer_option_description().  These three
 *  functions provide information about the command-line options that the
 *  tracer makes available.  They are intended for usage messages to tell the
 *  user what options are available.  The first of these functions,
 *  tracer_option_count(), tells the client code how many command-line options
 *  apply to this tracer.  The second, tracer_option_pattern(), returns a text
 *  string specifying the pattern in human-readable for for the specified
 *  command-line option.  For example, if ``lock'' is the name of one of the
 *  trace channels, one of the patterns will be "-trace-lock-file <file>".  The
 *  third function, tracer_option_description(), returns a text string
 *  describing, in English, what the specified command-line option means.
 *
 *  Next, there are six functions that allow the client code to specify trace
 *  output data.  If the specified channel doesn't have its output going
 *  anywhere, then these functions have no effect for that channel.  These
 *  functions can be used throughout the run of the client code.
 *
 *  The first of these six is the trace() function.  This is like printf() but
 *  with two additional arguments: a pointer to the tracer and a channel
 *  number.  If the specified channel has its output going somewhere, the
 *  format and additional arguments are used to generate a trace item that is
 *  sent out to the appropriate stream.
 *
 *  The vtrace() function is a variant of the trace() function that uses a
 *  va_list argument instead of ``...''.  This is analagous to the vprintf()
 *  function.
 *
 *  The final four of these six function are used when the output for a trace
 *  item doesn't nicely fit into a single formatted print call.  In this case,
 *  the client code should call open_trace_item() to specify the channel and
 *  get the output going, then call trace_text() and/or vtrace_text() one or
 *  more times, and then call close_trace_item().  Note that the channel number
 *  is only specified by the open_trace_item() call and the other calls
 *  implicitly go to the same channel.  It's illegal to call open_trace_item()
 *  again before calling close_trace_item().  The calls to trace_text() and
 *  vtrace_text() add formatted data to the trace item.
 *
 *  Finally, when all the execution that might use tracing is done, the client
 *  code should call delete_tracer() to cleanly close any open output files and
 *  to de-allocate the tracer object.
 *
 *
 *      Error Handling
 *
 *  The create_tracer() function returns NULL if it detects an error and the
 *  handle_tracer_option() function returns a negative number if it detects an
 *  error.  In addition, each of these functions sends a diagnostic message
 *  about the error before returning, so the client code doesn't have to send
 *  its own diagnostic.
 *
 *  The delete_tracer(), tracer_set_output_handler(), tracer_option_count(),
 *  tracer_option_pattern(), and tracer_option_description() functions don't
 *  have any error conditions that they are able to detect, so they don't
 *  return any status information.
 *
 *  The remaining six functions -- trace(), vtrace(), open_trace_item(),
 *  trace_text(), vtrace_text(), and close_trace_item() -- can encounter error
 *  conditions that the could detect and report back to the calling code.  But
 *  they do not do so.  That's because these functions are used to generate
 *  tracing information, and thus are intended to be spread throughout code
 *  that is doing other things.  The intention of this module is to allow
 *  tracing to be added to other code with minimal impact to that other code.
 *  Since testing for and handling errors on these calls would cause
 *  potentially complicated interactions with the code containing these calls,
 *  we wish to avoid all of that.  And any detectible errors during tracing
 *  won't directly affect the rest of the code, so it's safe to continue with
 *  whatever else is going on.
 *
 *  Of course, the rest of this section doesn't apply if there are bugs in this
 *  module, if there are bugs in the compiler compiling this module that cause
 *  incorrect code to be generated for it, if external memory corruption bugs
 *  corrupt the data structures or code used by this module, or if pointer
 *  parameters passed to this module are corrupt.  In those cases, all bets are
 *  off and there's no way to guarantee what might happen.
 *
 *
 *      Requirements
 *
 *  This module requires only an ANSI C compiler and other parts of the
 *  c_foundataions library, which itself only requires an ANSI C compiler.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, in 2009 and placed in the public
 *  domain at that time.  I've written somewhat similar code at least once
 *  before.  I believe one case where I wrote somewhat similar code was for
 *  Silicon Spice and/or Broadcom, when I was doing simulator work.  In that
 *  case, it was written for software to do cycle-by-cycle simulation of parts
 *  of computer chips and the tracing was to help debug (at different times)
 *  the simulator; the hardware which was being simulated; and software being
 *  developed on the simulator to run on the actual hardware.  Some of that
 *  version was based on ideas from earlier, less sophisticated, tracing
 *  implementations written by others for earlier simulators.
 *
 *  The present code, while incorporating some of the same ideas as the earlier
 *  code, was written entirely by me from scratch, without reference to any of
 *  that earlier code, either written by me or by others.  At the time of
 *  writing of the present code, I don't even know where to find any of that
 *  earlier code.
 *
 *  I've created this code right now because I have an immediate need for it
 *  for the CWX1/Salmon Interpreter Zero that I'm working on.  But I designed
 *  it to be generic enough to be useful in other circumstances.  If I'm ever
 *  working on another simulator written in C, I am likely to use this module
 *  for that, and I may find uses for it in other C code I write.  I also
 *  intend to make it available by itself, in case anyone else finds it useful
 *  to incorporate into their own code.
 *
 *
 *      Legal Issues
 *
 *  I've written this code from scratch, without using or refering to any other
 *  code.  I've written it on my own equipment and not for hire for anyone
 *  else, so I have full legal rights to place it in the public domain.
 *
 *  I've chosen to put this software in the public domain rather than
 *  copyrighting it and using the FSF's GPL or a Berkeley-style ``vanity''
 *  license because my personal opinion is that making it public domain
 *  maximizes its potential usefulness to others.  Anyone can feel free to use
 *  it for any purpose, including with their own proprietary code or with GPL
 *  code, without fear of intellectual property issues.  I have no desire to
 *  stop anyone else from making money on this code or getting any other
 *  advantages they can from it.
 *
 *  I do request that anyone who finds this software useful let me know about
 *  it.  You can drop me e-mail at "Chris Wilson" <chris@chriswilson.info> to
 *  let me know how you are using it and what is good and bad about it for you.
 *  Bug reports are also appreciated.  Also, if you release a product or
 *  software package of some sort that includes this software, I would like you
 *  to give me credit in the documentation as appropriate for the importance of
 *  my code in your product.  These are requests, not requirements, so you are
 *  not legally bound to do them, they're just a nice way to show appreciation.
 *
 *  Note that even though this software is public domain and there are no
 *  copyright laws that limit what you can do with it, other laws may apply.
 *  For example, if you lie and claim that you wrote this code when you did
 *  not, or you claim that I endorse a product of yours when I do not, that
 *  could be fraud and you could be legally liable.
 *
 *  There is absolutely no warranty for this software!  I am warning you now
 *  that it may or may not work.  It may have bugs that cause you a lot of
 *  problems.  I disclaim any implied warranties for merchantability or fitness
 *  for a particular purpose.  The fact that I have written some documentation
 *  on what I intended this software for should not be taken as any kind of
 *  warranty that it will actually behave that way.  I am providing this
 *  software as-is in the hope that it will be useful.
 *
 *          Chris Wilson, 2003, 2007, 2009
 */

typedef struct tracer tracer;


extern tracer *create_tracer(size_t channel_count, const char **channel_names);
extern void delete_tracer(tracer *the_tracer);

extern void tracer_set_output_handler(tracer *the_tracer,
        void (*handler)(FILE *fp, const char *format, va_list arg));

extern int handle_tracer_option(tracer *the_tracer, int argc, char *argv[]);

extern size_t tracer_option_count(tracer *the_tracer);
extern const char *tracer_option_pattern(tracer *the_tracer,
                                         size_t option_num);
extern const char *tracer_option_description(tracer *the_tracer,
                                             size_t option_num);

extern void trace(tracer *the_tracer, size_t channel, const char *format, ...);
extern void vtrace(tracer *the_tracer, size_t channel, const char *format,
                   va_list arg);

extern void open_trace_item(tracer *the_tracer, size_t channel);
extern void trace_text(tracer *the_tracer, const char *format, ...);
extern void vtrace_text(tracer *the_tracer, const char *format, va_list arg);
extern void close_trace_item(tracer *the_tracer);


#endif /* TRACE_H */
