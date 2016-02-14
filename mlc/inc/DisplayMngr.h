// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

#ifndef _DisplayMngr_h
#define _DisplayMngr_h 1

#include <DiagramMngr.h>

class DisplayManager {
   NO_COPY_CTOR(DisplayManager);
protected:
   GLDPref *gldPreference;
   DiagramManager *diagramManager;
   DiagramDrawingArea *diagramDrawingArea;
   PtrArray<Shape *> shapeArray;
   PtrArray<Shape *> colorSquareArray;
   PtrArray<Shape *> lineArray;
   Shape* boundingBox;
   
public:
   DisplayManager();
   DisplayManager(DiagramManager *diagramManager,
		  DiagramDrawingArea *diagramDrawingArea);
   virtual ~DisplayManager() { delete boundingBox; }
   virtual void initialize_display() = 0;
   virtual Shape *create_string(const MString& string) = 0;
   virtual void quit() = 0;
   virtual void run() = 0;
   void set_diagram_drawing_area(DiagramDrawingArea* dda);
   DiagramDrawingArea *get_diagram_drawing_area();
   virtual Shape *get_bounding_box();
   virtual Shape *get_shape(int shapeNum);
   virtual Shape *get_color_square(int shapeNum);
   virtual Shape *get_line(int thickness = 0);
};


class MotifDisplayManager : public DisplayManager {
   NO_COPY_CTOR(MotifDisplayManager);
   Widget toplevel;
   Widget canvas;
   Widget container;
   Widget quitButton;
   Widget quitBB;
   
   XtAppContext app;
   GC motifDisplayManagerGC;
   
   int horizontalPixels;
   int verticalPixels;

   Bool done;

protected:
   virtual void initialize_shapes();
   virtual void initialize_lines();
   virtual void create_application(int verticalPixels,
				   int horizontalPixels);
   virtual void create_canvas();
   virtual void add_callbacks();

   virtual GC get_manager_GC() const { return motifDisplayManagerGC; }
   virtual Widget get_drawing_widget() const { return canvas; }

public:
   MotifDisplayManager(DiagramManager *diagramManager,
		       DiagramDrawingArea *diagramDrawingArea,
		       int verticalPixels,
		       int horizontalPixels);
   ~MotifDisplayManager();
   void initialize_display() {}
   void run();
   Shape *create_string(const MString& newString);
   void quit();
};


class XfigDisplayManager : public DisplayManager {
   NO_COPY_CTOR(XfigDisplayManager);

   int horizontalPixels;
   int verticalPixels;

   Widget toplevel;
   XFontStruct *fontInfo;
   XtAppContext app;
   Display *display;
   MLCOStream *mlcOStream;
   
protected:
   virtual void initialize_shapes();
   virtual void initialize_lines();
   
public:
   XfigDisplayManager(DiagramManager *diagramManager,
		      DiagramDrawingArea *diagramDrawingArea,
		      int verticalPixels,
		      int horizontalPixels);
   ~XfigDisplayManager();
   void initialize_display();
   void run();
   Shape *create_string(const MString& newString);
   void quit();
};



#endif
