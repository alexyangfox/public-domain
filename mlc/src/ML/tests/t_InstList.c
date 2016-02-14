// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some "InstList.c" of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests read_files using test files "t_InstList.names" and
                    "t_InstList.data" as legal input files.  
		 Tests display().  "t_InstList.out1" and
		    "t_InstList.out2" should be diff'ed against
		    "t_InstList.exp1" and "t_InstList.exp2" respectively.
		 Tests a number of file format errors.
		 Tests that line counter and file name are correct in 
		    file error messages.
  Doesn't test : Doesn't cover real value tests a lot.  Specifically, needs
                   to check if a question mark followed by a space is OK (then
		   a comma).  This used to fail and was fixed in Attribute.c
                 Set operations (unimplemented)
		 Feature construction methods are tested in t_BagFeature.c.
  Enhancements :
  History      : Richard Long                                        9/13/93
                   Updated to use InstList
                 Richard Long                                        7/29/93
                    Initial revision

***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <InstList.h>
#include <AttrCat.h>
#include <MRandom.h>

RCSID("MLC++, File $Revision: 1.70 $")

const int MAJORITY_CAT = 3;  //determined in "t_InstList.data"
const int NUM_ATTR = 6;      //determined in "t_InstList.data"
const int NUM_INST = 6;      //determined in "t_InstList.data"

/***************************************************************************
  Description : This ugly function checks that the information read exactly
                  matches the data in the file.
		Also tests the Pix methods.
  Comments    : This must be changed if "t_InstList.names" or "t_InstList.data"
                  is changed.
***************************************************************************/
void match_contents(InstanceList& bag)
{
   ASSERT(bag.num_instances() == NUM_INST);

   Pix pix = bag.first();
   InstanceRC instance = bag.get_instance(pix);
   //check instanceInfo
   SchemaRC schema = instance.get_schema();
   for (int i = 0; i < bag.num_attr(); i++) {
      const NominalAttrInfo& nai = bag.nominal_attr_info(i);
      ASSERT(nai.name() == "ATTR" + MString(i,0));
      for (int j = FIRST_NOMINAL_VAL; j < FIRST_NOMINAL_VAL + nai.num_values();
	   j++) {
	 if (i == 5 && j == FIRST_NOMINAL_VAL)
	    ASSERT(nai.get_value(j) ==
		   "val5-0 is long enough to cause a new line in display");
	 else if (i == 4 && j == 3 + FIRST_NOMINAL_VAL)
	    ASSERT(nai.get_value(j) == "continuous");
	 else
	    ASSERT(nai.get_value(j) == 
		   "val" + MString(i,0) + "-"
		   + MString(j - FIRST_NOMINAL_VAL,0));
      }
   }
   
   for (int j = 0; j < NUM_ATTR; j++) {
      ASSERT(instance.attr_info(j).get_nominal_val(instance[j])
	     == FIRST_NOMINAL_VAL);
      ASSERT(instance.label_info().get_nominal_val(instance.get_label())
	     == FIRST_CATEGORY_VAL);
   }
   // check instance values
   // 1st 3 instances
   for (i = 1; i <= 2; i++) {
      bag.next(pix);
      InstanceRC instance = bag.get_instance(pix);
      ASSERT(instance.label_info().get_nominal_val(instance.get_label())
	     == i + FIRST_CATEGORY_VAL);
   }
   // 4th instance
#define CHECK(ins,val)                                                \
   ASSERT(instance.attr_info(ins).get_nominal_val(instance[ins])      \
	  == val + FIRST_NOMINAL_VAL)

#define CHECK_LABEL(val)                                              \
   ASSERT(instance.label_info().get_nominal_val(instance.get_label()) \
	  == val + FIRST_CATEGORY_VAL)
      
   bag.next(pix);
   instance = bag.get_instance(pix);
   CHECK(0, 2);
   CHECK(1, 2);
   CHECK(2, 3);
   CHECK(3, 3);
   CHECK(4, 2);
   CHECK(5, 2);
   CHECK_LABEL(3);
   
   // 5th instance
   bag.next(pix);
   instance = bag.get_instance(pix);
   CHECK(0, 2);
   CHECK(1, 2);
   CHECK(2, 4);
   CHECK(3, 3);
   CHECK(4, 2);
   CHECK(5, 2);
   CHECK_LABEL(3);
   
   // 6th instance
   bag.next(pix);
   instance = bag.get_instance(pix);
   CHECK(0, 0);
   CHECK(1, 1);
   CHECK(2, 2);
   CHECK(3, 3);
   CHECK(4, 3);
   CHECK(5, UNKNOWN_NOMINAL_VAL - FIRST_NOMINAL_VAL);
   CHECK_LABEL(3);
  
   //Should be no more instances
   bag.next(pix);

   // just to check that these do not cause fatal errors
   const InstanceBag& temp = bag;
   const InstanceList& instList = temp.cast_to_instance_list();
   InstanceBag& temp2 = bag;
   InstanceList& lil = temp2.cast_to_instance_list();
#ifndef MEMCHECK
   TEST_ERROR("InstanceBag::get_instance: tried to "
	      "dereference a Null Pix", bag.get_instance(pix));
   TEST_ERROR("InstanceBag::next: tried to dereference a Null Pix",
	      bag.next(pix));
   TEST_ERROR("InstanceBag::cast_to_ctr_instance_bag: "
	      "Illegal cast", CtrInstanceBag& cib =
	      bag.cast_to_ctr_instance_bag());
   TEST_ERROR("InstanceBag::cast_to_ctr_instance_bag() "
	      "const: Illegal cast", const CtrInstanceBag& cib =
	      instList.cast_to_ctr_instance_bag());
   TEST_ERROR("InstanceBag::cast_to_ctr_instance_list: "
	      "Illegal cast", CtrInstanceList& cil
	      = bag.cast_to_ctr_instance_list());
   TEST_ERROR("InstanceBag::cast_to_ctr_instance_list() const: "
	      "Illegal cast", const CtrInstanceList& cil
	      = instList.cast_to_ctr_instance_list());
#endif
}


