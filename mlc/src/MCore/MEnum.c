// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The MEnum class implements a dynamic enumeration type.
                   This type is implemented using a list of (name, value)
		   pairs which define the enumeration.
  Assumptions  : The same name is assumed not to appear twice in the
                   list.  We do not check for this condition until the
		   destructor because it requires time quadratic on the
		   length of the list to check.
  Comments     : The copy constructor exists because without it we cannot
                   initialize MEnum objects like:
		     MEnum myEnum = MEnum("a", 1) << MEnum("b",2) << ...
		   which was the whole point of having this class anyway.
		   The copy constructor performs a sanity (OK) check,
		   which results in a single sanity check per initialization.
		 The << operator is used for combination rather than a +
		   because operator+ must continually recopy the entire
		   MEnum each time we add something, resulting in quadratic
		   time for constructing an MEnum from its parts.
  Complexity   : Copy constructor takes time proportional to the length of
                   the list if debug mode is off, but takes time quadratic
		   on the length of the list in debug mode, because a
		   check for duplicate-valued names is performed here.
                 The << operator takes time proportional to the length of
		   the second list.
                 The two find functions take time proportional to the length
                   of the list.
		 The check_value function takes time proportional to the
		   length of the list.
  Enhancements : Sort by name to improve efficiency of OK().
  History      : Dan Sommerfield 				   10/26/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <DynamicArray.h>
#include <MEnum.h>

RCSID("MLC++, $RCSfile: MEnum.c,v $ $Revision: 1.4 $")


/***************************************************************************
  Description : Invariant checker.  Checks array bounds, and checks for
                  negative values, null names, and duplicate names.
  Comments    : This takes O(n^2), so we surrounded it with DBG statements,
                  (in the caller) and call it rarely.
***************************************************************************/
void MEnum::OK() const
{
   // check array bounds
   ASSERT(fields.low() == 0);

   // check items
   for(int i=0; i<fields.size(); i++) {
      ASSERT(fields[i].name != "");
      ASSERT(fields[i].value >= 0);
      for(int j=0; j<i; j++)
	 if(fields[i].name.equal_ignore_case(fields[j].name))
	    err << "MEnum::OK: name " << fields[i].name << " has conflicting "
	           "values " << fields[j].value << " and " << fields[i].value
	        << fatal_error;
   }
}
      


/***************************************************************************
  Description : Constructor which takes a name and a value.  Creates an
                  enumeration with one element.
  Comments    :
***************************************************************************/
MEnum::MEnum(const MString& name, int value)
   : fields(1)
{
   // make sure name is not empty
   if(name == "")
      err << "MEnum::MEnum: name for value " << value << " is empty"
	  << fatal_error;

   // make sure value is >= 0
   if(value < 0)
      err << "MEnum::MEnum: value for name " << name << " is < 0"
	  << fatal_error;
   
   // build with name and value.
   fields[0].name = name;
   fields[0].value = value;
}

/***************************************************************************
  Description : Copy constructor.
  Comments    : We need this or we won't be able to do nice initialization.
                  This is also a nice place to put a sanity check
		  (debug mode only).  This results in having the sanity
		  check performed once on initialization, which is
		  reasonable.
***************************************************************************/
MEnum::MEnum(const MEnum& other)
   : fields(other.fields, ctorDummy)
{
   DBG(OK());
}


/***************************************************************************
  Description : Appends one enumeration onto another.  Returns the resulting
                  combined enumeration so that operations may be chained.
  Comments    :
***************************************************************************/
MEnum& MEnum::operator<<(const MEnum& other)
{
   fields.append(other.fields);
   return *this;
}


/***************************************************************************
  Description : Displays the enumeration.  If full is TRUE, displays the
                  values along with the names.
  Comments    : 
***************************************************************************/
void MEnum::display(MLCOStream& out, Bool full) const
{
  int i;
  for(i=0; i<fields.size()-1; i++) {
      out << fields[i].name;
      if(full)
	 out << "=" << fields[i].value;
      out << ", ";
   }
   out << fields[i].name;
   if(full)
      out << "=" << fields[i].value;
}

DEF_DISPLAY(MEnum);


/***************************************************************************
  Description : Finds a value given its name.  Returns -1 on failure.
  Comments    : We return an error status rather than aborting because the
                  functions which call this are part of the user interface
		  code for the option help system.
***************************************************************************/
int MEnum::value_from_name(const MString& name) const
{
   for(int i=0; i<fields.size(); i++) {
      // get an uppercase version of target and source name
      MString targetUC = fields[i].name.to_upper();
      MString sourceUC = name.to_upper();

      // cut target to the size of source
      MString targetCutUC = targetUC.substring(
	 0,min(targetUC.length(), sourceUC.length()));

      // check for equality.  This determines if source is equal to
      // and prefix of target
      if(targetCutUC == sourceUC)
	 return fields[i].value;
   }
   return -1;
}


/***************************************************************************
  Description : Finds the name given a value.  In case we have two names for
                  the same value, returns the first one in the list.  If
		  the value is not found, returns "".
  Comments    : See value_from_name.
***************************************************************************/
MString MEnum::name_from_value(int value) const
{
   for(int i=0; i<fields.size(); i++)
      if(value == fields[i].value)
	 return fields[i].name;
   return "";
}






