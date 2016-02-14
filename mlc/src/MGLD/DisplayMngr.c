// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : DisplayManager is the first class in this file.
                   Note that each class has its own class information. 
                 DisplayManager is an abstract base class that is
                   designed to provide an abstraction for all
		   graphical display details into a single programming
		   interface.  Any user interface that can provide the
		   virtual function calls of the abstract base class
		   can thus easily replace any other type of DisplayManager
		   in the code.
  Assumptions  :
  Comments     : The DisplayManager is engineered specifically to work with
                   a DiagramDrawingArea and a DiagramManager.  This is
		   because it needs to know where to direct callbacks and
		   requests for information that is not specific to
		   the DisplayManager.  For example, the
		   DisplayManager should not need to know how to deal
		   with two attributes being swapped, but must
		   register callbacks for this event.  As a result,
		   the callback is passed along to the DisplayManager.
  Complexity   :
  Enhancements :
  History      : Dave Manley                                       3/19/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <DisplayMngr.h>
#include <GLDPref.h>

// These includes are for the Motif Display manager.
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/PushBG.h>

RCSID("MLC++, $RCSfile: DisplayMngr.c,v $ $Revision: 1.6 $")

#define DEFAULT_WINDOW_NAME "MLC++ General Logic Diagram"

const int MARKING_SHAPE_DEPTH = 1;
const int NORMAL_SHAPE_DEPTH = 2;
const int RIGHT_MARGIN = 50;
const int TOP_MARGIN = 100;


/***************************************************************************
    Description :  Default constructor.
    Comments    :  
***************************************************************************/
DisplayManager::DisplayManager()
: shapeArray(NUMBER_OF_SHAPES),
  colorSquareArray(NUMBER_OF_SHAPES),
  lineArray(NUMBER_OF_LINE_WIDTHS)
{
}


/***************************************************************************
    Description :  Initializes data members common to all
                     DisplayManagers. 
    Comments    :  Note that DisplayManager is an abstract base class,
                     so an actual DisplayManager is never instantiated. 
***************************************************************************/
DisplayManager::DisplayManager(DiagramManager *dm,
			       DiagramDrawingArea *dda)
: shapeArray(NUMBER_OF_SHAPES),
  colorSquareArray(NUMBER_OF_SHAPES),
  lineArray(NUMBER_OF_LINE_WIDTHS)
{
   diagramManager = dm;
   diagramDrawingArea = dda;
   gldPreference = diagramManager -> get_gld_preference();
}


/***************************************************************************
    Description :  Sets the current DiagramDrawingArea.
    Comments    :  Note that one may already set in the constructor, so
                     unless if the DiagramDrawingArea changes, this
		     will never be called besides in construction.
***************************************************************************/
void DisplayManager::set_diagram_drawing_area(DiagramDrawingArea* dda)
{
   diagramDrawingArea = dda;
}


/***************************************************************************
    Description :  Returns the current DiagramDrawingArea.
    Comments    :  
***************************************************************************/
DiagramDrawingArea *DisplayManager::get_diagram_drawing_area()
{
   return diagramDrawingArea;
} 


/***************************************************************************
    Description :  Gets the indexed shape from the shape array.  
    Comments    :  The user of this routine is NOT responsible for freeing
                     the shape.  The DisplayManager is responsible for
		     storing and freeing all of the standard shapes.
***************************************************************************/
Shape *DisplayManager::get_shape(int shapeNum)
{
   if (shapeNum >= NUMBER_OF_SHAPES)
      err << "DisplayManager::get_shape: non-existant shape "  <<
	 shapeNum << " requested."  << fatal_error;
   return shapeArray[shapeNum];
}


/***************************************************************************
    Description :  Gets the indexed color square.
    Comments    :  The user of this routine is NOT responsible for freeing
                     the shape.  
***************************************************************************/
Shape *DisplayManager::get_color_square(int squareNum)
{
   if (squareNum >= NUMBER_OF_SHAPES)
      err << "DisplayManager::get_color_square: non-existant color square "  <<
	 squareNum << " requested."  << fatal_error;
   return colorSquareArray[squareNum];
}


