// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.


#ifndef IBCategorizer_h
#define IBCategorizer_h

#include <TableCat.h>
#include <InstList.h>
#include <BagMinArray.h>

class IBCategorizer : public TableCategorizer {
private:   
   NO_COPY_CTOR(IBCategorizer);
   int kVal;
   Array<Real> *weightVector;
   InstanceList instStore;
   IBInducer::NeighborVote vote;
   IBInducer::NnkValue nnkValue;
protected:
   Real distance(const InstanceRC& inst1, const InstanceRC& inst2) const;
   Category major_category(const BagMinArray& minArray) const;   
public:

   virtual void OK(int level = 0) const;
   IBCategorizer(const MString& dscr, const InstanceBag& bag);
   IBCategorizer(const IBCategorizer& source, CtorDummy);
   virtual ~IBCategorizer();
   virtual AugCategory categorize(const InstanceRC& ) const;
   virtual void display_struct(MLCOStream& stream,
			       const DisplayPref& dp) const ;
   virtual Categorizer* copy() const;
   virtual void set_weights(const Array<Real>& weights);
   virtual const Array<Real>& get_weights() const { 
        ASSERT(weightVector); return *weightVector; }
   int get_k_val() const { return kVal; }
   void set_k_val(int k);
   // Returns the class id
   virtual int class_id() const { return CLASS_IB_CATEGORIZER;};

   virtual void add_instance(const InstanceRC& inst);
   virtual Bool operator==(const Categorizer&) const;
   virtual Bool operator==(const IBCategorizer&) const;
   virtual void set_neighbor_vote(IBInducer::NeighborVote v) { vote = v; }
   virtual IBInducer::NeighborVote get_neighbor_vote() const { return vote; }
   virtual void set_nnk_value(IBInducer::NnkValue v) { nnkValue = v; }
   virtual IBInducer::NnkValue get_nnk_value() const { return nnkValue; }
};
#endif
