// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/***************************************************************************
  Description  : Generate template files
***************************************************************************/

#include <basics.h>
#include <C45Inducer.h>
#include <ID3Inducer.h>
#include <LazyDTInducer.h>

TEMPL_GENERATOR(MTreeTemplates) 
{
   Array<CacheInfo> aci(1);
   Array<InfoValAndSize> aias(1);

   DynamicArray<node_struct*> dans(2);
   Array<node_struct*> ans(2,3);
   DynamicArray<edge_struct*> daes(2);

   ID3Inducer id3("id3");
   C45Inducer c45("c45");
   LazyDTInducer lazyDT("lazyDT");
   
   return 0; // return success to shell
}   