/***************************************************************************
  Description : Creates an instance and checks that
                  find_labelled()/unlabelled() returns the correct value.
		  The created instance matches the first instance in the bag.
  Comments    : Assumes the bag has at least one instance.
***************************************************************************/
void check_find(InstanceBag& bag)
{
   InstanceRC instance = bag.get_instance(bag.first());
   Pix pix = bag.find_labelled(instance);
   ASSERT(pix);
   InstanceRC inst = bag.get_instance(pix);
   ASSERT(inst == instance);
   InstanceRC changedLabel = instance;
   AttrValue_ av;
   changedLabel.label_info().set_unknown(av);
   pix = bag.find_unlabelled(changedLabel);
   inst = bag.get_instance(pix);
   ASSERT(inst == instance);
}


/***************************************************************************
  Description : Creates a Schema to pass to the constructor that
                  takes a parameter.  Calls the display() method with an
		  empty bag, sending output to cout.  Creates an
		  InstanceRC to test add_instance().
  Comments    :
***************************************************************************/
void check_bag()
{
   const int numAttr = 2;
   
   //create Schema to pass to constructor
   DblLinkList<AttrInfoPtr>* attrInfos = new DblLinkList<AttrInfoPtr>;
   DblLinkList<MString>* attrVals = new DblLinkList<MString>;
   for (int i = 0; i < numAttr; i++) {
      MString attrVal = "VAL" + MString(i,0);
      attrVals->append(attrVal);
   }
   AttrInfoPtr nai(new NominalAttrInfo("ATTR0", attrVals));
   ASSERT(attrVals == NULL); // nai gets ownership
   attrInfos->append(nai);

   attrVals = new DblLinkList<MString>;
   for (i = 0; i < numAttr; i++) {
      MString attrVal = "VAL" + MString(i,0);
      attrVals->append(attrVal);
   }
   MString attrVal = "LABEL";

  
   AttrInfo* label = new NominalAttrInfo("LABEL", attrVals);
   ASSERT(attrVals == NULL); // label gets ownership
   SchemaRC schema(attrInfos, label);
   ASSERT(attrInfos == NULL); ASSERT(label == NULL); // schema gets ownership
   InstanceBag bag(schema);
  
   // Check display() method on empty bag.
   bag.display();
  
   // create Instance to test add_instance()
   InstanceRC instance(bag.get_schema());
   instance.attr_info(0).set_nominal_val(instance[0], 1);
   bag.add_instance(instance);
   InstanceRC li2 = bag.get_instance(bag.first());
   ASSERT(li2.attr_info(0).get_nominal_val(li2[0]) == 1);
   ASSERT(li2.get_schema().equal(instance.get_schema(), TRUE));
   ASSERT(bag.num_instances() == 1);
#ifndef MEMCHECK
   TEST_ERROR("InstanceBag::cast_to_instance_list: "
	      "Illegal cast", InstanceList& il =
	      bag.cast_to_instance_list());
   const InstanceBag& constBag = bag;
   TEST_ERROR("InstanceBag::cast_to_instance_list() const: "
	      "Illegal cast", const InstanceList& il =
	      constBag.cast_to_instance_list());
#endif
}


