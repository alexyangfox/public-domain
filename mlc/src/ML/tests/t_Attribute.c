// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test Attribute.c of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Creates an array of NUMATTR NominalAttrInfo*.
                 The ith NominalAttrInfo has the following properties:
		   attrName: "test"+i
		   attrType: nominal
		   attrNum: i
		   values: a list of i values (VAL0..VALi)
		 AttrInfo is not instantiated directly, because it is
		   a superclass of NominalInstanceInfo and RealAttrInfo,
		   which call all of AttrInfo's methods.		 
  Doesn't test : read_attr_value() (tested in t_InstList.c)
                 LinearAttrInfo
		 AttrValue.treeStructured
		 struct Literal
                 Private constructors and operator=() for RealAttrValue_
		    and NominalAttrValue_.  They are private and
		    should never be called.
		 set/get_min/max() or normalized_value() for RealAttrInfo.
		    (tested in t_InstList.c)
  Enhancements : More rigorous tests for equal() and operator==.
  History      : Brian Frasca					     1/21/94
		   Added tests for AttrInfo::display()
		 Brian Frasca					     1/14/94
		   Added test for _equal_value, is_unknown, and
		      attr_type_to_string
		 Svetlozar Nestorov                                  1/08/94
                   Added test for clone() and copy constructors
                 Richard Long                                        7/15/93
                   Initial revision
***************************************************************************/

/*
Note: When switching the storage from float to double, we should have
the output as:

t_Attribute.c executing
The value 0 is converted to 0
 by RealAttrInfo::attrValue_to_string.
The value 123.0456789 is converted to 123.0456789
 by RealAttrInfo::attrValue_to_string.
The value -9876543210.01235 is converted to -9876543210.01235
 by RealAttrInfo::attrValue_to_string.
*/

#include <basics.h>
#include <math.h>
#include <errorUnless.h>
#include <Attribute.h>
#include <iomanip.h>
#include <MLCStream.h>

RCSID("MLC++, File $Revision: 1.39 $")

#ifdef __OBJECTCENTER__
const int NUMATTR = 5;  // Number of Attributes that will be created
#else
const int NUMATTR = 50;
#endif

/***************************************************************************
  Description : Checks that attr_type_to_string() returns the expected
                   values.
  Comments    :
***************************************************************************/
void test_attr_type_to_string()
{
   AttrType attrType;
#ifndef __OBJECTCENTER__
   // ObjectCenter warns about the illegal enumeration value
   attrType = (AttrType)INT_MAX;
   TEST_ERROR("Invalid AttrType value", attr_type_to_string(attrType));
#endif
   attrType = unknown;
   ASSERT(attr_type_to_string(attrType) == "unknown");
   attrType = real;
   ASSERT(attr_type_to_string(attrType) == "real");
   attrType = nominal;
   ASSERT(attr_type_to_string(attrType) == "nominal");
   attrType = linearNominal;
   ASSERT(attr_type_to_string(attrType) == "linearNominal");
   attrType = treeStructured;
   ASSERT(attr_type_to_string(attrType) == "treeStructured");
   attrType = internalDisjunction;
   ASSERT(attr_type_to_string(attrType) == "internalDisjunction");
   attrType = userReal;
   ASSERT(attr_type_to_string(attrType) == "userReal");
   attrType = userNominal;
   ASSERT(attr_type_to_string(attrType) == "userNominal");
   attrType = userLinearNominal;
   ASSERT(attr_type_to_string(attrType) == "userLinearNominal");
   attrType = userTreeStructured;
   ASSERT(attr_type_to_string(attrType) == "userTreeStructured");
   attrType = userInternalDisjunction;
   ASSERT(attr_type_to_string(attrType) == "userInternalDisjunction");
}


