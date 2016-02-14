// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : StateSpace implements the graph on which a search is
                   performed.  The graph is not stored explicitly in its
		   entirety; rather, it is constructed as the search
		   proceeds.  This class stored a representation of the
		   current state of the graph.
		 StateSpace is templated on the type of state used in the
		   search.  This is generally derived from the State class,
		   which is itself a template class.
  Assumptions  :
  Comments     :
  Complexity   : Not computed.
  Enhancements : 
  History      : Dan Sommerfield                                    6/15/95
                   Finished updating new framework
                 Brian Frasca                                       4/23/94
                   Initial revision (.h,.c)
***************************************************************************/
#include <basics.h>
#include <StateSpace.h>
#include <mlcIO.h>

#define MIN_EVALUATION -1

//This line causes id to be defined twice.
//RCSID("MLC++, $RCSfile: StateSpace.c,v $ $Revision: 1.7 $")

/***************************************************************************
  Description : Gets the preferences from the DisplayPref class.
  Comments    : This method applies only to DotPostscript display type.
***************************************************************************/
template <class StateType>
void StateSpace<StateType>::process_DotPoscript_preferences(MLCOStream& stream,
					       const DisplayPref& pref) const
{
   // Remember: These are preferences that only make sense for the
   // Postscript Display
   FloatPair pageSize, graphSize;
   pageSize = pref.typecast_to_DotPostscript().get_page_size();
   graphSize = pref.typecast_to_DotPostscript().get_graph_size();
   DotPostscriptPref::DisplayOrientation orientation;
   orientation = pref.typecast_to_DotPostscript().get_orientation();
   DotPostscriptPref::DotRatioType ratio;
   ratio = pref.typecast_to_DotPostscript().get_ratio();
   
   stream << "page = \"" << pageSize.x << ","
	  << pageSize.y << "\";" << endl
	  << "size = \"" << graphSize.x << ","
	  << graphSize.y << "\";" << endl;
   if (orientation == DotPostscriptPref::DisplayLandscape)
      stream << "orientation = landscape;" << endl;
   else
      stream << "orientation = portrait;" << endl;
   if (ratio == DotPostscriptPref::RatioFill)
      stream << "ratio = fill;" << endl;
}


/***************************************************************************
  Description : Protected Functions
  Comments    :
***************************************************************************/
template<class ST>
void StateSpace<ST>::free()
{
   NodePtr nodePtr;
   forall_nodes(nodePtr, *this)
      delete inf(nodePtr);
   clear();
}

/***************************************************************************
  Description : Converts the representation of the graph to dot format
                and directs it to the passed in stream.
  Comments    : Runs in O(e) time.
***************************************************************************/
template<class ST>
void StateSpace<ST>::convertToDotFormat(MLCOStream& stream,
					const DisplayPref& pref) const
{
   // send header and open brace to stream.
   stream << "/* Machine generated dot file */\n" << endl
	  << "digraph G { \n" << endl;
   
   // Preferences that only make sense for the Postscript Display
   if (pref.preference_type() == DisplayPref::DotPostscriptDisplay) {
      process_DotPoscript_preferences(stream, pref);
   }

   // We add each node to the dot output.
   NodePtr v; 
   forall_nodes(v,*this) {
      stream << "/*  node "<< index(v) << ":  */" << endl;
      
      stream << "node_" << index(v) <<" [label=\"";
      get_state(v).display_for_graph(stream);
      stream << "\""
	     << get_state(v).get_graph_options()
	     << "]" << endl;
      // For each edge, we add a line to the dot file. (use Leda call)
      EdgePtr e;
      forall_adj_edges(e,v) {
	 stream << "node_" << index(v) << "->" << "node_"
		<< index(target(e)) << " [label=\""
		<< MString(Mround(100*inf(e),2),0) << "\"] " << endl;
      }
      ASSERT (v != NULL);
   }
   ASSERT (v == NULL);
   stream << '}' << endl;
}


