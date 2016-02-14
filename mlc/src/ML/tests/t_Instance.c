// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test "Instance.c" of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : 
  Doesn't test : Instances/LabelledInstances with AttrInfo other than
                   NominalAttrInfo.
		 The display() routines.  They are tested in t_InstList.c
  Enhancements : 
  History      : Robert Allen                                       1/27/95
                   Added test for project().
		 James Dougherty                                    9/17/94
                   Added a new test for the set_schema() method of the
		   Instance class.
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
#include <InstanceRC.h>
#include <InstList.h>
#include <MLCStream.h>
#include <MRandom.h>
#include <SchemaRC.h>

// NUMVAL must be greater than NUMINST
#ifdef __OBJECTCENTER__
const int NUMVAL = 8;
const int NUMINST = 7;
const int NUM_SCHEMA=5;
#else
const int NUMVAL = 20;
const int NUMINST = 15;
const int NUM_SCHEMA=10;
#endif

const unsigned int RANDOM_SEED = 62830;
const int OUTER_LOOP = 50;
const int INNER_LOOP = 25;
const int DATA_SIZE = 100;

const MString defaultAttrVal("VAL");
const MString defaultAttrName("test nominal");

/***************************************************************************
  Description : Creates a NominalAttrInfo with the given number of values.
  Comments    : Values will be called VAL<n>, where <n> in [0,numVals-1].
                Name is set to "test nominal <numVals>"
***************************************************************************/
NominalAttrInfo* make_nai(int numVals, int num,
			  const MString& attrVal = defaultAttrVal,
	                  const MString& attrName = defaultAttrName)
{   DblLinkList<MString>* attrVals = new DblLinkList<MString>;
   for (int j = 0; j < numVals; j++) {
      MString val(attrVal + MString(j, 0));
      attrVals->append(val);
   }
   NominalAttrInfo* nai;
   nai = new NominalAttrInfo(attrName + MString(num, 0), attrVals);
   ASSERT(attrVals == NULL);
   return nai;
}


/***************************************************************************
  Description : Creates a list with "numAttr" AttrInfos.
                Each AttrInfo will have numVal values.
  Comments    : 
***************************************************************************/
DblLinkList<AttrInfoPtr>* make_ai_list(const int numAttr, const int numVal,
				    const MString& attrVal = defaultAttrVal,
				    const MString& attrName = defaultAttrName)
{
   DblLinkList<AttrInfoPtr>* list = new DblLinkList<AttrInfoPtr>;
   for (int i = 0; i < numAttr; i++) {
      AttrInfoPtr tmp(make_nai(numVal, i,attrVal,attrName));
      list->append(tmp);
   }
   return list;
}


/***************************************************************************
  Description : Creates a SchemaRC with "numAttr" AttrInfos.
                Each AttrInfo will have numVal values.
  Comments    :
***************************************************************************/
SchemaRC* make_schemaRC (const int numAttr, const int numVal,
			 const MString& attrVal = defaultAttrVal,
			 const MString& attrName = defaultAttrName)
{
   DblLinkList<AttrInfoPtr>* list = make_ai_list(numAttr, numVal, attrVal,
						 attrName);
   AttrInfo* ai = make_nai(numAttr, numVal,attrVal,attrName);
   SchemaRC* src = new SchemaRC(list, ai);

   return src;
}


/*****************************************************************************
  Description : Makes InstanceRC with the given schemaRC. The attribute values
                  are initially unknown.
	        Label value is not set if default argument is kept.
  Comments    :
*****************************************************************************/
InstanceRC* make_instanceRC(const SchemaRC& src, const AttrValue_* av=NULL)
{
   InstanceRC* irc;
   irc = new InstanceRC(src);

   if( av )
      irc->set_label(*av);

   return irc;
}





