// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  :  The class Instance provides instances without label
                     or with label and support for describing instances,
		     iterating through values, and looking at the
		     instance label.
  Assumptions  :
  Comments     : Most effort was directed at categorization of labelled
                   instances for supervised learning.
  Complexity   :
  Enhancements : Fix display() so that it doesn't put a trailing comma
                    after the last attribute. and we may need to skip
		    a line if there isn't enough room for the comma.
                 Extend to tree-structured attributes.
 History      :  Robert Allen                                       1/27/95
                   Added project().
                 James Dougherty                                    9/17/94
                   Added set_schema() method
                 Yeogirl Yun                                        6/2/94
                   Merged _Instance, _LabelledInstance, Instance, and
		   LabelledInstance into one Instance and made it
		   reference-count class.                           6/12/94
                 Richard Long                                       1/20/94
                   Added support weighted instances.
                 Svetlozar Nestorov                                 1/09/94
                   Added copy constructors to _Instance and _LabelledInstance.
                 Ronny Kohavi                                       9/20/93
                   Major rewriting to allow _Instance to be public.
                   and to use _instance() and _labelledInstance()
                     functions properly, saving some code.
                   Instance and LabelledInstance must be constructed
                     properly now with InstanceInfo which is a const member.
                   Using safe arrays for values.
                 Richard Long                                       7/14/93
                   Initial revision (.c)
                 Ronny Kohavi                                       7/13/93
                   Initial revision (.h)
***************************************************************************/

#include <basics.h>
#include <Instance.h>

RCSID("MLC++, $RCSfile: Instance.c,v $ $Revision: 1.70 $")

const MString instanceWrapIndent("   ");
const char endOfInstanceLine = '.';

/*****************************************************************************
  Description : Initialized attribute values and label value(if exists).
  Comments    : This is a private function.
*****************************************************************************/
void Instance::init_values()
{
   for (int attrNum = 0; attrNum < num_attr(); attrNum++)
      attr_info(attrNum).set_unknown(values[attrNum]);
   if (schema.is_labelled())
      label_info().set_unknown(labelValue);
}


/***************************************************************************
  Description : Check that the instance is OK by calling check_in_range on all
                  values 
  Comments    :
***************************************************************************/

void Instance::OK(int /* level */) const
{
   for (int attrNum = 0; attrNum < num_attr(); attrNum++)
      attr_info(attrNum).check_in_range(values[attrNum]);
}
   

/*****************************************************************************
  Description : Constructor, Copy Constructor, and Destructor.
  Comments    :
*****************************************************************************/
Instance::Instance(const SchemaRC& schemaRC)
: schema(schemaRC),
  values(schemaRC.num_attr())
{
   weight = 1.0; // default value
   refCount = 1;
   DBG(init_values());
   DBG(OK());
}


Instance::Instance(const Instance& source, CtorDummy)
: schema(source.schema),
  values(source.values, ctorDummy),
  labelValue(source.labelValue)
{
   weight = source.weight;
   refCount = 1;
   DBG(OK());
}


Instance:: ~Instance()
{
   DBG(OK());
}




/***************************************************************************
  Description : Returns the attribute value corresponding to the index.
  Comments    : The indexes start at 0. This is non-const version.
***************************************************************************/
AttrValue_& Instance::operator[](int index) 
{
   DBGSLOW(attr_info(index).check_in_range(values[index]));
   return values[index];   
}




/*****************************************************************************
  Description : Set label value.
  Comments    :
*****************************************************************************/
void Instance::set_label(const AttrValue_& lvalue)
{
   if (!schema.is_labelled())
      err << "Instance::set_label(): labelInfo is not set" <<
	 fatal_error;
  schema.label_info().check_in_range(lvalue);
  labelValue = lvalue;   
}


/*****************************************************************************
  Description : weight interfaces.
  Comments    :
*****************************************************************************/  
Real Instance::get_weight() const
{
   return weight;
}

void Instance::set_weight(Real wt)
{
   weight = wt;
}




