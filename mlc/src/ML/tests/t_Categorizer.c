// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test Categorizer.
                 We have to subclass it because Categorizer is an ABC.
  Doesn't test : Categorizer::short_display() which is protected.
  Enhancements :
  History      : Ronny Kohavi                                       8/03/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <Categorizer.h>

RCSID("MLC++, $RCSfile: t_Categorizer.c,v $ $Revision: 1.18 $")


class foo : public Categorizer {
public:
   foo(const int noCat, const MString& dscr) : Categorizer(noCat, dscr) {}
   virtual AugCategory categorize(const InstanceRC&) const {
      err << "t_Categorizer::Bad call to Categorizer" << fatal_error;
      return UNKNOWN_AUG_CATEGORY;}
   virtual void display_struct(MLCOStream& stream,
			    const DisplayPref& dp = defaultDisplayPref) const {
      (void)stream;
      (void)dp;
      err << "t_Categorizer::Bad call to Categorizer" << fatal_error;   
   }
   virtual Categorizer* copy() const { return NULL; }
   // dummy member function, just to make it concrete class
   virtual int class_id() const { return -1;}
   virtual Bool operator==(const Categorizer&) const {return TRUE;}
};


main()
{
   cout << "Executing t_Categorizer" << endl;

   foo* bar = new foo(3,"test"); // should work fine.
   ASSERT(bar->num_categories() == 3);
   ASSERT(bar->description() == "test");
   delete bar;

#ifndef MEMCHECK
   TEST_ERROR("Categorizer::Categorizer: number of categories"
              " must be positive (-3 is invalid)",
              new foo(-3, "test"));
   TEST_ERROR("Categorizer::Categorizer: empty description",
              new foo(3, ""));

   char *ptr = NULL; // test if passing NULL works.
   TEST_ERROR("Categorizer::Categorizer: empty description",
              new foo(3, ptr));
#endif
   
   return 0; // return success to shell
}   
