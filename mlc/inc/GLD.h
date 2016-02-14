// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _GLD_h
#define _GLD_h 1

#include <GLDPref.h>
#include <DiagramMngr.h>
#include <DisplayMngr.h>

class CatTestResult;

// Shape selection (as well as the acutal number of shapes) should
// eventually be further defined in a preferences class. 
#define MAX_NUM_SHAPES 4

class GLD {
private:
   NO_COPY_CTOR(GLD);
   LOG_OPTIONS;
   GLDPref *gldPreference;
   DiagramManager *diagramManager;
   DisplayManager *displayManager;
public:
   GLD(const CatTestResult& catTestResult, GLDPref* gldPreference);
   virtual ~GLD() { delete diagramManager; }
};

#endif





