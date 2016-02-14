// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a utility for MLC++
// It is not part of the MLC++ library itself, but instead uses the
//   library to provide a useful function.
/***************************************************************************
  Description  : Utility for GLDs.
  Usage        : Environment variables to set:
                   DATAFILE
		   INDUCER     
		   GLD_HORIZONTAL_PIXELS (integer)
		   GLD_VERTICAL_PIXELS (integer)
		   GLD_SET (Test, Predicted, Overlay, PredictedTest, or
		     PredictedTrain)
		   GLD_COLOR_FILL (yes, no) 
		   GLD_MANAGER (Xfig, Motif)
		   GLD_MAX_LINE_WIDTH (0 to 16) 
		   GLD_ALT_COLOR_SCHEME (yes, no)
		   GLD_MANUAL_ORDER (yes, no)
		   GLD_OUTFILE (any filename, defaults to GLD.fig)
		   GLD_HORIZONTAL_MARGIN (integer)
		   GLD_VERTICAL_MARGIN (integer)
		   GLD_COLOR_DISPLAY (yes, no)
		 Note that GLD_COLOR_FILL determines whether patches
		   of pure color will fill the cells instead of using
		   shapes.  Also, in the case of PredictedTest and
		   PredictedTrain, this option is ignored, as the
		   color fill option does not make sense.  (This is
		   because these options use both patches of color and
		   shapes.) 
  Enhancements : Move the full space generator into another file,
                   probably as part of some "data generator" tool.
                 Add preference of output file as an environment
		   variable. 
  History      : Dave Manley                                         2/5/94
                  Initial revision (.h,.c)
***************************************************************************/
#include <basics.h>
#include <CatTestResult.h>
#include <GLD.h>
#include <CtrInstList.h>
#include <env_inducer.h>
#include <Inducer.h>
#include <InstList.h>
#include <LabInstGen.h>
#include <mlcIO.h>
#include <FileNames.h>
#include <GetOption.h>

RCSID("MLC++, $RCSfile: u_GLD.c,v $ $Revision: 1.16 $")

const int DEFAULT_GLD_PIXELS = 400;
const int DEFAULT_H_MARGIN = 300;
const int DEFAULT_V_MARGIN = 100;

const MEnum specEnum =
  MEnum("test", Test) << MEnum("predicted", Predicted) <<
  MEnum("predictedTest", PredictedTest) <<
  MEnum("predictedTrain", PredictedTrain) <<
  MEnum("overlay", Overlay);

const MEnum displayManagerEnum =
  MEnum("Xfig", XfigDisplayManagerType) <<
  MEnum("Motif", MotifDisplayManagerType);

const MString specHelp = "This option determines the contents of the GLD.  "
  "TEST displays the test set. "
  "PREDICTED displays the predicted space. "
  "PREDICTEDTEST and PREDICTEDTRAIN combine the PREDICTED results with a "
  "display of the test or training sets. "
  "OVERLAY displays the difference between test and predicted sets.";
const MString readAttrsHelp = "This option determines whether or not to "
  "read the attribute ordering from the terminal.";
const MString colorSchemeHelp = "This option chooses between two possible "
  "color schemes.";
const MString verticalMarginHelp = "This option sets the vertical margin "
  "for the final display of the GLD";
const MString horizontalMarginHelp = "This option sets the horizontal "
  "margin for the final display of the GLD";
const MString displayManagerHelp = "This option chooses the form of the "
  "final display.  Choosing Motif will pop up a Motif window containing "
  "the diagram.  Choosing Xfig will create an Xfig-compatible file "
  "containing the diagram.";
const MString colorDisplayHelp = "This option determines whether to use "
  "color to display the GLD.  If color is not used, then the cells in the "
  "GLD will contain different shapes.";
const MString horizontalPixHelp = "This option determines how wide the "
  "GLD will be in pixels.  GLD's cells are automatically made square, "
  "so the value chosen for this option may be adjusted.";
