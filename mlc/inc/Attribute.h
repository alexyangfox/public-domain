// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _Attribute_h
#define _Attribute_h 1

#include <MString.h>
#include <DblLinkList.h>
#include <Pix.h>
#include <Array.h>

const int  NOMINAL_OFFSET = 1000;

// Declare a macro specifically for attribute debugging
#ifdef ATTR_DEBUG
#define ATTRDBG(stmts) if (debugLevel >= 1) {stmts;} else
#define ATTR_DECLARE(stmts) stmts
#else
#define ATTRDBG(stmts)
#define ATTR_DECLARE(stmts)
#endif

// Both Category and NominalVal are integers mapped from MStrings. 
// Values are mapped from FIRST_CATEGORY_VAL = UNKNOWN_CATEGORY_VAL+1,
// thus you can dimension arrays by the number of nominal values plus 1.
typedef int Category;
typedef int NominalVal;
typedef Array<MString> MStringArray;
extern const int UNKNOWN_CATEGORY_VAL;
extern const int UNKNOWN_NOMINAL_VAL;
extern const int FIRST_CATEGORY_VAL;
extern const int FIRST_NOMINAL_VAL;
extern const int MAX_NUM_CATEGORIES; // Safety only, may be increased.
extern const Real UNKNOWN_REAL_VAL;

typedef enum {unknown, real, boundedReal, nominal, linearNominal,
	      treeStructured, internalDisjunction, userReal,
	      userNominal, userLinearNominal, userTreeStructured,
	      userInternalDisjunction } AttrType;

// Returns the MString corresponding to the AttrType	      
MString attr_type_to_string(AttrType attrType);
MString protect_chars(const MString& str);

/***********************************************************************
 Attributes
 **********************************************************************/
// Possible values of attributes.
// The actual attribute type must be known in order to parse this union.
// Values can only be accessed through the AttrInfo
class AttrValue_ {
friend class NominalAttrInfo;
friend class RealAttrInfo;
   
// Need access to data members for copy constructors and assignment operator
friend class RealAttrValue_;
friend class NominalAttrValue_;
friend class InstanceHashTable_; // must have access to raw data   
   union {
      int  intVal;    // All nominal attributes are converted to int
      StoredReal realVal;  
      // The pointer below causes problems when OC is instrumented.  It claims
      //   that AttrValue is pointing to an illegal place (obviously, it was
      //   assigned an int).   
      //   void    *treeStructured; // will need pointer for this.
   } value;
   ATTR_DECLARE(AttrType type;)
   // type must go at the bottom because InstanceHash expects the value
   // union to be the first member of this class.

public:
   AttrValue_() {
      ATTRDBG(type = unknown;
	      // Zero out all of the union to avoid oc warnings.
	      memset(&value, 0, sizeof(value)));
   }
   AttrValue_(const AttrValue_& src) {
      ATTRDBG(type = src.type);
      value = src.value;
   }
   AttrValue_(AttrType aType) ATTR_DECLARE(: type(aType)) {(void)aType;}
   AttrValue_& operator=(const AttrValue_& src) {
      if (&src != this) {
	 ATTRDBG(type = src.type);
	 value = src.value;
      }
      return *this;
   }

};


class NominalAttrValue_ : public AttrValue_ {
   NominalAttrValue_(const RealAttrValue_&);
   NominalAttrValue_& operator=(const RealAttrValue_&);
public:
   NominalAttrValue_();
   NominalAttrValue_(const AttrValue_&);
   NominalAttrValue_(const NominalAttrValue_&);
   NominalAttrValue_& operator=(const AttrValue_&);
};


class RealAttrValue_ : public AttrValue_ {
   RealAttrValue_(const NominalAttrValue_&);
   RealAttrValue_& operator=(const NominalAttrValue_&);
public:
   RealAttrValue_();
   RealAttrValue_(const AttrValue_&);
   RealAttrValue_(const RealAttrValue_&);
   RealAttrValue_& operator=(const AttrValue_&);
};


// For Boolean domains, the attribute is either positive or negated.
struct Literal {
   int  attrNum;
   Bool negated;
};

// used as parameters
class NominalAttrInfo;
class RealAttrInfo;
class _Instance;
class InstanceInfo;

