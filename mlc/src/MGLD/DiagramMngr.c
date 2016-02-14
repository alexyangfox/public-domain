// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : This file contains the DiagramManager and the
                   AttrPositionInfo classes.  There is a separate
		   header below for the DiagramManager class. 
		 AttrPositionInfo is a helper class for
		   DiagramManager--its sole purpose is to keep track
		   of the attributes, which axis they are along, and
		   figure out grid spacing. 
  Assumptions  : 
  Comments     : This class is independant of what it draws on; in
                   other words, none of the routines are specific to X
		   or Motif.
		 The way in which attributes are initially arranged is
		   that a break point is found in the graph such that
		   the size of the graph is as close to a square as
		   possible.  The longer side is then swapped to the
		   horizontal axis to take advantadge of the fact that
		   displays are usually wider then they are tall.
  Complexity   : The following routines are all O(number of attributes):
                   AttrPositionInfo()
                   calculate_multipliers_along_axis()
		   calculate_multipliers()
		   create_attribute_names()
		   get_index()
  Enhancements : Construct a preferences class that can be passed in
                   to the constructor.
		 To allow the user to perform interactive graphical
		   manipulation, stubs must be provided to
		   the DiagramManager to deal with the swapping of
		   attributes.
		 To provide better heuristic for AttrPositionInfo()
		   which allows for nicer arranging of attributes.
  History      : Dave Manley                                      12/2/93
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <math.h>
#include <Diagram.h>
#include <DiagramMngr.h>
#include <DisplayMngr.h>
#include <GLDPref.h>

RCSID("MLC++, $RCSfile: DiagramMngr.c,v $ $Revision: 1.9 $")

const int NUM_AXES = 2;

/***************************************************************************
    Description :  Calculates and stores all of the multipliers along the
                     specified axis.
    Comments    :  Protected member.
***************************************************************************/
void AttrPositionInfo::calculate_multipliers_along_axis(const AttrOrientation
							orientation)
{
      int multiplier = 1;
      for (int attrNum = 0; attrNum < axesCount[orientation]; attrNum++) {
	 int currAttr = get_index(orientation, attrNum);
	 attrPositionArray[currAttr].multiplier = multiplier;
	 multiplier *= attrPositionArray[currAttr].numAttrValues;
      }
      totalSquares[orientation] = multiplier;  
}


/***************************************************************************
    Description :  Calculates the multipliers for all attributes.
                   Note that the numAttrValues, orientation, and
		     position all need to be filled in. 
    Comments    :  This needs to be called on callbacks which swap
                     attributes.
***************************************************************************/
void AttrPositionInfo::calculate_multipliers()
{
   calculate_multipliers_along_axis(HorizontalAttr);
   calculate_multipliers_along_axis(VerticalAttr);
}


/***************************************************************************
    Description :  The dividing point mentioned here is just the break
                     between putting attributes on the vertical and
		     horizontal axes.  In other words,  attributes are
		     added in order to one axis, until the dividing
		     point.  The remaining attributes are then added
		     to the other axis. 
                   The algorithm this method uses to find this point is
		     basic; attributes are added along one
		     axis so that the total product of the number of
		     attribute values on each side is as close to
		     equal as possible. 
		   At that point, the cofiguration closest to a square
		     is chosen.  The longer side is then placed on the
		     horizontal axis. 
		   This information is then returned via the
		     dividingCount and the orientationOrder parameters.
    Comments    :  Protected member.
***************************************************************************/
void AttrPositionInfo::find_dividing_point(const SchemaRC schema,
					   int& dividingCount,
					   Array<AttrOrientation>&
					   orientationOrder)
{
   int totalProduct = 1;
   int currentProduct = 1;
   int count = 0;
   
   axesCount[HorizontalAttr] = 0;
   axesCount[VerticalAttr] = 0;
   orientationOrder[HorizontalAttr] = HorizontalAttr; 
   orientationOrder[VerticalAttr] = VerticalAttr;

   // get the total product; useful in finding the dividing point
   for (int attrNum = 0; attrNum < schema.num_attr(); attrNum++) {
      const NominalAttrInfo& nai = schema.nominal_attr_info(attrNum);
      totalProduct *= nai.num_values();
   }

   // find the dividing point
   Bool found = FALSE;
   dividingCount = 0;
   int nextProduct;
   for (attrNum = 0; attrNum < schema.num_attr() && !found; attrNum++) {
      const NominalAttrInfo& nai = schema.nominal_attr_info(attrNum);
      
      nextProduct = currentProduct * nai.num_values();
      ASSERT (totalProduct%currentProduct == 0);
      ASSERT (totalProduct%nextProduct == 0);
      if (abs(currentProduct - (int)totalProduct/currentProduct) >=
	  abs(nextProduct - (int)totalProduct/nextProduct)) {
	 currentProduct = nextProduct;
	 count++;  
      }
      else {
	 found = TRUE;
	 dividingCount = count;
      }
   }

   // if not already, make horizontal side the longer of the two.
   if (currentProduct <= totalProduct) {
      orientationOrder[HorizontalAttr] = VerticalAttr;
      orientationOrder[VerticalAttr] = HorizontalAttr;
   }
}