/***************************************************************************
  Description : Checks the comparison operators and distance for
                   RealAttrInfo.
  Comments    :
***************************************************************************/
void check_real_comparators(RealAttrInfo& rai)
{
   RealAttrValue_ val1, val2;
   Real v1 = 1.414;
   Real v2 = 3.14159;
   rai.set_real_val(val1, v1);
   ASSERT((float)rai.get_real_val(val1) == (float)v1);
   rai.set_real_val(val2, v2);
   AttrValue_ val3 = val1; // Use copy constructor
   ASSERT(rai._equal_value(val1, val3));
   AttrValue_ val4;
   val4 = val2;            // Use operator=
   ASSERT(rai._equal_value(val2, val4));
   ASSERT(rai._equal_value(val1, val1));
   ASSERT(rai._equal_value(val2, val2));
   ASSERT(rai.less_than(val1, val2));
   ASSERT(rai.less_than_equal(val1, val2));
   ASSERT(rai.less_than_equal(val1, val1));
   ASSERT(rai.greater_than(val2, val1));
   ASSERT(rai.greater_than_equal(val2, val1));
   ASSERT(rai.greater_than_equal(val2, val2));
   ASSERT(rai.equal(val2, val2));
   ASSERT(rai.equal(val1, val1));
   ASSERT(rai.not_equal(val1, val2));
   rai.set_min(0);
   rai.set_max(1);
   ASSERT(fabs(rai.distance(val1, val2)  - (v2 -v1) < STORED_REAL_EPSILON));
   ASSERT(fabs(rai.distance(val2, val1)  - (v2 -v1) < STORED_REAL_EPSILON));
   rai.set_real_val(val2, UNKNOWN_REAL_VAL);
#ifndef MEMCHECK
   TEST_ERROR("Attribute.c::check_valid_real_comparison_operand: Comparison "
	      "methods are undefined for UNKNOWN_REAL_VAL",
	      rai.less_than(val1, val2));
#endif
}

/***************************************************************************
  Description : Checks the distance function for NominalAttrInfo.
  Comments    :
***************************************************************************/
void check_nominal_distance(const NominalAttrInfo& nai)
{
   NominalAttrValue_ val1, val2;
   nai.set_nominal_val(val1, FIRST_NOMINAL_VAL);
   nai.set_nominal_val(val2, FIRST_NOMINAL_VAL + 1);
   ASSERT(nai.distance(val1, val1) == 0);
   if (nai.num_values() > 1)
      ASSERT(nai.distance(val1, val2) == 1);
   else {
      TEST_ERROR("NominalAttrInfo::check_in_range: ",
		 nai.distance(val1, val2));
   }
}


/***************************************************************************
  Description : Creates a NominalAttrInfo with the given number of values.
  Comments    : Values will be called VAL<n>, where <n> in [0,numVals-1].
                Name is set to "test nominal <numVals>"
***************************************************************************/
NominalAttrInfo* make_nai(int numVals)
{
   DblLinkList<MString>* attrVals = new DblLinkList<MString>;
   for (int j = 0; j < numVals; j++) {
      MString val("VAL" + MString(j, 0));
      attrVals->append(val);
   }
   NominalAttrInfo* nai;
   nai = new NominalAttrInfo("test nominal" + MString(numVals, 0), attrVals);
   ASSERT(attrVals == NULL);
   return nai;
}