/***************************************************************************
  Description : This function checks a number of possible fatal_errors.
  Comments    : The code is dependent on the contents of the test files.
***************************************************************************/
void check_errors()
{
  TEST_ERROR("an incorrect attribute value is not a valid attribute value",
	     InstanceList bad1("t_InstList", ".names", ".bad1"));

  TEST_ERROR("line 8 of t_InstList.bad2.  Only comments & whitespace or a '.'",
	       InstanceList bad2("t_InstList", ".names", ".bad2"));

  TEST_ERROR("Another word expected on line 3 of t_InstList.bad3",
	     InstanceList bad3("t_InstList", ".names", ".bad3"));

  TEST_ERROR("unexpected ':' in list of labels line 3 of t_InstList.bad4",
	     InstanceList bad4("t_InstList", ".bad4"));

  TEST_ERROR("read_word: illegal name '?' on line 2 of t_InstList.bad5",
	     InstanceList bad5("t_InstList", ".bad5"));

  TEST_ERROR("line 8 of t_InstList.bad6.  Only comments & whitespace can "
	     "follow",   InstanceList bad6("t_InstList", ".names",
						   ".bad6"));

  TEST_ERROR("Unexpected end of file on line 3 of t_InstList.badeof",
	      InstanceList bad7("t_InstList", ".badeof"));

//  It's now OK to have an empty file, which will have zero instances.
//  TEST_ERROR("Unexpected end of file on line 3 of t_InstList.badeof",
//	     InstanceList bad8("t_InstList", ".names", ".badeof"));
  InstanceList bad8("t_InstList", ".names", ".badeof");

  TEST_ERROR("File 'nonexistent' does not exist in colon separated",
	     InstanceList bad9("", "nonexistent"));

  TEST_ERROR("File 'nonexistent' does not exist in colon separated",
	     InstanceList bad10("", "t_InstList.names",
					"nonexistent"));

  TEST_ERROR("attribute label ATTR1 val1-0 on line 6 of t_InstList.bad11",
	     InstanceList bad11("t_InstList", ".bad11"));

  TEST_ERROR("InstanceList::read_data_line: Unexpected end of "
	     "line following weight ",
	     InstanceList bad12("t_InstList", ".wt.names", ".bad12"));

  TEST_ERROR("InstanceList::read_data_line: Expecting ',' "
	       " instead of '",
	     InstanceList bad13("t_InstList", ".wt.names", ".bad13"));
}


/***************************************************************************
  Description : Reads the "golf" data.  The bags resulting from the split
                  are all displayed in t_InstList.out2.  majority_category()
		  is also tested on the golf bag.
  Comments    : This also checks verify_current_MLCOStream used in
                   operator<< for InstanceBag.
***************************************************************************/
static void check_split()
{
   InstanceList splitList("t_InstList-split");
   // majority category is "Play"
   ASSERT(splitList.majority_category() == FIRST_CATEGORY_VAL);
   AttrCategorizer ac(splitList.get_schema(), 0,
		      "AttrCat on outlook");
   BagPtrArray& bpa = *splitList.split(ac);
   MLCOStream splitStream("t_InstList.out2");
   splitStream << "Result of split:" << endl;
   for (int i = bpa.low(); i <= bpa.high(); i++)
      splitStream << *bpa[i];

   splitStream << "End of split test" << endl;

   delete &bpa;
}