/***************************************************************************
    Description :  This constructor arranges the attributes for the initial
                     GLD diagram.  The default constructor calls
		     find_dividing_point and puts all of the
		     attributes up to the dividing point on the first
		     axis specified by the orientationOrder, and the
		     rest on the second specified axis.  
    Comments    :  A more intelligent AttrPositionInfo could inherit
                     from this one, as this has the required data
		     structures to keep track of any setup.
***************************************************************************/
AttrPositionInfo::AttrPositionInfo(const SchemaRC schema,
				   DiagramManager *diagramMngr,
				   GLDPref *gldPreference)
:attrPositionArray(schema.num_attr()),
 axesCount(NUM_AXES),
 totalSquares(NUM_AXES)
{
   int dividingCount = 0;
   Array<AttrOrientation> orientationOrder(NUM_AXES);

   find_dividing_point(schema, dividingCount, orientationOrder);
   
   int count = 0;
   
   for (int attrNum = 0; attrNum < schema.num_attr(); attrNum++) {
      const NominalAttrInfo& nai = schema.nominal_attr_info(attrNum);
      attrPositionArray[count].numAttrValues = nai.num_values();

      // case that the user has requested a specific order.
      if (gldPreference->positions_specified()) {
	 attrPositionArray[count].position =
	    gldPreference->get_position(count);
   	 attrPositionArray[count].orientation =
	    gldPreference->get_orientation(count);
	 axesCount[gldPreference->get_orientation(count)]++;
      }
      // default case where an reasonable ordering is created.
      else {
	 if (count < dividingCount) {
	    attrPositionArray[count].orientation =
	       orientationOrder[HorizontalAttr];
	 }
	 else { 
	    attrPositionArray[count].orientation =
	       orientationOrder[VerticalAttr];
	 }
	 attrPositionArray[count].position =
	    axesCount[attrPositionArray[count].orientation]++;
      }
      count ++;
   }

   ASSERT (count == schema.num_attr());
   
   calculate_multipliers();
   diagramManager = diagramMngr;
}

   
/***************************************************************************
    Description :  Returns total attributes along either of the axes,
                     according to user specification.
    Comments    :  
***************************************************************************/
int AttrPositionInfo::num_attributes_along_axis(const AttrOrientation
						orientation)
{
   return axesCount[orientation];
}


/***************************************************************************
    Description :  Returns total vertical or horizontal squares,
                     according to user's specified orientation.
    Comments    :  
***************************************************************************/
int AttrPositionInfo::total_squares_along_axis(const AttrOrientation
					       orientation)
{
   return totalSquares[orientation];
}


/***************************************************************************
    Description :  Calculate the indices along each axis given an 
                     attribute and a value.  The offsets returned along
		     the x and y axes, respectively, are found by
		     multiplying the multiplier by value. 
    Comments    :  This depends on calculate_multipliers() having
                     already been called with the current attribute
		     setup. 
***************************************************************************/
void AttrPositionInfo::XY_offset(int attrNum, int value,
				 int& Xoffset, int& Yoffset)
{
   Xoffset = 0; Yoffset = 0;
   
   ASSERT(value >= FIRST_CATEGORY_VAL &&
	  value <= attrPositionArray[attrNum].numAttrValues
	  + FIRST_CATEGORY_VAL);
   
   if (attrPositionArray[attrNum].orientation == HorizontalAttr)
      Xoffset = (value - FIRST_CATEGORY_VAL)
	 * attrPositionArray[attrNum].multiplier;
   else
      Yoffset = (value - FIRST_CATEGORY_VAL)
	 * attrPositionArray[attrNum].multiplier;      
}


