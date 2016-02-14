// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _ThresholdCat_h
#define _ThresholdCat_h 1

#include <Categorizer.h>

class ThresholdCategorizer : public Categorizer {
   const AttrInfo& attrInfo;
   const int attrNum;
   Real thresholdVal;
   MString LTEDscr, GTDscr; // "less than or equal to" description
                            // "greater than" description
   NO_COPY_CTOR(ThresholdCategorizer);
public:
   ThresholdCategorizer(const SchemaRC& schema, int attributeNum,
                        Real threshold, const MString& dscr);
   ThresholdCategorizer(const AttrInfo& ai, int attributeNum, 
			Real threshold, const MString& dscr);
   ThresholdCategorizer(const ThresholdCategorizer& source,
                        CtorDummy);
   virtual AugCategory categorize(const InstanceRC&) const;
   virtual void display_struct(MLCOStream& stream = Mcout,
			       const DisplayPref& dp=defaultDisplayPref) const;
   // Returns a pointer to a deep copy of this ThresholdCategorizer
   virtual Categorizer* copy() const;
   // Returns class id
   virtual int class_id() const { return CLASS_THRESHOLD_CATEGORIZER; }
   virtual Bool operator==(const Categorizer &cat) const ;
   virtual Bool operator==(const ThresholdCategorizer &cat) const;
   int attr_num() const { return attrNum; }
   Real threshold() const {return thresholdVal;}
};

#endif