/***************************************************************************
  Description : Pops up a dotty window with graph.
  Comments    : This method applies only to XStream MLCOStream type.
***************************************************************************/
template<class ST>
void StateSpace<ST>::process_XStream_display(const DisplayPref& dp) const
{
   DBG(if (dp.preference_type() != DisplayPref::DotGraphDisplay)
          err << "StateSpace::process_XStream_display:  invalid "
              "combination of display requested, Xstream is only "
              "valid with DotGraphDisplay" << fatal_error);

   MString tmpfile = get_temp_file_name();
   // need to put this is a logical block so that the file is closed
   // make call to dotty.  If this is a common requirement, possibly
   // create some method with concomitant comment on correct usage
   {
      MLCOStream tempfile(tmpfile);
      convertToDotFormat(tempfile, dp);
   }
   
   if (system(get_env_default("DOTTY", "dotty") + " " + tmpfile))
      err << "StateSpace::display: Call to dotty returns an error"
	  << fatal_error;
   
   remove_file(tmpfile);  // delete the temporary file
}


/***************************************************************************
  Description : Employ dot to create a postscript file of the
                receiving graph in a graphical form.
  Comments    : This method applies only to the DotPostscript DisplayPref.
***************************************************************************/
template <class StateType>
void StateSpace<StateType>::process_DotPostscript_display(MLCOStream& stream,
						   const DisplayPref& dp) const
{
   MString tmpfile1 = get_temp_file_name();   
   // need to put this is a logical block so that the file is closed
   {
      MLCOStream tempfile(tmpfile1);      
      convertToDotFormat(tempfile, dp);
   }

   MString tmpfile2 = get_temp_file_name();
   if (system(get_env_default("DOT", "dot") + " -Tps " + tmpfile1 +
	     " -o " + tmpfile2))
      err << "StateSpace::display: Call to dot failed." << fatal_error;
   stream.include_file(tmpfile2);     // this feeds the correct output

   remove_file(tmpfile1); remove_file(tmpfile2);
}


/***************************************************************************
  Description : Employ dot to create a postscript file of the
                receiving graph via a dot description.
  Comments    : This method applies only to the DotGraph DisplayPref.
***************************************************************************/
template<class StateType>
void StateSpace<StateType>::process_DotGraph_display(MLCOStream& stream,
					      const DisplayPref& dp) const
{
   MString tmpfile1 = get_temp_file_name();
   // need to put this is a logical block so that the file is closed
   {
      MLCOStream tempfile(tmpfile1);      
      convertToDotFormat(tempfile, dp);
   }
   stream.include_file(tmpfile1);   // this feeds the correct output
                                    // to the correct stream.
   remove_file(tmpfile1);
}


/***************************************************************************
  Description : Constructor/Destructor.
  Comments    :
***************************************************************************/
template<class StateType>
StateSpace<StateType>::StateSpace()
   : GRAPH<StateType*,Real>(),
     numStatesTotal(0),
     numStatesExpanded(0)
{
}

template<class StateType>
StateSpace<StateType>::~StateSpace()
{
   free();
}

/***************************************************************************
  Description : Graph Manipulation
  Comments    :
***************************************************************************/
template<class StateType>
NodePtr StateSpace<StateType>::create_node(StateType*& state)
{
   NodePtr nodePtr = new_node(state);

   // mark state as in graph
   state->set_node(nodePtr);

   // log information on new state
   LOG(3,"added node " << *state << ": ");
   IFLOG(3, state->display_stats(get_log_stream()));
   LOG(3, endl);

   state = NULL;
   return nodePtr;
}

template<class StateType>
int StateSpace<StateType>::get_next_eval_num()
{
   return numStatesTotal++;
}

template<class StateType>
void StateSpace<StateType>::connect(NodePtr from, NodePtr to, Real edge)
{
   new_edge(from, to, edge);
}

