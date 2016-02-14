// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : Reads in a C4.5 tree and builds an instance of DecisionTree.
  Comments     : The C4.5 tree is stored in an "inorder" scan of the
                 tree, each node is stored, and then each child is
		 recursively stored. C4.5 contains more information in
		 the  trees than MLC++ does so some information is
		 ignored. NOTE: In order for this routine to work
		 properly, the MLCIStream actual parameter MUST have
		 previously read in 1 (ONE) character, i.e. if inFile
		 is an instance of MLCIStream, then a call to
		 infile.Get() must be performed prior to this call.
		 This is the way that the C4.5 format is stored.

  Comments     : Does just attribute categorizers and constant cat.
                 (aborts on everything else)

  Enhancements : Add support for thresholds and multi-valued splits.
  History      : Ronny Kohavi                                          06/01/95
                   Added support for thresholds.
                 James Dougherty                                       04/03/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <C45Inducer.h>
#include <AttrCat.h>
#include <SchemaRC.h>
#include <ConstCat.h>
#include <LogOptions.h>
#include <InstList.h>
#include <MLCStream.h>
#include <DecisionTree.h>
#include <ThresholdCat.h>

//These are definitions used in C4.5. See <u/mlc/>/R5/Src/defns.i
const int LEAF     =  0;
const int BRANCH   =  1;
const int CUT      =  2;
const int SUBSET   =  3;


DecisionTree* read_c45_tree(MLCIStream& file,
			    const SchemaRC& schema)
{
   // It seems that sometimes the first character isn't ready
   // after C4.5 runs, probably because of flushing problems.
   // here we wait a bit... total kludge
   for (int i = 0; i < 10000 && file.peek() == EOF; i++)
      ; // wait

   if (i > 0)
      Mcerr << "C45Tree: Warning: had to wait for C4.5 input" << endl;
  
   file.get();  // C4.5 ignores the first character
   return read_c45_tree(file, schema, NULL);
}


DecisionTree* read_c45_tree(MLCIStream& file,
			    const SchemaRC& schema,
			    DecisionTree* dt)
{
   int numClasses = schema.num_label_values();
   
   if (dt == NULL) {
      dt = new DecisionTree();
      GLOBLOG(6, "Num classes = " << numClasses << endl);
   }

   short nodeType,mostFreqCat;
   file.read_bin(nodeType);         //0=Leaf,1=Branch,2=Cut,3=Subset.
   file.read_bin(mostFreqCat);      //Most frequent class at this node

   float numItems,errors;
   file.read_bin(numItems);         //No of Items at this node
   file.read_bin(errors);           //No of errors at this node.
   Array<float> classDistribution(numClasses);  //Class distribution
   classDistribution.read_bin(file);
   
   NodePtr rootNode;
   if(nodeType != LEAF) {
      short testedAttr,numForks;
      file.read_bin(testedAttr);
      file.read_bin(numForks);
      ASSERT(numForks >0);
      switch(nodeType)
      {
	 //Node type is a branch
	 case BRANCH:
	    {
	       Categorizer *cat = new AttrCategorizer(schema, testedAttr,
					   schema.attr_name(testedAttr));
	       rootNode = dt->create_node(cat); 
	       for(int v = 0; v < numForks; v++) {
		  DecisionTree *dtChild = read_c45_tree(file, schema, dt);
		  ASSERT(dtChild == dt);
		  AugCategory* augCat = new
		     AugCategory(FIRST_CATEGORY_VAL + v,
				 schema.nominal_to_string(testedAttr,
						  FIRST_CATEGORY_VAL + v));
		  dt->connect(rootNode, dt->get_root(),augCat);
	       }
	    }
	    break;

	 case CUT:
	    {
	       ASSERT(numForks == 2);
	       float cut, lower, upper;
	       file.read_bin(cut);
	       file.read_bin(lower);
	       file.read_bin(upper);
	       Categorizer* cat = new ThresholdCategorizer(schema, testedAttr,
					   cut, schema.attr_name(testedAttr));
               // On CRX, we didn't get nice numbers, so this is
	       //   ensuring we rounds things before printing.
	       int roundAcc = get_env_default("C45TREE_ROUND_ACC",
					      "5").short_value();
	       Real thresh = Mround(cut, roundAcc);
	       
               // read left subtree
	       rootNode = dt->create_node(cat); 
	       DecisionTree *dtChild = read_c45_tree(file, schema, dt);
	       ASSERT(dtChild == dt);
	       AugCategory* augCat = new
		     AugCategory(FIRST_CATEGORY_VAL, "<= " +
				 MString(thresh, 0));
   	       dt->connect(rootNode, dt->get_root(),augCat);

               // read right subtree
	       dtChild = read_c45_tree(file, schema, dt);
	       ASSERT(dtChild == dt);
	       augCat = new AugCategory(FIRST_CATEGORY_VAL + 1, "> " +
					MString(thresh, 0));
   	       dt->connect(rootNode, dt->get_root(),augCat);
	       break;
	    }
	    
         case SUBSET:
	    err << "C4.5 Subset splits not supported" << fatal_error;
	    break;
	    
	 default:  
	    err << "read_c45_tree: Undefined nodeType "
		<< nodeType <<fatal_error;
      }
   }
   else{   // Create leaf node with a constant categorizer   
      MString labelStr(schema.category_to_label_string(FIRST_CATEGORY_VAL
						       + mostFreqCat));
     AugCategory leafCat(FIRST_CATEGORY_VAL + mostFreqCat, labelStr);
      Categorizer *constCat = new ConstCategorizer(labelStr, leafCat);
      rootNode = dt->create_node(constCat); 
   }
   dt->set_root(rootNode);
   return dt;
}






