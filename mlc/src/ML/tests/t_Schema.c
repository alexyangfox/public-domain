// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test "Schema.c" of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : 
  Doesn't test : Instances/LabelledInstances with AttrInfo other than
                   NominalAttrInfo.
		 The display() routines.  They are tested in t_InstList.c
  Enhancements : 
  History      : Robert Allen                                       1/25/95
                   Added test for project().
                 Chia-Hsin Li                                       9/30/94
                   Added test for display_names().
                 YeoGirl Yun                                        5/30/94
                   Modified existing testers for the reference count class.
		   Initial revison for the reference count class.
                 Richard Long                                       1/20/94
                   Added tests for weighted instances.
                 Svetlozar Nestorov                                 1/09/94
                   Added tests for the copy constructors.
                 Richard Long                                       7/29/93
                   Added tests for InstanceAlloc and LabelledInstanceAlloc
                 Richard Long                                       7/27/93
                   Made changes to reflect new definition of Instance and
                     LabelledInstance.  Added tests for NULL pointers.
                 Richard Long                                       7/22/93
                   Updated to reflect use of _Instance::member 
		     instead of operator[]
                 Richard Long                                       7/20/93
                   Added error tests using TEST_ERROR.
                 Richard Long                                       7/15/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <mlcIO.h>
#include <SchemaRC.h>
#include <Schema.h>
#include <MLCStream.h>
#include <MRandom.h>
#include <InstList.h>

// NUMVAL must be greater than NUMINST
#ifdef __OBJECTCENTER__
const int NUMVAL = 5;
const int NUMINST = 4;
const int NUM_SCHEMA=3;
#else
const int NUMVAL = 10;
const int NUMINST = 11;
const int NUM_SCHEMA=12;
#endif

const unsigned int RANDOM_SEED = 62830;
const int OUTER_LOOP = 50;
const int INNER_LOOP = 25;
const int DATA_SIZE = 100;



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
   nai = new NominalAttrInfo("test nominal" + MString(num, 0),
			     attrVals);
   ASSERT(attrVals == NULL);
   return nai;
}


/***************************************************************************
  Description : Creates a list with "numAttr" AttrInfos.
                Each AttrInfo will have numVal values.
  Comments    : 
***************************************************************************/
DblLinkList<AttrInfoPtr>* make_ai_list(const int numAttr, const int numVal)
{
   DblLinkList<AttrInfoPtr>* list = new DblLinkList<AttrInfoPtr>;
   for (int i = 0; i < numAttr; i++) {
      AttrInfoPtr tmp(make_nai(numVal, i));
      list->append(tmp);
   }
   return list;
}


/***************************************************************************
  Description : Creates an SchemaRC with "numAttr" AttrInfos.
                Each AttrInfo will have numVal values.
  Comments    :
***************************************************************************/
SchemaRC* make_schemaRC (const int numAttr, const int numVal)
{
   DblLinkList<AttrInfoPtr>* list = make_ai_list(numAttr, numVal);
   AttrInfo* ai = make_nai(numAttr, numVal);
   SchemaRC* src = new SchemaRC(list, ai);

   return src;
}




/*****************************************************************************
  Description  : Tests basic reference count operations.
  Comments     : To make sure there's no memory leak, it should be mlcproofed.	   
*****************************************************************************/
void schema_test()
{
   SchemaRC* srcArray[NUM_SCHEMA];
 
   // make a bunch of schemaRC's
   // each schema has i+1 attributes and
   // each attribute has i+1 attribute values
   
   for(int i = 0; i < NUM_SCHEMA; i++)  
      srcArray[i] = make_schemaRC(i+1,i+1);

   // now test schema display.
   for( i = 0; i < NUM_SCHEMA; i++ ) {
      if( i % NUM_SCHEMA/5 == 0 ) {
	 Mcout << *srcArray[i] << endl;
	 Mcout << "\n" << endl;
      }
   }

   // now delete all and see it doesn't leak
   for( i = 0; i < NUM_SCHEMA; i++)
      delete srcArray[i];

   // read two separate schemaRC and delete both
   SchemaRC* srcArray2[NUM_SCHEMA];
   for( i = 0; i < NUM_SCHEMA; i++) {
      srcArray[i] = make_schemaRC(i+1,i+1);
      srcArray2[i] = make_schemaRC(i+1,i+1);

      delete srcArray[i];
      delete srcArray2[i];
   }

   // read two separate schemaRC's and copy the first
   // to the second.
   for( i = 0; i < NUM_SCHEMA; i++) {
      srcArray[i] = make_schemaRC(i+1,i+1);
      srcArray2[i] = make_schemaRC(i+2,i+2);

      // copy the first to the second.
      *srcArray2[i] = *srcArray[i];
      ASSERT(*srcArray2[i] == *srcArray[i]);
      
      delete srcArray[i];
      delete srcArray2[i];
   }


   // Imagine that there are two schemas: A and B.
   // Make copies A1 and A2 of schema A.  Make copies B1 and B2 of B.
   // Delete A, A1, and B.
   SchemaRC* srcArray3[NUM_SCHEMA];
   SchemaRC* srcArray4[NUM_SCHEMA];
   SchemaRC* srcArray5[NUM_SCHEMA];
   SchemaRC* srcArray6[NUM_SCHEMA];   

   for( i = 0; i < NUM_SCHEMA; i++) {
      srcArray[i] = make_schemaRC(i+1,i+1);
      srcArray2[i] = make_schemaRC(i+2,i+2);
      srcArray3[i] = make_schemaRC(i+3,i+3);
      srcArray4[i] = make_schemaRC(i+1,i+1);
      srcArray5[i] = make_schemaRC(i+2,i+2);
      srcArray6[i] = make_schemaRC(i+3,i+3);
      *srcArray2[i] = *srcArray[i];
      ASSERT(*srcArray2[i] == *srcArray[i]);
      *srcArray3[i] = *srcArray[i];
      ASSERT(*srcArray3[i] == *srcArray[i]);
      *srcArray5[i] = *srcArray4[i];
      ASSERT(*srcArray5[i] == *srcArray[i]);
      *srcArray6[i] = *srcArray5[i];
      delete srcArray[i];
      delete srcArray2[i];
      delete srcArray4[i];

      delete srcArray3[i];
      delete srcArray5[i];
      delete srcArray6[i];
   }   
}