const MString verticalPixHelp = "This option determines how tall the "
  "GLD will be in pixels.  GLD's cells are automatically made square, "
  "so the value chosen for this option may be adjusted.";
const MString fillHelp = "Setting this option to TRUE causes cells to "
  "be filled with color.  Otherwise, cells will be filled with shapes.";
const MString outFileHelp = "This option specifies a filename to use for "
  "xfig-compatible output.";
const MString maxWidthHelp = "This option determines the maximum width "
  "permitted for one line in the GLD.";
const MString defaultFontHelp = "Pick the font for Motif.  "
   "See /usr/lib/X11/fonts.";
   


/***************************************************************************
  Description : Remove all instances with unknown attributes values.
  Comments    : Prints out a warning if any are actually removed.
***************************************************************************/
static void remove_inst_unknown_attr_vals(InstanceList *lil)
{
   int count = lil->num_instances(); 
   lil->remove_inst_with_unknown_attr();
   if (count != lil->num_instances())
      cout << "!! Warning.  Instances with unknown values have been" 
	 " removed. !!" << endl;
}


/***************************************************************************
  Description : Read in the given attributes and their order.
                Project the space on the used attributes, and reread the
		new datafile.  
  Comments    :
***************************************************************************/

// @@ temp files are never erased yet.  Best solution is probably
// @@ to have an Array<MString> of cleanup files and insert the names,
// @@ data, and possibly the test file to it.

static void set_attr_pref(GLDPref& gldPref, SetSpecification setSpec,
			  CtrInstanceList*& trainBag, InstanceList*& testBag)
{
   int totalNumAttrs = trainBag->num_attr();
   
   Array<AttrOrientation> fullAxis(totalNumAttrs);
   Array<int> fullPos(totalNumAttrs);
   BoolArray attributesUsed(0, totalNumAttrs, FALSE);

   Mcout << "Enter attribute numbers for vertical axis, "
      "starting at innermost. End with -1" << endl;
   int horizcount = 0;
   int horizNum;
   do {
      Mcin >> horizNum;
      if (horizNum >= 0  && horizNum < totalNumAttrs) {
	 if (attributesUsed[horizNum])
	    Mcout << "Duplicate attribute ignored" << endl;
	 else {
	    MString name(trainBag->attr_info(horizNum).name());
	    Mcout << "Attribute " << horizNum << " is " << name << endl;
	    fullAxis[horizNum] = HorizontalAttr;
	    fullPos[horizNum] = horizcount++;
	    attributesUsed[horizNum] = TRUE;
	 }
      } else if (horizNum != -1)
	 Mcout << "Invalid attribute number.  Must be 0 to "
	       << totalNumAttrs - 1 << endl;
   } while (horizNum != -1);
      

   Mcout << "Enter attribute numbers horizontal axis, "
	 "starting at topmost. End with -1" << endl;
   int vertcount = 0;
   int vertNum;
   do {
      Mcin >> vertNum;
      if (vertNum >= 0  && vertNum < totalNumAttrs) {
	 if (attributesUsed[vertNum])
	    Mcout << "Duplicate attribute ignored" << endl;
	 else {
	    MString name(trainBag->attr_info(vertNum).name());
	    Mcout << "Attribute " << vertNum << " is " << name << endl;
	    fullAxis[vertNum] = VerticalAttr;
	    fullPos[vertNum] = vertcount++;
	    attributesUsed[vertNum] = TRUE;
	 }
      } else if (vertNum != -1)
	 Mcout << "Invalid attribute number.  Must be 0 to "
	       << totalNumAttrs - 1<< endl;
   } while (vertNum != -1);

   int numPickedAttrs = vertcount + horizcount;
   Array<AttrOrientation> tempAxis(numPickedAttrs);
   Array<int> tempPos(numPickedAttrs);

   int posCount = 0;
   for (int i = 0; i < totalNumAttrs; i++) {
      if (attributesUsed[i]) {
	 tempPos[posCount] = fullPos[i];
	 tempAxis[posCount] = fullAxis[i];
	 posCount++;
      }
   }
   ASSERT (posCount == numPickedAttrs);
   gldPref.specify_positions(tempAxis, tempPos);

   CtrInstanceList *projectedTrain = &trainBag->project(attributesUsed)->
      cast_to_ctr_instance_list();

   delete trainBag;
   trainBag = projectedTrain;

   // Generated projected test set for those options that need them.
   if (setSpec != PredictedTrain && setSpec != Predicted) {
      ASSERT(testBag != NULL);
      InstanceList *projectedTest = &testBag->project(attributesUsed)->
	   cast_to_instance_list();   
      delete testBag;
      testBag = projectedTest;
   }
}



