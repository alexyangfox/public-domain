// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _InstList_h
#define _InstList_h 1

#include <BagSet.h>

extern const MString defaultDataExt;
extern const MString defaultNamesExt;

class InstanceList : virtual public InstanceBag {
   NO_COPY_CTOR(InstanceList);
protected:
   InstanceList();
   virtual DblLinkList<MString>* read_modifiers(MLCIStream& namesFile,
						int& line);
   virtual void apply_modifiers(const DblLinkList<MString>& modifiers);
   virtual void read_names(const MString& file);
   virtual void read_data_line(MLCIStream& dataFile, int& line);
public:
   // The file name suffices if the naems file and data file default to
   // ".names" and ".data" respectively.  If the files have a different
   // prefix, give an empty file name, and give the full file name as suffix.
   // Gets ownership of instance info and instances
   InstanceList(const MString& file,
		const MString& namesExtension = defaultNamesExt,
		const MString& dataExtension = defaultDataExt);
   // This constructor create an empty bag of instances.  Useful if
   //   you intend to assign, or generate the instances.
   InstanceList(const MString& file, ArgDummy dummyArg,
		const MString& namesExtension = defaultNamesExt);
   InstanceList(const SchemaRC& schemaRC);
   InstanceList(InstanceBag*& bag);
   virtual ~InstanceList();
   virtual InstanceRC remove_front();
   // The given file must match the list's instance info
   // Deletes old data
   virtual void read_data(const MString& file);
   virtual InstanceList* split_prefix(int numInSplit);
   // Gets ownership of and deletes bag
   virtual void unite(InstanceList*& bag);
   virtual InstanceList& cast_to_instance_list();
   virtual const InstanceList& cast_to_instance_list() const;
   virtual InstanceBag* create_my_type(const SchemaRC& schemaRC) const;
   InstanceList* shuffle(MRandom* mrandom = NULL,
			 InstanceBagIndex* index = NULL) const;
};

#endif
