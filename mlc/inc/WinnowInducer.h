// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _WinnowInducer_h
#define _WinnowInducer_h 1

#include <Inducer.h>
#include <LogOptions.h>
#include <MRandom.h>

class LinearDiscriminant;


class WinnowInducer : public Inducer {
  NO_COPY_CTOR(WinnowInducer);
  // Options
  Array<Real> *initialWeights;
  Real alpha, beta;
  int maxEpochs;
  static int defaultMaxEpochs;
  static Real defaultAlpha;
  static Real defaultBeta;
  unsigned int randomSeed;
protected:
  LinearDiscriminant *winnow;
public:
  WinnowInducer(const MString& dscr);
  WinnowInducer(const WinnowInducer& source, CtorDummy);
  virtual ~WinnowInducer();
  virtual int class_id() const { return WINNOW_INDUCER;}
  virtual void train();
  virtual Bool was_trained(Bool fatalOnFalse = TRUE) const;
  virtual const Categorizer& get_categorizer() const;
  virtual void set_user_options(const MString& prefix);
  virtual Inducer* copy() const;

   
  // Options methods
  Array<Real> *get_initialWeights() {return initialWeights;};
  void set_alpha(Real alph);
  void set_beta(Real bet);
  Real get_alpha() {return alpha;};
  Real get_beta() {return beta;};
  void set_maxEpochs(int me);
  int get_maxEpochs() {return maxEpochs;};
  void set_initialWeights(const Array<Real>& iw);

};


#endif