main()
{
   FileNames files;
   
   Mcout << "Generating GLD for " << files.data_file() << endl;
   GLDPref gldPreference;

   CtrInstanceList* trainList = new CtrInstanceList("", files.names_file(),
						    files.data_file());

   SetSpecification setSpec =
      get_option_enum("GLD_SET", specEnum, PredictedTest, specHelp, FALSE);
   Bool readAttrs =
      get_option_bool("GLD_MANUAL_ORDER", FALSE, readAttrsHelp, TRUE);

   // remove from trainlist
   remove_inst_unknown_attr_vals(trainList);
      
   Inducer *inducer = &env_inducer()->cast_to_inducer(); 
   InstanceList *testList = NULL;
         
   InstanceList *testAsTrainList = NULL;
   if (setSpec == Test || setSpec == Overlay) {
      testList = new InstanceList("", files.names_file(), files.test_file());
      remove_inst_unknown_attr_vals(testList);
      if (readAttrs)
	 set_attr_pref(gldPreference, setSpec, trainList, testList);
   }
   else if (setSpec == PredictedTest) {
      testAsTrainList = new InstanceList("", files.names_file(),
					 files.test_file());
      if (readAttrs)
	 set_attr_pref(gldPreference, setSpec, trainList, testAsTrainList);
      remove_inst_unknown_attr_vals(testAsTrainList);
      testList = new InstanceList(trainList->get_schema());
      full_space_generator(testList);
   } else if (setSpec == PredictedTrain || setSpec == Predicted) {
      if (readAttrs)
	 set_attr_pref(gldPreference, setSpec, trainList, testList);
      ASSERT(testList == NULL);
      testList = new InstanceList(trainList->get_schema());
      full_space_generator(testList);
   } else
      ASSERT(FALSE);
   
   ASSERT(testList != NULL);
   inducer->assign_data(trainList);
   ASSERT(trainList == NULL); // assign data gets ownership
   inducer->train();
   
   CatTestResult *result;
   
   // Note that in the case of PredictedTest, we need to pass the
   // "real" test list as the training list.  (By "real" I mean the
   // test list that has categories upon which predictions can be
   // evaluated.)  This is so that when using the CatTestResult to
   // construct a GLD, this information is available to decide which
   // instances of the "real" test list were predicted correctly.
   // Since we are using a full space generated test list as the test
   // list argument in constructing the CatTestResult, the only
   // place for the "real" test list, then, is the training list.

   if (setSpec == PredictedTest) 
      result = new CatTestResult(inducer->get_categorizer(),
				 *testAsTrainList,
				 *testList);
   else 
      result = new CatTestResult(inducer->get_categorizer(),
				 inducer->instance_bag(),
				 *testList);      
   gldPreference.set_set_specification(setSpec);
   
   // These are common color preferences.
   if(get_option_bool("GLD_ALT_COLOR_SCHEME", FALSE, colorSchemeHelp, TRUE)) {
      gldPreference.set_shape_color(0, Magenta);
      gldPreference.set_shape_color(5, Yellow);
      gldPreference.set_shape_color(1, Cyan);
      gldPreference.set_shape_color(7, Black);
      gldPreference.set_shape_color(6, White);
      gldPreference.set_shape_color(3, Blue);
   } else {
      gldPreference.set_shape_color(0, Magenta);
      gldPreference.set_shape_color(1, Yellow);
      gldPreference.set_shape_color(2, Green);
      gldPreference.set_shape_color(3, Blue);
      gldPreference.set_shape_color(4, Cyan);
      gldPreference.set_shape_color(5, White);
      gldPreference.set_shape_color(6, Red);
      gldPreference.set_shape_color(7, Black);
   }
      

   int horizontalDrawingMargin =
      get_option_int("GLD_HORIZONTAL_MARGIN", DEFAULT_H_MARGIN,
		     horizontalMarginHelp, TRUE);
   gldPreference.set_horizontal_margin(horizontalDrawingMargin);
   
   int verticalDrawingMargin =
      get_option_int("GLD_VERTICAL_MARGIN", DEFAULT_V_MARGIN,
		     verticalMarginHelp, TRUE);
   gldPreference.set_vertical_margin(verticalDrawingMargin);

   DisplayManagerType dmt =
      get_option_enum("GLD_MANAGER", displayManagerEnum,
		      MotifDisplayManagerType, displayManagerHelp,
		      FALSE);   

   if (dmt == MotifDisplayManagerType)
     defaultFontName = get_option_string("GLD_FONT", defaultFontName,
		       defaultFontHelp, FALSE);

   int horizontalPix = get_option_int("GLD_HORIZONTAL_PIXELS",
					DEFAULT_GLD_PIXELS,
				      horizontalPixHelp, TRUE);   
   int verticalPix = get_option_int("GLD_VERTICAL_PIXELS",
				    DEFAULT_GLD_PIXELS,
				    verticalPixHelp, TRUE);
   gldPreference.set_horizontal_pixels(horizontalPix);
   gldPreference.set_vertical_pixels(verticalPix);


   Bool colorDisp = TRUE;
   if (setSpec != PredictedTrain && setSpec!=PredictedTest)
     colorDisp = get_option_bool("GLD_COLOR_DISPLAY", TRUE, colorDisplayHelp,
				    TRUE);
   if(!colorDisp)
      for (int i = 0; i < NUMBER_OF_DIFFERENT_COLORS; i++)
         gldPreference.set_shape_color(i, Black);
       
      
   Bool fillOption = FALSE;
   if (setSpec != PredictedTrain && setSpec!=PredictedTest && colorDisp)
      fillOption = get_option_bool("GLD_COLOR_FILL", TRUE,
				     fillHelp, TRUE);

   if (fillOption && setSpec != PredictedTrain
       && setSpec!=PredictedTest)
      for (int i = 0; i < NUMBER_OF_SHAPES; i++)
	 gldPreference.set_shape_type(i, Fill);
   
   gldPreference.set_display_manager_type(dmt);

   if (dmt == XfigDisplayManagerType) {
      MString outFile = get_option_string("GLD_OUTFILE", "GLD.fig",
					  outFileHelp, TRUE);
      MLCOStream* outputFile = new MLCOStream(outFile);
      gldPreference.set_MLCOStream(outputFile);
   };

  

   int maxWidth = get_option_int_range("GLD_MAX_LINE_WIDTH", 4, 0, 16,
				 maxWidthHelp, TRUE);
   gldPreference.set_axes_line_width(maxWidth);
   // for predictedTest, the test set is fake and we'll always get 0 accuracy
   // if (setSpec != PredictedTest && setSpec != Predicted) 
   //    Mcout << *result << endl;
   
   GLD gld(*result, &gldPreference);
   delete testList;
   delete testAsTrainList;
   delete result;
   delete trainList;
   delete inducer;
   
   return 0; // return success to shell
}






