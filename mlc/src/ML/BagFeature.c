// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Augments the Bag class to allow feature construction.
                 In order to construct the feature a new bag is created
		   with copy_with_blank_feature(). The new bag is
		   unusable until the feature is defined with
		   change_feature().    
  Assumptions  :
  Comments     : The header file for BagFeature.c is BagSet.h.
  Complexity   : define_feature_info() takes O(numVal1*numVal2) time where
                   numVal1 and numVal2 are the numbers of AttrVal's of
		   the two attribuites combined into the feature.
		 fill_in_feature_values() takes O(num-instances in the
		   bag * O(newFeatureFuncPtr)) time.
                 copy_with_blank_feature() takes O(num-instances in
                   the bag * oldAttrNum) time.
		 change_feature() takes O(define_feature_info()) +
		   O(fill_in_feature_values()) time.
  Enhancements : Substitute function pointers with Functors. 
  History      : Svetlozar Nestorov                                  1/18/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <BagSet.h>

RCSID("MLC++, $RCSfile: BagFeature.c,v $ $Revision: 1.15 $")

/***************************************************************************
  Description : Creates a list that consists of the cross product of
                  the names of the attrValues of the two
		  NominalAttrInfos attrInfo1 and attrInfo2.
		Every MString in the list is of the following form:
		  name1*separator*name2  where '*' denotes concatenation
		  and name'i' is a name of an attrValue of attrInfo'i'(i=1,2).
  Comments    : Takes O(numVal1*numVal2)  where numVal'i' is the number
                  of different attrValues of attrInfo'i'(i=1,2).
		The caller gets ownership of the newly created
		   DblLinkList<MString>.
***************************************************************************/
static DblLinkList<MString> *create_cross_names(const NominalAttrInfo& nai1,
						const NominalAttrInfo& nai2,
						const MString& separator) 
{
   DblLinkList<MString> *strList = new DblLinkList<MString>();
   int numVal1 = nai1.num_values();
   int numVal2 = nai2.num_values();
   for (int i = FIRST_NOMINAL_VAL; i < FIRST_NOMINAL_VAL + numVal1; i++)
      for (int j = FIRST_NOMINAL_VAL; j < FIRST_NOMINAL_VAL + numVal2; j++)
	 strList->append(nai1.get_value(i) + separator + nai2.get_value(j));
   return strList;
}


/***************************************************************************
  Description : Creates a new NominalAttrInfo (the feature) combining
                  the two attributes from the Schema of
		  the Bag with respective numbers attrNum1 and attrNum2.
		Substitutes the last AttrInfo in the Schema of the Bag
		  with the newly created NominalAttrInfo.
		The substituted AttrInfo is deleted.
  Comments    : This method should only be called for Bags created by
		  copy_with_blank_feature() method, otherwise the result
		  is undefined.
		Note that the attrNum1 and attrNum2 are the numbers of
		  the AttrInfos, forming the feature, in the object's
		  Schema. They might be different from the numbers of
		  these two AttrInfo in the Schema of the source Bag from
		  which the object was created with copy_with_blank_feature().
***************************************************************************/
void InstanceBag::define_feature_info(int attrNum1, int attrNum2,
				      const MString& separator)
{
   // create a new Schema that is the same as the Schema of
   //   the bag but without the last attribute.

   DblLinkList<AttrInfoPtr>* attrInfos = new DblLinkList<AttrInfoPtr>;
   for (int i = 0; i < num_attr() - 1; i++) {
      AttrInfoPtr ai(attr_info(i).clone());
      attrInfos->append(ai);
   }

   MString featureName =  get_schema().attr_name(attrNum1) + separator +
                          get_schema().attr_name(attrNum2);
   DblLinkList<MString> *featureAttrVals;
   featureAttrVals = create_cross_names(nominal_attr_info(attrNum1),
					nominal_attr_info(attrNum2),
					separator);
   AttrInfoPtr featureAttrInfo(new NominalAttrInfo(featureName, 
						   featureAttrVals));
   attrInfos->append(featureAttrInfo);
   AttrInfo* labelInfo = label_info().clone();
   SchemaRC newSchema(attrInfos, labelInfo);
   // newSchema gets ownership
   ASSERT(attrInfos == NULL); ASSERT(labelInfo == NULL);
   
   set_schema(newSchema);
}


