// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _Diagram_h
#define _Diagram_h 1

#include <Array.h>
#include <Array2.h>
#include <Shape.h>

// since the DiagramDrawing area class only has a pointer to the
// DisplayManager, only a forward reference is needed.
class DisplayManager;

// We need lists of shapes for holding gridlines, attribute values,
// and shapes.  They are typedef'd for readability:

typedef Array<Shape *> ShapePtrArray;
typedef Array<ShapePtrArray *> ShapePtrArrayArray;
typedef Array2<Shape *> ShapePtr2dArray;

extern const int DEFAULT_MAX_ATTRIBUTE_DEPTH;

class DiagramDrawingArea {
   // Using copy constructor causes a fatal error. 
   DiagramDrawingArea(const DiagramDrawingArea& dda);

   // pointer to DisplayManager.  Note that DiagramManager is responsible
   // for setting this to the correct type of DisplayManager.
   DisplayManager *displayManager;
   
   // These are the number of pixels given to the entire
   // xmDrawingArea.  Thus, the number of pixels is the user specified
   // value for the GLD plus 2*drawingMargin. 
   int xPixels, yPixels;   // can't be const because of resizes.
   int horizontalDrawingMargin;
   int verticalDrawingMargin;

   const int maxAttributeDepth;
   int numRows, numColumns;
   int numAttributeRows, numAttributeColumns;

   // These instance variable do not contain any new information--just
   // convenient variables to store the size of a single square in
   // pixels. 
   int xInterval, yInterval;

   // We know that most likely all of these Shape *'s are Text *'s.
   // However, one could imagine using either shapes or icons as
   // attribute values in some cases.  Thus, we simply store Shape *'s
   // for flexibility.
   ShapePtrArrayArray rowAttributes; 
   ShapePtrArrayArray columnAttributes;

   // we might wish to assume that all of these Shape *'s will be
   // either Line *'s or inherited from Line *'s.  However, this would
   // again limit flexibility.  
   ShapePtrArray columnGridLines;      
   ShapePtrArray rowGridLines;
   
   // each grid square can have at most one shape.
   ShapePtr2dArray shapes;
   ShapePtr2dArray markings;

protected:
   virtual void draw_grid_lines() const;
   virtual void draw_attributes() const;
   virtual void draw_shapes() const;
   
public:
   DiagramDrawingArea(int numRows,
		      int numColumns,
		      int horizontalPixels,
		      int verticlePixels,
		      int horizontalMargin,
		      int verticalMargin,
		      int maximumAttributeDepth = DEFAULT_MAX_ATTRIBUTE_DEPTH);

   void set_display_manager(DisplayManager *displayManager);
   
   virtual void add_attribute_column(ShapePtrArray *attributeColumn);
   virtual void add_attribute_row(ShapePtrArray *attributeRow);
   virtual void add_shape_to_grid_position(Shape *shape,
					   int row,
					   int column);
   virtual void add_marking_to_grid_position(Shape *shape,
					     int row,
					     int column);

   virtual void create_default_grid(); 
   virtual void add_row_grid_line(Shape *shape, int row);
   virtual void add_column_grid_line(Shape *shape, int column);

   virtual void clear_labels() { numRows = 0; numColumns = 0; }
   virtual void clear_diagram();
   
   // this is called not only to draw the diagram the first time, but
   // also for refresh callbacks
   virtual void draw() const;

   int get_horizontal_drawing_margin() const;
   int get_vertical_drawing_margin() const;
};



#endif
