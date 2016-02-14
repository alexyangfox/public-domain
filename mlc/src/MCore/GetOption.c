// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : GetOption.c holds a set of global convenience
                   functions which help other classes get option values
		   from the environment.
		 Each function has two versions: one which takes a default
		   argument and one which does not.  Omitting a default for
		   a nuisance option may lead to a fatal error if the
		   option is not defined in the environment.
		 Prompting for options is controlled by the global option
		   prompt-level.  If prompt-level is set to required-only,
		   then only options with no default or shell-defined
		   values will be prompted.  If prompt-level is set to
		   basic, then all options with the nuisance flag
		   not set will be confirmed.  If prompt-level is set
		   to all, then all options will always be
		   confirmed.
		 It is an error to ask for an nuisance option without
		   providing a default value.
  Assumptions  : MOption::initialize() is called before any of these
                   functions are called.
  Comments     : These functions are really just a nice interface for the
                   MOption class hierarchy.  Option getting works by
		   instantiating a subclass of MOption and passing it to the
		   get_option function.  The subclasses of MOption handle
		   all option-type-specific behavior.
		 The get_option_enum function is templated so that we can
		   get options for a variety of enumerated types and still
		   maintain compiler type-checking.  This function's code
		   is defined in the header file to avoid generating extra
		   files in the ptrepository.
  Complexity   :
  Enhancements : Save a record of all options encountered to speed up access
                   in case we try to get the same option more than once.
		 Allow a display of all options and their values at any time.
		   Requires enhancement immediately above.
		 Convert these functions to a class GetOption with static
		   member functions.  This is presently impossible because
		   we cannot create templated class member functions in the
		   current version of C++.
		 Check if input is coming from a tty versus a file.  If input
		   is from a file, echo a newline to the display on each input
		   to keep the output orderly.
  History      : Dan Sommerfield                                    10/23/94
                   Initial revision (.c,.h)
***************************************************************************/

#include <basics.h>
#include <MEnum.h>
#include <GetOption.h>

RCSID("MLC++, $RCSfile: GetOption.c,v $ $Revision: 1.7 $")

/***************************************************************************
  Description : Basic option reading functions.  The optionName serves as
                  the enviornment variable name for the option.  It is also
		  printed out for the user to identify the option.  The
		  defaultValue parameter specifies a default value of the
		  appropriate type for this option.  The ranged option
		  functions also allow the user to set upper and lower
		  bounds.  Help string identifies a long text string to be
		  printed out when the user asks for help on this option.
		  The nuisance flag, if turned on, indicates that this
		  option will only be prompted if the promptLevel is set to
		  confirmAdvanced.
		Several of the parameters in this list are optional.  First,
		  the defaultValue, which is a parameter in the middle of
		  the list, may be omitted (even if you want to specify a
		  help string and an nuisance flag).  Second, the help
		  string and nuisance flag are always optional, although it
		  is an error to ever specify an nuisance option without a
		  default value.
		
  Comments    : We provide two versions of each function (except for string
                  and enum) so that the default value may be omitted from
		  the middle of the argument list.  Also, there is often
		  no reasonable value to which to assign a default value
		  (for example, there is no integer we can use to represent
		  "no default integer value"), so we cannot simply use
		  int defaultValue = ... and let the compiler take care of
		  the default for us.
***************************************************************************/
MString get_option_string(const MString& optionName,
			  const MString& defaultValue,
			  const MString& optionHelp,
			  Bool nuisance)
{
   return StringOption(optionName, defaultValue, optionHelp, nuisance).
      get(TRUE);
}

int get_option_int(const MString& optionName,
		   int defaultValue,
		   const MString& optionHelp,
		   Bool nuisance)
{
   return IntOption(optionName, MString(defaultValue, 0),
		    optionHelp, nuisance).get().long_value();
}

int get_option_int(const MString& optionName,
		   const MString& optionHelp,
		   Bool nuisance)
{
   return IntOption(optionName, "", optionHelp, nuisance).get().
      long_value();
}

int get_option_int_range(const MString& optionName,
			 int defaultValue,
			 int lowerBound,
			 int upperBound,
			 const MString& optionHelp,
			 Bool nuisance)
{
   return IntRangeOption(optionName, MString(defaultValue, 0),
			 lowerBound, upperBound, optionHelp,
			 nuisance).get().long_value();
}

int get_option_int_range(const MString& optionName,
			 int lowerBound,
			 int upperBound,
			 const MString& optionHelp,
			 Bool nuisance)
{
   return IntRangeOption(optionName, "",
			 lowerBound, upperBound, optionHelp,
			 nuisance).get().long_value();
}

Real get_option_real(const MString& optionName,
		 Real defaultValue,
		 const MString& optionHelp,
		 Bool nuisance)
{
   return RealOption(optionName, MString(defaultValue, 0),
		     optionHelp, nuisance).get().real_value();
}

Real get_option_real(const MString& optionName,
		    const MString& optionHelp,
		    Bool nuisance)
{
   return RealOption(optionName, "",
		     optionHelp, nuisance).get().real_value();
}

Real get_option_real_range(const MString& optionName,
			   Real defaultValue,
			   Real lowerBound,
			   Real upperBound,
			   const MString& optionHelp,
			   Bool nuisance)
{
   return RealRangeOption(optionName, MString(defaultValue, 0),
			  lowerBound, upperBound, optionHelp,
			  nuisance).get().real_value();
}

Real get_option_real_range(const MString& optionName,
			   Real lowerBound,
			   Real upperBound,
			   const MString& optionHelp,
			   Bool nuisance)
{
   return RealRangeOption(optionName, "",
			  lowerBound, upperBound, optionHelp,
			  nuisance).get().real_value();
}


/***************************************************************************
  Description : _get_option_enum is a helper function used by the template
                  get_option_enum functions in the header file.  This
		  function uses a MEnum type to specify allowable options
		  for an enumerated type option.  It should not be called
		  directly because it takes and returns an int, rather than
		  the appropriate enum type.  The templated version does
		  the casting for you, and then calls this function.
  Comments    :
***************************************************************************/
int _get_option_enum(const MString& optionName,
		     const MEnum& optionMEnum,
		     int defaultValue,
		     const MString& optionHelp,
		     Bool nuisance)
{
   // grab the value out of get_option
   // Try to lookup the string in the enum value.
   MString val = MEnumOption(optionName, optionMEnum,
			     optionMEnum.name_from_value(defaultValue),
			     optionHelp, nuisance).get();
   int enumValue = optionMEnum.value_from_name(val);
   ASSERT(enumValue >= 0);
   return enumValue;
}

int _get_option_enum(const MString& optionName,
		     const MEnum& optionMEnum,
		     const MString& optionHelp,
		     Bool nuisance)
{
   // grab the value out of get_option
   // Try to lookup the string in the enum value.
   MString val = MEnumOption(optionName, optionMEnum, "",
			     optionHelp, nuisance).get();
   int enumValue = optionMEnum.value_from_name(val);
   ASSERT(enumValue >= 0);
   return enumValue;
}
