// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "ConstInducer.c".

#ifndef _ConstInducer_h
#define _ConstInducer_h 1

#include <Inducer.h>

class ConstCategorizer;

class ConstInducer : public Inducer {
   ConstCategorizer* categorizer;
public:
   virtual void OK(int level = 0) const;
   ConstInducer(const MString& dscr) : Inducer(dscr) {categorizer = NULL;}
   ~ConstInducer();
   virtual void train();                                    // build structure
   // display learned structure
   virtual Bool was_trained(Bool fatalOnFalse = TRUE) const;
   virtual const Categorizer& get_categorizer() const; 
};


#endif
