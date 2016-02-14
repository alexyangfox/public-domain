// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests errors before the ConstInducer has been properly
                   initialized.
		 Reads in data from "t_ConstInducer.names" and 
		   "t_ConstInducer.data".
  Doesn't test : 
  Enhancements : Test using a test set.
  History      : Richard Long                                       8/17/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <ConstInducer.h>
#include <Instance.h>
#include <Categorizer.h>

RCSID("MLC++, $RCSfile: t_ConstInducer.c,v $ $Revision: 1.29 $")


#ifdef __OBJECTCENTER__
const int NUMATTR = 8;
const int NUMVAL = 5;
#else
const int NUMATTR = 25;
const int NUMVAL = 20;
#endif

/***************************************************************************
  Description : Allocates space for NominalAttrInfo with given name and num.
                The NominalAttrInfo has NUMVAL values.
		Returns pointer to the new NominalAttrInfo.
  Comments    :
***************************************************************************/
NominalAttrInfo* make_nai(const MString& name, const int num)
{
   DblLinkList<MString>* attrVals = new DblLinkList<MString>;
   // add NUMVAL attributes to the NominalAttrInfo
   for (int j = 0; j < NUMVAL; j++) {
      MString attr = "VAL" + MString(num,0) + "-" + MString(j,0);
      attrVals->append(attr);
   }
   NominalAttrInfo* nai = new NominalAttrInfo(name, attrVals);
   ASSERT(attrVals == NULL); // nai gets ownership
   return nai;
}


/***************************************************************************
  Description : Creates a list with "numAttr" AttrInfos.
                Each AttrInfo will have numVal values.
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
  Description : Creates an InstanceInfo with "numAttr" AttrInfos.
                Each AttrInfo will have numVal values.
  Comments    : 
***************************************************************************/
SchemaRC make_schemaRC(const int numAttr, const int numVal)
{
   DblLinkList<AttrInfoPtr>* aList = make_ai_list(numAttr, numVal);
   SchemaRC schema(aList);
   ASSERT(aList == NULL);
   return schema;
}


main()
{
  cout << "Executing t_ConstInducer" << endl;
  // Create instance info for InstanceAllocs
  
  SchemaRC schema = make_schemaRC(NUMATTR, NUMATTR);
  InstanceRC instance(schema);

  ConstInducer constInducer("Test ConstInducer");


#ifndef MEMCHECK
  TEST_ERROR("No categorizer", constInducer.predict(instance));
  TEST_ERROR("Training data has not been set", constInducer.train());
#endif

  constInducer.read_data("t_ConstInducer");

  constInducer.train();
  constInducer.display_struct();
  MLCOStream out1("t_ConstInducer.out1");
  constInducer.display_struct(out1);
  
  // This depends on the information in "t_ConstInducer.data"
  ASSERT(constInducer.predict(instance) == 3 + FIRST_CATEGORY_VAL);
  ASSERT(constInducer.get_categorizer().categorize(instance) ==
	 3 + FIRST_CATEGORY_VAL);
  return 0;
}
