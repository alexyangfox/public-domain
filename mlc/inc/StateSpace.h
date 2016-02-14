// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _StateSpace_h
#define _StateSpace_h 1

#include <basics.h>
#include <DblLinkList.h>
#include <CGraph.h>
#include <MLCStream.h>
#include <DisplayPref.h>
#include <LogOptions.h>

template<class StateType>
class StateSpace : public GRAPH<StateType*, Real> {
   LOG_OPTIONS;
   
   NodePtr initialNode;
   NodePtr finalNode;

   int numStatesTotal;
   int numStatesExpanded;

   void process_DotPoscript_preferences(MLCOStream& stream,
					const DisplayPref& pref) const;
protected:
   void free();

   virtual void process_XStream_display(const DisplayPref& dp) const;
   virtual void process_DotPostscript_display(MLCOStream& stream,
					      const DisplayPref& dp) const;
   virtual void process_DotGraph_display(MLCOStream& stream,
					 const DisplayPref& dp) const;
   virtual void convertToDotFormat(MLCOStream& stream,
				   const DisplayPref& pref) const;
public:
   StateSpace();
   virtual ~StateSpace();

   // Graph Manipulation
   NodePtr create_node(StateType*& state);
   int get_next_eval_num();
   void connect(NodePtr from, NodePtr to, Real edge);
   NodePtr find_state(const StateType& state) const;

   // Multiple state evaluation/insertion
   void insert_states(StateType& parent, DblLinkList<StateType*>* states);

   // Accessor Functions.
   StateType& get_state(NodePtr node) const;
   StateType& get_initial_state() const;
   StateType& get_final_state() const;
   int get_num_states() const;
   
   void set_initial_node(NodePtr node) { initialNode = node; }
   void set_final_node(NodePtr node);

   // Display Functions
   void display_initial_state(MLCOStream& stream = Mcout) const;
   void display_final_state(MLCOStream& stream = Mcout) const;
   virtual void display(MLCOStream& stream = Mcout,
                        const DisplayPref& dp = defaultDisplayPref) const;
};

// // LEDA functions
// void Print(const StateType*& /* nodeInfo */, ostream& /* stream */)
//   { err << "StateSpace::Print: no support for print" << fatal_error;}
// void Read (const StateType*& /* nodeInfo */, istream& /* stream */)
//   { err << "StateSpace::Read: no support for read" << fatal_error;}

template <class StateType>
DECLARE_DISPLAY(StateSpace<StateType>);

#endif
