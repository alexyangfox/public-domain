// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _ConstCategorizer_h
#define _ConstCategorizer_h 1

#include <Categorizer.h>

// Const Categorizer returns a constant category independent of the
// instance.
class ConstCategorizer : public Categorizer {
  const AugCategory category;
  NO_COPY_CTOR(ConstCategorizer);
public:
   // We copy dscr
   ConstCategorizer(const MString& dscr, const AugCategory& augCat);
   ConstCategorizer(const ConstCategorizer&, CtorDummy);
   virtual ~ConstCategorizer();
   // ignore instance
   virtual AugCategory categorize(const InstanceRC&) const
                                                     {return category;}
   virtual AugCategory get_category() const { return category;}
   virtual void display_struct(MLCOStream& stream = Mcout,
			    const DisplayPref& dp = defaultDisplayPref) const;
   // Returns a pointer to a deep copy of this ConstCategorizer
   virtual Categorizer* copy() const;

   // Returns the class ID
   virtual int class_id() const { return CLASS_CONST_CATEGORIZER; }

   virtual Bool operator==(const Categorizer& cat) const ;
   virtual Bool operator==(const ConstCategorizer &cat)const ;
};

#endif