/*****************************************************************************
  Description  : equal() compares the labelInfos for the schemata if
                    they exist.  Returns FALSE if one has a labelInfo
		    and the other does not.
  Comments     :
*****************************************************************************/  
Bool Instance::equal(const Instance& instance, Bool fatalOnFalse) const
{
   if ( is_labelled() ^ instance.is_labelled()) // Xor: Both must match
      if (fatalOnFalse) {
	 err << "Instance::equal(): equalilty failed(only one of the"
	     << "two instances has label)." << fatal_error;
	 return 0;
      } else
	 return FALSE;
   else {
      Bool eq;
      if (is_labelled()) { // both have label. 
	 DBGSLOW(schema.label_info().equal(instance.get_schema().label_info(),
					   TRUE));
	 eq = ((schema.label_info().
	       _equal_value(labelValue, instance.get_label())) &&
	       equal_no_label(instance, fatalOnFalse));
      }
      else // both have no label.
	 eq = equal_no_label(instance, fatalOnFalse);

      if (eq)
	 return TRUE;
      else {
	 if (fatalOnFalse) {
	    err << "Instance::equal(): equalilty failed(either"
	        << "attribute values or label values differ" << fatal_error;
	    return 0;
	 } else
	    return FALSE;
      }
   }
}


/*****************************************************************************
  Description  :  equal_no_label() does not use the labelInfo for
                    either Schema whether or not they exist.
  Comments     :  
*****************************************************************************/  
Bool Instance::equal_no_label(const Instance& instance,
			      Bool fatalOnFalse) const
{
   DBGSLOW(get_schema().equal_no_label(instance.get_schema(), TRUE));
   DBG(ASSERT(instance.values.size() == values.size()));
   for (int i = 0; i < schema.num_attr(); i++) {
      if (!attr_info(i)._equal_value( operator[](i), instance[i] )) {
	 if( fatalOnFalse )
	    err << "Instance::equal_no_label(): equality"
	        << "failed(without label comparison). " <<
	       fatal_error;
	 else
	    return FALSE;
      }
   }
   return TRUE;
}




/*****************************************************************************
  Description  : Interfaces to Schema functions.
  Comments     :
*****************************************************************************/
Bool Instance::is_labelled(Bool b) const
{
   return schema.is_labelled(b);
}

SchemaRC Instance::get_schema() const
{
   return schema;
}

void Instance::set_schema(const SchemaRC& schemaRC)
{
   
   DBG( if (schemaRC.num_attr() != schema.num_attr())
	err << "Instance::set_schema() : attribute count does not match "
	    << schemaRC.num_attr() << " != " << schema.num_attr()
	    << fatal_error );

   schema = schemaRC;
}

const AttrInfo& Instance::attr_info(int attrNum) const
{
   return schema.attr_info(attrNum);
}


const AttrInfo& Instance::label_info() const
{
   if (!schema.is_labelled())
      err << "Instance::label_info(): label is not set " <<
	 fatal_error;
   return schema.label_info();
}
      
const NominalAttrInfo& Instance::nominal_attr_info(int attrNum) const
{
   return schema.nominal_attr_info(attrNum);
}

const NominalAttrInfo& Instance::nominal_label_info() const
{
   return label_info().cast_to_nominal();
}


int Instance::num_attr_values(int attrNum) const
{
   return schema.num_attr_values(attrNum);
}


int Instance::num_label_values() const
{
   return nominal_label_info().num_values();
}


const MString& Instance::attr_name(int attrNum) const
{
   return schema.attr_name(attrNum);
}

const MString& Instance::nominal_to_string(int attrNum,
				 Category cat) const
{
   return schema.nominal_to_string(attrNum, cat);
}



/***************************************************************************
  Description : Remove an Attribute from an Instance.
                When doing many projections, it is more efficient to supply a
  		  schema with the deleted attribute.
  Comments    : For projections, use project().
***************************************************************************/

Instance* 
Instance::remove_attr(int attrNum, const SchemaRC& schemaWithDelAttr) const
{
   if ( attrNum < 0 || attrNum >= num_attr() )
      err << "Instance::remove_attr(int, const SchemaRC&): illegal attrNum " << endl
	  << attrNum << " is passed but the proper range is " << endl
	  << " 0 to " << num_attr() - 1 <<"." << fatal_error;
   
   DBG(schema.equal_except_del_attr(schemaWithDelAttr, attrNum));
   Instance* instance = new Instance(schemaWithDelAttr);
   for (int i = 0; i < schema.num_attr(); i++)
      if (i != attrNum)
	 (*instance)[i - (i > attrNum)] = operator[](i);

   if (schema.is_labelled()) // both are labelled becauseof
			     // equal_except_del_attr 
      instance->set_label(get_label());
   DBG(OK());
   return instance;
}

