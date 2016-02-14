// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _Shape_h
#define _Shape_h 1


// A #define of a String in X is needed if we also have a class
// called String in the MLC++ library.  For this reason, our
// String class has been named MString.
// However if we had a String, X allows for us to #define their String
// to something else.  For more on this, see line 55 of
// X11/Intrinsic.h.

// X11 include files 
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <X11/Intrinsic.h> 
#include <X11/Xutil.h>

// Motif include files
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/Label.h>

// Intrinsic includes Xlib.h which defines Bool to int.  This
// conflicts with our definition in basics.h, so we undef it.
#undef Bool

extern MString defaultFontName;

struct ShapeBoundingBox {
   int x1;
   int y1;
   int x2;
   int y2;
   ShapeBoundingBox(int x1, int y1, int x2,
		    int y2);
   int get_radius() const;
   ShapeBoundingBox create_box_smaller_by_one() const;
};


// Shape is an abstract base class that is meant to be the
// abstract idea of a shape.  In other words, only
// classes that inherit from Shape should know about environment
// specific things such as widgets.
class Shape {
   NO_COPY_CTOR(Shape);
public:      
   Shape() {}
   virtual ~Shape() {}
   virtual void draw(const ShapeBoundingBox& boundingBox) = 0;
};


// MotifShape is the abstract base class for all Shapes which are
// Motif specific.
class MotifShape : public Shape {
   NO_COPY_CTOR(MotifShape);
protected:
   GC gc;  // GC stands for graphics context.  It is an Xlib structure
           // which  includes information such as color and line style.
   Widget widget;
   Display *display;
   Window   window; 

public:
   MotifShape() {}
   virtual ~MotifShape() {}
   MotifShape(Widget w, GC newGC);
   virtual void draw(const ShapeBoundingBox& boundingBox) = 0;
   void set_GC(GC newGC);
   void set_widget(Widget w);
};


class MotifLine : public MotifShape {
   NO_COPY_CTOR(MotifLine);
public:
   MotifLine() {}
   ~MotifLine() {}
   MotifLine(Widget w, GC newGC); 
   virtual void draw(const ShapeBoundingBox& boundingBox);
};


class MotifCircle : public MotifShape {
   NO_COPY_CTOR(MotifCircle);
public:
   MotifCircle() {}
   ~MotifCircle() {}
   MotifCircle(Widget w, GC newGC);
   virtual void draw(const ShapeBoundingBox& boundingBox);
};

class MotifTopHalfCircle : public MotifShape {
   NO_COPY_CTOR(MotifTopHalfCircle);
public:
   MotifTopHalfCircle() {}
   ~MotifTopHalfCircle() {}
   MotifTopHalfCircle(Widget w, GC newGC);
   virtual void draw(const ShapeBoundingBox& boundingBox);
};

class MotifBottomHalfCircle : public MotifShape {
   NO_COPY_CTOR(MotifBottomHalfCircle);
public:
   MotifBottomHalfCircle() {}
   ~MotifBottomHalfCircle() {}
   MotifBottomHalfCircle(Widget w, GC newGC);
   virtual void draw(const ShapeBoundingBox& boundingBox);
};


class MotifXMark : public MotifShape {
   NO_COPY_CTOR(MotifXMark);
public:
   MotifXMark() {}
   ~MotifXMark() {}
   MotifXMark(Widget w, GC newGC);
   virtual void draw(const ShapeBoundingBox& boundingBox);
};


class MotifSlash : public MotifShape {
   NO_COPY_CTOR(MotifSlash);
public:
   MotifSlash() {}
   ~MotifSlash() {}
   MotifSlash(Widget w, GC newGC);
   virtual void draw(const ShapeBoundingBox& boundingBox);
};


class MotifBackSlash : public MotifShape {
   NO_COPY_CTOR(MotifBackSlash);
public:
   MotifBackSlash() {}
   ~MotifBackSlash() {}
   MotifBackSlash(Widget w, GC newGC);
   virtual void draw(const ShapeBoundingBox& boundingBox);
};


class MotifFilledCircle : public MotifShape {
   NO_COPY_CTOR(MotifFilledCircle);
public:
   MotifFilledCircle() {}
   ~MotifFilledCircle() {}
   MotifFilledCircle(Widget w, GC newGC);
   virtual void draw(const ShapeBoundingBox& boundingBox);
};


class MotifRectangle : public MotifShape {
   NO_COPY_CTOR(MotifRectangle);
public:
   MotifRectangle() {}
   ~MotifRectangle() {}
   MotifRectangle(Widget w, GC newGC);
   virtual void draw(const ShapeBoundingBox& boundingBox);
};


class MotifFill : public MotifShape {
   NO_COPY_CTOR(MotifFill);
public:
   MotifFill() {}
   ~MotifFill() {}
   MotifFill(Widget w, GC newGC);
   virtual void draw(const ShapeBoundingBox& boundingBox);
};


class MotifTriangle : public MotifShape {
   NO_COPY_CTOR(MotifTriangle);
public:
   MotifTriangle() {}
   ~MotifTriangle() {}
   MotifTriangle(Widget w, GC newGC);
   virtual void draw(const ShapeBoundingBox& boundingBox);
};


class MotifBoundingBox : public MotifShape {
   NO_COPY_CTOR(MotifBoundingBox);
public:
   MotifBoundingBox() {}
   ~MotifBoundingBox() {}
   MotifBoundingBox(Widget w, GC newGC);
   virtual void draw(const ShapeBoundingBox& boundingBox);
};


