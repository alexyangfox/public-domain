// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests LabelledInstanceGenerator and
                   IndependentLabInstGenerator as well as the
		   supporting functor classes in LabInstGenFunct.c.
		 Also tests the functions data_generator and
		   full_space_generator. 
  Doesn't test :
  Enhancements : Test data_generator on more data spaces. 
  History      : Svetlozar Nestorov                              1/24/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <LabInstGen.h>
#include <InstList.h>
#include <TableCat.h>
#include <CatTestResult.h>
#include <AccEstDispatch.h>
#include <IBInducer.h>


RCSID("MLC++, $RCSfile: t_LabInstGen.c,v $ $Revision: 1.14 $")

const int NUM_INSTANCES1 = 1000;
// Note that NUM_INSTANCES2 should not be changed because we want to generate
//   the whole space with no duplication using NUM_INSTANCES2 (equal to the
//   number of different instances in the whole space).
const int NUM_INSTANCES2 = 64;
const int NUM_INSTANCES3 = 1000;
const int SEED1 = 13;
const Real NOISE_RATE = 0.05;


// Generates boolean labels. Acts like a 2->4 multiplexer.
// The instance should have at least 6 attributes; the first
//   two are the address bits and the next four are the data bits.
class Mux2to4Functor : public LabelGenFunctor {

public:
   Mux2to4Functor() : LabelGenFunctor() {}
   virtual void operator() (const InstanceRC& instance, AttrValue_& label,
			    AttrValue_& noisyLabel);
};

void Mux2to4Functor::operator() (const InstanceRC& instance, AttrValue_& label,
				 AttrValue_& noisyLabel)
{
   int dataBitNum = instance.attr_info(1).get_nominal_val(instance[1])+
		     2*(instance.attr_info(0).get_nominal_val(instance[0]));
   label = instance[2+dataBitNum];
   // Here we're assuming that FIRST_NOMINAL_VAL is 0
   instance.label_info().set_nominal_val(noisyLabel,
		(instance.label_info().get_nominal_val(label) + 1) % 2);
}

class MofNFunctor : public LabelGenFunctor {
public:
   MofNFunctor() : LabelGenFunctor() {}
   virtual void operator() (const InstanceRC& instance, AttrValue_& label,
			    AttrValue_& noisyLabel);
};

// mofn of bits 2-7 (6 bits)
void MofNFunctor::operator() (const InstanceRC& instance, AttrValue_& label,
				 AttrValue_& noisyLabel)
{
   int sum = 0;
   for (int i = 2; i < 8; i++)
      sum += instance.attr_info(i).get_nominal_val(instance[i]);
      
   int b = (sum >= 3);
   instance.label_info().set_nominal_val(label, b);
   instance.label_info().set_nominal_val(noisyLabel, 1 - b);
}


// It's a zero in the first 10 bits
class BigOrFunctor : public LabelGenFunctor {
public:
   BigOrFunctor() : LabelGenFunctor() {}
   virtual void operator() (const InstanceRC& instance, AttrValue_& label,
			    AttrValue_& noisyLabel);
};

void BigOrFunctor::operator() (const InstanceRC& instance, AttrValue_& label,
				 AttrValue_& noisyLabel)
{
   Bool foundZero = FALSE;
   for (int i = 0; i < 10; i++)
      if (instance.attr_info(i).get_nominal_val(instance[i]) == 0)
	 foundZero = TRUE;

   instance.label_info().set_nominal_val(label, foundZero);
   instance.label_info().set_nominal_val(noisyLabel, 1 - foundZero);
}


// One or two 
class OneOrTwoFunctor : public LabelGenFunctor {
public:
   OneOrTwoFunctor() : LabelGenFunctor() {}
   virtual void operator() (const InstanceRC& instance, AttrValue_& label,
			    AttrValue_& noisyLabel);
};

void OneOrTwoFunctor::operator() (const InstanceRC& instance,AttrValue_& label,
				 AttrValue_& noisyLabel)
{
   int sum = 0;
   for (int i = 2; i < 8; i++)
      sum += instance.attr_info(i).get_nominal_val(instance[i]);
      
   int b = (sum >= 3);
   instance.label_info().set_nominal_val(label, b);
   instance.label_info().set_nominal_val(noisyLabel, 1 - b);
}



// used for error messages testing
class DummyFunctor : public LabelGenFunctor {

public:
   DummyFunctor() : LabelGenFunctor() {}
   virtual void operator() (const InstanceRC& instance, AttrValue_& label,
			    AttrValue_& noisyLabel);
};

