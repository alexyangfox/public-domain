// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Schema defines the attribute information.
  Assumptions  :
  Comments     : Most effort was directed at labelled instances for
                   supervised learning, and on discrete attributes.
  Complexity   : Schema::~Schema() takes time proportional 
                    to the number of AttrInfos it has.
		 Schema::member() takes time proportional to the
		   number of AttrInfos it has. 
		 Schema::equal_instance() takes time proportional to
		   the number of attributes in the instance.
  Enhancements : Extend tree-structured attribute.
  History      : Robert Allen
                   Added project().                                1/27/95
                 Chia-Hsin Li
                   Added display_names().                          9/29/94
                 Yeogirl Yun
                   Merged LabelledInstanceInfo and InstanceInfo into
		   Schema and made it reference-count class.       6/12/94
                 Richard Long                                      5/27/94
                   AttrInfo stored as Array instead of linked list.
                 Richard Long                                      1/21/94
                   Added weighted option.
		 Svetlozar Nestorov                                 1/8/94
                   Added copy constructors.
                 Ronny Kohavi                                       8/18/93
                   Change equal to non-virtual, eliminated combinations
                   of LabelledInstanceInfo and InstanceInfo for equal
                   and operator=.  See coding standards for explanations.
		 Richard Long                                       7/14/93
                   Initial revision (.c)
		 Ronny Kohavi                                       7/13/93
                   Initial revision (.h)
***************************************************************************/

#include <basics.h>
#include <Schema.h>

RCSID("MLC++, $RCSfile: Schema.c,v $ $Revision: 1.68 $")

SET_DLLPIX_CLEAR(AttrInfoPtr,NULL);

/***************************************************************************
  Description : Nothing currently
  Comments    :
***************************************************************************/
void Schema::OK(int /*level*/) const
{
   // nothing currently
}


/***************************************************************************
  Description : Constructor with attribute info.
  Comments    : Gets ownership of the list and the AttrInfo.
***************************************************************************/
Schema::Schema(DblLinkList<AttrInfoPtr>*& attrInfos)
   : attr(attrInfos->length())
{
   refCount = 1;
   labelInfo = NULL;
   DLLPix<AttrInfoPtr> pix(*attrInfos, 1);
   int i = 0;
   for (; pix; pix.next(), i++) {
      attr[i] = (*attrInfos)(pix).attrInfo;
   }
   delete attrInfos;
   attrInfos = NULL;
   DBG(OK());
}



/***************************************************************************
  Description : Constructor with attribute info and label info.
  Comments    : Gets ownership of the list and the AttrInfo.
***************************************************************************/
Schema::Schema(DblLinkList<AttrInfoPtr>*& attrInfos, AttrInfo*& lInfo)
   : attr(attrInfos->length())
{
   refCount = 1;
   labelInfo = lInfo;
   DLLPix<AttrInfoPtr> pix(*attrInfos, 1);
   int i = 0;
   for (; pix; pix.next(), i++) {
      attr[i] = (*attrInfos)(pix).attrInfo;
   }
   delete attrInfos;
   attrInfos = NULL;
   lInfo = NULL;
   DBG(OK());
}




/***************************************************************************
  Description : Copy constructor. Makes a deep copy of the source. 
  Comments    :
***************************************************************************/
Schema::Schema(const Schema& source, CtorDummy )
   :attr(source.num_attr())
{
   refCount = 1;
   for (int i = 0; i < source.attr.size(); i++) {
      AttrInfo* nextAttr = source.attr_info(i).clone();
      attr[i] = nextAttr;
   }

   if (source.is_labelled())
      labelInfo = (source.labelInfo)->clone();
   else
      labelInfo = NULL;
   
   DBG(OK());
}





/***************************************************************************
  Description : Destructor.  AttrInfo are deleted by PtrArray destructor.
  Comments    :
***************************************************************************/
Schema::~Schema()
{
   DBG(OK());
   delete labelInfo;
}




/***************************************************************************
  Description : Returns the AttrInfo for the label. If labelInfo is
                  NULL, it aborts.
  Comments    : 
***************************************************************************/
const AttrInfo& Schema::label_info() const
{
   if( !is_labelled() )
      err << "Schema::label_info() : labelInfo is NULL " <<
	 fatal_error ;

   return *labelInfo;
}


/*****************************************************************************
  Description : Retuns TRUE iff labelInfo is not NULL. If fatalOnFalse
                  is set TRUE, it aborts.
*****************************************************************************/
Bool Schema::is_labelled(Bool fatalOnFalse) const
{
   if (labelInfo == NULL)
      if( fatalOnFalse) {
	 err << "Schema::is_labelled() : labelInfo is NULL" <<
	    fatal_error;
	 return 0;
      } else
	 return FALSE;
   else
      return TRUE;
}



