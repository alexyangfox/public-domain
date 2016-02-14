// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _CtrInstList_h
#define _CtrInstList_h 1

#include <InstList.h>
#include <CtrBag.h>

class CtrInstanceList : public InstanceList, public CtrInstanceBag {
   NO_COPY_CTOR(CtrInstanceList);
protected:
  virtual void read_names(const MString& file);

public:
   // The file name suffices if the naems file and data file default to
   // ".names" and ".data" respectively.  If the files have a different
   // prefix, give an empty file name, and give the full file name as suffix.
   CtrInstanceList(const MString& file,
		   const MString& namesExtension = defaultNamesExt,
		   const MString& dataExtension = defaultDataExt);
   CtrInstanceList(const SchemaRC& schemaRC);
   CtrInstanceList(const CtrInstanceList& source, CtorDummy);
   // Gets ownership of Bag
   CtrInstanceList(CtrInstanceBag*& bag);
   virtual ~CtrInstanceList(); 
   virtual InstanceRC remove_front();
   virtual CtrInstanceList& cast_to_ctr_instance_list();
   virtual const CtrInstanceList& cast_to_ctr_instance_list() const;
   virtual InstanceBag* create_my_type(const SchemaRC& schemaRC) const;
   CtrInstanceList* shuffle(MRandom* mrandom = NULL,
			    InstanceBagIndex* index = NULL) const;
};				
#endif
