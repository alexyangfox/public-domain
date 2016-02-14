// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _ProjBag_h
#define _ProjBag_h 1

#include <DblLinkList.h>
#include <Array.h>
#include <InstanceRC.h>
#include <BagSet.h>

class InstanceBag;

// ProjectionInfo represents the destinations of projected instances,
//    without the actual instances (base class).
class ProjectionInfo {
   // The dimension of the array matches the number of values of the
   //   projected attribute.  Each element tells what the destination
   //   bag (Category) of the projected instance should be, if the attribute
   //   number has a value matching the given category number.
   Array<NominalVal> destBag;
   Array<Real> destCounts;
   int numDests; // number of elements in array which are not UNKNOWN.
   //  Number of instances projected.  This may be higher than the
   //    number of known instances in destBag since more than one
   //    instance may agree on a given destination.
   NO_COPY_CTOR(ProjectionInfo);
protected:
public:
   Real weight; // total weight
   void OK(int level = 0) const;
   ProjectionInfo(int numValues);
   ProjectionInfo(const ProjectionInfo& pi, CtorDummy);
   virtual ~ProjectionInfo();
   const Array<NominalVal>& dest_bag() const {return destBag;}
   const Array<Real>& dest_counts() const {return destCounts;}
   int num_dests() const {return numDests;}
   void set_dest(NominalVal dest, Category val, Real weight);
   void merge_dests(const Array<NominalVal>& dests, 
                    const Array<Real>& destCounts2);
   virtual void display(const NominalAttrInfo& attrInfo, 
                        MLCOStream& stream = Mcout) const;
};



// This class is used in ProjBag.  It would have been a nested class if
//   nested classes worked nicely (it doesn't because of the template
//   instantiation).
class InstanceProjection : public ProjectionInfo {
   NO_COPY_CTOR(InstanceProjection);
public:
   InstanceRC instance;
   void OK() const {ProjectionInfo::OK();}
   InstanceProjection(int numValues, const InstanceRC& instance);
   virtual ~InstanceProjection() {};
   virtual void display(MLCOStream& stream = Mcout) const;
   virtual void display(const NominalAttrInfo& attrInfo, 
                        MLCOStream& stream = Mcout) const;
};

DECLARE_DISPLAY(InstanceProjection);


typedef DblLinkList<InstanceProjection*> ProjList;
typedef DLLPix<InstanceProjection*> ProjListPix;

// ProjInfoWithBag has the bag of instances that agree on ProjectionInfo
class ProjInfoWithBag : public ProjectionInfo {
   NO_COPY_CTOR(ProjInfoWithBag);
public:
   InstanceBag* bag;
   void OK(int level = 0) const;
   ProjInfoWithBag(int numValues, const SchemaRC& schema);
   ProjInfoWithBag(InstanceProjection& ip);
   ~ProjInfoWithBag();
   Pix add_projection(InstanceProjection& ip);

};

typedef DblLinkList<ProjInfoWithBag*> ProjInfoPtrList;
typedef DLLPix<ProjInfoWithBag*> ProjInfoPtrPix;


// Projection of Instance Bag -- The MAIN class.

class ProjBag  {
   const SchemaRC schema;
   const NominalAttrInfo deletedAttrInfo;
   int deletedAttrNum;
   DBG_DECLARE(Real weightInstances;) // total weight accounted for

   
   InstanceProjection& find_or_create_proj(const InstanceRC& projInst);
   // Project one bag, defining the destination to be the given Category.
   virtual void project_bag(const InstanceBag& bag, Category category);
   NO_COPY_CTOR(ProjBag);
public:
   ProjList projList;
   virtual void OK(int level = 0) const;
   // The attribute number is the attribute to delete (i.e., the
   //   instance is projected on all other attributes).
   // The constructor considers the bag number where the
   //   instance came from as the destination category.
   ProjBag(const BagPtrArray& LIBags, int attrNum);
   virtual ~ProjBag();
   void del(ProjListPix& pix, int dir);
   virtual ProjListPix find_no_label(const InstanceRC& instance) const;
   // Get projected instance (both const and non-const versions).
   // Display writes the information in readable form.
   virtual void display(MLCOStream& stream = Mcout) const;
   SchemaRC get_schema() const {return  schema;}
   const NominalAttrInfo& deleted_attr_info() const
      {return deletedAttrInfo;}
};

DECLARE_DISPLAY(ProjBag);

#endif
