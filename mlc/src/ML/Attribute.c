// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The AttrInfo class allows giving attribute names,
                   types, and a position in the instance array.
		 The MetricAttrInfo class provides a method for determining
		   the distance between two AttrValue_s.
		 The PartialOrderAttrInfo class imposes a partial
		   order upon the AttrValue_s.
		 The LinearAttrInfo class imposes a linear ordering upon the
		   AttrValue_s.  It provides comparison operations.
		 The NominalAttrInfo class allows giving the different
		   values that the attribute may obtain.
		 The LinearNominalAttrInfo class requires that the given
		   attributes have a linear ordering.  The number of values
		   must be finite.
		 The RealAttrInfo class allows AttrValue_s of type
		   Real.  The standard definitions of the comparison
		   operators is implemented.
		 The IntegerAttrInfo class allows AttrValue_s to take
		   on any integer value.  The standard definitions of the
		   comparison operators is implemented.
                 This file also includes some of the basic definitions
                   like Category, NominalVal, Real.
		 AttrValue_ is a class which stores the vlaues of attributes.
		   The values can only be accessed through AttrInfo functions.
  Assumptions  : The value of UNKNOWN_REAL_VAL is never used for
                   a known real value.
  Comments     : Given k discrete categories, they will be mapped into
                   integers in the range [0..k-1].  -1 is used for
                   unknown/undefined.
  Complexity   : NominalAttrInfo::add_value() takes constant time.
                 NominalAttrInfo::int_to_nominal() and 
		    NominalAttrInfo::nominal_to_int take time
		    proportional to to the number of nominal 
		    atribute values.
		 equal(), operator==, and operator!= take time proportional
		    to the number of attribute values for NominalAttrInfo
		    times the length of the attribute names.
  Enhancements : Implement class LinearNominalAttrInfo.
		 Implement class IntegerAttrInfo.
		 Implement class RationalAttrInfo.
  History      : Richard Long                                       5/17/94   
                   Added AttrValue_, NominalAttrValue_, and RealAttrValue_
		   classes.
                 Brian Frasca					    1/21/94
                   Added AttrInfo::display().
                 Svetlozar Nestorov                                 1/15/94
                   Added clone(int aNum) in AttrInfo class.
                 Brian Frasca					    1/13/94
		   Added _equal_value(), is_unknown()
		 Svetlozar Nestorov                                 1/08/94
                   Added copy constructors and method clone().
                 Richard Long                                       7/14/93
                   Initial revision (.c)
                 Ronny Kohavi                                       7/13/93
                   Initial revision (.h)
***************************************************************************/
#include <basics.h>
#include <Attribute.h>
#include <mlcIO.h>
#include <math.h>
#include <MStringRC.h>

RCSID("MLC++, File $Revision: 1.79 $")

const int  UNKNOWN_CATEGORY_VAL = -1;
const int  UNKNOWN_NOMINAL_VAL  = UNKNOWN_CATEGORY_VAL;
const int  FIRST_CATEGORY_VAL   = UNKNOWN_NOMINAL_VAL+1;
const int  FIRST_NOMINAL_VAL    = FIRST_CATEGORY_VAL;
const int  MAX_NUM_CATEGORIES   = 1000; // Safety only, may be increased.
const Real UNKNOWN_REAL_VAL     = MINFLOAT;

SET_DLLPIX_CLEAR(MString,NULL);

static MString UNKNOWN_VAL_STR("?");

/***************************************************************************
  Description : Returns the MString corresponding to the AttrType
  Comments    : Might return const MString& if we put these in strings.
***************************************************************************/
MString attr_type_to_string(AttrType attrType)
{
  switch (attrType) {
  case unknown : return "unknown";
  case real: return "real";
  case nominal: return "nominal";
  case linearNominal: return "linearNominal";
  case treeStructured: return "treeStructured";
  case internalDisjunction: return "internalDisjunction";
  case userReal: return "userReal";
  case userNominal: return "userNominal";
  case userLinearNominal: return "userLinearNominal";
  case userTreeStructured: return "userTreeStructured";
  case userInternalDisjunction: return "userInternalDisjunction";
  default:
    err << "Attribute.c::attr_type_to_string: Invalid AttrType value "
	<< attrType << fatal_error;
    return "Attribute.c:attr_type bug";
  }
}


/***************************************************************************
  Description : Protect special characters by prefixing them with \.
                Currently only a period is protected
  Comments    : Should be made O(n) instead of O(n^2)
***************************************************************************/


