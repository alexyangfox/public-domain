// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _AttrCategorizer_h
#define _AttrCategorizer_h_h 1

#include <Categorizer.h>

// AttrCategorizer categorizes according to the value of a single
// attribute.
class AttrCategorizer : public Categorizer {
   AttrInfo *attrInfo;
   const int attrNum;
   NO_COPY_CTOR(AttrCategorizer);
public:
   AttrCategorizer(const SchemaRC& schema, int attrNum,
                   const MString& dscr);
   AttrCategorizer(const AttrInfo& aInfo, int attributeNum,
		   const MString& dscr);
   AttrCategorizer(const AttrCategorizer& source, CtorDummy);
   virtual ~AttrCategorizer() {delete attrInfo;}
   virtual AugCategory categorize(const InstanceRC&) const;
   virtual void display_struct(MLCOStream& stream = Mcout,
			       const DisplayPref& dp=defaultDisplayPref) const;
   // Returns a pointer to a deep copy of this AttrCategorizer
   virtual Categorizer* copy() const;
   // Returns the class id
   virtual int class_id() const { return CLASS_ATTR_CATEGORIZER; }

   virtual Bool operator==(const Categorizer &cat) const;
   virtual Bool operator==(const AttrCategorizer &cat) const;
   int attr_num(void) const { return attrNum; }
};


#endif
