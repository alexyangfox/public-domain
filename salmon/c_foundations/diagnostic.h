/* file "diagnostics.h" */

/*
 *  This file contains the interface to code for handling diagnostic messages.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stddef.h>
#include <stdarg.h>
#include "basic.h"


/*
 *      Usage
 *
 *  This module acts as an intermediary between pieces of code that generate
 *  diagnostic messages and code that formats and presents those messages to
 *  the user.  It also contains a default method of presenting those messages
 *  to the user, so that no code to handle the messages and present them to the
 *  user needs to be provided in cases where the default mechanism is adequate.
 *  By having this module be the common interface, other modules that generate
 *  diagnostic messages can be made portable across applications that route
 *  those messages to a user in very different ways.  The module generating the
 *  messages doesn't have to be aware of whether the messages go to the user by
 *  printing text to standard output on a console, dialog boxes in a windowing
 *  system, or through some other mechanism.
 *
 *  The interface to this module is composed of two pieces: the piece that is
 *  to be used by code that generates diagnostic messages and the piece that is
 *  to be used by code that handles diagnostic messages.
 *
 *  The first piece, that for code that generates diagnostic messages, starts
 *  with the three simplest ways to generate diagnostic messages: the
 *  basic_error(), basic_warning(), and basic_notice() functions.  The
 *  interface of each of these functions is the same: it takes a format
 *  character string and optional variable arguments as specified by the
 *  format.  The arguments have the same meaning that they do in a printf()
 *  statement.  Unlike the printf(), statement, however, these functions have
 *  no return value -- they are assumed to succeed and have no way to report
 *  back an error.
 *
 *  There are three of these functions for the three classifications of
 *  diagnostic messages used by this module.  The first is an error message.
 *  This is used to indicate something that is definitely wrong, and that
 *  likely keeps the program from doing what it was asked to do.  The second
 *  kind of diagnostic message is a warning message.  This is intended for
 *  messages to alert the user about something that looks suspicious, that may
 *  well be a problem, but isn't definitely a problem.  Often, the application
 *  will treat errors and warnings very differently, aborting a job after an
 *  error message but not after a warning message, for example.  There's no
 *  requirement built in to this module about the difference between an error
 *  and warning, however.  The third type of diagnostic message is a notice.  A
 *  notice is used for a message that isn't a warning or an error, just some
 *  information to pass to the user.  A notice is generally intended for
 *  information that the program doesn't see as a problem or possible problem,
 *  just information that that user might be interested in.
 *
 *  As the names of the first three functions -- basic_error(),
 *  basic_warning(), and basic_notice() -- imply, they are used to generate
 *  error messages, warning messages, and notices respectively.
 *
 *  The next 21 functions are variations on these first three.  There are 7
 *  variants, and each has an error, a warning, and a notice version, for a
 *  total of 21 variants.  Each function generates a single diagnostic message,
 *  has no return value, and takes a format and variable arguments to specify
 *  the text of the message.  But, in addition, each of the variants has some
 *  other information that it passes in addition to the format and related
 *  arguments.  The 7 variant forms each pass different additional information.
 *
 *  The first of the variant forms consists of the file_line_error(),
 *  file_line_warning(), and file_line_notice() functions.  Each of these
 *  functions adds a file name and line number to the base information for the
 *  diagnostic message it generates.  These variants are appropriate when the
 *  diagnostic message refers specifically to an input text file.  For example,
 *  a compiler could use these functions to point the user to a specific place
 *  in a program file that contains an error or potential error.  The file name
 *  and line number are usually sufficient to show the user what the source of
 *  the error is.  Note that it is legal to pass a NULL pointer as a file name
 *  -- this indicates that only a line number should be specified to the user,
 *  not a file name.  This can be appropriate when the input is piped for
 *  standard input, or when the input is in a buffer in memory instead of a
 *  file, for example.  In those cases, it makes sense to show the user a line
 *  number but not a file name.
 *
 *  The next variant form consists of the file_character_error(),
 *  file_character_warning(), and file_character_notice() functions.  Each of
 *  these functions is the same as the one in the previous group except that it
 *  adds one additional piece of information, a column number.  This allows the
 *  location in the input file that led to the problem or potential problem to
 *  be even more precisely specified and shown to the user.  The column number
 *  specifies how many characters in to the specified line the character that
 *  starts the problem occurs.
 *
 *  The next variant form consists of the file_selection_error(),
 *  file_selection_warning(), and file_selection_notice() functions.  Each of
 *  these functions adds one more piece of information to that specified by
 *  those of the previous group: a character count.  This character count
 *  allows both the start and end of the relevant section of the input file to
 *  be specified.  The file name, line number, and column number together
 *  specify the starting character exactly, and the character count specifies
 *  how many characters are being referred to.  This might allow a source
 *  editor, for example, to highlight the exact word, expression, or other
 *  piece of the source that the message refers to.
 *
 *  The next four variants are formed by taking the first four and to each
 *  adding a message number.  The message number is intended for display to the
 *  user, for cases when it's useful for the user to have a way to refer to a
 *  specific diagnostic message.  For example, if the user phones tech support,
 *  being able to provide the exact number of an error message might cut down
 *  on confusion.  Or the documentation might allow diagnostic messages to be
 *  looked up by message number.  Or the application itself might allow the
 *  user to turn warnings on and off by warning number, for example as
 *  command-line flags.  Or the application might use the message number to
 *  supress multiple copies of the same basic error or warning.  And there may
 *  be other uses for this number, too -- anything that having a unique handle
 *  for a given kind of message might allow.
 *
 *  In all of the functions described so far, a single function provides all
 *  the information for a single diagnostic message.  No state is kept from one
 *  function call to apply to later diagnostic messages.  But this module also
 *  provides some alternate interfaces where the information for one diagnostic
 *  message can come from more than one function call.
 *
 *  The next eight functions work this way.  They are the
 *  set_diagnostic_source_file_name(), unset_diagnostic_source_file_name(),
 *  set_diagnostic_source_line_number(), unset_diagnostic_source_line_number(),
 *  set_diagnostic_source_column_number(),
 *  unset_diagnostic_source_column_number(),
 *  set_diagnostic_source_character_count(), and
 *  unset_diagnostic_source_character_count() functions.  These eight functions
 *  are organized into pairs where each pair has names that differe only by the
 *  fact that one starts with ``set_'' and the other starts with ``unset_''.
 *  Each of the four ``set_'' functions sets a value that is used with all
 *  subsequent messages until another call to the same function changes it to a
 *  different value or a call to the ``unset_'' version changes it back to the
 *  original state of not have a value for that parameter that applies to
 *  subsequent messages.  Each of the four values is one of the values that can
 *  be passed in some of the variants of the generation functions: the file
 *  name, the line number, the column number, and the character count.  If the
 *  same value is specified in the generating function, the pre-set value is
 *  ignored, but if the generating function does not specify a value for that
 *  parameter, the one set by the ``set_'' function is used.
 *
 *  The next seven function provide a way to further split up the information
 *  for one message across several function calls.  The open_error(),
 *  open_warning(), or open_notice() function is used to start a message.
 *  Then, the diagnostic_text() or vdiagnostic_text() function is used one or
 *  more times to provide text for the message.  When all the text for one
 *  message has been provided, the close_diagnostic() function is called to
 *  mark the end of the message.  And, optionally, the
 *  assign_diagnostic_number() function can be called between the open_*
 *  function and the close_diagnostic() function to provide a message number.
 *  This allows messages that would be more difficult to compose as a single
 *  function call.  For example, a loop through a list could be used to
 *  generate a message, with each iteration calling diagnostic_text() to add to
 *  the message, without the complexity for the client code of allocating and
 *  managing a buffer to write the message into.
 *
 *  That completes the first part of the interface, the one for code that
 *  generates diagnostic messages.
 *
 *  The second part of the interface is for code that handles diagnostic
 *  messages.  This part starts with the diagnostic_kind type.  This type is
 *  used to specify whether a given message is an error message, warning
 *  message, or notice.
 *
 *  Next are two functions, get_diagnostic_handler() and
 *  set_diagnostic_handler().  These two are used to get the current value of
 *  and set, respectively, a pointer to a function that is held internally by
 *  this module.  It is this function that is called once for each diagnostic
 *  message to present it to the user.
 *
 *  Along with the function pointer is a generic data pointer, to provide any
 *  data that the function needs.  The user provides both the data and function
 *  pointer with the set_diagnostic_handler() function and gets the current
 *  value with the get_diagnostic_handler() function.  The function to get the
 *  current values is provided so that the client code can restore the old
 *  handler when it is done or so that it can use the old handler for certain
 *  messages or after munging or filtering them in some way.
 *
 *  The type of the handler is a function that returns no value and takes the
 *  generic data pointer that was installed along with the function pointer in
 *  addition to the format and argument list that specify the message, plus all
 *  the optional data that can be specified by the user when generating a
 *  message -- the file name, line number, column number, character count, and
 *  message number -- plus flags to specify which of those pieces of data are
 *  present, and a ``kind'' parameter to specify whether the message is an
 *  error message, warning message, or notice.  Note that there is no flag to
 *  specify whether the file name has been provided -- instead, a NULL pointer
 *  is passed to indicate no file name.
 *
 *
 *      Error Handling
 *
 *  Note that each of the functions in this module has a void return type.
 *  None of them has the ability to signal an error to its caller.  That's
 *  because the whole point of this module is to handle diagnostic messages.
 *  If handling a diagnostic message itself runs into problems, it's likely
 *  that trying to send a diagnostic message about the diagnostic message would
 *  also give an error.  And there'd be no end to the error handling if the
 *  client code tried to catch all those errors.  So instead, if this module
 *  has an error, it simply does its best to recover and deal with the
 *  situation, tries to use its own interfaces to send an error message if it
 *  can.  It doesn't bother trying to notify the caller directly.
 *
 *  Many of the functions in this module don't have failure modes anyway.  Most
 *  of the generating functions just call the handler function and don't
 *  interact with anything else that can generate an error condition.  The
 *  handler getting and setting functions don't have any detectible failure
 *  modes either.  The default handler can fail while trying to actually print
 *  the message to standard error, either because of file I/O errors or a bad
 *  format string or format data.  In these cases, it simply prints as much of
 *  the message as it can and returns.
 *
 *  The only other kind of failure mode this module can detect is a failure in
 *  memory allocation for buffers while using the diagnostic_text() or
 *  vdiagnostic_text() functions.  In this case, this module will generate an
 *  error message through its own handler function to specify a memory
 *  allocation failure.
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
 *  This requires only an ANSI C compiler.
 *
 *
 *      Design Goals
 *
 *  No diagnostic message interface is going to be able to be the best thing
 *  for all situations.  The decisions made in designing this interface reflect
 *  the functionality I would either have liked to have had in the past or
 *  imagine myself using in the future.
 *
 *  First of all, one design decision was to provide a relatively large number
 *  of functions in this interface.  This has the advantage of providing
 *  functions taylored to a large number of situations, but the disadvantage of
 *  making the interface more complex to understand and the function names
 *  longer, to distinguish more clearly all the different variants of similar
 *  functionality.  I would imagine that any given piece of code that uses this
 *  module would only use a handful of these functions.  So I would also
 *  imagine that some pieces of code will define local aliases for the
 *  functions in this interface that they most commonly use.  Note that the
 *  identifiers ``error'', ``warning'', and ``notice'' are not used here, so a
 *  particular piece of code can use those names to map to whichever variants
 *  of the functions given here it most often uses.  This also means that code
 *  that defines its own ``error'' or ``warning'' function or variable for its
 *  own purposes won't have namespace conflicts with this module.
 *
 *  The core of this module's interface is functions that generate a single
 *  diagnostic message each.  There are eight variants on the information
 *  passed to the generation functions.  For each of those eight, there are
 *  three forms, one for error messages, one for warning messages, and one for
 *  notice messages.  Having a dedicated version of every generator for each
 *  kind of mesage -- error, warning, or notice -- makes this interface larger
 *  than if the kind of message was passed as a parameter but makes the call
 *  sites smaller.  Since there are likely to be so many callsites for this
 *  functions, I judged it to be right trade-off.
 *
 *  Each of the generation functions uses a character string format and varargs
 *  to specify printf-style the text of the message.  Many of the generation
 *  functions provide additional information that can be used when presenting
 *  the message to the user.  Note that each of the stand-alone generating
 *  functions uses the ``...'' form of varargs, where the arguments are
 *  specified directly at the callsite, not the va_args form.  That means that
 *  if the client code writes a wrapper that itself takes a format and varargs,
 *  it can't pass that format and arguments on to one of these generating
 *  functions.  But that's OK because such as wrapper can use the open_/close_
 *  functions and the vdiagnostic_text() function.  In fact, it most likely
 *  would need that anyway because a wrapper would most likely be adding
 *  something to the message.  And the number of callsites like this is likely
 *  to be small, so requiring several functions instead of just one is OK for
 *  those cases.  It is the places where the format and variable argument list
 *  are actually being specified where it's important to keep the call sites
 *  simple.
 *
 *  The first triplet of functions in this interface -- the basic_error(),
 *  basic_warning(), and basic_notice() functions -- provide the simplest
 *  generator interface.  In each the client code supplies the format and
 *  varargs and nothing more.  Many clients of this module are likely to only
 *  ever use these three generators.  They were an obvious choice to provide
 *  because then the client code doesn't have to provide any more information.
 *  This is a minimal interface for a generator.  I think it's likely that the
 *  majority of generator calls will be to these three functions in most code.
 *
 *  The next triplet -- the file_line_error(), file_line_warning(), and
 *  file_line_notice() functions -- add a file name and line number to the set
 *  of information provided by the client.  In my experience, this is the most
 *  common way to point to a place in a source text document that generated an
 *  error.  Generally, lines are short enough that this is all the precision
 *  that's needed to tell the user quickly exactly what caused the problem.
 *  This is not my preference, though -- I tend to think that if one is going
 *  to bother to keep track of a source position in a file that caused a
 *  problem and point a user to it for a diagnostic message, one might as well
 *  keep track of the column of the problematic section of the file and the
 *  number of characters of that section, too.  The user doesn't always need to
 *  be given this information, but I think it makes sense to try to keep track
 *  of it and pass it to the diagnostic message interface and then have the
 *  other side of the interface decide whether or not to use that information.
 *  If the other side of the interface highlights a section of text in a source
 *  browser window, for example, having that column and character count
 *  information will really come in handy.  Even with text-based output, the
 *  code to handle the diagnostic message might print the line in question and
 *  then print a line with carets under the offending characters on the
 *  previous line.  But since many other people seem to use the file name/line
 *  number convention, and other people's debugging information formats will
 *  often only include that, it seemed useful to have forms of the generation
 *  functions that only take a file name and line number.
 *
 *  The third triplet of generator functions add a column number to the file
 *  name and line number.  This provides another commonly-useful level of
 *  precision.  And the fourth triplet provides the character count, too.  This
 *  provides everything needed to pinpoint precisely one contiguous section of
 *  code to associate with the message.  The goal in the code I write will be
 *  to try to generate this information for any errors associated with
 *  particular parts of a source file.
 *
 *  This wouldn't have to be the end of it.  Additional information could be
 *  allowed that would specify multiple parts of a source file to be associated
 *  with a particular diagnostic message, or allow refrences to parts of binary
 *  files or databases or web addresses or anything else to be associated with
 *  a particular message.  But the interface can't be entirely arbitrary for
 *  the handler on the other side to be able to make the right decisions about
 *  how to present the information to the user.  The diagnostic message itself
 *  can always provide that additional information in its text.  Or multiple
 *  diagnostic messages can be sent to refer to different parts of a source
 *  file or multiple source files.  This module is optimized for what I see as
 *  the common cases, and that includes specifying one particular contiguous
 *  sequence of characters in a source file but not naything more complex than
 *  that.
 *
 *  The next four triplets repeat the prior four but with a message number
 *  added.  A message number is another commonly used feature in diagnostic
 *  messages.  Compilers often allow warning messages to be enabled or disabled
 *  by number on the command line, among other things, for exmaple.  The
 *  default message handler doesn't have the functionality to turn messages on
 *  or off, but a client-installed handler can do this.
 *
 *  The four ``set_diagnostic_source_*'' and the four
 *  ``unset_diagnostic_source_*'' functions provide a way to set a source
 *  position once at the start of some processing instead of having to pass
 *  this information at every generator call site.  This can simplify the
 *  client code, especially in the case where it does a lot of processing on
 *  each line.
 *
 *  The open_error(), open_warning(), open_notice(),
 *  assign_diagnostic_number(), diagnostic_text(), vdiagnostic_text(), and
 *  close_diagnostic() functions provide an interface for more complex message
 *  generation.  This enables message generation that goes through loops, for
 *  example, or that is indirect, with a wrapper to add information to
 *  messages.
 *
 *  On the handler side, it makes life easier on the handler to only have to
 *  provide one handler function that takes all the optional information that
 *  may or not be provided rather than requiring one handler for each of the
 *  many functions provided to generate messages.  The handler function is
 *  generally going to do a lot of the same things regardless of whether or not
 *  it has certain pieces of information, so it usually requires less code
 *  duplication to have a single handler function for all messages.
 *
 *  The choice of C as the implementation language was dictated by the fact
 *  that I have a number of modules that I've chosen to do in C that could use
 *  this functionality.  The reason for using C in those projects is that it is
 *  so widely used and available, not particularly that C itself is
 *  intrinsically particularly suited for the task.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, in November of 2007 and placed
 *  in the public domain at that time.  I've written similar code more than
 *  once before, including for the SUIF compiler and for code I wrote for
 *  Silicon Spice and/or Broadcom.  I've also used other people's versions of
 *  similar code at various times, including with the SUIF system and at
 *  Silicon Spice.  But now I want a version that is my own, free of anyone
 *  else's copyright interest, and I want to be able to make it freely
 *  available to everyone.  Also, my ideas about the right interface for such a
 *  module have evolved over time, and in creating this module I did some
 *  things differently, I believe, than the way they were done in previous
 *  diagnostic handling modules I've written or used.
 *
 *  I intend it first for the memory allocation module I'm writing for my C
 *  code right now, but I intend to soon use it in multiple modules.  I also
 *  intend to make it available by itself, in case anyone else finds it useful
 *  to incorporate into their own code -- which is most likely to happen as a
 *  consequence of the use of other code of mine that depends on this module.
 *
 *
 *      Legal Issues
 *
 *  I've written this code from scratch, without using or refering to any other
 *  code, not even previous incarnations of similar functionality that I wrote
 *  myself (I don't even have any of the old versions easily acessible to me at
 *  the moment).  I've written it on my own equipment and not for hire for
 *  anyone else, so I have full legal rights to place it in the public domain.
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
 *          Chris Wilson, 2003, 2007
 */