/***************************************************************************
  Description : Return the AttrInfo matching the given attribute number.
                AttrInfos are numbered consecutively starting at zero.
  Comments    :
***************************************************************************/
const AttrInfo& Schema::attr_info(int num) const 
{
   if (attr[num] == NULL)
      err << "Schema::attr_info: NULL AttrInfo* stored" << fatal_error;
   return *attr[num];
}




/***************************************************************************
  Description : Checks whether an AttrInfo is in the Schema.
  Comments    : 
***************************************************************************/
Bool Schema::member(const AttrInfo& ainfo) const
{
   for (int i = 0; i < num_attr(); i++)
      if (attr_info(i) == ainfo)
	 return TRUE;
   return FALSE;
}




/***************************************************************************
  Description : Compares the labelInfos for the schemas if they exist.
                Returns FALSE if one has a labelInfo and the other
		  does not.
  Comments    :
***************************************************************************/
Bool Schema::equal(const Schema& schema,
		   Bool fatalOnFalse)const
{
   if ( is_labelled() ^ schema.is_labelled()) {
      if (fatalOnFalse)
         err << "Schema::equal: Only one of the two schemas has a"
	     << "label info" << fatal_error;
      return FALSE;
   }
   if (is_labelled())
      if (!label_info().equal(schema.label_info(), fatalOnFalse))
         return FALSE;
   return Schema::equal_no_label((Schema&)schema, fatalOnFalse);   
}


/***************************************************************************
  Description : Compares the equality between two Schema's only by
                   looking at the attribute values(not labels).
  Comments    :
***************************************************************************/
Bool Schema::equal_no_label(const Schema& info, Bool fatalOnFalse) const
{
   if (num_attr() != info.num_attr()) {
      if (fatalOnFalse)
	 err << "Schema::equal_no_label: Number of attributes different: "
	    << num_attr() << ", " << info.num_attr() << fatal_error;
      return FALSE;
   }
   for (int i=0; i < num_attr(); i++)
      if (!attr_info(i).equal(info.attr_info(i), fatalOnFalse))
	 return FALSE;
   
   return TRUE; 
}



/***************************************************************************
  Description : Check that the given schema matches except for the 
                  deleted attribute.
		Abort if not equal.
  Comments    : Private member.
***************************************************************************/

void Schema::equal_except_del_attr(const Schema& schema,
				   int deletedAttrNum) const
{
   if (num_attr() != schema.num_attr() + 1)
      err << "Schema::equal_except_del_attr failed.  Our info has " <<
      num_attr() << " attributes vs. " << schema.num_attr()
      << " attributes in the given instance" << fatal_error;

   for (int attrNum = 0; attrNum < num_attr() - 1; attrNum++){
      // skip over deleted attribute.  Add one to attr being compared if 
      //     attrNum > deleted.
      if (attrNum != deletedAttrNum)
         attr_info(attrNum + (attrNum > deletedAttrNum)).
              equal(schema.attr_info(attrNum), TRUE);
   }

   if (is_labelled() ^ schema.is_labelled())
      err << "Schema::equal_except_del_attr:: not both schemata are labelled"
          << is_labelled() << ", " << schema.is_labelled() << fatal_error;

}



/***************************************************************************
  Description : Returns the attribute name of the attribute given by
                  attrNum.
  Comments    : 
***************************************************************************/
const MString& Schema::attr_name(int attrNum) const
{
   return attr_info(attrNum).name();
}


/*****************************************************************************
  Descriptoin : Returns the nominal attribute info of the attribute
                  given by attrNum.
  Comments    :
*****************************************************************************/  
const NominalAttrInfo& Schema::nominal_attr_info(int attrNum) const
{
   return attr_info(attrNum).cast_to_nominal();
}


/*****************************************************************************
  Description : Returns the number of attribute values of the
                  attribute given by attrNum.
  Comments    :
*****************************************************************************/  
int Schema::num_attr_values(int attrNum) const
{
   return nominal_attr_info(attrNum).num_values();
}


/*****************************************************************************
  Description : Returns the string representation of the attribute
                  value of the given attribute.
  Comments    :
*****************************************************************************/  
const MString& Schema::nominal_to_string(int attrNum,
                                         Category cat) const
{
   return nominal_attr_info(attrNum).get_value(cat);
}


