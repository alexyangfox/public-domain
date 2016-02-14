// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _RootedCatGraph_h
#define _RootedCatGraph_h 1

#include <CatGraph.h>

class RootedCatGraph : public CatGraph {
  NodePtr root;
public:
  virtual void OK(int level = 0) const;
  RootedCatGraph();
  RootedCatGraph(CGraph&);
  RootedCatGraph(const RootedCatGraph& source, CtorDummy);
  virtual ~RootedCatGraph();
  NO_COPY_CTOR2(RootedCatGraph, CatGraph());
  void set_root(const NodePtr);
  NodePtr get_root(Bool abortOnNoRoot = FALSE) const;
  virtual void display(MLCOStream& stream = Mcout,
		       const DisplayPref& dp = defaultDisplayPref) const;
};

#endif
