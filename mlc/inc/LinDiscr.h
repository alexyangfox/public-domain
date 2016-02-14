// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.


#ifndef _LinearDiscriminant_h
#define _LinearDiscriminant_h 1

#include <Categorizer.h>
#include <MRandom.h>

class LinearDiscriminant : public Categorizer {
   RAND_OPTIONS;
   NO_COPY_CTOR(LinearDiscriminant);
private:
  const SchemaRC schema;
  AugCategory *cat0;
  AugCategory *cat1;
  Array<Real> weights;
  Real randLo, randHi;
  Real threshLo, threshHi;
public:
  virtual void OK(int level = 0) const;
  LinearDiscriminant(const SchemaRC& aSchema, const MString& descr,
		     Real randL = -1.0, Real randH = 1.0,
		     Real threshL = -1.0, Real threshH = 1.0);
  LinearDiscriminant(const LinearDiscriminant&, const CtorDummy);
  virtual ~LinearDiscriminant();
  virtual AugCategory categorize(const InstanceRC& inst) const;
  virtual void display_struct(MLCOStream& = Mcout,
			      const DisplayPref& = defaultDisplayPref) const;
  virtual Categorizer* copy() const;
  virtual Bool operator==(const Categorizer &cat) const;
  virtual Bool operator==(const LinearDiscriminant &cat) const;
  void init_weights(Array<Real>& weights);
  void set_weights(const Array<Real>&);
  void add_to_weights(const Array<Real>&);
  void subtract_from_weights(const Array<Real>&);
  void multiply_weights(const Array<Real>&);
  int get_num_weights() {return weights.size();};
  void set_randRange(Real lo, Real hi) {randLo = lo; randHi = hi;};
  void set_thresholdRange(Real lo, Real hi) {threshLo = lo; threshHi = hi;};
  Real get_randLow() {return randLo;};
  Real get_randHigh() {return randHi;};
  Real get_thresholdLow() {return threshLo;};
  Real get_thresholdHigh() {return threshHi;};
  virtual int class_id() const {return CLASS_LINDISCR_CATEGORIZER;}
  static unsigned int defaultRandomSeed;
};

#endif