MString protect_chars(const MString& str)
{
   // Common case should be fast.
   if (!str.contains("."))
      return str;

   MString newStr;
   for (int i = 0; i < str.length(); i++)
      if (str[i] == '.')
	 newStr += "\\.";
      else
	 newStr += MString(str[i]);
   return newStr;
}


/***************************************************************************
  Description : Constructors and copy constructor.
  Comments    : It is illegal to create a NominalAttrValue_ with a
                   RealAttrValue_, so that assignment operator is
		   private and will cause a compile time warning.
		   If the constructor were not defined the RealAtrValue_
		   would be automatically upcast to a AttrValue_ and
		   would go to that constructor and give a run-time warning.
***************************************************************************/
NominalAttrValue_::NominalAttrValue_(const RealAttrValue_&)
{
   err << "NominalAttrValue_::NominalAttrValue_(RealAttrValue_): Cannot "
          "construct NominalAttrValue_ with a RealAttrValue_"
       << fatal_error;
}


NominalAttrValue_::NominalAttrValue_() 
{
   value.intVal = UNKNOWN_NOMINAL_VAL + NOMINAL_OFFSET;
   ATTRDBG(type = nominal);
}

NominalAttrValue_::NominalAttrValue_(const AttrValue_& src)
{
   ATTRDBG(if (src.type != nominal)
          err << "NominalAttrValue_::NominalAttrValue_(AttrValue_): Cannot "
                 "construct a NominalAttrValue_ from a "
              << attr_type_to_string(src.type) << " AttrValue_"
              << fatal_error;
       type = nominal);
   value = src.value;
}


NominalAttrValue_::NominalAttrValue_(const NominalAttrValue_& src)
{
   ATTRDBG(ASSERT(src.type == nominal));
   ATTRDBG(type = nominal);
   value = src.value;
}


/***************************************************************************
  Description : Assignment operators for NominalAttrValue_.
                It is illegal to assign a RealAttrValue_ to a
		   NominalAttrValue_, so that assignment operator is
		   private and will cause a compile time warning.
  Comments    :
***************************************************************************/
NominalAttrValue_& NominalAttrValue_::operator=(const RealAttrValue_&)
{
   err << "NominalAttrValue_::operator=(const RealAttrValue_): Cannot "
          "assign a RealAttrValue_ to a NominalAttrValue_" << fatal_error;
   return *this;  // to avoid compiler warning
}


NominalAttrValue_& NominalAttrValue_::operator=(const AttrValue_& src)
{
   if (&src != this) {
      ATTRDBG(if (src.type != nominal)
	     err << "NominalAttrValue_::operator=: Cannot assign a "
	         << attr_type_to_string(src.type) << " AttrValue_ to "
	            "a NominalAttrValue_" << fatal_error);
      ATTRDBG(type = src.type);
      value = src.value;
   }
   return *this;
}


/***************************************************************************
  Description : Constructors and copy constructor.
  Comments    : It is illegal to create a RealAttrValue_ with a
                   NominalAttrValue_, so that constructor is private
		   and will cause a compile time warning.  If the
		   constructor were not defined the NominalAtrValue_ 
		   would be automatically upcast to a AttrValue_ and
		   would go to that constructor and give a run-time warning.
***************************************************************************/
RealAttrValue_::RealAttrValue_(const NominalAttrValue_&)
{
   err << "RealAttrValue_::RealAttrValue_(NominalAttrValue_): Cannot "
          "construct RealAttrValue_ with a NominalAttrValue_"
       << fatal_error;
}


RealAttrValue_::RealAttrValue_() 
{
   value.realVal = UNKNOWN_REAL_VAL;
   ATTRDBG(type = real);
}

RealAttrValue_::RealAttrValue_(const AttrValue_& src)
{
   ATTRDBG(if (src.type != real)
          err << "RealAttrValue_::RealAttrValue_(AttrValue_): Cannot "
                 "construct a RealAttrValue_ from a "
              << attr_type_to_string(src.type) << " AttrValue_"
              << fatal_error;
       type = real);
   value = src.value;
}


RealAttrValue_::RealAttrValue_(const RealAttrValue_& src)
{
   ATTRDBG(ASSERT(src.type == real));
   ATTRDBG(type = real);
   value = src.value;
}


/***************************************************************************
  Description : Assignment operators for RealAttrValue_.
                It is illegal to assign a NominalAttrValue_ to a
		   RealAttrValue_, so that assignment operator is
		   private and will cause a compile time warning.
  Comments    :
***************************************************************************/
RealAttrValue_& RealAttrValue_::operator=(const NominalAttrValue_&)
{
   err << "RealAttrValue_::operator=(const NominalAttrValue_): Cannot "
          "assign a NominalAttrValue_ to a RealAttrValue_" << fatal_error;
   return *this;  // to avoid compiler warning
}


