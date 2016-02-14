// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test the ConstCategorizer.
  Doesn't test :
  Enhancements :
  History      : Ronny Kohavi                                       7/17/93
                   Initial revision
***************************************************************************/


#include <basics.h>
#include <errorUnless.h>
#include <ConstCat.h>
#include <ThresholdCat.h>
#include <AttrCat.h>
#include <InstList.h>

RCSID("MLC++, $RCSfile: t_ConstCat.c,v $ $Revision: 1.28 $")


// 704 is my street address.  This avoids any "lucky" thing from
// happening here.
const int OFFSET = 704;
const int NUM_CATEGORIZERS = 100;
const MString CATEGORIZER_DESCR("My Const Categorizer");
const MString AUG_DESCR("Category");

/***************************************************************************
  Description : Creates a NominalAttrInfo with the given number of values.
  Comments    : Values will be called VAL<n>, where <n> in [0,numVals-1].
                Name is set to "test nominal <numVals>"
***************************************************************************/
NominalAttrInfo* make_nai(int numVals, int num)
{
   DblLinkList<MString>* attrVals = new DblLinkList<MString>;
   for (int j = 0; j < numVals; j++) {
      MString val("VAL" + MString(j, 0));
      attrVals->append(val);
   }
   NominalAttrInfo* nai;
   nai = new NominalAttrInfo("test nominal" + MString(num, 0), attrVals);
   ASSERT(attrVals == NULL);
   return nai;
}


/***************************************************************************
  Description : Creates a list with "numAttr" AttrInfos.
v                Each AttrInfo will have numVal values.
  Comments    : 
***************************************************************************/
DblLinkList<AttrInfoPtr>* make_ai_list(const int numAttr, const int numVal)
{
   DblLinkList<AttrInfoPtr>* aList = new DblLinkList<AttrInfoPtr>;
   for (int i = 0; i < numAttr; i++) {
      AttrInfoPtr tmp(make_nai(numVal, i));
      aList->append(tmp);
   }
   return aList;
}


/***************************************************************************
  Description : Creates a Schema with "numAttr" AttrInfos.
                Each AttrInfo will have numVal values.
  Comments    : 
***************************************************************************/
SchemaRC make_schema(const int numAttr, const int numVal)
{
   DblLinkList<AttrInfoPtr>* aList = make_ai_list(numAttr, numVal);
   SchemaRC schema(aList);
   ASSERT(aList == NULL);
   return schema;
}


main()
{
   SchemaRC schema = make_schema(1,1);
   InstanceRC instance(schema);
   ConstCategorizer *c[NUM_CATEGORIZERS];
                             
   Mcout << "t_ConstCat executing" << endl;
   MLCOStream out1("t_ConstCat.out1");
   for (int i = 0; i < NUM_CATEGORIZERS; i++) {
      MString description(CATEGORIZER_DESCR + MString(i,0));
      
      AugCategory aca(i + OFFSET + FIRST_CATEGORY_VAL,
			   AUG_DESCR + MString(i + OFFSET,0));
      c[i] = new ConstCategorizer(description, aca);
      c[i]->display_struct(out1);
      
      MLCOStream o(XStream);
      DotGraphPref dgp; 
// The following line may cause runtime error
      TEST_ERROR("valid", c[i]->display_struct(o));
// The following line may cause runtime error
    TEST_ERROR("valid", c[i]->display_struct(out1, dgp));
     
   }
  
   for (i = 0; i < NUM_CATEGORIZERS; i++) {
      ASSERT(c[i]->num_categories() == 1);
      ASSERT(c[i]->description() == CATEGORIZER_DESCR + MString(i,0));
      ASSERT(c[i]->categorize(instance) == i + OFFSET + FIRST_CATEGORY_VAL);
   }

   // Test of operator == for different classes of categorizers.
   ASSERT((*c[0])==(*c[0]));
   ASSERT(!((*c[0])==(*c[1])));
	
   InstanceList bag1("t_ThresholdCat");
   Real val = 4.3;
   ThresholdCategorizer tc(bag1.get_schema(), 1, val,"Test1 ThresholdCat");
   ASSERT(!( *c[0] == tc));

   ASSERT( (*(Categorizer *)c[0]) == (*(Categorizer *)c[0]));
   
   InstanceList bag2("t_AttrCat");
   AttrCategorizer ac(bag2.get_schema().attr_info(2), 2,
                      "Test2 AttrCat");
   ASSERT(!(*c[0] == ac));

   for (i = 0; i < NUM_CATEGORIZERS; i++)
      delete c[i];

   return 0;
}
