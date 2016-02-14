// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/***************************************************************************
  Description  : Generate template files
***************************************************************************/

#include <basics.h>
#include <CatTestResult.h>
#include <GLD.h>

TEMPL_GENERATOR(MGLDTemplates) 
{
   Array<PtrArray<Shape*>*> apas(2);
   Array<AttrOrientation>   aao(2);
   Array<Arg>               aa(2); 

   GLDPref gldPreference;
   
   CatTestResult* result = NULL;
   GLD gld(*result, &gldPreference);

   return 0; // return success to shell
}   
