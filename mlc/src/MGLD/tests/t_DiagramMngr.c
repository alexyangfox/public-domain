// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Tests DiagramManager.c.      
  Doesn't test :
  Enhancements : 
  History      : Dave Manley                                      12/18/93
                   Initial revision (.h,.c)
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <CatTestResult.h>
#include <BagSet.h>
#include <mlcIO.h>
#include <GLD.h>

#include <ConstInducer.h>
#include <ID3Inducer.h>
#include <CtrInstList.h>
#include <DisplayMngr.h>
#include <Instance.h>

RCSID("MLC++, $RCSfile: t_DiagramMngr.c,v $ $Revision: 1.11 $")

main()
{
   cout << "running t_DiagramMngr.c" << endl;

#ifdef NO_DISPLAY
   // This dummy call is needed to avoid link error which says that
   // XtInherit is not defined.
   XtToolkitInitialize();
#else
   
   InstanceList* constTrainList;

   MString filename = "t_DiagramMngr";
   constTrainList = new InstanceList(filename);
   InstanceList constTestList(filename, ".names", ".test");

   ConstInducer constInducer(filename + " const inducer");
   constInducer.assign_data(constTrainList);
   ASSERT(constTrainList == NULL); // assign data gets ownership
   constInducer.train();
   
   CatTestResult catTestResult(constInducer.get_categorizer(),
			       constInducer.instance_bag().
			       cast_to_instance_list(),
			       constTestList);

   const Array<CatOneTestResult>& results = catTestResult.get_results();
   const InstanceBag& instanceBag = catTestResult.get_training_bag();

   GLDPref gldPreference;
   SetSpecification ss = Test;
   gldPreference.set_set_specification(ss);
   DisplayManagerType dmt = MotifDisplayManagerType;
   gldPreference.set_display_manager_type(dmt);
   MLCOStream output_file("t_DiagramMngr.out1");
   gldPreference.set_MLCOStream(&output_file);
   
   DiagramManager *diagramManager =
      new DiagramManager(instanceBag.get_schema(), &gldPreference);

   DisplayManager *displayManager =
      diagramManager->get_display_manager();

   diagramManager->swap_attributes(HorizontalAttr, 0, VerticalAttr, 0);
   
   AttrPositionInfo api(instanceBag.get_schema(), diagramManager,
			&gldPreference);
   
   // test for record that doesn't exist
   TEST_ERROR("get_index found no record of the requested",
	      api.get_index((AttrOrientation)22, 101));

   
   for (int i = 0; i < results.size(); i++) {
      const InstanceRC instance = *results[i].instance;
      int shapeNum = instance.label_info().get_nominal_val(
 	                         instance.get_label()) - FIRST_CATEGORY_VAL;
      diagramManager->add_instance(instance,
				   displayManager->get_shape(shapeNum));
   }

   // create an instance with an unknown value.
   InstanceRC instance(instanceBag.get_schema());
   AttrValue_ val;
   const NominalAttrInfo& nai =
      instanceBag.get_schema().nominal_attr_info(0);
   nai.set_nominal_val(val, UNKNOWN_NOMINAL_VAL);
   instance[0] = val;

   // test for UNKNOWN attribute error.   
   TEST_ERROR("with UNKNOWN_CATEGORY_VAL", 
	      diagramManager->add_instance(instance,
					   displayManager->get_shape(0)));

   delete diagramManager;

#endif
   
   return 0;
}

