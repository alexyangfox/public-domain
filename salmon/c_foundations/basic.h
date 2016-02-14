/* file "basic.h" */

/*
 *  This file contains some very basic, general purpose C definitions and
 *  declarations.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef BASIC_H
#define BASIC_H


/*
 *      Usage
 *
 *  The definitions of boolean, TRUE, and FALSE here are fairly self-
 *  explanatory -- there are a type that can store a boolean value, and the
 *  values ``true'' and ``false'', respectively.
 *
 *  The ``verdict'' type is intended for use of a function to return to its
 *  caller whether or not it succeeded in doing what it was asked to do.  The
 *  MISSION_ACCOMPLISHED value means it did succeed, and the MISSION_FAILED
 *  value means that it did not.
 *
 *
 *      Requirements
 *
 *  This requires only an ANSI C compiler.
 *
 *
 *      Design Goals
 *
 *  This file is meant to provide some very basic things that are missing from
 *  standard C -- a boolean type and constants for TRUE and FALSE; and an
 *  enumerated type for functions to specify whether or not they succeeded at
 *  what they were asked to do.  This file is intended for any such things that
 *  I use all the time in C code.  What qualifies as ``basic'' and ``needed all
 *  the time'' are very subjective; this file contains things that meet my
 *  definitions of those terms.  Your milage may vary.  I may add things to
 *  this file from time to time, but I don't intend to remove things from
 *  future versions.
 *
 *  I chose to make ``boolean'' an ``int'' because that causes the fewest
 *  headaches.  The advantages are that it's completely compatible with C's
 *  idea of boolean expressions, which it gives type ``int''.  The disadvantage
 *  is that there's no type checking to make sure it's actually used as a
 *  boolean and not for random integer values.  In the past, in earlier
 *  versions of C++ that didn't have built-in boolean types, I experimented
 *  with making ``boolean'' an enumerated type and having appropriate cast
 *  operators, but ultimately there were always weird corner cases (mostly
 *  involving overloading resolution) that didn't work right when I did this.
 *  With C instead of C++, there's no overloading problem but also there's no
 *  way to create automatic casts.  In the end, I prefer to use ``int'' because
 *  it gives a little less safety but at least doesn't make anything that
 *  should work not work (which is essentially the C philosophy anyway).
 *
 *  For functions to signal their success or failure, I chose an enumerated
 *  type over an integer or boolean so that the type name makes it clear what
 *  the value means.  The most common standard for C is for functions to return
 *  integer values where 0 means success and non-zero means failure.  In some
 *  cases, negative values mean failure and 0 or higher encode various success
 *  values.  And in other cases, the returned integer indicates how many
 *  characters were printed or something like that.  So just looking at a
 *  prototype, one can't necessarily tell whether the returned integer is a
 *  zero for success or some other integer.  Boolean return values are a little
 *  less problematic, but still have the issue that the meaning isn't clear --
 *  the boolean value might mean something entirely different from whether
 *  there was an error while executing the function.  And even if the boolean
 *  return value is known to signal success or failure, is ``TRUE'' a success
 *  or an error?  So I created the ``verdict'' type to make it less ambiguous.
 *  Having a different type than boolean or int also makes it possible to do
 *  some automated error checking to catch certain bugs where a value is used
 *  in the wrong way.
 *
 *  The names used for this type -- ``verdict'', ``MISSION_ACCOMPLISHED'', and
 *  ``MISSION_FAILED'' -- have some problems but I couldn't come up with
 *  anything I liked better, and they have some advantages over the other
 *  possibilities I could come up with.  The name ``MISSION_ACCOMPLISHED'' has
 *  the advantage that it makes pretty clear that it's talking about whether or
 *  not something was successfully done.  It also has the advantage that it's
 *  not as likely to conflict with some other name in use by client code, such
 *  as ``SUCCESS'' might.  It does have the disadvantage of being a little
 *  long, though.  Once I had chosen ``MISSION_ACCOMPLISHED'',
 *  ``MISSION_FAILED'' was the best counterpart to that I could come up with.
 *  It's not quite a perfect fit, but I think it's not bad.
 *
 *  For the name of the type itself, I chose ``verdict'' for several reasons.
 *  A lot of it has to do with making function prototypes easy to read.  For
 *  function prototypes to be easy to read, keeping the name short is very
 *  important, and seven characters is pretty short.  It's not an abbreviation,
 *  which has the advantage that it can't be misinterpretted or accidentally
 *  abbreviated a different way.  The word makes it clear that it's a binary
 *  judgement about something and strongly suggests that it's a binary, success
 *  or failure type of judgement.  Also, the term ``verdict'' is not in common
 *  use in C or computer programming in general for other purposes.  On the
 *  other hand, ``verdict'' first calls to mind ``guilty or not guilty'', not
 *  ``succeeded or failed''.  But the use of ``verdict'' for things other than
 *  ``guilty or not guilty'' is pretty common in English, too.  It's not
 *  uncommon, for example, to ask someone for their verdict on, say, a movie,
 *  meaning their final judgement of it.  In that sense, it fits well here --
 *  the verdict on a call to a function is the final judgement on whether it
 *  worked or not.  Of course, ``verdict'' and ``MISSION_ACCOMPLISHED'' don't
 *  fit great together, but I can't think of a set of names that works better
 *  overall.
 *
 *  Here are some other possibilities I considered when trying to come up with
 *  names for this type and its two enumeration values:
 *
 *    * RESULT_SUCCESS
 *    * RESULT_FAILURE
 *          I thought these two were pretty good choices, actually.  But
 *          ``MISSION_ACCOMPLISHED'' worked even better, I thought, at
 *          conveying the idea that it was a particular action that was being
 *          judged on being accomplished successfully or failing.  The term
 *          ``RESULT'' could apply to something being found by the function
 *          rather than the action of executing the function itself.  Still, it
 *          was close between these names and the MISSION_* names I finally
 *          chose.
 *    * function_result_status
 *          This would have gone reasonably with the RESULT_* names for the
 *          enumeration values.  But it's pretty long, and ``result'' and
 *          ``status'' are somewhat vague and could be interpretted in
 *          different ways.
 *    * success_or_failure
 *          This is also pretty long and doesn't have a very good ring to it.
 *    * is_ok
 *          This is short, but doesn't flow terribly well as a type name.  It
 *          works better as a variable name.
 *    * execution_status
 *          This one is also on the long side, and again it uses the term
 *          ``status'' which is a little bit vague.  There are lots of statuses
 *          of various things in various programs, so using the term for one
 *          particular status could lead to confusion about which status among
 *          several is being referred to in a particular place.
 *    * health
 *    * HEALTH_GOOD
 *    * HEALTH_BAD
 *          I thought these three made a pretty good combination.  But
 *          ``health'' is often used in my experience with the ongoing status
 *          of a piece of hardware or other system, for example the health of a
 *          disk drive.  Using it to refer to a one-time event -- the success
 *          or failure of a particular operation -- goes against the most
 *          common usage of this term in my experience with computers.
 *    * mission
 *    * mission_result
 *    * debriefing
 *    * report
 *    * action_report
 *          Once I had hit on ``MISSION_ACCOMPLISHED'', I came up with these as
 *          tie-ins to that.  But standing alone in a function prototype they
 *          just wouldn't work very well.
 *    * status
 *          Like the other names I considered that used the term ``status'', I
 *          decided that this one was too prone to confusion with other
 *          statuses.  It is also such a commonly used word that by itself it
 *          might conflict with a lot of existing C code.
 *    * howd_it_go
 *          This is a form of the question ``How'd it go?'' without the
 *          punctuation.  But ultimately I thought it sounded a bit too much
 *          like slang, which could be distracting.
 *    * flag
 *    * mailbox
 *    * drop
 *    * lowdown
 *    * finding
 *    * discovery
 *    * revelation
 *    * check
 *    * determination
 *          These are all related to the idea returning a status after
 *          performing an operation, but ultimately I thought none was as good
 *          as ``verdict''.  Some are vague or the same as terms commonly used
 *          for other purposes, and others have too obscure a connection to be
 *          immediately obvious what they mean.
 *
 *  This is written in C as opposed to some other language simply because when
 *  I'm writing in C, I want to have this functionality.  Most other languages
 *  will not need exactly these things.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, in 2003 and 2007 and placed in
 *  the public domain at that time.  I've been using C definitions of boolean
 *  types and the TRUE and FALSE constants for a long time, in different
 *  contexts.  Essentially wherever I progam in C, I either use a definition
 *  like this that already exists in the code base I'm using or I create a new
 *  one like this very quickly.
 *
 *  I don't really remember where the specific names ``boolean'' (lower case
 *  and not ``bool'' like some use) and ``TRUE'' and ``FALSE'' (upper case)
 *  come from.  They may be something I chose on my own or they might be things
 *  that I started using from someone else's code, such as the SUIF compiler
 *  code.  I've seen several forms of boolean types for C and this is the one I
 *  like best.  Using upper case for TRUE and FALSE follows the general
 *  convention I like to stick to of making macros all upper case (though on
 *  occasion I've been known to use macros that don't follow this rule if I
 *  think there's a good reason for an exception).
 *
 *  When writing code that is shared by C and C++, TRUE and FALSE can be
 *  defined and ``true'' and ``false'' for C++ and ``boolean'' can be
 *  typedef'ed to ``bool''.  But be careful because in C++ the ``bool'' and
 *  ``int'' types might not be the same size, so code that is shared between C
 *  and C++ can't portably use this trick.
 *
 *  For the verdict type, I've thought at various times over the years of
 *  having such a type, but in the past I usually decided against creating a
 *  special type like this.  Instead, I would pass a success, failure, or error
 *  flag through a parameter that was a boolean pointer, which had the
 *  advantage that the name of the parameter made it clear exactly what it was
 *  for.  Or I would sometimes have a function return a boolean value and
 *  explicitly append ``_return_if_successful'' or something like that to the
 *  name of the function to make it clear what the meaning of the return value
 *  was.  I also used systems in the past that other people had created that
 *  had #define's for ``success'' and ``failure'' codes to return as integer
 *  return values, but I found those made it too easy to misinterpret the
 *  return value.  Partly, I hesitated to create my own enumerated type because
 *  I couldn't come up with names I liked for the type or the enumeration
 *  constants.  But, eventually, I decided I prefered to have an enumeration
 *  type after all and created the verdict type in this file.
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

typedef int boolean;

#define TRUE 1
#define FALSE 0

typedef enum { MISSION_ACCOMPLISHED, MISSION_FAILED } verdict;


#endif /* BASIC_H */
