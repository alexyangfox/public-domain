// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _CascadeCat_h
#define _CascadeCat_h 1

#include <Categorizer.h>
#include <Array.h>

class CascadeCat : public Categorizer {
   PtrArray<Categorizer *> * catList;
   NO_COPY_CTOR(CascadeCat);
   
public:
   virtual void OK(int level = 0) const;

   CascadeCat( const MString& dscr,
	       PtrArray<Categorizer *>*& cats );
   
   CascadeCat( const CascadeCat& source, const CtorDummy);
   virtual ~CascadeCat();

   virtual const PtrArray<Categorizer *> & get_categorizer_list() const
       {return *catList;}

   virtual AugCategory categorize(const InstanceRC& inst) const;

   virtual void display_struct(MLCOStream& stream,
			       const DisplayPref& dp) const;

   virtual void display(MLCOStream& stream = Mcout,
			const DisplayPref& dp = defaultDisplayPref) const;

   virtual Categorizer* copy() const;

   
   virtual int class_id() const { return CLASS_CASCADE_CATEGORIZER; }

   virtual Bool operator==(const Categorizer &cat) const;
   virtual Bool operator==(const CascadeCat &cat) const;
   virtual int num_categorizers() const { return catList->size(); }
};

DECLARE_DISPLAY(CascadeCat);

#endif