Instance* 
Instance::remove_attr(int attrNum) const
{
   if ( attrNum < 0 || attrNum >= num_attr() )
      err << "Instance::remove_attr(int): illegal attrNum " << endl
	  << attrNum << " is passed but the proper range is " << endl
	  << " 0 to " << num_attr() - 1 <<"." << fatal_error;

   SchemaRC schemaNew = schema.remove_attr(attrNum);
   return remove_attr(attrNum, schemaNew);
}

 
/***************************************************************************
  Description : Displays the instance.
                The output is sent to the given MLCOStream and is formatted
		  for lines of length lineWidth.
		Displays label value if one exists.
		Display_unlabelled will display the instance without the
  		  label, even if it's labelled.
		ProtectChars protects special characters with backslashes
  Comments    : 
***************************************************************************/
void Instance::display_unlabelled(MLCOStream& stream,
				  Bool protectChars,
				  Bool displayWeight,
				  Bool normalizeReal) const
{
   MString separator(", ");
   if (displayWeight) {
      MString data = MString(weight,DEFAULT_PRECISION);
      stream << weight << separator;
   }
   for (int attrNum = 0; attrNum < schema.num_attr(); attrNum++) {
      const AttrInfo& ai = schema.attr_info(attrNum);
      if (normalizeReal && ai.can_cast_to_real()) {
	 const RealAttrInfo& rai = ai.cast_to_real();
	 if (rai.is_unknown(values[attrNum]))
	    stream << "?" << separator;
	 else
	    stream << rai.normalized_value(values[attrNum]) << separator;
      }
      else {
	 MString data = ai.attrValue_to_string(values[attrNum]);
	 if (ai.can_cast_to_nominal() && protectChars)
	    data = protect_chars(data);
	 stream << data << separator;
      }
   }
}

void Instance::display(MLCOStream& stream,
		       Bool protectChars,
		       Bool displayWeight,
		       Bool normalizeReals) const
{
   display_unlabelled(stream, protectChars, displayWeight, normalizeReals);
   if (is_labelled()) {
      const AttrInfo& ai = schema.label_info();
      MString data = ai.attrValue_to_string(labelValue);
      if (protectChars)
	 data = protect_chars(data);
      stream << data << endOfInstanceLine << endl;
   }
}

/***************************************************************************
  Description : Displays the instance in Buntine format based on the given
                   schema.
                The output is sent to the given MLCOStream and is formatted
		   for lines of length lineWidth.
  Comments    : A string that is longer than lineWidth will be printed
		   on a line by itself.
		Ignores weight.
***************************************************************************/
void Instance::buntine_display(MLCOStream& stream) const
{
   const AttrInfo& ai = schema.label_info();
   MString labelString = ai.attrValue_to_string(labelValue) + " "; 
   stream << labelString;
   MString separator = " ";
   for (int attrNum = 0; attrNum < schema.num_attr(); attrNum++) {
      const AttrInfo& ai = schema.attr_info(attrNum);
      MString data = ai.attrValue_to_string(values[attrNum]) + separator;
      stream << data;
   }
   stream << endl;
}


DEF_DISPLAY(Instance);

/***************************************************************************
  Description : Returns a copy with only the attributes included that are
                  are indicated by the mask.  SchemaRC::project() should be
		  called to create the schema passed in the first argument.
  Comments    : 
***************************************************************************/
Instance* Instance::project(const SchemaRC& shortSchemaRC,
			    const BoolArray& attrMask) const
{
   DBG(OK());
   ASSERT(attrMask.size() == num_attr());

   Instance*  newInst = new Instance(shortSchemaRC);
   int newnum = 0;
   for (int i = 0; i < num_attr(); i++) {
      if (attrMask[i]) {
	 (*newInst)[newnum] = values[i];
	 newnum++;
      }
   }

   newInst->set_label(get_label());
   return newInst;
}
