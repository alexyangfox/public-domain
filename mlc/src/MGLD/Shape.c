// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Contains the routines for drawing and setting up
                   all of the types of shapes.  
		 In drawing itself, the only parameter necessary
		   is a ShapeBoundingBox.  This is the area the shape is
 		   expected to draw itself into.  A ShapeBoundingBox
		   is created via a constructor which takes four integers.
		   The first two are the coordinates of the upper left
		   corner of the box, and the last two are the lower
		   right corner of the box. 
  Assumptions  : All drawable shapes are inherited from the class Shape.
  Comments     : XFigShape and MotifShape are the only kind of shapes
		   that are currently subclassed from Shape.
		 MotifShapes are XWindow specific.  They have their own
                   graphics context and know only how to draw
		   themselves onto a XDrawingArea (which must be
		   passed to the constructor).
		 The first coordinate of the ShapeBoundingBox is
		   expected to have a smaller value than the third
		   value.  Similarly, the second  value must be
		   smaller than the fourth.  All four values must be
		   non-negative.  Finally, the constant
		   MIN_BOX_DIMENSION defines the smallest allowable
		   dimensions on each side of the defined box.  
		 Currently, all shapes assume that the bounding box is
		   composed of square pixels.  Eventually, displays
		   with non square pixels should be supported.  The
		   routine xdpyinfo(1) provides information on the
		   dimensions of a pixel which can be used to break
		   this dependancy on square pixels.  Note that the
		   ratio happens to be 1 for products engineered by
		   Sun. 
		 For Xfig, note that different shapes have different
		   depths.  This is useful not only because we want
		   grid lines to appear over shapes, but also because
		   we want shapes to appear over color patches. 
  Complexity   : The time complexity of drawing a string is O(number
                   of characters).
  Enhancements : Add new shapes, such as a pixmap shape.
                 Make the shapes non-XWindow or XFig specific. For example:
		   have Windows shapes.
  History      : Dave Manley                                      10/25/93
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <Shape.h>
#include <mlcIO.h>

RCSID("MLC++, $RCSfile: Shape.c,v $ $Revision: 1.8 $")

MString defaultFontName = "9x15";

// Bounding boxes are not allowed to be negative.
const int MIN_BOX_DIMENSION = 0;

// Whether a bounding box has dimensions of at least this size
// determines whether it gets the smaller or the larger thicknesses
// in the following constants. 
const int LARGE_BOX_THRESHOLD = 16;
const int LARGE_BOX_THICKNESS = 4;
const int SMALL_BOX_THICKNESS = 2;

// If there is no display, this is taken to be the default character
// width. 
const int CHAR_WIDTH = 12;

// Gradations in an arc are measured in 64ths of degree, thus
// 360*64 is a full circle of gradations.  (Gradations are what
// X arc drawing routines use as a parameter.)
const int GRADATIONS_IN_DEGREE = 64;
const int DEGREES_IN_CIRCLE = 360;


/***************************************************************************
    Description :  The ShapeBoundingBox constructor takes four corners
                     of a box.
    Comments    :  Since a bounding box of some very small shapes doesn't
                     always make sense for drawing a shape, we get a fatal
		     error if the dimensions are too small.  
***************************************************************************/
ShapeBoundingBox::ShapeBoundingBox(int X1, int Y1,
				   int X2, int Y2)
: x1(X1), y1(Y1), x2(X2), y2(Y2) 
{
   if (x2 - x1 < MIN_BOX_DIMENSION)
      err << "ShapeBoundingBox::ShapeBoundingBox: too small spacing on"
	<< "  the x axis (" << x1 << ", " << x2 << ")"
	<< fatal_error;

   if (y2 - y1 < MIN_BOX_DIMENSION)
      err << "ShapeBoundingBox::ShapeBoundingBox: too small spacing on"
	 << "  the y axis (" << y1 << ", " << y2 << ")"
	 << fatal_error;
}


/***************************************************************************
    Description :  Finds the radius of the bounding box such that a
                     circle drawn with that radius with fit entirely
		     in the box. 
    Comments    :  Returns two minus this number so that circles fit
                     comfortably in this box.
***************************************************************************/
int ShapeBoundingBox::get_radius() const
{
   Real length_x = Real(x2 - x1) / 2;
   Real length_y = Real(y2 - y1) / 2;
   if (length_x < length_y)
      return max(1, int(length_x-0.5));
   else
      return max(1, int(length_y-0.5));
}