// Information about attribute or label
class AttrInfo { //ABC
   const MString   attrName;   // attribute name
   AttrType attrType;   // attribute type
   NO_COPY_CTOR(AttrInfo);
protected:
   Bool equal_shallow(const AttrInfo&, Bool fatalOnFalse) const;
   AttrType  type() const {return attrType;}
public:
   AttrInfo(const MString& aname, AttrType atype);
   // copy constructor
   AttrInfo(const AttrInfo& source, CtorDummy);
   virtual ~AttrInfo() {}
   const MString&   name() const {return attrName;}
   virtual void check_in_range(const AttrValue_) const = 0;
   // Note that we do not return a MString& because in some cases (say
   //   the attribute is real or the value is unknown), we have to create
   //   the string.
   virtual MString attrValue_to_string(const AttrValue_) const = 0;
   // Assumes the given AttrValue_s are of the same AttrInfo as this.
   virtual Bool equal(const AttrInfo&, Bool fatalOnFalse) const = 0;
   virtual Bool operator==(const AttrInfo& info) const
      {return equal_shallow(info, FALSE);}
   virtual Bool operator!=(const AttrInfo& info) const
      {return !equal_shallow(info, FALSE);}
   // Causes fatal error if not derived from NominalAttrInfo
   virtual NominalAttrInfo& cast_to_nominal();
   virtual const NominalAttrInfo& cast_to_nominal() const;
   virtual RealAttrInfo& cast_to_real();
   virtual const RealAttrInfo& cast_to_real() const;
   // Beware of using can_cast_to_*.  It is usually the case
   //   that a virtual function would do the job better than an if().
   virtual Bool can_cast_to_nominal() const;
   virtual Bool can_cast_to_real() const;
   virtual AttrValue_ read_attr_value(MLCIStream&, int& line) const = 0;
   virtual AttrInfo* clone() const = 0;
   virtual Bool _equal_value(const AttrValue_, const AttrValue_) const = 0;
   virtual Bool is_unknown(const AttrValue_) const = 0;
   virtual void display(MLCOStream& stream = Mcout) const;
   virtual void display_attr_values(MLCOStream& stream = Mcout, 
				    Bool protectChars = FALSE) const = 0;
   virtual Real distance(const AttrValue_, const AttrValue_) const = 0;
   virtual void set_nominal_val(AttrValue_&, int) const;
   virtual NominalVal get_nominal_val(const AttrValue_) const;
   virtual void set_real_val(AttrValue_&, Real) const;
   virtual Real get_real_val(const AttrValue_) const;
   virtual void set_unknown(AttrValue_&) const = 0;
   virtual int storage_size() const;
};

DECLARE_DISPLAY(AttrInfo);

class PartialOrderAttrInfo : public virtual AttrInfo { // ABC
   NO_COPY_CTOR(PartialOrderAttrInfo);
public:
  PartialOrderAttrInfo(const MString& aname, AttrType atype);
  // copy constructor
  PartialOrderAttrInfo(const PartialOrderAttrInfo& source,
		       CtorDummy dummyArg);
  virtual ~PartialOrderAttrInfo() {}
  virtual AttrInfo* clone() const = 0;
  virtual Bool _equal_value(const AttrValue_, const AttrValue_) const = 0;
  virtual Bool is_unknown(const AttrValue_) const = 0;
};


// Linear attributes have an ordered set of mutually exclusive values
// (e.g. reals or integers).
class LinearAttrInfo : public PartialOrderAttrInfo { //ABC
    NO_COPY_CTOR(LinearAttrInfo);
public:
   LinearAttrInfo(const MString& aname, AttrType atype);
   // copy constructor.
   LinearAttrInfo(const LinearAttrInfo& source, CtorDummy);
   virtual ~LinearAttrInfo() {}
   virtual Bool equal(const AttrInfo&, Bool fatalOnFalse) const = 0;
   virtual Bool less_than(const AttrValue_, const AttrValue_) const = 0;
   virtual Bool less_than_equal(const AttrValue_, const AttrValue_) const;
   virtual Bool greater_than(const AttrValue_, const AttrValue_) const;
   virtual Bool greater_than_equal(const AttrValue_, const AttrValue_) const;
   virtual Bool equal(const AttrValue_, const AttrValue_) const;
   virtual Bool not_equal(const AttrValue_, const AttrValue_) const;
   virtual AttrInfo* clone() const = 0;
   virtual Bool _equal_value(const AttrValue_, const AttrValue_) const = 0;
   virtual Bool is_unknown(const AttrValue_) const = 0;
};


