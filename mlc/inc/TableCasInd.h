// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

#ifndef _TableCasInd_h
#define _TableCasInd_h 1

#include <CtrInducer.h>
#include <Categorizer.h>
#include <CascadeCat.h>
#include <ProjectInd.h>
#include <AttrOrder.h>
#include <ConstCat.h>

class TableCasInd : public CtrInducer {
private:
   NO_COPY_CTOR(TableCasInd);
   CascadeCat* cat;     // a list of table categorizers and const categorizer.
   AttrOrder ao;

protected:
   virtual InstanceBag* assign_bag(InstanceBag*& newTS);

public:
   virtual void OK(int level = 0) const;
   TableCasInd(const MString& dscr);
   TableCasInd(const MString& dscr, const Array<int>& attrNums);
   TableCasInd(const TableCasInd& source, CtorDummy);
   
   ~TableCasInd();

   virtual void train();
   virtual Bool was_trained(Bool fatalOnFalse = TRUE) const;
   virtual const Categorizer& get_categorizer() const;
   virtual Inducer* copy() const;

   virtual const Array<int>& get_attr_nums() const { return ao.get_order(); }
   virtual void set_user_options(const MString& preFix);
   virtual AttrOrder& get_attr_order_info() { return ao; }
   // convenience function
   void set_order(const Array<int>& order) { ao.set_order(order);}
};
#endif
