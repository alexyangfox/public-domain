// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _AugCategory_h
#define _AugCategory_h 1

#include <Attribute.h>
#include <MStringRC.h>

class AugCategory {
   // Default copy constructor OK.
   const Category cat;
   const MStringRC  catDscr;
public:
   AugCategory(Category aCat, const MStringRC& dscr);
   virtual ~AugCategory() {}
   int num() const {return cat;}
   MStringRC description() const {return catDscr;}
   operator Category() const { return cat;}
   AugCategory(const AugCategory& ac);
   void display(MLCOStream& ostream) const;
   Bool operator==(const AugCategory& ac) const;
   Bool operator!=(const AugCategory& ac) const {
      return !operator==(ac);}
};

DECLARE_DISPLAY(AugCategory);

extern const AugCategory UNKNOWN_AUG_CATEGORY;

#endif
