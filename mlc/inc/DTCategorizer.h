// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _DTCategorizer_h
#define _DTCategorizer_h 1

#include <RDGCat.h>
#include <DecisionTree.h>

// DT is Decision Tree
class DTCategorizer: public RDGCategorizer {
public:
  virtual void OK(int level = 0)const;
  // gets ownership of DecisionTree
  DTCategorizer(DecisionTree*& dt, const MString& dscr, int numCat);
  DTCategorizer(const DTCategorizer& source, CtorDummy);
  virtual ~DTCategorizer();
  virtual Categorizer* copy() const;
};

#endif