/***************************************************************************
    Description :  Takes a bounding box, and returns a bounding box
                     that is smaller by one in each direction.
    Comments    :  Useful for generating progressively smaller boxes,
                     such as is done in generating an outline of a
		     box.  
***************************************************************************/
ShapeBoundingBox ShapeBoundingBox::create_box_smaller_by_one() const
{
   // copy construct a temporary bounding box like the one that was
   // received, except with one less pixel in each direction.
   ShapeBoundingBox tempBoundingBox(x1 + 1, y1 + 1, x2 - 1, y2 - 1);

   // return a new copy of this bounding box.
   return tempBoundingBox;
}


/***************************************************************************
    Description :  Store shape information specific to Motif.
    Comments    : 
***************************************************************************/
MotifShape::MotifShape(Widget w, GC newGC)
{
   set_widget(w);
   set_GC(newGC);
}


/***************************************************************************
    Description :  An example of where one might wish to change the
                     graphics context of a shape is when one wants a
		     new color. 
    Comments    : 
***************************************************************************/
void MotifShape::set_GC(GC newGC)
{
   gc = newGC;
}


/***************************************************************************
    Description :  A MotifShape can be set to one widget at a time;
                     this routine allows that widget to change.
    Comments    : 
***************************************************************************/
void MotifShape::set_widget(Widget w)
{
   widget = w;
   display = XtDisplay(w);
   window = XtWindow(w);
}


/***************************************************************************
    Description :  The following routines are the constructors and the
                     drawing routines for Motif Shapes.
		   The constructors do the same work as the base
		     (MotifShape). 
                   We pass in only the bounding box that the
		     routine is supposed to draw itself within for drawing.
    Comments    :  It is assumed that the graphics context has already been
                     filled in for the shape.
		   It is assumed that the Widget passed in is something that
		     can be used for drawing.
***************************************************************************/
MotifLine::MotifLine(Widget w, GC newGC)
: MotifShape(w, newGC)
{
}

void MotifLine::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   display = XtDisplay(widget);
   window = XtWindow(widget);
   XDrawLine(display, window, gc, shapeBoundingBox.x1, shapeBoundingBox.y1,
	     shapeBoundingBox.x2, shapeBoundingBox.y2); 
}      


MotifCircle::MotifCircle(Widget w, GC newGC)
: MotifShape(w, newGC)
{
}


void MotifCircle::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   display = XtDisplay(widget);
   window = XtWindow(widget);
   XDrawArc(display, window, gc, shapeBoundingBox.x1, shapeBoundingBox.y1,
	    shapeBoundingBox.x2 - shapeBoundingBox.x1,
	    shapeBoundingBox.y2 - shapeBoundingBox.y1, 0,
	    GRADATIONS_IN_DEGREE * DEGREES_IN_CIRCLE); 
}


MotifTopHalfCircle::MotifTopHalfCircle(Widget w, GC newGC)
: MotifShape(w, newGC)
{
}


void MotifTopHalfCircle::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   display = XtDisplay(widget);
   window = XtWindow(widget);
   XFillArc(display, window, gc, shapeBoundingBox.x1, shapeBoundingBox.y1,
	    shapeBoundingBox.x2 - shapeBoundingBox.x1,
	    shapeBoundingBox.y2 - shapeBoundingBox.y1, 0,
	    GRADATIONS_IN_DEGREE * DEGREES_IN_CIRCLE / 2); 
}


MotifBottomHalfCircle::MotifBottomHalfCircle(Widget w, GC newGC)
: MotifShape(w, newGC)
{
}


void MotifBottomHalfCircle::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   display = XtDisplay(widget);
   window = XtWindow(widget);
   XFillArc(display, window, gc, shapeBoundingBox.x1, shapeBoundingBox.y1,
	    shapeBoundingBox.x2 - shapeBoundingBox.x1,
	    shapeBoundingBox.y2 - shapeBoundingBox.y1,
	    GRADATIONS_IN_DEGREE * DEGREES_IN_CIRCLE/2,
   	    GRADATIONS_IN_DEGREE * DEGREES_IN_CIRCLE/2); 
}      


MotifXMark::MotifXMark(Widget w, GC newGC)
: MotifShape(w, newGC)
{
}
  