RealAttrValue_& RealAttrValue_::operator=(const AttrValue_& src)
{
   if (&src != this) {
      ATTRDBG(if (src.type != real)
	     err << "RealAttrValue_::operator=: Cannot assign a "
	         << attr_type_to_string(src.type) << " AttrValue_ to "
	            "a RealAttrValue_" << fatal_error);
      ATTRDBG(type = real);
      value = src.value;
   }
   return *this;
}


/***************************************************************************
  Description : Compares attrName, attrType, and attrNum.
  Comments    : Protected method.
***************************************************************************/
Bool AttrInfo::equal_shallow(const AttrInfo& info, Bool fatalOnFalse) const
{
  if (attrName != info.name()) {
    if (fatalOnFalse)
      err << "AttrInfo::equal_shallow: names are different: "
	  << attrName << ", " << info.name() << fatal_error;
    return FALSE;
  }

  if (attrType != info.type()) {
    if (fatalOnFalse)
      err << "AttrInfo::equal_shallow: types are different: "
	  << attr_type_to_string(attrType) << ", " 
	  << attr_type_to_string(info.type()) << fatal_error;
    return FALSE;
  }

  return TRUE;
}


/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
AttrInfo::AttrInfo(const MString& aname, AttrType atype)
         : attrName(aname), attrType(atype) { }


/***************************************************************************
  Description : Copy constructor with an extra argument. 
  Comments    : 
***************************************************************************/
AttrInfo::AttrInfo(const AttrInfo& source, CtorDummy /*dummyArg*/):
  attrName(source.attrName), attrType(source.attrType)
{ }


/***************************************************************************
  Description : Causes a fatal_error.
  Comments    : Should be defined again in NominalAttrInfo/RealAttrInfo.
***************************************************************************/
NominalAttrInfo& AttrInfo::cast_to_nominal()
{
  err << "AttrInfo::cast_to_nominal: Type " << attr_type_to_string(type())
      << " is not derived from NominalAttrInfo" << fatal_error;
  return *(NominalAttrInfo*)NULL_REF; // to avoid no return value error
}


const NominalAttrInfo& AttrInfo::cast_to_nominal() const
{
  err << "AttrInfo::cast_to_nominal() const: Type " 
      << attr_type_to_string(type()) 
      << " is not derived from NominalAttrInfo" << fatal_error;
  return *(NominalAttrInfo*)NULL_REF; // to avoid no return value error
}


RealAttrInfo& AttrInfo::cast_to_real()
{
  err << "AttrInfo::cast_to_real: Type " << attr_type_to_string(type())
      << " is not derived from RealAttrInfo" << fatal_error;
  return *(RealAttrInfo*)NULL_REF; // to avoid no return value error
}


const RealAttrInfo& AttrInfo::cast_to_real() const
{
  err << "AttrInfo::cast_to_real() const: Type " 
      << attr_type_to_string(type()) 
      << " is not derived from RealAttrInfo" << fatal_error;
  return *(RealAttrInfo*)NULL_REF; // to avoid no return value error
}


Bool AttrInfo::can_cast_to_nominal() const
{
   return FALSE;
}


Bool AttrInfo::can_cast_to_real() const
{
   return FALSE;
}



/***************************************************************************
  Description : Displays the attribute name, type, and index number.
  Comments    :
***************************************************************************/
void AttrInfo::display(MLCOStream& stream) const
{
   stream << "\"" << name() << "\", " << attr_type_to_string(type());
}

// Define operator<< for display()
DEF_DISPLAY(AttrInfo)


/***************************************************************************
  Description : These functions cause fatal_errors.  They should be
                   overridden as appropriate in subclasses.
  Comments    :
***************************************************************************/
void AttrInfo::set_nominal_val(AttrValue_&, int) const
{
   err << "AttrInfo::set_nominal_val: Cannot be called for a "
       << attr_type_to_string(type()) << " AttrInfo" << fatal_error;
}


NominalVal AttrInfo::get_nominal_val(const AttrValue_) const
{
   err << "AttrInfo::get_nominal_val: Cannot be called for a "
       << attr_type_to_string(type()) << " AttrInfo" << fatal_error;
   return 0;  // To avoid compiler warning
}


void AttrInfo::set_real_val(AttrValue_&, Real) const
{
   err << "AttrInfo::set_real_val: Cannot be called for a "
       << attr_type_to_string(type()) << " AttrInfo" << fatal_error;
}


Real AttrInfo::get_real_val(const AttrValue_) const
{
   err << "AttrInfo::get_real_val: Cannot be called for a "
       << attr_type_to_string(type()) << " AttrInfo" << fatal_error;
   return 0;  // To avoid compiler warning
}


