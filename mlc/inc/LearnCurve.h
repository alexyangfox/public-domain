// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _LearnCurve_h
#define _LearnCurve_h 1

#include <LogOptions.h>
#include <Array.h>
#include <MRandom.h>
#include <MLCStream.h>

class BaseInducer;
class InstanceList;

struct LearnCurveInfo {
   int numTrainings;
   int trainSize;
};

extern const MString defaultFileStem;

class LearnCurve {
   LOG_OPTIONS;
   RAND_OPTIONS;
   MString fileStem;
   Array<LearnCurveInfo> info;
   PtrArray<Array<Real>*> result;
public:
   virtual void OK(int level = 0) const;
   LearnCurve(const Array<LearnCurveInfo>& learnCurveInfoArray,
	      const MString& fileName = defaultFileStem);
   virtual ~LearnCurve();
   virtual const MString& get_file_stem() const;
   virtual void set_file_stem(const MString& fileName);
   virtual void learn_curve(BaseInducer& inducer, InstanceList& trainBag);
   virtual Real get_accuracy(int level, int trainingNum) const;
   virtual Real get_average_accuracy(int level) const;
   virtual void display(MLCOStream& stream = Mcout) const;
};

DECLARE_DISPLAY(LearnCurve);

#endif