/***************************************************************************
    Description :  Return a line of the requested thickness.
    Comments    :  The user of this routine should NOT free the line
                     that is returned. One benefit of using these
		     lines is tha many routines can share it. 
		   A line of thickness 0 is a default line and is a
		     the thinnest possible line.
***************************************************************************/
Shape *DisplayManager::get_line(int thickness)
{
   if (thickness >= NUMBER_OF_LINE_WIDTHS)
      err << "DisplayManager::get_line: this thickness  "  <<
	 thickness << " requested is not available."  << fatal_error;
   return lineArray[thickness];
}


/***************************************************************************
    Description :  Return a bounding box shape.
    Comments    :  The user of this routine should NOT free the
                     bounding box shape.
                   The thickness of a bounding box is itself variable,
		     making a thickness parameter unnecessary.   
***************************************************************************/
Shape *DisplayManager::get_bounding_box()
{
   return boundingBox;
}


/***************************************************************************
  Description  : MotifDisplayManger is the second class in this module.
                   For general file information see initial header. 
  Assumptions  :
  Comments     : Note also that when initialization is finished, the
		   xmDrawingArea is already managed.
		 The refresh callback is handled in an odd way for those
		   accustomed to C callbacks--the data field passed to the
		   callback is the DisplayArea object.  When called,
		   the callback makes a draw function call to this object.
  Complexity   :
  Enhancements : Since this is the class which deals with the creation
		   and initialization of the Xapplicaiton, an error
		   handler should be provided which deals with errors
		   from X and Xt.  For more on this, see section 14.1
		   of Volume Four of Nye and O'Reilly.
		 Scrollbars should be added.  Commonly, GLDs will take
		   up more than the size of a single screen.
		 Resize callbacks should be handled appropriately.
		   One might think that the way to do this is to
		   proportionally increase the size of the diagram's
		   drawing.  Although this would be easy to accomplish
		   with the flexible shape design, this is probably not
		   the ideal user interface because the entire image
		   might not be inside the window (for example,
		   consider the case with scrollbars).  As a result,
		   the appropriate response to a resize callback is to
		   do a redraw on the pixmap and adjust the
		   scrollbar state if necessary.
		 However, the ability to change the size of the
		   drawing's elements is useful and should be made
		   available via a zooming mechanism.  This could be
		   done either via clicking arrow keys, or a sliding
		   bar.  In terms of implementation,  this would be easy to
		   accomplish--one merely needs to change the xPixels
		   and yPixels.  
                 The architecture of the dialog could be made more flexible.
		   Currently, the quit button is put onto the top margin
		   of the drawing area.  Better solution: read the layout
		   in from a resource file that would place the quit button
		   and the canvas inside the dialog.  Although I'm
		   not sure exactly how this would be done, I've seen
		   it done with form widgets.
***************************************************************************/


/***************************************************************************
    Description :  Tell the DiagramDrawingArea to redraw all of the
                     shapes. 
    Comments    :  This is called, for example, when the window is covered
                     by another, and then is uncovered again.  Without
		     refresh, covering by another window would destroy what
		     was in the drawing area.
		   Static function.
***************************************************************************/
static void refreshCB(Widget /*w*/, MotifDisplayManager *motifDisplayManager,
		      caddr_t /*calldata*/)
{
   motifDisplayManager->get_diagram_drawing_area()->draw();
}


/***************************************************************************
    Description :  Callback called when the the user chooses to quit.
                   This is currently done via a quit dialog button,
		     but will eventually be a menu choice.
    Comments    :  Shuts down the current display.
                   Static function.
***************************************************************************/
static void quitCB(Widget /*w*/, MotifDisplayManager *mdm,
	    XmAnyCallbackStruct* /*call_data*/)
{ 
   mdm -> quit(); 
} 