/*****************************************************************************
   Description : Returns the storage size actually used by each attribute
                    type. If an attribute has an undefined storage size, it
		    aborts.
   Comments    : Since this is a base class for all AttrInfo, it aborts. Each
                    AttrInfo class will overwrite this method appropriately.
*****************************************************************************/
int AttrInfo::storage_size() const
{
   err << "The storage size for this attribute is not defined(fixed)"
       << fatal_error;
   return 0; // this is to avoid complaints from compiler.
}




/***************************************************************************
  Description : Copy constructor.
  Comments    : 
***************************************************************************/
PartialOrderAttrInfo::PartialOrderAttrInfo(const PartialOrderAttrInfo& source,
					   CtorDummy dummyArg)
   : AttrInfo(source, dummyArg)
{} 



/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
PartialOrderAttrInfo::PartialOrderAttrInfo(const MString& aname, 
					   AttrType atype)
                    : AttrInfo(aname, atype) { }


/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
LinearAttrInfo::LinearAttrInfo(const MString& aname, AttrType atype)
              : PartialOrderAttrInfo(aname, atype),
                AttrInfo(aname, atype) { }


/***************************************************************************
  Description : Copy constructor.
  Comments    : 
***************************************************************************/
LinearAttrInfo::LinearAttrInfo(const LinearAttrInfo& source,
			       CtorDummy dummyArg):
  PartialOrderAttrInfo(source, dummyArg), AttrInfo(source, dummyArg)
{}

/***************************************************************************
  Description : Comparators.  All are defined in terms of less_than().
                  Thus, to define an order among attribute values, the
		  user need only define the less_than() method.
  Comments    : All of the methods are virtual, so they can be overridden
                  in a subclass if there is a more efficient way of
		  implementing them.
***************************************************************************/
Bool LinearAttrInfo::less_than_equal(const AttrValue_ val1,
				     const AttrValue_ val2) const
{
   return !less_than(val2, val1);
}


Bool LinearAttrInfo::greater_than(const AttrValue_ val1,
				  const AttrValue_ val2) const
{
   return less_than(val2, val1);
}


Bool LinearAttrInfo::greater_than_equal(const AttrValue_ val1,
					const AttrValue_ val2) const
{
   return !less_than(val1, val2);
}


Bool LinearAttrInfo::equal(const AttrValue_ val1, const AttrValue_ val2) const
{
   return !less_than(val1, val2) && !less_than(val2, val1);
}


Bool LinearAttrInfo::not_equal(const AttrValue_ val1,
			       const AttrValue_ val2) const
{
   return less_than(val1, val2) || less_than(val2, val1);
}


/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
MetricAttrInfo::MetricAttrInfo(const MString& aname, AttrType atype)
              : AttrInfo(aname, atype)
{}


/***************************************************************************
  Description : Copy constructor.
  Comments    : 
***************************************************************************/
MetricAttrInfo::MetricAttrInfo(const MetricAttrInfo& source,
			       CtorDummy dummyArg):
AttrInfo(source, dummyArg)
{}


/***************************************************************************
  Description : Causes fatal_error if the given AttrValue_ is not nominal.
  Comments    :
***************************************************************************/
void NominalAttrInfo::check_valid_attr_value_type(const AttrValue_ val) const
{
   (void)val; // for fast mode.
   ATTRDBG(if (val.type != nominal)
          err << "NominalAttrInfo::check_valid_attr_value_type: "
                 "Non-nominal AttrValue_ type "
              << attr_type_to_string(val.type) << fatal_error);
}


/***************************************************************************
  Description : Constructor.  Copies string pointers to the values array.
  Comments    : Gets ownership of attrVals.
                AttrType is the last parameter so that it can have a
		  default value, which the subclasses (e.g.
		  LinearNominalAttrInfo) can override.
***************************************************************************/
NominalAttrInfo::NominalAttrInfo(const MString& aname,
		       DblLinkList<MString>*& attrVals, AttrType atype)
  : AttrInfo(aname, atype),
    values(FIRST_NOMINAL_VAL, attrVals->length())
{
   DBG(if (attrVals->length() == 0)
      err << "NominalAttrInfo::NominalAttrInfo: 0 length list of strings"
          << fatal_error);

   DLLPix<MString> pix(*attrVals,1);
   for (int i = values.low(); i <= values.high() && pix; i++, pix.next())
      values[i] = *pix;
   delete attrVals;
   attrVals = NULL;
}