void MotifXMark::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   display = XtDisplay(widget);
   window = XtWindow(widget);
   XDrawLine(display, window, gc, shapeBoundingBox.x1, shapeBoundingBox.y1,
	     shapeBoundingBox.x2, shapeBoundingBox.y2); 
   XDrawLine(display, window, gc, shapeBoundingBox.x1, shapeBoundingBox.y2,
	     shapeBoundingBox.x2, shapeBoundingBox.y1); 
}


MotifSlash::MotifSlash(Widget w, GC newGC)
: MotifShape(w, newGC)
{
}
  

void MotifSlash::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   display = XtDisplay(widget);
   window = XtWindow(widget);
   XDrawLine(display, window, gc, shapeBoundingBox.x1, shapeBoundingBox.y2,
	     shapeBoundingBox.x2, shapeBoundingBox.y1); 
}


MotifBackSlash::MotifBackSlash(Widget w, GC newGC)
: MotifShape(w, newGC)
{
}
  

void MotifBackSlash::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   display = XtDisplay(widget);
   window = XtWindow(widget);
   XDrawLine(display, window, gc, shapeBoundingBox.x1, shapeBoundingBox.y1,
	     shapeBoundingBox.x2, shapeBoundingBox.y2); 
}      


MotifFilledCircle::MotifFilledCircle(Widget w, GC newGC)
: MotifShape(w, newGC)
{
}


void MotifFilledCircle::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   display = XtDisplay(widget);
   window = XtWindow(widget);
   XFillArc(display, window, gc, shapeBoundingBox.x1, shapeBoundingBox.y1,
	    shapeBoundingBox.x2 - shapeBoundingBox.x1,
	    shapeBoundingBox.y2 - shapeBoundingBox.y1,
	    0, GRADATIONS_IN_DEGREE* DEGREES_IN_CIRCLE); 
}


MotifRectangle::MotifRectangle(Widget w, GC newGC)
: MotifShape(w, newGC)
{
}


void MotifRectangle::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   display = XtDisplay(widget);
   window = XtWindow(widget);
   XFillRectangle(display, window, gc, shapeBoundingBox.x1 + 2,
		  shapeBoundingBox.y1 + 2,
		  shapeBoundingBox.x2 - shapeBoundingBox.x1 - 4,
		  shapeBoundingBox.y2 - shapeBoundingBox.y1 - 4); 
}


MotifFill::MotifFill(Widget w, GC newGC)
: MotifShape(w, newGC)
{
}


void MotifFill::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   display = XtDisplay(widget);
   window = XtWindow(widget);
   XFillRectangle(display, window, gc, shapeBoundingBox.x1 - 1,
		  shapeBoundingBox.y1 - 1,
		  shapeBoundingBox.x2 - shapeBoundingBox.x1 + 3,
		  shapeBoundingBox.y2 - shapeBoundingBox.y1 + 3); 
}


MotifTriangle::MotifTriangle(Widget w, GC newGC)
: MotifShape(w, newGC)
{
}


void MotifTriangle::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   display = XtDisplay(widget);
   window = XtWindow(widget);
   XPoint points[3];
   points[0].x = (shapeBoundingBox.x1 + shapeBoundingBox.x2) / 2;
   points[1].x = shapeBoundingBox.x1;
   points[2].x = shapeBoundingBox.x2;
   points[0].y = shapeBoundingBox.y1;
   points[1].y = shapeBoundingBox.y2;
   points[2].y = shapeBoundingBox.y2;
   XFillPolygon(display, window, gc, points, 3, Convex, CoordModeOrigin);
}


MotifBoundingBox::MotifBoundingBox(Widget w, GC newGC)
: MotifShape(w, newGC)
{
}


void MotifBoundingBox::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   ShapeBoundingBox tempBoundingBox = shapeBoundingBox;
      
   // bounding boxes with a size above the threshold get a greater 
   // thickness:
   int thickness;
   if ((shapeBoundingBox.x2 - shapeBoundingBox.x1 >
	LARGE_BOX_THRESHOLD) &&
       (shapeBoundingBox.y2 - shapeBoundingBox.y1 >
	LARGE_BOX_THRESHOLD)) 
      thickness = LARGE_BOX_THICKNESS;
   else
      thickness = SMALL_BOX_THICKNESS;

   display = XtDisplay(widget);
   window = XtWindow(widget);
   
   for (int i = 0; i < thickness; i++) {
      XDrawRectangle(display, window, gc, shapeBoundingBox.x1,
		    shapeBoundingBox.y1,
		    shapeBoundingBox.x2 - shapeBoundingBox.x1,
		    shapeBoundingBox.y2 - shapeBoundingBox.y1); 
      tempBoundingBox = tempBoundingBox.create_box_smaller_by_one(); 
   }
}


