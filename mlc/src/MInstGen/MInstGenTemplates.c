// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/***************************************************************************
  Description  : Generate template files
***************************************************************************/

#include <basics.h>
#include <LabInstGen.h>
#include <InstList.h>


class foo : public LabelGenFunctor {
public:
   foo() : LabelGenFunctor() {}
   virtual void operator() (const InstanceRC&, AttrValue_&,
			    AttrValue_&) {};
};


TEMPL_GENERATOR(MInstGenTemplates) 
{
   InstanceBag *testBag = NULL;
   foo functor;
   IndependentLabInstGenerator labInstGen(testBag->get_schema(), functor);

   return 0; // return success to shell
}   
