// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The MOption class hierarchy implements most of the common
                   option functionality.  This is implemented as a class
		   hierarchy because most types of options share basic
		   functionality (such as how to prompt the user), while
		   differing in a few important areas (such as what values
		   to prompt the user with and what types to return).
		 The interface to these classes is handled the global
		   functions in the GetOption module.  These funcions
		   provide an easy-to-use interface to the MOption
		   hierarchy.		   
  Assumptions  : MOption::initialize is called before any other functions
                   here are used.
  Comments     : 
  Complexity   : 
  Enhancements : 
  History      : Dan Sommerfield                                    10/23/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <MOption.h>
#include <GetOption.h>
#include <LogOptions.h>

RCSID("MLC++, $RCSfile: MOption.c,v $ $Revision: 1.13 $");



/***************************************************************************
  Description : The only invariant in the base class is that an empty option
                  name is illegal.
  Comments    : 
***************************************************************************/
void MOption::OK() const
{
   if(name == "")
      err << "MOption::OK: empty option name is not allowed"
	  << fatal_error;
}


/***************************************************************************
  Description : Prompts the user for the option.  The full parameter
                  indicates whether the long version of the prompt should
		  appear.  Long prompts are used after help strings.
  Comments    : Protected method.
                The short version for the base class does nothing because
                  we have no information about the option at the base
		  class level.
***************************************************************************/
void MOption::prompt_user(const MString& value, Bool full) const
{
   if(full) {
      Mcout << "Please enter a value for " << name;
      if(value != "")
	 Mcout << " (Default is " << value << ")";
   }
   Mcout << ": ";
}


/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
MOption::MOption(const MString& aName, const MString& def,
		 const MString& hlp, Bool nuis) :
    name(aName),
    defValue(def),
    help(hlp),
    nuisance(nuis)
{
   OK();
}


/***************************************************************************
  Description : Initialization.  This function gets the prompt level option
                  because we will need it to determine whether or not to
		  prompt for other options.
  Comments    : We force the prompt level to requiredOnly before we call
                  for the promptLevel option to avoid a recursion problem.
		  The effect of this is that prompt level will never get
		  prompted.
***************************************************************************/
void MOption::initialize()
{
    promptLevelEnum = 
      MEnum("required-only", requiredOnly) <<
      MEnum("basic", promptBasic) <<
      MEnum("all", promptAll);

    boolEnum = 
      MEnum("true", TRUE) << MEnum("false", FALSE) <<
      MEnum("yes", TRUE) << MEnum("no", FALSE);

   // get the prompt level for options.  Currently, the help string will
   // never be seen because we default prompt level to requiredOnly before
   // calling this function, and promptLevel is not a required option.
   promptLevel = get_option_enum(
          "PROMPTLEVEL", promptLevelEnum, promptLevel,
	  "This setting determines which other options should be set directly"
	  " by the user, and which should be obtained from default values.  "
	  "The required-only setting only prompts the user for options which "
	  "have no defaults.  The basic setting prompts the user for "
	  "all non-nuisance options.  The all setting prompts the"
	  " user for all options.", FALSE);

   // get a name for the sourceable file in which to store options
   // this is a "super-nuisance" options which is handled using get_env
   // so that it will never, ever prompt.
   optionDump = get_env_default("OPTION_DUMP", ".mlcoptions");
   
   // open the file
   optionDumpOut = new MLCOStream(optionDump);
}

/***************************************************************************
  Description : Finishing function.  This should get called on program
                  exit.
  Comments    : 
***************************************************************************/
void MOption::finish()
{
   // close the dump file
   delete optionDumpOut;
}