/***************************************************************************
  Description : Tests InstanceBag::split_label()
  Comments    : Appends output to "t_InstList.out2"
                Uses t_InstList-split.[names,data]
***************************************************************************/
static void check_split_by_label()
{
   InstanceList splitList("t_InstList-split");
   BagPtrArray& bpa = *splitList.split_by_label();
   MLCOStream splitStream("t_InstList.out2", FileStream, TRUE);
   splitStream << "Result of split_by_label:" << endl;
   for (int i = bpa.low(); i <= bpa.high(); i++)
      bpa[i]->display(splitStream);
   delete &bpa;
}


/***************************************************************************
  Description : Checks the methods split_prefix() and unite().
  Comments    : Appends output to "t_InstList.out2".
                Uses t_InstList-split.[names,data]
***************************************************************************/
void check_split_prefix_unite()
{
   // Test on AllocBag
   InstanceList bag("t_InstList-split");
   MLCOStream outStream("t_InstList.out2", FileStream, TRUE);
   outStream << "List before split:" << endl;
   bag.display(outStream);

   int origSize = bag.num_instances();
   int numInSplit = 3;
   InstanceList* prefixList = bag.split_prefix(numInSplit);
   ASSERT(prefixList->num_instances() == numInSplit);
   ASSERT(bag.num_instances() == origSize - numInSplit);
   outStream << "Alloc List prefix of " << numInSplit << " instances:"
	     << endl;
   prefixList->display(outStream);

   outStream << "Alloc List with prefix removed:" << endl;
   bag.display(outStream);

   bag.unite(prefixList);
   ASSERT(prefixList == NULL);
   ASSERT(origSize == bag.num_instances());
   outStream << "United Alloc List:" << endl;
   bag.display(outStream);

   // Test on non-AllocBag
   BagPtrArray& bpa = *bag.split_by_label();
   // Must convert the bag to a list to call split_prefix()
   InstanceList* nonAllocList;
   nonAllocList = new InstanceList(bpa[bpa.high()]);
   delete &bpa;
   
   outStream << "Non Alloc List before split:" << endl;
   nonAllocList->display(outStream);
   origSize = nonAllocList->num_instances();
   
   prefixList = nonAllocList->split_prefix(numInSplit);
   ASSERT(prefixList->num_instances() == numInSplit);
   ASSERT(nonAllocList->num_instances() == origSize - numInSplit);
   outStream << "Non Alloc List prefix of " << numInSplit
	     << " instances:" << endl;
   prefixList->display(outStream);

   outStream << "Non Alloc List with prefix removed:" << endl;
   nonAllocList->display(outStream);

   nonAllocList->unite(prefixList);
   ASSERT(prefixList == NULL);
   ASSERT(nonAllocList->num_instances() == origSize);
   outStream << "United Non Alloc List:" << endl;
   nonAllocList->display(outStream);
   delete nonAllocList;

#ifndef MEMCHECK
   TEST_ERROR("InstanceList::split_prefix: Cannot remove ",
	      bag.split_prefix(bag.num_instances() + 1));
   prefixList = &bag;
   TEST_ERROR("InstanceList::unite: Cannot unite bag with itself",
	      bag.unite(prefixList));
#endif
}


