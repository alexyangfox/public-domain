// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Tests GLD.c, and by its nature, tests much of the
                   following related classes: Shape.c, Diagram.c,
		   DiagramMngr.c, GLDPref.c, and DisplayMngr.c.      
  Doesn't test : Some of the code that is tested by other testers in
                   the GLD family.  For example, the complete array of
		   Shapes is tested in t_Shape.c.  Also, some of the
		   error cases at other levels are impossible to test
		   here, so they are done in other testers.  
  Enhancements : New tests for preferences will need to be added when
                   a GLDPreferences class is added.
  History      : Dave Manley                                      12/20/93
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <ConstInducer.h>
#include <CtrInstList.h>
#include <CatTestResult.h>
#include <BagSet.h>
#include <GLD.h>

RCSID("MLC++, $RCSfile: t_GLD.c,v $ $Revision: 1.6 $")

main()
{
   cout << "t_GLD.c executing" << endl;
   InstanceList* constTrainList;

   MString filename = "t_GLD";
   constTrainList = new InstanceList(filename);
   InstanceList constTestList(filename, ".names", ".test");

   ConstInducer constInducer(filename + " const inducer");
   constInducer.assign_data(constTrainList);
   ASSERT(constTrainList == NULL); // assign data gets ownership
   constInducer.train();
   
   CatTestResult constResult(constInducer.get_categorizer(),
			     constInducer.instance_bag().
			     cast_to_instance_list(), constTestList);

   GLDPref gldPreference;
   SetSpecification ss = Test;
   gldPreference.set_set_specification(ss);
   DisplayManagerType dmt = XfigDisplayManagerType;
   gldPreference.set_display_manager_type(dmt);
   MLCOStream output_file("t_GLD.out1");
   gldPreference.set_MLCOStream(&output_file);

   gldPreference.set_axes_line_width(8);
   ASSERT(gldPreference.get_axes_line_width() == 8);

   gldPreference.set_shape_color(0 , Cyan);
   ASSERT(gldPreference.get_shape_color(0) == Cyan);

   gldPreference.set_shape_type(0, FilledCircle);
   ASSERT(gldPreference.get_shape_type(0) == FilledCircle);

   gldPreference.set_horizontal_pixels(800);
   ASSERT(gldPreference.get_horizontal_pixels() == 800);

   gldPreference.set_vertical_pixels(1400);
   ASSERT(gldPreference.get_vertical_pixels() == 1400);

   gldPreference.set_horizontal_margin(200);
   ASSERT(gldPreference.get_horizontal_margin() == 200);

   gldPreference.set_vertical_margin(200);
   ASSERT(gldPreference.get_vertical_margin() == 200);

   gldPreference.use_display(FALSE);
   ASSERT(!gldPreference.is_display());
   
   GLD *gld = new GLD(constResult, &gldPreference);
   delete gld;

   return 0; // return success to shell
}