/***************************************************************************
    Description : This creates the arrays of names (implemented as
                    text shapes) which are later sent to the Diagram as
		    vertical and horizontal axes labels.  To
		    place these attributes into the correct format:
		      (1) the ordering along the x and y axes must be
		        setup.
		      (2) names must be repeated as necessary.
    Comments    : 
***************************************************************************/
void AttrPositionInfo::create_attribute_names(const SchemaRC schema,
           	   ShapePtrArrayArrayArray& attrShapeArrays)
{
   Array<int> totalSquares(NUM_AXES);
   totalSquares[VerticalAttr] = total_squares_along_axis(VerticalAttr);
   totalSquares[HorizontalAttr] = total_squares_along_axis(HorizontalAttr);
   int repetitions = 0;
   int pos = 0;
   AttrOrientation orientation;
 
   for (int count = 0; count < schema.num_attr();  count++) {
      const NominalAttrInfo& nai = schema.nominal_attr_info(count);

      AttrPositionRec& attrPositionRec = attrPositionArray[count];
      
      // make sure that the attrPositionArray has already been set up.
      ASSERT (nai.num_values() == attrPositionRec.numAttrValues);

      pos = attrPositionRec.position;
      orientation = attrPositionRec.orientation;
      repetitions = totalSquares[orientation] /
	 (attrPositionRec.multiplier *
	  attrPositionRec.numAttrValues);
      
      attrShapeArrays[orientation]->index(pos)
	 = new PtrArray<Shape *>(nai.num_values()*repetitions);
      for (int j = 0; j < repetitions; j++) 
	 for (int i = FIRST_CATEGORY_VAL ;
	      i < FIRST_CATEGORY_VAL + nai.num_values();
	      i++) {
	    attrShapeArrays[orientation]->index(pos)->
	       index((i-FIRST_CATEGORY_VAL)+j*nai.num_values()) =
	       diagramManager->get_display_manager()
	       ->create_string(nai.get_value(i));
	 }
   }
   ASSERT (count == schema.num_attr());
}


/***************************************************************************
    Description :  Finds the index for a specific row and column into
                     the attrPositionArray.
    Comments    :  
***************************************************************************/
int AttrPositionInfo::get_index(const AttrOrientation orientation,
				int position)
{
   for (int i = 0; i < attrPositionArray.size(); i++)
      if (attrPositionArray[i].position == position &&
	  attrPositionArray[i].orientation == orientation)
	 return(i);
   
   err << "AttrPositionInfo::get_index found no record of the requested"
      " (orientation, position) pair of: (" << orientation <<
      ", " << position << ")" << fatal_error;

   return -1; // dummy to avoid warning.
}

/***************************************************************************
    Description :  Swaps the axis position of two attributes.
    Comments    :  Everything should be redrawn after a swap.
***************************************************************************/
void AttrPositionInfo::swap_attributes(const AttrOrientation orientation1,
				       int position1,
				       const AttrOrientation orientation2,
				       int position2) 
{
   int attrNum1 = get_index(orientation1, position1);
   int attrNum2 = get_index(orientation2, position2);
   swap(attrPositionArray[attrNum1].position,
	attrPositionArray[attrNum2].position);
   swap(attrPositionArray[attrNum1].orientation,
	attrPositionArray[attrNum2].orientation);
      
   calculate_multipliers();
}


