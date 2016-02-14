// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _Schema_h
#define _Schema_h 1

#include <Attribute.h>
#include <DLList.h>
#include <Pix.h>
#include <BoolArray.h>

extern const MString defaultHeader;

// This class encapsulates the pointer for doubly-linked-list.
class AttrInfoPtr {
public:
   AttrInfoPtr(AttrInfo *ai) : attrInfo(ai) {}
   AttrInfo *attrInfo;
   operator==(const AttrInfoPtr& aip) { return (*aip.attrInfo) == (*attrInfo);}
};


// Instance information
class Schema {
   friend class SchemaRC;
   
   // The schema owns all of the AttrInfo.
   PtrArray<AttrInfo*> attr;
   AttrInfo* labelInfo;
   int refCount;
   
   NO_COPY_CTOR2(Schema, attr(0));
   
public:
   virtual void OK(int level = 0) const;
   // note that there is no default constructor.
   Schema(DblLinkList<AttrInfoPtr>*& attrInfos);
   Schema(DblLinkList<AttrInfoPtr>*& attrInfos, AttrInfo*& lInfo);
   // copy constructor
   Schema(const Schema& source, CtorDummy);
   virtual ~Schema();

   virtual const AttrInfo& label_info() const;
   virtual Bool is_labelled(Bool fatalOnFalse=FALSE) const;
   virtual int num_attr() const { return attr.size(); }
   virtual const AttrInfo& attr_info(int num) const;
   // checks whether an AttrInfo is contained in Schema.
   virtual Bool member(const AttrInfo& ainfo) const;

   // equal() compares the labelInfos for the schemas if they exist.
   // Returns FALSE if one has a labelInfo and the other does not.
   virtual Bool equal(const Schema&, Bool fatalOnFalse = FALSE) const;
   // equal_no_label() does not use the labelInfo for either Schema whether
   // or not they exist.
   virtual Bool equal_no_label(const Schema&, Bool fatalOnFalse = FALSE) const;
   virtual Bool operator==(const Schema& schema) const {
      return equal(schema, FALSE);
   }
   virtual Bool operator!=(const Schema& schema) const {
      return !equal(schema, FALSE);
   }
   virtual void equal_except_del_attr(const Schema& schema,
				      int deletedAttrNum) const;
   virtual const MString& attr_name(int attrNum) const;
   virtual const NominalAttrInfo& nominal_attr_info(int attrNum) const;
   virtual int num_attr_values(int attrNum) const;
   virtual int num_label_values() const;
   virtual const MString& nominal_to_string(int attrNum,
					    Category cat) const;
   virtual MString category_to_label_string(Category cat) const;
   virtual const NominalAttrInfo& nominal_label_info() const;
   virtual Schema* remove_attr(int attrNum) const;
   virtual void display(MLCOStream& stream = Mcout,
			Bool protectChars = TRUE) const;
   virtual void display_names(MLCOStream& stream = Mcout,
			      Bool protectChars = TRUE,
                              const MString& header = defaultHeader) const;
   virtual Schema* project(const BoolArray& attrMask) const;
};

DECLARE_DISPLAY(Schema);

#endif