/***************************************************************************
  Description : Copy constructor. 
  Comments    :
***************************************************************************/
NominalAttrInfo::NominalAttrInfo(const NominalAttrInfo& source,
				 CtorDummy dummyArg):
  AttrInfo(source, dummyArg), values(source.values, ctorDummy)
{}



/***************************************************************************
  Description : Destructor.
  Comments    :
***************************************************************************/
NominalAttrInfo::~NominalAttrInfo()
{}


/***************************************************************************
  Description : Returns number of values that the attribute has.
  Comments    : 
***************************************************************************/
int NominalAttrInfo::num_values() const
{
   return values.high() - values.low() + 1;
}


/***************************************************************************
  Description : Returns the ith Attribute value.
  Comments    : 
***************************************************************************/
const MString& NominalAttrInfo::get_value(int i) const 
{
  if (i == UNKNOWN_NOMINAL_VAL)
     return UNKNOWN_VAL_STR;
  
  if (i < values.low() || i > values.high())
    err << "NominalAttrInfo::get_value: given value " << i
	<< " not in the range " << values.low() << '-' << values.high()
	<< " for " << name() << fatal_error;
  return values[i];
}


/***************************************************************************
  Description : Causes fatal_error if the given value is not a valid 
                  nominal value for this NominalAttrInfo.  
		  Otherwise does nothing.
  Comments    : Assumes that value stored in val is an integer, since
                  this is a NominalAttrInfo.
***************************************************************************/
void NominalAttrInfo::check_in_range(const AttrValue_ val) const
{
   check_valid_attr_value_type(val);
   if (val.value.intVal < UNKNOWN_NOMINAL_VAL + NOMINAL_OFFSET ||
       val.value.intVal > UNKNOWN_NOMINAL_VAL + num_values() + NOMINAL_OFFSET)
      err << "NominalAttrInfo::check_in_range: "
	  << val.value.intVal - NOMINAL_OFFSET
	  << " must be in the range " << UNKNOWN_NOMINAL_VAL << " to "
	  << UNKNOWN_NOMINAL_VAL + num_values() << " for " << name()
	  << fatal_error;
}


/***************************************************************************
  Description : Converts the given AttrValue_ to the corresponding MString 
                  representation of the nominal attribute value.
  Comments    : "?" will be returned for UNKNOWN_NOMINAL_VAL.
                Assumes that value stored in val is an integer, since
                  this is a NominalAttrInfo.
***************************************************************************/
MString NominalAttrInfo::attrValue_to_string(const AttrValue_ val) const
{
  return get_value(get_nominal_val(val));
}


/***************************************************************************
  Description : Converts the given MString description of a nominal
                  attribute into the integer representation used
		  to store the information.
  Comments    : Returns UNKNOWN_NOMINAL_VAL for "?".
                If the nominal attribute value is not found, a fatal_error
                  will occur.
***************************************************************************/
int NominalAttrInfo::nominal_to_int(const MString& attrVal) const
{
  if (attrVal == UNKNOWN_VAL_STR)
    return UNKNOWN_NOMINAL_VAL;

  for (int i = values.low(); i <= values.high(); i++)
     if (get_value(i) == attrVal)
	return i;

  err << "NominalAttrInfo::nominal_to_int: " << attrVal << " is not "
         "a valid attribute value for " << name() << fatal_error;

  return -1; // dummy
}


/***************************************************************************
  Description : Returns TRUE iff info matches this exactly.
  Comments    : 
***************************************************************************/
Bool NominalAttrInfo::equal(const AttrInfo& info,
			    Bool fatalOnFalse) const
{
  if (!equal_shallow(info, fatalOnFalse))
    return FALSE;

  const NominalAttrInfo& nai = info.cast_to_nominal();
  if (num_values() != nai.num_values()) {
    if (fatalOnFalse)
      err << "NominalAttrInfo::equal: this had " << num_values()
	  << " values; info had " << nai.num_values() << fatal_error;
    return FALSE;
  }

  for (int i = values.low(); i <= values.high(); i++)
     if (get_value(i) != nai.get_value(i)) {
	if (fatalOnFalse) 
	   err << "NominalAttrInfo::equal: this had value " << get_value(i)
	       << "where info had value " << nai.get_value(i)
	       << fatal_error;
	return FALSE;
     }
  
  return TRUE;
}


