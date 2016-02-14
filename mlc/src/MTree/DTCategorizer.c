// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : DTCategorizer performs the same way as a RDGCategorizer, 
                   but for the DTCategorizer the associated graph must be
		   a tree.  See "RDGCat.c" for details.
  Assumptions  : The graph exists.
  Comments     : 
  Complexity   : OK() takes complexity of CatGraph::check_tree() + 
                   RootCatGraph::OK().
  Enhancements : 
  History      : Richard Long                                       9/05/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <DTCategorizer.h>

RCSID("MLC++, $RCSfile: DTCategorizer.c,v $ $Revision: 1.17 $")

/***************************************************************************
  Description : Checks that the graph is a tree.
  Comments    : 
***************************************************************************/
void DTCategorizer::OK(int /*level*/)const
{
   RDGCategorizer::OK();
   if (rcGraph != NULL)
      rcGraph->check_tree(rcGraph->get_root());
}


/***************************************************************************
  Description : This function is only called by the constructor, so
                  the static object it returns will exist until the
		  constructor exits.  It will not be called recursively.
  Comments    : This is to avoid passing a temporary to the RDGCat
                  constructor because of the automatic type conversion
		  between RootedCatGraph*& and DecisionTree*&.
***************************************************************************/
static RootedCatGraph*& constructor_cast(DecisionTree*& dt) 
{
   static RootedCatGraph* rcg;

   rcg = dt; // cast
   dt = NULL;
   return rcg;
}


/***************************************************************************
  Description : Passes arguments to RDGCategorizer constructor.
  Comments    : Gets ownership of the DecisionTree.
***************************************************************************/
DTCategorizer::DTCategorizer(DecisionTree*& dt, const MString& dscr,
			     int numCat)
                             : RDGCategorizer(constructor_cast(dt),
					      dscr, numCat) 
{}


/***************************************************************************
    Description : Copy constructor which takes an extra argument.
    Comments    : We do not use the standard issue copy constructor because
                     we want to know exactly when we call the copy
		     constructor.  
***************************************************************************/
DTCategorizer::DTCategorizer(const DTCategorizer& source,
			     CtorDummy /*dummyArg*/)
   : RDGCategorizer(source, ctorDummy)
{}


/***************************************************************************
  Description : Checks that the graph is a tree before destruction.
  Comments    : 
***************************************************************************/
DTCategorizer::~DTCategorizer()
{
  DBG(OK());
}


/***************************************************************************
  Description : Returns a pointer to a deep copy of this DTCategorizer.
  Comments    :
***************************************************************************/
Categorizer* DTCategorizer::copy() const
{
   return new DTCategorizer(*this, ctorDummy);
}