/***************************************************************************
  Description  : DiagramManger is the second class in this module.
                   For general file information see initial header.
                 Middle layer between GLD and DiagramDrawingArea
                   classes.  Provides a very simple interface to a
		   GLD--the only things a GLD has to worry about
		   is providing a schema for the
		   constructor and adding in each instance.
		 The way each instance is added is that the desired
		   shape is passed along with the instance to
		   the add_instance method.
  Assumptions  : 
  Comments     : Can be used for environments other than X Windows.
                   However, the way it is structured now is to only
		   use it for X window purposes.  If it was to be
		   extended for other environments, then the
		   DiagramDrawingArea class would need to be
		   subclassed, since all DiagramManagers need a
		   DiagramDrawingArea. 
  Complexity   : The constructor for DiagramManager and init_diagram
		   are O(number of rows * number of columns).
		 add_instance() is O(number of attributes^2)
  Enhancements : Construct a preferences class that can be passed in
                   to the constructor.
		 To allow the user to perform interactive graphical
		   manipulation, routines  must be provided to
		   DiagramDrawingArea(which will be called by callbacks).
		   (An example of this would be a swap() method.)
***************************************************************************/



/***************************************************************************
    Description :  Initialize a display manager.
    Comments    :  This is the only section of the code that is
                     specific to the output format.  Once we create
		     the correct type of display manager, it doesn't
		     matter if we are working with Xfig or Motif
		     output.
		   Protected member.
***************************************************************************/
void DiagramManager::init_display_manager(int verticalPixels,
					  int horizontalPixels)
{
   DisplayManagerType displayManagerType =
      gldPreference->get_display_manager_type();
   
   if (displayManagerType == MotifDisplayManagerType) 
      displayManager = new MotifDisplayManager(this, diagram,
					       verticalPixels,
					       horizontalPixels);
   else if (displayManagerType == XfigDisplayManagerType) 
      displayManager = new XfigDisplayManager(this, diagram,
					      verticalPixels,
					      horizontalPixels);
   else 
      err << "DiagramManager::init_display_manager: the " <<
	 "DisplayManagerType of the GLDPref is " << displayManagerType <<
	 " which does not exist. " << fatal_error;

   diagram->set_display_manager(displayManager);
}


/***************************************************************************
    Description :  This routine sets up a grid with varying widths
                     which help divide different attribute values into
		     logical blocks. 
    Comments    :  Protected member.
***************************************************************************/
void DiagramManager::init_grid()
{
   int verticalSquares =
      attrPositionInfo.total_squares_along_axis(VerticalAttr);
   int horizontalSquares =
      attrPositionInfo.total_squares_along_axis(HorizontalAttr);

   int numHorizontalAttrs = 
      attrPositionInfo.num_attributes_along_axis(HorizontalAttr);
   int numVerticalAttrs =
      attrPositionInfo.num_attributes_along_axis(VerticalAttr); 

   int maxWidth = gldPreference->get_axes_line_width();
   
   for (int k = 0; k < numHorizontalAttrs; k++) {
      int lineWidth = max(1, maxWidth/(int)pow(2,numHorizontalAttrs-1-k));
      Shape *line = displayManager -> get_line(lineWidth);
      for (int i = 0; i <= horizontalSquares; i += (horizontalSquares /
	   attrShapeArrays[HorizontalAttr]->index(k)->size()))
	 diagram->add_column_grid_line(line, i);
   }
   
   for (k = 0; k < numVerticalAttrs; k++) {
      int lineWidth = max(1, maxWidth/(int)pow(2,numVerticalAttrs-1-k));
      Shape *line = displayManager -> get_line(lineWidth);
      for (int i = 0; i <= verticalSquares; i += (verticalSquares /
	   attrShapeArrays[VerticalAttr]->index(k)->size()))
	 diagram->add_row_grid_line(line, i);
   }
}
   