/***************************************************************************
  Description : Reads in an attribute value from the stream.
                Returns attribute value.
  Comments    : Attribute value to be read assumed to match AttrInfo.
		Takes time proportional to the length of the attribute value
		  + the number of nominal attribute values for that 
		  attribute.
***************************************************************************/
AttrValue_ NominalAttrInfo::read_attr_value(MLCIStream& stream,
					    int& line) const
{
  MStringRC str = read_word_on_same_line(stream, line, TRUE);
  NominalAttrValue_ av;
  int val;

  // similar to nominal_to_int, but checks for errors.
  if (str == UNKNOWN_VAL_STR)
    val = UNKNOWN_NOMINAL_VAL;
  else {
     for (val = values.low(); val <= values.high() && 
				   get_value(val) != str; val++)
        ; // NULL
     if (val == values.high() + 1) {
	err << "NominalAttrInfo::read_attr_value: " << str << " is not "
               "a valid attribute value for " << name()
	    << " at line " << line << ".\nPossible values are: ";
        display_attr_values(err);
        err << fatal_error;
     }
  }

  set_nominal_val(av, val);
  return av;
}


/***************************************************************************
  Description : Returns a pointer to a deep copy of the object. 
  Comments    : The caller becomes the owner.
***************************************************************************/
AttrInfo* NominalAttrInfo::clone() const
{
   ATTRDBG(ASSERT(type() == nominal));
   return new NominalAttrInfo(*this, ctorDummy);
}


/***************************************************************************
  Description : Returns TRUE iff given AttrValue_s are equivalent.
  Comments    : Assumes that given AttrValue_s are both of the type described
		by the instance of NominalAttrInfo calling this function.
***************************************************************************/
Bool NominalAttrInfo::_equal_value(const AttrValue_ val1,
                                   const AttrValue_ val2) const
{
   ATTRDBG(check_valid_attr_value_type(val1); check_valid_attr_value_type(val2));
   return (val1.value.intVal == val2.value.intVal);
}


/***************************************************************************
  Description : Returns TRUE iff given AttrValue_ is unknown.
  Comments    : Assumes that given AttrValue_ is of the type described by
		the instance of NominalAttrInfo calling this function.
***************************************************************************/
Bool NominalAttrInfo::is_unknown(const AttrValue_ nominalValue) const
{
   ATTRDBG(check_valid_attr_value_type(nominalValue));
   return (nominalValue.value.intVal == UNKNOWN_NOMINAL_VAL + NOMINAL_OFFSET);
}


/***************************************************************************
  Description : Sets the given AttrValue_ to be unknown.
  Comments    :
***************************************************************************/
void NominalAttrInfo::set_unknown(AttrValue_& val) const
{
   set_nominal_val(val, UNKNOWN_NOMINAL_VAL);
}


/***************************************************************************
  Description : This returns 1 if the AttrValue_s are different, and 0
                   if the AttrValue_s are equal.
  Comments    : Handling of unknown values is silly. 
***************************************************************************/
Real NominalAttrInfo::distance(const AttrValue_ val1,
			       const AttrValue_ val2) const
{
   DBGSLOW(check_in_range(val1);check_in_range(val2));
   if (is_unknown(val1) || is_unknown(val2))
      if (is_unknown(val1) && is_unknown(val2))
	 return 0;
      else
	 return 1;

   if (val1.value.intVal == val2.value.intVal)
      return 0;
   return 1;
}


/***************************************************************************
  Description : Displays the attribute values in names file format.
                ProtectChars forces quoting of special characters (now
		  only periods).
  Comments    :
***************************************************************************/


void NominalAttrInfo::display_attr_values(MLCOStream& stream,
					  Bool protectChars) const
{
   int numValues = num_values();
   for (int i = 0; i < numValues; i++) {
      if (protectChars)
	 stream << protect_chars(get_value(FIRST_CATEGORY_VAL+i));
      else 
	 stream << get_value(FIRST_CATEGORY_VAL+i);
      if (i != numValues - 1)
         stream << ", ";
      else
         stream << "." << endl;
   }
}


/***************************************************************************
  Description : Sets/returns the integer representation of the given
                   AttrValue_.
  Comments    :
***************************************************************************/
void NominalAttrInfo::set_nominal_val(AttrValue_& av, int val) const
{
   ATTRDBG(if (av.type == unknown)
          av.type = nominal;
       else if (av.type != nominal)
          err << "NominalAttrInfo::set_nominal_val: Cannot assign a nominal "
                 "value to a " << attr_type_to_string(av.type) << " AttrValue_"
              << fatal_error);

   av.value.intVal = val + NOMINAL_OFFSET;
   DBG(check_in_range(av));
}


/***************************************************************************
  Description : Causes fatal_error if the given AttrValue_ is not of type real.
  Comments    :
***************************************************************************/
void RealAttrInfo::check_valid_attr_value_type(const AttrValue_) const
{

}


