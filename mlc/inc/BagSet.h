// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _BagSet_h
#define _BagSet_h 1

#include <InstanceRC.h>
#include <Pix.h>
#include <Categorizer.h>
#include <MLCStream.h>
#include <DLList.h>
#include <BoolArray.h>
#include <StatData.h>

typedef DLList<InstanceRC*> InstancePtrList;
typedef Array<Pix> InstanceBagIndex;
class InstanceBag; // defined later in the file
typedef PtrArray<InstanceBag*> BagPtrArray;

class InstanceList;    // defined in InstList.h
class CtrInstanceBag;  // defined in CtrBag.h
class CtrInstanceList; // defined in CtrInstList.h
class MRandom;         // defined in MRandom.h

// used in feature construction methods
typedef AttrValue_ (*FeatureFuncPtr) (const AttrValue_&, const AttrInfo&,
				      const AttrValue_&, const AttrInfo&,
				      const AttrInfo&);  
extern const MString defaultHeader;

class InstanceBag {
   InstancePtrList* instances;
   // schema is a pointer because there is not default constructor for
   // SchemaRC and we don't always know at construction what the SchemaRC will
   // be (i.e. when reading a list from a file)
   SchemaRC* schema;
   Bool weighted;
   static Real normalizeConfInterval;
   
   InstancePtrList& instance_list(); 
   const InstancePtrList& instance_list() const;
   NO_COPY_CTOR(InstanceBag);

   // feature construnction functions defined in BagFeature.c.
   void define_feature_info(int attrNum1, int attrNum2,
			    const MString& separator);
   void fill_in_feature_values(int attrNum1, int attrNum2,
			       const FeatureFuncPtr newFeatureFuncPtr);
protected:
   // This should only be called when InstanceList::read_files()
   // will be called
   InstanceBag();
   // Takes on values of given bag; gets ownership of bag
   virtual void copy(InstanceBag*& bag);
   // Each version of shuffle() should call this function and cast to
   // the appropriate return type.
   virtual InstanceBag* shuffle_(MRandom* mrandom,
					 InstanceBagIndex* index) const;
   virtual void extreme_normalize_attr(int attrNum,const RealAttrInfo& rai,
				       RealAttrValue_& min,
				       RealAttrValue_& max) const;
   virtual void interquartile_normalize_attr(int attrNum,
					     const RealAttrInfo& rai,
					     RealAttrValue_& min,
					     RealAttrValue_& max) const;

public:
   enum NormalizationMethod { none, extreme, interquartile };
   // Do nothing, but allow for derived classes to do checks here.
   virtual void OK(int level = 1) const;
   InstanceBag(const SchemaRC& schema);
   // copy constructor
   InstanceBag(const InstanceBag& source, CtorDummy);
   InstanceBag& operator=(const InstanceBag& source);
   virtual ~InstanceBag();
   SchemaRC get_schema() const;
   virtual void  set_schema(const SchemaRC& schemaRC);
   virtual Pix add_instance(const InstanceRC& instance);
   virtual InstanceRC remove_instance(Pix& pix);
   virtual void remove_all_instances();
   Bool no_instances() const;
   int num_instances() const;
   int num_categories() const;
   Pix first() const;     // pix
   void next(Pix& pix) const;
   InstanceRC get_instance(const Pix pix) const;
   // Display writes the bag information in readable form.
   virtual void display(MLCOStream& stream = Mcout,
			Bool protectChars = TRUE,
			Bool normalizeReal = FALSE) const;
   // If we have a tie and one of them is the given tieBreaker, we prefer it.
   // The idea is to give the majority of some higher level.
   virtual Category majority_category(Category tieBreaker = UNKNOWN_CATEGORY_VAL) const;
   // Returns Pix for matching  instance or NULL if not found.
   virtual Pix find_labelled(const InstanceRC&) const;
   virtual Pix find_unlabelled(const InstanceRC&) const;
   virtual BagPtrArray* split(const Categorizer& cat) const;
   virtual BagPtrArray* split_by_label() const;
   // Some useful functions from LabelledInstanceInfo (note they do not
   //   need to be virtual because they are just an interface)
   const AttrInfo& attr_info(int attrNum) const {
      return get_schema().attr_info(attrNum);}
   const NominalAttrInfo& nominal_attr_info(int attrNum) const {
      return attr_info(attrNum).cast_to_nominal();}
   const AttrInfo& label_info() const {
      return get_schema().label_info();}
   const NominalAttrInfo& nominal_label_info() const {
      return label_info().cast_to_nominal();}
   int num_label_values() const {
      return nominal_label_info().num_values();}
   int num_attr_values(int attrNum) const {
      return nominal_attr_info(attrNum).num_values();}
   int num_attr() const {
      return get_schema().num_attr();}
   virtual InstanceList& cast_to_instance_list();
   virtual const InstanceList& cast_to_instance_list() const;
   virtual CtrInstanceBag& cast_to_ctr_instance_bag();
   virtual const CtrInstanceBag& cast_to_ctr_instance_bag() const;
   virtual CtrInstanceList& cast_to_ctr_instance_list();
   virtual const CtrInstanceList& cast_to_ctr_instance_list() const;
   virtual InstanceBagIndex* create_bag_index() const;
   virtual InstanceBag* independent_sample(int size,
				   MRandom* mrandom = NULL,
				   InstanceBagIndex* index = NULL) const;
   virtual InstanceBag* independent_sample(int size,
				   InstanceBag *restOfBag,
				   MRandom* mrandom = NULL,
			           InstanceBagIndex* index = NULL) const;
   InstanceBag* shuffle(MRandom* mrandom = NULL,
				InstanceBagIndex* index = NULL) const;
   virtual InstanceBag* create_my_type(const SchemaRC& schemaRC) const;
   virtual void remove_inst_with_unknown_attr();
   virtual void remove_inst_with_unknown_attr(int attrNumX);
   virtual void remove_conflicting_instances();
   virtual InstanceBag* project(const BoolArray& attrMask) const;
   virtual void set_weighted(Bool weight);
   virtual Bool get_weighted() const;
   virtual void set_weight(Pix pix, Real wt);
   virtual Real get_weight(Pix pix) const;
   virtual Real total_weight() const;
   virtual void buntine_display(MLCOStream& stream = Mcout,
			        Bool test = FALSE) const;
   virtual void display_names(MLCOStream& stream = Mcout,
			      Bool protectChars = TRUE,
                              const MString& header = defaultHeader) const;
   virtual void normalize_attr(int attrNum, NormalizationMethod method);
   virtual void normalize_attr(int attrNum, NormalizationMethod method,
			       RealAttrValue_& min, RealAttrValue_& max) const;
   virtual void normalize_bag(NormalizationMethod method);

   // feature construction function defined in BagFeature.c.
   virtual InstanceBag *copy_with_blank_feature() const;
   // should be used ONLY for bags created with
   // copy_with_blank_feature(). Defined in BagFeature.c.
   virtual void change_feature(int attrNum1, int attrNum2,
			       const MString& separator,
			       const FeatureFuncPtr newFeatureFuncPtr);

   InstanceBag *clone() const;

   // comparison
   virtual Bool operator==(const InstanceBag& other) const;
   Bool operator!=(const InstanceBag& other) const {
      return !operator==(other); }
   // corrupt_values changes each attribute value in each instance
   // to unknown with the given probability.
   virtual void corrupt_values_to_unknown(Real rate, MRandom& mrandom);
};

DECLARE_DISPLAY(InstanceBag);

#endif