/***************************************************************************
  Description : This is the workhorse function of this class.  The get
                  function attempts to get a value for this option using
		  one of several possible methods:  the environment, the
		  programmer-specified default, and the user.  Which
		  source is used depends on the value of the option
		  PROMPTLEVEL.
  Comments    : 
***************************************************************************/
MString MOption::get(Bool emptyOK) const
{
   OK();

   // we need a non-const version of the nuisance flag to use within this
   // routine.
   Bool nuisFlag = nuisance;

   Bool usedProgDefault;
         
   // Try the environment first.  If no environment variable, try the
   // programmer default.  We can't use get_env_default because we need
   // to record whether or not we got the programmer default.
   char *charValue = getenv(name);
   MString value;
   if(emptyOK && charValue && *charValue == '\0') {
      // if its setenv'd to nothing and it is a string option, make
      // the default to be the empty string.
      usedProgDefault = FALSE;
      value = "";
   }
   else if(emptyOK && charValue == NULL) {
      // make sure to consider this a NO DEFAULT VALUE case
      value = defValue;
      usedProgDefault = TRUE;
      emptyOK = FALSE;
   }
   else if(charValue == NULL || *charValue == '\0') {
      usedProgDefault = TRUE;
      value = defValue;
   }
   else {
      usedProgDefault = FALSE;
      value = charValue;
   }

   // If the default value ends with a "!", then force the option to be
   // nuisance (i.e. generally don't prompt for the option)
   // If the option is just a lone !, then force-prompt
   if(value == "!") {
      // force programmer default always on single !
      value = defValue;
      nuisFlag = TRUE;
   }
   else if(value != "" && value[value.length()-1] == '!') {
      nuisFlag = TRUE;
      value = value.substring(0, value.length()-1);
   }
   
   // If the option is nuisance and has no value yet, it is an error.
   if(value == "" && nuisFlag && !emptyOK)
      err << "MOption::get: option " << name << " is a nuisance "
	  "option, but has no default or environment value set" <<
	  fatal_error;

   // Now, depending on our prompt level, we may need to prompt for
   // the option here.
   int shouldPrompt;
   switch(promptLevel) {

   case requiredOnly:
      // at this level, only prompt if no option value so far
      shouldPrompt = (value == "" && !emptyOK);
      break;

   case promptBasic:
      // prompt only if the option is a basic option
      shouldPrompt = !nuisFlag;
      break;

   case promptAll:
      // always prompt for these options
      shouldPrompt = TRUE;

   }

   // check if the default value we have is valid
   if(value == "?") {
      // silently force-prompt the option
      shouldPrompt = TRUE;
      value = "";
   }
   else if(value != "" && !result_OK(value)) {
      // force the option to be prompted and issue a warning about the
      // illegal default
      shouldPrompt = TRUE;
      Mcerr << "WARNING: Default value \"" << value << "\" for option " <<
	 name << " is ILLEGAL!  Default ignored." << endl;
      value = "";
   }
   
   // If we need to prompt for the option, do so here
   if(shouldPrompt) {   
      Mcout << "Option " << name;
      if(value != "")
	 Mcout << " [" << value << "]";
      prompt_user(value, FALSE);
      int goodValue = FALSE;

      // continue looping until we get a good value
      do {
	 MString newValue;
	 Mcout << flush << reset_pos_in_line;
	 newValue.get_line(Mcin);
	 if(Mcin.eof())
	    // endl needed here to avoid wrapping problems from EOF message
	    err << endl <<
	           "MOption::get: EOF encountered while attempting to input "
	           "option " << name << fatal_error;

	 // if the value ends with a '!', set the nuisance flag now and
	 // set the option to the part preceeding the '!':
	 if(newValue.length() && newValue[newValue.length()-1] == '!') {
	    newValue = newValue.substring(0, newValue.length()-1);
	    nuisFlag = TRUE;
	 }
	 
	 if(newValue == "" && (value != "" || emptyOK)) {
	    // accept the default--quit the loop
	    goodValue = TRUE;
	    ASSERT(result_OK(value));
	 }
	 else if(newValue == " " && emptyOK) {
	    // set the string to the empty string
	    value = "";
	    goodValue = TRUE;
	    usedProgDefault = FALSE;
	    ASSERT(result_OK(value));
	 }
	 else if(newValue == "?" || newValue == "") {
	    // display full help and full prompt.  Then loop again
	    if(help != "")
	       Mcout << help << endl;
	    prompt_user(value, TRUE);
	 }
	 else if(result_OK(newValue)) {
	    // result checked out OK, so quit the loop
	    goodValue = TRUE;
	    usedProgDefault = FALSE;
	    value = newValue;
	 }
	 else {
	    // user typed a bad value.  Show full prompt and repeat loop.
	    Mcout << "Invalid response: " << newValue <<
	       " (type '?' for help)" << endl;
	    prompt_user(value, TRUE);
	 }
      } while(!goodValue);
   }

   ASSERT(result_OK(value));

   // log the value before we return it, so that we can get a nice
   // display of the values of all our options
   GLOBLOG(1, "OPTION " << name << " = " << value << endl);

   // if the dump file has a name, write to it
   if(optionDump != "") {
      ASSERT(optionDumpOut);

      // if programmer default was used, then either unsetenv or setenv
      // to '!', depending on the nuisance flag
      if(usedProgDefault) {
	 if(nuisFlag)
	    (*optionDumpOut) << "setenv " << name << " '\\!'" << endl;
	 else
	    (*optionDumpOut) << "unsetenv " << name << endl;
      }

      // otherwise setenv the option.  Include a '!' at the end if the
      // nuisance flag was set.
      else {
	 (*optionDumpOut) << "setenv " << name << " '" << value;
	 if(nuisFlag)
	    (*optionDumpOut) << "\\!'" << endl;
	 else
	    (*optionDumpOut) << "'" << endl;
      }
   }
   return value;
}