/***************************************************************************
    Description :  This routine is called whenever a new GLD needs to
                     be popped up.  This occurs not only at
		     initialization, but also when attributes are swapped. 
    Comments    :  The default is to make each grid entry the largest
		     possible square such that the diagram will have
		     dimensions less than or equal to the dimensions
		     given above.
		   Protected member.
***************************************************************************/
void DiagramManager::init_diagram(const SchemaRC schema)
{
   for (int i = 0; i < NUM_AXES; i++)
      attrShapeArrays[i] =
	 new PtrArray<PtrArray<Shape *> *>(schema.num_attr());

   int verticalSquares =
      attrPositionInfo.total_squares_along_axis(VerticalAttr);
   int horizontalSquares =
      attrPositionInfo.total_squares_along_axis(HorizontalAttr);

   int horizontalPix = gldPreference -> get_horizontal_pixels();

   int horizontalCellSize = horizontalPix / horizontalSquares;

   int verticalPix = gldPreference -> get_vertical_pixels();
   
   int verticalCellSize = verticalPix / verticalSquares;

   verticalCellSize = horizontalCellSize =
      min(verticalCellSize, horizontalCellSize);

   // Note that the DiagramDrawingArea needs to know the numbers of
   // squares and pixels so that it can correctly space out the
   // shapes. 
   diagram = new DiagramDrawingArea(verticalSquares,
				    horizontalSquares,
				    horizontalCellSize * horizontalSquares,
				    verticalCellSize * verticalSquares,
				    gldPreference -> get_horizontal_margin(),
				    gldPreference -> get_vertical_margin());

   // Note also that the DisplayManager also needs to know the number
   // of pixels so that it can create a XmDrawingArea of the correct
   // dimensions.
   init_display_manager(verticalCellSize * verticalSquares,
			horizontalCellSize * horizontalSquares);

   // This method takes care of creating arrays of the shapes for the
   // attribute values that are placed along the axes. 
   attrPositionInfo.create_attribute_names(schema, attrShapeArrays); 

   // These calls needs be be done after the DisplayManager is
   // initialized since it uses some of the DisplayManager's
   // resources.
   // Note also that we create the a default grid before calling
   // init_grid(), so that if init_grid() does not fill in any of the
   // lines, they will at least have a single width.
   diagram->create_default_grid();
   init_grid();

   Array<int> numAttrs(NUM_AXES);
   numAttrs[HorizontalAttr] =
      attrPositionInfo.num_attributes_along_axis(HorizontalAttr);
   numAttrs[VerticalAttr] =
      attrPositionInfo.num_attributes_along_axis(VerticalAttr); 

   // Here we add the rows and columns that were created above by the
   // AttrPositionInfo class to the DiagramDrawingArea.  
   for (int k = 0; k < numAttrs[HorizontalAttr]; k++) 
      diagram->add_attribute_row(attrShapeArrays[HorizontalAttr]
				 ->index(k));

   for (k = 0; k < numAttrs[VerticalAttr]; k++) 
      diagram->add_attribute_column(attrShapeArrays[VerticalAttr]
				    ->index(k));
}


/***************************************************************************
    Description :  The constructor for DiagramManager takes an
                     instance and produces the grid with the attribute 
		     values along the axes.  Note that a major part of
		     this work is done by the AttrPositionInfo class.
    Comments    :  The constructor's responsibility in accomplishing
                     the above also includes:
		        (1) creating the toplevel widget, and
			(2) realizing the top level widget after
		            the other widgets have been created.
		   As a sub-note with regards to step(2) above:
		     all widgets need to be managed at some point--(2)
		     isn't the only way to do this; it is convient to
		     do this since it insures that all widgets will
		     get realized.
***************************************************************************/
DiagramManager::DiagramManager(const SchemaRC aSchema, GLDPref *gldPref)
               : attrPositionInfo(aSchema, this, gldPref),
		 schema(aSchema),
		 attrShapeArrays(NUM_AXES)
{
   // set to TRUE only when quit() is called.
   done = FALSE;
   gldPreference = gldPref;

   init_diagram(schema);
}


/***************************************************************************
    Description :  Deletes diagram.
    Comments    :  The GC needs to be released before the display is
                     closed.   
***************************************************************************/
DiagramManager::~DiagramManager()
{
   delete diagram;
   delete displayManager;
}


/***************************************************************************
    Description : Returns the GLDPref for this DiagramManager. 
    Comments    : Note that while attributes in this object may
                    change the structure itself will remain the same.
		    As a result, a client may wish to call this
		    routine only once and then can keep the pointer to
		    this preference.
***************************************************************************/
GLDPref *DiagramManager::get_gld_preference()
{
   return gldPreference;
}


