// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Main() creates NUMTREES trees, where the root has ROOTBRANCH 
                   children.  Each tree branche has j + 5 nodes.  All of the
		   descendants of the root have degree 1 or 0.
		 So the tree will look like this
		      R
		 / / ... \ \   (root has ROOTBRANCH children)
		 N N      N N
		 | | ...  | |
		 N N ...  N N  
		     ...
		 | | ...  | |
		 N N ...  N N
		 total of ROOTBRANCH * (j + 5) + 1 nodes (R and N are nodes)
  Doesn't test :
  Enhancements :
  History      : Richard Long                                       9/03/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <DecisionTree.h>
#include <ConstCat.h>

RCSID("MLC++, $RCSfile: t_DecisionTree.c,v $ $Revision: 1.16 $")

#ifdef __OBJECTCENTER__
const int NUMTREES = 3;
#else
const int NUMTREES = 20;
#endif

const int ROOTBRANCH = 3;

/***************************************************************************
  Description : This function creates a DecisionTree that is not a tree
                  to check the OK() function.
  Comments    :
***************************************************************************/
void test_non_tree()
{
  const int numNodes = 4;
  NodePtr nodePtrs[numNodes];
  DecisionTree* bad = new DecisionTree;
  for (int i = 0; i < numNodes; i++) { 
    nodePtrs[i] = bad->create_node();
    AugCategory constCatAugCat(UNKNOWN_CATEGORY_VAL, "unknown");
    Categorizer* cat = new ConstCategorizer("node" + MString(i,0), 
					    constCatAugCat);
    bad->set_categorizer(nodePtrs[i], cat);
    ASSERT(cat == NULL);  // set_categorizer() gets ownership
  }
  bad->set_root(nodePtrs[0]);
  AugCategory* aca;
  aca = new AugCategory(UNKNOWN_CATEGORY_VAL, "unknown");
  bad->connect(nodePtrs[0], nodePtrs[1], aca);
  ASSERT(aca == NULL); // connect() gets ownership
  aca = new AugCategory(FIRST_CATEGORY_VAL, "first");
  bad->connect(nodePtrs[0], nodePtrs[2], aca);
  ASSERT(aca == NULL); // connect() gets ownership
  aca = new AugCategory(UNKNOWN_CATEGORY_VAL, "unknown");
  bad->connect(nodePtrs[1], nodePtrs[3], aca);
  ASSERT(aca == NULL); // connect() gets ownership
  aca = new AugCategory(UNKNOWN_CATEGORY_VAL, "unknown");
  bad->connect(nodePtrs[2], nodePtrs[3], aca);
  ASSERT(aca == NULL); // connect() gets ownership
  TEST_ERROR("CatGraph::check_tree: A tree with 4 nodes must have 3 edges",
	     delete bad);
}


main()
{
  cout << "t_DecisionTree executing" << endl;
  
  DecisionTree dt[NUMTREES];
  
  for (int j = 0; j < NUMTREES; j++) {
    int numNodes = j + 5;
    NodePtr root = dt[j].create_node();
    AugCategory rootAugCat(UNKNOWN_CATEGORY_VAL, "unknown");
    Categorizer* cat = new ConstCategorizer("Root ConstCat-" + 
					    MString(j,0), rootAugCat);
    dt[j].set_categorizer(root, cat);
    ASSERT(cat == NULL);
    dt[j].set_root(root);

    Category rootEdgeLabel = UNKNOWN_CATEGORY_VAL;

    for (int k = 0; k < ROOTBRANCH; k++) {
      NodePtr source = dt[j].create_node();
      AugCategory sourceAugCat(UNKNOWN_CATEGORY_VAL, "unknown");
      cat = new ConstCategorizer("ConstCat" + MString(j,0) + "-" 
				 + MString(k,0) + "-0", sourceAugCat);
      dt[j].set_categorizer(source, cat);
      ASSERT(cat == NULL);
      AugCategory* aca;
      aca = new AugCategory(rootEdgeLabel,
				 "edge" + MString(rootEdgeLabel,0));
      dt[j].connect(dt[j].get_root(), source, aca);
      ASSERT(aca == NULL);
      rootEdgeLabel++;
      for (int i = 1; i < numNodes; i++) {
	 AugCategory targetAugCat(UNKNOWN_CATEGORY_VAL, "unknown");
	 cat = new ConstCategorizer("ConstCat" + MString(j,0) + "-" 
				    + MString(k,0) + "-" 
				    + MString(i,0), targetAugCat);
	 const MString& descr = cat->description();
	 NodePtr target = dt[j].create_node(cat);
	 ASSERT(cat == NULL);
	 ASSERT(dt[j].get_categorizer(target).description() == descr);
	 aca = new AugCategory(UNKNOWN_CATEGORY_VAL, "edge");
	 dt[j].connect(source, target, aca);
	 ASSERT(aca == NULL);
	 source = target;
      }
    }
  }

  dt[0].display();
  MLCOStream out1("t_DecisionTree.out1");
  dt[2].display(out1);

#ifndef MEMCHECK
  test_non_tree();
#endif
  
  return 0; // return success to shell
}   
