// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : This class will eventually contain all of the user
                   adjustable preferences for the GLD class.  When
		   constructing a GLD, the user is forced to create a
		   GLDPref and pass it into the GLD.
		 Currently, two types of display formats are
		   supported, Xfig and Motif.
		 A user has a choice of five sets of information to
		   display on a GLD:
		     1.  Test, which corresponds to the test set
		     passed into the CatTestResult,
		     2.  Predicted, which corresponds to the results
		     produced by the CatTestResult's inducer, and
		     3.  Overlay, which corresponds to the difference
		     between sets 1 and 2.
		     4.  PredictedTest, which overlays X's on the
		     predicted set when it is wrong.
		     5.  PredictedTrain, which overlays the training
		     set on the predicted set.
		 GLD uses MLCOStream for writing all file output
		   in the creation of persistant GLDs.  This is
		   currently important for an Xfig diagram, since this
		   is the file to which all the Xfig output is written.  
  Assumptions  : If the client of this module does not set any of the
                   options, it is assumed that defaults will be used.
		   However, once an option is set, there is no way to
		   reset the options to default values.
  Comments     : Although all of the options have methods to change
                   their attributes, it is case-dependant as to
		   whether changing an attribute after passing a
		   GLDPref into the GLD will immediately trigger the
		   concomitant output update.
		 A convenient utility exists in /u/mlc/util which
		   allows these options to be set via environment
		   variables.  This saves the user recompilation. 
  Complexity   :
  Enhancements : Other user options to be added: font preferences,
                   line style preferences, and restrictions to shape
		   size (eg: lower bound of how small a shape can get).
  History      : Dave Manley                                      03/02/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <GLDPref.h>
#include <mlcIO.h>

RCSID("MLC++, $RCSfile: GLDPref.c,v $ $Revision: 1.5 $")

const MString DEFAULT_GLD_FILE_NAME = "GLDoutput.out";
const int NUMBER_OF_LINE_WIDTHS = 36;
const int NUMBER_OF_SHAPES = 20;
const int DEFAULT_AXES_LINE_WIDTH = 4;

// NUMBER_OF_DIFFERENT_SHAPES and NUMBER_OF_DIFFERENT_COLORS should be
// less than or equal to the number of shapes and colors // defined in
// GLDShape and GLDColor, respectively.

const int NUMBER_OF_DIFFERENT_SHAPES = 9;
const int NUMBER_OF_DIFFERENT_COLORS = 7;

// Defined to be able to fit on an Xfig page.
const int DEFAULT_PIXEL_SIZE = 295;

// The drawing margin is the space between the grid and the edges of
// the window.  The left and bottom margins are reserved for placing
// names and labels.  The value is in number of pixels.
const int DEFAULT_DRAWING_MARGIN = 185;

/***************************************************************************
    Description :  Construct a GLDPref.
    Comments    :  The default values are assumed until the user sets
                     other values.
***************************************************************************/  
GLDPref::GLDPref()
: shapePreference(NUMBER_OF_SHAPES),
  colorPreference(NUMBER_OF_SHAPES)
{
   // initialize Shape and Color values to defaults.
   int shapeType = 0;
   int shapeColor = 0;
   for (int i = 0; i < NUMBER_OF_SHAPES; i++) {
      shapePreference[i] = (GLDShape)shapeType++;
      colorPreference[i] = (GLDColor)shapeColor++;
      shapeType = shapeType % NUMBER_OF_DIFFERENT_SHAPES;
      shapeColor = shapeColor % NUMBER_OF_DIFFERENT_COLORS;
   }
   defaultMLCOStreamInUse = FALSE;
   mlcOStream = NULL;
   horizontalPixels = DEFAULT_PIXEL_SIZE;
   verticalPixels = DEFAULT_PIXEL_SIZE;
   horizontalMargins = DEFAULT_DRAWING_MARGIN;
   verticalMargins = DEFAULT_DRAWING_MARGIN;
   axesLineWidth = DEFAULT_AXES_LINE_WIDTH;
   positionsSpecified = FALSE;
   displayAvailable = TRUE;
}


/***************************************************************************
    Description :  If the default mlcOstream was used, this means that
                     this module allocated the default stream, and it
		     must be deleted.  
    Comments    :  
***************************************************************************/  
GLDPref::~GLDPref()
{
   if (defaultMLCOStreamInUse)
      delete mlcOStream;
   if (positionsSpecified) {
      delete axisArray;
      delete positionArray;
   }
}


/***************************************************************************
    Description :  The SetSpecification can take on the values of
                     Test, Predicted, or Overlay.
    Comments    :  The default is Test.
***************************************************************************/  
void GLDPref::set_set_specification(const SetSpecification
				    newSetSpecification)
{
   setSpecification = newSetSpecification; 
}

SetSpecification GLDPref::get_set_specification() const
{
   return setSpecification;
}