/***************************************************************************
  Description : Checks the methods shuffle(), independent_sample(),
                   and create_bag_index().
  Comments    : Reads from the file t_Instlist-shuffle.[names, data] 
                  (lenses-full)
***************************************************************************/
void check_shuffle()
{
   MRandom mrandom(78589);   // Need to always use the same random
			     // seed so that this always produces the
			     // same results. 
   InstanceList sourceList("t_InstList-shuffle");
   cout << "List before shuffle:" << endl;
   sourceList.display();
   InstanceList& shuffledList =
                sourceList.shuffle(&mrandom)->cast_to_instance_list();
   cout << "List after shuffling" << endl;
   shuffledList.display();
   delete &shuffledList;

#ifndef MEMCHECK
   TEST_ERROR(") must be greater than zero and less than or equal to ",
	      sourceList.independent_sample(-1));
   TEST_ERROR(") must be greater than zero and less than or equal to ",
	 sourceList.independent_sample(sourceList.num_instances()+1));
   InstanceBagIndex* index = sourceList.create_bag_index();
   sourceList.remove_front();
   TEST_ERROR("InstanceBag::independent_sample: The index size(",
	      sourceList.shuffle(&mrandom, index));
#endif
}


/***************************************************************************
  Description : Tests bags with weighted instances.
  Comments    :
***************************************************************************/
void check_weights()
{
   InstanceList bag("golf");
   Real wt = 0.1;
   for (Pix pix = bag.first(); pix; bag.next(pix)) {
      ASSERT(bag.get_weight(pix) == 1);
      bag.set_weight(pix, wt);
      ASSERT(bag.get_weight(pix) == wt);
      wt += 1;
   }
   MLCOStream out5("t_InstList.out5");
   bag.display(out5);
#ifndef MEMCHECK
   TEST_ERROR("InstanceBag::set_weight: Null pix",
	      bag.set_weight(pix, 12.3));
#endif
   InstanceList wtList("t_InstList.wt");
   ASSERT(wtList.get_weighted() == TRUE);
   wt = 0.1;
   for (pix = wtList.first(); pix; wtList.next(pix)) {
      ASSERT(wtList.get_weight(pix) == wt);
      wt += 1;
   }
   wtList.display(out5);
}


/***************************************************************************
  Description : Tests the functions for normalizing reals.
  Comments    :
***************************************************************************/
void test_normalize()
{
   InstanceList instList("t_InstList1");

   const RealAttrInfo& rai =instList.get_schema().attr_info(1).cast_to_real();
   RealAttrValue_ val;
#ifndef MEMCHECK
   InstanceBag bag(instList.get_schema());
   TEST_ERROR("AttrInfo::cast_to_real", instList.normalize_attr(0,InstanceBag::extreme));
   TEST_ERROR("RealAttrInfo::normalized_value: Minimum and maximum values "
	      "for RealAttrInfo ", rai.normalized_value(val));
   InstanceList sameValues("t_InstList1", ".names", ".data.same");
#endif      
   
   instList.normalize_bag(InstanceBag::extreme);
   SchemaRC schema = instList.get_schema();
   const RealAttrInfo& rai1 = schema.attr_info(1).cast_to_real();
   const RealAttrInfo& rai2 = schema.attr_info(2).cast_to_real();


   instList.display();

   Mcout << "\nNormalized values:" << endl;
   for (Pix pix = instList.first(); pix; instList.next(pix)) {
      InstanceRC instance = instList.get_instance(pix);
      Mcout << rai1.normalized_value(instance[1]) << '\t'
	     << rai2.normalized_value(instance[2]) << endl;
   }
   TEST_ERROR("RealAttrInfo::normalized_value: Unknown attribute value passed "
	      "to RealAttrInfo ", rai1.normalized_value(val));
   Mcout << "Normalizing MAXFLOAT and -MAXFLOAT" << endl;
   rai1.set_real_val(val, MAXFLOAT);
   Mcout << rai1.normalized_value(val) << '\t';
   rai1.set_real_val(val, -MAXFLOAT);
   Mcout << rai1.normalized_value(val) << endl;
}


/***************************************************************************
  Description : Checks that the proper error message is given based on
                  owenrship of instances.
  Comments    :
***************************************************************************/
void test_remove()
{
   InstanceList* trainData = new InstanceList("t_InstList-conflict");
   
   InstanceBag* copiedData = new InstanceBag(trainData->get_schema());

   for (Pix pix = trainData->first(); pix; trainData->next(pix)) {
      InstanceRC instance = trainData->get_instance(pix);
      copiedData->add_instance(instance);
   }
   copiedData->remove_conflicting_instances();
   delete copiedData;
   delete trainData;
}


