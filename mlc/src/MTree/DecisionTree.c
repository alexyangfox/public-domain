// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : DecisonTrees are RootedCatGraphs where each node other than
                   the root has exactly on parent.  The root has no parents.
  Assumptions  :
  Comments     :
  Complexity   :
  Enhancements : Write display() routine similar to C4.5
  History      : Richard Long                                       9/02/93
                   Initial revision (.c)
		 Richard Long                                       8/04/93
                   Initial revision (.h)
***************************************************************************/

#include <basics.h>
#include <DecisionTree.h>
#include <DynamicArray.h>
#include <ConstCat.h>



RCSID("MLC++, $RCSfile: DecisionTree.c,v $ $Revision: 1.13 $")

/***************************************************************************
  Description : Checks that the graph is a tree and calls 
                  RootedCatGraph::OK().
  Comments    : If the DecisionTree does not own the cGraph, then the whole
                  cGraph may not necessarily be a tree, so that check is
		  not performed.
***************************************************************************/
void DecisionTree::OK(int level) const
{
   switch (level) {
   case 0:
      RootedCatGraph::OK();
   case 1:
      if (graphAlloc)
	 check_tree(get_root(), TRUE);
   }
}


/***************************************************************************
  Description : Pops up a dotty window with graph.
  Comments    : This method applies only to XStream MLCOStream type.
                Private.
***************************************************************************/
void DecisionTree::process_XStream_display(const DisplayPref& dp) const
{
   if (dp.preference_type() == DisplayPref::TreeVizDisplay) {
      MString tmpFile = get_temp_file_name();
      MString tmpTVizConf = tmpFile + ".treeviz";
      MString tmpTVizData = tmpFile + ".treeviz.data";
      MLCOStream TVizConf(tmpTVizConf);
      MLCOStream TVizData(tmpTVizData);
      convertToTreeVizFormat(TVizConf, TVizData, dp);
      TVizConf.close();
      TVizData.close();
      if(system(get_env_default("TREEVIZ", "treeviz") + " " + tmpTVizConf))
	 cerr << "CatGraph::display: Call to TreeViz returns an error.\n";
      remove_file(tmpTVizConf);  // delete the temporary files
      remove_file(tmpTVizData);
   } else
      RootedCatGraph::process_XStream_display(dp);
}

/***************************************************************************
  Description : Helper function to display subtree and update label names.
  Comments    :
***************************************************************************/

void write_subtree(MLCOStream& data, MString prefix, const MString& sep,
		   const CatGraph& graph, NodePtr root, 
		   DynamicArray<MString>& labels)
{
   const Categorizer& cat = graph.get_categorizer(root);
   const MString& descr = cat.description();
   if (descr.contains(sep))
      err << "DecisionTree::write_subtree: cannot write description "
	  << descr << " because it contains separator " << sep << fatal_error;
   ASSERT(descr != "");
   if (graph.num_children(root) == 0) { // leaf
      ASSERT(cat.class_id() == CLASS_CONST_CATEGORIZER);
      // Safe cast because we checked the id
      ConstCategorizer& constCat = (ConstCategorizer &)cat;
      AugCategory augCat = constCat.get_category();
      int catNum = augCat.num();
      if (labels[catNum] == "")
	 labels[catNum] = descr;
      else
	 ASSERT(labels[catNum] == descr);

      data << prefix << '\t' << cat.get_distr() << endl;
   } else { // recurse on all children
      if (prefix == "") // no separator
	 prefix = descr;
      else
	 prefix += sep + descr;
      EdgePtr e;
      const CGraph& g = graph.get_graph();
      forall_adj_edges(e,root) {
         // Skip edges with unknown values.
	 if (g.inf(e)->num() != UNKNOWN_CATEGORY_VAL) {
            // We have to downgrade description to MString for += to work.
	    MString descr(*g.inf(e)->description().read_rep());
	    MString edge;
	    if (cat.class_id() == CLASS_ATTR_CATEGORIZER) {
               if (descr[0] == '[' && descr[descr.length()-1] == ']')
   	          edge = prefix + " in " + descr;		  
	       else
		  edge = prefix + "=" + descr;
	    } else if (cat.class_id() == CLASS_THRESHOLD_CATEGORIZER)
	       edge  = prefix + descr;
	    else
	       err << "DecisionTree::write_subtree: unrecognized categorizer"
		   << fatal_error;
	    write_subtree(data, edge, sep, graph, g.target(e), labels);
	 }
      }
   }
}   

/***************************************************************************
  Description : Converts the representation of the graph to TreeViz format
                and directs it to the passed in stream.
  Comments    : This is shared by XStream and file dumps.
***************************************************************************/

void DecisionTree::convertToTreeVizFormat(MLCOStream& conf, MLCOStream& data,
					  const DisplayPref&) const
{
   // We have to construct the labels here since all we have is a graph
   //   with no access to the names file.
   DynamicArray<MString> labels(UNKNOWN_CATEGORY_VAL, 0, "");
   NodePtr root = get_root(TRUE);
   write_subtree(data, "", ":", *this, root, labels);

   for (int i = FIRST_CATEGORY_VAL; i <= labels.high(); i++)
      if (labels[i] == "")
	 cerr << "Warning: class " << i << " not in tree." << endl;

   conf << "# MLC++ generated file for TreeViz.\n"
           "input {\n"
           "\t file \"" << data.description() << "\";\n"
	   "\t key string labels {\n";
   

   for (i = FIRST_CATEGORY_VAL; i <= labels.high(); i++) {
      conf << "\t\t \"" << labels[i] << '"';
      if (i != labels.high())
         conf << ",";
      conf << endl;
   }

   conf << "\t };\n"
	"\t string nodelabel [] separator ':';\n"
	"\t int instCount [labels] separator ',' ;\n"
        "}\n\n";

   conf << "hierarchy {\n"
           "\t levels nodelabel;\n"
           "\t key instCount;\n"
           "\t aggregate {sum instCount;}\n"
           "}\n";

   conf << "view hierarchy landscape {\n"
           "\t height instCount, normalize, max 5.0;\n"
           "\t base height max 2.0;\n"
           "\t color key;\n"
           "\t options columns " << labels.size() - 1 << ";\n"
           "\t options root label \"\";\n"
           "\t message \"%,d\", instCount;\n"
           "}\n";
}



/***************************************************************************
  Description : Constructors.
  Comments    :
***************************************************************************/
DecisionTree::DecisionTree() : RootedCatGraph() {}
DecisionTree::DecisionTree(CGraph& grph) : RootedCatGraph(grph) {}
DecisionTree::DecisionTree(const DecisionTree& source,
			   CtorDummy /*dummyArg*/)
   : RootedCatGraph(source, ctorDummy) {}


/***************************************************************************
  Description : Checks that DecisionTree is valid before destruction.
  Comments    :
***************************************************************************/
DecisionTree::~DecisionTree()
{
  DBG(OK(1)); // don't call with zero because RootCatGraph's destructor's
              // calls OK for base class.

}


/***************************************************************************
  Description : See CatGraph::display.
                We support TreeViz here.
  Comments    : The data file will be our stream name with .data suffix.
***************************************************************************/
void DecisionTree::display(MLCOStream& stream, const DisplayPref& dp) const
{
   // Note that if the display is XStream, our virtual function gets it
   if (stream.output_type() == XStream || 
      dp.preference_type() != DisplayPref::TreeVizDisplay)
      RootedCatGraph::display(stream, dp);
   else {
      MString dataName = stream.description() + ".data";
      MLCOStream data(dataName);
      convertToTreeVizFormat(stream, data, dp);
   }
}
