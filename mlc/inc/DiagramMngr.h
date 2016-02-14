// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _DiagramMngr_h
#define _DiagramMngr_h 1

#include <Diagram.h>
#include <InstanceRC.h>
#include <LogOptions.h>
#include <GLDPref.h>

class DisplayManager;

typedef PtrArray<PtrArray<PtrArray<Shape *> *> *> ShapePtrArrayArrayArray;

// Record to keep track of all of the positioning information
// concerning a single attribute's position relative to the GLD. 
struct AttrPositionRec {
private:
   NO_COPY_CTOR(AttrPositionRec);

public:
   // we need this constructor because there is a copy constructor,
   // meaning that the default constructor is no longer given for
   // free. 
   AttrPositionRec() {}
   
   // an enumerated type with vertical and horizontal
   // as the two options
   AttrOrientation orientation;
   int position;

   // product of inner attributes number of values--useful for determing
   // the offset along the appropriate axis.  (The offset due to a
   // single attribute is the multiplier * the attribute's number.)
   int multiplier;

   // useful when recalculating multipliers after an attribute has
   // been swapped with another
   int numAttrValues;
};

// AttrPositionInfo keeps a pointer to the DiagramManager that created
// it.  Hence the forward reference.  Note that the two classes cannot
// simply be switched in order to avoid this since the DiagramManager
// contains an AttrPositionInfo.
class DiagramManager;

class AttrPositionInfo {
   NO_COPY_CTOR(AttrPositionInfo);
   LOG_OPTIONS;
   Array<int> axesCount;     // number of attributes along each axis
   Array<int> totalSquares;  // total squares needed horizontally
			     // and vertically
   Array<AttrPositionRec> attrPositionArray;
   DiagramManager *diagramManager;

protected:
   virtual void calculate_multipliers_along_axis(const AttrOrientation
						 orientation);
   virtual void calculate_multipliers();
   virtual void find_dividing_point(const SchemaRC schema,
				    int& dividingCount,
				    Array<AttrOrientation>& orientationOrder);

public:
   // constructor can be overridden in a subclass to have a
   // different heuristic for arranging attributes along axes.
   AttrPositionInfo(const SchemaRC schema,
		    DiagramManager *diagramManager,
		    GLDPref *gldPreference);

   virtual int num_attributes_along_axis(const AttrOrientation orientation);
   virtual int total_squares_along_axis(const AttrOrientation orientation);
   virtual void XY_offset(int attrNum, int value, int& Xoffset,
			  int& Yoffset);
   
   virtual void create_attribute_names(const SchemaRC schema,
				       ShapePtrArrayArrayArray&
				       attrShapeArrays);
   virtual int get_index(const AttrOrientation orientation,
			 int position);
   
   // this is all that needs to be done (besides a redraw), to deal
   // with the user switching two attributes.
   virtual void swap_attributes(const AttrOrientation orientation1,
				int position1,
				const AttrOrientation orientation2,
				int position2);
};

class GLDPref;

class DiagramManager {
   NO_COPY_CTOR(DiagramManager);
   LOG_OPTIONS;
   const SchemaRC schema;
   DisplayManager *displayManager;
   DiagramDrawingArea *diagram;
   Bool done; 
   ShapePtrArrayArrayArray attrShapeArrays;
   // Menu *menu;   --not implemented
   // Key *key;     --not implemented
   AttrPositionInfo attrPositionInfo;
   GLDPref* gldPreference;

protected:
   virtual void init_grid();
   virtual void init_diagram(const SchemaRC schema);
   virtual void init_display_manager(int verticalPixels,
				     int horizontalPixels);

public:
   DiagramManager(const SchemaRC schema, GLDPref *gldPreference);
   virtual ~DiagramManager();

   DisplayManager *get_display_manager();
   GLDPref *get_gld_preference();

   virtual void add_instance_marking(const InstanceRC instance, Shape *shape);
   virtual void add_instance(const InstanceRC instance, Shape *shape);
   virtual void swap_attributes(const AttrOrientation orientation1, 
                                int position1,
				const AttrOrientation orientation2, 
				int position2);
   virtual void quit();
   virtual void run();
};

#endif
