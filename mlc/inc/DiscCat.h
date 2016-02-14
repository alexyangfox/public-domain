// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _DiscCat_h
#define _DiscCat_h 1


#include <basics.h>
#include <InstList.h>
#include <RealDiscretizor.h>
#include <EntropyDisc.h>
#include <BinningDisc.h>
#include <OneR.h>

class DiscCat : public Categorizer{

   SchemaRC discSchema;
   Categorizer* categorizer;
   PtrArray<RealDiscretizor*>* disc;  // @@ change to PtrArray
   
public:
   DiscCat( const MString& description,
	    PtrArray<RealDiscretizor*>*& discretizors, //gets ownership
	    const SchemaRC& discretizedSchema,
	    Categorizer*& cat);
   DiscCat(const DiscCat& source, CtorDummy);
   virtual ~DiscCat();
   virtual void OK(int level = 0) const;
   virtual AugCategory categorize(const InstanceRC& inst) const;
   virtual void display_struct(MLCOStream& stream,
		       const DisplayPref& dp) const;
   virtual Categorizer* copy() const;
   virtual int class_id() const { return CLASS_DISC_CATEGORIZER; }
   virtual Bool operator==(const Categorizer &cat) const;
   virtual Bool operator==(const DiscCat &cat) const;
};

#endif