/***************************************************************************
  Description : Constructors.
  Comments    : There is an AttrType parameter so that subclasses
                  (e.g. BoundedRealAttrInfo) can override the default with
		  their own AttrType.
***************************************************************************/
RealAttrInfo::RealAttrInfo(const MString& aname, AttrType atype)
  : AttrInfo(aname, atype), MetricAttrInfo(aname, atype),
    LinearAttrInfo(aname, atype)
{
   min = max = UNKNOWN_REAL_VAL;
}


/***************************************************************************
  Description : Copy constructor. 
  Comments    :
***************************************************************************/
RealAttrInfo::RealAttrInfo(const RealAttrInfo& source,
			   CtorDummy dummyArg):
  AttrInfo(source, dummyArg), MetricAttrInfo(source, dummyArg),
  LinearAttrInfo(source, dummyArg)
{
   min = source.min;
   max = source.max;
}
    


/***************************************************************************
  Description : Returns a string that corresponds to the given AttrValue_.
  Comments    : Returns "?" for UNKNOWN_REAL_VAL.
***************************************************************************/
MString RealAttrInfo::attrValue_to_string(const AttrValue_ val) const
{
   if (is_unknown(val))
      return UNKNOWN_VAL_STR;
   return MString(val.value.realVal, 0);
}


/***************************************************************************
  Description : Returns an AttrValue_ which has its realVal field set to
                  the value indicated by the next item in the given stream.
  Comments    : '?' indicates an unknown real value.
                The value to be read should be on the line indicated
		  by the line parameter, otherwise an error will occur.
***************************************************************************/
AttrValue_ RealAttrInfo::read_attr_value(MLCIStream& stream, int& line) const
{
   const int startLine = line;
   static RealAttrValue_ attrValue;
   if (!skip_white_comments_same_line(stream, line))
      err << "RealAttrInfo::read_attr_value: attribute value expected on "
	     "line " << startLine << " but it was on line " << line
          << fatal_error;
   char ch = stream.peek();

   if (ch != '?') {
      MStringRC realStr = read_word(stream, line, FALSE, TRUE);
      attrValue.value.realVal = realStr.real_value();
   } else {
      MStringRC str = read_word_on_same_line(stream, line, TRUE);
      DBGSLOW(ASSERT(str[0] == '?'));
      attrValue.value.realVal = UNKNOWN_REAL_VAL;
   }
   return attrValue;
}


/***************************************************************************
  Description : Returns equal_shallow().
  Comments    : 
***************************************************************************/
Bool RealAttrInfo::equal(const AttrInfo& ai, Bool fatalOnFalse) const
{
   return equal_shallow(ai, fatalOnFalse);
}


/***************************************************************************
  Description : Causes a fatal error if one of the values is
                  UNKNOWN_REAL_VAL.
  Comments    : Assumes that the given AttrValue_s are of type Real.
***************************************************************************/
void RealAttrInfo::check_valid_real_comparison_operand(const AttrValue_ val1,
						 const AttrValue_ val2) const
{
   ATTRDBG(check_valid_attr_value_type(val1); check_valid_attr_value_type(val2));
   if (val1.value.realVal == UNKNOWN_REAL_VAL ||
       val2.value.realVal == UNKNOWN_REAL_VAL)
      err << "Attribute.c::check_valid_real_comparison_operand: Comparison "
	     "methods are undefined for UNKNOWN_REAL_VAL" << fatal_error;
}

/***************************************************************************
  Description : Comparators.  Use the built-in comparators for Reals.
  Comments    :
***************************************************************************/
Bool RealAttrInfo::less_than(const AttrValue_ val1,
			     const AttrValue_ val2) const
{
   check_valid_real_comparison_operand(val1, val2);
   return val1.value.realVal < val2.value.realVal;
}


Bool RealAttrInfo::less_than_equal(const AttrValue_ val1,
				   const AttrValue_ val2) const
{
   check_valid_real_comparison_operand(val1, val2);
   return val1.value.realVal <= val2.value.realVal;
}


Bool RealAttrInfo::greater_than(const AttrValue_ val1,
				const AttrValue_ val2) const
{
   check_valid_real_comparison_operand(val1, val2);
   return val1.value.realVal > val2.value.realVal;
}


Bool RealAttrInfo::greater_than_equal(const AttrValue_ val1,
				      const AttrValue_ val2) const
{
   check_valid_real_comparison_operand(val1, val2);
   return val1.value.realVal >= val2.value.realVal;
}


Bool RealAttrInfo::equal(const AttrValue_ val1, const AttrValue_ val2) const
{
   check_valid_real_comparison_operand(val1, val2);
   return fabs(val1.value.realVal - val2.value.realVal) < STORED_REAL_EPSILON;
}


