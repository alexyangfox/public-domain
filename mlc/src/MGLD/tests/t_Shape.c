// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test the XFig shapes. 
  Doesn't test : The MotifShapes.
                 The MotifShapes are tested by the Diagram and
                   DiagramManager testers.  This is because a large
		   amount of the code (i.e. setting up an X
		   application), is needed for the Motif Shapes. 
  Enhancements :
  History      : Dave Manley                                        3/30/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <Shape.h>
#include <mlcIO.h>
#include <Array.h>

RCSID("MLC++, $RCSfile: t_Shape.c,v $ $Revision: 1.5 $")


main()
{
   Mcout << "t_Shape executing" << endl;
   MLCOStream out0("t_Shape.out1");

   // This dummy call is needed to avoid link error which says that
   // XtInherit is not defined.
   XtToolkitInitialize();

   out0 << "#FIG 2.1"<< endl;
   out0 << "80 2"<< endl << endl;
   
   XfigCircle circ(&out0);
   ShapeBoundingBox sbb(10, 20, 30, 30);
   circ.draw(sbb);

   XfigLine l(&out0, 4, 5);
   ShapeBoundingBox sbb4(110, 120, 130, 130);
   l.draw(sbb4);


   XfigTopHalfCircle thcirc(&out0);
   ShapeBoundingBox thsbb(140, 150, 190, 400);
   thcirc.draw(thsbb);

   XfigBackSlash bsl(&out0);
   XfigSlash sl(&out0);
   bsl.draw(thsbb);
   sl.draw(thsbb);
   
   XfigBottomHalfCircle bhcirc(&out0);
   ShapeBoundingBox bhsbb(188, 340, 210, 380);
   bhcirc.draw(bhsbb);
   l.draw(bhsbb);


   XfigXMark xm(&out0);
   ShapeBoundingBox sbb3(70, 90, 120, 140);
   xm.draw(sbb3);

   XfigTriangle xft(&out0, 3, 5);
   ShapeBoundingBox sbbT(400, 200, 500, 300);
   xft.draw(sbbT);

   XfigFilledCircle fc(&out0);
   ShapeBoundingBox sbb5(210, 220, 270, 260);
   fc.draw(sbb5);

   XfigRectangle r(&out0);
   ShapeBoundingBox sbb6(50, 20, 220, 80);
   r.draw(sbb6);

   XfigFill xffill(&out0);
   ShapeBoundingBox sbb99(5, 2, 22, 80);
   xffill.draw(sbb99);

   XfigText texts(&out0, "behold, look, lo, what say you", NULL);
   ShapeBoundingBox sbb23(200, 200, 300, 300);
   texts.draw(sbb23);
   
   XfigBoundingBox bb(&out0);
   ShapeBoundingBox sbb7(110, 120, 135, 145);
   bb.draw(sbb7);

   
   XfigBoundingBox bbsmall(&out0);
   ShapeBoundingBox sbbsmall(310, 320, 320, 330);
   bbsmall.draw(sbbsmall); 

   // these calls test memory management, which I had a bug in
   // at one point.  (If these calls do not cause a segmentation
   // fault, no error has occured.)
   MLCOStream foo("f");
   PtrArray<Shape *> shapeArray2(1);
   shapeArray2[0] = new XfigCircle(&foo);
   
   XfigCircle *cc = new XfigCircle(&out0);
   delete cc;

//   TEST_ERROR("too small spacing",
//	      new ShapeBoundingBox(110, 120, 35, 145));

//   TEST_ERROR("too small spacing",
//	      new ShapeBoundingBox(110, 120, 135, 45));

   return 0; // return success to shell
}   
