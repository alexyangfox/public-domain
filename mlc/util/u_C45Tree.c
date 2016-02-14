// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a utility for MLC++
// It is not part of the MLC++ library itself, but instead uses the
//   library to provide a useful function.

/***************************************************************************
  Description  : Runs read_c45tree() to build an instance of
                 DecisionTree from a c45 datafile set
  
  Usage        : Environment variables to set:
                   DATAFILE  (file stem i.e. "monk1-full")
                   DISPGRAPH (if yes, displays the graph.  Defaults
                              to no).
		   C45TREE_ROUND_ACC accuracy to round thresholds (5).
  Enhancements :
  
  History      :  Ronny Kohavi                                    06/01/95
                    Actually runs C4.5 and uses temporary output files.
  		    It used to be that you had to have .tree, .unpruned
		      in your directory, which was a nuisance.
                  James Dougherty                                 05/04/94
                    Initial Revision 
***************************************************************************/

#include <basics.h>
#include <MString.h>
#include <C45Inducer.h> // for read_c45_tree()
#include <InstList.h>
#include <MLCStream.h>
#include <FileNames.h>
#include <GetOption.h>

const MString dispGraphHelp = "This option determines whether or not to "
  "display the resulting graphs (pruned and unpruned)";

int main()
{
   FileNames files;
   
   InstanceList trainSet("", files.names_file(), files.data_file());
   InstanceList testSet("", files.names_file(), files.test_file());

   MString c45Flags = get_option_string("C45_FLAGS",
					C45Inducer::defaultPgmFlags);
   C45Inducer c45("C45Tree", c45Flags);
   Real acc = c45.train_and_test(&trainSet, testSet);

   Mcout << "Accuracy of pruned C45 tree is " << acc << endl;

   MLCIStream prunedTreeStream(c45.file_stem() + ".tree");
   DecisionTree *prunedTree = read_c45_tree(prunedTreeStream,
					    trainSet.get_schema());
   MLCIStream unprunedTreeStream(c45.file_stem() + ".unpruned");
   DecisionTree *unprunedTree = read_c45_tree(unprunedTreeStream,
					      trainSet.get_schema());


   DotGraphPref dotPref;
   MLCOStream prunedDotOut(files.root_name() + "-pruned.dot");
   MLCOStream unprunedDotOut(files.root_name() + "-unpruned.dot");

   //It would be nice to be able to put both of these topmost concurrently
   unprunedTree->display(unprunedDotOut,dotPref);
   prunedTree->display(prunedDotOut,dotPref);
			      
   
   if (get_option_bool("DISPGRAPH", FALSE, dispGraphHelp, TRUE)) {
      MLCOStream window1(XStream);
      MLCOStream window2(XStream);
      unprunedTree->display(window1,dotPref);
      prunedTree->display(window2,dotPref);
   }
   
   delete unprunedTree;
   delete prunedTree;

   return 0;
}
