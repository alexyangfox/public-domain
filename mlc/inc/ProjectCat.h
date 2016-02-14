// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _ProjectCat_h
#define _ProjectCat_h 1

#include <Categorizer.h>
#include <BoolArray.h>

class ProjectCat : public Categorizer {
   NO_COPY_CTOR(ProjectCat);
   BoolArray attrMask;
   SchemaRC fullSchema;
   SchemaRC shortSchema;
   Categorizer* categorizer;
   
public:
   virtual void OK(int level = 0)const;

   ProjectCat( const MString&   dscr,
	       const BoolArray& attrmask,
	       const SchemaRC&  schema,
	       Categorizer*&    cat );
   
   ProjectCat( const ProjectCat& source,
	       CtorDummy);

   virtual ~ProjectCat();

   virtual AugCategory categorize(const InstanceRC& inst) const;

   virtual void display_struct(MLCOStream& stream,
			       const DisplayPref& dp) const;

   virtual Categorizer* copy() const;

   virtual int class_id() const { return CLASS_PROJECT_CATEGORIZER; }

   virtual Bool operator==(const Categorizer &cat) const;
   virtual Bool operator==(const ProjectCat &cat) const;
};

#endif





