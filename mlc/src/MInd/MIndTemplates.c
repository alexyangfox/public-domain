// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/***************************************************************************
  Description  : Generate template files
***************************************************************************/

#include <basics.h>
#include <IBInducer.h>
#include <NaiveBayesInd.h>
#include <TableInducer.h>
#include <BagAndDistance.h>
#include <MinArray.h>


TEMPL_GENERATOR(MIndTemplates) 
{
   Array2<NBNorm> anbn(2,3,4,5);
   Array<BagAndDistance> abad(2);
   MinArray<BagAndDistance> mbad(3);

   IBInducer ib("ib");
   NaiveBayesInd nb("nb");
   TableInducer ti("ti");
   
   return 0; // return success to shell
}   