/***************************************************************************
    Description :  Text is the only shape that does more than store a
                     Graphics Context and a Widget. Note that not only
		     is the text stored, but some font information is
		     also stored.  
    Comments    :  
***************************************************************************/
MotifText::MotifText(Widget w, GC newGC, const MString& string)
: MotifShape(w, newGC)
{
   textString = string;
   fontName = defaultFontName;

   // Make sure that we get a fontInfo.
   fontInfo = NULL;
   display = XtDisplay(widget);
   fontInfo = XLoadQueryFont(display, fontName);
   if (fontInfo == NULL)
      err << "Cannot find font " << fontName << fatal_error;

   XGCValues values;
   values.font = fontInfo->fid;

   XChangeGC(display, newGC, GCFont, &values);
}


/***************************************************************************
    Description :  Destructor
    Comments    :  fontInfo must be freed by the call to bXFreeFont--it
                     is an error to call delete on it even though it
		     is a pointer. 
***************************************************************************/
MotifText::~MotifText()
{
   display = XtDisplay(widget);
   XFreeFont(display, fontInfo);
} 


/***************************************************************************
    Description :  Draws the text within the given bounding box.
    Comments    :  It is assumed that the graphics context and the
                     shape info has already been filled in for the
		     MotifText shape. 
		   It is assumed that the default font is used.  More
                     work needs to be done to support other fonts.
		     Specifically, see section 6.2 of the Xlib
		     Programming Manual.
		   The basic idea of the calculations is that we need
		     to compensate for the height and centerpoint of a
		     font so that we get reasonable centering of the
		     text within the box.   
***************************************************************************/
void MotifText::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   // find the maximal string that will fit in the bounding box.
   int len = 0;
   int width = 0;

   ASSERT(fontInfo);
   while (len < textString.length() &&
	  width < shapeBoundingBox.x2-shapeBoundingBox.x1) 
      width = XTextWidth(fontInfo, textString, ++len);
   
   if (width >= shapeBoundingBox.x2-shapeBoundingBox.x1 && len > 0)
      len--;
   
   // now center the truncated string in the given bounding box
   int x = shapeBoundingBox.x1 +
      (shapeBoundingBox.x2-shapeBoundingBox.x1-width+1)/2;

   // for more on what ascent and descent are, see page 6.2.4 of the
   // Xlib Programming Manual
   int y = (shapeBoundingBox.y1+shapeBoundingBox.y2+1)/2 +
      (fontInfo->ascent-fontInfo->descent)/2; 

   display = XtDisplay(widget);
   window = XtWindow(widget);
   XDrawString(display, window, gc, x, y, textString, len);
}


/***************************************************************************
    Description :  This function marks the beginning of the Xfig
                     code. 
                   Outputs an Xfig line to the stream.
    Comments    :  Only purpose is to make code a little easier to
                     read.
		   Doesn't create a comment--routine that calls this
		     is responsible for creating comment since it
		     knows more about what it is drawing.
		   Static function.
***************************************************************************/
static void draw_line(MLCOStream *output,
		      const ShapeBoundingBox& shapeBoundingBox,
		      int shapeColor,
		      int shapeDepth,
		      int thickness = 1)
{
   *output << "2 1 0 " << thickness << " " << shapeColor <<
	  " " << shapeDepth << " 0 0 0.000 -1 0 0" <<
	  endl << "\t" << shapeBoundingBox.x1 << " " <<
          shapeBoundingBox.y1 << " " << shapeBoundingBox.x2 <<
	  " " << shapeBoundingBox.y2 << " 9999 9999" << endl;
}   



/***************************************************************************
    Description :  Abstract base class constructor.
    Comments    :  
***************************************************************************/
XfigShape::XfigShape(MLCOStream *newOutput, int color, int depth)
{
   shapeColor = color;
   set_output(newOutput);
   shapeDepth = depth;
}