/***************************************************************************
    Description : Returns the DisplayManager for this DiagramManager. 
    Comments    :  
***************************************************************************/
DisplayManager *DiagramManager::get_display_manager()
{
   return displayManager;
}


/***************************************************************************
    Description :  Given a shape and an instance, this method adds the
                     shape to the corresponding grid position.
    Comments    :  Makes extesive use of the AttrPositionInfo class to
                     get the correct space given the attribute numbers
		     and values.
***************************************************************************/
void DiagramManager::add_instance(const InstanceRC instance, Shape *shape)
{
   // coordinates of grid to add the shape
   int column = 0;
   int row = 0;

   int count = 0;
   for (int attrNum = 0; attrNum <instance.get_schema().num_attr();attrNum++) {
      const NominalAttrInfo& nai = instance.get_schema().
                              	 nominal_attr_info(attrNum);

      if (nai.get_nominal_val(instance[attrNum]) == UNKNOWN_CATEGORY_VAL)
	 err << "DiagramManager::add_instance: Diagram Manager is not"
	    " equipped to deal with UNKNOWN_CATEGORY_VAL" << fatal_error;
      
      int Xoffset, Yoffset;
      attrPositionInfo.XY_offset(count,
				 nai.get_nominal_val(instance[attrNum]),
				 Xoffset, Yoffset);
      row += Yoffset;
      column += Xoffset;

      count++;
   }
   ASSERT(count == instance.get_schema().num_attr());
	  
   LOG(3,"DiagramManager::add_instance: adding a shape to " <<
       "position(" << row << "," << column<<")" << endl);

   diagram->add_shape_to_grid_position(shape,row,column);
}


/***************************************************************************
    Description :  Given a shape and an instance, this method adds the
                     shape to the corresponding grid position.
		   Note that it uses the marking array for these
		     shapes.  The only difference in the marking
		     is that it is drawn on top of the shape.
    Comments    :  Makes extesive use of the AttrPositionInfo class to
                     get the correct space given the attribute numbers
		     and values.
***************************************************************************/
void DiagramManager::add_instance_marking(const InstanceRC instance,
					  Shape *shape)
{
   // coordinates of grid to add the shape
   int column=0,row=0;

   for (int count = 0; count < instance.get_schema().num_attr(); count++) {

      const NominalAttrInfo& nai =
	 instance.get_schema().nominal_attr_info(count);

      if (nai.get_nominal_val(instance[count]) == UNKNOWN_CATEGORY_VAL)
	 err << "DiagramManager::add_instance: Diagram Manager is not"
	    " equipped to deal with UNKNOWN_CATEGORY_VAL" << fatal_error;
      
      int Xoffset, Yoffset;
      attrPositionInfo.XY_offset(count,
				 nai.get_nominal_val(instance[count]),
				 Xoffset, Yoffset);
      row += Yoffset;
      column += Xoffset;
   }
   ASSERT(count == instance.get_schema().num_attr());
	  
   LOG(3,"DiagramManager::add_instance_marking: adding a shape to " <<
       "position(" << row << "," << column<<")" << endl);

   diagram->add_marking_to_grid_position(shape,row,column);
}


/***************************************************************************
    Description :  This swaps the shape arrays (which contain the
                     names of the attribute values) for the specified
		     attributes.                      
    Comments    :  Also calls the swap() method for attrPositionInfo.
***************************************************************************/
void DiagramManager::swap_attributes(const AttrOrientation orientation1,
				     int position1,
				     const AttrOrientation orientation2,
				     int position2)
{
  swap(attrShapeArrays[orientation1]->index(position1),
       attrShapeArrays[orientation2]->index(position2));
   
  attrPositionInfo.swap_attributes(orientation1, position1,
				   orientation2, position2);
}


/***************************************************************************
    Description :  Sets a flag so that the main event loop knows to
                     exit.    		   
    Comments    :  
***************************************************************************/
void DiagramManager::quit()
{
   done = TRUE;
}


/***************************************************************************
    Description :  Go into the main loop of the Display Manager.
    Comments    :  Callbacks are now the only possible source of program
                     execution for this Diagram Manager.
***************************************************************************/
void DiagramManager::run()
{
   displayManager->run();
}
