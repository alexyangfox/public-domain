// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _AttrEqCategorizer_h
#define _AttrEqCategorizer_h_h 1

#include <Categorizer.h>

// AttrEqCategorizer categorizes according to the value of a single
// attribute.
class AttrEqCategorizer : public Categorizer {
   AttrInfo *attrInfo;
   const int attrNum;
   const NominalVal attrVal;
   NO_COPY_CTOR(AttrEqCategorizer);

   const MString equal;
   const MString notEqual;
   const MString unknown;
   Bool  separateUnknowns; // On unknown, return unknown or simply not-equal
public:
   AttrEqCategorizer(const SchemaRC& schema, int attrNum,
		     NominalVal attrVal, const MString& dscr,
		     Bool separateUnknown);
   AttrEqCategorizer(const AttrInfo& aInfo, int attributeNum,
		     NominalVal attrVal, const MString& dscr,
		     Bool separateUnknown);
   AttrEqCategorizer(const AttrEqCategorizer& source, CtorDummy);
   virtual ~AttrEqCategorizer() {delete attrInfo;}
   virtual AugCategory categorize(const InstanceRC&) const;
   virtual void display_struct(MLCOStream& stream = Mcout,
			       const DisplayPref& dp=defaultDisplayPref) const;
   // Returns a pointer to a deep copy of this AttrEqCategorizer
   virtual Categorizer* copy() const;
   // Returns the class id
   virtual int class_id() const { return CLASS_ATTR_EQ_CATEGORIZER; }

   virtual Bool operator==(const Categorizer &cat) const;
   virtual Bool operator==(const AttrEqCategorizer &cat) const;
};


#endif