class MotifText : public MotifShape {
   NO_COPY_CTOR(MotifText);
   // I didn't make any of these members const because I thought that
   // one might want to specialize from MotifText to create a FlexMotifText
   // class where all fields can be manipulated.
   MString textString;

   XFontStruct *fontInfo;
   MString fontName;
   Display *display;

public:
   MotifText() {}
   ~MotifText();

   // Constructor for MotifText needs to store string and get some font
   // information. 
   MotifText(Widget w, GC newGC, const MString& str);
   virtual void draw(const ShapeBoundingBox& boundingBox);  
};


// XfigShape is the abstract base class for all Shapes which are
// Xfig specific.
class XfigShape : public Shape {
   NO_COPY_CTOR(XfigShape);

protected:
   MLCOStream *output;
   int shapeColor;
   int shapeDepth;
   
public:
   XfigShape() {}
   virtual ~XfigShape() {}
   XfigShape(MLCOStream *output, int color = 0, int depth = 2);
   virtual void draw(const ShapeBoundingBox& boundingBox) = 0;
   void set_output(MLCOStream *output);
};


class  XfigLine : public XfigShape {
   NO_COPY_CTOR(XfigLine);
   int lineThickness;
public:
   XfigLine() {}
   ~XfigLine() {}
   XfigLine(MLCOStream *output, int thickness = 1, int color = 0,
	    int depth = 0);
   void draw(const ShapeBoundingBox& boundingBox);
};


class  XfigCircle : public XfigShape {
   NO_COPY_CTOR(XfigCircle);
public:
   XfigCircle() {}
   ~XfigCircle() {}
   XfigCircle(MLCOStream *output, int color = 0, int depth = 2);
   void draw(const ShapeBoundingBox& boundingBox);
};


class  XfigTopHalfCircle : public XfigShape {
   NO_COPY_CTOR(XfigTopHalfCircle);
public:
   XfigTopHalfCircle() {}
   ~XfigTopHalfCircle() {}
   XfigTopHalfCircle(MLCOStream *output, int color = 0, int depth = 2);
   void draw(const ShapeBoundingBox& boundingBox);
};


class  XfigBottomHalfCircle : public XfigShape {
   NO_COPY_CTOR(XfigBottomHalfCircle);
public:
   XfigBottomHalfCircle() {}
   ~XfigBottomHalfCircle() {}
   XfigBottomHalfCircle(MLCOStream *output, int color = 0, int depth = 2);
   void draw(const ShapeBoundingBox& boundingBox);
};


class  XfigFilledCircle : public XfigShape {
   NO_COPY_CTOR(XfigFilledCircle);
public:
   XfigFilledCircle() {}
   ~XfigFilledCircle() {}
   XfigFilledCircle(MLCOStream *output, int color = 0, int depth = 2);
   void draw(const ShapeBoundingBox& boundingBox);
};


class  XfigXMark : public XfigShape {
   NO_COPY_CTOR(XfigXMark);
public:
   XfigXMark() {}
   ~XfigXMark() {}
   XfigXMark(MLCOStream *output, int color = 0, int depth = 2);
   void draw(const ShapeBoundingBox& boundingBox);
};


class  XfigSlash : public XfigShape {
   NO_COPY_CTOR(XfigSlash);
public:
   XfigSlash() {}
   ~XfigSlash() {}
   XfigSlash(MLCOStream *output, int color = 0, int depth = 2);
   void draw(const ShapeBoundingBox& boundingBox);
};


class  XfigBackSlash : public XfigShape {
   NO_COPY_CTOR(XfigBackSlash);
public:
   XfigBackSlash() {}
   ~XfigBackSlash() {}
   XfigBackSlash(MLCOStream *output, int color = 0, int depth = 2);
   void draw(const ShapeBoundingBox& boundingBox);
};


class  XfigRectangle : public XfigShape {
   NO_COPY_CTOR(XfigRectangle);
public:
   XfigRectangle() {}
   ~XfigRectangle() {}
   XfigRectangle(MLCOStream *output, int color = 0, int depth = 2);
   void draw(const ShapeBoundingBox& boundingBox);
};


class  XfigFill : public XfigShape {
   NO_COPY_CTOR(XfigFill);
public:
   XfigFill() {}
   ~XfigFill() {}
   XfigFill(MLCOStream *output, int color = 0, int depth = 2);
   void draw(const ShapeBoundingBox& boundingBox);
};


class  XfigTriangle : public XfigShape {
   NO_COPY_CTOR(XfigTriangle);
public:
   XfigTriangle() {}
   ~XfigTriangle() {}
   XfigTriangle(MLCOStream *output, int color = 0, int depth = 2);
   void draw(const ShapeBoundingBox& boundingBox);
};


class  XfigBoundingBox : public XfigShape {
   NO_COPY_CTOR(XfigBoundingBox);
public:
   XfigBoundingBox() {}
   ~XfigBoundingBox() {}
   XfigBoundingBox(MLCOStream *output, int color = 0, int depth = 2);
   void draw(const ShapeBoundingBox& boundingBox);
};


class  XfigText : public XfigShape {
   NO_COPY_CTOR(XfigText);
   MString textString;
   XFontStruct *fontInfo;   
   
public:
   XfigText() {}
   ~XfigText() {}
   XfigText(MLCOStream *output, const MString& string,
	    XFontStruct *fontInfo, int color = 0, int depth = 0);
   void draw(const ShapeBoundingBox& boundingBox);
};


#endif