// MetricAttrInfo has a distance method defined for it
// (e.g. 2-D plane is metric without order)
class MetricAttrInfo : public virtual AttrInfo { // ABC
    NO_COPY_CTOR(MetricAttrInfo);
public:
   MetricAttrInfo(const MString& aname, AttrType atype);
   // copy constructor.
   MetricAttrInfo(const MetricAttrInfo& source,  CtorDummy);
   virtual ~MetricAttrInfo() {}
   // Returned value should always be non-negative.
   virtual AttrInfo* clone() const = 0;
   virtual Bool _equal_value(const AttrValue_, const AttrValue_) const = 0;
   virtual Bool is_unknown(const AttrValue_) const = 0;
};


// A nominal attribute takes on a finite, unordered set of mutually
// exclusive values (e.g. color).
class NominalAttrInfo : public virtual AttrInfo {
   MStringArray values;

   NO_COPY_CTOR(NominalAttrInfo);
   void check_valid_attr_value_type(const AttrValue_) const;
public:
   // Gets ownership of attrVals
   NominalAttrInfo(const MString& aname, 
		   DblLinkList<MString>*& attrVals, AttrType atype = nominal);
   // copy constructor IS implemented.
   NominalAttrInfo(const NominalAttrInfo& source, CtorDummy );
   virtual ~NominalAttrInfo();
   int num_values() const;
   const MString& get_value(int i) const;
   virtual void check_in_range(const AttrValue_) const;
   virtual MString attrValue_to_string(const AttrValue_) const;
   virtual int nominal_to_int(const MString& valName) const;
   virtual Bool equal(const AttrInfo&, Bool fatalOnFalse) const;
   virtual Bool operator==(const AttrInfo& info) const
      {return equal(info, FALSE);}
   virtual Bool operator!=(const AttrInfo& info) const
      {return !equal(info, FALSE);}
   virtual NominalAttrInfo& cast_to_nominal() {return *this;}
   virtual const NominalAttrInfo& cast_to_nominal() const {return *this;}
   // Beware of using can_cast_to_nominal.  It is usually the case
   //   that a virtual function would do the job better than an if().
   virtual Bool can_cast_to_nominal() const {return TRUE;}
   virtual AttrValue_ read_attr_value(MLCIStream&, int& line) const;
   virtual AttrInfo* clone() const;
   virtual Bool _equal_value(const AttrValue_, const AttrValue_) const;
   virtual Bool is_unknown(const AttrValue_) const;
   virtual Real distance(const AttrValue_, const AttrValue_) const;
   virtual void display_attr_values(MLCOStream& stream = Mcout, 
				    Bool protectChars = FALSE) const;
   virtual void set_nominal_val(AttrValue_&, int) const;
   inline virtual NominalVal get_nominal_val(const AttrValue_) const;
   virtual void set_unknown(AttrValue_&) const;
   virtual int storage_size() const { return sizeof(int); }
};


// Profiling of ID3/shuttle showed this is a time bottleneck.
// Note that it's important to use get_nominal_val on NominalAttrInfo
//   directly because then the virtual mechanism is not used.
NominalVal NominalAttrInfo::get_nominal_val(const AttrValue_ av) const
{
   ATTRDBG(if (av.type != nominal)
          err << "NominalAttrInfo::get_nominal_val: Cannot get a nominal "
                 "value from a " << attr_type_to_string(av.type)
              << " AttrValue." << fatal_error);
   DBGSLOW(check_in_range(av));
   return av.value.intVal - NOMINAL_OFFSET;
}




// LinearNominal attributes have a finite ordered set of
// mutually exclusive values.
// Note that we do not want to support an infinite number of attribute
// values here because each value must have a string descriptor (which
// we store in an array)
class LinearNominalAttrInfo : public LinearAttrInfo, public NominalAttrInfo {
   NO_COPY_CTOR(LinearNominalAttrInfo);
public:
   LinearNominalAttrInfo(const MString& aname,
			 DblLinkList<MString>*& attrVals, 
			 AttrType atype = linearNominal);
   // copy constructor
   LinearNominalAttrInfo(const LinearNominalAttrInfo& source,
			 CtorDummy );
   virtual ~LinearNominalAttrInfo() {}
   virtual Bool equal(const AttrInfo&, Bool fatalOnFalse) const;
   virtual Bool less_than(const AttrValue_, const AttrValue_) const;
   virtual Bool less_than_equal(const AttrValue_, const AttrValue_) const;
   virtual Bool greater_than(const AttrValue_, const AttrValue_) const;
   virtual Bool greater_than_equal(const AttrValue_, const AttrValue_) const;
   virtual Bool equal(const AttrValue_, const AttrValue_) const;
   virtual Bool not_equal(const AttrValue_, const AttrValue_) const;
   virtual AttrInfo* clone() const;
   virtual Bool _equal_value(const AttrValue_, const AttrValue_) const;
   virtual Bool is_unknown(const AttrValue_) const;
};