/*****************************************************************************
  Description  : Tests basic reference count operations.
                   Try every possible operations such as assignement,
		   delete, copy_on_write, etc.
  Comments     : To make sure there's no memory leak, it should be mlcproofed.	   
*****************************************************************************/
void instance_test()
{
   SchemaRC* srcArray[NUM_SCHEMA];
   InstanceRC* ircArray[NUM_SCHEMA];


   Mcout << "Tests on the basic operations of reference counting classes"
	 << endl;
   
   // make a bunch of InstanceRC's
   // each InstanceRC has i+1 attributes and
   // each attribute has i+1 attribute values
   
   for(int i = 0; i < NUM_SCHEMA; i++) {
      srcArray[i] = make_schemaRC(i+1,i+1);

      NominalAttrValue_ label;
      ircArray[i] = make_instanceRC(*srcArray[i]);
      ircArray[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray[i]->set_label(label);
   }


   // now delete all and see it doesn't leak
   for( i = 0; i < NUM_SCHEMA; i++) {
      delete srcArray[i];
      delete ircArray[i];
   }

   // read two InstanceRC and delete both
   SchemaRC* srcArray2[NUM_SCHEMA];
   InstanceRC* ircArray2[NUM_SCHEMA];
   for( i = 0; i < NUM_SCHEMA; i++) {
      srcArray[i] = make_schemaRC(i+1,i+1);
      srcArray2[i] = make_schemaRC(i+1,i+1);
      NominalAttrValue_ label;

      ircArray[i] = make_instanceRC(*srcArray[i]);
      ircArray[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray[i]->set_label(label);
      
      ircArray2[i] = make_instanceRC(*srcArray2[i]);
      ircArray2[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray2[i]->set_label(label);
      delete srcArray[i];
      delete srcArray2[i];
      delete ircArray[i];
      delete ircArray2[i];
   }

   // read two separate InstanceRC's and copy the first
   // to the second.
   for( i = 0; i < NUM_SCHEMA; i++) {
      srcArray[i] = make_schemaRC(i+1,i+1);
      srcArray2[i] = make_schemaRC(i+2,i+2);

      NominalAttrValue_ label;

      ircArray[i] = make_instanceRC(*srcArray[i]);
      ircArray[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray[i]->set_label(label);
      
      ircArray2[i] = make_instanceRC(*srcArray2[i]);
      ircArray2[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray2[i]->set_label(label);

      // copy the first to the second.
      *ircArray2[i] = *ircArray[i];
      ASSERT(*ircArray2[i] == *ircArray[i]);
      
      delete srcArray[i];
      delete srcArray2[i];
      delete ircArray[i];
      delete ircArray2[i];
   }

   // same as the above but update one of them
   for( i = 0; i < NUM_SCHEMA; i++) {
      srcArray[i] = make_schemaRC(i+1,i+1);
      srcArray2[i] = make_schemaRC(i+2,i+2);

      NominalAttrValue_ label;

      ircArray[i] = make_instanceRC(*srcArray[i]);
      ircArray[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray[i]->set_label(label);
      
      ircArray2[i] = make_instanceRC(*srcArray2[i]);
      ircArray2[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray2[i]->set_label(label);

      // copy the first to the second.
      *ircArray2[i] = *ircArray[i];
      ASSERT(*ircArray2[i] == *ircArray[i]);
      // now update one of them
      ircArray[i]->set_weight(23.2);
      ASSERT(ircArray2[i]->get_weight() == 1.0);
      
      delete srcArray[i];
      delete srcArray2[i];
      delete ircArray[i];
      delete ircArray2[i];
   }

   // same as the above but  make copy, delete first, then update.
   for( i = 0; i < NUM_SCHEMA; i++) {
      srcArray[i] = make_schemaRC(i+1,i+1);
      srcArray2[i] = make_schemaRC(i+2,i+2);

      NominalAttrValue_ label;

      ircArray[i] = make_instanceRC(*srcArray[i]);
      ircArray[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray[i]->set_label(label);
      
      ircArray2[i] = make_instanceRC(*srcArray2[i]);
      ircArray2[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray2[i]->set_label(label);

      // copy the first to the second.
      *ircArray2[i] = *ircArray[i];
      // delete the first one
      delete ircArray[i];
      // now update one of them
      ircArray2[i]->set_weight(23.2);

      delete srcArray[i];
      delete srcArray2[i];
      delete ircArray2[i];
   }

   // Imagine that there are two schemas: A and B.
   // Make copies A1 and A2 of schema A.  Make copies B1 and B2 of B.
   // Delete A, A1, and B.
   SchemaRC* srcArray3[NUM_SCHEMA];
   SchemaRC* srcArray4[NUM_SCHEMA];
   SchemaRC* srcArray5[NUM_SCHEMA];
   SchemaRC* srcArray6[NUM_SCHEMA];   
   InstanceRC* ircArray3[NUM_SCHEMA];   
   InstanceRC* ircArray4[NUM_SCHEMA];   
   InstanceRC* ircArray5[NUM_SCHEMA];   
   InstanceRC* ircArray6[NUM_SCHEMA];

   for( i = 0; i < NUM_SCHEMA; i++) {
      srcArray[i] = make_schemaRC(i+1,i+1);
      srcArray2[i] = make_schemaRC(i+2,i+2);
      srcArray3[i] = make_schemaRC(i+3,i+3);
      srcArray4[i] = make_schemaRC(i+1,i+1);
      srcArray5[i] = make_schemaRC(i+2,i+2);
      srcArray6[i] = make_schemaRC(i+3,i+3);
      
      NominalAttrValue_ label;

      ircArray[i] = make_instanceRC(*srcArray[i]);
      ircArray[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray[i]->set_label(label);
      
      ircArray2[i] = make_instanceRC(*srcArray2[i]);
      ircArray2[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray2[i]->set_label(label);
      
      ircArray3[i] = make_instanceRC(*srcArray3[i]);
      ircArray3[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray3[i]->set_label(label);
      
      ircArray4[i] = make_instanceRC(*srcArray4[i]);
      ircArray4[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray4[i]->set_label(label);

      ircArray5[i] = make_instanceRC(*srcArray5[i]);
      ircArray5[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray5[i]->set_label(label);
      
      ircArray6[i] = make_instanceRC(*srcArray6[i]);
      ircArray6[i]->label_info().set_nominal_val(label, i +
						UNKNOWN_CATEGORY_VAL);
      ircArray6[i]->set_label(label);

      delete ircArray[i];
      delete ircArray2[i];
      delete ircArray4[i];
      delete srcArray[i];
      delete srcArray2[i];
      delete srcArray4[i];      

      delete srcArray3[i];
      delete srcArray5[i];
      delete srcArray6[i];
      delete ircArray3[i];
      delete ircArray5[i];
      delete ircArray6[i];            
   }   
}



/*****************************************************************************
  Description  : Tests thoroughly the operations of InstanceRC class
                      by performing random operations.
  Comments     :
*****************************************************************************/  
void random_test_irc()
{

   Mcout << "Random Test on InstanceRC" << endl;
   MRandom randGen(RANDOM_SEED);

   // Initialize an array of 100 InstanceRC's such that each element
   // has distinct values.
   // Also initialize integer array which is parallel to InstanceRC array
   InstanceRC* ircArray[DATA_SIZE];
   int intArray[DATA_SIZE];

   for( int i=0; i < DATA_SIZE; i++) {
      SchemaRC* src = make_schemaRC(1,3);
      ircArray[i] = make_instanceRC(*src);
      intArray[i] = i%3;
      NominalAttrValue_ av;
      ircArray[i]->attr_info(0).set_nominal_val(av, FIRST_NOMINAL_VAL
						+ intArray[i]);
      (*ircArray[i])[0] = av;
      
      // all the instances have the same label value but they have
      // one of three distinct attribute values.
      AttrValue_ label;
      ircArray[i]->label_info().set_nominal_val(label, 1 +
						UNKNOWN_CATEGORY_VAL);
      ircArray[i]->set_label(label);

      // check buntine_display and display
      if( i % DATA_SIZE/5 == 0 ) {
	 Mcout << *ircArray[i];
	 ircArray[i]->buntine_display();
      }

      delete src;
   }


   for( i =0; i < OUTER_LOOP; i++) {
      // pick the two arbitrary numbers.
      int num1 = randGen.integer(DATA_SIZE);
      int num2 = randGen.integer(DATA_SIZE);

      *ircArray[num1] = *ircArray[num2];
      intArray[num1] = intArray[num2];

      for(int j=0; j < INNER_LOOP; j++) {
	 int num3 = randGen.integer(DATA_SIZE);
	 // the operation causing a copy-on-write
         ircArray[num3]->set_weight(12.2);
      }
   }

   
   for( int instanceNum = 0; instanceNum < 3; instanceNum++ ) {
      InstanceRC* irc=NULL;
      for(int j = 0; j < DATA_SIZE; j++)
	 if(intArray[j] == instanceNum) 
	    if( irc == NULL )
	       irc = ircArray[j];
            else 
	       ASSERT(*irc == *ircArray[j]);
   }

   // now delete ircArray
   for( i = 0; i < DATA_SIZE; i++)
      delete ircArray[i];   
   Mcout << "OK." << endl;
}


/*****************************************************************************
  Description  : Tests the basic operations of Instance and Schema
                    classes.
  Comments     : No reference count specific tests are intended. They
                    are tested in the other test procedures.
*****************************************************************************/
void basic_test()
{
   InstanceRC*    irc[NUMINST];
   MRandom randGen(RANDOM_SEED);

   for (int i = 1; i < NUMINST; i++) {
      SchemaRC* src = make_schemaRC(i, NUMVAL);

      irc[i] = make_instanceRC(*src);
      if (irc[i]->get_schema() != *src)
	 err << "t_Instance.c: The Schema does not match the one "
	        "passed to the constructor for irc[" << i << "]"
	     << fatal_error;
      
      AttrValue_ label;
    
      irc[i]->label_info().set_nominal_val(label, i +
					   UNKNOWN_CATEGORY_VAL);
      irc[i]->set_label(label);
      ASSERT(irc[i]->label_info()._equal_value(irc[i]->get_label(), label));

      irc[i]->set_weight(123.456789);
      ASSERT(irc[i]->get_weight() == 123.456789);


      // remove the last attribute.
      if( i > 1 ) {
	 InstanceRC newIrc(irc[i]->remove_attr(i-1));
	 for(int j=0; j < i-1; j++)
	    ASSERT(newIrc.attr_info(j) == irc[i]->attr_info(j));
      }

      // remove the first attribute.
      if( i > 1 ) {
	 InstanceRC newIrc(irc[i]->remove_attr(0));
	 for(int j=0; j < i-1; j++)
	    ASSERT(newIrc.attr_info(j) == irc[i]->attr_info(j+1));
      }      


      // check the number of attribute(label) values.
      ASSERT( irc[i]->num_attr_values(0) == NUMVAL );
      ASSERT( irc[i]->num_label_values() == i );

      // check attr_name method
      ASSERT( irc[i]->attr_name(0) == "test nominal0");
      
      // Checking operator[]
      for (int j = 1; j <= i; j++) {
	 InstanceRC& instRC = *irc[i];
	 instRC.attr_info(j-1).set_nominal_val(instRC[j-1], i - j);
      }

      delete src;
      delete irc[i];
   }
   
   Mcout << "OK." << endl;
   
}

void test_set_schema()
{

   SchemaRC* schema = make_schemaRC(3,3,"Nominal-", "Name-");
   InstanceRC* instance = make_instanceRC(*schema);
   
   // Set attributes 0 and 1 to have 3rd value, attribute 2 to 2nd value.
   // Label to 1st value
   NominalAttrValue_ av;
   instance->attr_info(0).set_nominal_val(av, 2);
   (*instance)[0] = av;
   (*instance)[1] = av;
   instance->attr_info(2).set_nominal_val(av, 1);   
   (*instance)[2] = av;
   
   instance->label_info().set_nominal_val(av, 0);
   instance->set_label(av);

   Mcout << "Instance before change ..." << endl;
   Mcout << *instance << endl;
   SchemaRC* newSchema = make_schemaRC(3,3,"Alt-Nominal-", "Alt-Name-");
   instance->set_schema(*newSchema);
   Mcout << "Instance after change ..." << endl;
   Mcout << *instance << endl;

   delete newSchema;
   delete instance;
   delete schema;
}

void test_project() {
   int i, j;
   Mcout << endl << "Testing project()" << endl;
   InstanceList file("monk2");
   SchemaRC schema = file.get_schema();
   Array<int> iMask(0, schema.num_attr(), 0);

   int numProj = 0;
   for (i=1; i < schema.num_attr(); i += 2) {
      iMask[i] = 1;
      numProj++;
   }
   BoolArray mask(iMask);
   SchemaRC shortSchema = schema.project(mask);
   
   for (Pix pix = file.first(); pix; file.next(pix)) {
      const InstanceRC& inst = file.get_instance(pix);
      InstanceRC shortInst = inst.project(shortSchema, mask);
      for (i=1, j=0; j < numProj; i += 2, j++) {
	 const AttrInfo& fullAInfo = schema.attr_info(i);
	 const AttrInfo& shortAInfo = shortSchema.attr_info(j);
	 ASSERT( fullAInfo.get_nominal_val(inst[i]) ==
		 shortAInfo.get_nominal_val(shortInst[j]) );
      }
   }
   Mcout << "  project() OK." << endl;
      
}

int main()
{

   basic_test();
   instance_test();
   random_test_irc();
   test_set_schema();
   test_project();
   
   return 0;
}