/***************************************************************************
  Description : Fills in the the value of the feature in all
                  instances in the bag.
		The feature is assumed to be constructed from the
		  Attributes with respective numbers attrNum1 and
		  attrNum2 and its AttrInfo to be the last in the
		  LabelledInstanceInfo of the bag.
		The values filled in are generated by the function 
		  which is passed as a parameter.
  Comments    : 
***************************************************************************/
void InstanceBag::fill_in_feature_values
                                (int attrNum1, int attrNum2,
				 const FeatureFuncPtr newFeatureFuncPtr)
{
   const AttrInfo& attrInfo1 = attr_info(attrNum1);
   const AttrInfo& attrInfo2 = attr_info(attrNum2);
   int featureNum = num_attr() - 1; // the array is indexed from 0
   
   for (Pix pix = instance_list().first(); pix; next(pix)) {
      InstanceRC& instance = *instance_list()(pix);
      instance[featureNum] = (*newFeatureFuncPtr)(instance[attrNum1],
						  attrInfo1,
						  instance[attrNum2],
						  attrInfo2,
						  attr_info(num_attr()-1));
   }
}


/***************************************************************************
  Description : Creates a new bag from the object in the following way:
                  The Schema of the new bag is obtained by compressing
		     the Schema of the object and appending (at the end)
		     a "dummy" AttrInfo.
		  All instances in the object are copied to the new
		    bag augmented with an additional attribute (the Feature),
		    corresponding to the dummy AttrInfo, which value
		    is uninitialized. 
  Comments    : The bag should be used only after the feature is
		  changed with change_feature().
		The caller gets ownership of the newly created bag.
***************************************************************************/
InstanceBag* InstanceBag::copy_with_blank_feature() const
{
   // define a couple of shortcuts
   int oldNumAttr = get_schema().num_attr();
   
   DblLinkList<AttrInfoPtr>* attrInfos = new DblLinkList<AttrInfoPtr>;
   for (int i = 0; i < oldNumAttr; i++) {
      AttrInfoPtr ai(attr_info(i).clone());
      attrInfos->append(ai);
   }
   AttrInfo* labelInfo = label_info().clone();
   
   // create a dumb list that contains only one MString--"dummy"
   DblLinkList<MString> *dumbList = new DblLinkList<MString>;
   dumbList->append("dummy"); 
   
   AttrInfo *dummyAttrInfo = new NominalAttrInfo("dummy",  dumbList);
   attrInfos->append(dummyAttrInfo);
   AttrValue_ dummyVal;  // The dummy is unknown
   dummyAttrInfo->set_unknown(dummyVal);

   SchemaRC newSchema(attrInfos, labelInfo);
   // newSchema gets ownership
   ASSERT(attrInfos == NULL); ASSERT(labelInfo == NULL);
   
   InstanceBag *featureBag = new InstanceBag(newSchema);
   
   for (Pix pix = first(); pix; next(pix)) {
      InstanceRC oldInst = get_instance(pix);
      InstanceRC newInst(newSchema);
      for (i = 0; i < oldNumAttr; i++)
	 newInst[i] = oldInst[i];
      newInst.set_label(oldInst.get_label());
      // Without setting the dummyVal to unknown, it is random
      //   so the bag cannot be printed out (nominal values out of
      //   range, etc).
      newInst[i] = dummyVal;
      
      featureBag->add_instance(newInst);
   }
   return featureBag;
}

   
/***************************************************************************
  Description : Changes the last AttrInfo of the Schema of the object to a
                  NominalAttrInfo constructed from the AttrInfo's in
		  the Schema with respective numbers attrNum1 and attrNum2.
	        Updates the value of the newly created NominalAttrInfo
	          in all instances in the bag using the function
		  newFeatureFuncPtr.
  Comments    : 
***************************************************************************/
void InstanceBag::change_feature (int attrNum1, int attrNum2,
				  const MString& separator,
				  const FeatureFuncPtr newFeatureFuncPtr)
{
   define_feature_info(attrNum1, attrNum2, separator);
   fill_in_feature_values(attrNum1, attrNum2, newFeatureFuncPtr);
}
