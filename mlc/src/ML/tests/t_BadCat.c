// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Creates BadCategorizer and calls all methods.
  Doesn't test :
  Enhancements :
  History      : Richard Long                                       8/03/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <BadCat.h>

RCSID("MLC++, $RCSfile: t_BadCat.c,v $ $Revision: 1.24 $")

class Foo : public Categorizer {
public:
   Foo(const int noCat, const MString& dscr) : Categorizer(noCat, dscr) {}
   virtual AugCategory categorize(const InstanceRC&) const {
      return UNKNOWN_AUG_CATEGORY; // to avoid error
   }
   virtual void display_struct(MLCOStream& stream,
			     const DisplayPref& dp = defaultDisplayPref) const
   {
      (void)stream;
      (void)dp;
      err << "t_Categorizer::Bad call to Categorizer" << fatal_error;   
   }
   virtual Categorizer* copy() const {return NULL;}
  // Returns the class id
   virtual int class_id() const { return -1; };

   virtual Bool operator==(const Categorizer&)const { return TRUE;} ;

   
};


main()
{
  cout << "t_BadCat executing" << endl;
  ASSERT(badCategorizer.description() == "Bad Categorizer");

#ifndef MEMCHECK
  TEST_ERROR("BadCategorizer::num_categories: Bad categorizer.",
	     badCategorizer.num_categories());

  DblLinkList<AttrInfoPtr>* empty = new DblLinkList<AttrInfoPtr>;
  SchemaRC schema(empty);
  TEST_ERROR("BadCategorizer::categorize: Bad categorizer.",
	     badCategorizer.categorize(schema));

  TEST_ERROR("BadCategorizer::BadCategorizer: There is already an instance",
	     BadCategorizer badcat2);

  TEST_ERROR("BadCategorizer::display_struct: Bad categorizer",
	     badCategorizer.display_struct());
#endif
  
  ASSERT(BadCategorizer::is_bad_categorizer(badCategorizer));
  
  Foo foo(1, "Not Bad");
  ASSERT(!BadCategorizer::is_bad_categorizer(foo));

  TEST_ERROR("BadCategorizer::operator==: Bad Categorizer",
	     if (badCategorizer == badCategorizer););
	     

  return 0; // return success to shell
}   
