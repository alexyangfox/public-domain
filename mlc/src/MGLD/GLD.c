// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : This class creates a general logic diagram.  For more
                   on general logic diagrams and their purpose, see
		   Michalski[78] and Wnek, et. al[90].  For more
		   information on the overall GLD design, see
		   /u/mlc/doc/GLD-design.ps.  For the current status
		   of the GLD code, see /u/mlc/doc/-status.ps.  
		 The constructor currently simply takes a
		   CatTestResult and a GLDPref.  The GLDPref has a
		   number of options that control the what is to be
		   displayed. A default GLDPref can be used (one that
		   is simply constructed with the default
		   constructor), but get the desired various options
		   must be set.  For example, the set which is
		   supposed to be displayed, and the display type are
		   fundamental as to what type of output is obtained.
		 This is the highest level GLD class.  It makes direct
		   use of the GLD support classes of the DiagramManager
		   and Shape classes.  
  Assumptions  : The GLDPref that the GLD is initialized with is used
                   initially.  In other words, it is wise to set the
		   options that are wanted before passing the GLDPref
		   in to the constructor, not after.
		 When the PredictedTrain option is chosen, it is
		   assumed that the training list is actually a test
		   list with valid categories that were not known to
		   the inducer.  See the constructor for the
		   motivation for this.
  Comments     : 
  Complexity   : The time complexity of the GLD constructor is
                   O(number of rows*number of columns) 
  Enhancements : Need to add support for callbacks, such as a callback
                   registering a user request to switch from the
		   training list to the test list.
		 Need to add a mechanism so that the shapes are chosen
		   intelligently to represent categories.
  History      : Dave Manley                                      12/19/93
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <CatTestResult.h>
#include <BagSet.h>
#include <GLD.h>
#include <LabInstGen.h>
#include <BagSet.h>
#include <Instance.h>

RCSID("MLC++, $RCSfile: GLD.c,v $ $Revision: 1.5 $")


/***************************************************************************
  Description : Returns TRUE if all instances in the bag
                  have the same label as the value passed in.
  Comments    : Time complexity is O(number of instances in lib).
***************************************************************************/
static Bool all_instances_match(const InstanceBag &bag,
				const InstanceRC& instance,
				int value)
{
   for (Pix bagPix = bag.first(); bagPix; bag.next(bagPix)) {
      const InstanceRC& inst(bag.get_instance(bagPix));
      if (instance.equal_no_label(inst))
 	 if (bag.label_info().get_nominal_val(inst.get_label())!= value) 
	    return FALSE;
   }
   return TRUE;
}


/***************************************************************************
  Description : Returns TRUE if all instances in the labelled instance
                  bag have the same label as the value passed in.
  Comments    : Time complexity is O(number of instances in bag).
***************************************************************************/
static Bool at_least_one_instance_matches(const InstanceBag &bag,
					   const InstanceRC& instance,
					   int value)
{
   for (Pix bagPix = bag.first(); bagPix; bag.next(bagPix)) {
      const InstanceRC& inst(bag.get_instance(bagPix));
      if (instance.equal_no_label(inst))
	 if (bag.label_info().get_nominal_val(inst.get_label()) == value) 
	    return TRUE;
   }
   return FALSE;
}


/***************************************************************************
    Description :  Constructor to pop up a GLD currently takes a
                     CatTestResult and a GLDPref which
		     contains information as to exactly what the 
		     user wishes to be displayed.
    Comments    :  Note that the DiagramManager is responsible for
                     creating the DisplayManager.
		   Note that in the case of PredictedTrain, we need to
		     pass the "real" test list as the training list.
		     (By "real" I mean the test list that has categories
		     upon which predictions can be evaluated.)  This is
		     so that when using the CatTestResult to construct a
		     GLD, this information is available to decide which
		     instances of the "real" test list were predicted
		     correctly. Since we are using a full space
		     generated test list as the test list argument in
		     constructing the CatTestResult, the only place for
		     the "real" test list, then, is the training list. 
***************************************************************************/
GLD::GLD(const CatTestResult& catTestResult, GLDPref *gldPref)
{
   gldPreference = gldPref;
   const Array<CatOneTestResult>& results = catTestResult.get_results();
   const InstanceBag& trainingBag =  catTestResult.get_training_bag();
   const InstanceBag& testBag = catTestResult.get_testing_bag();
      
   diagramManager =
      new DiagramManager(trainingBag.get_schema(), gldPreference);
   displayManager = diagramManager->get_display_manager();

   SetSpecification setSpec = gldPref->get_set_specification();

   // Find the marking to use by looping through each instance in the
   // training set.  This is currently only useful to the
   // PredictedTrain option. The PredictedTest option also uses the
   // training set, but it accesses it in the previous loop.
   if (setSpec == PredictedTrain)
      for (Pix bagPix = trainingBag.first(); bagPix;
	   trainingBag.next(bagPix)) {
	 const InstanceRC& instance(trainingBag.get_instance(bagPix));
	 int markingNum = trainingBag.label_info().get_nominal_val(
	    instance.get_label()) - FIRST_CATEGORY_VAL;
	 diagramManager->add_instance_marking(instance,
				      displayManager->get_shape(markingNum)); 
      }

   // For predicted test, we first put shapes in all of the spaces
   // which were correct.
   if (setSpec == PredictedTest) {
      for (int i = 0; i < results.size(); i++) {
	 const InstanceRC& instance(*results[i].instance);
		 
	 // If the set specified is PredictedTest, we need to add a
	 // marking everywhere in the test set (which is actually the 
	 // CatTestResult's training set ) where the label in the test 
	 // matches the label of at least one predicted instance.
	 if (at_least_one_instance_matches(trainingBag,
					   instance,
					   results[i].augCat->num()))
	    diagramManager->add_instance_marking(instance,
					   displayManager->get_shape(0));
      }
   }
   
   
   //loop through each element of the results array and find out which
   //shapes need to be added to the grid for each result.
   for (int i = 0; i < results.size(); i++) {
      const InstanceRC& instance(*results[i].instance);
      int shapeNum;
      
      // find the shape number to use, depending, of course,
      // on the set specification that was obtained from the user's
      // GLDPref. 
      if (setSpec == Test)
	 shapeNum =
	    trainingBag.label_info().get_nominal_val(instance.get_label())
	       - FIRST_CATEGORY_VAL;
      else if (setSpec == Predicted || setSpec == PredictedTrain ||
	       setSpec == PredictedTest)
	 shapeNum = results[i].augCat->num() - FIRST_CATEGORY_VAL;
      else if (setSpec == Overlay)
	 shapeNum = (results[i].augCat->num() ==
	     trainingBag.label_info().get_nominal_val(instance.get_label()));
      
      // If the set specified is PredictedTest, we need to add a
      // marking everywhere in the test set (which is actually the 
      // CatTestResult's training set ) where the label in the test 
      // does not match the label of the predicted instance.
      if (setSpec == PredictedTest) {
	 if (!all_instances_match(trainingBag, instance,
				  results[i].augCat->num()))
	       diagramManager->add_instance_marking(instance,
					    displayManager->get_shape(1));
      }
      
      LOG(2,"GLD::GLD: Adding shape number " << shapeNum <<
	  " for label " << instance.label_info().get_nominal_val(
	     instance.get_label()));
      
      if (setSpec == PredictedTrain || setSpec == PredictedTest) {
	 diagramManager->add_instance(instance,
			      displayManager->get_color_square(shapeNum));
      }
      else
	 diagramManager->add_instance(instance,
				      displayManager->get_shape(shapeNum));
   }
   
   diagramManager->run();
}





