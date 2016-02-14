// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _ID3Inducer_h
#define _ID3Inducer_h 1

#include <TDDTInducer.h>
#include <SplitInfo.h>

class ID3Inducer : public TDDTInducer {
   int level;
public:
   ID3Inducer(const MString& dscr, CGraph* aCgraph = NULL);
   ID3Inducer(const ID3Inducer& source, CtorDummy);
   virtual ~ID3Inducer();
   virtual Categorizer* best_split(MStringArray*& catNames) const;
   virtual TDDTInducer* create_subinducer(const MString& dscr,
                                          CGraph& aCgraph) const;
   virtual SplitInfo split_info(int attrNum) const;
   virtual Categorizer* splitInfo_to_cat(const SplitInfo& si,
				 MStringArray*& catNames) const;
   int get_level() const {return level;}
   void set_level(int lvl) {level = lvl;}
   virtual Inducer* copy() const;
};
#endif