extern void basic_error(const char *format, ...);
extern void basic_warning(const char *format, ...);
extern void basic_notice(const char *format, ...);

extern void vbasic_error(const char *format, va_list arg);
extern void vbasic_warning(const char *format, va_list arg);
extern void vbasic_notice(const char *format, va_list arg);

extern void file_line_error(const char *file_name, size_t line_number,
                            const char *format, ...);
extern void file_line_warning(const char *file_name, size_t line_number,
                              const char *format, ...);
extern void file_line_notice(const char *file_name, size_t line_number,
                             const char *format, ...);

extern void file_character_error(const char *file_name, size_t line_number,
        size_t column_number, const char *format, ...);
extern void file_character_warning(const char *file_name, size_t line_number,
        size_t column_number, const char *format, ...);
extern void file_character_notice(const char *file_name, size_t line_number,
        size_t column_number, const char *format, ...);

extern void file_selection_error(const char *file_name, size_t line_number,
        size_t column_number, size_t character_count, const char *format, ...);
extern void file_selection_warning(const char *file_name, size_t line_number,
        size_t column_number, size_t character_count, const char *format, ...);
extern void file_selection_notice(const char *file_name, size_t line_number,
        size_t column_number, size_t character_count, const char *format, ...);

extern void numbered_error(size_t message_number, const char *format, ...);
extern void numbered_warning(size_t message_number, const char *format, ...);
extern void numbered_notice(size_t message_number, const char *format, ...);

