// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file for Reference Counted handles.
// For a full description of reference counting in MLC++ see the
//   appropriate document.
// Note that there is no matching .c file.  See the base class for 
//   further description.


/***************************************************************************
  Comments     : SchemaRC is the handler class of Schema.
  History      : Richard Long, YeoGirl Yun                          6/13/94
                   Initial revision
***************************************************************************/

#ifndef _SchemaRC_h
#define _SchemaRC_h 1
#include <Schema.h>

class SchemaRC {

#define HANDLE_CLASS SchemaRC
#define BODY_CLASS Schema
#include <RefCount.h>

public:
   SchemaRC(DblLinkList<AttrInfoPtr>*& attrInfos) {
      set_rep(new Schema(attrInfos));
   }
   
   SchemaRC(DblLinkList<AttrInfoPtr>*& attrInfos, AttrInfo*& lInfo) {
      set_rep(new Schema(attrInfos, lInfo));
   }

   // The "ordinary" copy constructor defined in <RefCount.h>
   // destructor defined by <RefCount.h>

   void OK(int level=0) const  { read_rep()->OK(level); }

   const AttrInfo& label_info() const  { return read_rep()->label_info(); }

   int num_attr() const  { return read_rep()->num_attr(); }

   Bool is_labelled(Bool fatalOnFalse = FALSE) const {
      return read_rep()->is_labelled(fatalOnFalse);
   }

   const AttrInfo& attr_info(int num) const {
      return read_rep()->attr_info(num);
   }

   // checks whether an AttrInfo is contained in Schema.
   Bool member(const AttrInfo& ainfo) const {
      return read_rep()->member(ainfo);
   }

   Bool equal(const SchemaRC schema, Bool fatalOnFalse = FALSE) const  {
      if (rep == schema.rep)
	 return TRUE;
      else
	 return read_rep()->equal( *schema.read_rep(), fatalOnFalse);
   }

   Bool equal_no_label(const SchemaRC schema, Bool fatalOnFalse = FALSE)
      const {
      if (rep == schema.rep)
	 return TRUE;	 
      else
	 return read_rep()->equal_no_label( *(schema.read_rep()),
					   fatalOnFalse);
   }

   Bool operator==(const SchemaRC schema) const {
      return read_rep()->operator==(*schema.read_rep());
   }

   Bool operator!=(const SchemaRC schema) const {
      return read_rep()->operator!=(*schema.read_rep());
   }

   void equal_except_del_attr(const SchemaRC schema,
			      int deletedAttrNum) const {
      read_rep()->equal_except_del_attr(*schema.read_rep(),
					deletedAttrNum);
   }

   const MString& attr_name(int attrNum) const {
      return read_rep()->attr_name(attrNum);
   }

   const NominalAttrInfo& nominal_attr_info(int attrNum) const {
      return read_rep()->nominal_attr_info(attrNum);
   }

   int num_attr_values(int attrNum) const {
      return read_rep()->num_attr_values(attrNum);
   }

   int num_label_values() const  { return read_rep()->num_label_values(); }
   const MString& nominal_to_string(int attrNum,
				    Category cat) const {
      return read_rep()->nominal_to_string(attrNum, cat);
   }				       

   virtual MString category_to_label_string(Category cat) const {
      return read_rep()->category_to_label_string(cat);
   }

   const NominalAttrInfo& nominal_label_info() const {
      return read_rep()->nominal_label_info();
   }

   virtual SchemaRC remove_attr(int attrNum) const  {
      // Can't unite the following because must pass non-temporary
      Schema* newSchema = read_rep()->remove_attr(attrNum);
      return SchemaRC(newSchema);
   }

   virtual void display(MLCOStream& stream = Mcout,
			Bool protectChars = TRUE) const {
      read_rep()->display(stream, protectChars);
   }

   virtual void display_names(MLCOStream& stream,
			      Bool protectChars = TRUE,
                              const MString& header = defaultHeader) const {
      read_rep()->display_names(stream, protectChars, header);
   }

   virtual SchemaRC project(const BoolArray& attrMask) const {
      Schema* newSchema = read_rep()->project(attrMask);
      return SchemaRC(newSchema);
   }
};

// Note inline because there is no .c file and we don't want this to
// be multiply defined 
inline DEF_DISPLAY(SchemaRC);
#endif