void DummyFunctor::operator() (const InstanceRC& instance,
			       AttrValue_& label, AttrValue_& noisyLabel)
{
   instance.label_info().set_nominal_val(label, FIRST_NOMINAL_VAL);
   instance.label_info().set_nominal_val(noisyLabel, FIRST_NOMINAL_VAL + 1);
}


/***************************************************************************
  Description : Creates a NominalAttrInfo with the given number of values.
  Comments    : Values will be called VAL<n>, where <n> in [0,numVals-1].
                Name is set to "test nominal <numVals>"
***************************************************************************/
static NominalAttrInfo* make_nai(int numVals, int num)
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
                Each AttrInfo will have numVal values.
  Comments    : 
***************************************************************************/
static DblLinkList<AttrInfoPtr>*
make_ai_list(const int numAttr, const int numVal)
{
   DblLinkList<AttrInfoPtr>* aList = new DblLinkList<AttrInfoPtr>;
   for (int i = 0; i < numAttr; i++) {
      AttrInfoPtr tmp(make_nai(numVal, i));
      aList->append(tmp);
   }
   return aList;
}


/***************************************************************************
  Description : Creates an LabelledInstanceInfo with "numAttr" AttrInfos.
                Each AttrInfo will have numVal values.
  Comments    :
***************************************************************************/
static SchemaRC  make_schema(const int numAttr, const int numVal)
{
   DblLinkList<AttrInfoPtr>* aList = make_ai_list(numAttr, numVal);
   AttrInfo* ai = make_nai(numAttr, numAttr);
   SchemaRC schema(aList, ai);
   ASSERT(aList == NULL);
   ASSERT(ai == NULL);
   return schema;
}


/***************************************************************************
  Description : Creates a new bag using "t_LabInstGen1.names" file and
                  returns a pointer to it. 
                The new bag contains numInstances which are generated
		  by data_generator with allowDup and noiseRate.
  Comments    : The caller gets ownership of the bag.
***************************************************************************/
static InstanceBag *muxcheck(int numInstances, Bool allowDup,
			      Real noiseRate = 0)
{     
   // initialize a bag to be passed to the data_generator according to
   //   the names file LabInstGen1.names (2 to 4 multiplexer).
   InstanceBag *testBag = new InstanceList("t_LabInstGen1", argDummy);
   Mux2to4Functor muxFunctor;
   IndependentLabInstGenerator labInstGen(testBag->get_schema(), muxFunctor);
   // test a couple of one liners
   labInstGen.set_seed(SEED1);
   AttrGenFunctor* uag = new UniformAttrGenFunctor
				           (labInstGen.get_rand_num_gen());
   labInstGen.set_attr_generator(0, uag);
   labInstGen.set_label_noise(noiseRate);
   data_generator(*testBag, numInstances, labInstGen, allowDup);
   return testBag;
}


static void test_error_msgs()
{
   ArgDummy dummyArg = ArgDummy(0);
   // Iniatilize a very simple bag which is used in testing error
   //   messages according to the names file "t_LabInstGen2.names".
   InstanceBag *errorBag = new InstanceList("t_LabInstGen2", dummyArg);
   DummyFunctor dummyFunctor;
   IndependentLabInstGenerator errorGen(errorBag->get_schema(), dummyFunctor);
   errorGen.set_label_noise(NOISE_RATE);
   DblLinkList<MString> *valList = new DblLinkList<MString>();
   valList->append("0");
   AttrInfo *phonyAI = new NominalAttrInfo("PHONY", valList);
   
   #ifndef MEMCHECK
     TEST_ERROR("is out of range(0-1)", errorGen.set_label_noise(200.0));
     TEST_ERROR("IndependentLabInstGenerator::gen_attr_value: The AttrInfo",
   		errorGen.gen_attr_value(*phonyAI, 1));
     TEST_ERROR("is greater than 0 and the duplication mode is turned off",
		data_generator(*errorBag, 1, errorGen, FALSE));
     AttrGenFunctor* nag = NULL;
     TEST_ERROR("AttrGenFunctor passed in is NULL",
		errorGen.set_attr_generator(0, nag));
   #endif
   
   delete phonyAI;
   delete errorBag;
}

static void full_space_test()
{
   // run full_space_generator on a space of six boolean attributes
   // note that the bag is exct copy of the one used in muxcheck
   MString val0("0");
   MString val1("1");
   SchemaRC schema = make_schema(6, 2);

   InstanceBag *testBag = new InstanceBag(schema);
   full_space_generator(testBag);
   MString file("fullspace.out");
   MLCOStream fs(file, FileStream);
   fs << *testBag;
   delete testBag;
   MString cmd("sort fullspace.out | uniq > t_LabInstGen.out1");
   if (system(cmd))
      err << "t_LabInstGen.c::full_space_test: system call '" << cmd
	  << "' returned bad status"  << fatal_error;
}

