// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/***************************************************************************
  Description  : Generate template files
***************************************************************************/

#include <basics.h>
#include <Bootstrap.h>
#include <AccEstInducer.h>
#include <BaseInducer.h>
#include <EntropyODGInducer.h>
#include <COODGInducer.h>


TEMPL_GENERATOR(MWrapperTemplates) 
{
   Array<SampleElement> ase(2);
   Array<ProjLevel*> apl(2);
   AccEstInducer aei("foo", "aei");
   EntropyODGInducer eodg("eodg");
   COODGInducer coodg("coodg");
   
   return 0; // return success to shell
}   
