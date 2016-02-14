// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file for Reference Counted handles.
// For a full description of reference counting in MLC++ see the
//   appropriate document.
// Note that there is no matching .c file.  See the base class for 
//   further description.

#ifndef _InstanceRC_h
#define _InstanceRC_h 1

/***************************************************************************
  Comments     : InstanceRC is the handler class of Instance.
  History      :  James Dougherty                                 9/17/94
                     Added set_schema() method.
		   Richard Long, YeoGirl Yun.                     6/13/94
                     Initial revision
***************************************************************************/



#include <Instance.h>

class InstanceRC {
#define HANDLE_CLASS InstanceRC
#define BODY_CLASS Instance
#include <RefCount.h>

public:
   InstanceRC(const SchemaRC& instSchema) { set_rep(new Instance(instSchema));}

   AttrValue_ operator[](int index) const { 
      return read_rep()->operator[](index);  }

   AttrValue_& operator[](int index) {
      return write_rep()->operator[](index); }

   AttrValue_ get_label() const { return read_rep()->get_label();}

   void set_label(const AttrValue_& lvalue) { write_rep()->set_label(lvalue);}

   virtual Real get_weight() const {return read_rep()->get_weight();}

   virtual void set_weight(Real wt) {write_rep()->set_weight(wt);}

   Bool equal(const InstanceRC& irc, Bool fatalOnFalse = FALSE) const {
      if (rep == irc.rep)
	 return TRUE;
      else
	 return read_rep()->equal( *(irc.read_rep()), fatalOnFalse);
   }

   Bool equal_no_label(const InstanceRC& irc,
		       Bool fatalOnFalse =  FALSE) const {
      if (rep == irc.rep)
	 return TRUE;
      else
	 return read_rep()->equal_no_label(*(irc.read_rep()),
					   fatalOnFalse);
   }

   Bool operator==(const InstanceRC& irc) const {
      return read_rep()->operator==( *(irc.read_rep()) );
   }

   Bool operator!=(const InstanceRC& irc) const {
      return read_rep()->operator!=( *(irc.read_rep()) );
   }

   // SGI compiler said it doesn't make sense to make this const
   //  (because it's an object which is copied anyway.  NICE!)
   SchemaRC get_schema() const { return read_rep()->get_schema(); }

   void set_schema(const SchemaRC& schemaRC) {
      write_rep()->set_schema(schemaRC);
   }

   const AttrInfo& attr_info(int attrNum) const {
      return read_rep()->attr_info(attrNum);
   }
   
   const AttrInfo& label_info() const { return read_rep()->label_info(); }

   Bool is_labelled(Bool fatalOnFalse = FALSE) const {
      return read_rep()->is_labelled(fatalOnFalse);
   }
      
   const NominalAttrInfo& nominal_attr_info(int attrNum) const {
      return read_rep()->nominal_attr_info(attrNum);
   }

   const NominalAttrInfo& nominal_label_info() const {
      return read_rep()->nominal_label_info();
   }

   int num_attr_values(int attrNum) const {
      return read_rep()->num_attr_values(attrNum);
   }

   int num_attr() const  { return read_rep()->num_attr(); }

   int num_label_values() const  { return read_rep()->num_label_values(); }

   const MString& attr_name(int attrNum) const {
      return read_rep()->attr_name(attrNum);
   }

   const MString& nominal_to_string(int attrNum,
				    Category cat) const {
      return read_rep()->nominal_to_string(attrNum,cat);
   } 

   InstanceRC remove_attr(int attrNum) const {
      // Can't unite the following because must pass non-temporary
      Instance* newRep = read_rep()->remove_attr(attrNum);
      return InstanceRC(newRep);
   }

   InstanceRC remove_attr(int attrNum, const SchemaRC&
			  schemaWithDelAttr) const {
	    // Can't unite the following because must pass non-temporary
      Instance* newRep = read_rep()->remove_attr(attrNum, schemaWithDelAttr);
      return InstanceRC(newRep);
   }

   void display_unlabelled(MLCOStream& stream = Mcout,
			   Bool protectChars = FALSE,
			   Bool displayWeight = FALSE,
			   Bool normalizeReal = FALSE) const {
      read_rep()->display_unlabelled(stream, protectChars,
				     displayWeight, normalizeReal);
   } 

   void display(MLCOStream& stream = Mcout,
		Bool protectChars = FALSE,
	        Bool displayWeight = FALSE,
		Bool normalizeReal = FALSE) const {
      read_rep()->display(stream, protectChars, displayWeight, normalizeReal);
   }		  

   void buntine_display(MLCOStream& stream = Mcout) const {
      read_rep()->buntine_display(stream);
   }

   InstanceRC project(const SchemaRC& shortSchemaRC,
		      const BoolArray& attrMask) const {
      Instance* newRep = read_rep()->project(shortSchemaRC, attrMask);
      return InstanceRC(newRep);
   }
};

// Note inline because there is no .c file and we don't want this to
// be multiply defined 
inline DEF_DISPLAY(InstanceRC);
#endif
