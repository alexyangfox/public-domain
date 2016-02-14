// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test CompState by setting up a simple search space
                   and making sure that compound operators are properly
		   generated.
		 Since CompState is an ABC, we need to generate a small
		   derived class to use for testing.
  Doesn't test : 
  Enhancements : 
  History      : Dan Sommerfield                                   4/28/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <CompState.h>
#include <math.h>

//RCSID("MLC++, $RCSfile: t_CompState.c,v $ $Revision: 1.5 $")


class TestInfo : public AccEstInfo {
public:
   virtual int lower_bound(int) { return 0; }
   int upper_bound(int) { return 9; }
   void display_values(const Array<int>& values, MLCOStream& out) const {
      out << values;
   }
   Bool use_compound() { return TRUE; }
};

class TestState : public CompState {
public:
   TestState(Array<int>*& initStateInfo, const AccEstInfo& gI)
      : CompState(initStateInfo, gI) { }

   virtual Real eval(AccEstInfo *info, Bool computeReal = TRUE,
		     Bool computeEstimated = TRUE);

   virtual void display_info(MLCOStream& stream = Mcout) const
   {
      stream << "[";
      globalInfo.display_values(get_info(), stream);
      stream << "]";
   }
				   
   virtual void display_stats(MLCOStream& stream = Mcout) const
   { stream << fitness; }
   
   virtual CompState *create_state(Array<int>*& initInfo) {
      return new TestState(initInfo, globalInfo); }
};

// evaluation is n-dimensional distance from center of space
Real TestState::eval(AccEstInfo *info, Bool, Bool)
{
   (void)info;
   Real sum = 0;
   for(int i=0; i<get_info().size(); i++)
      sum += (get_info().index(i)-5) * (get_info().index(i)-5);
   Real result = sqrt(sum);
   Mcout << "Evaluating state ";
   display(Mcout);
   Mcout << "  (number " << get_eval_num() << "): " << result << endl;
   fitness = 1.0/(result+1.0);
   return fitness;
}


// some tests
void basic_test()
{
   TestInfo testInfo;
   Array<int> *vector = new Array<int>(0,5,3);
   TestState *newState = new TestState(vector, testInfo);
   newState->eval(&testInfo);
   
   // create a state space
   StateSpace< State<Array<int>, AccEstInfo> > *space =
      new StateSpace< State<Array<int>, AccEstInfo> >;
   State<Array<int>, AccEstInfo> *state = newState;
   space->create_node(state);
   
   // generate some successors
   DblLinkList<State<Array<int>, AccEstInfo> *> *stateList =
      newState->gen_succ(&testInfo, space, TRUE);

   delete stateList;
}

   
main()
{
   basic_test();
   return 0;
}