/***************************************************************************
    Description :  Changes or sets the stream used by a shape.
    Comments    :  
***************************************************************************/
void XfigShape::set_output(MLCOStream *newOutput)
{
   output = newOutput;
}


/***************************************************************************
    Description :  The following routines are the constructors and the
                     drawing routines for Xfig Shapes.
		   The constructors do the same work as the base
		     class--XfigShape. 
                   We pass in only the bounding box that the
		     routine is supposed to draw itself within for the
		     draw routine.
    Comments    :  Note that initialize should be called before any of
                     the Shapes do there drawing.
***************************************************************************/
XfigLine::XfigLine(MLCOStream *output, int thickness, int color,
		   int depth) 
: XfigShape(output, color, depth)
{
   lineThickness = thickness;
}


void XfigLine::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   *output << "#Line: " << endl;
   draw_line(output, shapeBoundingBox, shapeColor, shapeDepth,
	     lineThickness);
   *output << endl;
}


XfigCircle::XfigCircle(MLCOStream *output, int color, int depth)
: XfigShape(output, color, depth)
{
}


void XfigCircle::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   *output << "#Circle: " << endl;

   // we need to find the midpoint and the radius of the circle, since
   // these are the parameters that Xfig uses for drawing.
   int radius = shapeBoundingBox.get_radius();
   int midpoint_x = (shapeBoundingBox.x1 + shapeBoundingBox.x2) / 2;
   int midpoint_y = (shapeBoundingBox.y1 + shapeBoundingBox.y2) / 2;
   
   *output << "1 3 0 1 " << shapeColor << " " << shapeDepth << 
	  " 0 0 0.000 1 0.000 " << midpoint_x <<
	  " " << midpoint_y << " " << radius << " " << radius << " " <<
	  midpoint_y << " " << midpoint_x << " " << midpoint_y << " "
	  << midpoint_x << endl
	  << endl;      
}


XfigTopHalfCircle::XfigTopHalfCircle(MLCOStream *output, int color,
				     int depth)
: XfigShape(output, color, depth)
{
}


void XfigTopHalfCircle::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   *output << "#TopHalfCircle: " << endl;

   // we need to find the midpoint and the radius of the circle, since
   // these are the parameters that Xfig uses for drawing.
   int radius = shapeBoundingBox.get_radius();
   int midpoint_x = (shapeBoundingBox.x1 + shapeBoundingBox.x2) / 2;
   int midpoint_y = (shapeBoundingBox.y1 + shapeBoundingBox.y2) / 2;

   *output << "5 1 0 1 " << shapeColor << " " << shapeDepth <<
	  " 0 0 0.000 0 0 0 " << midpoint_x <<
	  " " << midpoint_y << " " << -radius+midpoint_x << " " <<
	  midpoint_y << " " << midpoint_x << " " <<
	  -radius+midpoint_y << " " << radius +
	  midpoint_x << " " << midpoint_y << endl << endl;  
}


XfigBottomHalfCircle::XfigBottomHalfCircle(MLCOStream *output, int color,
					   int depth)
: XfigShape(output, color, depth)
{
}


void XfigBottomHalfCircle::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   *output << "#BottomHalfCircle: " << endl;

   // we need to find the midpoint and the radius of the circle, since
   // these are the parameters that Xfig uses for drawing.
   int radius = shapeBoundingBox.get_radius();
   int midpoint_x = (shapeBoundingBox.x1 + shapeBoundingBox.x2) / 2;
   int midpoint_y = (shapeBoundingBox.y1 + shapeBoundingBox.y2) / 2;

   *output << "5 1 0 1 " << shapeColor << " " << shapeDepth <<
	  " 0 0 0.000 1 0 0 " << midpoint_x <<
	  " " << midpoint_y << " " << -radius+midpoint_x << " " <<
	  midpoint_y << " " << midpoint_x << " " <<
	  -radius+midpoint_y << " " << radius +
	  midpoint_x << " " << midpoint_y << endl << endl;      
}


XfigFilledCircle::XfigFilledCircle(MLCOStream *output, int color,
				   int depth)
: XfigShape(output, color, depth)
{
}