/***************************************************************************
  Description : Check removal of instances with unknown attributes, display of
                   names file, removal of conflicting instances, removal of
		   attributes, and removal of multiple attributes.
  Comments    :
***************************************************************************/
void check_remove()
{
   // check removal of instances with unknown attributes
   InstanceList liListUnknown("t_InstList-unknown");
   MLCOStream out4("t_InstList.out4");
   out4 << "Data with unknows" << endl;
   liListUnknown.display(out4);
   liListUnknown.remove_inst_with_unknown_attr();
   out4 << endl <<"Data with unknows removed" << endl;
   liListUnknown.display(out4);

   // check display of names file
   out4 << endl <<"Displaying names file" << endl;
   liListUnknown.display_names(out4, TRUE, "Test names file");

   // check removal of conflicting instances
   InstanceList liListConflict("t_InstList-conflict");
   out4 << endl <<"Data with conflicting instances" << endl;
   liListConflict.display(out4);
   liListConflict.remove_conflicting_instances();
   out4 << endl <<"Data with conflicting instances removed" << endl;
   liListConflict.display(out4);

   // check removal of attributes
   InstanceBag *origBag = new InstanceBag(liListUnknown, ctorDummy);
   out4 << endl <<"Data with all attributes" << endl;
   origBag->display(out4);
   BoolArray mask(0, origBag->num_attr(), TRUE);
   mask[3] = FALSE;
   InstanceBag *projectedBag = origBag->project(mask);
   out4 << endl <<"Data with an attribute removed" << endl;
   projectedBag->display(out4);
   delete origBag;
   delete projectedBag;

   // check removal of multiple attributes
   InstanceBag *origBag2 = new InstanceBag(liListUnknown, ctorDummy);
   out4 << endl <<"Data with all attributes" << endl;
   origBag2->display(out4);
   BoolArray attrMask(0,origBag2->num_attr(),TRUE);
   attrMask[1] = attrMask[3] = FALSE;
   InstanceBag *projectedBag2 = origBag2->project(attrMask);
   out4 << endl <<"Data with multiple attributes removed" << endl;
   projectedBag2->display(out4);
   delete origBag2;
   delete projectedBag2;
}


/***************************************************************************
  Description : Tests that set_schema() is working correctly.
  Comments    :
***************************************************************************/
void test_set_schema()
{
   InstanceList bag1("monk1");
   InstanceList bag2("monk1-full", argDummy);
   Mcout << bag2; //Should give no instances
   bag1.set_schema(bag2.get_schema());
   MLCOStream out6("t_InstList.out6");
   out6 << bag1; //Should be the same as monk1-full.data
}

/***************************************************************************
  Description : Tests the equality check.
  Comments    :
***************************************************************************/
void test_equality()
{
   // basic positive case
   InstanceList bag1("monk1");
   InstanceList bag2("monk1");
   ASSERT(bag1 == bag2);

   // negative case: bad lengths
   InstanceList *bagx = bag1.split_prefix(2);
   ASSERT(*bagx != bag2);

   // negative case: bad ordering
   bag1.unite(bagx);
   ASSERT(bag1 != bag2);
}


/***************************************************************************
  Description : Tests projection of zero attributes
  Comments    :
***************************************************************************/
void test_zero_projection(const InstanceBag& bag)
{
   // project the bag on zero attributes
   BoolArray zeroMask(0, bag.num_attr(), FALSE);
   InstanceBag *zeroBag = bag.project(zeroMask);
   ASSERT(zeroBag->num_attr() == 0);

   // we should be able to project this bag again on all FALSE and
   // this should give us the same bag out.
   BoolArray zeroMask2(0, zeroBag->num_attr(), FALSE);
   InstanceBag *zeroBag2 = zeroBag->project(zeroMask2);
   ASSERT(zeroBag2->num_attr() == 0);
   ASSERT(*zeroBag == *zeroBag2);
   
   // now, create an empty bag and try projecting THAT on nothing
   InstanceBag emptyBag(bag.get_schema());
   BoolArray zeroMask3(0, emptyBag.num_attr(), FALSE);
   InstanceBag *zeroBag3 = emptyBag.project(zeroMask3);


   
   // display
   Mcout << "zeroBag: " << endl << *zeroBag << endl;
   Mcout << "zeroBag2: " << endl << *zeroBag2 << endl;
   Mcout << "zeroBag3: " << endl << *zeroBag3 << endl;

   // clean up
   delete zeroBag;
   delete zeroBag2;
}

