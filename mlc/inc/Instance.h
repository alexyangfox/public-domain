// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _Instance_h
#define _Instance_h 1

#include <SchemaRC.h>
#include <Array.h>
#include <BoolArray.h>

// If a printed instance takes more than 1 line, lines after the 1st will
// be indented by this much
extern const MString instanceWrapIndent;

// Note that there are no virtuals here because instances should work
// fast. 
class Instance {

friend class InstanceRC;
   
   SchemaRC schema;
   Array<AttrValue_> values; // Array of values as described by schema
   AttrValue_ labelValue;
   Real weight;
   int refCount;

   NO_COPY_CTOR(Instance);
   void init_values();

public:
   void OK(int level = 0) const; 
   Instance(const SchemaRC& schemaRC);
   // copy constructor
   Instance(const Instance& source, CtorDummy);
   ~Instance();

   inline AttrValue_ operator[](int index) const;
   AttrValue_& operator[](int index);

   inline AttrValue_ get_label() const;
   void set_label(const AttrValue_& lvalue);

   Real get_weight() const;
   void set_weight(Real wt);

   // equal() compares the labelInfos for the schemata if they exist.
   // Returns FALSE if one has a labelInfo and the other does not.
   Bool equal(const Instance&, Bool fatalOnFalse = FALSE) const;
   // equal_no_label() does not use the labelInfo for either Schema whether
   // or not they exist.
   Bool equal_no_label(const Instance&,
			       Bool fatalOnFalse = FALSE) const;
   Bool operator==(const Instance& instance) const {
      return equal(instance, FALSE);
   }
   Bool operator!=(const Instance& instance) const {
      return !equal(instance, FALSE);
   }

   SchemaRC get_schema() const;
   void set_schema(const SchemaRC& schemaRC);
   const AttrInfo& attr_info(int attrNum) const;
   Bool is_labelled(Bool fatalOnFalse = FALSE) const;
   const AttrInfo& label_info() const;
   const NominalAttrInfo& nominal_attr_info(int attrNum) const;
   const NominalAttrInfo& nominal_label_info() const;
   int num_attr_values(int attrNum) const;
   int num_attr() const { return schema.num_attr(); }
   int num_label_values() const;
   const MString& attr_name(int attrNum) const;
   const MString& nominal_to_string(int attrNum,
					    Category cat) const;
   Instance* remove_attr(int attrNum) const;
   Instance* remove_attr(int attrNum,
				 const SchemaRC& schemaWithDelAttr) const;
   void display_unlabelled(MLCOStream& stream = Mcout,
                                   Bool protectChars = FALSE,
				   Bool displayWeight = FALSE,
				   Bool normalizeReals = FALSE) const;
   void display(MLCOStream& stream = Mcout,
                        Bool protectChars = FALSE,
			Bool displayWeight = FALSE,
			Bool normalizeReals = FALSE) const;
   void buntine_display(MLCOStream& stream = Mcout) const;
   Instance* project(const SchemaRC& shortSchemaRC,
			     const BoolArray& attrMask) const;
};

DECLARE_DISPLAY(Instance);

// profiling of ID3 on shuttle (43,500 instances) showed operator[] const
//   and get label takes lots of time.

/***************************************************************************
  Description : Returns the attribute value corresponding to the index.
  Comments    : The indexes start at 0. This is const version.
***************************************************************************/
AttrValue_ Instance::operator[](int index) const
{
   DBGSLOW(attr_info(index).check_in_range(values[index]));
   return values[index];
}


/*****************************************************************************
  Description : Returns *labelInfo in Schema. If labelInfo is not set,
                  fatal error is generated.
  Comments    :
*****************************************************************************/
AttrValue_ Instance::get_label() const
{
   DBGSLOW(label_info().check_in_range(labelValue));
   return labelValue;
}

#endif