/***************************************************************************
    Description :  Returns default graphics context given a widget.
    Comments    :  The client must remember to free this GC when
                     finished.
		   At first glance, one might wonder why this routine
		     isn't simply collapsed into the next one with a
		     default argument of "black" for the color.  The
		     motivation is that the client might not want to
		     ever deal with color allocation or allocating a
		     default color map.  For example, a client that
		     wishes to only deal with black and white
		     primitives has no reason to allocate a color map. 
		   Also, a default line width of zero may seem odd,
		     but it turns out that 0 is actually the default
		     line width for a graphics context.  0 tells the
		     X-server that you want a line width of 1, and
		     that it can go ahead and use the fastest
		     algorithm that it has for drawing this line.
		   Static function.
***************************************************************************/
static GC get_widget_GC(Widget w, int lineWidth = 0)
{
   XGCValues values;
   Display *display = XtDisplay(w);
   int screen = DefaultScreen(display);

   values.foreground = BlackPixel(display, screen); 
   values.background = WhitePixel(display, screen);
 
   values.line_width = lineWidth;
   values.fill_style = FillTiled;

   return (XtGetGC(w, GCForeground | GCBackground | 
		   GCFillStyle | GCLineWidth , &values));
}


/***************************************************************************
    Description :  Given a GLD color, Return an MString.
    Comments    :  Note that while this seems like a standard mapping,
                     depending on what colors are available in the
		     color map, this routine may not return the exact
		     color that is requested.
		   Static funciton.
***************************************************************************/
static MString get_motif_color_name(const GLDColor gldColor)
{
   switch (gldColor) {
   case Black:
      return("black");
   case White:
      return("white");
   case Blue:
      return("blue");
   case Green:
      return("green");
   case Cyan:
      return("cyan");
   case Red:
      return("red");
   case Magenta:
      return("magenta");
   case Yellow:
      return("yellow");
   default:
      err << "get_motif_color_name: the color " << gldColor <<
	 " which you requested " << "does not exist" << fatal_error;
      return "DisplayMngr bug";
   }
}


/***************************************************************************
    Description :  Returns graphics context with the foreground set to
                     the given color.
    Comments    :  The client must remember to free this GC when
                     finished.
		   Static function.
***************************************************************************/
static GC get_widget_GC(Widget w, const GLDColor gldColor)
{
   Bool isColor;
   XColor color;
   XGCValues values;
   Display *display = XtDisplay(w);
   int screen = DefaultScreen(display);
   Colormap cmap = DefaultColormap(display, screen);
   MString colorName = get_motif_color_name(gldColor);
   
   isColor = (XDisplayCells (display, screen) > 2);

   if (isColor &&
       XParseColor (display, cmap, colorName, &color) &&
       XAllocColor (display, cmap, &color))
      values.foreground = color.pixel;
   else
      values.foreground = BlackPixel(display, screen); 

   values.background = WhitePixel(display, screen);
   values.fill_style = FillTiled;
   
   return (XtGetGC(w, GCForeground | GCBackground |
		GCFillStyle, &values));
}
 

