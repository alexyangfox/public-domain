// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _PerceptronInducer_h
#define _PerceptronInducer_h 1

#include <Inducer.h>
#include <LogOptions.h>
#include <MRandom.h>

class LinearDiscriminant;


class PerceptronInducer : public Inducer {
  NO_COPY_CTOR(PerceptronInducer);
  // Options
  Array<Real> *initialWeights;
  Real learningRate;        
  int maxEpochs;
  unsigned int randomSeed;

protected:
  LinearDiscriminant *perceptron;

public:
  static int defaultMaxEpochs;
  static Real defaultLearningRate;
  static unsigned int defaultRandomSeed;
  PerceptronInducer(const MString& dscr);
  PerceptronInducer(const PerceptronInducer& source, CtorDummy);
  virtual ~PerceptronInducer();
  virtual void train();
  virtual Bool was_trained(Bool fatalOnFalse = TRUE) const;
  virtual const Categorizer& get_categorizer() const;
  virtual void set_user_options(const MString& prefix);
  virtual Inducer* copy() const;
   
  // Options methods
  void set_initialWeights(const Array<Real>& iw) 
    {initialWeights = new Array<Real>(iw,ctorDummy);};
  Array<Real> *get_initialWeights() {return initialWeights;};
  void set_learningRate(Real lr);
  Real get_learningRate() {return learningRate;};
  void set_maxEpochs(int me);
  int get_maxEpochs() {return maxEpochs;};
};


#endif









