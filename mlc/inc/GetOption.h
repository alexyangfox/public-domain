// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _GetOption_h
#define _GetOption_h 1

#include <basics.h>
#include <MEnum.h>
#include <MOption.h>

MString get_option_string(const MString& optionName,
			  const MString& defaultValue = emptyString,
			  const MString& optionHelp = emptyString,
			  Bool nuisance = FALSE);

int get_option_int(const MString& optionName,
		   int defaultValue,
		   const MString& optionHelp = emptyString,
		   Bool nuisance = FALSE);

int get_option_int(const MString& optionName,
		   const MString& optionHelp = emptyString,
		   Bool nuisance = FALSE);

int get_option_int_range(const MString& optionName,
			 int defaultValue,
			 int lowerBound,
			 int upperBound,
			 const MString& optionHelp = emptyString,
			 Bool nuisance = FALSE);

int get_option_int_range(const MString& optionName,
			 int lowerBound,
			 int upperBound,
			 const MString& optionHelp = emptyString,
			 Bool nuisance = FALSE);

Real get_option_real(const MString& optionName,
		 Real defaultValue,
		 const MString& optionHelp = emptyString,
		 Bool nuisance = FALSE);

Real get_option_real(const MString& optionName,
		    const MString& optionHelp = emptyString,
		    Bool nuisance = FALSE);

Real get_option_real_range(const MString& optionName,
			   Real defaultValue,
			   Real lowerBound,
			   Real upperBound,
			   const MString& optionHelp = emptyString,
			   Bool nuisance = FALSE);

Real get_option_real_range(const MString& optionName,
			   Real lowerBound,
			   Real upperBound,
			   const MString& optionHelp = emptyString,
			   Bool nuisance = FALSE);
   
int _get_option_enum(const MString& optionName,
		     const MEnum& optionMEnum,
		     int defaultValue,
		     const MString& optionHelp = emptyString,
		     Bool nuisance=FALSE);
int _get_option_enum(const MString& optionName,
		     const MEnum& optionMEnum,
		     const MString& optionHelp = emptyString,
		     Bool nuisance=FALSE);

// templated functions must have EXACT MATCHES in their parameters.
// Therefore, we do not get automatic casting between MString and const
// char *.  Since both MStrings and const char *'s are commonly used
// as parameters to this function, we must define multiple versions
// of this function.

// we define this as a macro to avoid duplicating the code.
// This macro creates four versions of get_option_enum so that we may
// pass MString or const char * for each of the strings used in this
// function.  GetOptionEnum behaves exactly like the basic options
// functions above, except that it takes a MEnum parameter which it
// uses to specify the allowable values for the enumerated option.
#define GET_OPTION_ENUM(T1, T2)			                           \
template <class EnumT>				                           \
inline EnumT get_option_enum(T1 optionName,                                \
			    const MEnum& optionMEnum,                      \
			    EnumT defValue,	                           \
			    T2 optionHelp,                                 \
			    Bool nuisance)		                   \
{						                           \
   return (EnumT) _get_option_enum(optionName, optionMEnum, (int)defValue, \
			      optionHelp, nuisance);	                   \
}                                                                          \
                                                                           \
template <class EnumT>				                           \
inline void get_option_enum(T1 optionName,	                           \
			    const MEnum& optionMEnum,                      \
			    T2 optionHelp,	                           \
			    Bool nuisance,                                 \
			    EnumT& returnVal)	                           \
{						                           \
   returnVal =  (EnumT)(_get_option_enum(optionName, optionMEnum,          \
				   optionHelp, nuisance));                 \
}


GET_OPTION_ENUM(const char *, const char *)
GET_OPTION_ENUM(const MString&, const char *)
GET_OPTION_ENUM(const char *, const MString&)
GET_OPTION_ENUM(const MString&, const MString&)

// get_option_bool is just a specific version of get_option_enum
inline Bool get_option_bool(const MString& optionName, Bool defaultValue,
			    const MString& optionHelp = emptyString,
			    Bool nuisance=FALSE)
{
   return (Bool) _get_option_enum(optionName, MOption::boolEnum,
				  (int)defaultValue, optionHelp,
				  nuisance);
}

// get_option_bool with no default
inline Bool get_option_bool(const MString& optionName,
			    const MString& optionHelp = emptyString,
			    Bool nuisance=FALSE)
{
   return (Bool) _get_option_enum(optionName, MOption::boolEnum,
				  optionHelp, nuisance);
}


#endif




