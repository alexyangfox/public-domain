// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests CatGraph.c procedures for NUMGRAPHS CatGraphs.
                 Output from the display() routine is sent to 
		   "t_CatGraph.out1".
		 For each CatGraph j in an array, main() constructs a graph
		    with j + 10 nodes.  Each node i (numbered from 0) is
		    connected to nodes i+1 and i+2, with edge labels
		    UNKNOWN_CATEGORY_VAL and FIRST_CATEGORY_VAL respectively.
  Doesn't test : 
  Enhancements : Test using Categorizers other than ConstCategorizer.
  History      : Richard Long                                       8/20/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <CatGraph.h>
#include <ConstCat.h>

RCSID("MLC++, $RCSfile: t_CatGraph.c,v $ $Revision: 1.29 $")



#ifdef __OBJECTCENTER__
#define NUMGRAPHS 5
#else
#define NUMGRAPHS 20
#endif

/***************************************************************************
  Description : Checks most of the fatal_errors that can arise in CatGraph.c
  Comments    : Does not check the errors in check_edges().  They should
                  never occur if connect() works properly.
***************************************************************************/
void check_errors()
{
  CatGraph* badCatGraph = new CatGraph;
  badCatGraph->create_node();

  CatGraph* cycleCatGraph = new CatGraph;
  AugCategory cycleAugCat(UNKNOWN_CATEGORY_VAL, "unknown");
  Categorizer* cat = new ConstCategorizer("cycle", cycleAugCat);
  NodePtr nodePtr1 = cycleCatGraph->create_node(cat);
  ASSERT(cat == NULL);
  cat = new ConstCategorizer("cycle", cycleAugCat);
  NodePtr nodePtr2 = cycleCatGraph->create_node(cat);
  ASSERT(cat == NULL);
  cat = new ConstCategorizer("cycle", cycleAugCat);
  NodePtr nodePtr3 = cycleCatGraph->create_node(cat);
  ASSERT(cat == NULL);
  AugCategory* aca;
  aca = new AugCategory(UNKNOWN_CATEGORY_VAL, "Unknown");
  cycleCatGraph->connect(nodePtr1, nodePtr2, aca);
  ASSERT(aca == NULL);
  aca = new AugCategory(UNKNOWN_CATEGORY_VAL, "Unknown");
  cycleCatGraph->connect(nodePtr2, nodePtr3, aca);
  ASSERT(aca == NULL);
  aca = new AugCategory(UNKNOWN_CATEGORY_VAL, "Unknown");
  cycleCatGraph->connect(nodePtr3, nodePtr1, aca);
  ASSERT(aca == NULL);
  TEST_ERROR("CatGraph::OK: The graph contains a cycle", 
	     delete cycleCatGraph);

  badCatGraph = new CatGraph;
  nodePtr1 = badCatGraph->create_node();
  aca = new AugCategory(FIRST_CATEGORY_VAL, "first");
  cat = new ConstCategorizer("First Categorizer", *aca);
  badCatGraph->set_categorizer(nodePtr1, cat);
  ASSERT(cat == NULL);
  cat = new ConstCategorizer("First Categorizer", *aca);  

  nodePtr2 = badCatGraph->create_node();
  aca = new AugCategory(FIRST_CATEGORY_VAL + 1, "too high");
  TEST_ERROR("CatGraph::connect: The first edge must have label ",
	     badCatGraph->connect(nodePtr1, nodePtr2, aca));
  aca = new AugCategory(UNKNOWN_CATEGORY_VAL, "Unknown");
  badCatGraph->connect(nodePtr1, nodePtr2, aca);
  ASSERT(aca == NULL);
  aca = new AugCategory(FIRST_CATEGORY_VAL + 1, "too high");
  TEST_ERROR(" must follow edge label ",
	     badCatGraph->connect(nodePtr1, nodePtr2, aca));
  aca = new AugCategory(FIRST_CATEGORY_VAL, "first");
  badCatGraph->connect(nodePtr2, nodePtr1, aca);
  ASSERT(aca == NULL);
  aca = new AugCategory(UNKNOWN_CATEGORY_VAL, "Unknown");
  TEST_ERROR(" must follow edge label ",
	     badCatGraph->connect(nodePtr2, nodePtr1, aca));
  TEST_ERROR("CatGraph::get_child: Node does not have an edge labelled",
	     badCatGraph->get_child(nodePtr2, *aca));
  TEST_ERROR("CatGraph::check_node_in_graph: The given node ",
	     badCatGraph->check_node_in_graph(nodePtr3, TRUE));
  TEST_ERROR("CatGraph::check_node_in_graph: The given node 0x0 is not"
	     " in this graph", badCatGraph->check_node_in_graph(NULL, TRUE));
}