static void skewed_attr_test()
{
   InstanceList bag("t_LabInstGen3", argDummy);
   MofNFunctor mofNFunctor;
   IndependentLabInstGenerator labInstGen(bag.get_schema(), mofNFunctor);
   labInstGen.set_seed(9333126);
   AttrGenFunctor* sa = new 
      SkewedAttrGenFunctor(labInstGen.get_rand_num_gen(), 0.9);
   labInstGen.set_attr_generator(0, sa);
   AttrGenFunctor* sa2 = new 
      SkewedAttrGenFunctor(labInstGen.get_rand_num_gen(), 0.9);
   labInstGen.set_attr_generator(1, sa2);
   data_generator(bag, 1000, labInstGen, TRUE);

   MLCOStream data("t_LabInstGen.out2");
   data << bag << endl;

   InstanceList bag2("t_LabInstGen3", argDummy);
   BigOrFunctor bigOrFunctor;
   IndependentLabInstGenerator labInstGen2(bag2.get_schema(), bigOrFunctor);
   labInstGen2.set_seed(9333126);
   for (int i = 0; i < 10; i++) {
      AttrGenFunctor* s = new 
	 SkewedAttrGenFunctor(labInstGen2.get_rand_num_gen(), 0.05);
      labInstGen2.set_attr_generator(i, s);
   }
   data_generator(bag2, 1000, labInstGen2, TRUE);
   MLCOStream data2("t_LabInstGen.out3");
   data2 << bag2 << endl;
}

// mod checks the first attribute.  The label is 0 or 1 depending
// on the value mod 2.
class ModFunctor : public LabelGenFunctor {
public:
   ModFunctor() : LabelGenFunctor() {}
   virtual void operator() (const InstanceRC& instance, AttrValue_& label,
			    AttrValue_& noisyLabel);
};

void ModFunctor::operator() (const InstanceRC& instance,AttrValue_& label,
				 AttrValue_& noisyLabel)
{
   int b = instance.attr_info(0).get_nominal_val(instance[0]) % 2;
   instance.label_info().set_nominal_val(label, b);
   instance.label_info().set_nominal_val(noisyLabel, 1 - b);
}


static void mod_test()
{
   InstanceList bag("t_LabInstGen4", argDummy);
   ModFunctor modFunctor;
   IndependentLabInstGenerator labInstGen(bag.get_schema(), modFunctor);
   labInstGen.set_seed(9333126);

   data_generator(bag, 1000, labInstGen, TRUE);
   MLCOStream data("t_LabInstGen.out4");
   data << bag << endl;
}
   

// Helper function copied from IBCategorizer.
static Real distance(const InstanceRC& inst1, const InstanceRC& inst2,
		     const Array<Real>& weightVector)
{
   Real dist = 0;
   const SchemaRC& schema = inst1.get_schema();
   int  numAttr = schema.num_attr();
   for (int i = 0; i < numAttr; i++)
	 dist += squareReal(weightVector.index(i) *
			    schema.attr_info(i).distance(inst1[i], inst2[i])); 
   return dist;
}

const int protoNumAttr = 5;

// Two prototypes at (.25,....25) and (.75,....75)
class ProtoFunctor : public LabelGenFunctor {
   InstanceRC protoZero;
   InstanceRC protoOne;
   Array<Real> weightVector;
public:
   ProtoFunctor(const SchemaRC& schema, const Array<Real>& weights);
   virtual void operator() (const InstanceRC& instance, AttrValue_& label,
			    AttrValue_& noisyLabel);
};

ProtoFunctor::ProtoFunctor(const SchemaRC& schema, const Array<Real>& weights)
 : LabelGenFunctor(),
   protoZero(schema),
   protoOne(schema),
   weightVector(weights, ctorDummy)
{
   AttrValue_ label;
   protoOne.get_schema().label_info().set_unknown(label);
   protoOne.set_label(label);
   protoZero.set_label(label);

   for (int attrNum = 0; attrNum < schema.num_attr(); attrNum++) {
      const AttrInfo& ai = schema.attr_info(attrNum);
      RealAttrInfo& rai = ((AttrInfo&)ai).cast_to_real();
      rai.set_max(1);
      rai.set_min(0);
   }
   for (int i = 0; i < protoNumAttr; i++) {
      Real x = 0.25;
      Real y = 0.75;
      protoZero.attr_info(i).set_real_val(protoZero[i], x);
      protoOne.attr_info(i).set_real_val(protoOne[i], y);
   }
   Mcout << "Proto zero is:" << protoZero << endl;
   Mcout << "Proto one is:" << protoOne << endl;
}

