// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/***************************************************************************
  Description  : Generate template files
***************************************************************************/

#include <basics.h>
#include <OneRInducer.h>
#include <BinningDisc.h>
#include <EntropyDisc.h>


TEMPL_GENERATOR(MTransTemplates) 
{
   Array<AttrAndLabel> aal(2);
   DynamicArray<AttrAndLabel> daal(2);
   DynamicArray<BagAndBestSplitEntropy*> dbabs(2);
   PtrArray<RealDiscretizor*> pard(2);


   OneRInducer ori("ori", -1);
   BinningRealDiscretizor brd( -1, -1, *(SchemaRC*)NULL_REF);
   EntropyDiscretizor ed(-1, *(SchemaRC*)NULL_REF, 0);
   return 0; // return success to shell
}   