template<class StateType>
NodePtr StateSpace<StateType>::find_state(const StateType& state) const
{
   NodePtr nodePtr;
   forall_nodes(nodePtr, *this)
      if (get_state(nodePtr) == state)
	 return nodePtr;
   return NULL;
}

/***************************************************************************
  Description : Inserts a list of states into the graph.  The states will
                  be connected to the named parent state.  Any states
		  which were previously in the graph will be deleted from
		  the list, and not added again.
  Comments    :
***************************************************************************/
template<class StateType>
void StateSpace<StateType>::insert_states(StateType& parent,
					  DblLinkList<StateType*>* states)
{
   DLLPix<StateType*> statePix(*states, 1);
   Real parentEval = parent.get_fitness();
   ASSERT(parentEval > MIN_EVALUATION);
   NodePtr parentNode = parent.get_node(this);
   ASSERT(parentNode);

   LOG(3, "Adding children of " << parent << endl);

   while(statePix) {
      // add to graph if state is not there already
      StateType *state = (*states)(statePix);
      NodePtr nodePtr = state->get_node(this);
      if(nodePtr) {
	 // delete from list (PIX will be advanced automatically)
	 states->del(statePix);
      }
      else {
	 // add to graph.  Then advance PIX (because we didn't in the for
	 // loop).
	 Real childEval = state->get_fitness();
	 ASSERT(childEval > MIN_EVALUATION);
	 NodePtr childNode = create_node(state);
	 connect(parentNode, childNode, childEval - parentEval);
	 statePix.next();
      } 
   }
}

/***************************************************************************
  Description : These accessor functions should be inlined, but couldn't
                  be because inlining them sets off an internal compiler
		  bug.
  Comments    :
***************************************************************************/
template<class StateType>
StateType& StateSpace<StateType>::get_state(NodePtr node) const
{
   return *inf(node);
}

template<class StateType>
StateType& StateSpace<StateType>::get_initial_state() const
{
   return get_state(initialNode);
}

template<class StateType>
StateType& StateSpace<StateType>::get_final_state() const
{
   return get_state(finalNode);
}

template<class StateType>
int StateSpace<StateType>::get_num_states() const
{
   return numStatesTotal;
}


/***************************************************************************
  Description : Sets the state identified as final in the graph.
  Comments    :
***************************************************************************/
template<class StateType>
void StateSpace<StateType>::set_final_node(NodePtr node)
{
   // Can't use get_state because set_final_state is non-const.
   finalNode = node;
   inf(node)->set_final_state();
}





/***************************************************************************
  Description : Display Functions
  Comments    : Oneliners are not inlined due to internal compiler bug
***************************************************************************/
template<class StateType>
void StateSpace<StateType>::display_initial_state(MLCOStream& stream) const
{
   get_initial_state().display(stream);
}

template<class StateType>
void StateSpace<StateType>::display_final_state(MLCOStream& stream) const
{
   get_final_state().display(stream);
}

template<class StateType>
void StateSpace<StateType>::display(MLCOStream& stream, 
				    const DisplayPref& dp) const
{
   // XStream is a special case--the only option so far where you don't
   // just send something to the MLCOStream.
   if (stream.output_type() == XStream) {
      process_XStream_display(dp);
      return;
   }

   // Other cases are depend only on DisplayPreference
   switch (dp.preference_type()) {

   case DisplayPref::ASCIIDisplay:
      print(stream.get_stream());    // this is Leda's print routine
      stream << flush;
      break;

   case DisplayPref::DotPostscriptDisplay:
      process_DotPostscript_display(stream, dp);
      break;

   case DisplayPref::DotGraphDisplay:
      process_DotGraph_display(stream,dp);
      break;

   default:
      err << "StateSpace<>::display: Unrecognized output type: "
          << stream.output_type() << fatal_error;
   }
}

// Define operator<< for display()
template <class StateType>
DEF_DISPLAY(StateSpace<StateType>)

