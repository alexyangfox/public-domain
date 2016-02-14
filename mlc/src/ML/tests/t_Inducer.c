// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests Inducer.  
                 It must be subclassed because it is an ABC.
  Doesn't test : assign_data()
  Enhancements :
  History      : Richard Long                                       8/17/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <Inducer.h>
#include <InstList.h>

RCSID("MLC++, $RCSfile: t_Inducer.c,v $ $Revision: 1.25 $")


class Foo : public Inducer {
public:
  Foo(const MString& description) : Inducer(description) {}
  virtual void train() 
  {err << "Foo::train: Bad call to train." << fatal_error;}
  virtual AugCategory predict(const InstanceRC&) const
  {
    err << "Foo::predict: Bad call to predict()." << fatal_error;
    return UNKNOWN_AUG_CATEGORY;
  }
  // display training set.
  // used to check that read_data() works
  virtual void display_struct(MLCOStream& stream = Mcout,
       			      const DisplayPref& dp = defaultDisplayPref) const
  {
     (void)dp;
     TS->display(stream);
  }

  virtual Bool was_trained(Bool x) const {NOT_IMPLEMENTED; return x;}
  virtual const Categorizer& get_categorizer() const {
     NOT_IMPLEMENTED; return *((Categorizer*)NULL_REF);
  }
};


main()
{
  cout << "Executing t_Inducer" << endl;

  Foo foo("Test t_Inducer");

  ASSERT(foo.has_data(FALSE) == FALSE);
  TEST_ERROR("Training data has not been set", foo.has_data());
  foo.read_data("t_Inducer");
  ASSERT(foo.has_data() == TRUE);
  MLCOStream out1("t_Inducer.out1");
  out1.set_width(80); 
  foo.display_struct(out1);
  
  return 0; // return success to shell
}   
