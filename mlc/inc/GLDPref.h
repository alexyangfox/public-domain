// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _GLDPref_h
#define _GLDPref_h 1

#include <Array.h>

extern const int NUMBER_OF_LINE_WIDTHS;
extern const int NUMBER_OF_SHAPES;
extern const int NUMBER_OF_DIFFERENT_COLORS;

// PredictedTest shows X's over the shape on the instances that were
// predicted incorrectly.  PredictedTrain shows the training set over
// the predicted instances.
enum SetSpecification { Test, Predicted, Overlay, PredictedTrain,
			PredictedTest };

enum DisplayManagerType { MotifDisplayManagerType,
			  XfigDisplayManagerType };

enum AttrOrientation { HorizontalAttr, VerticalAttr };

extern const int NUMBER_OF_DIFFERENT_SHAPES;
extern const int NUMBER_OF_DIFFERENT_COLORS;
enum GLDShape { Circle, XMark, Slash, BackSlash, FilledCircle,
		Rectangle, Triangle, TopHalf, BottomHalf, Line, Fill };
enum GLDColor { Black, Blue, Green, Cyan, Red, Magenta, Yellow, White };

class GLDPref {
   MLCOStream *mlcOStream;
   Bool defaultMLCOStreamInUse;
   SetSpecification setSpecification;
   DisplayManagerType displayManagerType;
   int axesLineWidth;

   Array<GLDShape> shapePreference;
   Array<GLDColor> colorPreference;

   int horizontalPixels;
   int verticalPixels;
   int horizontalMargins;
   int verticalMargins;
   
   Bool positionsSpecified;
   Array<AttrOrientation> *axisArray;
   Array<int> *positionArray; 

   int displayAvailable;

public:
   GLDPref();
   virtual ~GLDPref();
   
   void set_set_specification(const SetSpecification setSpecification);
   SetSpecification get_set_specification() const;
   
   void set_MLCOStream(MLCOStream *mlcOStream);
   // not const because if the user hasn't set a stream, a default one
   // is opened.
   MLCOStream *get_MLCOStream(); 

   void set_display_manager_type(const DisplayManagerType
					displayManagerType);
   DisplayManagerType get_display_manager_type() const;

   void set_axes_line_width(int newLineWidth);
   int get_axes_line_width() const;

   void set_shape_color(int shapeNumber,
			const GLDColor newColor);
   GLDColor get_shape_color(int shapeNumber) const;

   void set_shape_type(int shapeNumber,
		       const GLDShape newShape);
   GLDShape get_shape_type(int shapeNumber) const;

   void set_horizontal_pixels(int pixels);
   int get_horizontal_pixels() const;
   
   void set_vertical_pixels(int pixels);
   int get_vertical_pixels() const;

   void set_horizontal_margin(int margin);
   int get_horizontal_margin() const;
   
   void set_vertical_margin(int margin);
   int get_vertical_margin() const;
   
   void specify_positions(Array<AttrOrientation> &tempAxis,
			  Array<int> &tempPos);
   Bool positions_specified() const;
   AttrOrientation get_orientation(int index) const;
   int get_position(int index) const;

   void use_display(Bool displayIsAvailable);
   Bool is_display() const;
};
   

#endif
