// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  :  Looks up the given string in the environment.
                  get_env_default() returns default if the environment 
                    variable is not found via getenv.
                  get_env() aborts if not found or if empty.
  Assumptions  :  
  Comments     : 
  Complexity   :
  Enhancements : 
  History      : Dave Manley                                   9/27/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <strstream.h>

RCSID("MLC++, $RCSfile: get_env.c,v $ $Revision: 1.8 $")

MString get_env_default(const MString& envVarName, const MString& defaultName)
{
   char *value = getenv(envVarName);
   if (value == NULL || *value == '\0')
      return defaultName;
   return value;
}

MString get_env(const MString& envVarName)
{
   MString val(get_env_default(envVarName, ""));
   if (val == "")
      err << "get_env: Environment variable " << envVarName
          << " undefined or empty" << fatal_error;

   return val;
}