extern void numbered_file_line_error(size_t message_number,
        const char *file_name, size_t line_number, const char *format, ...);
extern void numbered_file_line_warning(size_t message_number,
        const char *file_name, size_t line_number, const char *format, ...);
extern void numbered_file_line_notice(size_t message_number,
        const char *file_name, size_t line_number, const char *format, ...);

extern void numbered_file_character_error(size_t message_number,
        const char *file_name, size_t line_number, size_t column_number,
        const char *format, ...);
extern void numbered_file_character_warning(size_t message_number,
        const char *file_name, size_t line_number, size_t column_number,
        const char *format, ...);
extern void numbered_file_character_notice(size_t message_number,
        const char *file_name, size_t line_number, size_t column_number,
        const char *format, ...);

extern void numbered_file_selection_error(size_t message_number,
        const char *file_name, size_t line_number, size_t column_number,
        size_t character_count, const char *format, ...);
extern void numbered_file_selection_warning(size_t message_number,
        const char *file_name, size_t line_number, size_t column_number,
        size_t character_count, const char *format, ...);
extern void numbered_file_selection_notice(size_t message_number,
        const char *file_name, size_t line_number, size_t column_number,
        size_t character_count, const char *format, ...);

extern void set_diagnostic_source_file_name(const char *file_name);
extern void unset_diagnostic_source_file_name(void);
extern void set_diagnostic_source_line_number(size_t line_number);
extern void unset_diagnostic_source_line_number(void);
extern void set_diagnostic_source_column_number(size_t column_number);
extern void unset_diagnostic_source_column_number(void);
extern void set_diagnostic_source_character_count(size_t character_count);
extern void unset_diagnostic_source_character_count(void);

extern void open_error(void);
extern void open_warning(void);
extern void open_notice(void);
extern void assign_diagnostic_number(size_t message_number);
extern void diagnostic_text(const char *format, ...);
extern void vdiagnostic_text(const char *format, va_list arg);
extern void close_diagnostic(void);


typedef enum { DK_ERROR, DK_WARNING, DK_NOTICE } diagnostic_kind;

extern void (*get_diagnostic_handler(void **data))(void *data,
        diagnostic_kind kind, boolean has_message_number,
        size_t message_number, const char *file_name, boolean has_line_number,
        size_t line_number, boolean has_column_number, size_t column_number,
        boolean has_character_count, size_t character_count,
        const char *format, va_list arg);

extern void set_diagnostic_handler(void *data,
        void (*function)(void *data, diagnostic_kind kind,
                boolean has_message_number, size_t message_number,
                const char *file_name, boolean has_line_number,
                size_t line_number, boolean has_column_number,
                size_t column_number, boolean has_character_count,
                size_t character_count, const char *format, va_list arg));

extern void cleanup_diagnostic(void);


#endif /* DIAGNOSTICS_H */
