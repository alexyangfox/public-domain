// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "ProjectInd.c".

#ifndef ProjectInd_h
#define ProjectInd_h


#include <basics.h>
#include <IncrInducer.h>
#include <AugCategory.h>
#include <Instance.h>
#include <BagSet.h>
#include <ProjectCat.h>

class ProjectInd : public CtrInducer {
   NO_COPY_CTOR(ProjectInd);
   ProjectCat * categorizer;
   BaseInducer * wrappedInducer;
   BoolArray * attrMask;   

// shortSchema only needed for incremental enhancement
//   SchemaRC * shortSchema;
   
public:
   virtual void OK(int level = 0) const;
   
   ProjectInd(const MString& description);
   virtual ~ProjectInd();

   virtual void set_wrapped_inducer(BaseInducer*& ind);
   virtual const BaseInducer& get_wrapped_inducer() const;
   virtual BaseInducer* release_wrapped_inducer(); // releases ownership
   
   virtual Bool has_wrapped_inducer(Bool fatalOnFalse = FALSE) const;

   virtual void set_project_mask(BoolArray*& attr);
   virtual const BoolArray& get_project_mask() const;
   virtual Bool has_project_mask(Bool fatalOnFalse = FALSE) const;

   virtual void train();
   virtual Real train_and_test(InstanceBag* trainingSet,
			       const InstanceBag& testBag);
   virtual Bool was_trained(Bool fatalOnFalse = TRUE) const;
   virtual const Categorizer& get_categorizer() const;
   virtual Categorizer* release_categorizer(); // transfers ownership to caller

   virtual Bool can_cast_to_inducer() const;
   virtual Bool can_cast_to_incr_inducer() const;
//   IncrInducer& cast_to_incr_inducer();

//   virtual Pix add_instance(const InstanceRC& instance);
//   virtual InstanceRC del_instance(Pix& pix);

   
   virtual void display(MLCOStream& stream = Mcout,
			const DisplayPref& dp = defaultDisplayPref) const;

};

DECLARE_DISPLAY(ProjectInd);
#endif









