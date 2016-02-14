// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/***************************************************************************
  Description  : Generate template files
***************************************************************************/

#include <basics.h>
//@@#include <DiscInducer.h>
#include <FSSInducer.h>
#include <C45APInducer.h>
#include <CompState.h>
#include <HCSearch.h>
#include <BFSearch.h>
#include <SASearch.h>


static void state_space()
{
   StateSpace<State<Array<int>,AccEstInfo> > ss;
   Array<CompElem> ace(2);
   Mcout << ace;
   DynamicArray<CompElem> dace(2);

   BFSearch<Array<int>, AccEstInfo> bfs;
   HCSearch<Array<int>, AccEstInfo> hcs;
   SASearch<Array<int>, AccEstInfo> sas;

}

TEMPL_GENERATOR(MFSSTemplates) 
{

   FSSInducer fssi("FSS");
   C45APInducer c45AP("C45AP");

   state_space();
   //@@ DiscInducer   di("DISC", "di");

   return 0; // return success to shell
}   
