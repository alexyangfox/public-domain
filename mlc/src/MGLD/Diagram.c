// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : This file provides support for creating and drawing
                   a graph.  The user may then add Shapes to the
		   diagram.  There are also methods to add grid lines
		   and to add attribute values along the axes. 
		 Whenever the refresh callback is invoked all of the
		   shapes are redrawn, along with the grid and the
		   labels along the axes. 
  Assumptions  : 
  Comments     : If no routines are called to draw labels on the axes,
		   they are left blank.
  Complexity   : Routines to create and draw gridlines are O(rows +
                   columns). 
		 Drawing the attributes is O(total number of
		   attributes * the time it takes to draw text).
		 All other drawing routines (including clear_diagram()
		   and refresh()) are O(rows*columns). 
		 Initialization is O(rows*columns).
  Enhancements : One thing that is tricky is the placement of labels
                   along the x and y axes.  It is important for the
		   readability of GLDs that this be readable and as
		   descriptive as possible.  Two techniques already
		   implemented are:
		     (1)  The text object truncates the amount of text
		            that it draws to print in the given box.  
		     (2)  In the routines which draw these,
		            the maximal space is given for the text
		            to draw itself.
		   However, further enhancements are possible:
		     (3)  Text could be drawn vertically where it has
		            a greater available vertical component.
		     (4)  The margins could be adjusted
		            for the number of rows/columns of labels and
		            the space that those rows/columns need.
		     (5)  Different fonts could be used depending on
		            the size of the text and the available space.
		 The ability to swap attributes is important.  Since
		   all of the information necessary is available in
		   Diagram.c, it could be done here, however, it makes
		   more sense functionality-wise to pass the information
		   up to the DiagramManager that two attributes have
		   been clicked.  In any event, the user interface for
		   this involves some work--either making the attributes
		   into widgets, which, because of the way the widgets
		   appear, could be a cosmetic blemish, or doing the
		   hit resolution manually (which shouldn't be very
		   hard).  Also, I think that the easiest thing to do
		   at the DiagramManager level is to free the current
		   Diagram and draw an entirely new one. 
		 Printing a Diagram could be provided by
		   translating the shapes into an Xfig file.
		   Alternatively, it is always possible to dump the
		   window via xv. A third option is to translate the
		   information directly to postscript.  However, the
		   drawback of this method is that there wouldn't be a
		   way to edit the shapes via a drawing program.
		   Also, it should be noted that if an Xfig file is
		   generated, postscript can be created from the Xfig
		   file.
  History      : Dave Manley                                      10/26/93
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <Diagram.h>
#include <DisplayMngr.h>

RCSID("MLC++, $RCSfile: Diagram.c,v $ $Revision: 1.6 $")

// The attribute depth is the maximal number of rows or columns of
// attributes that are allowed in a diagram.
const int DEFAULT_MAX_ATTRIBUTE_DEPTH = 20;



/***************************************************************************
    Description :  Copy constructor which returns an error.
    Comments    :  There are no NO_COPY_CTORx where x>5.
		   This method is not covered by t_Diagram.c.
		     This is because it is impossible to test a private
		     method that is never called by a public method.
          	   Private member.
***************************************************************************/
DiagramDrawingArea::DiagramDrawingArea(const DiagramDrawingArea& /*dda*/)
:shapes(0,0,NULL),
 markings(0,0,NULL),
 rowGridLines(0), columnGridLines(0),
 rowAttributes(0, 0, NULL),    
 columnAttributes(0, 0, 0), 
 maxAttributeDepth(0) 
{
   err << "No copy constructor for DiagramDrawingArea" << fatal_error;
}