/***************************************************************************
    Description :  Sets the MLCOStream used by the GLD.  Note that
                     this is only necessarily required by the
		     XfigDisplayManager.  
    Comments    :  The default is a file named with the
                     DEFAULT_GLD_FILE_NAME.
		   get_MLCOStream() is not const because if the user
		     hasn't set a stream, a default one is opened. 
***************************************************************************/  
void GLDPref::set_MLCOStream(MLCOStream *newMLCOStream)
{
   if (defaultMLCOStreamInUse && defaultMLCOStreamInUse) {
      delete mlcOStream;
      defaultMLCOStreamInUse = FALSE;
   }
   mlcOStream = newMLCOStream;
}

MLCOStream *GLDPref::get_MLCOStream() 
{
   if (!mlcOStream) {
      mlcOStream = new MLCOStream(DEFAULT_GLD_FILE_NAME);
      defaultMLCOStreamInUse = TRUE;
   }
   return mlcOStream;
}


/***************************************************************************
    Description :  The options for the DisplayManagerType are
                     MotifDisplayManagerType and XfigDisplayManagerType.
    Comments    :  The default is MotifDisplayManagerType.
***************************************************************************/  
void GLDPref::set_display_manager_type(const DisplayManagerType
				       newDisplayManagerType)
{
   displayManagerType = newDisplayManagerType;
}


DisplayManagerType GLDPref::get_display_manager_type() const 
{
   return displayManagerType;
}


/***************************************************************************
    Description :  The maximum line width is for bounding how thick
                     the axes lines can get in the GLD. 
    Comments    :  The default is DEFAULT_AXES_LINE_WIDTH.
***************************************************************************/  
void GLDPref::set_axes_line_width(int newLineWidth)
{
   ASSERT(newLineWidth > -1);
   axesLineWidth = newLineWidth;
}


int GLDPref::get_axes_line_width() const
{
   return axesLineWidth;
}


/***************************************************************************
    Description :  These routines get and set the colors and shapes
                     used for displaying instance labels.
    Comments    :  
***************************************************************************/  
void GLDPref::set_shape_color(int shapeNumber,
			      const GLDColor newColor)
{
   colorPreference[shapeNumber] = newColor;
}
   

GLDColor GLDPref::get_shape_color(int shapeNumber) const
{
   return colorPreference[shapeNumber];
}


void GLDPref::set_shape_type(int shapeNumber,
			     const GLDShape newType)
{
   shapePreference[shapeNumber] = newType;
}
   

GLDShape GLDPref::get_shape_type(int shapeNumber) const
{
   return shapePreference[shapeNumber];
}


/***************************************************************************
    Description :  These routines get and set the dimensions of the
                     diagram. 
    Comments    :  The unit of pixels is used both by Xfig and Motif.
***************************************************************************/  
void GLDPref::set_vertical_pixels(int pixels)
{
   ASSERT(pixels > 0);
   verticalPixels = pixels;
}

int GLDPref::get_vertical_pixels() const
{
   return verticalPixels;
}

void GLDPref::set_horizontal_pixels(int pixels)
{
   ASSERT(pixels > 0);
   horizontalPixels = pixels;
}

int GLDPref::get_horizontal_pixels() const
{
   return horizontalPixels;
}

/***************************************************************************
    Description :  These routines get and set the dimensions of the
                     margins of the diagram in pixels. 
    Comments    :  The unit of pixel is used both by Xfig and Motif.
***************************************************************************/  
void GLDPref::set_vertical_margin(int margin)
{
   ASSERT(margin > 0);
   verticalMargins = margin;
}

int GLDPref::get_vertical_margin() const
{
   return verticalMargins;
}

void GLDPref::set_horizontal_margin(int margin)
{
   ASSERT(margin > 0);
   horizontalMargins = margin;
}

int GLDPref::get_horizontal_margin() const
{
   return horizontalMargins;
}
   

/***************************************************************************
    Description :  These routines allow the user to specify the order
                     on which the attributes will appear on the axes. 
    Comments    :  Since the position arrays are copied, alterations made to
                     the arrays after being passed to specify_positions()
		     are ignored.
		   u_GLD.c is a good example of how these functions
		     may be used.
***************************************************************************/  
void GLDPref::specify_positions(Array<AttrOrientation> &tempAxis,
				Array<int> &tempPos)
{
   positionsSpecified = TRUE;
   axisArray = new Array<AttrOrientation>(tempAxis, ctorDummy);
   positionArray = new Array<int>(tempPos, ctorDummy);
}

Bool GLDPref::positions_specified() const 
{
   return positionsSpecified;
}

AttrOrientation GLDPref::get_orientation(int index) const
{
   return (*axisArray)[index];
}

int GLDPref::get_position(int index) const
{
   return (*positionArray)[index];
}


/***************************************************************************
    Description :  These routines allow the user to specify whether an
                     X window display is available.
    Comments    :  The main purpose for this is to allow night runs
                     (which do not have displays).  Clearly, at other
		     times, when one is generating an Xfig diagram,
		     they will want to have an X terminal in order to
		     view the result.
		   We considered making this a compile time flag, but
		     the problem is that the library (as opposed to
		     just the tester) would have to be compiled every
		     time that this flag changed.
***************************************************************************/  
void GLDPref::use_display(Bool displayIsAvailable) {
   displayAvailable = displayIsAvailable;
}

Bool GLDPref::is_display() const {
   return displayAvailable;
}