/***************************************************************************
  Description : Prompt the user for a string.
  Comments    : Protected method.
***************************************************************************/
void StringOption::prompt_user(const MString& value, Bool full) const
{
   if(full) {
      Mcout << "Please enter a STRING value for " << name;
      if(value != "")
	 Mcout << " (Default is " << value << ")";
      Mcout << ": ";
   }
   else
      Mcout << "(string): ";
}

/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
StringOption::StringOption(const MString& name, const MString& def,
			   const MString& hlp, Bool nuisance)
   : MOption(name, def, hlp, nuisance)
{
   OK();
}


/***************************************************************************
  Description : Prompt the user for an integer.
  Comments    : Protected method.
***************************************************************************/
void IntOption::prompt_user(const MString& value, Bool full) const
{
   if(full) {
      Mcout << "Please enter an INTEGER value for " << name;
      if(value != "")
	 Mcout << " (Default is " << value << ")";
      Mcout << ": ";
   }
   else
      Mcout << "(integer): ";
}

/***************************************************************************
  Description : See if input is actually an integer.
  Comments    : Protected method.
***************************************************************************/
Bool IntOption::result_OK(const MString& string) const
{
   long intValue;
   return string.convert_to_long(intValue);
}


/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
IntOption::IntOption(const MString& name, const MString& def,
	     const MString& hlp, Bool nuisance)
   : MOption(name, def, hlp, nuisance)
{
   OK();
}

/***************************************************************************
  Description : Validate an integer range option.  This means checking that
                  its upper bound is >= its lower bound.
  Comments    : 
***************************************************************************/
void IntRangeOption::OK() const
{
   MOption::OK();
   if(lowerBound > upperBound)
      err << "IntRangeOption::OK: lower bound of " << lowerBound <<
	 " is bigger than upper bound of " << upperBound << fatal_error;
}

/***************************************************************************
  Description : Result validation for integer ranges.  Check if the value
                  of the result is in range.
  Comments    : Protected method.
***************************************************************************/
Bool IntRangeOption::result_OK(const MString& result) const
{
   if(!IntOption::result_OK(result))
      return FALSE;
   //@@ change to int_value once that's added to MString
   int intVal = int(result.long_value());
   return (intVal >= lowerBound && intVal <= upperBound);
}

/***************************************************************************
  Description : Prompt the user for the integer range.  Show the range to
                  the user in the prompt.
  Comments    : Protected method.
***************************************************************************/
void IntRangeOption::prompt_user(const MString& value, Bool full) const
{
   if(full) {
      Mcout << "Please enter an INTEGER value between " <<
	 lowerBound << " and " << upperBound << " for " << name;
      if(value != "")
	 Mcout << " (Default is " << value << ")";
      Mcout << ": ";
   }
   else
      Mcout << "(integer: [" << lowerBound << "," << upperBound << "]): ";
}   