/***************************************************************************
    Description :  This routine draws the grid lines stored by the Diagram.
    Comments    :  Should be called on redraws.
                   The time complexity is O(rows*columns).
		   Private member.
***************************************************************************/
void DiagramDrawingArea::draw_grid_lines() const
{
   // draw row grid lines
   for (int i = 0; i <= numRows; i++) {
	 ShapeBoundingBox shapeBoundingBox(horizontalDrawingMargin,
			     verticalDrawingMargin + i * yInterval,
			     horizontalDrawingMargin + numColumns * xInterval,
    			     verticalDrawingMargin + i * yInterval);
	 if (rowGridLines[i])
	    rowGridLines[i] -> draw(shapeBoundingBox);
   }

   // draw column grid lines
   for (i = 0; i <=numColumns; i++) {
      ShapeBoundingBox shapeBoundingBox(horizontalDrawingMargin+i*xInterval,
				 verticalDrawingMargin,
				 horizontalDrawingMargin+i*xInterval,
				 verticalDrawingMargin + numRows * yInterval);
      if (columnGridLines[i])
	 columnGridLines[i] -> draw(shapeBoundingBox);
   }
}

  
/***************************************************************************
    Description :  This routine draws the attributes on the X and Y axes.
    Comments    :  Should be called on redraws.
                   Although it may not look like it, this code is
		     significantly simplified by the assumption that
		     text knows how to fit itself inside of a bounding
		     box. 
                   The time complexity is O(total number of attributes
		     * the time it takes to draw text).  
		   Protected member.
***************************************************************************/
void DiagramDrawingArea::draw_attributes() const
{
   // draw column attributes
   if (numAttributeColumns) {
      int xOffset = horizontalDrawingMargin / numAttributeColumns;
      for (int i = 0; i < numAttributeColumns; i++) {
	 int yOffset = yInterval * numRows /
	    (columnAttributes[i]->size());
	 for (int j = 0; j < columnAttributes[i]->size(); j++) {
	    ShapeBoundingBox sBBox(horizontalDrawingMargin - (i+1) * xOffset, 
				   verticalDrawingMargin + j * yOffset + 1,
				   horizontalDrawingMargin - i * xOffset,
				   verticalDrawingMargin + (j+1)*yOffset + 1);
	    columnAttributes[i] -> index(j) -> draw(sBBox);
	 }
      }
   }

   // draw row attributes
   if (numAttributeRows) {
      int startY = verticalDrawingMargin + numRows * yInterval;
      int yOffset = verticalDrawingMargin / numAttributeRows;
      for (int i = 0; i < numAttributeRows; i++) {
	 int xOffset = xInterval * numColumns / (rowAttributes[i]->size());
	 for (int j = 0; j < rowAttributes[i]->size(); j++) {
	    ShapeBoundingBox sBBox(horizontalDrawingMargin+ j*xOffset,
				   startY + yOffset * i,
				   horizontalDrawingMargin+(j+1) * xOffset + 1,
				   startY + yOffset * (i+1));
	    rowAttributes[i] -> index(j) -> draw(sBBox);
	 }
      }
   }
}


/***************************************************************************
    Description :  This routine draws the shapes in the grid.
    Comments    :  Should be called on redraws.
		   The time complexity is O(total shapes in the grid
		     * the time it takes to draw text).  
		   Protected member.
***************************************************************************/
void DiagramDrawingArea::draw_shapes() const
{
   for (int i = 0; i < numRows; i++)
      for (int j = 0; j < numColumns; j++) {
	 ShapeBoundingBox sBBox(horizontalDrawingMargin + j * xInterval + 1,
				verticalDrawingMargin + i * yInterval + 1 ,
				horizontalDrawingMargin + (j+1)*xInterval - 1 ,
				verticalDrawingMargin+(i+1) * yInterval - 1);
	 if (shapes(i,j))
	    (shapes(i,j))->draw(sBBox);
	 if (markings(i,j))
	    (markings(i,j))->draw(sBBox);
      }
}


/***************************************************************************
    Description :  Initialize the diagram with no shapes, attributes,
                     or grid lines. 
    Comments    :  
***************************************************************************/
DiagramDrawingArea::DiagramDrawingArea(int numOfRows,
				       int numOfColumns,
				       int horizontalPixels,
				       int verticalPixels,
				       int horizontalMargin,
				       int verticalMargin,
				       int maximumAttributeDepth)	       
:shapes(numOfRows,numOfColumns,NULL),
 markings(numOfRows,numOfColumns,NULL),
 // note that the 0 cannot be removed--the constructor will not be
 // Array(size, default).  It will be mistaken for the Array(low, high).
 rowGridLines(0, numOfRows + 1, NULL),
 columnGridLines(0, numOfColumns + 1, NULL),
 rowAttributes(0, maximumAttributeDepth, NULL),    
 columnAttributes(0, maximumAttributeDepth, NULL), 
 maxAttributeDepth(maximumAttributeDepth) 
{
   numRows = numOfRows;
   numColumns = numOfColumns;
   numAttributeRows = 0;
   numAttributeColumns = 0;

   horizontalDrawingMargin = horizontalMargin;
   verticalDrawingMargin = verticalMargin;
      
   xPixels = horizontalPixels + horizontalDrawingMargin*2;
   yPixels = verticalPixels + verticalDrawingMargin*2;
   
   xInterval = horizontalPixels / numColumns;
   yInterval = verticalPixels / numRows;
   
   clear_diagram();
}


/***************************************************************************
    Description :  Set the display manager to be used by the Diagram.
    Comments    :  DiagramManager is expected to set this to the appropriate
                     type of DisplayManager.
***************************************************************************/
void DiagramDrawingArea::set_display_manager(DisplayManager *dispManager)
{
   displayManager = dispManager;
}


/***************************************************************************
    Description :  Add in a column of attributes
    Comments    :  
***************************************************************************/
void DiagramDrawingArea::add_attribute_column(ShapePtrArray *labelColumn)
{
   if (numAttributeColumns >= maxAttributeDepth)
      err<<"DiagramDrawingArea::add_attribute_column:"
	 "Attempt to exceed maximum number of attribute columns with column "
	 << maxAttributeDepth+1 << fatal_error;
   
   columnAttributes[numAttributeColumns] = labelColumn;
   numAttributeColumns++;
}


/***************************************************************************
    Description :  Add in a row of attributes
    Comments    :  
***************************************************************************/
void DiagramDrawingArea::add_attribute_row(ShapePtrArray *labelRow)
{
   if (numAttributeRows >= maxAttributeDepth)
      err<<"DiagramDrawingArea::add_attribute_row():"
	 "Attempt to exceed maximum number of attribute rows with row "
	 << maxAttributeDepth+1 << fatal_error;
   
   rowAttributes[numAttributeRows] = labelRow;
   numAttributeRows++;	 
}