Bool RealAttrInfo::not_equal(const AttrValue_ val1, const AttrValue_ val2) const
{
   check_valid_real_comparison_operand(val1, val2);
   return fabs(val1.value.realVal - val2.value.realVal) >= STORED_REAL_EPSILON;
}


/***************************************************************************
  Description : Returns the normalized distance between the two given values.
                Note: It is customary to square this or take fabs.
  Comments    : 
***************************************************************************/
Real RealAttrInfo::distance(const AttrValue_ val1,
			    const AttrValue_ val2) const
{
   if (is_unknown(val1) || is_unknown(val2))
      if (is_unknown(val1) && is_unknown(val2))
	 return 0;
      else
	 return 1;

   return normalized_value(val1) - normalized_value(val2);
}


/***************************************************************************
  Description : Returns a pointer to a deep copy of the object. 
  Comments    : The caller becomes the owner.
***************************************************************************/
AttrInfo* RealAttrInfo::clone() const
{
   ATTRDBG(ASSERT(type() == real));
   return new RealAttrInfo(*this, ctorDummy);
}


/***************************************************************************
  Description : Returns TRUE iff given AttrValue_s are equivalent.
  Comments    : Assumes that given AttrValue_s are both of the type described
		by the instance of RealAttrInfo calling this function.
***************************************************************************/
// @@ why does this exist if we have equal() ?
Bool RealAttrInfo::_equal_value(const AttrValue_ realValue1,
                                const AttrValue_ realValue2) const
{
   return fabs(realValue1.value.realVal - realValue2.value.realVal)
      < STORED_REAL_EPSILON;
}



/***************************************************************************
  Description : Sets the given AttrValue_ to be unknown.
  Comments    :
***************************************************************************/
void RealAttrInfo::set_unknown(AttrValue_& val) const
{
   set_real_val(val, UNKNOWN_REAL_VAL);
}


/***************************************************************************
  Description : Displays the attribute values in names file format.
  Comments    :
***************************************************************************/
void RealAttrInfo::display_attr_values(MLCOStream& stream, Bool) const
{
   stream << "continuous." << endl;
}


/***************************************************************************
  Description : Returns a normalized value for the given AttrValue_.
                Based on the normalization ranges min/max, we will
		  return values which are usually between 0 and 1,
		  but even under normalization::extreme, if the test
		  set has values outside the ranges, we may generate
                  values outside the range.  This is especially
		  important for interquartile normalization, where
		  "noisy" instances can be outside the range and
		  will thus be seen as far.
		If the minimum and maximum are equal, it returns 0.5.
		Causes fatal_error for unknown AttrValue_.
  Comments    :
***************************************************************************/
Real RealAttrInfo::normalized_value(const AttrValue_ val) const
{
   DBG(if (min == UNKNOWN_REAL_VAL || max == UNKNOWN_REAL_VAL)
          err << "RealAttrInfo::normalized_value: Minimum and maximum values "
                 "for RealAttrInfo " << name() << " have not been set"
              << fatal_error;
      ASSERT(min <= max);
      if (is_unknown(val))
         err<<"RealAttrInfo::normalized_value: Unknown attribute value passed "
	     "to RealAttrInfo " << name() << fatal_error);
   if (min == max)
      return 0.5;
   else
      return (val.value.realVal - min) / (max - min);
}


/***************************************************************************
  Description : Set/get the minimum/maximum value for the RealAttrInfo.
                These values are used for normalizing.
  Comments    :
***************************************************************************/
void RealAttrInfo::set_min(Real newMin)
{
   if (newMin == UNKNOWN_REAL_VAL)
      err << "RealAttrInfo::set_min: Cannot set minimum to unknown"
	  << fatal_error; 
   min = newMin;
}

void RealAttrInfo::set_max(Real newMax)
{
   if (newMax == UNKNOWN_REAL_VAL)
      err << "RealAttrInfo::set_max: Cannot set maximum to unknown"
	  << fatal_error; 
   max = newMax;
}


Real RealAttrInfo::get_min() const
{
   return min;
}


Real RealAttrInfo::get_max() const
{
   return max;
}


/***************************************************************************
  Description : Sets the real valued representation of the given
                   AttrValue_.
  Comments    :
***************************************************************************/
void RealAttrInfo::set_real_val(AttrValue_& av, Real val) const
{
   ATTRDBG(if (av.type == unknown)
          av.type = real;
       else if (av.type != real)
          err << "RealAttrInfo:set_real_val: Cannot assign a real value to a"
              << attr_type_to_string(av.type) << "AttrValue_"
              << fatal_error);
   av.value.realVal = val;
}