void XfigFilledCircle::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   *output <<"#FilledCircle: " << endl;

   // we need to find the midpoint and the radius of the FilledCircle, since
   // these are the parameters that Xfig uses for drawing.
   int radius = shapeBoundingBox.get_radius();
   int midpoint_x = (shapeBoundingBox.x1 + shapeBoundingBox.x2) / 2;
   int midpoint_y = (shapeBoundingBox.y1 + shapeBoundingBox.y2) / 2;

   *output << "1 3 0 1 " << shapeColor << " " << shapeDepth <<
	  " 0 21 0.000 1 0.000 " <<
	  midpoint_x << " " << midpoint_y << " " << radius << " " <<
	  radius << " " << midpoint_x << " " << midpoint_y << " " <<
	  midpoint_x << " " << midpoint_y << endl << endl;      
}


XfigXMark::XfigXMark(MLCOStream *output, int color, int depth)
: XfigShape(output, color, depth)
{
}


void XfigXMark::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   *output << "#XMark (composed of two lines): " << endl;
      
   // draw line from the upper-left corner of the box to the
   // lower-right corner.
   draw_line(output, shapeBoundingBox, shapeColor, shapeDepth, 2);

   // draw line from the lower-left corner of the box to the
   // upper-right corner.
   ShapeBoundingBox crossSBB = shapeBoundingBox;
   crossSBB.x1 = shapeBoundingBox.x2;
   crossSBB.x2 = shapeBoundingBox.x1;
   draw_line(output, crossSBB, shapeColor, shapeDepth, 2);
   *output << endl;
}


XfigBackSlash::XfigBackSlash(MLCOStream *output, int color, int depth)
: XfigShape(output, color, depth)
{
}


void XfigBackSlash::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   *output << "#BackSlash: " << endl;
      
   // draw line from the lower-left corner of the box to the
   // upper-right corner.
   ShapeBoundingBox crossSBB = shapeBoundingBox;
   crossSBB.x1 = shapeBoundingBox.x2;
   crossSBB.x2 = shapeBoundingBox.x1;
   draw_line(output, crossSBB, shapeColor, shapeDepth, 3);
   *output << endl;
}


XfigSlash::XfigSlash(MLCOStream *output, int color, int depth)
: XfigShape(output, color, depth)
{
}


void XfigSlash::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   *output << "#Slash: " << endl;
      
   // draw line from the upper-left corner of the box to the
   // lower-right corner.
   draw_line(output, shapeBoundingBox, shapeColor, shapeDepth, 3);

   *output << endl;
}


XfigRectangle::XfigRectangle(MLCOStream *output, int color, int depth)
: XfigShape(output, color, depth)
{
}


void XfigRectangle::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   *output << "#Rectangle: " << endl;

   *output << "2 2 0 1 " << shapeColor << " " << shapeDepth <<
	  " 0 21 0.000 0 0 0" << endl << "\t" <<
	  shapeBoundingBox.x1+2 << " " <<
          shapeBoundingBox.y1+2 << " " << shapeBoundingBox.x1+2<< " " <<
	  shapeBoundingBox.y2-2 << " " << shapeBoundingBox.x2-2 << " " <<
          shapeBoundingBox.y2-2 << " " << shapeBoundingBox.x2-2 << " " <<
          shapeBoundingBox.y1+2 << " " << shapeBoundingBox.x1+2 << " " <<
	  shapeBoundingBox.y1+2 <<" 9999 9999" << endl;
   
   *output << endl;
}


XfigFill::XfigFill(MLCOStream *output, int color, int depth)
: XfigShape(output, color, depth)
{
}


void XfigFill::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   *output << "#Fill up box with color: " << endl;
      
   // draw line from the upper-left corner of the box to the
   // lower-right corner.
   *output << "2 2 0 1 " << shapeColor << " " << shapeDepth <<
	  " 0 21 0.000 0 0 0" << endl << "\t" <<
	  shapeBoundingBox.x1-1 << " " <<
          shapeBoundingBox.y1-1 << " " << shapeBoundingBox.x1-1<< " " <<
	  shapeBoundingBox.y2+1 << " " << shapeBoundingBox.x2+1 << " " <<
          shapeBoundingBox.y2+1 << " " << shapeBoundingBox.x2+1 << " " <<
          shapeBoundingBox.y1-1 << " " << shapeBoundingBox.x1-1 << " " <<
	  shapeBoundingBox.y1-1 <<" 9999 9999" << endl;
   
   *output  << endl;
}


