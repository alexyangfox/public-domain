// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests RootedCatGraph methods.
		 For each RootCatGraph j in an array, main() constructs a graph
		    with j + 10 nodes.  Each node i (numbered from 0) is
		    connected to nodes i+1 and i+2, with edge labels
		    UNKNOWN_CATEGORY and FIRST_CATEGORY_VAL respectively. 
		    The root is node 0.
  Doesn't test : 
  Enhancements : Test using categorizers other than ConstCategorizer.
  History      : Richard Long                                       8/27/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <RootCatGraph.h>
#include <ConstCat.h>

RCSID("MLC++, $RCSfile: t_RootCatGraph.c,v $ $Revision: 1.17 $")

#ifdef __OBJECTCENTER__
const int NUMGRAPHS = 3;
#else
const int NUMGRAPHS = 20;
#endif


/***************************************************************************
  Description : Tests fatal_errors not tested in t_CatGraph.c
  Comments    :
***************************************************************************/
void check_errors()
{
  RootedCatGraph *rcGraph = new RootedCatGraph;
  TEST_ERROR("RootedCatGraph::get_root: Root has not been set",
	     rcGraph->get_root(TRUE));
  AugCategory firstAugCat(FIRST_CATEGORY_VAL, "first");
  Categorizer* cat = new ConstCategorizer("Root Const Categorizer",
					  firstAugCat);
  NodePtr root = rcGraph->create_node(cat);
  ASSERT(cat == NULL);
  rcGraph->set_root(root);

  cat = new ConstCategorizer("Trouble maker", firstAugCat);
  NodePtr nodePtr = rcGraph->create_node(cat);
  ASSERT(cat == NULL);
  TEST_ERROR("CatGraph::check_reachable: Not all nodes were reachable from "
	     "Root Const Categorizer", delete rcGraph);
  AugCategory* aca = new AugCategory(UNKNOWN_CATEGORY_VAL,
					       "unknown");
  rcGraph->connect(nodePtr, root, aca);
  ASSERT(aca == NULL); // connect() gets ownership
  aca = new AugCategory(UNKNOWN_CATEGORY_VAL, "unknown");
  rcGraph->connect(root, nodePtr, aca);
  ASSERT(aca == NULL);
  TEST_ERROR("RootedCatGraph::OK: Root node cannot have parents, but it has 1",
	     delete rcGraph);
}


main()
{
  cout << "t_RootCatGraph executing" << endl;
  
  RootedCatGraph rcGraph[NUMGRAPHS];

  for (int j = 0; j < NUMGRAPHS; j++) {
    int numNodes = j + 10;
    NodePtr source = rcGraph[j].create_node();
    AugCategory firstAugCat(FIRST_CATEGORY_VAL, "first");
    Categorizer* cat = new ConstCategorizer("Root ConstCat-" + 
					    MString(j,0), firstAugCat);
    rcGraph[j].set_categorizer(source, cat);
    ASSERT(cat == NULL);
    rcGraph[j].set_root(source);
    ASSERT(rcGraph[j].get_root() == source);
    for (int i = 1; i < numNodes; i++) {
      cat = new ConstCategorizer("ConstCat" + MString(j,0) + "-"
				 + MString(i,0), firstAugCat);
      NodePtr target = rcGraph[j].create_node();
      MString descr = cat->description();
      rcGraph[j].set_categorizer(target, cat);
      ASSERT(cat == NULL);
      ASSERT(rcGraph[j].get_categorizer(target).description() == descr);
      AugCategory* aca = new AugCategory(UNKNOWN_CATEGORY_VAL,
						   "unknown");
      rcGraph[j].connect(source, target, aca);
      ASSERT(aca == NULL); // connect() gets ownership
      source = target;
    }
    
    source = rcGraph[j].get_root();
    for (i = 0; i < numNodes - 2; i++) {
       AugCategory unknownAugCat(UNKNOWN_CATEGORY_VAL, "unknown");
       NodePtr child = rcGraph[j].get_child(source, unknownAugCat);
       AugCategory* aca = new AugCategory(FIRST_CATEGORY_VAL,
						    "first");
       rcGraph[j].connect(source, rcGraph[j].get_child(child, unknownAugCat),
			  aca);
       ASSERT(aca == NULL); // connect() gets ownership
       source = child;
    }
    ASSERT(rcGraph[j].num_leaves() == 1);
 }

  rcGraph[0].display();
  rcGraph[1].display();
  MLCOStream out1("t_RootCatGraph.out1");
  rcGraph[2].display(out1);

#ifndef MEMCHECK
  check_errors();
#endif

  return 0; // return success to shell
}