/***************************************************************************
    Description :  Sets up the shape array.
    Comments    :  This routine queries the gldPreferences for the
                     user's desired shape and color scheme.
		   Protected member.
***************************************************************************/
void MotifDisplayManager::initialize_shapes()
{
   for (int i = 0; i < NUMBER_OF_SHAPES; i++) {

      GC colorGC = get_widget_GC(canvas,
				 gldPreference->get_shape_color(i));
      GC shapeGC;
      // get the color for this shape.  For the set specifications of
      // PredictedTrain and PredictedTest, we use black, since the
      // background will already have a color.
      if (gldPreference->get_set_specification() == PredictedTrain ||
	  gldPreference->get_set_specification() == PredictedTest)
	 shapeGC = motifDisplayManagerGC;
      else
	 shapeGC = colorGC;

      colorSquareArray[i] = new MotifFill(canvas, colorGC);      
      
      // use this color to get the shape.
      switch(gldPreference->get_shape_type(i)) {
      case FilledCircle:
	 shapeArray[i] = new MotifFilledCircle(canvas, shapeGC);
	 break;
      case XMark:
	 shapeArray[i] = new MotifXMark(canvas, shapeGC);
	 break;
      case Triangle:
	 shapeArray[i] = new MotifTriangle(canvas, shapeGC);
	 break;
      case Line:
	 shapeArray[i] = new MotifLine(canvas, shapeGC);
	 break;
      case Circle:
	 shapeArray[i] = new MotifCircle(canvas, shapeGC);
	 break;
      case TopHalf:
	 shapeArray[i] = new MotifTopHalfCircle(canvas,
						shapeGC);
	 break;
      case BottomHalf:
	 shapeArray[i] = new MotifBottomHalfCircle(canvas,
						   shapeGC);
	 break;
      case Slash:
	 shapeArray[i] = new MotifSlash(canvas,
					shapeGC);
	 break;
      case BackSlash:
	 shapeArray[i] = new MotifBackSlash(canvas,
					    shapeGC);
	 break;
      case Rectangle:
	 shapeArray[i] = new MotifRectangle(canvas, shapeGC);
	 break;
      case Fill:
	 shapeArray[i] = new MotifFill(canvas, shapeGC);
	 break;
      default:
	 err << "MotifDisplayManager::initialize_shapes: the " << 
	        "MotifDisplayManager does not know about the shape " <<
		gldPreference->get_shape_type(i) << " that you " << 
		"requested. " << fatal_error;
      }
   }
}


/***************************************************************************
    Description :  Sets up the line array.
    Comments    :  Indexing is done by line thickness.
		   Protected member.
***************************************************************************/
void MotifDisplayManager::initialize_lines()
{
   GC thicknessGC;
   thicknessGC = get_widget_GC(canvas, 0);
   for (int thickness = 1; thickness < NUMBER_OF_LINE_WIDTHS;
	thickness++) {
      thicknessGC = get_widget_GC(canvas, thickness);
      lineArray[thickness] =  new MotifLine(canvas, thicknessGC);
   }
}


/***************************************************************************
    Description :  Initializes the application.
    Comments    :  These elements still need to be realized.
                   The dialog is made to be no bigger than the size that
		     the drawing area needs.  The quit button is put onto
		     the top margin.  See enhancements for better solution.  
                   Protected member.
***************************************************************************/
void MotifDisplayManager::create_application(int verticalPixels,
					     int horizontalPixels)
{
   int numArgs = 1;
   Array<char *> titleArg(1);
   titleArg[0] = DEFAULT_WINDOW_NAME;

   int           n = 0;
   Array<Arg>    wargs(2);
   

   int xPixels = horizontalPixels +
      diagramDrawingArea->get_horizontal_drawing_margin() + RIGHT_MARGIN;
   int yPixels = verticalPixels +
      diagramDrawingArea->get_vertical_drawing_margin() + TOP_MARGIN;
   XtSetArg(wargs[n], XmNwidth, xPixels); n++;
   XtSetArg(wargs[n], XmNheight, yPixels); n++;
   
   toplevel = XtAppInitialize(&app, "MLC++GLD", NULL, 0,
			      &numArgs, titleArg.get_elements(), 
			      NULL, wargs.get_elements(), n);
   n = 0;

   create_canvas();
}

   
/***************************************************************************
    Description :  Creates a drawing area inside the widgets created
                     in the create_application() routine.  The drawing
		     area has the number of pixels specified in the
		     routine parameters.
		   Also sets the DisplayMngr Graphics Context.
    Comments    :  This widget still needs to be realized.
    		   Protected member.
***************************************************************************/
void MotifDisplayManager::create_canvas()
{
   canvas = XtCreateManagedWidget("canvas",
				  xmDrawingAreaWidgetClass, 
				  toplevel, NULL, 0);

   quitButton = XtCreateManagedWidget("  Quit  ", xmPushButtonGadgetClass,
				      canvas, NULL, 0);

   motifDisplayManagerGC = get_widget_GC(canvas);
}


