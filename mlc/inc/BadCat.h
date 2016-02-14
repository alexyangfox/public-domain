// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _BadCategorizer_h
#define _BadCategorizer_h 1

#include <Categorizer.h>

const MString BadCatDscr = "Bad Categorizer";

class BadCategorizer : public Categorizer {
  static Bool instantiated;
public:
  BadCategorizer();
  virtual ~BadCategorizer() {}
  virtual int num_categories() const;
  virtual AugCategory categorize(const InstanceRC&) const;
  virtual void display_struct(MLCOStream& stream = Mcout,
			  const DisplayPref& dp = defaultDisplayPref) const;
  virtual Categorizer* copy() const;

  // Returns the class id
  virtual int class_id() const { return CLASS_BAD_CATEGORIZER; };

  virtual Bool operator==(const Categorizer&) const;

  static Bool is_bad_categorizer(const Categorizer&);
};

extern BadCategorizer badCategorizer;
#endif