/*****************************************************************************
  Description  : Tests thoroughly the operations of SchemaRC class by
                    performing random operations.
  Comments     : If any method causing a copy-on_write is added to
                    Schema, it should be substituted for remove_attr()
		    used as a tentative operation causing a
		    copy-on-write in the current test.
*****************************************************************************/  
void random_test_src()
{

   Mcout << "Random Test on SchemaRC" << endl;
   MRandom randGen(RANDOM_SEED);

   // Initialize an array of 100 schemaRC's such that each element
   // has distinct values.
   // Also initialize integer array which is parallel to schemaRC array
   SchemaRC* srcArray[DATA_SIZE];
   int intArray[DATA_SIZE];
   for( int i=0; i < DATA_SIZE; i++) {
      if( i%3 == 0 ) {
	 srcArray[i] = make_schemaRC(3,3);
	 intArray[i] = 0;
      }
      else if(i%3 == 1) {
	 srcArray[i] = make_schemaRC(4,4);
	 intArray[i] = 1;
      }
      else {
	 srcArray[i] = make_schemaRC(5,5);
	 intArray[i] = 2;
      }
   }
   
   for( i =0; i < OUTER_LOOP; i++) {
      // pick the two arbitrary numbers.
      int num1 = randGen.integer(DATA_SIZE);
      int num2 = randGen.integer(DATA_SIZE);
      *srcArray[num1] = *srcArray[num2];
      intArray[num1] = intArray[num2];

      for(int j=0; j < INNER_LOOP; j++) {
	 int num3 = randGen.integer(DATA_SIZE);
	 // the operation causing a copy-on-write
	 if( srcArray[num3]->num_attr() > 3 )
	    *srcArray[num3] = srcArray[num3]->remove_attr(0);
      }
   }

   for( int schemaNum = 0; schemaNum < 3; schemaNum++ ) {
      SchemaRC* src=NULL;
      for(int j = 0; j < DATA_SIZE; j++)
	 if(intArray[j] == schemaNum)
	    if( src == NULL )
	       src = srcArray[j];
            else
	       ASSERT(*src == *srcArray[j]);
   }

   // now delete srcArray
   for( i = 0; i < DATA_SIZE; i++)
      delete srcArray[i];
   
   Mcout << "OK." << endl;
}



/*****************************************************************************
  Description  : Tests the basic operations(not reference-count ones)
                    of Schema classes.
  Comments     : No reference count specific tests are intended. They
                    are tested in the other test procedures.
*****************************************************************************/
void basic_test()
{
   MRandom randGen(RANDOM_SEED);

   for (int i = 1; i < NUMINST; i++) {
      SchemaRC* src = make_schemaRC(i, NUMVAL);
      // make a schema without label.
      DblLinkList<AttrInfoPtr>* list = make_ai_list(i, NUMVAL);
      SchemaRC srcWithoutLabel(list);
      // compare the two schema.
      ASSERT(srcWithoutLabel != *src);

      // try copy constructor of Schema.
      DblLinkList<AttrInfoPtr>* list2 = make_ai_list(i, NUMVAL);
      Schema schema(list2);
      Schema schema2(schema, ctorDummy);
      ASSERT(schema == schema2);
      
      // remove the last attribute.
      if( i > 1 ) {
	 SchemaRC newSrc = src->remove_attr(i-1);
	 for(int j=0; j < i-1; j++)
	    ASSERT( newSrc.attr_info(j) == src->attr_info(j));
      }

      // remove the first attribute.
      if( i > 1 ) {
	 SchemaRC newSrc = src->remove_attr(0);
	 for(int j=0; j < i-1; j++)
	    ASSERT( newSrc.attr_info(j) == src->attr_info(j+1));
      }

      
      // check the number of attribute(label) values.
      ASSERT( src->num_attr_values(0) == NUMVAL );
      ASSERT( src->num_label_values() == i );

      // check attr_name method
      ASSERT( src->attr_name(0) == "test nominal0");
      
      delete src;

   }
   Mcout << "OK." << endl;
}

void display_name_test()
{
   InstanceList file("monk2");
   SchemaRC src = file.get_schema();
   
   file.display_names(Mcout, FALSE, "The name file of monk2");
}

void project_test()
{
   int i, j;
   InstanceList file("monk2");
   SchemaRC src = file.get_schema();
   Array<int> iMask(0, src.num_attr(), 0);

   int numProj = 0;
   for (i=1; i < src.num_attr(); i += 2) {
      iMask[i] = 1;
      numProj++;
   }
   BoolArray mask(iMask);
   SchemaRC out = src.project(mask);
   ASSERT( out.num_attr() == numProj );
   ASSERT( out.label_info() == src.label_info() );
   for (i=1, j=0; j < numProj; i += 2, j++) 
      ASSERT(src.attr_info(i) == out.attr_info(j));
   Mcout << endl << "project(): " << endl << "   The source schema "
	 << src << endl << "   Projected every other " << out << endl;
}

main()
{
   basic_test();
   schema_test();
   random_test_src();
   display_name_test();
   project_test();
   return 0;
}