/*****************************************************************************
  Description : Returns nominal label info of the Schema.
  Comments    :
*****************************************************************************/  
const NominalAttrInfo& Schema::nominal_label_info() const
{
   return label_info().cast_to_nominal();
}


/*****************************************************************************
  Description : Returns the number of label values of the Schema.
  Comments    :
*****************************************************************************/  
int Schema::num_label_values() const
{
   return nominal_label_info().num_values();
}



/***************************************************************************
  Description : Converts a category to the matching label string.
  Comments    : Can be called only if the label is a nominal attribute.
***************************************************************************/
MString Schema::category_to_label_string(Category cat) const
{
   const NominalAttrInfo& nai = nominal_label_info();
   AttrValue_ av;
   nai.set_nominal_val(av, cat);
   return nai.attrValue_to_string(av);
}


/*****************************************************************************
  Description  : Returns a Schema with the given AttrInfo removed and the infos
                   renumbered to be sequential starting at 0
  Comments     : Caller gets ownership of the new Schema.
*****************************************************************************/
Schema* Schema::remove_attr(int attrNum) const
{
   if ( attrNum < 0 || attrNum >= num_attr() )
      err << "Schema::remove_attr(const int): illegal attrNum " << endl
	  << attrNum << " is passed but the proper range is " << endl
	  << " 0 to " << num_attr() - 1 <<"." << fatal_error;

   DblLinkList<AttrInfoPtr>* attrInfos = new DblLinkList<AttrInfoPtr>;
   // Add AttrInfos that keep the same attrNum
   for (int i = 0; i < attrNum; i++) {
      AttrInfoPtr ai(attr_info(i).clone());
      attrInfos->append(ai);
   }
   // Add AttrInfos that need to have their attrNums subtracted by 1
   for (i = attrNum + 1; i < num_attr(); i++) {
      AttrInfoPtr ai(attr_info(i).clone());
      attrInfos->append(ai);
   }

   Schema* newSchema;
   if (is_labelled()) {
      AttrInfo* label = label_info().clone();
      newSchema = new Schema(attrInfos, label);
      ASSERT(label == NULL);
   }
   else
      newSchema = new Schema(attrInfos);
   
   ASSERT(attrInfos == NULL);
   return newSchema;
   
}



/***************************************************************************
  Description : Displays the attribute info for each attribute on a
		  separate line and also displays the info for the
		  label if it exists.
  Comments    :
***************************************************************************/
void Schema::display(MLCOStream& stream, Bool protectChars) const
{
   stream << "Attributes:" << endl;
   for (int i=0; i < num_attr(); i++) {
      stream << attr_info(i).name() << ": ";
      attr_info(i).display_attr_values(stream, protectChars);
   }

   if (is_labelled()) {
      stream << "Label: ";
      stream << label_info().name() << ": ";
      label_info().display_attr_values(stream, protectChars);
   }
   else
      stream << "No label" << endl;
}


DEF_DISPLAY(Schema);

/***************************************************************************
  Description : Displays the names file associated with the bag.
  Comments    : The reason we don't call labelInfo->display_values() is
                to avoid printing the name of the label since a label
		doesn't have a name.
***************************************************************************/
void Schema::display_names(MLCOStream& stream, 
			   Bool protectChars, const MString& header) const 
{
   stream << "|" << header << endl;
   label_info().display_attr_values(stream, protectChars);
   stream << endl; // Have an extra blank line for clarity

   // display attribute values
   for (int attrNum = 0; attrNum < num_attr(); attrNum++) {
      stream << attr[attrNum]->name() << ": ";
      attr[attrNum]->display_attr_values(stream, protectChars);
   }
}


/***************************************************************************
  Description : Returns a copy of the schema with only the attributes
                  indicated in the mask.
  Comments    : 
***************************************************************************/
Schema* Schema::project(const BoolArray& attrMask) const
{
   DBG(OK());
   ASSERT(attrMask.size() == num_attr());
   DblLinkList<AttrInfoPtr>* attrInfos = new DblLinkList<AttrInfoPtr>;
   int newnum = 0;
   for (int i = 0; i < num_attr(); i++) {
      if (attrMask[i]) {
	 AttrInfoPtr ai(attr_info(i).clone());
	 newnum++;
	 attrInfos->append(ai);
      }
   }
   AttrInfo* labelInfo = label_info().clone();
   
   Schema* newSchema = new Schema(attrInfos, labelInfo);
   // newSchema gets ownership
   ASSERT(attrInfos == NULL); ASSERT(labelInfo == NULL);
   
   return newSchema;
}


   