XfigTriangle::XfigTriangle(MLCOStream *output, int color, int depth)
: XfigShape(output, color, depth)
{
}


void XfigTriangle::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   *output  << "#Triangle: " << endl;

   int midX = (shapeBoundingBox.x1 + shapeBoundingBox.x2)/2;
   	    
   *output << "2 3 0 1 " << shapeColor << " " << shapeDepth <<
	  " 0 21 0.000 -1 0 0" << endl << "\t" <<
	  midX << " " << shapeBoundingBox.y1 << " " <<
	  shapeBoundingBox.x1<< " " << shapeBoundingBox.y2 <<
	  " " << shapeBoundingBox.x2 << " " << shapeBoundingBox.y2 <<
	  " " << midX << " " << shapeBoundingBox.y1 << 
	  " 9999 9999" << endl;
   
   *output  << endl;
}


XfigBoundingBox::XfigBoundingBox(MLCOStream *output, int color, int depth)
: XfigShape(output, color, depth)
{
}
 

void XfigBoundingBox::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   *output  << "#BoundingBox (composed of a number of rectangles): "
	  << endl;
   ShapeBoundingBox tempBoundingBox = shapeBoundingBox;
      
   // bounding boxes with a size above the threshold get a greater 
   // thickness:
   int thickness;
   if ((shapeBoundingBox.x2 - shapeBoundingBox.x1 >
	LARGE_BOX_THRESHOLD) &&
       (shapeBoundingBox.y2 - shapeBoundingBox.y1 >
	LARGE_BOX_THRESHOLD)) 
      thickness = LARGE_BOX_THICKNESS;
   else
      thickness = SMALL_BOX_THICKNESS;

   for (int i = 0; i < thickness; i++) {
      *output << "2 2 0 1 " << shapeColor << " " << shapeDepth <<
	     " 0 21 0.000 0 0 0" << endl << "\t" <<
	     shapeBoundingBox.x1+2 << " " <<
	     shapeBoundingBox.y1+2 << " " << shapeBoundingBox.x1+2<< " " <<
	     shapeBoundingBox.y2-2 << " " << shapeBoundingBox.x2-2 << " " <<
	     shapeBoundingBox.y2-2 << " " << shapeBoundingBox.x2-2 << " " <<
	     shapeBoundingBox.y1+2 << " " << shapeBoundingBox.x1+2 << " " <<
	     shapeBoundingBox.y1+2 <<" 9999 9999" << endl;
      tempBoundingBox = tempBoundingBox.create_box_smaller_by_one(); 
   }
   *output  << endl;
}

 
XfigText::XfigText(MLCOStream *output, const MString& string,
		   XFontStruct *fontInformation, int color, int depth) 
: XfigShape(output, color, depth)
{
   fontInfo = fontInformation;
   textString = string;
}


void XfigText::draw(const ShapeBoundingBox& shapeBoundingBox)
{
   // find the maximal string that will fit in the bounding box.
   int len = 0;
   int width = 0;

   // if there is no font info, this means that xfig is running on a
   // machine that does not have an x display.  As a result, it just
   // appoximates the size of the characters by assuming a fixed font.
   if (fontInfo) 
      while (len < textString.length() &&
	     width <= shapeBoundingBox.x2-shapeBoundingBox.x1) 
	 width = XTextWidth(fontInfo, textString, ++len);
   else 
      len = min(textString.length(),
		(shapeBoundingBox.x2 - shapeBoundingBox.x1) /
		CHAR_WIDTH);
   
   // even if their space overlaps slightly, it makes sense to at
   // least put the first character of the string into the diagram.
   len = max(len, 1);
   MString shortenedString(textString, len);

   
   // now center the truncated string in the given bounding box
   int x = shapeBoundingBox.x1 +
      (shapeBoundingBox.x2-shapeBoundingBox.x1-width+1)/2;
   
   // Note that the ^A is used by Xfig to denote the end of the text
   // string.
   *output  << "#Text: " << textString << endl;
   *output  << "4 0 0 12 2  " << shapeColor << " " << shapeDepth
	  << " 0.000 0 8 " <<
	  shapeBoundingBox.x2-shapeBoundingBox.x1 << " " << 
	  x+1 << " " << ((shapeBoundingBox.y1 + shapeBoundingBox.y2)/2+2)
	  << " " << shortenedString << "" << endl << endl;
}