/***************************************************************************
    Description :  This places the shape into a specific grid
                     position. 
    Comments    :  Note that there is only room for one shape in a
                     given grid position.
		   The current decision is to print out the case when
		     a shape is added to a position that already has
		     a shape because it is useful in catching bugs.
***************************************************************************/
void DiagramDrawingArea::add_shape_to_grid_position(Shape *shape,
						    int row,
						    int column)
{
   DBG(if (row >= numRows || row < 0)
       err<<"DiagramDrawingArea::add_shape_to_grid_position: Row position: "
       << row << " does not exist.  Legal values range from 0 to "
       << numRows-1 << fatal_error);
   
   DBG(if (column >= numColumns || column < 0)
       err<<"DiagramDrawingArea::add_shape_to_grid_position: Column position: "
       << column << " does not exist.  Legal values range from 0 to "
       << numColumns-1 << fatal_error);

   if (shapes(row, column) != NULL && shapes(row,column) != shape)
      Mcout << "!! Adding a shape to position (" << row << "," <<
	     column << ") which already has a shape!! " << endl;
   
   shapes(row,column) = shape;
}


/***************************************************************************
    Description :  This adds a second shape to a grid position.
    Comments    :  Useful for PredictedTest and PredictedTrain
		     diagrams.
		   The current decision is to print out the case when
		     a shape is added to a position that already has
		     a shape because it is useful in catching bugs.
***************************************************************************/
void DiagramDrawingArea::add_marking_to_grid_position(Shape *marking,
						      int row,
						      int column)
{
   DBG(if (row >= numRows || row < 0)
       err<<"DiagramDrawingArea::add_marking_to_grid_position: Row position: "
       << row << " does not exist.  Legal values range from 0 to "
       << numRows-1 << fatal_error);
   
   DBG(if (column >= numColumns || column < 0)
       err<<"DiagramDrawingArea::add_marking_to_grid_position:"
       << "Column position: " << column <<
       " does not exist.  Legal values range from 0 to " <<
       numColumns-1 << fatal_error);

   if (markings(row, column) != NULL && markings(row,column) != marking)
      Mcout << "!! Adding a marking to position (" << row << "," <<
	     column << ") which already has a marking!! " << endl;
   
   markings(row,column) = marking;
}


/***************************************************************************
    Description :  Creates a grid with the specified number of rows
                     and columns.  This can be erased (along with the
		     shapes) in ClearDiagram().
		   The default grid only has lines of a single (default)
		     pixel width.
    Comments    :  
***************************************************************************/
void DiagramDrawingArea::create_default_grid()
{
   // need to do one more than the numRows since x+1 lines make x rows
   for (int i = 0; i <= numRows; i++)
      rowGridLines[i] = displayManager->get_line(0);
   
   for (i = 0; i <= numColumns; i++) 
      columnGridLines[i] = displayManager->get_line(0);
}


/***************************************************************************
    Description :  Add the given grid line to the requested position.
    Comments    :  
***************************************************************************/
void DiagramDrawingArea::add_row_grid_line(Shape *shape,
					   int row)
{
   DBG(if ((row > numRows) || (row < 0))
       err<<"DiagramDrawingArea::add_row_grid_line: Row position: "
       << row << " does not exist.  Legal values range from 0 to "
       << numRows << fatal_error);
   
   rowGridLines[row] = shape;
}

void DiagramDrawingArea::add_column_grid_line(Shape *shape,
					      int column)
{
   DBG(if (column > numColumns || column < 0)
       err<<"DiagramDrawingArea::add_column_grid_line: Column position: "
       << column << " does not exist.  Legal values range from 0 to "
       << numColumns << fatal_error);

   columnGridLines[column] = shape;
}


/***************************************************************************
    Description :  This method removes all of the shapes from the
                     grid and puts in the bounding box shape.
		   It doesn't free the shapes in the array because
		     allocating the shapes is not its responsibility.
		     One can imagine the client of the code putting
		     the shapes upon the stack and only passing in a
		     pointer. 		     
    Comments    :  
***************************************************************************/
void DiagramDrawingArea::clear_diagram()
{
   for (int i = 0; i < numRows; i++)
      for (int j = 0; j < numColumns; j++) {
	 markings(i,j) = NULL;
	 shapes(i,j) = NULL;
      }
} 


/***************************************************************************
    Description :  Draw everything (including the shapes, grid lines,
                     and attributes).
    Comments    :  
***************************************************************************/
void DiagramDrawingArea::draw() const
{
   draw_attributes();
   draw_shapes();
   draw_grid_lines();
}


/***************************************************************************
    Description :  Return the drawing margins.
    Comments    :  
***************************************************************************/
int DiagramDrawingArea::get_vertical_drawing_margin() const
{
   return verticalDrawingMargin;
}

int DiagramDrawingArea::get_horizontal_drawing_margin() const
{
   return horizontalDrawingMargin;
}