/***************************************************************************
    Description :  Register all callback routines.
    Comments    :  Protected member.
***************************************************************************/
void MotifDisplayManager::add_callbacks()
{
   // This looks odd, but what we're doing is passing this object to
   // the refresh callback so that we can make method calls to this
   // object.
   XtAddCallback(canvas, XmNexposeCallback, (XtCallbackProc)refreshCB,
		 this);
   XtAddCallback(canvas, XmNinputCallback, (XtCallbackProc)refreshCB,
		 this);
   
   done = FALSE;
   
   XtAddCallback(quitButton, XmNactivateCallback,(XtCallbackProc)quitCB,
		 this);
}


/***************************************************************************
    Description :  Create a MotifDisplayManager.  The Display Manager
                     initializes the application with the window
		     manager, creates the necessary widgets, registers
		     the callbacks for these widgets, and then
		     realizes the widgets.
    Comments    :  Although written for the Motif Window Manager,
                     these routines should be fairly functional under
		     twm and Open Windows as Motif is a fairly
		     standard window manager.  However, I did notice that
		     a 0 sized window will pop up under Motif, but
		     will not come up under twm.  
***************************************************************************/
MotifDisplayManager::MotifDisplayManager(DiagramManager *diagramManager,
					 DiagramDrawingArea *dda,
					 int verticalPixels,
					 int horizontalPixels)
: DisplayManager(diagramManager, dda)
{
   create_application(verticalPixels, horizontalPixels);

   add_callbacks();

   // initialize all shapes that are made available for all
   // DisplayManagers via Shape pointers.
   initialize_shapes();
   initialize_lines();
   boundingBox = new MotifBoundingBox(canvas, motifDisplayManagerGC);

   // this needs to be done after all of the widgets that use it for a
   // parent have been created
   XtRealizeWidget(toplevel);
}


/***************************************************************************
    Description :  Free the default GC.
    Comments    :  The call to ReleaseGC will fail if the canvas
                     widget is already deallocated.
***************************************************************************/
MotifDisplayManager::~MotifDisplayManager()
{
   delete boundingBox; 
}


/***************************************************************************
    Description :  Gets a string shape initialized with the default
                     graphics context and the string of the user's
		     choice.   
    Comments    :  The user of this routine is responsible for freeing
                     the shape.   
***************************************************************************/
Shape *MotifDisplayManager::create_string(const MString& newString)
{
   Shape *newShape = new MotifText(canvas, motifDisplayManagerGC,
				   newString);
   return(newShape);
}


/***************************************************************************
    Description :  Go into the main event loop.
    Comments    :  Callbacks are now the only source of program
                     execution.
                   When a quit callback is registered, this loop
		     exits.  (done is set to TRUE.)
***************************************************************************/
void MotifDisplayManager::run()
{
   while (!done) {
      XEvent event;
      XtAppNextEvent(app, &event);
      XtDispatchEvent(&event);
   }
}


/***************************************************************************
    Description :  Quit out of the main event loop.
    Comments    :  Note that neither does the entire program not exit,
                     but also that the MotifDisplayManager is not
		     deallocated.  Only the current execution of the
		     event loop is terminated. 
***************************************************************************/
void MotifDisplayManager::quit()
{
   done = TRUE;
}