void ProtoFunctor::operator() (const InstanceRC& instance,AttrValue_& label,
				 AttrValue_& noisyLabel)
{
   Real dist0 = distance(protoZero, instance, weightVector);
   Real dist1 = distance(protoOne, instance,  weightVector);
   int b = (dist1 > dist0);
   instance.label_info().set_nominal_val(label, b);
   instance.label_info().set_nominal_val(noisyLabel, 1 - b);
}


static void est_accuracy(const Array<Real>& weights, const MString& fileName)
{
   AccEstDispatch accEst;
   accEst.set_accuracy_estimator(AccEstDispatch::holdOut);
   accEst.set_holdout_times(2);
   accEst.set_holdout_number(100);
   accEst.set_seed(7258789);

   IBInducer ib("IB");
   InstanceList il("", "t_LabInstGen5.names", fileName);
   Real acc = accEst.estimate_accuracy(ib, il);
   Mcout << fileName << " IB equal weights acc=" << acc << endl;
   ib.set_weights(weights);
   acc = accEst.estimate_accuracy(ib, il);   
   Mcout << fileName << " IB perfect weights acc=" << acc << endl;   
}
   

static void real_val_test(const Array<Real>& weights, const MString& fileName)
{
   InstanceList bag("t_LabInstGen5", argDummy);
   ProtoFunctor protoFunctor(bag.get_schema(), weights);
   IndependentLabInstGenerator labInstGen(bag.get_schema(), protoFunctor);
   labInstGen.set_seed(9333126);

   data_generator(bag, 1000, labInstGen, TRUE);
   MLCOStream data(fileName);
   data << bag << endl;
   data.close();

   est_accuracy(weights, fileName);
}
   

// Generate files as follows:
//  out5 : 1,1,1,1,1
//  out6 : 1,1,0,1,0
//  out7 : 1,1,0.5,0.5.0
static void real_val_test()
{
   Array<Real> weights(0, protoNumAttr, 1.0);
   real_val_test(weights, "t_LabInstGen.out5");

   //weights[4] = 0; weights[6] = 0;
   weights[2] = 0; weights[4] = 0;
   real_val_test(weights, "t_LabInstGen.out6");   
   
   weights[2] = 0.5; weights[3] = 0.5; //weights[5] = 0.5; weights[7] = 0.5;
   real_val_test(weights, "t_LabInstGen.out7");


}


   
main()
{
   cout << "t_LabelledInstanceGenerator.c executing" << endl;

   test_error_msgs();
   
   // Test data_generator with duplication allowed and no noise
   InstanceBag *testBag = muxcheck(NUM_INSTANCES1, TRUE);
   TableCategorizer *tableCat = new TableCategorizer(*testBag,
		     UNKNOWN_CATEGORY_VAL,  "test");
   CatTestResult *catResult = new CatTestResult(*tableCat, *testBag,
						"t_LabInstGen");
   ASSERT((catResult->num_incorrect()==0)&&(catResult->accuracy()==1.0));
   delete catResult;
   delete tableCat;
   delete testBag;
						
   // Test data_generator with no duplication allowed and no noise
   // note that NUM_INSTANCES2 should be the number of different instances in
   //   the data space.
   testBag = muxcheck(NUM_INSTANCES2, FALSE);
   ASSERT(testBag->num_instances() == NUM_INSTANCES2);
   tableCat = new TableCategorizer(*testBag, UNKNOWN_CATEGORY_VAL, "test");
   catResult = new CatTestResult(*tableCat, *testBag, "t_LabInstGen");
   ASSERT((catResult->num_incorrect()==0) &&
	  (catResult->num_correct()==NUM_INSTANCES2));
   delete catResult;
   delete tableCat;
   delete testBag;

   
   // Test data_generator with duplication allowed and NOISE_RATE% noise
   // removing conflicting instances from the bag should leave only
   //   the actual (not noisy) instances in the bag.
   testBag = muxcheck(NUM_INSTANCES3, TRUE, NOISE_RATE);
   testBag->remove_conflicting_instances();
   tableCat = new TableCategorizer(*testBag, UNKNOWN_CATEGORY_VAL, "test");
   catResult = new CatTestResult(*tableCat, *testBag, "t_LabInstGen");
   ASSERT((catResult->num_incorrect()==0) && (catResult->accuracy()==1.0));
   delete catResult;
   delete tableCat;
   delete testBag;
   
   full_space_test();
   skewed_attr_test();
   mod_test();
   real_val_test();
   
   return 0; // return success to shell
}
