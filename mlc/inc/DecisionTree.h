// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _Tree_h
#define _Tree_h 1

#include <RootCatGraph.h>
#include <mlcIO.h>

class DecisionTree : public RootedCatGraph {
  NO_COPY_CTOR(DecisionTree);
  void convertToTreeVizFormat(MLCOStream& conf, //@@ make virtual
			      MLCOStream& data,
			      const DisplayPref& pref)const;
  void process_XStream_display(const DisplayPref& dp) const;

public:
  virtual void OK(int level = 0) const;
  DecisionTree();
  DecisionTree(CGraph& grph);
  DecisionTree(const DecisionTree& source, CtorDummy);
  virtual ~DecisionTree();
  virtual void display(MLCOStream& stream = Mcout,
		       const DisplayPref& dp = defaultDisplayPref) const;
};

#endif