class RealAttrInfo : public MetricAttrInfo, public LinearAttrInfo {
   Real min, max;

   NO_COPY_CTOR(RealAttrInfo);
   void check_valid_real_comparison_operand(const AttrValue_,
					    const AttrValue_) const;
   void check_valid_attr_value_type(const AttrValue_) const;
public:
   RealAttrInfo(const MString& aname, AttrType atype = real);
   // copy constructor
   RealAttrInfo(const RealAttrInfo& source, CtorDummy );
   virtual ~RealAttrInfo() {}
   virtual void check_in_range(const AttrValue_) const {}
   virtual MString attrValue_to_string(const AttrValue_) const;
   virtual RealAttrInfo& cast_to_real() {return *this;}
   virtual const RealAttrInfo& cast_to_real() const {return *this;}
   virtual Bool can_cast_to_real() const {return TRUE;}
   virtual AttrValue_ read_attr_value(MLCIStream&, int& line) const;
   virtual Bool equal(const AttrInfo&, Bool fatalOnFalse) const;
   virtual Bool less_than(const AttrValue_, const AttrValue_) const;
   virtual Bool less_than_equal(const AttrValue_, const AttrValue_) const;
   virtual Bool greater_than(const AttrValue_, const AttrValue_) const;
   virtual Bool greater_than_equal(const AttrValue_, const AttrValue_) const;
   virtual Bool equal(const AttrValue_, const AttrValue_) const;
   virtual Bool not_equal(const AttrValue_, const AttrValue_) const;
   virtual Real distance(const AttrValue_, const AttrValue_) const;
   virtual AttrInfo* clone() const;
   virtual Bool _equal_value(const AttrValue_, const AttrValue_) const;
   inline virtual Bool is_unknown(const AttrValue_) const;
   virtual void display_attr_values(MLCOStream& stream = Mcout, 
				    Bool protectChars = FALSE) const;
   virtual Real normalized_value(const AttrValue_) const;
   virtual void set_min(Real);
   virtual void set_max(Real);
   virtual Real get_min() const;
   virtual Real get_max() const;
   virtual void set_real_val(AttrValue_&, Real) const;
   inline virtual Real get_real_val(const AttrValue_) const;
   virtual void set_unknown(AttrValue_&) const;
   virtual int storage_size() const { return sizeof(StoredReal); }
};

// Profiling of ID3/shuttle showed this is a time bottleneck.
// Note that it's important to use this on RealAttrInfo 
//   directly because then the virtual mechanism is not used.

/***************************************************************************
  Description : Returns TRUE iff given AttrValue_ is unknown.
  Comments    : Assumes that given AttrValue_ is of the type described by
		the instance of RealAttrInfo calling this function.
***************************************************************************/
Bool RealAttrInfo::is_unknown(const AttrValue_ realValue) const
{
   return (realValue.value.realVal == UNKNOWN_REAL_VAL);
}

/***************************************************************************
  Description : Return the real valued representation of the given
                   AttrValue_.
  Comments    :
***************************************************************************/

Real RealAttrInfo::get_real_val(const AttrValue_ av) const
{
   ATTRDBG(if (av.type != real)
          err << "RealAttrInfo::get_real_val: Cannot get a real value "
                 "from a " << attr_type_to_string(av.type) << "AttrValue_"
              << fatal_error);
   DBG(if (is_unknown(av))
      err << "RealAttrInfo::get_real_val: trying to get UNKNOWN value"
          << fatal_error);	 
   return av.value.realVal;
}





class IntegerAttrInfo : public MetricAttrInfo, public LinearAttrInfo {
   virtual AttrInfo* clone() const;
   virtual Bool _equal_value(const AttrValue_, const AttrValue_) const;
   virtual Bool is_unknown(const AttrValue_) const;
   virtual Real distance(const AttrValue_, const AttrValue_) const;
   virtual int storage_size() const { return sizeof(int); }
};


// I am not sure how these values will be stored.  Perhaps the are
// TreeStructured with the structure having numerator and denominator fields?
class RationalAttrInfo : public MetricAttrInfo, public LinearAttrInfo {
   virtual AttrInfo* clone() const;
   virtual Bool _equal_value(const AttrValue_, const AttrValue_) const;
   virtual Bool is_unknown(const AttrValue_) const;
   virtual Real distance(const AttrValue_, const AttrValue_) const;
};

#endif