/***************************************************************************
  Description  :  XfigDisplayManger is the second class in this module.
                    For general file information see initial header.
		  The XfigDisplayManager is somewhat simpler than the
		    MotifDisplayManager since it doesn't need to deal
		    with refreshes, user manipulation, or the X server. 
  Assumptions  :  The XfigDisplayManager can only flush an Xfig output
                    to an MLCOStream.  It does not interactively pop up
		    an XfigDisplay on the reasoning that if the user
		    wants to interactively view a diagram he will use
		    the MotifDisplayManager.
  Comments     :  Output is not sent to the file until the
                    DisplayMngr's run() funtion is called.  At this
		    point, the diagram's draw() function is called,
		    which tells all of the shapes to draw themselves.
		  Note that that initial_display must be called before
                    any of the shapes can be drawn.  This function is
		    responsible for initializing the file with some
		    version information.
  Complexity   :  
  Enhancements : 
***************************************************************************/


/***************************************************************************
    Description :  Initialize all of the shapes in the shape array.
    Comments    :  This should be adjusted so that the user can specify
                     his preferences without directly modifying this
		     routine. 
		   Protected member.
***************************************************************************/
void XfigDisplayManager::initialize_shapes()
{
   for (int i = 0; i < NUMBER_OF_SHAPES; i++) {

      // get the color for this shape.  For the set specifications of
      // PredictedTrain and PredictedTest, we use black, since the
      // background will already have a color.
      GLDColor shapeColor;
      int shapeDepth;
      
      if (gldPreference->get_set_specification() == PredictedTrain ||
	  gldPreference->get_set_specification() == PredictedTest) {
	 shapeDepth = MARKING_SHAPE_DEPTH;
	 shapeColor = Black;
      }
      else {
	 shapeColor = gldPreference->get_shape_color(i);
	 shapeDepth = NORMAL_SHAPE_DEPTH;
      }
      
      colorSquareArray[i] = new XfigFill(mlcOStream,
					 gldPreference->get_shape_color(i),2);

      switch(gldPreference->get_shape_type(i)) {
      case FilledCircle:
	 shapeArray[i] = new XfigFilledCircle(mlcOStream, shapeColor,
					      shapeDepth); 
	 break;
      case Triangle:
	 shapeArray[i] = new XfigTriangle(mlcOStream, shapeColor, shapeDepth);
	 break;
      case XMark:
	 shapeArray[i] = new XfigXMark(mlcOStream, shapeColor, shapeDepth);
	 break;
      case Line:
	 shapeArray[i] = new XfigLine(mlcOStream, 2, shapeColor, shapeDepth);
	 break;
      case Circle:
	 shapeArray[i] = new XfigCircle(mlcOStream, shapeColor, shapeDepth);
	 break;
      case TopHalf:
	 shapeArray[i] = new XfigTopHalfCircle(mlcOStream, shapeColor,
					       shapeDepth); 
	 break;
      case BottomHalf:
	 shapeArray[i] = new XfigBottomHalfCircle(mlcOStream, shapeColor,
						  shapeDepth); 
	 break;
      case BackSlash:
	 shapeArray[i] = new XfigBackSlash(mlcOStream, shapeColor,
					   shapeDepth); 
	 break;
      case Slash:
	 shapeArray[i] = new XfigSlash(mlcOStream, shapeColor, shapeDepth);
	 break;
      case Rectangle:
	 shapeArray[i] = new XfigRectangle(mlcOStream, shapeColor,
					   shapeDepth); 
	 break;
      case Fill:
	 shapeArray[i] = new XfigFill(mlcOStream, shapeColor, shapeDepth);
	 break;
      default:
	 err << "XfigDisplayManager::initialize_shapes: the " << 
	        "XfigDisplayManager does not know about the shape " <<
		gldPreference->get_shape_type(i) << " that you " << 
		"requested. " << fatal_error;
      }
   }
}


/***************************************************************************
    Description :  Sets up the line array.
    Comments    :  Indexing is done by line thickness.
		   Protected member.
***************************************************************************/
void XfigDisplayManager::initialize_lines()
{
   // A thickness of 0 is a special case because Xfig does not have a
   // default (as for example Motif does).
   lineArray[0] =  new XfigLine(mlcOStream, 1, 0, 0);
   
   for (int thickness = 1; thickness < NUMBER_OF_LINE_WIDTHS;
	thickness ++) 
      lineArray[thickness] =  new XfigLine(mlcOStream, thickness, 0, 0);
}