void check_corrupt_unknown()
{
   MRandom mrandom(4975497);

   InstanceList bag("crx");
   bag.remove_inst_with_unknown_attr(); // no unknowns now.
   
   InstanceBag bag2(bag, ctorDummy);

   bag2.corrupt_values_to_unknown(1, mrandom);
   // make sure everything is unknown.
   for (Pix pix = bag2.first(); pix; bag2.next(pix)) { 
      const InstanceRC& instance = bag2.get_instance(pix);
      for (int i = 0; i < bag2.num_attr(); i++) { // for each attribute
	 const AttrInfo& ai = bag2.attr_info(i);
	 ASSERT(ai.is_unknown(instance[i]));
      }
   }

   Mcout << "Check corrupt unknown part 1 OK" << endl;

   InstanceBag bag3(bag, ctorDummy);
   bag3.corrupt_values_to_unknown(0, mrandom);

   ASSERT(bag == bag3);   
   Mcout << "Check corrupt unknown part 2 OK" << endl;
   
   // corrupt half the instances
   bag.corrupt_values_to_unknown(0.5/bag.num_attr(), mrandom);
   MLCOStream out7("t_InstList.out7");
   out7 << bag << endl;

   int numInst = bag.num_instances();
   bag.remove_inst_with_unknown_attr(); // no unknowns now.
   // Should be over half because two unknowns can be in an instance.
   // The total number of unknowns should be about half.
   Mcout << "Check corrupt had " << numInst << " instances, now "
            " it has " << bag.num_instances()
	 << " (should be somewhat over a half)" << endl;
   
}


main()
{
   Mcout << "t_InstList executing" << endl;

   test_equality();
   test_remove();

   // test read_names() and read_data() on legal files
   InstanceList instList("t_InstList");
   MLCOStream out1("t_InstList.out1");
   out1.set_width(80);
   InstanceBag* bag = new InstanceBag(instList, ctorDummy);
   instList.display(out1);
   match_contents(instList);
   if (instList.majority_category() != MAJORITY_CAT + FIRST_CATEGORY_VAL)
      err << "t_InstList: majority_category mismatch, got " <<
      instList.majority_category() << " and should have gotten " <<
      MAJORITY_CAT + FIRST_CATEGORY_VAL << fatal_error;
   check_find(instList);

   Mcout << "Testing zero projection" << endl;
   test_zero_projection(*bag);
   
   Mcout << "Testing my copy!" << endl;
   InstanceList myList(bag);
   Mcout << "Testing clone" << endl;
   InstanceBag *myBag2 = myList.clone();   
   InstanceList *myList2 = &(myBag2->cast_to_instance_list());
   match_contents(*myList2);  
   match_contents(myList);
   delete myList2;
   myList.display(out1);
   Mcout << "Done with my copy and clone!" << endl;

   check_bag();
   check_split();
   check_split_by_label();
   check_split_prefix_unite();
   check_shuffle();
   check_weights();

   // This list contains RealAttrInfo and unknown data values 
   InstanceList realLil("labor-neg");
   MLCOStream out3("t_InstList.out3");
   realLil.display(out3);
   
#ifndef MEMCHECK
   check_errors();
#endif

   check_remove();

   // Ronny's lines
   InstanceList liList2("t_InstList", argDummy);
   Mcout << liList2;
   
   test_normalize();

   test_set_schema();
   check_corrupt_unknown();
   return 0;
}