main()
{
   cout << "t_Attribute.c executing" << endl;
   NominalAttrInfo* nai[NUMATTR];
   RealAttrInfo* rai[NUMATTR];
   MLCOStream out1("t_Attribute.out1");
   for (int i = 1; i < NUMATTR; i++) {
      nai[i] = make_nai(i);
      ASSERT (nai[i]->name() == "test nominal" + MString(i, 0));
      ASSERT (nai[i]->num_values() == i);
      out1 << "Nominal Attribute is: " << *nai[i] << endl;
      out1 << "\tnames format: ";
      out1 << nai[i]->name() <<  ": " ;
      nai[i]->display_attr_values(out1);
      out1 << endl;

      rai[i] = new RealAttrInfo("test real" + MString(i, 0));
      ASSERT (rai[i]->name() == "test real" + MString(i, 0));
      out1 << "Real Attribute is: " << *rai[i] << endl;
      out1 << "\tnames format: ";
      out1 << rai[i]->name() << ": ";
      rai[i]->display_attr_values(out1);
      out1 << endl;

      if (i > 1)
	 check_nominal_distance(*nai[i]);
      if (i == 3) {
	 cout << setprecision(REAL_MANTISSA_LEN);
	 AttrValue_ attrValue;
	 Real val = 0;
	 rai[i]->set_unknown(attrValue);
	 ASSERT(rai[i]->is_unknown(attrValue));
	 rai[i]->set_real_val(attrValue, val);
	 cout << "The value " << val << " is converted to "
	      << rai[i]->attrValue_to_string(attrValue) << endl
	      << " by RealAttrInfo::attrValue_to_string." << endl;
	 val = 123.0456789;
	 rai[i]->set_real_val(attrValue, val);
	 cout << "The value " << val << " is converted to "
	      << rai[i]->attrValue_to_string(attrValue) << endl
	      << " by RealAttrInfo::attrValue_to_string." << endl;
	 val = -9876543210.0123456789;
	 rai[i]->set_real_val(attrValue, val);
	 cout << "The value " << val << " is converted to "
	      << rai[i]->attrValue_to_string(attrValue) << endl
	      << " by RealAttrInfo::attrValue_to_string." << endl;
	 val = UNKNOWN_REAL_VAL;
	 rai[i]->set_real_val(attrValue, val);
	 ASSERT(rai[i]->is_unknown(attrValue));
	 ASSERT(rai[i]->attrValue_to_string(attrValue) == '?');
	 NominalAttrValue_ nom1, nom2;
	 AttrValue_ nom3;
	 nai[i]->set_unknown(nom1);
	 ASSERT(nai[i]->is_unknown(nom1));
	 nai[i]->set_nominal_val(nom1, 1);
	 nai[i]->set_nominal_val(nom2, 1);
	 nai[i]->set_nominal_val(nom3, 2);
	 ASSERT(nai[i]->_equal_value(nom1, nom2));
	 NominalAttrValue_ nom4 = nom1;
	 ASSERT(nai[i]->_equal_value(nom1, nom4));
	 nom4 = nom3;
	 ASSERT(nai[i]->_equal_value(nom3, nom4));	 
	 NominalAttrValue_ nom5 = nom3;
	 ASSERT(nai[i]->_equal_value(nom3, nom5));
	 nom5 = nom1;
	 ASSERT(nai[i]->_equal_value(nom1, nom5));	 
	 ASSERT(!nai[i]->_equal_value(nom1, nom3));
	 check_real_comparators(*rai[i]);
      }
      
      for (int j = 1; j < i; j++) {
	 AttrValue_ attrValue;
	 nai[i]->set_nominal_val(attrValue, j + FIRST_NOMINAL_VAL);
	 MString val = "VAL" + MString(j, 0);
	 ASSERT (nai[i]->attrValue_to_string(attrValue) == val);
	 ASSERT (nai[i]->nominal_to_int(val) == j + FIRST_NOMINAL_VAL);
      }
   }

   // test the copy constructors
   NominalAttrInfo myNAI(*(nai[1]), ctorDummy);
   RealAttrInfo myRAI(*(rai[1]), ctorDummy);
   ASSERT(myNAI == *(nai[1]));
   ASSERT(myRAI == *(rai[1]));

   // test clone()
   AttrInfo *nominfo1 = new NominalAttrInfo(myNAI, ctorDummy);
   AttrInfo *nominfo2 = nominfo1->clone();
   ASSERT(*nominfo1== *nominfo2);
   ASSERT(*nominfo2==myNAI);
   ASSERT(myNAI==*nominfo2);
   ASSERT(*nominfo2!=*(nai[2]));
  
   AttrInfo *realinfo1 = new RealAttrInfo(myRAI, ctorDummy);
   AttrInfo *realinfo2 = realinfo1->clone();
   ASSERT(*realinfo1==*realinfo2);
   ASSERT(*realinfo2==myRAI);
   ASSERT(myRAI==*realinfo2);
   ASSERT(*realinfo2!=*(rai[2]));

   delete nominfo1;
   delete nominfo2;
   delete realinfo1;
   delete realinfo2;
   
   for (i = 1; i < NUMATTR; i++) {
      for (int j = FIRST_NOMINAL_VAL;
	   j < FIRST_NOMINAL_VAL + nai[i]->num_values(); j++) {
	 MString expected = "VAL" + MString(j - FIRST_NOMINAL_VAL, 0);
	 ASSERT (nai[i]->get_value(j) == expected);
      }
   }
   
   AttrValue_ attrValue;
   nai[NUMATTR-1]->set_nominal_val(attrValue, UNKNOWN_NOMINAL_VAL);
   ASSERT(nai[NUMATTR-1]->get_nominal_val(attrValue) == UNKNOWN_NOMINAL_VAL);
   ASSERT(nai[NUMATTR-1]->is_unknown(attrValue));
   ASSERT(nai[NUMATTR-1]->attrValue_to_string(attrValue) == "?");
   ASSERT(*nai[NUMATTR-1] == *nai[NUMATTR - 1]);
   ASSERT(*nai[1] != *nai[NUMATTR - 1]);
  
   NominalAttrInfo& nainfo = nai[NUMATTR-1]->cast_to_nominal();
   ASSERT(nainfo.equal(*nai[NUMATTR-1], TRUE));
   RealAttrInfo& rainfo = rai[NUMATTR-1]->cast_to_real();
   ASSERT(rainfo.equal(*rai[NUMATTR-1], TRUE));
   ASSERT(nai[NUMATTR-1]->can_cast_to_nominal());
   ASSERT(rai[NUMATTR-1]->can_cast_to_real());
   const NominalAttrInfo& constnainfo = nai[NUMATTR-1]->cast_to_nominal();
   ASSERT(constnainfo.equal(*nai[NUMATTR-1], TRUE));

   TEST_ERROR("NominalAttrInfo::check_in_range:", // not in range
	      nai[NUMATTR-1]->set_nominal_val(attrValue,
					      NUMATTR + FIRST_NOMINAL_VAL));
   DBGSLOW(TEST_ERROR(" must be in the range ",
	      nai[NUMATTR-1]->attrValue_to_string(attrValue)));
   MString invString = "Invalid";
   TEST_ERROR(" is not a valid attribute value for ",
	      nai[NUMATTR-1]->nominal_to_int(invString));
   ASSERT(nai[NUMATTR-1]->nominal_to_int('?') == UNKNOWN_NOMINAL_VAL);

   AttrValue_ tempVal;

   RealAttrValue_ rav;
   NominalAttrValue_ nav;
   
#ifndef MEMCHECK
   TEST_ERROR("AttrInfo::set_real_val: Cannot be called for a ",
	      nai[NUMATTR-1]->set_real_val(tempVal, 0));
   TEST_ERROR("AttrInfo::get_real_val: Cannot be called for a ",
	      nai[NUMATTR-1]->get_real_val(tempVal));
   TEST_ERROR("AttrInfo::set_nominal_val: Cannot be called for a ",
	      rai[NUMATTR-1]->set_nominal_val(tempVal, 0));
   TEST_ERROR("AttrInfo::get_nominal_val: Cannot be called for a ",
	      rai[NUMATTR-1]->get_nominal_val(tempVal));
   ASSERT(!nai[NUMATTR-1]->can_cast_to_real());
   ASSERT(!rai[NUMATTR-1]->can_cast_to_nominal());
   TEST_ERROR("AttrInfo::cast_to_nominal: Type ", NominalAttrInfo& nomCast
	      = rai[NUMATTR-1]->cast_to_nominal());
   TEST_ERROR("AttrInfo::cast_to_nominal() const: Type ",
	      const NominalAttrInfo& nomConstCast =
	      ((const AttrInfo*)rai[NUMATTR-1])->cast_to_nominal());
   TEST_ERROR("AttrInfo::cast_to_real() const: Type ", const RealAttrInfo&
	      realConstCast = ((const AttrInfo*)nai[NUMATTR-1])
	      ->cast_to_real());
   TEST_ERROR("AttrInfo::cast_to_real: Type ",
	      RealAttrInfo& realCast = nai[NUMATTR-1]->cast_to_real());
#endif
   
   for (i = 1; i < NUMATTR; i++) {
      delete nai[i];
      delete rai[i];
   }

   test_attr_type_to_string();
   
   return 0;
}