/***************************************************************************
    Description :  Set up initial parameters of the
                     XfigDisplayManager. 
    Comments    :  The pixels of the diagram is currently not used in
                     the Xfig version.  For uniformity of interface,
		     this is kept the same as the other display
		     manager constructor interfaces. 
***************************************************************************/
XfigDisplayManager::XfigDisplayManager(DiagramManager *diagramManager,
				       DiagramDrawingArea *dda,
				       int /*verticalPixels*/,
				       int /*horizontalPixels*/)
: DisplayManager(diagramManager, dda),
  mlcOStream(gldPreference->get_MLCOStream())
{
// This flag is available though gldPref so that you don't need to have your
// display set to an X server to run Xfig.  This is very useful for
// running testers.  The only thing that is currently lost is that Strings 
// don't query the X server to get the size of a font.

   MString fontName = defaultFontName;

   fontInfo = NULL;

   if (gldPreference->is_display()) {
      int numArgs = 1;
      Array<char *> titleArg(1);
      titleArg[0] = DEFAULT_WINDOW_NAME;
      toplevel = XtAppInitialize(&app, "MLC++GLD", NULL, 0,
				 &numArgs, titleArg.get_elements(), 
				 NULL, NULL, 0);
      // Make sure that we get a fontInfo if there is a display.  If not,
      // we just approximate the fontInfo in Shape.c by a fixed size
      // constant font.
      display = XtDisplay(toplevel);
      fontInfo = XLoadQueryFont(display, fontName);
      ASSERT (fontInfo != NULL);
   }
   
   initialize_shapes();
   initialize_lines();
   boundingBox = new XfigBoundingBox(mlcOStream);

   initialize_display();
}


/***************************************************************************
    Description :  Free the font info.
    Comments    :  
***************************************************************************/
XfigDisplayManager::~XfigDisplayManager()
{
   delete boundingBox;
   // Note that if we do not have a display, then trying to free it
   // will cause a segmentation fault.
   if (fontInfo)
      XFreeFont(display, fontInfo);
}

/***************************************************************************
    Description :  The stream which the user has chosen to use for
                     creating an Xfig file is obtained from the
		     preferences object.
    Comments    :  Xfig version information is set up via a comment in
                     the output file. 
***************************************************************************/
void XfigDisplayManager::initialize_display()
{
   mlcOStream = gldPreference->get_MLCOStream();
   
   // Note that there is also a format for Xfig 1.2.  Unless this
   // comment is the first line of the file, Xfig will assume that the
   // format is that of 1.2, and will thus fail while reading 2.1's
   // format.
   *mlcOStream  << "#FIG 2.1" << endl << "80 2" << endl;
}
 

/***************************************************************************
    Description :  This is where the shapes are actually output to a
                     file. 
    Comments    :  Note that unlike the MotifDiagramManager, this
                     routine will not go into a loop which waits for a
		     user's response to quit; it will do some work and
		     exit without user input.
***************************************************************************/
void XfigDisplayManager::run()
{
   get_diagram_drawing_area()->draw();
}


/***************************************************************************
    Description :  Gets a string shape initialized with the default
                     graphics context and the string of the user's
		     choice.   
    Comments    :  The user of this routine is responsible for freeing
                     the shape.   
***************************************************************************/
Shape *XfigDisplayManager::create_string(const MString& newString)
{
   Shape *newShape = new XfigText(mlcOStream, newString, fontInfo);
   return(newShape);
}


/***************************************************************************
    Description :  Unlike the Motif version, calling quit on an
                     XfigDisplayManager doesn't make sense.
    Comments    :  This needs to be defined since it is a pure virtual
                     function in the base class. 
***************************************************************************/
void XfigDisplayManager::quit()
{
   err << "XfigDisplayManager::quit() should never be called." <<
      fatal_error;
}





