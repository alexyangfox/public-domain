
// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : 
  Usage        : 
  Enhancements :
  History      : Chia-Hsin Li                                     10/28/94
                   Initial version 
***************************************************************************/

#include <basics.h>
#include <ODGInducer.h>
#include <EntropyODGInducer.h>
#include <CtrInstList.h>
#include <CatTestResult.h>
#include <GetOption.h>
#include <isocat.h> 

RCSID("MLC++, $RCSfile: t_isocat.c,v $ $Revision: 1.5 $")

void check_connection(const CGraph& cg)
{
   int noInDegreeNum = 0;
   Mcout << "Testing the connectivity..." << endl;
   NodePtr iterNode;
   forall_nodes(iterNode, cg) {
      if (cg.indeg(iterNode) == 0) {
	 noInDegreeNum ++;
	 if (noInDegreeNum > 1)
	    err << "Some part of the graph is unconnnected. " << fatal_error;
      }
   }
}

/***************************************************************************
  Description : Get the node whose index is nodeNum. 
  Comments    : This function is inefficient. However, it's in the tester.
                   No body cares.
***************************************************************************/
NodePtr get_node(const CGraph& G, int nodeNum)
{
   NodePtr iterNode;
   forall_nodes(iterNode, G) {
      if (index(iterNode) == nodeNum)
	 return iterNode;
   }
   err << "Can not find the node whose index is " << nodeNum << fatal_error;
   return NULL;
}

void check_first_level_subgraphs(const CatGraph& catG)
{
   const CGraph& G = catG.get_graph(); 
   NodePtr redPtr = get_node(G, 1);
   NodePtr yellowPtr = get_node(G, 2);
   NodePtr greenPtr = get_node(G, 3);
   NodePtr bluePtr = get_node(G, 4);
   LogOptions logOp;

   logOp.set_log_level(0);
   Bool changeLabel = FALSE;

   // Test red and yellow subgraph
   Mcout << "Testing red and yellow jacket subgraphs"
	 << endl;
   int conflict = acyclic_graph_merge_conflict(logOp, catG, redPtr,
					       catG, yellowPtr, changeLabel);
   Mcout << "Conflict Number:" << conflict << endl;
   // Test red and green subgraph
   Mcout << "Testing red and green jacket subgraphs"
	 << endl;
   conflict = acyclic_graph_merge_conflict(logOp, catG, redPtr,
					       catG, greenPtr, changeLabel);
   Mcout << "Conflict Number:" << conflict << endl;

   // Test red and blue subgraph
   Mcout << "Testing red and blue jacket subgraphs"
	 << endl;
   conflict = acyclic_graph_merge_conflict(logOp, catG, redPtr,
					       catG, bluePtr, changeLabel);
   Mcout << "Conflict Number:" << conflict << endl;

   // Test yellow and green jacket subgraph
   Mcout << "Testing yellow and green jacket subgraphs"
	 << endl;
   conflict = acyclic_graph_merge_conflict(logOp, catG, yellowPtr,
					       catG, greenPtr, changeLabel);
   Mcout << "Conflict Number:" << conflict << endl;

   // Test yellow and blue jacket subgraph
   Mcout << "Testing yellow and blue jacket subgraphs"
	 << endl;
   conflict = acyclic_graph_merge_conflict(logOp, catG, yellowPtr,
					       catG, bluePtr, changeLabel);
   Mcout << "Conflict Number:" << conflict << endl;

   // Test green and blue jacket subgraph
   Mcout << "Testing green and blue jacket subgraphs"
	 << endl;
   conflict = acyclic_graph_merge_conflict(logOp, catG, greenPtr,
					       catG, bluePtr, changeLabel);
   Mcout << "Conflict Number:" << conflict << endl;
}

main()
{
   MString datafile = "monk1-full";
   EntropyODGInducer inducer("ODG Inducer");
   inducer.read_data(datafile);
   Mcout << "Training ... " << endl;
   inducer.set_post_proc(set_unknown);
   inducer.set_debug(TRUE);
   inducer.set_cv_prune(FALSE);
   inducer.set_unknown_edges(TRUE);
   inducer.train();
   const CatGraph& catG = inducer.get_cat_graph();
   const CGraph& cg = inducer.get_graph();
 
   NodePtr root = inducer.get_root();
   
   Mcout << "Graph has " << inducer.num_nodes() << " nodes, and "
          << inducer.num_leaves() << " leaves." << endl;

/*
  DotGraphPref pref;
   MLCOStream out(XStream);
   inducer.display_struct(out,pref);
*/
   check_first_level_subgraphs(catG);
   check_connection(cg);
   return 0; // return success to shell
}   