/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
IntRangeOption::IntRangeOption(const MString& name, const MString& def,
			       int lower, int upper,
		               const MString& hlp, Bool nuisance)
   : IntOption(name, def, hlp, nuisance),
     lowerBound(lower),
     upperBound(upper)
{
   OK();
}

/***************************************************************************
  Description : Prompt the user for a Real.
  Comments    : Protected method.
***************************************************************************/
void RealOption::prompt_user(const MString& value, Bool full) const
{
   if(full) {
      Mcout << "Please enter a REAL value for " << name;
      if(value != "")
	 Mcout << " (Default is " << value << ")";
      Mcout << ": ";
   }
   else
      Mcout << "(Real): ";
}
/***************************************************************************
  Description : See if input is actually a valid real.
  Comments    : Protected method.
***************************************************************************/
Bool RealOption::result_OK(const MString& string) const
{
   Real realValue;
   return string.convert_to_real(realValue);
}



/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
RealOption::RealOption(const MString& name, const MString& def,
		       const MString& hlp, Bool nuisance)
   : MOption(name, def, hlp, nuisance)
{
   OK();
}

/***************************************************************************
  Description : Make sure lower bound is not bigger than upper bound
  Comments    :
***************************************************************************/
void RealRangeOption::OK() const
{
   MOption::OK();
   if(lowerBound > upperBound)
      err << "RealRangeOption::OK: lower bound of " << lowerBound <<
	 " is bigger than upper bound of " << upperBound << fatal_error;
}

/***************************************************************************
  Description : Result validation for real ranges.  Check if the value is
                  in range.
  Comments    : Protected method.
***************************************************************************/
Bool RealRangeOption::result_OK(const MString& result) const
{
   if(!RealOption::result_OK(result))
      return FALSE;
   Real realVal = result.real_value();
   return (realVal >= lowerBound && realVal <= upperBound);
}

/***************************************************************************
  Description : Prompt the user for a real range.
  Comments    : Protected method.
***************************************************************************/
void RealRangeOption::prompt_user(const MString& value, Bool full) const
{
   if(full) {
      Mcout << "Please enter a REAL value between " <<
	 lowerBound << " and " << upperBound << " for " << name;
      if(value != "")
	 Mcout << " (Default is " << value << ")";
      Mcout << ": ";
   }
   else
      Mcout << "(Real: [" << lowerBound << "," << upperBound << "]): ";
}   

/***************************************************************************
  Description : Constructor.
  Comments    : 
***************************************************************************/
RealRangeOption::RealRangeOption(const MString& name, const MString& def,
			       Real lower, Real upper,
		               const MString& hlp, Bool nuisance)
   : RealOption(name, def, hlp, nuisance),
     lowerBound(lower),
     upperBound(upper)
{
   OK();
}


/***************************************************************************
  Description : Result validation for enumerated options.  Use the MEnum
                  to determine if the string is part of the enum.
  Comments    : Protected method.
***************************************************************************/
Bool MEnumOption::result_OK(const MString& result) const
{
   // use value_from_name to figure out if this string is in the enum
   int val = enumSpec.value_from_name(result);
   return (val >= 0);
}

/***************************************************************************
  Description : Prompt the user for an enumerated value.  Display all
                  possible values in the prompt.
  Comments    : Protected method.
***************************************************************************/
void MEnumOption::prompt_user(const MString& value, Bool full) const
{
   if(full) {
      Mcout << "Please enter an enumerated value for " << name;
      if(value != "")
	 Mcout << " (Default is " << value << ")";
      Mcout << "." << endl;
      Mcout << "Allowable values are: " << enumSpec << endl;
      Mcout << ": " << endl;
   }
   else {
      Mcout << "(" << enumSpec << "): ";
   }
}

/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
MEnumOption::MEnumOption(const MString& name, const MEnum& mEnum,
			 const MString& def, const MString& hlp,
			 Bool nuisance)
   : MOption(name, def, hlp, nuisance),
     enumSpec(mEnum)
{
   OK();
}







