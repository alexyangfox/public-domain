// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/***************************************************************************
  Description  : Generate template files
***************************************************************************/

#include <basics.h>
#include <HOODGInducer.h>
#include <isocat.h>
#include <Array.h>
#include <ODGInducer.h>


TEMPL_GENERATOR(MGraphTemplates) 
{
   DblLinkList<ProjInfoWithBag*> dblpiwb;
   Array<UsedAttributes> aua(2,2);


   HOODGInducer hi("hi");
   Array<UsedAttributes> foo(0);
   
   return 0; // return success to shell
}   