/***************************************************************************
  Description : We will test the following graph:
                        n1
		       / \
		      n2  n3
  Comments    :
***************************************************************************/
void check_cgraph()
{
   CGraph cg;
   AugCategory unknownAugCat(UNKNOWN_CATEGORY_VAL, "Unknown 1");
   // Node 1
   Categorizer* cat = new ConstCategorizer("ConstCat1", unknownAugCat);
   NodeInfo* nodeInfo = new NodeInfo(cat, defaultLevel);
   ASSERT(cat == NULL);
   NodePtr n1 = cg.new_node(nodeInfo);
   // Node 2
   cat = new ConstCategorizer("ConstCat2", unknownAugCat);
   nodeInfo = new NodeInfo(cat, defaultLevel);
   ASSERT(cat == NULL);   
   NodePtr n2 = cg.new_node(nodeInfo);
   // Node 3
   cat = new ConstCategorizer("ConstCat3", unknownAugCat);
   nodeInfo = new NodeInfo(cat, defaultLevel);
   ASSERT(cat == NULL);   
   NodePtr n3 = cg.new_node(nodeInfo);
   // Edge 1
   AugCategory* aca;
   aca = new AugCategory(UNKNOWN_CATEGORY_VAL, "Edge 1");
   EdgePtr e1 = cg.new_edge(n1, n2, aca);
   // Edge 2
   aca = new AugCategory(UNKNOWN_CATEGORY_VAL, "Edge 2");
   EdgePtr e2 = cg.new_edge(n1, n3, aca);

   cg.OK();

   cg.del_edge(e1);
   cg.OK();

   cg.del_node(n2);
   cg.OK();

   cg.del_edge(e2);
   cg.OK();

   cg.del_node(n3);
   cg.OK();

   cg.del_node(n1);
   cg.OK();
}


main()
{
  cout << "Executing t_CatGraph" << endl;

  check_cgraph();
  
  CatGraph catGraph[NUMGRAPHS];

  for (int j = 0; j < NUMGRAPHS; j++) {
    const int numNodes = j + 10;
    AugCategory unknownAugCat(UNKNOWN_CATEGORY_VAL, "Unknown");
    Categorizer* cat = new ConstCategorizer("ConstCat0", unknownAugCat);
    NodePtr firstNodePtr = catGraph[j].create_node(cat);
    ASSERT(cat == NULL); // create_node() gets ownership
    ASSERT(catGraph[j].check_tree(firstNodePtr));
    ASSERT(catGraph[j].level(firstNodePtr) == defaultLevel);
    NodePtr source = firstNodePtr;
    for (int i = 1; i < numNodes; i++) {
      cat = new ConstCategorizer("ConstCat"+MString(i,0), unknownAugCat);
      NodePtr target = catGraph[j].create_node();
      MString descr = cat->description();
      catGraph[j].set_categorizer(target, cat);
      ASSERT(cat == NULL); // set_categorizer() gets ownership
      ASSERT(catGraph[j].get_categorizer(target).description() == descr);
      AugCategory* aca;
      aca = new AugCategory(UNKNOWN_CATEGORY_VAL, "Unknown");
      catGraph[j].connect(source, target, aca);
      ASSERT(aca == NULL);
      source = target;
    }
    ASSERT(catGraph[j].num_nodes() == numNodes);
    ASSERT(catGraph[j].num_leaves() == 1);
    source = firstNodePtr;
    for (i = 0; i < numNodes - 2; i++) {
      NodePtr child = catGraph[j].get_child(source, unknownAugCat);
      AugCategory* aca;
      aca = new AugCategory(FIRST_CATEGORY_VAL, "first");
      catGraph[j].connect(source, catGraph[j].get_child(child, unknownAugCat),
			  aca);
      ASSERT(aca == NULL);
      ASSERT(catGraph[j].num_children(source) == 2);
      source = child;
    }
    ASSERT(catGraph[j].check_reachable(firstNodePtr));
    ASSERT(!catGraph[j].check_reachable(source));
    ASSERT(!catGraph[j].check_tree(firstNodePtr));
    catGraph[j].get_graph().OK();
  }

  catGraph[1].display();
  cout << endl << "default finished" << endl;

  CatGraph* CGcp = new CatGraph(catGraph[1], ctorDummy);
  CGcp->OK();
  cout << "copy constructor ok" << endl;
  delete CGcp;
  cout << "copy constructor deleted" << endl;
  
  DotPostscriptPref dpp;
  DotGraphPref dgp;
  ASCIIPref ap;
{
  MLCOStream out0("t_CatGraph.out0");
  catGraph[1].display(out0, ap);
  catGraph[1].display(out0, dgp);
  catGraph[1].display(out0, dpp);
}

system("grep -v '%%For:' t_CatGraph.out0 > t_CatGraph.out1");  // new

// This invokes dotty.  Bad idea for an automatically run test,
// but a good idea to try every once in a while.
//  MLCOStream out2(XStream);
//  catGraph[1].display(out2,dgp);

  
#ifndef MEMCHECK
  check_errors();
#endif

  return 0; // return success to shell
}   




