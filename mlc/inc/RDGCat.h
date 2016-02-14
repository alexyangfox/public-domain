// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _RDGCategorizer_h
#define _RDGCategorizer_h 1

#include <Categorizer.h>
#include <RootCatGraph.h>

// RDG is rooted decision graph
class RDGCategorizer : public Categorizer {
// DTCategorizer needs access to this member
protected:
   RootedCatGraph* rcGraph;
public:
   virtual void OK(int level = 0)const;
   // Gets ownership of RootedCatGraph
   RDGCategorizer(RootedCatGraph*& rcg, const MString& dscr, int numCat);
   RDGCategorizer(const RDGCategorizer& source, CtorDummy);
   virtual ~RDGCategorizer();
   virtual AugCategory categorize(const InstanceRC&) const;   
   virtual void display_struct(MLCOStream& stream = Mcout,
			    const DisplayPref& dp = defaultDisplayPref) const;
   virtual Categorizer* copy() const;
   virtual const RootedCatGraph& rooted_cat_graph() const;
   // Convenience functions
   virtual int num_nodes() const  {return rooted_cat_graph().num_nodes();}
   virtual int num_leaves() const {return rooted_cat_graph().num_leaves();}

   virtual int class_id() const {return CLASS_RDG_CATEGORIZER; }
   virtual Bool operator==(const Categorizer &cat) const ;
   virtual Bool operator==(const RDGCategorizer &) const ;
   virtual RootedCatGraph* release_rooted_cat_graph();
};

#endif
